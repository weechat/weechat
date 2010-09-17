#
# Copyright (C) 2003-2010 Sebastien Helleu <flashcode@flashtux.org>
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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = relay.la

relay_la_SOURCES = relay.c \
                   relay.h \
                   relay-buffer.c \
                   relay-buffer.h \
                   relay-client.c \
                   relay-client.h \
                   relay-client-irc.c \
                   relay-client-irc.h \
                   relay-client-weechat.c \
                   relay-client-weechat.h \
                   relay-command.c \
                   relay-command.h \
                   relay-completion.c \
                   relay-completion.h \
                   relay-config.c \
                   relay-config.h \
                   relay-info.c \
                   relay-info.h \
                   relay-raw.c \
                   relay-raw.h \
                   relay-server.c \
                   relay-server.h \
                   relay-upgrade.c \
                   relay-upgrade.h

relay_la_LDFLAGS = -module
relay_la_LIBADD  = $(RELAY_LFLAGS)

EXTRA_DIST = CMakeLists.txt
