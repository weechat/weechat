# Copyright (c) 2003-2009 FlashCode <flashcode@flashtux.org>
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

cmake_minimum_required(VERSION 2.4)

SET(CMAKE_VERBOSE_MAKEFILE OFF)
SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
SET(CMAKE_SKIP_RPATH ON)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Werror-implicit-function-declaration")
IF(PREFIX)
  SET(CMAKE_INSTALL_PREFIX ${PREFIX} CACHE PATH "Install path prefix" FORCE)
ENDIF(PREFIX)

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "3")
SET(VERSION_PATCH "0-rc3")
SET(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
SET(PKG_STRING "${PROJECT_NAME} ${VERSION}")
SET(LIBDIR ${CMAKE_INSTALL_PREFIX}/lib/${PROJECT_NAME})
SET(SHAREDIR ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME})
SET(LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
SET(INCLUDEDIR ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})
STRING(REPLACE "\";\"" "\ " PKG_STRING ${PKG_STRING})

OPTION(DISABLE_NCURSES       "Disable Ncurses interface")
OPTION(ENABLE_GTK            "Enable GTK interface")
OPTION(DISABLE_NLS           "Disable Native Language Support")
OPTION(DISABLE_GNUTLS        "Disable SSLv3/TLS connection support")
OPTION(DISABLE_LARGEFILE     "Disable Large File Support")
OPTION(DISABLE_ALIAS         "Disable Alias plugin")
OPTION(DISABLE_ASPELL        "Disable Aspell plugin")
OPTION(DISABLE_CHARSET       "Disable Charset plugin")
OPTION(ENABLE_DEMO           "Enable Demo plugin")
OPTION(DISABLE_FIFO          "Disable FIFO plugin")
OPTION(DISABLE_IRC           "Disable IRC plugin")
OPTION(DISABLE_LOGGER        "Disable Logger plugin")
OPTION(ENABLE_RELAY          "Enable Relay plugin")
OPTION(DISABLE_SCRIPTS       "Disable script plugins")
OPTION(DISABLE_PERL          "Disable Perl scripting language")
OPTION(DISABLE_PYTHON        "Disable Python scripting language")
OPTION(DISABLE_RUBY          "Disable Ruby scripting language")
OPTION(DISABLE_LUA           "Disable Lua scripting language")
OPTION(DISABLE_TCL           "Disable Tcl scripting language")
OPTION(DISABLE_XFER          "Disable Xfer plugin (file transfer and direct chat)")
OPTION(DISABLE_DOC           "Disable Doc")

IF(NOT DISABLE_NLS)
  ADD_SUBDIRECTORY( po )
ENDIF(NOT DISABLE_NLS)

ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( doc )

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

# pkgconfig file
SET(PACKAGE "${PROJECT_NAME}")
SET(prefix "${CMAKE_INSTALL_PREFIX}")
SET(exec_prefix "\${prefix}")
SET(libdir "\${exec_prefix}/lib")
SET(includedir "\${prefix}/include")
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/weechat.pc.in ${CMAKE_CURRENT_BINARY_DIR}/weechat.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/weechat.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)

# packages
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Fast, light and extensible chat client")
SET(CPACK_PACKAGE_VENDOR "FlashCode")
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
  "/ltmain\\\\.sh$" "/\\\\.deps/" "/date\\\\.xml$" "/html/" "/html1/"
  "\\\\.pdf$" "/Makefile$" "/Makefile\\\\.in$" "stamp" "/po/.*\\\\.header$"
  "\\\\.gmo$" "~$" "\\\\.o$" "\\\\.lo$" "\\\\.a$" "\\\\.la$" "\\\\.lai$"
  "\\\\.Plo$" "/weechat-curses$" "/weechat-gtk$"
)

INCLUDE(CPack)
