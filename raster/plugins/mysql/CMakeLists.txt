# Copyright (C) 2017, Yeolar

set(RASTER_PLUGINS_MYSQL_SRCS
    MySQLClient.cpp
    MySQLQuery.cpp
    Query.cpp
)
add_library(raster_plugins_mysql OBJECT ${RASTER_PLUGINS_MYSQL_SRCS})
