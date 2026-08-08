# SPDX-FileCopyrightText: 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
# SPDX-FileCopyrightText: 2006-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Lua REQUIRED)

add_library(lua MODULE
  weechat-lua.c weechat-lua.h
  weechat-lua-api.c weechat-lua-api.h
)
set_target_properties(lua PROPERTIES PREFIX "")

add_definitions(${LUA_CFLAGS})
include_directories(${LUA_INCLUDE_DIRS})
target_link_libraries(lua ${LUA_LDFLAGS} weechat_plugins_scripts coverage_config)

install(TARGETS lua LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
