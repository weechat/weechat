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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(PYTHON_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = python.la

python_la_SOURCES = weechat-python.c \
                    weechat-python.h \
                    weechat-python-api.c \
                    weechat-python-api.h
python_la_LDFLAGS = -module -no-undefined
python_la_LIBADD  = ../lib_weechat_plugins_scripts.la $(PYTHON_LIBS)

EXTRA_DIST = CMakeLists.txt
