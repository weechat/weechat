# SPDX-FileCopyrightText: 2015-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(V8 REQUIRED)

enable_language(CXX)

add_library(javascript MODULE
  weechat-js.cpp weechat-js.h
  weechat-js-v8.cpp weechat-js-v8.h
  weechat-js-api.cpp weechat-js-api.h
)
set_target_properties(javascript PROPERTIES PREFIX "")

include_directories(${V8_INCLUDE_DIR})
target_link_libraries(javascript ${V8_LIBRARY} weechat_plugins_scripts coverage_config)

install(TARGETS javascript LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
