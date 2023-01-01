#
# Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
# Copyright (C) 2006-2023 Sébastien Helleu <flashcode@flashtux.org>
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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(ASPELL_CFLAGS) $(ENCHANT_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = spell.la

spell_la_SOURCES = spell.c \
                   spell.h \
                   spell-bar-item.c \
                   spell-bar-item.h \
                   spell-command.c \
                   spell-command.h \
                   spell-completion.c \
                   spell-completion.h \
                   spell-config.c \
                   spell-config.h \
                   spell-info.c \
                   spell-info.h \
                   spell-speller.c \
                   spell-speller.h
spell_la_LDFLAGS = -module -no-undefined
spell_la_LIBADD  = $(ASPELL_LFLAGS) $(ENCHANT_LIBS)

EXTRA_DIST = CMakeLists.txt
