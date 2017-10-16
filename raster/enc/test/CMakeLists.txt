# Copyright (C) 2017, Yeolar

set(RASTER_ENC_TEST_SRCS
    CharPropertyTest.cpp
)

foreach(test_src ${RASTER_ENC_TEST_SRCS})
    get_filename_component(test_name ${test_src} NAME_WE)
    set(test raster_enc_${test_name})
    add_executable(${test} ${test_src})
    target_link_libraries(${test} ${GTEST_BOTH_LIBRARIES} raster_static)
    add_test(${test} ${test} CONFIGURATIONS Debug)
endforeach()

execute_process(
    COMMAND ln -fs "${PROJECT_SOURCE_DIR}/raster/enc/test/char.pecker.def"
                   "${PROJECT_BINARY_DIR}/raster/enc/test/char.pecker.def"
)
