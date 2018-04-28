# Copyright 2017 Yeolar

set(RASTER_SERIALIZER_TEST_SRCS
    SerializerTest.cpp
)

foreach(test_src ${RASTER_SERIALIZER_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test raster_serializer_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test} ${GTEST_BOTH_LIBRARIES} raster_static)
    add_test(${test} ${test} CONFIGURATIONS ${CMAKE_BUILD_TYPE})
endforeach()
