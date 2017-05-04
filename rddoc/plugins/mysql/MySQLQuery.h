/*
 * Copyright (C) 2017, Yeolar
 */

#pragma once

#include <map>
#include <string>
#include <vector>
#include "rddoc/util/String.h"

namespace rdd {

class MySQLQuery {
public:
  static std::string escapeKey(const std::string& key) {
    return to<std::string>('`', key, '`');
  }

  static std::string escapeValue(const std::string& value) {
    std::string escaped;
    escaped.resize(value.length());
    for (auto& c : value) {
      if (c == '\\' || c == '\'') {
        escaped.push_back('\\');
      }
      escaped.push_back(c);
    }
    return escaped;
  }

  MySQLQuery(const std::string& table) : table_(table) {}

  std::string str() const {
    return sql_ + ";";
  }

  /*
   * main
   */

  MySQLQuery& SELECT(const std::vector<std::string>& keys,
                     bool distinct = false);
  MySQLQuery& SELECT_ALL();
  MySQLQuery& INSERT(const std::map<std::string, std::string>& map);
  MySQLQuery& UPDATE(const std::map<std::string, std::string>& map);
  MySQLQuery& DELETE();

  /*
   * condition
   */

  MySQLQuery& WHERE(const std::string& key, const std::string& condition = "");
  MySQLQuery& AND(const std::string& key, const std::string& condition);
  MySQLQuery& OR(const std::string& key, const std::string& condition);
  MySQLQuery& IN(const std::vector<std::string>& values);
  MySQLQuery& BETWEEN(const std::string& min, const std::string& max);
  MySQLQuery& LIKE(const std::string& pattern);
  MySQLQuery& NOT();
  MySQLQuery& LIMIT(size_t n);

private:
  std::string table_;
  std::string sql_;
};

}

