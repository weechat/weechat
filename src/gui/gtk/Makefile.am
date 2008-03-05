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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(GTK_CFLAGS)

bin_PROGRAMS = weechat-gtk

weechat_gtk_LDADD = ../../core/weechat.o \
                    ../../core/wee-backtrace.o \
                    ../../core/wee-command.o \
                    ../../core/wee-config.o \
                    ../../core/wee-config-file.o \
                    ../../core/wee-debug.o \
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
                    $(GTK_LFLAGS)

weechat_gtk_SOURCES = gui-gtk-bar.c \
                      gui-gtk-chat.c \
                      gui-gtk-color.c \
                      gui-gtk-infobar.c \
                      gui-gtk-input.c \
                      gui-gtk-keyboard.c \
                      gui-gtk-main.c \
                      gui-gtk-nicklist.c \
                      gui-gtk-status.c \
                      gui-gtk-window.c \
                      gui-gtk.h
