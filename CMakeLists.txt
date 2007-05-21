# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

PROJECT(weechat)

SET(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
SET(CMAKE_SKIP_RPATH ON)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W")
IF(PREFIX)
  SET(CMAKE_INSTALL_PREFIX ${PREFIX} CACHE PATH "Install path prefix" FORCE)
ENDIF(PREFIX)

SET(VERSION 0.2.5-cvs)
SET(PKG_STRING "${PROJECT_NAME} ${VERSION}")
SET(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/weechat)
SET(SHAREDIR ${CMAKE_INSTALL_PREFIX}/share/weechat)
SET(LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
STRING(REPLACE "\";\"" "\ " PKG_STRING ${PKG_STRING})

OPTION(DISABLE_NCURSES "Disable Ncurses interface")
OPTION(DISABLE_NLS     "Disable Native Language Support")
OPTION(DISABLE_GNUTLS  "Disable SSLv3/TLS connection support")
OPTION(DISABLE_PLUGINS "Disable Plugins support")
OPTION(DISABLE_PERL    "Disable Perl scripting language")
OPTION(DISABLE_PYTHON  "Disable Python scripting language")
OPTION(DISABLE_RUBY    "Disable Ruby scripting language")
OPTION(DISABLE_LUA     "Disable Lua scripting language")
OPTION(DISABLE_ASPELL  "Disable Aspell plugin")
OPTION(DISABLE_CHARSET "Disable Charset plugin")
OPTION(DISABLE_DOC     "Disable Doc")

ADD_SUBDIRECTORY( po ) 
ADD_SUBDIRECTORY( src ) 
IF(NOT DISABLE_DOC)
  ADD_SUBDIRECTORY( doc ) 
ENDIF(NOT DISABLE_DOC)

CONFIGURE_FILE(config.h.cmake config.h @ONLY)
