/*
 * Copyright (C) 2017, Yeolar
 */

#include "rddoc/plugins/mysql/MySQLClient.h"
#include <gtest/gtest.h>
#include "rddoc/util/Logging.h"
#include "rddoc/util/String.h"

using namespace rdd;

TEST(MySQLClient, fetch) {
  std::vector<MySQLClient::Column> table;

  /*
  MySQLClient mysql(...);
  mysql.connect();
  mysql.fetch("select * from doc_pages limit 10", table);
  for (auto& col : table) {
    RDDLOG(INFO)
      << " name: " << col.name
      << " type: " << col.type
      << " data: " << join(" | ", col.data);
  }
  */
}
