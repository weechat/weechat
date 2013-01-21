#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
  endif(POLICY CMP0003)
  if(POLICY CMP0017)
    cmake_policy(SET CMP0017 NEW)
  endif(POLICY CMP0017)
endif(COMMAND cmake_policy)

ADD_DEFINITIONS(-DHAVE_CONFIG_H)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)

CHECK_INCLUDE_FILES("langinfo.h" HAVE_LANGINFO_CODESET)
CHECK_INCLUDE_FILES("sys/resource.h" HAVE_SYS_RESOURCE_H)

CHECK_FUNCTION_EXISTS(mallinfo HAVE_MALLINFO)

CHECK_SYMBOL_EXISTS("eat_newline_glitch" "term.h" HAVE_EAT_NEWLINE_GLITCH)

# weechat_gui_common MUST be the first lib in the list
SET(STATIC_LIBS weechat_gui_common)

# Check for Large File Support
IF(ENABLE_LARGEFILE)
  ADD_DEFINITIONS(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE -D_LARGE_FILES)
ENDIF(ENABLE_LARGEFILE)

# Check for Gettext
IF(ENABLE_NLS)
  FIND_PACKAGE(Gettext)
  IF(GETTEXT_FOUND)
    ADD_DEFINITIONS(-DENABLE_NLS)
  ENDIF(GETTEXT_FOUND)
ENDIF(ENABLE_NLS)

# Check for libgcrypt
FIND_PACKAGE(GCRYPT REQUIRED)
ADD_DEFINITIONS(-DHAVE_GCRYPT)
LIST(APPEND EXTRA_LIBS ${GCRYPT_LDFLAGS})

# Check for GnuTLS
IF(ENABLE_GNUTLS)
  FIND_PACKAGE(GnuTLS)

  IF(GNUTLS_FOUND)
    STRING(REGEX REPLACE "/[^/]*$" "" GNUTLS_LIBRARY_PATH "${GNUTLS_LIBRARY}")
    ADD_DEFINITIONS(-DHAVE_GNUTLS)
    INCLUDE_DIRECTORIES(${GNUTLS_INCLUDE_PATH})
    SET(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -L${GNUTLS_LIBRARY_PATH}")
    LIST(APPEND EXTRA_LIBS gnutls)
  ENDIF(GNUTLS_FOUND)
ENDIF(ENABLE_GNUTLS)

# Check for zlib
FIND_PACKAGE(ZLIB REQUIRED)
ADD_DEFINITIONS(-DHAVE_ZLIB)

# Check for iconv
FIND_PACKAGE(Iconv)
IF(ICONV_FOUND)
  ADD_DEFINITIONS( -DHAVE_ICONV )
ENDIF(ICONV_FOUND)

# Check for CURL
FIND_PACKAGE(CURL REQUIRED)

FIND_LIBRARY(DL_LIBRARY
  NAMES dl
  PATHS /lib /usr/lib /usr/libexec /usr/local/lib /usr/local/libexec
)
LIST(APPEND STATIC_LIBS weechat_plugins)
IF(DL_LIBRARY)
  STRING(REGEX REPLACE "/[^/]*$" "" DL_LIBRARY_PATH "${DL_LIBRARY}")
  SET(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -L${DL_LIBRARY_PATH}")
  LIST(APPEND EXTRA_LIBS dl)
ENDIF(DL_LIBRARY)

IF(COMMAND cmake_policy)
  cmake_policy(SET CMP0005 NEW)
  ADD_DEFINITIONS(-DWEECHAT_VERSION="${VERSION}" -DWEECHAT_LICENSE="${LICENSE}")
ELSE(COMMAND cmake_policy)
  ADD_DEFINITIONS(-DWEECHAT_VERSION='"${VERSION}"' -DWEECHAT_LICENSE='"${LICENSE}"')
ENDIF(COMMAND cmake_policy)

ADD_SUBDIRECTORY( core )
LIST(APPEND STATIC_LIBS weechat_core)

ADD_SUBDIRECTORY( plugins )

ADD_SUBDIRECTORY( gui )
