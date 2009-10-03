# Copyright (c) 2003-2009 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(NCURSES_CFLAGS)

bin_PROGRAMS = weechat-curses

# Because of a linker bug, we have to link 2 times with lib_weechat_core.a
# (and it must be 2 different path/names to be kept by linker)
weechat_curses_LDADD = ./../../core/lib_weechat_core.a \
                       ../../plugins/lib_weechat_plugins.a \
                       ../lib_weechat_gui_common.a \
                       ../../core/lib_weechat_core.a \
                       $(PLUGINS_LFLAGS) \
                       $(NCURSES_LFLAGS) \
                       $(GNUTLS_LFLAGS) \
                       $(IKSEMEL_LFLAGS)

weechat_curses_SOURCES = gui-curses-bar-window.c \
                         gui-curses-chat.c \
                         gui-curses-color.c \
                         gui-curses-keyboard.c \
                         gui-curses-main.c \
                         gui-curses-window.c \
                         gui-curses.h

EXTRA_DIST = CMakeLists.txt
