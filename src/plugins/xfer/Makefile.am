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

lib_LTLIBRARIES = xfer.la

xfer_la_SOURCES = xfer.c \
                  xfer.h \
                  xfer-buffer.c \
                  xfer-buffer.h \
                  xfer-chat.c \
                  xfer-chat.h \
                  xfer-command.c \
                  xfer-command.h \
                  xfer-config.c \
                  xfer-config.h \
                  xfer-dcc.c \
                  xfer-dcc.h \
                  xfer-file.c \
                  xfer-file.h \
                  xfer-network.c \
                  xfer-network.h \
                  xfer-upgrade.c \
                  xfer-upgrade.h

xfer_la_LDFLAGS = -module
xfer_la_LIBADD  = $(XFER_LFLAGS)
