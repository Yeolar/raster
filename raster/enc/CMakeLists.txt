# Copyright (C) 2017, Yeolar

set(RASTER_ENC_SRCS
    CharProperty.cpp
    uconv.cpp
)
add_library(raster_enc OBJECT ${RASTER_ENC_SRCS})

install(FILES
    CharProperty.h
    cp936map.h
    psedoutf8.h
    uconv.h
    un2gbkmap.h
    DESTINATION include/raster/enc)

add_executable(CharPropertyBuilder
    CharPropertyBuilder.cpp
)
target_link_libraries(CharPropertyBuilder raster_static)
