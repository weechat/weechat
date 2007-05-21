# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

ADD_LIBRARY(lua MODULE weechat-lua.c)
SET_TARGET_PROPERTIES(lua PROPERTIES PREFIX "")

IF(LUA_FOUND)
  INCLUDE_DIRECTORIES(${LUA_INCLUDE_PATH} )
  IF(LUA_LIBRARY AND LUALIB_LIBRARY)
    TARGET_LINK_LIBRARIES(lua ${LUA_LIBRARY} ${LUALIB_LIBRARY} weechat_scripts)
  ELSE(LUA_LIBRARY AND LUALIB_LIBRARY)
    TARGET_LINK_LIBRARIES(lua ${LUA_LIBRARY} weechat_scripts)
  ENDIF(LUA_LIBRARY AND LUALIB_LIBRARY)
ENDIF(LUA_FOUND)

INSTALL(TARGETS lua LIBRARY DESTINATION lib/${PROJECT_NAME}/plugins)
