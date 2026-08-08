# SPDX-FileCopyrightText: 2025-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

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
