#
# Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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

AM_CPPFLAGS = -DLOCALEDIR=\"$(datadir)/locale\" $(CPPUTEST_CFLAGS) -I$(abs_top_srcdir)

noinst_LIBRARIES = lib_weechat_unit_tests_core.a

lib_weechat_unit_tests_core_a_SOURCES = unit/core/test-core-arraylist.cpp \
                                        unit/core/test-core-calc.cpp \
                                        unit/core/test-core-command.cpp \
                                        unit/core/test-core-config-file.cpp \
                                        unit/core/test-core-crypto.cpp \
                                        unit/core/test-core-dir.cpp \
                                        unit/core/test-core-eval.cpp \
                                        unit/core/test-core-hashtable.cpp \
                                        unit/core/test-core-hdata.cpp \
                                        unit/core/test-core-hook.cpp \
                                        unit/core/test-core-infolist.cpp \
                                        unit/core/test-core-list.cpp \
                                        unit/core/test-core-network.cpp \
                                        unit/core/test-core-secure.cpp \
                                        unit/core/test-core-signal.cpp \
                                        unit/core/test-core-string.cpp \
                                        unit/core/test-core-url.cpp \
                                        unit/core/test-core-utf8.cpp \
                                        unit/core/test-core-util.cpp \
                                        unit/gui/test-gui-bar-item.cpp \
                                        unit/gui/test-gui-bar-item-custom.cpp \
                                        unit/gui/test-gui-bar-window.cpp \
                                        unit/gui/test-gui-buffer.cpp \
                                        unit/gui/test-gui-chat.cpp \
                                        unit/gui/test-gui-color.cpp \
                                        unit/gui/test-gui-filter.cpp \
                                        unit/gui/test-gui-input.cpp \
                                        unit/gui/test-gui-line.cpp \
                                        unit/gui/test-gui-nick.cpp \
                                        scripts/test-scripts.cpp

noinst_PROGRAMS = tests

# Due to circular references, we must link two times with libweechat_core.a
# (and it must be 2 different path/names to be kept by linker)
tests_LDADD = ./../src/core/lib_weechat_core.a \
              ../src/plugins/lib_weechat_plugins.a \
              ../src/gui/lib_weechat_gui_common.a \
              ../src/gui/curses/headless/lib_weechat_gui_headless.a \
              ../src/gui/curses/headless/lib_weechat_ncurses_fake.a \
              lib_weechat_unit_tests_core.a \
              ../src/core/lib_weechat_core.a \
              $(PLUGINS_LFLAGS) \
              $(GCRYPT_LFLAGS) \
              $(GNUTLS_LFLAGS) \
              $(CURL_LFLAGS) \
              $(ZLIB_LFLAGS) \
              $(ZSTD_LFLAGS) \
              $(CPPUTEST_LFLAGS) \
              -lm
tests_LDFLAGS = -rdynamic

tests_SOURCES = tests.cpp \
                tests.h

lib_LTLIBRARIES = lib_weechat_unit_tests_plugins.la

if PLUGIN_IRC
tests_irc = unit/plugins/irc/test-irc-buffer.cpp \
            unit/plugins/irc/test-irc-channel.cpp \
            unit/plugins/irc/test-irc-color.cpp \
            unit/plugins/irc/test-irc-config.cpp \
            unit/plugins/irc/test-irc-ignore.cpp \
            unit/plugins/irc/test-irc-join.cpp \
            unit/plugins/irc/test-irc-message.cpp \
            unit/plugins/irc/test-irc-mode.cpp \
            unit/plugins/irc/test-irc-nick.cpp \
            unit/plugins/irc/test-irc-protocol.cpp \
            unit/plugins/irc/test-irc-sasl.cpp \
            unit/plugins/irc/test-irc-server.cpp \
            unit/plugins/irc/test-irc-tag.cpp
endif

if PLUGIN_LOGGER
tests_logger = unit/plugins/logger/test-logger-backlog.cpp
endif

if PLUGIN_RELAY
tests_relay = unit/plugins/relay/test-relay-auth.cpp
endif

if PLUGIN_TRIGGER
tests_trigger = unit/plugins/trigger/test-trigger.cpp \
                unit/plugins/trigger/test-trigger-config.cpp
endif

if PLUGIN_TYPING
tests_typing = unit/plugins/typing/test-typing.cpp \
               unit/plugins/typing/test-typing-status.cpp
endif

if PLUGIN_XFER
tests_xfer = unit/plugins/xfer/test-xfer-file.cpp \
             unit/plugins/xfer/test-xfer-network.cpp
endif

lib_weechat_unit_tests_plugins_la_SOURCES = unit/plugins/test-plugins.cpp \
                                            $(tests_irc) \
                                            $(tests_logger) \
                                            $(tests_relay) \
                                            $(tests_trigger) \
                                            $(tests_typing) \
                                            $(tests_xfer)

lib_weechat_unit_tests_plugins_la_LDFLAGS = -module -no-undefined

EXTRA_DIST = CMakeLists.txt
