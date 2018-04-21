# Copyright 2018 Yeolar

include(ExternalProject)

set(flatbuffers_URL https://github.com/Yeolar/flatbuffers.git)
set(flatbuffers_TAG 20a400e940634108bad812f571582eeffc728eac)
set(flatbuffers_BUILD ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers/)
set(flatbuffers_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers/src/flatbuffers/include)
set(flatbuffers_STATIC_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/flatbuffers/src/flatbuffers/libflatbuffers.a)

ExternalProject_Add(flatbuffers
    PREFIX flatbuffers
    GIT_REPOSITORY ${flatbuffers_URL}
    GIT_TAG ${flatbuffers_TAG}
    DOWNLOAD_DIR "${DOWNLOAD_LOCATION}"
    BUILD_IN_SOURCE 1
    BUILD_BYPRODUCTS ${flatbuffers_STATIC_LIBRARIES}
    PATCH_COMMAND ""
    INSTALL_COMMAND ""
    CMAKE_CACHE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DBUILD_GMOCK:BOOL=OFF
        -DBUILD_GTEST:BOOL=ON
        -Dgtest_force_shared_crt:BOOL=ON
)
