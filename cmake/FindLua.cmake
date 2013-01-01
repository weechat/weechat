#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
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

IF(LUA_FOUND)
   # Already in cache, be silent
   SET(LUA_FIND_QUIETLY TRUE)
ENDIF(LUA_FOUND)

FIND_PATH(
        LUA51_INCLUDE_PATH lua.h
        PATHS /usr/include /usr/local/include /usr/pkg/include
        PATH_SUFFIXES lua51 lua5.1 lua-5.1
)

FIND_LIBRARY(
        LUA51_LIBRARY NAMES lua51 lua5.1 lua-5.1 lua
        PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
        PATH_SUFFIXES lua51 lua5.1 lua-5.1
)

IF(LUA51_INCLUDE_PATH AND LUA51_LIBRARY)
  SET(LUA_INCLUDE_PATH "${LUA51_INCLUDE_PATH}")
  SET(LUA_LIBRARY "${LUA51_LIBRARY}")
  SET(LUA_VERSION "5.1")
  SET(LUA_FOUND TRUE)
ELSE(LUA51_INCLUDE_PATH AND LUA51_LIBRARY)
  FIND_PATH(
        LUA50_INCLUDE_PATH lua.h
        PATHS /usr/include /usr/local/include /usr/pkg/include
        PATH_SUFFIXES lua50 lua5.0 lua-5.0 lua
  )

  FIND_LIBRARY(
        LUA50_LIBRARY NAMES lua50 lua5.0 lua-5.0 lua
        PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
        PATH_SUFFIXES lua50 lua5.0 lua-5.0 lua
  )

  FIND_LIBRARY(
        LUALIB50_LIBRARY NAMES lualib50 lualib5.0 lualib-5.0 lualib
        PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
        PATH_SUFFIXES lua50 lua5.0 lua-5.0 lua
  )

  IF(LUA50_INCLUDE_PATH AND LUA50_LIBRARY AND LUALIB50_LIBRARY)
    SET(LUA_INCLUDE_PATH "${LUA50_INCLUDE_PATH}")
    SET(LUA_LIBRARY "${LUA50_LIBRARY}")
    SET(LUALIB_LIBRARY "${LUALIB50_LIBRARY}")
    SET(LUA_VERSION "5.0")
    SET(LUA_FOUND TRUE)
  ENDIF(LUA50_INCLUDE_PATH AND LUA50_LIBRARY AND LUALIB50_LIBRARY)
ENDIF(LUA51_INCLUDE_PATH AND LUA51_LIBRARY)


MARK_AS_ADVANCED(
  LUA_INCLUDE_PATH
  LUA_LIBRARY
  LUALIB_LIBRARY
#  LUA_VERSION
)
