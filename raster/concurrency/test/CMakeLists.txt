# Copyright 2017 Yeolar

set(ACCELERATOR_CONCURRENCY_TEST_SRCS
    ThreadPoolExecutorTest.cpp
)

foreach(test_src ${ACCELERATOR_CONCURRENCY_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test accelerator_concurrency_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test} ${GTEST_BOTH_LIBRARIES} accelerator_static)
    add_test(${test} ${test} CONFIGURATIONS ${CMAKE_BUILD_TYPE})
endforeach()
