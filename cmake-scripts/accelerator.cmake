# Copyright 2018 Yeolar

include(ExternalProject)

set(accelerator_URL https://github.com/Yeolar/accelerator.git)
set(accelerator_TAG v1.2.0)
set(accelerator_BUILD ${CMAKE_CURRENT_BINARY_DIR}/accelerator/)
set(accelerator_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/accelerator/src/accelerator)
set(accelerator_STATIC_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/accelerator/src/accelerator/libaccelerator.a)

ExternalProject_Add(accelerator
    PREFIX accelerator
    GIT_REPOSITORY ${accelerator_URL}
    GIT_TAG ${accelerator_TAG}
    DOWNLOAD_DIR "${DOWNLOAD_LOCATION}"
    BUILD_IN_SOURCE 1
    BUILD_BYPRODUCTS ${accelerator_STATIC_LIBRARIES}
    PATCH_COMMAND ""
    INSTALL_COMMAND ""
    CMAKE_CACHE_ARGS
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        -DBUILD_GMOCK:BOOL=OFF
        -DBUILD_GTEST:BOOL=ON
        -Dgtest_force_shared_crt:BOOL=ON
)
