# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

noinst_LIBRARIES = lib_weechat_gui_common.a

lib_weechat_gui_common_a_SOURCES = gui-action.c \
                                   gui-action.h \
                                   gui-buffer.c \
                                   gui-buffer.h \
                                   gui-chat.c \
                                   gui-chat.h \
                                   gui-color.c \
                                   gui-color.h \
                                   gui-completion.c \
                                   gui-completion.h \
                                   gui-history.c \
                                   gui-history.h \
                                   gui-hotlist.c \
                                   gui-hotlist.h \
                                   gui-infobar.c \
                                   gui-infobar.h \
                                   gui-input.c \
                                   gui-input.h \
                                   gui-keyboard.c \
                                   gui-keyboard.h \
                                   gui-log.c \
                                   gui-log.h \
                                   gui-main.h \
                                   gui-status.h \
                                   gui-window.c \
                                   gui-window.h \
                                   gui-nicklist.c \
                                   gui-nicklist.h

if GUI_NCURSES
curses_dir=curses
endif

if GUI_WXWIDGETS
wxwidgets_dir=wxwidgets
endif

if GUI_GTK
gtk_dir=gtk
endif

if GUI_QT
qt_dir=qt
endif

SUBDIRS = . $(curses_dir) $(wxwidgets_dir) $(gtk_dir) $(qt_dir)
