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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

bin_PROGRAMS = weechat-curses

if PLUGINS
weechat_curses_LDADD = ../lib_weechat_gui_common.a \
                       ../../common/lib_weechat_main.a \
                       ../../irc/lib_weechat_irc.a \
                       ../../plugins/lib_weechat_plugins.a \
                       $(PLUGINS_LIBS) \
                       $(NCURSES_LIBS) \
                       $(GNUTLS_LFLAGS)
else
weechat_curses_LDADD = ../lib_weechat_gui_common.a \
                       ../../common/lib_weechat_main.a \
                       ../../irc/lib_weechat_irc.a \
                       $(PLUGINS_LIBS) \
                       $(NCURSES_LIBS) \
                       $(GNUTLS_LFLAGS)
endif

weechat_curses_SOURCES = gui-curses-chat.c \
                         gui-curses-color.c \
                         gui-curses-infobar.c \
                         gui-curses-input.c \
                         gui-curses-keyboard.c \
                         gui-curses-main.c \
                         gui-curses-nicklist.c \
                         gui-curses-status.c \
                         gui-curses-window.c \
                         gui-curses-panel.c \
                         gui-curses.h
