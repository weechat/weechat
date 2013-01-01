#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
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

IF(GNUTLS_INCLUDE_PATH AND GNUTLS_LIBRARY)
   # Already in cache, be silent
   set(GNUTLS_FIND_QUIETLY TRUE)
ENDIF(GNUTLS_INCLUDE_PATH AND GNUTLS_LIBRARY)

FIND_PROGRAM(PKG_CONFIG_EXECUTABLE NAMES pkg-config)

EXECUTE_PROCESS(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable=prefix gnutls
   OUTPUT_VARIABLE GNUTLS_PREFIX
)

EXECUTE_PROCESS(COMMAND ${PKG_CONFIG_EXECUTABLE} --cflags gnutls
   OUTPUT_VARIABLE GNUTLS_CFLAGS
)
STRING(REGEX REPLACE "[\r\n]" "" GNUTLS_CFLAGS "${GNUTLS_FLAGS}")

EXECUTE_PROCESS(COMMAND ${PKG_CONFIG_EXECUTABLE} --libs gnutls
   OUTPUT_VARIABLE GNUTLS_LDFLAGS
)
STRING(REGEX REPLACE "[\r\n]" "" GNUTLS_LDFLAGS "${GNUTLS_LDFLAGS}")

SET(GNUTLS_POSSIBLE_INCLUDE_PATH "${GNUTLS_PREFIX}/include")
SET(GNUTLS_POSSIBLE_LIB_DIR "${GNUTLS_PREFIX}/lib")

FIND_PATH(GNUTLS_INCLUDE_PATH
  NAMES gnutls/gnutls.h
  PATHS GNUTLS_POSSIBLE_INCLUDE_PATH
)

FIND_LIBRARY(GNUTLS_LIBRARY
  NAMES gnutls
  PATHS GNUTLS_POSSIBLE_LIB_DIR
)

IF (GNUTLS_INCLUDE_PATH AND GNUTLS_LIBRARY)
  SET(GNUTLS_FOUND TRUE)
ENDIF (GNUTLS_INCLUDE_PATH AND GNUTLS_LIBRARY)

MARK_AS_ADVANCED(
  GNUTLS_INCLUDE_PATH
  GNUTLS_LIBRARY
  GNUTLS_CFLAGS
  GNUTLS_LDFLAGS
)
