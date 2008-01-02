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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\" $(PERL_CFLAGS)

libdir = ${weechat_libdir}/plugins

lib_LTLIBRARIES = perl.la

perl_la_SOURCES = perl.c
perl_la_LDFLAGS = -module
perl_la_LIBADD = ../lib_weechat_plugins_scripts.la $(PERL_LFLAGS)
