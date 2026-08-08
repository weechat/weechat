# SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# - Find V8 (Google's JavaScript engine)
# This module finds if libv8 is installed and determines where
# the include files and libraries are.
#
# This code sets the following variables:
#
#  V8_INCLUDE_DIR = path to where v8.h can be found
#  V8_LIBRARY = path to where libv8.so* can be found

if(V8_FOUND)
  # Already in cache, be silent
  SET(V8_FIND_QUIETLY TRUE)
endif()

set(V8_INC_PATHS
  /usr/include
  ${CMAKE_INCLUDE_PATH}
)
find_path(V8_INCLUDE_DIR v8.h PATHS ${V8_INC_PATHS})
find_library(V8_LIBRARY
  NAMES v8
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

find_package_handle_standard_args(V8 DEFAULT_MSG V8_LIBRARY V8_INCLUDE_DIR)

mark_as_advanced(
  V8_INCLUDE_DIR
  V8_LIBRARY
)
