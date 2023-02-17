#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

if(ICONV_FOUND)
  # Already in cache, be silent
  set(ICONV_FIND_QUIETLY TRUE)
endif()

include(CheckLibraryExists)
include(CheckFunctionExists)

find_path(ICONV_INCLUDE_PATH
  NAMES iconv.h
  PATHS /usr/include /usr/local/include /usr/pkg/include
)

find_library(ICONV_LIBRARY
  NAMES iconv
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

if(ICONV_INCLUDE_PATH)
  if(ICONV_LIBRARY)
    check_library_exists("${ICONV_LIBRARY}" libiconv_open "" LIBICONV_OPEN_FOUND)
    check_library_exists("${ICONV_LIBRARY}" iconv_open "" ICONV_OPEN_FOUND)
    if(LIBICONV_OPEN_FOUND OR ICONV_OPEN_FOUND)
       set(ICONV_FOUND TRUE)
    endif()
  else()
    check_function_exists(iconv_open ICONV_FOUND)
  endif()
endif()

include(CheckCSourceCompiles)

if(ICONV_LIBRARY)
  set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARY})
  set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_PATH})
endif()

set(CMAKE_REQUIRED_FLAGS -Werror)
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

mark_as_advanced(
  ICONV_INCLUDE_PATH
  ICONV_LIBRARY
  ICONV_FOUND
)
