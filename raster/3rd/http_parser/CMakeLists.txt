#
# Copyright 2015 Yeolar
#

set(HTTPPARSER_SRCS
    http_parser_cpp.cpp
)
add_library(httpparser OBJECT ${HTTPPARSER_SRCS})

add_executable(httpparser_test
    test.c
    http_parser.c
)
add_test(httpparser_test httpparser_test CONFIGURATIONS ${CMAKE_BUILD_TYPE})

