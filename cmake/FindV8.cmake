# SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# - Find V8 (Google's JavaScript engine)
# This module finds if V8 is installed and determines where the include files
# and libraries are.
#
# V8 is searched first in Node.js shared library (libnode), then as a
# standalone library (libv8 and libv8_libplatform).
#
# This code sets the following variables:
#
#  V8_FOUND       = V8 is installed
#  V8_VERSION     = V8 version (for example "13.6.233.10")
#  V8_INCLUDE_DIR = path to where v8.h can be found
#  V8_LIBRARIES   = libraries to link with

if(V8_FOUND)
  # Already in cache, be silent.
  set(V8_FIND_QUIETLY TRUE)
endif()

# V8 provided by Node.js (libnode).
find_library(V8_NODE_LIBRARY NAMES node)
if(V8_NODE_LIBRARY)
  find_path(V8_INCLUDE_DIR v8.h PATH_SUFFIXES node)
  set(V8_LIBRARIES ${V8_NODE_LIBRARY})
endif()

# Standalone V8 (libv8 + libv8_libplatform).
if(NOT V8_INCLUDE_DIR)
  find_path(V8_INCLUDE_DIR v8.h PATH_SUFFIXES v8)
  find_library(V8_LIBRARY NAMES v8)
  find_library(V8_LIBPLATFORM_LIBRARY NAMES v8_libplatform)
  if(V8_LIBRARY AND V8_LIBPLATFORM_LIBRARY)
    set(V8_LIBRARIES ${V8_LIBRARY} ${V8_LIBPLATFORM_LIBRARY})
  endif()
endif()

if(V8_INCLUDE_DIR AND EXISTS "${V8_INCLUDE_DIR}/v8-version.h")
  file(STRINGS "${V8_INCLUDE_DIR}/v8-version.h" V8_VERSION_DEFINES
    REGEX "^#define V8_(MAJOR|MINOR)_VERSION|^#define V8_(BUILD_NUMBER|PATCH_LEVEL)")
  foreach(part MAJOR_VERSION MINOR_VERSION BUILD_NUMBER PATCH_LEVEL)
    string(REGEX MATCH "#define V8_${part} +[0-9]+" V8_${part} "${V8_VERSION_DEFINES}")
    string(REGEX REPLACE "#define V8_${part} +" "" V8_${part} "${V8_${part}}")
  endforeach()
  set(V8_VERSION "${V8_MAJOR_VERSION}.${V8_MINOR_VERSION}.${V8_BUILD_NUMBER}.${V8_PATCH_LEVEL}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(V8
  REQUIRED_VARS V8_LIBRARIES V8_INCLUDE_DIR
  VERSION_VAR V8_VERSION
)

mark_as_advanced(
  V8_INCLUDE_DIR
  V8_NODE_LIBRARY
  V8_LIBRARY
  V8_LIBPLATFORM_LIBRARY
)
