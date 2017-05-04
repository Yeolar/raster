# Copyright (C) 2017, Yeolar

set(RDD_ENC_SRCS
    CharProperty.cpp
    uconv.cpp
)
add_library(rdd_enc OBJECT ${RDD_ENC_SRCS})

add_executable(CharPropertyBuilder
    CharPropertyBuilder.cpp
)
target_link_libraries(CharPropertyBuilder rdd_static)
