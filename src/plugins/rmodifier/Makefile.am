#
# Copyright (C) 2010-2013 Sebastien Helleu <flashcode@flashtux.org>
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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(RMODIFIER_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = rmodifier.la

rmodifier_la_SOURCES = rmodifier.c \
                       rmodifier.h \
                       rmodifier-command.c \
                       rmodifier-command.h \
                       rmodifier-completion.c \
                       rmodifier-completion.h \
                       rmodifier-config.c \
                       rmodifier-config.h \
                       rmodifier-debug.c \
                       rmodifier-debug.h \
                       rmodifier-info.c \
                       rmodifier-info.h
rmodifier_la_LDFLAGS = -module
rmodifier_la_LIBADD  = $(RMODIFIER_LFLAGS)

EXTRA_DIST = CMakeLists.txt
