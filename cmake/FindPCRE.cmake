#
# Copyright (C) 2015 Simmo Saan <simmo.saan@gmail.com>
# Copyright (C) 2003-2015 Sébastien Helleu <flashcode@flashtux.org>
#
# This file is part of WeeChat, the extensible chat client.
#
# WeeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# WeeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

# - Find PCRE
# This module finds if libpcre and libpcreposix are installed and determines where
# the include files and libraries are.
#
# This code sets the following variables:
#
#  PCRE_INCLUDE_PATH = path to where <pcre.h> can be found
#  PCRE_LIBRARY = path to where libpcre.so* can be found
#  PCREPOSIX_INCLUDE_PATH = path to where <pcreposix.h> can be found
#  PCREPOSIX_LIBRARY = path to where libpcreposix.so* can be found
#  PCRE_CFLAGS = cflags to use to compile
#  PCRE_LDFLAGS = ldflags to use to compile

if(PCRE_INCLUDE_PATH AND PCRE_LIBRARY)
   # Already in cache, be silent
   set(PCRE_FIND_QUIETLY TRUE)
endif()

find_program(PKG_CONFIG_EXECUTABLE NAMES pkg-config)

execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=prefix libpcre libpcreposix
   OUTPUT_VARIABLE PCRE_PREFIX
)

execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --cflags libpcre libpcreposix
   OUTPUT_VARIABLE PCRE_CFLAGS
)
string(REGEX REPLACE "[\r\n]" "" PCRE_CFLAGS "${PCRE_CFLAGS}")

execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --libs libpcre libpcreposix
   OUTPUT_VARIABLE PCRE_LDFLAGS
)
string(REGEX REPLACE "[\r\n]" "" PCRE_LDFLAGS "${PCRE_LDFLAGS}")

set(PCRE_POSSIBLE_INCLUDE_PATH "${PCRE_PREFIX}/include")
set(PCRE_POSSIBLE_LIB_DIR "${PCRE_PREFIX}/lib")

find_path(PCRE_INCLUDE_PATH
  NAMES pcre.h
  PATHS PCRE_POSSIBLE_INCLUDE_PATH
)

find_library(PCRE_LIBRARY
  NAMES pcre
  PATHS PCRE_POSSIBLE_LIB_DIR
)

find_path(PCREPOSIX_INCLUDE_PATH
  NAMES pcreposix.h
  PATHS PCRE_POSSIBLE_INCLUDE_PATH
)

find_library(PCREPOSIX_LIBRARY
  NAMES pcreposix
  PATHS PCRE_POSSIBLE_LIB_DIR
)

if(PCRE_INCLUDE_PATH AND PCRE_LIBRARY AND PCREPOSIX_INCLUDE_PATH AND PCREPOSIX_LIBRARY)
  set(PCRE_FOUND TRUE)
endif()

mark_as_advanced(
  PCRE_INCLUDE_PATH
  PCRE_LIBRARY
  PCREPOSIX_INCLUDE_PATH
  PCREPOSIX_LIBRARY
  PCRE_CFLAGS
  PCRE_LDFLAGS
)
