#
# Copyright (C) 2017 Adam Saponara <as@php.net>
# Copyright (C) 2017-2025 Sébastien Helleu <flashcode@flashtux.org>
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

if(PHP_FOUND)
  set(PHP_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_search_module(PHP php8 php7)
endif()

if(NOT PHP_FOUND)
  find_program(PHP_CONFIG_EXECUTABLE NAMES
    php-config8.4 php-config84
    php-config8.3 php-config83
    php-config8.2 php-config82
    php-config8.1 php-config81
    php-config8.0 php-config80
    php-config8
    php-config7.4 php-config74
    php-config7.3 php-config73
    php-config7.2 php-config72
    php-config7.1 php-config71
    php-config7.0 php-config70
    php-config7
    php-config
  )
  if (PHP_CONFIG_EXECUTABLE)
    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --prefix OUTPUT_VARIABLE PHP_LIB_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --includes OUTPUT_VARIABLE PHP_INCLUDE_DIRS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --libs OUTPUT_VARIABLE PHP_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND ${PHP_CONFIG_EXECUTABLE} --version OUTPUT_VARIABLE PHP_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(${PHP_VERSION} MATCHES "^[78]")
      find_library(PHP_LIB
        NAMES php8.4 php84 php8.3 php83 php8.2 php82 php8.1 php81 php8.0 php80 php8 php7.4 php74 php7.3 php73 php7.2 php72 php7.1 php71 php7.0 php70 php7 php
        HINTS ${PHP_LIB_PREFIX} ${PHP_LIB_PREFIX}/lib ${PHP_LIB_PREFIX}/lib64
      )
      if(PHP_LIB)
        get_filename_component(PHP_LIB_DIR ${PHP_LIB} DIRECTORY)
        string(REPLACE "-I" "" PHP_INCLUDE_DIRS ${PHP_INCLUDE_DIRS})
        SEPARATE_ARGUMENTS(PHP_INCLUDE_DIRS)
        set(PHP_LDFLAGS "-L${PHP_LIB_DIR} ${PHP_LIBS}")
        set(PHP_FOUND 1)
      endif()
    endif()
  endif()
endif()

if(NOT PHP_FOUND)
  message(WARNING "Could not find libphp. "
    "Ensure PHP >=7.0.0 development libraries are installed and compiled with `--enable-embed`. "
    "Ensure `php-config` is in `PATH`. "
    "You may set `-DCMAKE_LIBRARY_PATH=...` to the directory containing libphp."
  )
endif()
