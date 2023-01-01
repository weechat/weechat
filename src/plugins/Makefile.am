#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2006-2009 Emmanuel Bouthenot <kolter@openics.org>
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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\"

noinst_LIBRARIES = lib_weechat_plugins.a

lib_weechat_plugins_a_SOURCES = weechat-plugin.h \
                                plugin.c \
                                plugin.h \
                                plugin-api.c \
                                plugin-api.h \
                                plugin-api-info.c \
                                plugin-api-info.h \
                                plugin-config.c \
                                plugin-config.h

noinst_LTLIBRARIES = lib_weechat_plugins_scripts.la

lib_weechat_plugins_scripts_la_SOURCES = plugin-script.c \
                                         plugin-script.h \
                                         plugin-script-api.c \
                                         plugin-script-api.h \
                                         plugin-script-config.c \
                                         plugin-script-config.h

if PLUGIN_ALIAS
alias_dir = alias
endif

if PLUGIN_BUFLIST
buflist_dir = buflist
endif

if PLUGIN_CHARSET
charset_dir = charset
endif

if PLUGIN_EXEC
exec_dir = exec
endif

if PLUGIN_FIFO
fifo_dir = fifo
endif

if PLUGIN_FSET
fset_dir = fset
endif

if PLUGIN_IRC
irc_dir = irc
endif

if PLUGIN_LOGGER
logger_dir = logger
endif

if PLUGIN_RELAY
relay_dir = relay
endif

if PLUGIN_SCRIPT
script_dir = script
endif

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

if PLUGIN_GUILE
guile_dir = guile
endif

if PLUGIN_JAVASCRIPT
javascript_dir = javascript
endif

if PLUGIN_PHP
php_dir = php
endif

if PLUGIN_SPELL
spell_dir = spell
endif

if PLUGIN_TRIGGER
trigger_dir = trigger
endif

if PLUGIN_TYPING
typing_dir = typing
endif

if PLUGIN_XFER
xfer_dir = xfer
endif

SUBDIRS = . \
          $(alias_dir) \
          $(buflist_dir) \
          $(charset_dir) \
          $(exec_dir) \
          $(fifo_dir) \
          $(fset_dir) \
          $(irc_dir) \
          $(logger_dir) \
          $(relay_dir) \
          $(script_dir) \
          $(perl_dir) \
          $(python_dir) \
          $(ruby_dir) \
          $(lua_dir) \
          $(tcl_dir) \
          $(guile_dir) \
          $(javascript_dir) \
          $(php_dir) \
          $(spell_dir) \
          $(trigger_dir) \
          $(typing_dir) \
          $(xfer_dir)

EXTRA_DIST = CMakeLists.txt

# install plugin header

install-data-hook:
	$(INSTALL) -d $(DESTDIR)/$(includedir)/$(PACKAGE)
	$(INSTALL_DATA) $(abs_top_srcdir)/src/plugins/weechat-plugin.h $(DESTDIR)/$(includedir)/$(PACKAGE)

# uninstall plugin header

uninstall-hook:
	$(RM) $(DESTDIR)/$(includedir)/$(PACKAGE)/weechat-plugin.h
