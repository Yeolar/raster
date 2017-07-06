# Copyright (C) 2017, Yeolar

set(RDD_UTIL_TEST_SRCS
    ArenaTest.cpp
    AutoTaskTest.cpp
    Base64Test.cpp
    ChecksumTest.cpp
    FixedStreamTest.cpp
    HashTest.cpp
    LoggingTest.cpp
    ReflectObjectTest.cpp
    ShaTest.cpp
    SingletonTest.cpp
    StringTest.cpp
    ThreadUtilTest.cpp
    TimedHeapTest.cpp
    TraitsTest.cpp
    UnicodeTest.cpp
)

foreach(test_src ${RDD_UTIL_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test rdd_util_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test}
        ${GTEST_BOTH_LIBRARIES} ${GMOCK_LIBRARIES} rdd_static)
    add_test(${test} ${test} CONFIGURATIONS Debug)
endforeach()
