# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
# SPDX-FileCopyrightText: 2007-2008 Julien Louis <ptitlouis@sysif.net>
# SPDX-FileCopyrightText: 2008-2009 Emmanuel Bouthenot <kolter@openics.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.18)

project(weechat C)

# CMake options
set(CMAKE_VERBOSE_MAKEFILE OFF)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" "${CMAKE_MODULE_PATH}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SKIP_RPATH ON)

# Compiler options
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char -fms-extensions -Wall -Wextra -Werror-implicit-function-declaration -Wformat -Werror=format-security")
# C++ options are not added to CMAKE_CXX_FLAGS: C++ is not enabled yet, and
# this would hide the flags from environment variable CXXFLAGS.
add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fsigned-char;-fms-extensions;-Wall;-Wextra>")
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
  # Extra options specific to GCC
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat-overflow=2 -Wformat-truncation=2")
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-Wformat-overflow=2;-Wformat-truncation=2>")
endif()

# Version
execute_process(COMMAND "${CMAKE_SOURCE_DIR}/version.sh" devel-major OUTPUT_VARIABLE VERSION_MAJOR)
execute_process(COMMAND "${CMAKE_SOURCE_DIR}/version.sh" devel-minor OUTPUT_VARIABLE VERSION_MINOR)
execute_process(COMMAND "${CMAKE_SOURCE_DIR}/version.sh" devel-patch OUTPUT_VARIABLE VERSION_PATCH)
string(REGEX REPLACE "\n" "" VERSION_MAJOR "${VERSION_MAJOR}")
string(REGEX REPLACE "\n" "" VERSION_MINOR "${VERSION_MINOR}")
string(REGEX REPLACE "\n" "" VERSION_PATCH "${VERSION_PATCH}")
if(VERSION_PATCH STREQUAL "")
  set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")
else()
  set(VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
endif()

# Add definitions for version and license.
add_definitions(-DWEECHAT_VERSION="${VERSION}" -DWEECHAT_LICENSE="GPL3")

# Package string
set(PKG_STRING "${PROJECT_NAME} ${VERSION}")
string(REPLACE "\";\"" "\ " PKG_STRING ${PKG_STRING})

if(NOT DEFINED LIBDIR)
  set(LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
endif()

if(NOT DEFINED WEECHAT_LIBDIR)
  set(WEECHAT_LIBDIR "${LIBDIR}/${PROJECT_NAME}")
endif()

if(NOT DEFINED DATAROOTDIR)
  set(DATAROOTDIR "${CMAKE_INSTALL_PREFIX}/share")
endif()

if(NOT DEFINED WEECHAT_SHAREDIR)
  set(WEECHAT_SHAREDIR "${DATAROOTDIR}/weechat")
endif()

if(NOT DEFINED MANDIR)
  set(MANDIR "${DATAROOTDIR}/man")
endif()

if(NOT DEFINED LOCALEDIR)
  set(LOCALEDIR "${DATAROOTDIR}/locale")
endif()

if(DEFINED INCLUDEDIR)
  set(INCLUDEDIR "${INCLUDEDIR}/${PROJECT_NAME}")
else()
  set(INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}")
endif()

# Interfaces and features
option(BUILD_NCURSES              "Build the Ncurses interface"     ON)
option(BUILD_HEADLESS             "Build the headless binary"       ON)
option(ENABLE_NLS                 "Enable Native Language Support"  ON)
option(ENABLE_LARGEFILE           "Enable Large File Support"       ON)
option(ENABLE_ZSTD                "Enable Zstandard compression"    ON)

# Plugins
option(BUILD_PLUGIN_ALIAS         "Build Alias plugin"              ON)
option(BUILD_PLUGIN_BUFLIST       "Build Buflist plugin"            ON)
option(BUILD_PLUGIN_CHARSET       "Build Charset plugin"            ON)
option(BUILD_PLUGIN_EXEC          "Build Exec plugin"               ON)
option(BUILD_PLUGIN_FIFO          "Build FIFO plugin"               ON)
option(BUILD_PLUGIN_FSET          "Build Fast Set plugin"           ON)
option(BUILD_PLUGIN_IRC           "Build IRC plugin"                ON)
option(BUILD_PLUGIN_LOGGER        "Build Logger plugin"             ON)
option(BUILD_PLUGIN_OTR           "Build OTR plugin"                ON)
option(BUILD_PLUGIN_RELAY         "Build Relay plugin"              ON)
option(BUILD_PLUGIN_SCRIPT        "Build Script plugin"             ON)
option(BUILD_SCRIPTING_PLUGINS    "Build all scripting plugins"     ON)
option(BUILD_PLUGIN_PERL          "Build Perl plugin"               ON)
option(BUILD_PLUGIN_PYTHON        "Build Python plugin"             ON)
option(BUILD_PLUGIN_RUBY          "Build Ruby plugin"               ON)
option(BUILD_PLUGIN_LUA           "Build Lua plugin"                ON)
option(BUILD_PLUGIN_TCL           "Build Tcl plugin"                ON)
option(BUILD_PLUGIN_GUILE         "Build Guile (Scheme) plugin"     ON)
option(BUILD_PLUGIN_JAVASCRIPT    "Build JavaScript plugin"         ON)
option(BUILD_PLUGIN_PHP           "Build PHP plugin"                ON)
option(BUILD_PLUGIN_SPELL         "Build Spell checker plugin"      ON)
option(USE_ENCHANT                "Use Enchant lib in Spell plugin" OFF)
option(BUILD_PLUGIN_TRIGGER       "Build Trigger plugin"            ON)
option(BUILD_PLUGIN_TYPING        "Build Typing plugin"             ON)
option(BUILD_PLUGIN_XFER          "Build Xfer plugin"               ON)

# Documentation, tests and code coverage
option(BUILD_MAN                  "Build man page"                  OFF)
option(BUILD_DOC                  "Build documentation"             OFF)
option(BUILD_DOC_ALLOW_INCOMPLETE "Allow incomplete doc"            OFF)
option(BUILD_TESTS                "Build tests"                     OFF)
option(BUILD_FUZZERS              "Build fuzzers"                   OFF)
option(ENABLE_CODE_COVERAGE       "Enable code coverage"            OFF)

# Code coverage
add_library(coverage_config INTERFACE)
if(ENABLE_CODE_COVERAGE)
  if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    # GCC: counters are updated atomically, as some code runs in threads
    # (for example the transfer of hook_url).
    target_compile_options(coverage_config INTERFACE -O0 -g --coverage -fprofile-update=atomic)
    target_link_libraries(coverage_config INTERFACE --coverage)
  elseif(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    # Clang
    target_compile_options(coverage_config INTERFACE -g -fprofile-instr-generate -fcoverage-mapping)
    target_link_libraries(coverage_config INTERFACE -fprofile-instr-generate)
  endif()
endif()

# Headless mode is required for documentation.
if(BUILD_DOC AND NOT BUILD_HEADLESS)
  message(FATAL_ERROR "Headless mode is required to build documentation.")
endif()

# All plugins are required for documentation.
if(BUILD_DOC
    AND NOT BUILD_DOC_ALLOW_INCOMPLETE
    AND (NOT ENABLE_NLS
      OR NOT BUILD_PLUGIN_ALIAS
      OR NOT BUILD_PLUGIN_BUFLIST
      OR NOT BUILD_PLUGIN_CHARSET
      OR NOT BUILD_PLUGIN_EXEC
      OR NOT BUILD_PLUGIN_FIFO
      OR NOT BUILD_PLUGIN_FSET
      OR NOT BUILD_PLUGIN_IRC
      OR NOT BUILD_PLUGIN_LOGGER
      OR NOT BUILD_PLUGIN_OTR
      OR NOT BUILD_PLUGIN_RELAY
      OR NOT BUILD_PLUGIN_SCRIPT
      OR NOT BUILD_SCRIPTING_PLUGINS
      OR NOT BUILD_PLUGIN_PERL
      OR NOT BUILD_PLUGIN_PYTHON
      OR NOT BUILD_PLUGIN_RUBY
      OR NOT BUILD_PLUGIN_LUA
      OR NOT BUILD_PLUGIN_TCL
      OR NOT BUILD_PLUGIN_GUILE
      OR NOT BUILD_PLUGIN_JAVASCRIPT
      OR NOT BUILD_PLUGIN_PHP
      OR NOT BUILD_PLUGIN_SPELL
      OR NOT BUILD_PLUGIN_TRIGGER
      OR NOT BUILD_PLUGIN_TYPING
      OR NOT BUILD_PLUGIN_XFER))
  message(
    FATAL_ERROR
    " All plugins are required to build documentation.\n"
    " If you really want to build incomplete docs, enable this option:\n"
    "   -DBUILD_DOC_ALLOW_INCOMPLETE=ON"
  )
endif()

# Headless mode is required for tests.
if(BUILD_TESTS AND NOT BUILD_HEADLESS)
  message(FATAL_ERROR "Headless mode is required for tests.")
endif()

# Set this to override aspell's dictionaries directory.
if(ASPELL_DICT_DIR)
  add_definitions(-DASPELL_DICT_DIR="${ASPELL_DICT_DIR}")
endif()

# Set this to override the myspell dictionaries directory when using enchant.
if(ENCHANT_MYSPELL_DICT_DIR)
  add_definitions(-DENCHANT_MYSPELL_DICT_DIR="${ENCHANT_MYSPELL_DICT_DIR}")
endif()

# Option WEECHAT_HOME
set(WEECHAT_HOME "${WEECHAT_HOME}" CACHE
  STRING "Force a single WeeChat home directory for config, logs, scripts, etc."
  FORCE)
mark_as_advanced(CLEAR WEECHAT_HOME)

add_definitions(-DHAVE_CONFIG_H)

include(FindPkgConfig)

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)

check_include_files("langinfo.h" HAVE_LANGINFO_CODESET)
check_include_files("sys/resource.h" HAVE_SYS_RESOURCE_H)

check_include_files("malloc.h" HAVE_MALLOC_H)
check_symbol_exists("malloc_trim" "malloc.h" HAVE_MALLOC_TRIM)

check_function_exists(mallinfo HAVE_MALLINFO)
check_function_exists(mallinfo2 HAVE_MALLINFO2)

check_symbol_exists("htonll" "sys/types.h;netinet/in.h;inttypes.h" HAVE_HTONLL)

check_symbol_exists("eat_newline_glitch" "term.h" HAVE_EAT_NEWLINE_GLITCH)

# Functions to close file descriptors (used to run in background);
# _GNU_SOURCE is required by glibc for close_range() (defined in config.h).
set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
check_symbol_exists("close_range" "unistd.h" HAVE_CLOSE_RANGE)
check_symbol_exists("closefrom" "unistd.h" HAVE_CLOSEFROM)
unset(CMAKE_REQUIRED_DEFINITIONS)

# Check if res_init requires libresolv.
check_function_exists(res_init, LIBC_HAS_RES_INIT)
if(NOT LIBC_HAS_RES_INIT)
  find_library(RESOLV_LIBRARY resolv)
  if(RESOLV_LIBRARY)
    check_library_exists("${RESOLV_LIBRARY}" res_init "" LIBRESOLV_HAS_RES_INIT)
    if(LIBRESOLV_HAS_RES_INIT)
      list(APPEND EXTRA_LIBS ${RESOLV_LIBRARY})
    endif()
  endif()
endif()

# Check for Large File Support.
if(ENABLE_LARGEFILE)
  add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE -D_LARGE_FILES)
endif()

# _XPG4_2 is needed for macros like CMSG_SPACE
# __EXTENSIONS__ is needed for constants like NI_MAXHOST and for struct timeval
if(CMAKE_HOST_SOLARIS)
  add_definitions(-D_XPG4_2 -D__EXTENSIONS__)
endif()

# Check for libgcrypt.
pkg_check_modules(LIBGCRYPT REQUIRED libgcrypt>=1.8.0)
include_directories(${LIBGCRYPT_INCLUDE_DIRS})
list(APPEND EXTRA_LIBS ${LIBGCRYPT_LDFLAGS})

# Check for GnuTLS.
pkg_check_modules(GNUTLS REQUIRED gnutls>=3.6.3)
include_directories(${GNUTLS_INCLUDE_DIRS})
list(APPEND EXTRA_LIBS ${GNUTLS_LDFLAGS})

# Check for zlib.
find_package(ZLIB REQUIRED)
list(APPEND EXTRA_LIBS ${ZLIB_LIBRARY})

# Check for zstd.
if(ENABLE_ZSTD)
  pkg_check_modules(LIBZSTD REQUIRED libzstd>=1.4.0)
  include_directories(${LIBZSTD_INCLUDE_DIRS})
  list(APPEND EXTRA_LIBS ${LIBZSTD_LDFLAGS})
  add_definitions(-DHAVE_ZSTD)
endif()

# Check for iconv.
find_package(Iconv)
if(ICONV_FOUND)
  if(ICONV_LIBRARY)
    list(APPEND EXTRA_LIBS ${ICONV_LIBRARY})
  endif()
  add_definitions(-DHAVE_ICONV)
endif()

# Check for CURL
# NOTE: keep version in sync with tools/check_curl_symbols.py
pkg_check_modules(LIBCURL REQUIRED libcurl>=7.68.0)
include_directories(${LIBCURL_INCLUDE_DIRS})
list(APPEND EXTRA_LIBS ${LIBCURL_LDFLAGS})

list(APPEND EXTRA_LIBS ${CMAKE_DL_LIBS})

add_subdirectory(icons)

if(ENABLE_NLS)
  find_package(Gettext REQUIRED)
  if(LIBINTL_LIBRARY)
    list(APPEND EXTRA_LIBS ${LIBINTL_LIBRARY})
  endif()
  find_package(Intl REQUIRED)
  include_directories(${Intl_INCLUDE_DIRS})
  list(APPEND EXTRA_LIBS "${Intl_LIBRARIES}")
  add_subdirectory(po)
else()
  add_custom_target(translations COMMAND true)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  find_library(EXECINFO_LIB_PATH execinfo /usr/local/lib)
  check_function_exists(backtrace HAVE_BACKTRACE)
  list(APPEND EXTRA_LIBS "execinfo")
else()
  check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Haiku")
  list(APPEND EXTRA_LIBS "network")
else()
  list(APPEND EXTRA_LIBS "pthread")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
  list(APPEND EXTRA_LIBS "socket" "nsl")
endif()

list(APPEND EXTRA_LIBS "m")

add_subdirectory(src)
add_subdirectory(doc)

enable_testing()
add_subdirectory(tests)

configure_file(config.h.cmake config.h @ONLY)

# Set the git version in "config-git.h".
add_custom_target(version_git ALL
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/set_git_version.sh" "${CMAKE_CURRENT_SOURCE_DIR}" "${VERSION}" "config-git.h"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY
)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
)

add_custom_target(dist
  "${CMAKE_CURRENT_SOURCE_DIR}/tools/makedist.sh" "${VERSION}" "HEAD" "${CMAKE_CURRENT_BINARY_DIR}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Pkgconfig file
set(PACKAGE "${PROJECT_NAME}")
set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
string(REPLACE "${CMAKE_INSTALL_PREFIX}" "\${prefix}" libdir "${LIBDIR}")
set(includedir "\${prefix}/include")
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/weechat.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/weechat.pc" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/weechat.pc" DESTINATION "${LIBDIR}/pkgconfig")

# Cygport file (used to build Cygwin packages)
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/weechat.cygport.in" "${CMAKE_CURRENT_BINARY_DIR}/weechat-${VERSION}-1.cygport" @ONLY)

# Install some files (only on Cygwin).
if(CYGWIN)
  install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/AUTHORS.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/CHANGELOG.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/CONTRIBUTING.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/UPGRADING.md"
    DESTINATION "${DATAROOTDIR}/doc/${PROJECT_NAME}"
  )
endif()

# Desktop file
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/weechat.desktop" DESTINATION "${DATAROOTDIR}/applications")
