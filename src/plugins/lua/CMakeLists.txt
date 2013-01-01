#
# Copyright (C) 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
# Copyright (C) 2006-2013 Sebastien Helleu <flashcode@flashtux.org>
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

ADD_LIBRARY(lua MODULE weechat-lua.c weechat-lua.h weechat-lua-api.c
weechat-lua-api.h)

SET_TARGET_PROPERTIES(lua PROPERTIES PREFIX "")

IF(LUA_FOUND)
  INCLUDE_DIRECTORIES(${LUA_INCLUDE_PATH} )
  IF(LUA_LIBRARY AND LUALIB_LIBRARY)
    TARGET_LINK_LIBRARIES(lua ${LUA_LIBRARY} ${LUALIB_LIBRARY} weechat_plugins_scripts)
  ELSE(LUA_LIBRARY AND LUALIB_LIBRARY)
    TARGET_LINK_LIBRARIES(lua ${LUA_LIBRARY} weechat_plugins_scripts)
  ENDIF(LUA_LIBRARY AND LUALIB_LIBRARY)
ENDIF(LUA_FOUND)

INSTALL(TARGETS lua LIBRARY DESTINATION ${LIBDIR}/plugins)
