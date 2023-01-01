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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(PERL_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = perl.la

perl_la_SOURCES = weechat-perl.c \
                  weechat-perl.h \
                  weechat-perl-api.c \
                  weechat-perl-api.h
perl_la_LDFLAGS = -module -no-undefined
perl_la_LIBADD = ../lib_weechat_plugins_scripts.la $(PERL_LFLAGS)

EXTRA_DIST = CMakeLists.txt
