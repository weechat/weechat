# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Ruby REQUIRED)

add_library(ruby MODULE
  weechat-ruby.c weechat-ruby.h
  weechat-ruby-api.c weechat-ruby-api.h
)
set_target_properties(ruby PROPERTIES PREFIX "")

# Temporary fix: ignore all warnings on unused parameters due to warnings
# caused by Ruby headers (with Ruby ≥ 3.1.0).
# See: https://github.com/ruby/ruby/pull/7085
add_definitions(-Wno-unused-parameter)
include_directories(${Ruby_INCLUDE_DIRS})
target_link_libraries(ruby ${Ruby_LIBRARIES} weechat_plugins_scripts coverage_config)

install(TARGETS ruby LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
