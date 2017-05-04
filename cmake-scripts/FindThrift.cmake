# Find thrift
#
#  THRIFT_INCLUDE_DIR - where to find thrift/Thrift.h, etc.
#  THRIFT_LIBRARY     - List of libraries when using thrift.
#  THRIFT_FOUND       - True if thrift found.

find_package(PkgConfig)
pkg_check_modules(THRIFT QUIET thrift)

IF (THRIFT_INCLUDE_DIR)
  # Already in cache, be silent
  SET(THRIFT_FIND_QUIETLY TRUE)
ENDIF ()

FIND_PATH(THRIFT_INCLUDE_DIR thrift/Thrift.h)

FIND_LIBRARY(THRIFT_LIBRARY thrift)

# handle the QUIETLY and REQUIRED arguments and set thrift_FOUND to TRUE 
# if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(THRIFT DEFAULT_MSG THRIFT_LIBRARY THRIFT_INCLUDE_DIR)

SET(THRIFT_VERSION 0)

IF(THRIFT_FOUND)
  if (EXISTS "${THRIFT_INCLUDE_DIR}/thrift/config.h")
    FILE(READ "${THRIFT_INCLUDE_DIR}/thrift/config.h" _THRIFT_VERSION_CONTENTS)
  endif()
  if (_THRIFT_VERSION_CONTENTS)
    STRING(REGEX REPLACE ".*#define VERSION \"([0-9.]+)\".*" "\\1" THRIFT_VERSION "${_THRIFT_VERSION_CONTENTS}")
  endif()
ENDIF()

SET(THRIFT_VERSION ${THRIFT_VERSION} CACHE STRING "Version number of thrift")

MARK_AS_ADVANCED(THRIFT_LIBRARY THRIFT_INCLUDE_DIR THRIFT_VERSION)
