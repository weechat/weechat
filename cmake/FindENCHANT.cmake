# - Try to find the Enchant spell checker
# Once done this will define
#
#  ENCHANT_FOUND - system has ENCHANT
#  ENCHANT_INCLUDE_DIR - the ENCHANT include directory
#  ENCHANT_LIBRARIES - Link these to use ENCHANT
#  ENCHANT_DEFINITIONS - Compiler switches required for using ENCHANT

# Copyright (c) 2006, Zack Rusin, <zack@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


if(ENCHANT_INCLUDE_DIR AND ENCHANT_LIBRARIES)
  # in cache already
  set(ENCHANT_FOUND TRUE)
else()
  if(NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_check_modules(PC_ENCHANT enchant)
    set(ENCHANT_DEFINITIONS ${PC_ENCHANT_CFLAGS_OTHER})
  endif()

  find_path(ENCHANT_INCLUDE_DIR
    NAMES enchant++.h
    HINTS ${PC_ENCHANT_INCLUDEDIR} ${PC_ENCHANT_INCLUDE_DIRS}
    PATH_SUFFIXES enchant-2 enchant
  )

  find_library(ENCHANT_LIBRARIES
    NAMES enchant-2 enchant
    HINTS ${PC_ENCHANT_LIBDIR}
    ${PC_ENCHANT_LIBRARY_DIRS}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ENCHANT DEFAULT_MSG ENCHANT_INCLUDE_DIR ENCHANT_LIBRARIES)

  mark_as_advanced(ENCHANT_INCLUDE_DIR ENCHANT_LIBRARIES)

  # check if function enchant_get_version() exists
  set(CMAKE_REQUIRED_INCLUDES ${ENCHANT_INCLUDE_DIR})
  set(CMAKE_REQUIRED_LIBRARIES ${ENCHANT_LIBRARIES})
  check_symbol_exists(enchant_get_version "enchant.h" HAVE_ENCHANT_GET_VERSION)
  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
endif()
