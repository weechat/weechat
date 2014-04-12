#
# Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2007-2008 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2008-2009 Emmanuel Bouthenot <kolter@openics.org>
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

if(COMMAND cmake_policy)
  if(POLICY CMP0003)
    cmake_policy(SET CMP0003 NEW)
  endif()
  if(POLICY CMP0017)
    cmake_policy(SET CMP0017 NEW)
  endif()
endif()

add_definitions(-DHAVE_CONFIG_H)

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)

check_include_files("langinfo.h" HAVE_LANGINFO_CODESET)
check_include_files("sys/resource.h" HAVE_SYS_RESOURCE_H)

check_function_exists(mallinfo HAVE_MALLINFO)

check_symbol_exists("eat_newline_glitch" "term.h" HAVE_EAT_NEWLINE_GLITCH)

# weechat_gui_common MUST be the first lib in the list
set(STATIC_LIBS weechat_gui_common)

# Check for Large File Support
if(ENABLE_LARGEFILE)
  add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE -D_LARGE_FILES)
endif()

# Check for Gettext
if(ENABLE_NLS)
  find_package(Gettext)
  if(GETTEXT_FOUND)
    add_definitions(-DENABLE_NLS)
  endif()
endif()

# Check for libgcrypt
find_package(GCRYPT REQUIRED)
add_definitions(-DHAVE_GCRYPT)
list(APPEND EXTRA_LIBS ${GCRYPT_LDFLAGS})

# Check for GnuTLS
if(ENABLE_GNUTLS)
  find_package(GnuTLS)
  if(GNUTLS_FOUND)
    string(REGEX REPLACE "/[^/]*$" "" GNUTLS_LIBRARY_PATH "${GNUTLS_LIBRARY}")
    add_definitions(-DHAVE_GNUTLS)
    include_directories(${GNUTLS_INCLUDE_PATH})
    set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -L${GNUTLS_LIBRARY_PATH}")
    list(APPEND EXTRA_LIBS gnutls)
  endif()
endif()

# Check for zlib
find_package(ZLIB REQUIRED)
add_definitions(-DHAVE_ZLIB)

# Check for iconv
find_package(Iconv)
if(ICONV_FOUND)
  add_definitions( -DHAVE_ICONV )
endif()

# Check for CURL
find_package(CURL REQUIRED)

find_library(DL_LIBRARY
  NAMES dl
  PATHS /lib /usr/lib /usr/libexec /usr/local/lib /usr/local/libexec
)
list(APPEND STATIC_LIBS weechat_plugins)
if(DL_LIBRARY)
  string(REGEX REPLACE "/[^/]*$" "" DL_LIBRARY_PATH "${DL_LIBRARY}")
  set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -L${DL_LIBRARY_PATH}")
  list(APPEND EXTRA_LIBS dl)
endif()

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
  add_definitions(-DWEECHAT_VERSION="${VERSION}" -DWEECHAT_LICENSE="${LICENSE}")
else()
  add_definitions(-DWEECHAT_VERSION='"${VERSION}"' -DWEECHAT_LICENSE='"${LICENSE}"')
endif()

add_subdirectory( core )
list(APPEND STATIC_LIBS weechat_core)

add_subdirectory( plugins )

add_subdirectory( gui )
