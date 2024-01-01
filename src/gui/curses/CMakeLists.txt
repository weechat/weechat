#
# Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2007-2008 Julien Louis <ptitlouis@sysif.net>
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

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" AND HAVE_BACKTRACE)
  list(APPEND EXTRA_LIBS "execinfo")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
  list(APPEND EXTRA_LIBS "socket" "nsl")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Haiku")
  list(APPEND EXTRA_LIBS "network")
endif()

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Haiku")
  list(APPEND EXTRA_LIBS "pthread")
endif()

if(ICONV_LIBRARY)
  list(APPEND EXTRA_LIBS ${ICONV_LIBRARY})
endif()

if(LIBINTL_LIBRARY)
  list(APPEND EXTRA_LIBS ${LIBINTL_LIBRARY})
endif()

list(APPEND EXTRA_LIBS "m")

list(APPEND EXTRA_LIBS ${CURL_LIBRARIES})

list(APPEND EXTRA_LIBS ${ZLIB_LIBRARY})

if(ENABLE_ZSTD)
  list(APPEND EXTRA_LIBS ${LIBZSTD_LDFLAGS})
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # link with resolv lib on macOS
  list(APPEND EXTRA_LIBS "resolv")
endif()

if(ENABLE_NCURSES)
  subdirs(normal)
endif()

if(ENABLE_HEADLESS)
  subdirs(headless)
endif()
