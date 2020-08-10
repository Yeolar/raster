# Copyright 2017 Yeolar

set(RASTER_PROTOCOL_HTTP_TEST_SRCS
    HTTPHeadersTest.cpp
    UtilTest.cpp
)

foreach(test_src ${RASTER_PROTOCOL_HTTP_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test raster_protocol_http_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test} ${GTEST_BOTH_LIBRARIES} raster)
    add_test(${test} ${test} CONFIGURATIONS ${CMAKE_BUILD_TYPE})
endforeach()

