# Copyright (c) 2003-2005 FlashCode <flashcode@flashtux.org>
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

noinst_LIBRARIES = lib_weechat_gui_common.a

lib_weechat_gui_common_a_SOURCES = gui-common.c \
                                   gui.h

if GUI_NCURSES
curses_dir=curses
endif

if GUI_GTK
gtk_dir=gtk
endif

if GUI_QT
qt_dir=qt
endif

SUBDIRS = . $(curses_dir) $(gtk_dir) $(qt_dir)
