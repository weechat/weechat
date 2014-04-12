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

cmake_minimum_required(VERSION 2.4)

project(weechat C)

set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_SKIP_RPATH ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror-implicit-function-declaration")
if(PREFIX)
  set(CMAKE_INSTALL_PREFIX ${PREFIX} CACHE PATH "Install path prefix" FORCE)
endif()

set(VERSION_MAJOR "0")
set(VERSION_MINOR "4")
set(VERSION_PATCH "4-dev")
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(LICENSE "GPL3")
set(PKG_STRING "${PROJECT_NAME} ${VERSION}")
string(REPLACE "\";\"" "\ " PKG_STRING ${PKG_STRING})

if(DEFINED LIBDIR)
  set(LIBDIR ${LIBDIR}/${PROJECT_NAME})
else()
  set(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME})
endif()

if(NOT DEFINED SHAREDIR)
  set(SHAREDIR ${CMAKE_INSTALL_PREFIX}/share)
endif()

if(NOT DEFINED MANDIR)
  set(MANDIR ${SHAREDIR}/man)
endif()

if(NOT DEFINED LOCALEDIR)
  set(LOCALEDIR ${SHAREDIR}/locale)
endif()

if(DEFINED INCLUDEDIR)
  set(INCLUDEDIR ${INCLUDEDIR}/${PROJECT_NAME})
else()
  set(INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})
endif()

option(ENABLE_NCURSES   "Enable Ncurses interface"                  ON)
option(ENABLE_NLS       "Enable Native Language Support"            ON)
option(ENABLE_GNUTLS    "Enable SSLv3/TLS support"                  ON)
option(ENABLE_LARGEFILE "Enable Large File Support"                 ON)
option(ENABLE_ALIAS     "Enable Alias plugin"                       ON)
option(ENABLE_ASPELL    "Enable Aspell plugin"                      ON)
option(ENABLE_ENCHANT   "Enable Enchant lib for Aspell plugin"      OFF)
option(ENABLE_CHARSET   "Enable Charset plugin"                     ON)
option(ENABLE_EXEC      "Enable Exec plugin"                        ON)
option(ENABLE_FIFO      "Enable FIFO plugin"                        ON)
option(ENABLE_IRC       "Enable IRC plugin"                         ON)
option(ENABLE_LOGGER    "Enable Logger plugin"                      ON)
option(ENABLE_RELAY     "Enable Relay plugin"                       ON)
option(ENABLE_SCRIPT    "Enable Script plugin (scripts manager)"    ON)
option(ENABLE_SCRIPTS   "Enable script plugins (perl, python, ...)" ON)
option(ENABLE_PERL      "Enable Perl scripting language"            ON)
option(ENABLE_PYTHON    "Enable Python scripting language"          ON)
option(ENABLE_PYTHON3   "Use Python 3.x if found (NOT recommended)" OFF)
option(ENABLE_RUBY      "Enable Ruby scripting language"            ON)
option(ENABLE_LUA       "Enable Lua scripting language"             ON)
option(ENABLE_TCL       "Enable Tcl scripting language"             ON)
option(ENABLE_GUILE     "Enable Scheme (guile) scripting language"  ON)
option(ENABLE_TRIGGER   "Enable Trigger plugin"                     ON)
option(ENABLE_XFER      "Enable Xfer plugin"                        ON)
option(ENABLE_MAN       "Enable build of man page"                  OFF)
option(ENABLE_DOC       "Enable build of documentation"             OFF)

# option WEECHAT_HOME
if(NOT DEFINED WEECHAT_HOME OR "${WEECHAT_HOME}" STREQUAL "")
  set(WEECHAT_HOME "~/.weechat")
endif()
set(WEECHAT_HOME "${WEECHAT_HOME}" CACHE
  STRING "WeeChat home directory for config, logs, scripts.. (default is \"~/.weechat\")"
  FORCE)
mark_as_advanced(CLEAR WEECHAT_HOME)

# option CA_FILE
if(NOT DEFINED CA_FILE OR "${CA_FILE}" STREQUAL "")
  set(CA_FILE "/etc/ssl/certs/ca-certificates.crt")
endif()
set(CA_FILE "${CA_FILE}" CACHE
  STRING "File containing the certificate authorities (default is \"/etc/ssl/certs/ca-certificates.crt\"). This is the default value of option \"weechat.network.gnutls_ca_file\"."
  FORCE)
mark_as_advanced(CLEAR CA_FILE)

if(ENABLE_NLS)
  add_subdirectory( po )
endif()

add_subdirectory( src )
add_subdirectory( doc )

configure_file(config.h.cmake config.h @ONLY)

# set the git version in "config-git.h"
add_custom_target(version_git ALL
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/git-version.sh" "${CMAKE_CURRENT_SOURCE_DIR}" "${VERSION}" "config-git.h"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

add_custom_target(dist
  "${CMAKE_CURRENT_SOURCE_DIR}/makedist.sh" "${VERSION}" "HEAD"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# pkgconfig file
set(PACKAGE "${PROJECT_NAME}")
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
set(libdir "\${exec_prefix}/lib")
set(includedir "\${prefix}/include")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/weechat.pc.in ${CMAKE_CURRENT_BINARY_DIR}/weechat.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.pc DESTINATION ${LIBDIR}/../pkgconfig)

# cygport file (used to build Cygwin packages)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/weechat.cygport.in ${CMAKE_CURRENT_BINARY_DIR}/weechat-${VERSION}-1.cygport @ONLY)

# icon
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/weechat.png DESTINATION ${SHAREDIR}/icons/hicolor/32x32/apps)

# packages
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast, light and extensible chat client")
set(CPACK_PACKAGE_VENDOR "Sébastien Helleu")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.asciidoc")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})

# binary package
set(CPACK_GENERATOR "STGZ;TGZ;TBZ2")
set(CPACK_PACKAGE_FILE_NAME weechat-binary-${VERSION})

# source package
set(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
set(CPACK_SOURCE_PACKAGE_FILE_NAME weechat-${VERSION})
set(CPACK_SOURCE_IGNORE_FILES "/\\\\.git" "/build/" "/m4/"
  "/autom4te\\\\.cache/" "/ABOUT-NLS$" "/config\\\\.guess$" "/config\\\\.h$"
  "/config\\\\.h.in$" "/config\\\\.log$" "/config\\\\.rpath$"
  "/config\\\\.status$" "/config\\\\.sub$" "/configure$" "/depcomp$"
  "/install-sh$" "/missing$" "/intl/" "/libtool$" "/\\\\.libs/"
  "/ltmain\\\\.sh$" "/\\\\.deps/" "/html/" "/html1/" "/Makefile$"
  "/Makefile\\\\.in$" "stamp" "/po/.*\\\\.header$" "\\\\.gmo$" "~$" "\\\\.o$"
  "\\\\.lo$" "\\\\.a$" "\\\\.la$" "\\\\.lai$" "\\\\.Plo$" "/weechat$"
)

include(CPack)
