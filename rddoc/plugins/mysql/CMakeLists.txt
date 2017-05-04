# Copyright (C) 2017, Yeolar

set(RDD_PLUGINS_MYSQL_SRCS
    MySQLClient.cpp
    MySQLQuery.cpp
    Query.cpp
)
add_library(rdd_plugins_mysql OBJECT ${RDD_PLUGINS_MYSQL_SRCS})
