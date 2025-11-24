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

if ("$ENV{LIB_FUZZING_ENGINE}" MATCHES "-fsanitize=.*fuzzer")
  set(ENV{LIB_FUZZING_ENGINE} "$ENV{LIB_FUZZING_ENGINE};-fsanitize=fuzzer-no-link")
endif()

remove_definitions(-DHAVE_CONFIG_H)
include_directories(
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

list(APPEND FUZZ_TARGET_LINK
  weechat_core
  weechat_plugins
  weechat_gui_common
  weechat_gui_headless
  weechat_ncurses_fake
  ${EXTRA_LIBS}
  -rdynamic
)

# fuzz targets
set(FUZZ_CORE_TARGETS calc eval crypto secure string utf8 util)

foreach(fuzz_target ${FUZZ_CORE_TARGETS})
  add_executable(weechat_core_${fuzz_target}_fuzzer core/${fuzz_target}-fuzzer.cc)
  target_link_libraries(weechat_core_${fuzz_target}_fuzzer ${FUZZ_TARGET_LINK} coverage_config)
  set_target_properties(weechat_core_${fuzz_target}_fuzzer PROPERTIES LINK_OPTIONS "$ENV{LIB_FUZZING_ENGINE}")
endforeach()
