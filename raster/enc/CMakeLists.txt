# Copyright (C) 2017, Yeolar

set(RASTER_ENC_SRCS
    CharProperty.cpp
    uconv.cpp
)
add_library(raster_enc OBJECT ${RASTER_ENC_SRCS})

add_executable(CharPropertyBuilder
    CharPropertyBuilder.cpp
)
target_link_libraries(CharPropertyBuilder raster_static)
