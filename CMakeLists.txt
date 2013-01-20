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

PROJECT(weechat C)

cmake_minimum_required(VERSION 2.4)

SET(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
SET(CMAKE_SKIP_RPATH ON)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Werror-implicit-function-declaration")
IF(PREFIX)
  SET(CMAKE_INSTALL_PREFIX ${PREFIX} CACHE PATH "Install path prefix" FORCE)
ENDIF(PREFIX)

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "4")
SET(VERSION_PATCH "1-dev")
SET(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
SET(LICENSE "GPL3")
SET(PKG_STRING "${PROJECT_NAME} ${VERSION}")
STRING(REPLACE "\";\"" "\ " PKG_STRING ${PKG_STRING})

IF(DEFINED LIBDIR)
  SET(LIBDIR ${LIBDIR}/${PROJECT_NAME})
ELSE(DEFINED LIBDIR)
  SET(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME})
ENDIF(DEFINED LIBDIR)

IF(NOT DEFINED SHAREDIR)
  SET(SHAREDIR ${CMAKE_INSTALL_PREFIX}/share)
ENDIF(NOT DEFINED SHAREDIR)

IF(NOT DEFINED MANDIR)
  SET(MANDIR ${SHAREDIR}/man)
ENDIF(NOT DEFINED MANDIR)

IF(NOT DEFINED LOCALEDIR)
  SET(LOCALEDIR ${SHAREDIR}/locale)
ENDIF(NOT DEFINED LOCALEDIR)

IF(DEFINED INCLUDEDIR)
  SET(INCLUDEDIR ${INCLUDEDIR}/${PROJECT_NAME})
ELSE(DEFINED INCLUDEDIR)
  SET(INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})
ENDIF(DEFINED INCLUDEDIR)

OPTION(ENABLE_NCURSES   "Enable Ncurses interface"                  ON)
OPTION(ENABLE_GTK       "Enable GTK interface"                      OFF)
OPTION(ENABLE_NLS       "Enable Native Language Support"            ON)
OPTION(ENABLE_GNUTLS    "Enable SSLv3/TLS support"                  ON)
OPTION(ENABLE_LARGEFILE "Enable Large File Support"                 ON)
OPTION(ENABLE_ALIAS     "Enable Alias plugin"                       ON)
OPTION(ENABLE_ASPELL    "Enable Aspell plugin"                      ON)
OPTION(ENABLE_CHARSET   "Enable Charset plugin"                     ON)
OPTION(ENABLE_DEMO      "Enable Demo plugin"                        OFF)
OPTION(ENABLE_FIFO      "Enable FIFO plugin"                        ON)
OPTION(ENABLE_IRC       "Enable IRC plugin"                         ON)
OPTION(ENABLE_LOGGER    "Enable Logger plugin"                      ON)
OPTION(ENABLE_RELAY     "Enable Relay plugin"                       ON)
OPTION(ENABLE_RMODIFIER "Enable Rmodifier plugin"                   ON)
OPTION(ENABLE_SCRIPT    "Enable Script plugin (scripts manager)"    ON)
OPTION(ENABLE_SCRIPTS   "Enable script plugins (perl, python, ...)" ON)
OPTION(ENABLE_PERL      "Enable Perl scripting language"            ON)
OPTION(ENABLE_PYTHON    "Enable Python scripting language"          ON)
OPTION(ENABLE_PYTHON3   "Use Python 3.x if found (NOT recommended)" OFF)
OPTION(ENABLE_RUBY      "Enable Ruby scripting language"            ON)
OPTION(ENABLE_LUA       "Enable Lua scripting language"             ON)
OPTION(ENABLE_TCL       "Enable Tcl scripting language"             ON)
OPTION(ENABLE_GUILE     "Enable Scheme (guile) scripting language"  ON)
OPTION(ENABLE_XFER      "Enable Xfer plugin"                        ON)
OPTION(ENABLE_DOC       "Enable Documentation"                      ON)

IF(NOT DEFINED WEECHAT_HOME OR "${WEECHAT_HOME}" STREQUAL "")
  SET(WEECHAT_HOME "~/.weechat")
ENDIF(NOT DEFINED WEECHAT_HOME OR "${WEECHAT_HOME}" STREQUAL "")

SET(WEECHAT_HOME "${WEECHAT_HOME}" CACHE
  STRING "WeeChat home directory for config, logs, scripts.. (default is \"~/.weechat\")"
  FORCE)
MARK_AS_ADVANCED(CLEAR WEECHAT_HOME)

IF(ENABLE_NLS)
  ADD_SUBDIRECTORY( po )
ENDIF(ENABLE_NLS)

ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( doc )

CONFIGURE_FILE(config.h.cmake config.h @ONLY)

# set the git version in "config-git.h"
ADD_CUSTOM_TARGET(version_git ALL
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/git-version.sh" "${CMAKE_CURRENT_SOURCE_DIR}" "${VERSION}" "config-git.h"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

ADD_CUSTOM_TARGET(dist
  "${CMAKE_CURRENT_SOURCE_DIR}/makedist.sh" "${VERSION}" "HEAD"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# pkgconfig file
SET(PACKAGE "${PROJECT_NAME}")
SET(prefix "${CMAKE_INSTALL_PREFIX}")
SET(exec_prefix "\${prefix}")
SET(libdir "\${exec_prefix}/lib")
SET(includedir "\${prefix}/include")
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/weechat.pc.in ${CMAKE_CURRENT_BINARY_DIR}/weechat.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.pc DESTINATION ${LIBDIR}/../pkgconfig)

# packages
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast, light and extensible chat client")
SET(CPACK_PACKAGE_VENDOR "Sebastien Helleu")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})

# binary package
SET(CPACK_GENERATOR "STGZ;TGZ;TBZ2")
SET(CPACK_PACKAGE_FILE_NAME weechat-binary-${VERSION})

# source package
SET(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME weechat-${VERSION})
SET(CPACK_SOURCE_IGNORE_FILES "/\\\\.git" "/build/" "/m4/"
  "/autom4te\\\\.cache/" "/ABOUT-NLS$" "/config\\\\.guess$" "/config\\\\.h$"
  "/config\\\\.h.in$" "/config\\\\.log$" "/config\\\\.rpath$"
  "/config\\\\.status$" "/config\\\\.sub$" "/configure$" "/depcomp$"
  "/install-sh$" "/missing$" "/intl/" "/libtool$" "/\\\\.libs/"
  "/ltmain\\\\.sh$" "/\\\\.deps/" "/html/" "/html1/" "/Makefile$"
  "/Makefile\\\\.in$" "stamp" "/po/.*\\\\.header$" "\\\\.gmo$" "~$" "\\\\.o$"
  "\\\\.lo$" "\\\\.a$" "\\\\.la$" "\\\\.lai$" "\\\\.Plo$" "/weechat-curses$"
  "/weechat-gtk$"
)

INCLUDE(CPack)
