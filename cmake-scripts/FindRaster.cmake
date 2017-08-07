# Find raster
#
#  RASTER_INCLUDE_DIR - where to find raster/*.
#  RASTER_LIBRARY     - List of libraries when using raster.
#  RASTER_FOUND       - True if raster found.

find_package(PkgConfig)
pkg_check_modules(RASTER QUIET raster)

IF (RASTER_INCLUDE_DIR)
  # Already in cache, be silent
  SET(RASTER_FIND_QUIETLY TRUE)
ENDIF ()

FIND_PATH(RASTER_INCLUDE_DIR raster/framework/Config.h)

FIND_LIBRARY(RASTER_LIBRARY raster)

# handle the QUIETLY and REQUIRED arguments and set RASTER_FOUND to TRUE 
# if all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RASTER DEFAULT_MSG RASTER_LIBRARY RASTER_INCLUDE_DIR)

MARK_AS_ADVANCED(RASTER_LIBRARY RASTER_INCLUDE_DIR)
