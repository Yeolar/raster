# Copyright (C) 2017, Yeolar

set(RASTER_IO_EVENT_TEST_SRCS
    PollTest.cpp
)

foreach(test_src ${RASTER_IO_EVENT_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test raster_io_event_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test} ${GTEST_BOTH_LIBRARIES} raster_static)
    add_test(${test} ${test} CONFIGURATIONS Debug)
endforeach()
