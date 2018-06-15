# Find raster
#
#  RASTER_INCLUDE_DIR - where to find raster/*.
#  RASTER_LIBRARY     - List of libraries when using raster.
#  RASTER_FOUND       - True if raster found.

find_package(PkgConfig)
pkg_check_modules(RASTER QUIET raster)

if(RASTER_INCLUDE_DIR)
  # Already in cache, be silent
  set(RASTER_FIND_QUIETLY TRUE)
endif()

find_path(RASTER_INCLUDE_DIR raster/raster-config.h PATHS
    ${PROJECT_BINARY_DIR}/raster/raster/include
    ${PROJECT_BINARY_DIR}-deps/raster/raster/include)
find_library(RASTER_LIBRARY raster PATHS
    ${PROJECT_BINARY_DIR}/raster/raster/lib
    ${PROJECT_BINARY_DIR}-deps/raster/raster/lib)

# handle the QUIETLY and REQUIRED arguments and set RASTER_FOUND to TRUE 
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RASTER DEFAULT_MSG RASTER_LIBRARY RASTER_INCLUDE_DIR)

mark_as_advanced(RASTER_LIBRARY RASTER_INCLUDE_DIR)
