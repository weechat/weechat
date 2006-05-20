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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(GTK_CFLAGS)

bin_PROGRAMS = weechat-gtk

if PLUGINS
weechat_gtk_LDADD = ../lib_weechat_gui_common.a \
                    ../../common/lib_weechat_main.a \
                    ../../irc/lib_weechat_irc.a \
                    ../../plugins/lib_weechat_plugins.a \
                    $(PLUGINS_LIBS) \
                    $(GTK_LIBS) \
                    $(GNUTLS_LFLAGS)
else
weechat_gtk_LDADD = ../lib_weechat_gui_common.a \
                    ../../common/lib_weechat_main.a \
                    ../../irc/lib_weechat_irc.a \
                    $(PLUGINS_LIBS) \
                    $(GTK_LIBS) \
                    $(GNUTLS_LFLAGS)
endif

weechat_gtk_SOURCES = gui-gtk-chat.c \
                      gui-gtk-color.c \
                      gui-gtk-infobar.c \
                      gui-gtk-input.c \
                      gui-gtk-keyboard.c \
                      gui-gtk-main.c \
                      gui-gtk-nicklist.c \
                      gui-gtk-status.c \
                      gui-gtk-window.c \
                      gui-gtk-panel.c \
                      gui-gtk.h
