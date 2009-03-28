# Copyright (c) 2009 FlashCode <flashcode@flashtux.org>
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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(GNUTLS_CFLAGS) $(IKSEMEL_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = jabber.la

jabber_la_SOURCES = jabber.c \
                    jabber.h \
                    jabber-bar-item.c \
                    jabber-bar-item.h \
                    jabber-buddy.c \
                    jabber-buddy.h \
                    jabber-buffer.c \
                    jabber-buffer.h \
                    jabber-command.c \
                    jabber-command.h \
                    jabber-completion.c \
                    jabber-completion.h \
                    jabber-config.c \
                    jabber-config.h \
                    jabber-debug.c \
                    jabber-debug.h \
                    jabber-display.c \
                    jabber-display.h \
                    jabber-info.c \
                    jabber-info.h \
                    jabber-input.c \
                    jabber-input.h \
                    jabber-muc.c \
                    jabber-muc.h \
                    jabber-server.c \
                    jabber-server.h \
                    jabber-upgrade.c \
                    jabber-upgrade.h \
                    jabber-xmpp.c \
                    jabber-xmpp.h

jabber_la_LDFLAGS = -module
jabber_la_LIBADD  = $(JABBER_LFLAGS) $(GNUTLS_LFLAGS) $(IKSEMEL_LFLAGS)

EXTRA_DIST = CMakeLists.txt
