#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(GTK_CFLAGS)

bin_PROGRAMS = weechat-gtk

# Because of a linker bug, we have to link 2 times with lib_weechat_core.a
# (and it must be 2 different path/names to be kept by linker)
weechat_gtk_LDADD = ./../../core/lib_weechat_core.a \
                    ../../plugins/lib_weechat_plugins.a \
                    ../lib_weechat_gui_common.a \
                    ../../core/lib_weechat_core.a \
                    $(PLUGINS_LFLAGS) \
                    $(GTK_LFLAGS) \
                    $(GCRYPT_LFLAGS) \
                    $(GNUTLS_LFLAGS) \
                    $(CURL_LFLAGS)

weechat_gtk_SOURCES = gui-gtk-bar-window.c \
                      gui-gtk-chat.c \
                      gui-gtk-color.c \
                      gui-gtk-key.c \
                      gui-gtk-main.c \
                      gui-gtk-mouse.c \
                      gui-gtk-term.c \
                      gui-gtk-window.c \
                      gui-gtk.h

EXTRA_DIST = CMakeLists.txt
