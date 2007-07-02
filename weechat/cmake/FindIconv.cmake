# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

# - Find Iconv
# This module finds if libiconv is installed and determines where 
# the include files and libraries are.
#
# This code sets the following variables:
#
#  ICONV_INCLUDE_PATH = path to where <iconv.h> can be found
#  ICONV_LIBRARY = path to where libiconv.so* can be found (on non glibc based systems)
#
#  ICONV_FOUND = is iconv usable on system?

IF(ICONV_FOUND)
   # Already in cache, be silent
   set(ICONV_FIND_QUIETLY TRUE)
ENDIF(ICONV_FOUND)

FIND_PATH(ICONV_INCLUDE_PATH
  NAMES iconv.h
  PATHS /usr/include /usr/local/include /usr/pkg/include
)

FIND_LIBRARY(ICONV_LIBRARY
  NAMES iconv
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

IF(ICONV_INCLUDE_PATH)
  IF(ICONV_LIBRARY)
    STRING(REGEX REPLACE "/[^/]*$" "" ICONV_LIB_PATH "${ICONV_LIBRARY}")
    CHECK_LIBRARY_EXISTS(iconv libiconv_open ${ICONV_LIB_PATH} ICONV_FOUND)
    IF(NOT ICONV_FOUND)
      CHECK_LIBRARY_EXISTS(iconv iconv_open ${ICONV_LIB_PATH} ICONV_FOUND)
    ENDIF(NOT ICONV_FOUND)
  ELSE(ICONV_LIBRARY)
    CHECK_FUNCTION_EXISTS(iconv_open ICONV_FOUND)
  ENDIF(ICONV_LIBRARY)
ENDIF(ICONV_INCLUDE_PATH)

include(CheckCSourceCompiles)

IF(ICONV_LIBRARY)
  SET(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
  SET(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_PATH})
ENDIF(ICONV_LIBRARY)

SET(CMAKE_REQUIRED_FLAGS -Werror)
check_c_source_compiles("
  #include <iconv.h>
  int main(){
    iconv_t conv = 0;
    const char* in = 0;
    size_t ilen = 0;
    char* out = 0;
    size_t olen = 0;
    iconv(conv, &in, &ilen, &out, &olen);
    return 0;
  }
" ICONV_2ARG_IS_CONST)
MARK_AS_ADVANCED(
  ICONV_INCLUDE_PATH
  ICONV_LIBRARY
  ICONV_FOUND
)
