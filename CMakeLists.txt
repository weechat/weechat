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

# CMake options
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_SKIP_RPATH ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror-implicit-function-declaration")
if(PREFIX)
  set(CMAKE_INSTALL_PREFIX ${PREFIX} CACHE PATH "Install path prefix" FORCE)
endif()

# version
set(VERSION_MAJOR "1")
set(VERSION_MINOR "1-dev")
set(VERSION_PATCH "")
if(VERSION_PATCH STREQUAL "")
  set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
else()
  set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
endif()

# license
set(LICENSE "GPL3")

# package string
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
option(ENABLE_TESTS     "Enable tests"                              OFF)

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
  add_definitions(-DHAVE_ICONV)
endif()

# Check for CURL
find_package(CURL REQUIRED)

# weechat_gui_common MUST be the first lib in the list
set(STATIC_LIBS weechat_gui_common)

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

if(ENABLE_NLS)
  add_subdirectory(po)
endif()

add_subdirectory(src)
add_subdirectory(doc)

if(ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

configure_file(config.h.cmake config.h @ONLY)

# set the git version in "config-git.h"
add_custom_target(version_git ALL
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/git-version.sh" "${CMAKE_CURRENT_SOURCE_DIR}" "${VERSION}" "config-git.h"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

add_custom_target(dist
  "${CMAKE_CURRENT_SOURCE_DIR}/makedist.sh" "${VERSION}" "HEAD" "${CMAKE_CURRENT_BINARY_DIR}"
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

# install some files (only on Cygwin)
if(CYGWIN)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/AUTHORS.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/ChangeLog.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/Contributing.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/README.asciidoc
    ${CMAKE_CURRENT_SOURCE_DIR}/ReleaseNotes.asciidoc
    DESTINATION ${SHAREDIR}/doc/${PROJECT_NAME})
endif()

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
