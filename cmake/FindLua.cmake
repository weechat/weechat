#
# Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
  pkg_search_module(LUA lua lua5.4 lua-5.4 lua54 lua5.3 lua-5.3 lua53 lua5.2 lua-5.2 lua52 lua5.1 lua-5.1 lua51 lua-5.0 lua5.0 lua50)
endif()
