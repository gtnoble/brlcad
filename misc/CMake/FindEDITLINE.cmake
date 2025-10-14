# - Try to find libedit/editline
# Once done this will define
#  EDITLINE_FOUND - System has libedit
#  EDITLINE_INCLUDE_DIRS - The libedit include directories
#  EDITLINE_LIBRARIES - The libraries needed to use libedit
#  EDITLINE_DEFINITIONS - Compiler switches required for using libedit

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_EDITLINE QUIET libedit)
endif()

set(EDITLINE_DEFINITIONS ${PC_EDITLINE_CFLAGS_OTHER})

find_path(EDITLINE_INCLUDE_DIR histedit.h
  HINTS ${PC_EDITLINE_INCLUDEDIR} ${PC_EDITLINE_INCLUDE_DIRS}
  PATH_SUFFIXES editline
)

find_library(EDITLINE_LIBRARY
  NAMES edit
  HINTS ${PC_EDITLINE_LIBDIR} ${PC_EDITLINE_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set EDITLINE_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(EDITLINE DEFAULT_MSG
                                  EDITLINE_LIBRARY EDITLINE_INCLUDE_DIR)

mark_as_advanced(EDITLINE_INCLUDE_DIR EDITLINE_LIBRARY)

set(EDITLINE_LIBRARIES ${EDITLINE_LIBRARY})
set(EDITLINE_INCLUDE_DIRS ${EDITLINE_INCLUDE_DIR})
