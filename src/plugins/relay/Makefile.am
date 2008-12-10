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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = relay.la

relay_la_SOURCES = relay.c \
                   relay.h \
                   relay-buffer.c \
                   relay-buffer.h \
                   relay-client.c \
                   relay-client.h \
                   relay-command.c \
                   relay-command.h \
                   relay-config.c \
                   relay-config.h \
                   relay-info.c \
                   relay-info.h \
                   relay-network.c \
                   relay-network.h \
                   relay-upgrade.c \
                   relay-upgrade.h

relau_la_LDFLAGS = -module
relay_la_LIBADD  = $(RELAY_LFLAGS)
