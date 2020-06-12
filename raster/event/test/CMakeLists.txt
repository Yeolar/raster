# Copyright 2017-present Yeolar

set(RASTER_EVENT_TEST_SRCS
    PollTest.cpp
    SelectPollTest.cpp
)

foreach(test_src ${RASTER_EVENT_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test raster_event_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test} ${GTEST_BOTH_LIBRARIES} raster_static)
    add_test(${test} ${test} CONFIGURATIONS ${CMAKE_BUILD_TYPE})
endforeach()

set(RASTER_EVENT_BENCHMARK_SRCS
)

foreach(bench_src ${RASTER_EVENT_BENCHMARK_SRCS})
    get_filename_component(bench_name ${bench_src} NAME_WE)
    set(bench raster_event_${bench_name})
    add_executable(${bench} ${bench_src})
    target_link_libraries(${bench} raster_static)
endforeach()

