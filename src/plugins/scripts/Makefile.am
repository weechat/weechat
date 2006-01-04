# Copyright (c) 2003-2006 FlashCode <flashcode@flashtux.org>
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
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

noinst_LTLIBRARIES = lib_weechat_plugins_scripts.la

lib_weechat_plugins_scripts_la_SOURCES = weechat-script.h \
                                         weechat-script.c

if PLUGIN_PERL
perl_dir = perl
endif

if PLUGIN_PYTHON
python_dir = python
endif

if PLUGIN_RUBY
ruby_dir = ruby
endif

SUBDIRS = . $(perl_dir) $(python_dir) $(ruby_dir)
