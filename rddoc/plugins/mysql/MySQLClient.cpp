/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/plugins/mysql/MySQLClient.h"
#include "rddoc/util/Logging.h"
#include "rddoc/util/String.h"

namespace rdd {

bool MySQLClient::connect() {
  if (!mysql_real_connect(&conn_,
                          option_.host.c_str(),
                          option_.user.c_str(),
                          option_.pass.c_str(),
                          option_.db.c_str(),
                          option_.port,
                          nullptr, 0)) {
    RDDLOG(ERROR) << "mysql: connect " << uri() << " error: "
      << mysql_error(&conn_);
    return false;
  }
  if (mysql_query(&conn_, "set names utf8") != 0) {
    RDDLOG(ERROR) << "mysql: query \"set names utf8\" error: "
      << mysql_error(&conn_);
    return false;
  }
  return true;
}

bool MySQLClient::query(const std::string& sql) {
  if (mysql_query(&conn_, sql.c_str()) != 0) {
    RDDLOG(ERROR) << "mysql: query \"" << shortQuery(sql) << "\" error: "
      << mysql_error(&conn_);
    return false;
  }
  return true;
}

bool MySQLClient::fetch(const std::string& sql, std::vector<Column>& table) {
  if (!query(sql)) {
    return false;
  }
  MYSQL_RES* res = mysql_store_result(&conn_);
  if (!res) {
    RDDLOG(ERROR) << "msql: fetch result error: " << mysql_error(&conn_);
    return false;
  }
  size_t col = mysql_num_fields(res);
  size_t row = mysql_num_rows(res);
  table.resize(col);

  MYSQL_FIELD* field;
  for (size_t i = 0; (field = mysql_fetch_field(res)); i++) {
    table[i].name = field->name;
    table[i].type = field->type;
    table[i].data.resize(row);
  }
  for (size_t i = 0; i < row; i++) {
    MYSQL_ROW result_row = mysql_fetch_row(res);
    for (size_t j = 0; j < col; j++) {
      table[j].data[i] = result_row[j] ?: "";
    }
  }
  mysql_free_result(res);
  return true;
}

} // namespace rdd
