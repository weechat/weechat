# Copyright (c) 2003-2008 FlashCode <flashcode@flashtux.org>
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

weechat_curses_LDADD = ../../core/weechat.o \
                       ../../core/wee-backtrace.o \
                       ../../core/wee-command.o \
                       ../../core/wee-config.o \
                       ../../core/wee-config-file.o \
                       ../../core/wee-hook.o \
                       ../../core/wee-input.o \
                       ../../core/wee-list.o \
                       ../../core/wee-log.o \
                       ../../core/wee-upgrade.o \
                       ../../core/wee-string.o \
                       ../../core/wee-utf8.o \
                       ../../core/wee-util.o \
                       ../../plugins/lib_weechat_plugins.a \
                       ../lib_weechat_gui_common.a \
                       $(PLUGINS_LFLAGS) \
                       $(NCURSES_LFLAGS)

weechat_curses_SOURCES = gui-curses-chat.c \
                         gui-curses-color.c \
                         gui-curses-infobar.c \
                         gui-curses-input.c \
                         gui-curses-keyboard.c \
                         gui-curses-main.c \
                         gui-curses-nicklist.c \
                         gui-curses-status.c \
                         gui-curses-window.c \
                         gui-curses.h
