# - Try to find Guile
# Once done this will define
#  GUILE_FOUND - System has Guile
#  GUILE_INCLUDE_DIRS - The Guile include directories
#  GUILE_LIBRARIES - The libraries needed to use Guile
#  GUILE_VERSION - The version of Guile found
#  GUILE_EXECUTABLE - The Guile executable

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_GUILE QUIET guile-3.0)
endif()

find_path(GUILE_INCLUDE_DIR
  NAMES libguile.h
  PATHS ${PC_GUILE_INCLUDE_DIRS}
  PATH_SUFFIXES guile/3.0
)

find_library(GUILE_LIBRARY
  NAMES guile-3.0
  PATHS ${PC_GUILE_LIBRARY_DIRS}
)

find_program(GUILE_EXECUTABLE
  NAMES guile guile-3.0
  PATHS ${PC_GUILE_PREFIX}/bin
)

if(GUILE_EXECUTABLE)
  execute_process(
    COMMAND ${GUILE_EXECUTABLE} --version
    OUTPUT_VARIABLE GUILE_VERSION_OUTPUT
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" GUILE_VERSION "${GUILE_VERSION_OUTPUT}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GUILE
  REQUIRED_VARS GUILE_LIBRARY GUILE_INCLUDE_DIR
  VERSION_VAR GUILE_VERSION
)

if(GUILE_FOUND)
  set(GUILE_LIBRARIES ${GUILE_LIBRARY})
  set(GUILE_INCLUDE_DIRS ${GUILE_INCLUDE_DIR})
endif()

mark_as_advanced(GUILE_INCLUDE_DIR GUILE_LIBRARY GUILE_EXECUTABLE)
