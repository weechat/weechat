#
# Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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
include_directories(${CPPUTEST_INCLUDE_DIRS} ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

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
  unit/core/test-core-arraylist.cpp
  unit/core/test-core-calc.cpp
  unit/core/test-core-command.cpp
  unit/core/test-core-config-file.cpp
  unit/core/test-core-crypto.cpp
  unit/core/test-core-dir.cpp
  unit/core/test-core-eval.cpp
  unit/core/test-core-hashtable.cpp
  unit/core/test-core-hdata.cpp
  unit/core/test-core-hook.cpp
  unit/core/test-core-infolist.cpp
  unit/core/test-core-input.cpp
  unit/core/test-core-list.cpp
  unit/core/test-core-network.cpp
  unit/core/test-core-secure.cpp
  unit/core/test-core-signal.cpp
  unit/core/test-core-string.cpp
  unit/core/test-core-url.cpp
  unit/core/test-core-utf8.cpp
  unit/core/test-core-util.cpp
  unit/core/test-core-sys.cpp
  unit/core/hook/test-hook-command.cpp
  unit/core/hook/test-hook-command-run.cpp
  unit/core/hook/test-hook-completion.cpp
  unit/core/hook/test-hook-config.cpp
  unit/core/hook/test-hook-connect.cpp
  unit/core/hook/test-hook-fd.cpp
  unit/core/hook/test-hook-focus.cpp
  unit/core/hook/test-hook-hdata.cpp
  unit/core/hook/test-hook-hsignal.cpp
  unit/core/hook/test-hook-info.cpp
  unit/core/hook/test-hook-info-hashtable.cpp
  unit/core/hook/test-hook-infolist.cpp
  unit/core/hook/test-hook-line.cpp
  unit/core/hook/test-hook-modifier.cpp
  unit/core/hook/test-hook-print.cpp
  unit/core/hook/test-hook-process.cpp
  unit/core/hook/test-hook-signal.cpp
  unit/core/hook/test-hook-timer.cpp
  unit/core/hook/test-hook-url.cpp
  unit/gui/test-gui-bar.cpp
  unit/gui/test-gui-bar-item.cpp
  unit/gui/test-gui-bar-item-custom.cpp
  unit/gui/test-gui-bar-window.cpp
  unit/gui/test-gui-buffer.cpp
  unit/gui/test-gui-chat.cpp
  unit/gui/test-gui-color.cpp
  unit/gui/test-gui-filter.cpp
  unit/gui/test-gui-hotlist.cpp
  unit/gui/test-gui-input.cpp
  unit/gui/test-gui-key.cpp
  unit/gui/test-gui-line.cpp
  unit/gui/test-gui-nick.cpp
  unit/gui/test-gui-nicklist.cpp
  unit/gui/curses/test-gui-curses-mouse.cpp
  scripts/test-scripts.cpp
)
add_library(weechat_unit_tests_core STATIC ${LIB_WEECHAT_UNIT_TESTS_CORE_SRC})

# unit tests (plugins)
set(LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
  unit/plugins/test-plugins.cpp
  unit/plugins/test-plugin-api-info.cpp
  unit/plugins/test-plugin-config.cpp
)

if(ENABLE_ALIAS)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    unit/plugins/alias/test-alias.cpp
  )
endif()

if(ENABLE_IRC)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    unit/plugins/irc/test-irc-batch.cpp
    unit/plugins/irc/test-irc-buffer.cpp
    unit/plugins/irc/test-irc-channel.cpp
    unit/plugins/irc/test-irc-color.cpp
    unit/plugins/irc/test-irc-command.cpp
    unit/plugins/irc/test-irc-config.cpp
    unit/plugins/irc/test-irc-ctcp.cpp
    unit/plugins/irc/test-irc-info.cpp
    unit/plugins/irc/test-irc-ignore.cpp
    unit/plugins/irc/test-irc-join.cpp
    unit/plugins/irc/test-irc-list.cpp
    unit/plugins/irc/test-irc-message.cpp
    unit/plugins/irc/test-irc-mode.cpp
    unit/plugins/irc/test-irc-nick.cpp
    unit/plugins/irc/test-irc-protocol.cpp
    unit/plugins/irc/test-irc-sasl.cpp
    unit/plugins/irc/test-irc-server.cpp
    unit/plugins/irc/test-irc-tag.cpp
  )
endif()

if(ENABLE_LOGGER)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    unit/plugins/logger/test-logger.cpp
    unit/plugins/logger/test-logger-backlog.cpp
    unit/plugins/logger/test-logger-tail.cpp
  )
endif()

if (ENABLE_RELAY)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    unit/plugins/relay/test-relay-auth.cpp
    unit/plugins/relay/test-relay-bar-item.cpp
    unit/plugins/relay/test-relay-http.cpp
    unit/plugins/relay/test-relay-raw.cpp
    unit/plugins/relay/test-relay-remote.cpp
    unit/plugins/relay/test-relay-websocket.cpp
    unit/plugins/relay/irc/test-relay-irc.cpp
  )
  if (ENABLE_CJSON)
    list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
      unit/plugins/relay/api/test-relay-api.cpp
      unit/plugins/relay/api/test-relay-api-msg.cpp
      unit/plugins/relay/api/test-relay-api-protocol.cpp
      unit/plugins/relay/api/remote/test-relay-remote-network.cpp
    )
  endif()
endif()

if(ENABLE_TRIGGER)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    unit/plugins/trigger/test-trigger.cpp
    unit/plugins/trigger/test-trigger-config.cpp
  )
endif()

if(ENABLE_TYPING)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    unit/plugins/typing/test-typing.cpp
    unit/plugins/typing/test-typing-status.cpp
  )
endif()

if(ENABLE_XFER)
  list(APPEND LIB_WEECHAT_UNIT_TESTS_PLUGINS_SRC
    unit/plugins/xfer/test-xfer-file.cpp
    unit/plugins/xfer/test-xfer-network.cpp
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
