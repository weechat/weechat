#
# SPDX-FileCopyrightText: 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later
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

enable_language(CXX)

remove_definitions(-DHAVE_CONFIG_H)
include_directories(
  ${CPPUTEST_INCLUDE_DIRS}
  ${PROJECT_BINARY_DIR}
  ${PROJECT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(NOT CYGWIN)
  add_definitions(-fPIC)
endif()

if(ENABLE_PYTHON)
  add_definitions(-DHAVE_PYTHON)
endif()
if(ENABLE_PERL)
  add_definitions(-DHAVE_PERL)
endif()
if(ENABLE_RUBY)
  add_definitions(-DHAVE_RUBY)
endif()
if(ENABLE_LUA)
  add_definitions(-DHAVE_LUA)
endif()
if(ENABLE_TCL)
  add_definitions(-DHAVE_TCL)
endif()
if(ENABLE_GUILE)
  add_definitions(-DHAVE_GUILE)
endif()
if(ENABLE_JAVASCRIPT)
  add_definitions(-DHAVE_JAVASCRIPT)
endif()
if(ENABLE_PHP)
  add_definitions(-DHAVE_PHP)
endif()

# unit tests (core)
set(LIB_WEECHAT_UNIT_TESTS_CORE_SRC
  core/test-core-arraylist.cpp
  core/test-core-calc.cpp
  core/test-core-command.cpp
  core/test-core-config-file.cpp
  core/test-core-crypto.cpp
  core/test-core-dir.cpp
  core/test-core-eval.cpp
  core/test-core-hashtable.cpp
  core/test-core-hdata.cpp
  core/test-core-hook.cpp
  core/test-core-infolist.cpp
  core/test-core-input.cpp
  core/test-core-list.cpp
  core/test-core-network.cpp
  core/test-core-secure.cpp
  core/test-core-signal.cpp
  core/test-core-string.cpp
  core/test-core-url.cpp
  core/test-core-utf8.cpp
  core/test-core-util.cpp
  core/test-core-sys.cpp
  core/hook/test-hook-command.cpp
  core/hook/test-hook-command-run.cpp
  core/hook/test-hook-completion.cpp
  core/hook/test-hook-config.cpp
  core/hook/test-hook-connect.cpp
  core/hook/test-hook-fd.cpp
  core/hook/test-hook-focus.cpp
  core/hook/test-hook-hdata.cpp
  core/hook/test-hook-hsignal.cpp
  core/hook/test-hook-info.cpp
  core/hook/test-hook-info-hashtable.cpp
  core/hook/test-hook-infolist.cpp
  core/hook/test-hook-line.cpp
  core/hook/test-hook-modifier.cpp
  core/hook/test-hook-print.cpp
  core/hook/test-hook-process.cpp
  core/hook/test-hook-signal.cpp
  core/hook/test-hook-timer.cpp
  core/hook/test-hook-url.cpp
  gui/test-gui-bar.cpp
  gui/test-gui-bar-item.cpp
  gui/test-gui-bar-item-custom.cpp
  gui/test-gui-bar-window.cpp
  gui/test-gui-buffer.cpp
  gui/test-gui-chat.cpp
  gui/test-gui-color.cpp
  gui/test-gui-filter.cpp
  gui/test-gui-hotlist.cpp
  gui/test-gui-input.cpp
  gui/test-gui-key.cpp
  gui/test-gui-line.cpp
  gui/test-gui-nick.cpp
  gui/test-gui-nicklist.cpp
  gui/curses/test-gui-curses-mouse.cpp
  scripts/test-scripts.cpp
)
add_library(weechat_unit_tests_core STATIC ${LIB_WEECHAT_UNIT_TESTS_CORE_SRC})

# unit tests (plugins)
set(LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
  plugins/test-plugins.cpp
  plugins/test-plugin-api-info.cpp
  plugins/test-plugin-config.cpp
)

if(ENABLE_ALIAS)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    plugins/alias/test-alias.cpp
  )
endif()

if(ENABLE_IRC)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    plugins/irc/test-irc-batch.cpp
    plugins/irc/test-irc-buffer.cpp
    plugins/irc/test-irc-channel.cpp
    plugins/irc/test-irc-color.cpp
    plugins/irc/test-irc-command.cpp
    plugins/irc/test-irc-config.cpp
    plugins/irc/test-irc-ctcp.cpp
    plugins/irc/test-irc-info.cpp
    plugins/irc/test-irc-ignore.cpp
    plugins/irc/test-irc-join.cpp
    plugins/irc/test-irc-list.cpp
    plugins/irc/test-irc-message.cpp
    plugins/irc/test-irc-mode.cpp
    plugins/irc/test-irc-nick.cpp
    plugins/irc/test-irc-protocol.cpp
    plugins/irc/test-irc-sasl.cpp
    plugins/irc/test-irc-server.cpp
    plugins/irc/test-irc-tag.cpp
  )
endif()

if(ENABLE_LOGGER)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    plugins/logger/test-logger.cpp
    plugins/logger/test-logger-backlog.cpp
    plugins/logger/test-logger-tail.cpp
  )
endif()

if (ENABLE_RELAY)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    plugins/relay/test-relay-auth.cpp
    plugins/relay/test-relay-bar-item.cpp
    plugins/relay/test-relay-http.cpp
    plugins/relay/test-relay-raw.cpp
    plugins/relay/test-relay-remote.cpp
    plugins/relay/test-relay-websocket.cpp
    plugins/relay/irc/test-relay-irc.cpp
  )
  if (ENABLE_CJSON)
    list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
      plugins/relay/api/test-relay-api.cpp
      plugins/relay/api/test-relay-api-msg.cpp
      plugins/relay/api/test-relay-api-protocol.cpp
      plugins/relay/api/remote/test-relay-remote-network.cpp
    )
  endif()
endif()

if(ENABLE_TRIGGER)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    plugins/trigger/test-trigger.cpp
    plugins/trigger/test-trigger-config.cpp
  )
endif()

if(ENABLE_TYPING)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    plugins/typing/test-typing.cpp
    plugins/typing/test-typing-status.cpp
  )
endif()

if(ENABLE_XFER)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    plugins/xfer/test-xfer-file.cpp
    plugins/xfer/test-xfer-network.cpp
  )
endif()

add_library(weechat_unit_tests_plugins MODULE ${LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC})

if(ICONV_LIBRARY)
  list(APPEND EXTRA_LIBS ${ICONV_LIBRARY})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD" OR ${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
  list(APPEND EXTRA_LIBS "intl")
  if(HAVE_BACKTRACE)
    list(APPEND EXTRA_LIBS "execinfo")
  endif()
endif()

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Haiku")
  list(APPEND EXTRA_LIBS "pthread")
endif()

# binary to run tests
set(WEECHAT_TESTS_SRC
  tests.cpp tests.h
  tests-record.cpp tests-record.h
)
add_executable(tests ${WEECHAT_TESTS_SRC})
target_link_libraries(tests
  weechat_core
  weechat_plugins
  weechat_gui_common
  weechat_gui_headless
  weechat_ncurses_fake
  weechat_unit_tests_core
  ${EXTRA_LIBS}
  ${ZLIB_LIBRARY}
  ${LIBZSTD_LDFLAGS}
  ${CPPUTEST_LIBRARIES}
  -rdynamic
)
add_dependencies(tests
  weechat_core
  weechat_plugins
  weechat_gui_common
  weechat_gui_headless
  weechat_ncurses_fake
  weechat_unit_tests_core
)

# test for cmake (ctest)
add_test(NAME unit
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND tests -v
)
set_property(TEST unit PROPERTY
  ENVIRONMENT "WEECHAT_TESTS_ARGS=-p;"
  "WEECHAT_EXTRA_LIBDIR=${PROJECT_BINARY_DIR}/src;"
  "WEECHAT_TESTS_SCRIPTS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/scripts/python;"
  "WEECHAT_TESTS_PLUGINS_LIB=${CMAKE_CURRENT_BINARY_DIR}/libweechat_unit_tests_plugins.so"
)
