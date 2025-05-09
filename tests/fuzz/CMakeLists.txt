#
# SPDX-FileCopyrightText: 2025 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later
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

enable_language(CXX)

if(NOT DEFINED ENV{LIB_FUZZING_ENGINE})
  set(ENV{LIB_FUZZING_ENGINE} "-fsanitize=address,fuzzer")
endif()

remove_definitions(-DHAVE_CONFIG_H)
include_directories(
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(NOT CYGWIN)
  add_definitions(-fPIC)
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" AND HAVE_BACKTRACE)
  list(APPEND EXTRA_LIBS "execinfo")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
  list(APPEND EXTRA_LIBS "socket" "nsl")
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Haiku")
  list(APPEND EXTRA_LIBS "network")
endif()

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Haiku")
  list(APPEND EXTRA_LIBS "pthread")
endif()

if(ICONV_LIBRARY)
  list(APPEND EXTRA_LIBS ${ICONV_LIBRARY})
endif()

if(LIBINTL_LIBRARY)
  list(APPEND EXTRA_LIBS ${LIBINTL_LIBRARY})
endif()

list(APPEND EXTRA_LIBS "m")

list(APPEND EXTRA_LIBS ${ZLIB_LIBRARY})

if(ENABLE_ZSTD)
  list(APPEND EXTRA_LIBS ${LIBZSTD_LDFLAGS})
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # link with resolv lib on macOS
  list(APPEND EXTRA_LIBS "resolv")
endif()

list(APPEND FUZZ_TARGET_LINK
  weechat_core
  weechat_plugins
  weechat_gui_common
  weechat_gui_headless
  weechat_ncurses_fake
  ${EXTRA_LIBS}
  ${ZLIB_LIBRARY}
  ${LIBZSTD_LDFLAGS}
  -rdynamic
)
list(APPEND FUZZ_TARGET_DEPS
  weechat_core
  weechat_plugins
  weechat_gui_common
  weechat_gui_headless
  weechat_ncurses_fake
)

# fuzz targets
set(FUZZ_CORE_TARGETS calc crypto secure string utf8 util)

foreach(fuzz_target ${FUZZ_CORE_TARGETS})
  add_executable(weechat_core_${fuzz_target}_fuzzer core/${fuzz_target}-fuzzer.cc)
  target_link_libraries(weechat_core_${fuzz_target}_fuzzer ${FUZZ_TARGET_LINK} coverage_config)
  set_target_properties(weechat_core_${fuzz_target}_fuzzer PROPERTIES LINK_FLAGS "$ENV{LIB_FUZZING_ENGINE}")
  add_dependencies(weechat_core_${fuzz_target}_fuzzer ${FUZZ_TARGET_DEPS})
endforeach()
