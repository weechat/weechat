#
# Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(EXEC_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = exec.la

exec_la_SOURCES = exec.c \
                  exec.h \
                  exec-buffer.c \
                  exec-buffer.h \
                  exec-command.c \
                  exec-command.h \
                  exec-completion.c \
                  exec-completion.h \
                  exec-config.c \
                  exec-config.h

exec_la_LDFLAGS = -module -no-undefined
exec_la_LIBADD  = $(EXEC_LFLAGS)

EXTRA_DIST = CMakeLists.txt
