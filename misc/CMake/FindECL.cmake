# FindECL.cmake - Find ECL (Embeddable Common Lisp) library
# This module defines:
#  ECL_FOUND - True if ECL is found
#  ECL_INCLUDE_DIRS - Include directories for ECL
#  ECL_LIBRARIES - Libraries to link against
#  ECL_VERSION_STRING - Version of ECL found

# First, try to find ecl-config
find_program(ECL_CONFIG_EXECUTABLE NAMES ecl-config)

if(ECL_CONFIG_EXECUTABLE)
  # Get include directories
  execute_process(
    COMMAND ${ECL_CONFIG_EXECUTABLE} --cflags
    OUTPUT_VARIABLE ECL_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  # Extract include path from cflags
  string(REGEX MATCH "-I[^ ]+" ECL_INCLUDE_FLAG "${ECL_CFLAGS}")
  string(REGEX REPLACE "-I" "" ECL_INCLUDE_DIR "${ECL_INCLUDE_FLAG}")
  
  # Get libraries
  execute_process(
    COMMAND ${ECL_CONFIG_EXECUTABLE} --libs
    OUTPUT_VARIABLE ECL_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  # Parse libraries and library directories
  set(ECL_LIBRARY_DIRS "")
  set(ECL_LIBRARIES "")
  
  # Split the libs string into components
  separate_arguments(ECL_LIBS_LIST UNIX_COMMAND "${ECL_LIBS}")
  
  foreach(lib ${ECL_LIBS_LIST})
    if(lib MATCHES "^-L")
      # Library directory
      string(REGEX REPLACE "^-L" "" libdir "${lib}")
      list(APPEND ECL_LIBRARY_DIRS "${libdir}")
    elseif(lib MATCHES "^-l")
      # Library name
      string(REGEX REPLACE "^-l" "" libname "${lib}")
      list(APPEND ECL_LIBRARIES "${libname}")
    endif()
  endforeach()
  
  # Find the actual library files
  set(ECL_LIBRARY_FILES "")
  foreach(libname ${ECL_LIBRARIES})
    find_library(ECL_LIB_${libname}
      NAMES ${libname}
      HINTS ${ECL_LIBRARY_DIRS}
    )
    if(ECL_LIB_${libname})
      list(APPEND ECL_LIBRARY_FILES "${ECL_LIB_${libname}}")
    endif()
  endforeach()
  
  # Try to get version from ecl executable
  find_program(ECL_EXECUTABLE NAMES ecl)
  if(ECL_EXECUTABLE)
    execute_process(
      COMMAND ${ECL_EXECUTABLE} --version
      OUTPUT_VARIABLE ECL_VERSION_OUTPUT
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
    # Extract version from output like "ECL (Embeddable Common-Lisp) 21.2.1"
    string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" ECL_VERSION_STRING "${ECL_VERSION_OUTPUT}")
  endif()
  
  # Set standard CMake variables
  if(ECL_INCLUDE_DIR AND ECL_LIBRARY_FILES)
    set(ECL_FOUND TRUE)
    set(ECL_INCLUDE_DIRS "${ECL_INCLUDE_DIR}")
    set(ECL_LIBRARIES "${ECL_LIBRARY_FILES}")
    
    # Cache the results
    set(ECL_FOUND "${ECL_FOUND}" CACHE BOOL "ECL found")
    set(ECL_INCLUDE_DIRS "${ECL_INCLUDE_DIRS}" CACHE PATH "ECL include directories")
    set(ECL_LIBRARIES "${ECL_LIBRARIES}" CACHE FILEPATH "ECL libraries")
    set(ECL_VERSION_STRING "${ECL_VERSION_STRING}" CACHE STRING "ECL version")
    
    message(STATUS "Found ECL: ${ECL_LIBRARIES}")
    if(ECL_VERSION_STRING)
      message(STATUS "ECL version: ${ECL_VERSION_STRING}")
    endif()
  else()
    set(ECL_FOUND FALSE)
  endif()
  
else(ECL_CONFIG_EXECUTABLE)
  set(ECL_FOUND FALSE)
  if(ECL_FIND_REQUIRED)
    message(FATAL_ERROR "ECL not found: ecl-config not found in PATH")
  endif()
endif(ECL_CONFIG_EXECUTABLE)

# Handle standard arguments
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ECL
  REQUIRED_VARS ECL_INCLUDE_DIRS ECL_LIBRARIES
  VERSION_VAR ECL_VERSION_STRING
)

mark_as_advanced(
  ECL_CONFIG_EXECUTABLE
  ECL_EXECUTABLE
  ECL_INCLUDE_DIR
  ECL_LIBRARY_DIRS
)
