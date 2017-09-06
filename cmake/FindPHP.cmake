#
# Copyright (C) 2003-2017 Adam Saponara <as@php.net>
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

if(PHP_FOUND)
  set(PHP_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PHP php7)
endif()

if(NOT PHP_FOUND)
  find_program(PHP_CONFIG_EXECUTABLE NAMES
    php-config php-config7
    php-config7.2 php-config72
    php-config7.1 php-config71
    php-config7.0 php-config70)
  if (PHP_CONFIG_EXECUTABLE)
    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --prefix OUTPUT_VARIABLE PHP_LIB_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --includes OUTPUT_VARIABLE PHP_INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --libs OUTPUT_VARIABLE PHP_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --version OUTPUT_VARIABLE PHP_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(${PHP_VERSION} MATCHES "^7")
      string(REPLACE "-I" "" PHP_INCLUDE_DIRS ${PHP_INCLUDE_DIRS})
      SEPARATE_ARGUMENTS(PHP_INCLUDE_DIRS)
      set(PHP_LDFLAGS "-L${PHP_LIB_PREFIX}/lib/ ${PHP_LIBS} -lphp7")
      set(PHP_FOUND 1)
    endif()
  endif()
endif()
