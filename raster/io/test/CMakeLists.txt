# Copyright (C) 2017, Yeolar

set(RASTER_IO_TEST_SRCS
    FileTest.cpp
    FlatDictTest.cpp
    FSUtilTest.cpp
    IOBufCursorTest.cpp
    IOBufTest.cpp
    PathTest.cpp
)

foreach(test_src ${RASTER_IO_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test raster_io_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test} ${GTEST_BOTH_LIBRARIES} raster_static)
    add_test(${test} ${test} CONFIGURATIONS Debug)
endforeach()

set(RASTER_IO_BENCHMARK_SRCS
    FlatDictBenchmark.cpp
)

foreach(bench_src ${RASTER_IO_BENCHMARK_SRCS})
    get_filename_component(bench_name ${bench_src} NAME_WE)
    set(bench raster_io_${bench_name})
    add_executable(${bench} ${bench_src})
    target_link_libraries(${bench} raster_static)
endforeach()

