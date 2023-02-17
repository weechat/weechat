#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2009 Emmanuel Bouthenot <kolter@openics.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

# - Find GnuTLS
# This module finds if libgnutls is installed and determines where
# the include files and libraries are.
#
# This code sets the following variables:
#
#  GNUTLS_INCLUDE_PATH = path to where <gnutls/gnutls.h> can be found
#  GNUTLS_LIBRARY = path to where libgnutls.so* can be found
#  GNUTLS_CFLAGS = cflags to use to compile
#  GNUTLS_LDFLAGS = ldflags to use to compile

if(GNUTLS_INCLUDE_PATH AND GNUTLS_LIBRARY)
  # Already in cache, be silent
  set(GNUTLS_FIND_QUIETLY TRUE)
endif()

find_program(PKG_CONFIG_EXECUTABLE NAMES pkg-config)

execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=prefix gnutls
  OUTPUT_VARIABLE GNUTLS_PREFIX
)

execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --cflags gnutls
  OUTPUT_VARIABLE GNUTLS_CFLAGS
)
string(REGEX REPLACE "[\r\n]" "" GNUTLS_CFLAGS "${GNUTLS_CFLAGS}")

execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --libs gnutls
  OUTPUT_VARIABLE GNUTLS_LDFLAGS
)
string(REGEX REPLACE "[\r\n]" "" GNUTLS_LDFLAGS "${GNUTLS_LDFLAGS}")

set(GNUTLS_POSSIBLE_INCLUDE_PATH "${GNUTLS_PREFIX}/include")
set(GNUTLS_POSSIBLE_LIB_DIR "${GNUTLS_PREFIX}/lib")

find_path(GNUTLS_INCLUDE_PATH
  NAMES gnutls/gnutls.h
  PATHS GNUTLS_POSSIBLE_INCLUDE_PATH
)

find_library(GNUTLS_LIBRARY
  NAMES gnutls
  PATHS GNUTLS_POSSIBLE_LIB_DIR
)

if(NOT GNUTLS_INCLUDE_PATH OR NOT GNUTLS_LIBRARY)
  message(FATAL_ERROR "GnuTLS was not found")
endif()

mark_as_advanced(
  GNUTLS_INCLUDE_PATH
  GNUTLS_LIBRARY
  GNUTLS_CFLAGS
  GNUTLS_LDFLAGS
)
