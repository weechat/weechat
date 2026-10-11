# SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(V8 11.3 REQUIRED)

enable_language(CXX)

add_library(javascript MODULE
  weechat-js.cpp weechat-js.h
  weechat-js-v8.cpp weechat-js-v8.h
  weechat-js-api.cpp weechat-js-api.h
)
set_target_properties(javascript PROPERTIES
  PREFIX ""
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

# V8 can not be initialized again in the same process, so the plugin
# must never be unloaded from memory.
if(NOT APPLE)
  target_link_options(javascript PRIVATE "-Wl,-z,nodelete")
endif()

target_include_directories(javascript SYSTEM PRIVATE ${V8_INCLUDE_DIR})
target_link_libraries(javascript ${V8_LIBRARIES} weechat_plugins_scripts coverage_config)

install(TARGETS javascript LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
