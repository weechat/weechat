#
# Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

set(LIB_CORE_SRC
  weechat.c weechat.h
  core-arraylist.c core-arraylist.h
  core-backtrace.c core-backtrace.h
  core-calc.c core-calc.h
  core-command.c core-command.h
  core-completion.c core-completion.h
  core-config.c core-config.h
  core-config-file.c core-config-file.h
  core-crypto.c core-crypto.h
  core-debug.c core-debug.h
  core-dir.c core-dir.h
  core-doc.c core-doc.h
  core-eval.c core-eval.h
  core-hashtable.c core-hashtable.h
  core-hdata.c core-hdata.h
  core-hook.c core-hook.h
  core-infolist.c core-infolist.h
  core-input.c core-input.h
  core-list.c core-list.h
  core-log.c core-log.h
  core-network.c core-network.h
  core-proxy.c core-proxy.h
  core-secure.c core-secure.h
  core-secure-buffer.c core-secure-buffer.h
  core-secure-config.c core-secure-config.h
  core-signal.c core-signal.h
  core-string.c core-string.h
  core-sys.c core-sys.h
  core-upgrade.c core-upgrade.h
  core-upgrade-file.c core-upgrade-file.h
  core-url.c core-url.h
  core-utf8.c core-utf8.h
  core-util.c core-util.h
  core-version.c core-version.h
  hook/hook-command-run.c hook/hook-command-run.h
  hook/hook-command.c hook/hook-command.h
  hook/hook-completion.c hook/hook-completion.h
  hook/hook-config.c hook/hook-config.h
  hook/hook-connect.c hook/hook-connect.h
  hook/hook-fd.c hook/hook-fd.h
  hook/hook-focus.c hook/hook-focus.h
  hook/hook-hdata.c hook/hook-hdata.h
  hook/hook-hsignal.c hook/hook-hsignal.h
  hook/hook-info-hashtable.c hook/hook-info-hashtable.h
  hook/hook-info.c hook/hook-info.h
  hook/hook-infolist.c hook/hook-infolist.h
  hook/hook-line.c hook/hook-line.h
  hook/hook-modifier.c hook/hook-modifier.h
  hook/hook-print.c hook/hook-print.h
  hook/hook-process.c hook/hook-process.h
  hook/hook-signal.c hook/hook-signal.h
  hook/hook-timer.c hook/hook-timer.h
  hook/hook-url.c hook/hook-url.h
)

# Check for flock support
include(CheckSymbolExists)
check_symbol_exists(flock "sys/file.h" HAVE_FLOCK)

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  find_library(EXECINFO_LIB_PATH execinfo /usr/local/lib)
  set(CMAKE_REQUIRED_LIBRARIES "${EXECINFO_LIB_PATH}")
  check_function_exists(backtrace HAVE_BACKTRACE)
else()
  check_symbol_exists(backtrace "execinfo.h" HAVE_BACKTRACE)
endif()

include_directories(${GNUTLS_INCLUDE_PATH})

include_directories(${CURL_INCLUDE_DIRS})

if(ENABLE_ZSTD)
  include_directories(${LIBZSTD_INCLUDE_DIRS})
endif()

if(ENABLE_CJSON)
  include_directories(${LIBCJSON_INCLUDE_DIRS})
endif()

include_directories("${CMAKE_BINARY_DIR}")
add_library(weechat_core STATIC ${LIB_CORE_SRC})
target_link_libraries(weechat_core coverage_config)
add_dependencies(weechat_core version_git)
