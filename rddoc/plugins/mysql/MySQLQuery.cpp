/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/plugins/mysql/MySQLQuery.h"

namespace rdd {

MySQLQuery& MySQLQuery::SELECT(const std::vector<std::string>& keys,
                               bool distinct) {
  std::vector<std::string> ks;
  for (auto& k : keys) {
    ks.push_back(escapeKey(k));
  }
  sql_ = to<std::string>("SELECT ", (distinct ? "DISTINCT " : ""),
                         join(',', ks),
                         " FROM ", escapeKey(table_));
  return *this;
}

MySQLQuery& MySQLQuery::SELECT_ALL() {
  sql_ = to<std::string>("SELECT *",
                         " FROM ", escapeKey(table_));
  return *this;
}

MySQLQuery& MySQLQuery::INSERT(const std::map<std::string, std::string>& map) {
  std::vector<std::string> ks;
  std::vector<std::string> vs;
  for (auto& kv : map) {
    ks.push_back(escapeKey(kv.first));
    vs.push_back(kv.second);
  }
  sql_ = to<std::string>("INSERT INTO ", escapeKey(table_),
                         " (", join(',', ks), ")",
                         " VALUES (", join(',', vs), ")");
  return *this;
}

MySQLQuery& MySQLQuery::UPDATE(const std::map<std::string, std::string>& map) {
  std::vector<std::string> ps;
  for (auto& kv : map) {
    ps.push_back(to<std::string>(escapeKey(kv.first), "=", kv.second));
  }
  sql_ = to<std::string>("UPDATE ", escapeKey(table_),
                         " SET ", join(',', ps));
  return *this;
}

MySQLQuery& MySQLQuery::DELETE() {
  sql_ = to<std::string>("DELETE FROM ", escapeKey(table_));
  return *this;
}

MySQLQuery& MySQLQuery::WHERE(const std::string& key,
                              const std::string& condition) {
  sql_ = to<std::string>(sql_, " WHERE ", escapeKey(key), condition);
  return *this;
}

MySQLQuery& MySQLQuery::AND(const std::string& key,
                            const std::string& condition) {
  sql_ = to<std::string>(sql_, " AND ", escapeKey(key), condition);
  return *this;
}

MySQLQuery& MySQLQuery::OR(const std::string& key,
                           const std::string& condition) {
  sql_ = to<std::string>(sql_, " OR ", escapeKey(key), condition);
  return *this;
}

MySQLQuery& MySQLQuery::IN(const std::vector<std::string>& values) {
  sql_ = to<std::string>(sql_, " IN (", join(',', values), ")");
  return *this;
}

MySQLQuery& MySQLQuery::BETWEEN(const std::string& min,
                                const std::string& max) {
  sql_ = to<std::string>(sql_, " BETWEEN ", min, " AND ", max);
  return *this;
}

MySQLQuery& MySQLQuery::LIKE(const std::string& pattern) {
  sql_ = to<std::string>(sql_, " LIKE ", pattern);
  return *this;
}

MySQLQuery& MySQLQuery::NOT() {
  sql_ = to<std::string>(sql_, " NOT");
  return *this;
}

MySQLQuery& MySQLQuery::LIMIT(size_t n) {
  sql_ = to<std::string>(sql_, " LIMIT ", n);
  return *this;
}

} // namespace rdd
