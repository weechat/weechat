# Copyright (c) 2003-2006 FlashCode <flashcode@flashtux.org>
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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(LUA_CFLAGS) 

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = lua.la

lua_la_SOURCES = weechat-lua.c
lua_la_LDFLAGS = -module
lua_la_LIBADD  = ../lib_weechat_plugins_scripts.la $(LUA_LFLAGS)
