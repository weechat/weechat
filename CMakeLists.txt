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

PROJECT(weechat)

SET(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
SET(CMAKE_SKIP_RPATH ON)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W")
IF(PREFIX)
  SET(CMAKE_INSTALL_PREFIX ${PREFIX} CACHE PATH "Install path prefix" FORCE)
ENDIF(PREFIX)

SET(VERSION 0.2.7-dev)
SET(PKG_STRING "${PROJECT_NAME} ${VERSION}")
SET(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/weechat)
SET(SHAREDIR ${CMAKE_INSTALL_PREFIX}/share/weechat)
SET(LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
STRING(REPLACE "\";\"" "\ " PKG_STRING ${PKG_STRING})

OPTION(DISABLE_NCURSES "Disable Ncurses interface")
OPTION(ENABLE_GTK      "Enable GTK interface")
OPTION(DISABLE_NLS     "Disable Native Language Support")
OPTION(DISABLE_GNUTLS  "Disable SSLv3/TLS connection support")
OPTION(DISABLE_IRC     "Disable IRC plugin")
OPTION(DISABLE_SCRIPTS "Disable script plugins")
OPTION(DISABLE_PERL    "Disable Perl scripting language")
OPTION(DISABLE_PYTHON  "Disable Python scripting language")
OPTION(DISABLE_RUBY    "Disable Ruby scripting language")
OPTION(DISABLE_LUA     "Disable Lua scripting language")
OPTION(DISABLE_ASPELL  "Disable Aspell plugin")
OPTION(DISABLE_CHARSET "Disable Charset plugin")
OPTION(DISABLE_FIFO    "Disable FIFO plugin")
OPTION(DISABLE_TRIGGER "Disable Trigger plugin")
OPTION(DISABLE_DOC     "Disable Doc")

ADD_SUBDIRECTORY( po ) 
ADD_SUBDIRECTORY( src ) 
IF(NOT DISABLE_DOC)
  ADD_SUBDIRECTORY( doc ) 
ENDIF(NOT DISABLE_DOC)

CONFIGURE_FILE(config.h.cmake config.h @ONLY)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/makedist.sh.in"
  "${CMAKE_CURRENT_BINARY_DIR}/makedist.sh" IMMEDIATE
  @ONLY)

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

ADD_CUSTOM_TARGET(dist
  "${CMAKE_BINARY_DIR}/makedist.sh"
  DEPENDS doc
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

