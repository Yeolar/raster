/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <algorithm>
#include <string>
#include <mysql/mysql.h>
#include "rddoc/plugins/mysql/MySQLQuery.h"
#include "rddoc/util/Conv.h"

namespace rdd {

inline std::string shortQuery(const std::string& query, size_t limit = 100) {
  size_t n = std::max(3ul, limit) - 3;
  return query.length() > limit ? query.substr(0, n) + " ..." : query;
}

class MySQLClient {
public:
  struct Option {
    std::string host;
    int port;
    std::string user;
    std::string pass;
    std::string db;
  };

  struct Column {
    std::string name;
    int type;
    std::vector<std::string> data;
  };

  static void libraryInit() {
    mysql_library_init(0, nullptr, nullptr);
  }

  static void libraryEnd() {
    mysql_library_end();
  }

  MySQLClient(const std::string& host, int port,
              const std::string& user, const std::string& pass,
              const std::string& db)
    : option_({host, port, user, pass, db}) {
    init();
  }

  MySQLClient(const Option& option)
    : option_(option) {
    init();
  }

  ~MySQLClient() {
    close();
  }

  std::string uri() const {
    return to<std::string>(
      "mysql://", option_.user.c_str(), "@",
      option_.host.c_str(), ":", option_.port,
      "/", option_.db.c_str());
  }

  bool connect();

  // any SQL
  bool query(const std::string& sql);
  bool query(const MySQLQuery& q) {
    return query(q.str());
  }

  // SELECT
  bool fetch(const std::string& sql, std::vector<Column>& table);
  bool fetch(const MySQLQuery& q, std::vector<Column>& table) {
    return fetch(q.str(), table);
  }

private:
  void init() {
    mysql_init(&conn_);
  }

  void close() {
    mysql_close(&conn_);
  }

  MYSQL conn_;
  Option option_;
};

}

