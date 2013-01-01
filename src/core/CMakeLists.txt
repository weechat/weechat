#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2008 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2009 Emmanuel Bouthenot <kolter@openics.org>
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

SET(LIB_CORE_SRC
weechat.c weechat.h
wee-backtrace.c wee-backtrace.h
wee-command.c wee-command.h
wee-completion.c wee-completion.h
wee-config.c wee-config.h
wee-config-file.c wee-config-file.h
wee-debug.c wee-debug.h
wee-eval.c wee-eval.h
wee-hashtable.c wee-hashtable.h
wee-hdata.c wee-hdata.h
wee-hook.c wee-hook.h
wee-infolist.c wee-infolist.h
wee-input.c wee-input.h
wee-list.c wee-list.h
wee-log.c wee-log.h
wee-network.c wee-network.h
wee-proxy.c wee-proxy.h
wee-string.c wee-string.h
wee-upgrade.c wee-upgrade.h
wee-upgrade-file.c wee-upgrade-file.h
wee-url.c wee-url.h
wee-utf8.c wee-utf8.h
wee-util.c wee-util.h
wee-version.c wee-version.h)

# Check for flock support
INCLUDE(CheckSymbolExists)
CHECK_INCLUDE_FILES("sys/file.h" HAVE_SYS_FILE_H)
CHECK_SYMBOL_EXISTS(flock "sys/file.h" HAVE_FLOCK)

IF(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  FIND_LIBRARY(EXECINFO_LIB_PATH execinfo /usr/local/lib)
  SET(CMAKE_REQUIRED_LIBRARIES "${EXECINFO_LIB_PATH}")
  CHECK_FUNCTION_EXISTS(backtrace HAVE_BACKTRACE)
ELSE(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  CHECK_SYMBOL_EXISTS(backtrace "execinfo.h" HAVE_BACKTRACE)
ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")

IF(GNUTLS_FOUND)
  INCLUDE_DIRECTORIES(${GNUTLS_INCLUDE_PATH})
ENDIF(GNUTLS_FOUND)

INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIRS})

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_core STATIC ${LIB_CORE_SRC})
ADD_DEPENDENCIES(weechat_core version_git)
