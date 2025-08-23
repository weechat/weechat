/*
SPDX-FileCopyrightText: 2007-2025 Sébastien Helleu <flashcode@flashtux.org>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#cmakedefine01 ENABLE_NCURSES
#cmakedefine01 ENABLE_HEADLESS
#cmakedefine01 ENABLE_NLS
#cmakedefine01 ENABLE_LARGEFILE
#cmakedefine01 ENABLE_ZSTD
#cmakedefine01 ENABLE_CJSON
#cmakedefine01 ENABLE_ALIAS
#cmakedefine01 ENABLE_BUFLIST
#cmakedefine01 ENABLE_CHARSET
#cmakedefine01 ENABLE_EXEC
#cmakedefine01 ENABLE_FIFO
#cmakedefine01 ENABLE_FSET
#cmakedefine01 ENABLE_IRC
#cmakedefine01 ENABLE_LOGGER
#cmakedefine01 ENABLE_RELAY
#cmakedefine01 ENABLE_SCRIPT
#cmakedefine01 ENABLE_SCRIPTS
#cmakedefine01 ENABLE_PERL
#cmakedefine01 ENABLE_PYTHON
#cmakedefine01 ENABLE_RUBY
#cmakedefine01 ENABLE_LUA
#cmakedefine01 ENABLE_TCL
#cmakedefine01 ENABLE_GUILE
#cmakedefine01 ENABLE_JAVASCRIPT
#cmakedefine01 ENABLE_PHP
#cmakedefine01 ENABLE_SPELL
#cmakedefine01 ENABLE_ENCHANT
#cmakedefine01 ENABLE_TRIGGER
#cmakedefine01 ENABLE_TYPING
#cmakedefine01 ENABLE_XFER
#cmakedefine01 ENABLE_MAN
#cmakedefine01 ENABLE_DOC
#cmakedefine01 ENABLE_DOC_INCOMPLETE
#cmakedefine01 ENABLE_TESTS
#cmakedefine01 ENABLE_CODE_COVERAGE

#cmakedefine HAVE_SYS_RESOURCE_H
#cmakedefine HAVE_FLOCK
#cmakedefine HAVE_LANGINFO_CODESET
#cmakedefine HAVE_BACKTRACE
#cmakedefine ICONV_2ARG_IS_CONST 1
#cmakedefine HAVE_MALLINFO
#cmakedefine HAVE_MALLINFO2
#cmakedefine HAVE_MALLOC_H
#cmakedefine HAVE_MALLOC_TRIM
#cmakedefine HAVE_HTONLL
#cmakedefine HAVE_EAT_NEWLINE_GLITCH
#cmakedefine HAVE_ASPELL_VERSION_STRING
#cmakedefine HAVE_GUILE_GMP_MEMORY_FUNCTIONS

#define CMAKE_BUILD_TYPE "@CMAKE_BUILD_TYPE@"
#define CMAKE_INSTALL_PREFIX "@CMAKE_INSTALL_PREFIX@"

#define PACKAGE_VERSION "@VERSION@"
#define PACKAGE "@PROJECT_NAME@"
#define PACKAGE_NAME "@PROJECT_NAME@"
#define PACKAGE_STRING "@PKG_STRING@"
#define WEECHAT_LIBDIR "@WEECHAT_LIBDIR@"
#define WEECHAT_SHAREDIR "@WEECHAT_SHAREDIR@"
#define LOCALEDIR "@LOCALEDIR@"
#define WEECHAT_HOME "@WEECHAT_HOME@"
#define _GNU_SOURCE 1
