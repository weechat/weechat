#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(GCRYPT_CFLAGS) $(GNUTLS_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = irc.la

irc_la_SOURCES = irc.c \
                 irc.h \
                 irc-bar-item.c \
                 irc-bar-item.h \
                 irc-buffer.c \
                 irc-buffer.h \
                 irc-channel.c \
                 irc-channel.h \
                 irc-color.c \
                 irc-color.h \
                 irc-command.c \
                 irc-command.h \
                 irc-completion.c \
                 irc-completion.h \
                 irc-config.c \
                 irc-config.h \
                 irc-ctcp.c \
                 irc-ctcp.h \
                 irc-debug.c \
                 irc-debug.h \
                 irc-ignore.c \
                 irc-ignore.h \
                 irc-info.c \
                 irc-info.h \
                 irc-input.c \
                 irc-input.h \
                 irc-join.c \
                 irc-join.h \
                 irc-message.c \
                 irc-message.h \
                 irc-mode.c \
                 irc-mode.h \
                 irc-modelist.c \
                 irc-modelist.h \
                 irc-msgbuffer.c \
                 irc-msgbuffer.h \
                 irc-nick.c \
                 irc-nick.h \
                 irc-notify.c \
                 irc-notify.h \
                 irc-protocol.c \
                 irc-protocol.h \
                 irc-raw.c \
                 irc-raw.h \
                 irc-redirect.c \
                 irc-redirect.h \
                 irc-sasl.c \
                 irc-sasl.h \
                 irc-server.c \
                 irc-server.h \
                 irc-tag.c \
                 irc-tag.h \
                 irc-typing.c \
                 irc-typing.h \
                 irc-upgrade.c \
                 irc-upgrade.h

irc_la_LDFLAGS = -module -no-undefined
irc_la_LIBADD  = $(IRC_LFLAGS) $(GCRYPT_LFLAGS) $(GNUTLS_LFLAGS)

EXTRA_DIST = CMakeLists.txt
