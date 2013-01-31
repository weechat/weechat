#
# Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
# Copyright (C) 2006-2013 Sebastien Helleu <flashcode@flashtux.org>
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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(ASPELL_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = aspell.la

aspell_la_SOURCES = weechat-aspell.c \
                    weechat-aspell.h \
                    weechat-aspell-bar-item.c \
                    weechat-aspell-bar-item.h \
                    weechat-aspell-command.c \
                    weechat-aspell-command.h \
                    weechat-aspell-completion.c \
                    weechat-aspell-completion.h \
                    weechat-aspell-config.c \
                    weechat-aspell-config.h \
                    weechat-aspell-info.c \
                    weechat-aspell-info.h \
                    weechat-aspell-speller.c \
                    weechat-aspell-speller.h
aspell_la_LDFLAGS = -module
aspell_la_LIBADD  = $(ASPELL_LFLAGS)

EXTRA_DIST = CMakeLists.txt
