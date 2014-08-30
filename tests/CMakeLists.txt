#
# Copyright (C) 2014 Sébastien Helleu <flashcode@flashtux.org>
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

enable_language(CXX)

find_package(CppUTest REQUIRED)

# fake ncurses lib (it does nothing)
set(LIB_WEECHAT_NCURSES_FAKE_SRC ncurses-fake.c)
add_library(weechat_ncurses_fake STATIC ${LIB_WEECHAT_NCURSES_FAKE_SRC})

remove_definitions(-DHAVE_CONFIG_H)
include_directories(${CPPUTEST_INCLUDE_DIRS} ${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR})

# unit tests
set(LIB_WEECHAT_UNIT_TESTS_SRC
  unit/core/test-arraylist.cpp
  unit/core/test-eval.cpp
  unit/core/test-hashtable.cpp
  unit/core/test-hdata.cpp
  unit/core/test-infolist.cpp
  unit/core/test-list.cpp
  unit/core/test-string.cpp
  unit/core/test-url.cpp
  unit/core/test-utf8.cpp
  unit/core/test-util.cpp
)
add_library(weechat_unit_tests STATIC ${LIB_WEECHAT_UNIT_TESTS_SRC})

# binary to run tests
set(WEECHAT_TESTS_SRC tests.cpp tests.h)
add_executable(tests ${WEECHAT_TESTS_SRC})
set(LIBS
  ${LIBS}
  ${PROJECT_BINARY_DIR}/src/core/libweechat_core.a
  ${PROJECT_BINARY_DIR}/src/plugins/libweechat_plugins.a
  ${PROJECT_BINARY_DIR}/src/gui/libweechat_gui_common.a
  ${PROJECT_BINARY_DIR}/src/gui/curses/libweechat_gui_curses.a
  ${CMAKE_CURRENT_BINARY_DIR}/libweechat_ncurses_fake.a
  ${CMAKE_CURRENT_BINARY_DIR}/libweechat_unit_tests.a
  # due to circular references, we must link two times with libweechat_core.a
  ${PROJECT_BINARY_DIR}/src/core/libweechat_core.a
  ${EXTRA_LIBS}
  ${CURL_LIBRARIES}
  ${CPPUTEST_LIBRARIES})
target_link_libraries(tests ${LIBS})
add_dependencies(tests
  weechat_core weechat_plugins weechat_gui_common weechat_gui_curses
  weechat_ncurses_fake
  weechat_unit_tests)

# test for cmake (ctest)
add_test(NAME unit
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND tests -v)
