# Find flatbuffers
#
#  FLATBUFFERS_INCLUDE_DIR - where to find flatbuffers/*.
#  FLATBUFFERS_LIBRARY     - List of libraries when using flatbuffers.
#  FLATBUFFERS_FOUND       - True if flatbuffers found.

find_package(PkgConfig)
pkg_check_modules(FLATBUFFERS QUIET flatbuffers)

if(FLATBUFFERS_INCLUDE_DIR)
  # Already in cache, be silent
  set(FLATBUFFERS_FIND_QUIETLY TRUE)
endif()

find_path(FLATBUFFERS_INCLUDE_DIR flatbuffers/flatbuffers.h PATHS ${PROJECT_BINARY_DIR}/flatbuffers/flatbuffers/include)
find_library(FLATBUFFERS_LIBRARY flatbuffers PATHS ${PROJECT_BINARY_DIR}/flatbuffers/flatbuffers/lib)

# handle the QUIETLY and REQUIRED arguments and set FLATBUFFERS_FOUND to TRUE 
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FLATBUFFERS DEFAULT_MSG FLATBUFFERS_LIBRARY FLATBUFFERS_INCLUDE_DIR)

mark_as_advanced(FLATBUFFERS_LIBRARY FLATBUFFERS_INCLUDE_DIR)
