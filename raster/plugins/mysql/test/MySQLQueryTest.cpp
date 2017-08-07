/*
 * Copyright (C) 2017, Yeolar
 */

#include "raster/plugins/mysql/MySQLQuery.h"
#include <gtest/gtest.h>
#include "raster/util/Logging.h"
#include "raster/util/String.h"

using namespace rdd;

TEST(MySQLQuery, select) {
  EXPECT_STREQ("SELECT `Store_Name` FROM `Store_Information`;",
               MySQLQuery("Store_Information")
               .SELECT({"Store_Name"})
               .str().c_str());
  EXPECT_STREQ("SELECT `Store_Name`,`Sales` FROM `Store_Information`;",
               MySQLQuery("Store_Information")
               .SELECT({"Store_Name", "Sales"})
               .str().c_str());
  EXPECT_STREQ("SELECT * FROM `Store_Information`;",
               MySQLQuery("Store_Information")
               .SELECT_ALL()
               .str().c_str());
  EXPECT_STREQ("SELECT DISTINCT `Store_Name` FROM `Store_Information`;",
               MySQLQuery("Store_Information")
               .SELECT({"Store_Name"}, true)
               .str().c_str());
  EXPECT_STREQ("SELECT `Store_Name` FROM `Store_Information`"
               " WHERE `Sales`>1000;",
               MySQLQuery("Store_Information")
               .SELECT({"Store_Name"})
               .WHERE("Sales", ">1000")
               .str().c_str());
  EXPECT_STREQ("SELECT `Store_Name` FROM `Store_Information`"
               " WHERE `Sales`>1000 OR `Sales`<500;",
               MySQLQuery("Store_Information")
               .SELECT({"Store_Name"})
               .WHERE("Sales", ">1000")
               .OR("Sales", "<500")
               .str().c_str());
  EXPECT_STREQ("SELECT * FROM `Store_Information`"
               " WHERE `Store_Name` IN ('Los Angeles','San Diego');",
               MySQLQuery("Store_Information")
               .SELECT_ALL()
               .WHERE("Store_Name")
               .IN({"'Los Angeles'", "'San Diego'"})
               .str().c_str());
  EXPECT_STREQ("SELECT * FROM `Store_Information`"
               " WHERE `Sales` NOT BETWEEN 280 AND 1000;",
               MySQLQuery("Store_Information")
               .SELECT_ALL()
               .WHERE("Sales")
               .NOT()
               .BETWEEN("280", "1000")
               .str().c_str());
  EXPECT_STREQ("SELECT * FROM `Store_Information`"
               " WHERE `Store_Name` LIKE '%AN%';",
               MySQLQuery("Store_Information")
               .SELECT_ALL()
               .WHERE("Store_Name")
               .LIKE("'%AN%'")
               .str().c_str());
}

TEST(MySQLQuery, insert) {
  EXPECT_STREQ("INSERT INTO `Store_Information`"
               " (`Manager_ID`,`Sales`,`Store_Name`,`Txn_Date`)"
               " VALUES (10,900,'Los Angeles','Jan-10-1999');",
               MySQLQuery("Store_Information")
               .INSERT({
                       {"Store_Name", "'Los Angeles'"},
                       {"Manager_ID", "10"},
                       {"Sales", "900"},
                       {"Txn_Date", "'Jan-10-1999'"}})
               .str().c_str());
}

TEST(MySQLQuery, update) {
  EXPECT_STREQ("UPDATE `Store_Information`"
               " SET `Sales`=600,`Txn_Date`='Jan-15-1999'"
               " WHERE `Store_Name`='San Diego';",
               MySQLQuery("Store_Information")
               .UPDATE({{"Sales", "600"}, {"Txn_Date", "'Jan-15-1999'"}})
               .WHERE("Store_Name", "='San Diego'")
               .str().c_str());
}

TEST(MySQLQuery, del) {
  EXPECT_STREQ("DELETE FROM `Store_Information`"
               " WHERE `Store_Name`='Los Angeles';",
               MySQLQuery("Store_Information")
               .DELETE()
               .WHERE("Store_Name", "='Los Angeles'")
               .str().c_str());
}

