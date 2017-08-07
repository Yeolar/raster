/*
 * Copyright (C) 2017, Yeolar
 */

//
// This class represents queries to execute against a MySQL database.
//
// DO NOT ENCODE SQL VALUES DIRECTLY.  That's evil.  The library will
// try to prevent this kind of thing.  All values for where clauses,
// inserts, etc should be parameterized via the encoding methods
// below.  This is will make your code more robust and reliable while
// also avoiding common security issues.
//
// Usage is simple; construct the query using special printf-like
// markup, provide parameters for the substitution, and then hand to
// the database libraries.  Alternatively, you can call the render()
// method to see the actual SQL it would run.
//
// Example:
//
// Query q("SELECT foo, bar FROM Table WHERE id = %d", 17);
// LOG(INFO) << "query: " << q.render();
//
// dynamic condition(dynamic::object("id1", 7)("id2", 14));
// Query q("SELECT %LC FROM %T WHERE %W",
//         dynamic({"id1_type", "data"}),
//         "assoc_info", condition);
// auto op = Connection::beginQuery(std::move(conn), q);
//
// Values for substitution into the query should be dynamic
// values (or convertible to them).  Composite values expected by some
// codes such as %W, %U, etc, are also dynamic objects that
// have array or map values.
//
// Codes:
//
// %s, %d, %f - strings, integers, or floats; NULL if a nullptr is
//              passed in.
// %m - dynamic, gets converted to string/integer/float,
//      throws otherwise
// %=s, %=d, %=f - like the previous except suitable for comparison,
//                 so "%s" becomes " = VALUE".  nulls become "IS NULL"
// %T - a table name.  enclosed with ``.
// %C - like %T, except for column names. Optionally supply two-/three-tuple
//      to define qualified column name or qualified column name with
//      an alias. `QualifiedColumn{"table_name", "column_name"}` will become
//      "`table_name`.`column_name`" and
//      `AliasedQualifiedColumn{"table_name", "column_name", "alias"}`
//      will become "`table_name`.`column_name` AS `alias`"
// %V - VALUES style row list; expects a list of lists, each of the same
//      length.
// %Ls, %Ld, %Lf - strings/ints/floats separated by commas
// %LC - list of column names separated by commas. Optionally supplied as
//       a list of two-/three-tuples to define qualified column names or
//       qualified column names with aliases. Similar to %C.
// %LO, %LA - key/value pair rendered as key1=val1 OR/AND key2=val2 (similar
//            to %W)
// %U, %W - keys and values suitable for UPDATE and WHERE clauses,
//          respectively.  %U becomes "`col1` = val1, `col2` = val2"
//          and %W becomes "`col1` = val1 AND `col2` = val2"
// %Q - literal string, evil evil.  don't use.
// %K - an SQL comment.  Will put the /* and */ for you.
//
// For more details, check out queryfx in the www codebase.

#pragma once

#include "raster/util/dynamic.h"
#include "raster/util/Exception.h"
#include "raster/util/Memory.h"
#include "raster/util/Optional.h"
#include "raster/util/Range.h"
#include "raster/util/String.h"

#include <string>
#include <tuple>
#include <boost/variant.hpp>
#include <mysql/mysql.h>

namespace rdd {

using QualifiedColumn = std::tuple<std::string, std::string>;
using AliasedQualifiedColumn =
  std::tuple<std::string, std::string, std::string>;

class QueryArgument;

class Query {
 struct QueryText;
 public:
  // Query can be constructed with or without params.
  // By default we deep copy the query text
  explicit Query(const StringPiece query_text)
      : query_text_(query_text) {}

  explicit Query(QueryText&& query_text)
      : query_text_(std::move(query_text)) {}

  ~Query();

  // default copy and move constructible
  Query(const Query&) = default;
  Query(Query&&) = default;

  Query& operator=(const Query&) = default;
  Query& operator=(Query&&) = default;

  // Parameters will be coerced into dynamic.
  template <typename... Args>
  Query(const StringPiece query_text, Args&&... args);
  Query(const StringPiece query_text, std::vector<QueryArgument> params);

  void append(const Query& query2);
  void append(Query&& query2);

  Query& operator+=(const Query& query2) {
    append(query2);
    return *this;
  }

  Query& operator+=(Query&& query2) {
    append(std::move(query2));
    return *this;
  }

  Query operator+(const Query& query2) const {
    Query ret(*this);
    ret.append(query2);
    return ret;
  }

  // If you need to construct a raw query, use this evil function.
  static Query unsafe(const StringPiece query_text,
                      bool shallowCopy = false) {
    Query ret{shallowCopy ? QueryText::makeShallow(query_text)
                          : QueryText{query_text}};
    ret.allowUnsafeEvilQueries();
    return ret;
  }

  bool isUnsafe() const noexcept {
    return unsafe_query_;
  }

  // Wrapper around mysql_real_escape_string() - please use placeholders
  // instead.
  //
  // This is provided so that non-Facebook users of the HHVM extension have
  // a familiar API.
  template <typename string>
  static string escapeString(MYSQL* conn, const string& unescaped) {
    string escaped;
    escaped.resize((2 * unescaped.size()) + 1);
    size_t escaped_size = mysql_real_escape_string(
        conn, &escaped[0], unescaped.data(), unescaped.size());
    escaped.resize(escaped_size);
    return escaped;
  }

  static std::string renderMultiQuery(
      MYSQL* conn,
      const std::vector<Query>& queries);

  // render either with the parameters to the constructor or specified
  // ones.
  std::string render(MYSQL* conn) const;
  std::string render(MYSQL* conn, const std::vector<QueryArgument>& params)
      const;

  // render either with the parameters to the constructor or specified
  // ones.  This is mainly for testing as it does not properly escape
  // the MySQL strings.
  std::string renderInsecure() const;
  std::string renderInsecure(
      const std::vector<QueryArgument>& params) const;

  StringPiece getQueryFormat() const { return query_text_.getQuery(); }

 private:

  // QueryText is a container for query stmt used by the Query (see below).
  // Its a union like structure that supports managing either a shallow copy
  // or a deep copy of a query stmt. If QueryText holds a shallow reference
  // and a modification is requested, it will automatically copy the data
  // before modifying the data.
  //
  // Invariants:
  // sp -> string piece field representing the query stmt
  // sb -> string buffer that contains the query if deep copy
  //
  // if shallow copy, sb is empty and sp point to the query stmt
  // if deep copy, sb has the query stmt and sp points to sb
  struct QueryText {
    // By default make a deep copy of the query
    explicit QueryText(StringPiece query) {
      query_buffer_.assign(std::string(query.begin(), query.size()));
      query_ = StringPiece(*query_buffer_);
      sanityChecks();
    }

    // Make a shallow copy of the query
    static QueryText makeShallow(StringPiece query) {
      QueryText res{};
      res.query_ = query;
      res.sanityChecks();
      return res;
    }

    // Copy constructor and copy assignment
    QueryText(const QueryText& other) {
      *this = other;
    }
    QueryText& operator=(const QueryText& other) {
      if (this == &other) {
        return *this;
      }
      if (!other.query_buffer_.hasValue()) {
        /* shallow copy string */
        query_buffer_.clear();
        query_ = other.query_;
      } else {
        query_buffer_ = other.query_buffer_;
        query_ = StringPiece(*query_buffer_);
      }
      sanityChecks();
      return *this;
    }

    /// Move constructor and move assignment
    QueryText(QueryText&& other) noexcept {
      *this = std::move(other);
    }
    QueryText& operator=(QueryText&& other) {
      if (this == &other) {
        return *this;
      }
      if (!other.query_buffer_.hasValue()) {
        /* shallow copy */
        query_buffer_.clear();
        query_ = other.query_;
      } else {
        query_buffer_ = std::move(other.query_buffer_);
        query_ = StringPiece(*query_buffer_);
        other.query_ = {};
        other.query_buffer_.clear();
      }
      sanityChecks();
      return *this;
    }

    QueryText& operator+=(const QueryText& other) {
      if (!query_buffer_.hasValue()) {
        // this was a shallow copy before; we need to copy now
        query_buffer_.assign(std::string(query_.begin(), query_.size()));
      }
      DCHECK_EQ(query_, *query_buffer_);
      *query_buffer_ += " ";
      *query_buffer_ += other.getQuery().toString();
      query_ = StringPiece(*query_buffer_);
      sanityChecks();
      return *this;
    }

    StringPiece getQuery() const noexcept {
      return query_;
    }

   private:
    QueryText() {}

    // ensures invariants are met
    void sanityChecks() {
      if (!query_buffer_.hasValue()) {
        /* shallow copy */
        return;
      }
      DCHECK_EQ((uintptr_t)query_.data(), (uintptr_t)query_buffer_->data());
      DCHECK_EQ(query_.size(), query_buffer_->length());
    }

    Optional<std::string> query_buffer_;
    StringPiece query_;
  };  // end QueryText class

  // Allow queries that look evil (aka, raw queries).  Don't use this.
  // It's horrible.
  void allowUnsafeEvilQueries() {
    unsafe_query_ = true;
  }

  // append an int, float, or string to the specified buffer
  void appendValue(
      std::string* s,
      size_t offset,
      char type,
      const QueryArgument& d,
      MYSQL* conn) const;

  // append a dynamic::object param as key=value joined with sep;
  // values are passed to appendValue
  void appendValueClauses(
      std::string* ret,
      size_t* idx,
      const char* sep,
      const QueryArgument& param,
      MYSQL* connection) const;

  template <typename Arg, typename... Args>
  void unpack(Arg&& arg, Args&&... args);
  void unpack() {}

  QueryText query_text_;
  bool unsafe_query_ = false;
  std::vector<QueryArgument> params_{};
};

// Wraps many queries and holds a buffer that contains the rendered multi query
// from all the subqueries.
class MultiQuery {
 public:
  explicit MultiQuery(std::vector<Query>&& queries)
      : queries_(std::move(queries)) {}

  // Construct an unsafe multi query.
  // Caller must guarantee the lifetime of the string
  static MultiQuery unsafe(StringPiece multi_query) {
    return MultiQuery{multi_query};
  }

  StringPiece renderQuery(MYSQL* conn);

  const Query& getQuery(size_t index) const {
    CHECK_THROW(index < queries_.size(), std::invalid_argument);
    return queries_[index];
  }

 private:

  explicit MultiQuery(StringPiece multi_query)
      : unsafe_multi_query_(multi_query) {}

  StringPiece unsafe_multi_query_;
  std::string rendered_multi_query_;
  std::vector<Query> queries_;
};

class QueryArgument {
 private:
  boost::variant<
      int64_t,
      double,
      bool,
      std::string,
      std::nullptr_t,
      Query,
      std::vector<QueryArgument>,
      std::vector<std::pair<std::string, QueryArgument>>,
      std::tuple<std::string, std::string>,
      std::tuple<std::string, std::string, std::string>>
      value_;

 public:
  /* implicit */ QueryArgument(StringPiece val);
  /* implicit */ QueryArgument(char const* val);
  /* implicit */ QueryArgument(const std::string& val);
  /* implicit */ QueryArgument(std::string&& val);
  /* implicit */ QueryArgument(Query q);

  template <
      typename T,
      typename std::enable_if<std::is_integral<T>::value, T>::type = 0>
  /* implicit */ QueryArgument(T int_val)
      : value_(static_cast<int64_t>(int_val)) {}
  template <
      typename T,
      typename = typename std::enable_if<std::is_enum<T>::value, T>::type>
  /* implicit */ QueryArgument(T enum_val)
      : value_(static_cast<int64_t>(enum_val)) {}
  /* implicit */ QueryArgument(double double_val);

  /* implicit */ QueryArgument(std::initializer_list<QueryArgument> list);
  /* implicit */ QueryArgument(std::vector<QueryArgument> arg_list);
  /* implicit */ QueryArgument(std::tuple<std::string, std::string> tup)
      : value_(tup) {}
  /* implicit */ QueryArgument(
        std::tuple<std::string, std::string, std::string> tup
      )
      : value_(tup) {}
  /* implicit */ QueryArgument(std::nullptr_t n) : value_(n) {}

  // Pair constructors
  QueryArgument();
  QueryArgument(StringPiece param1, QueryArgument param2);

  // Since we already have callsites that use dynamic, we are keeping the
  // support, but internally we unpack them.
  // This constructor will throw exception if the dynamic isn't acceptable
  /* implicit */ QueryArgument(const dynamic& param) {
    initFromDynamic(param);
  }
  /* implicit */ QueryArgument(dynamic&& param) {
    initFromDynamic(param);
  }

  QueryArgument&& operator()(StringPiece q1, const QueryArgument& q2);
  QueryArgument&& operator()(std::string&& q1, QueryArgument&& q2);
  std::string asString() const;

  double getDouble() const;
  int64_t getInt() const;
  bool getBool() const;
  const Query& getQuery() const;
  const std::string& getString() const;
  const std::vector<std::pair<std::string, QueryArgument>>& getPairs()
      const;
  const std::vector<QueryArgument>& getList() const;
  const std::tuple<std::string, std::string>& getTwoTuple() const;
  const std::tuple<std::string, std::string, std::string>&
      getThreeTuple() const;

  bool isString() const;
  bool isQuery() const;
  bool isPairList() const;
  bool isBool() const;
  bool isNull() const;
  bool isList() const;
  bool isDouble() const;
  bool isInt() const;
  bool isTwoTuple() const;
  bool isThreeTuple() const;

  std::string typeName() const {
    return value_.type().name();
  }

 private:
  void initFromDynamic(const dynamic& dyn);
  std::vector<std::pair<std::string, QueryArgument>>& getPairs();
};

template <typename... Args>
Query::Query(const StringPiece query_text, Args&&... args)
    : query_text_(query_text),
      unsafe_query_(false),
      params_() {
  params_.reserve(sizeof...(args));
  unpack(std::forward<Args>(args)...);
}
template <typename Arg, typename... Args>
void Query::unpack(Arg&& arg, Args&&... args) {
  params_.emplace_back(std::forward<Arg>(arg));
  unpack(std::forward<Args>(args)...);
}

} // namespace rdd
