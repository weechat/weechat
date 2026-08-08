# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# - Find Lua
# This module finds if liblua is installed and determines where
# the include files and libraries are.
#
# This code sets the following variables:
#
#  LUA_INCLUDE_PATH = path to where <lua.h> can be found
#  LUA_LIBRARY = path to where liblua.so* (and liblualib.so* for lua <can be found (on non glibc based systems)
#
#  LUA_FOUND = is liblua usable on system?

if(LUA_FOUND)
  # Already in cache, be silent
  set(LUA_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_search_module(LUA lua lua5.4 lua-5.4 lua54 lua5.3 lua-5.3 lua53)
endif()
