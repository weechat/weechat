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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(GNUTLS_CFLAGS)

noinst_LIBRARIES = lib_weechat_main.a

lib_weechat_main_a_SOURCES = weechat.c \
                             weechat.h \
                             command.c \
                             command.h \
                             completion.c \
                             completion.h \
                             weelist.c \
                             weelist.h \
                             weeconfig.c \
                             weeconfig.h \
                             history.c \
                             history.h \
                             hotlist.c \
                             hotlist.h \
                             log.c \
                             log.h \
                             fifo.c \
                             fifo.h \
                             session.c \
                             session.h \
                             utf8.c \
                             utf8.h
