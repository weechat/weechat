# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
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

if PLUGIN_PERL
script_dir = scripts
endif

if PLUGIN_PYTHON
script_dir = scripts
endif

if PLUGIN_RUBY
script_dir = scripts
endif

if PLUGIN_LUA
script_dir = scripts
endif

if PLUGIN_ASPELL
aspell_dir = aspell
endif

if PLUGIN_CHARSET
charset_dir = charset
endif

SUBDIRS = $(script_dir) $(aspell_dir) $(charset_dir)

noinst_LIBRARIES = lib_weechat_plugins.a

lib_weechat_plugins_a_SOURCES = weechat-plugin.h \
				plugins.h \
                                plugins.c \
                                plugins-interface.c \
                                plugins-config.h \
                                plugins-config.c
