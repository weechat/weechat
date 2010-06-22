#
# Copyright (C) 2003-2010 Sebastien Helleu <flashcode@flashtux.org>
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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

noinst_LTLIBRARIES = lib_weechat_plugins_scripts.la

lib_weechat_plugins_scripts_la_SOURCES = script.c \
                                         script.h \
                                         script-callback.c \
                                         script-callback.h \
                                         script-api.c \
                                         script-api.h

if PLUGIN_PERL
perl_dir = perl
endif

if PLUGIN_PYTHON
python_dir = python
endif

if PLUGIN_RUBY
ruby_dir = ruby
endif

if PLUGIN_LUA
lua_dir = lua
endif

if PLUGIN_TCL
tcl_dir = tcl
endif

SUBDIRS = . $(perl_dir) $(python_dir) $(ruby_dir) $(lua_dir) $(tcl_dir)

EXTRA_DIST = CMakeLists.txt
