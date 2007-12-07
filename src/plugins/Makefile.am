# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
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

noinst_LIBRARIES = lib_weechat_plugins.a

lib_weechat_plugins_a_SOURCES = weechat-plugin.h \
				plugin.c \
                                plugin.h \
                                plugin-api.c \
                                plugin-api.h \
                                plugin-config.c \
                                plugin-config.h \
                                plugin-infolist.c \
                                plugin-infolist.h

if PLUGIN_IRC
irc_dir = irc
endif

if PLUGIN_ALIAS
alias_dir = alias
endif

if PLUGIN_ASPELL
aspell_dir = aspell
endif

if PLUGIN_CHARSET
charset_dir = charset
endif

if PLUGIN_FIFO
fifo_dir = fifo
endif

if PLUGIN_LOGGER
logger_dir = logger
endif

if PLUGIN_TRIGGER
trigger_dir = trigger
endif

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

if PLUGIN_DEMO
demo_dir = demo
endif

SUBDIRS = . $(irc_dir) $(alias_dir) $(aspell_dir) $(charset_dir) $(fifo_dir) \
            $(logger_dir) $(trigger_dir) $(demo_dir) $(script_dir)
