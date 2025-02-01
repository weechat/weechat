#
# Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

add_definitions(-DWEECHAT_HEADLESS)

# fake ncurses lib (for headless mode and tests)
set(LIB_WEECHAT_NCURSES_FAKE_SRC
  ncurses-fake.h
  ncurses-fake.c
)
add_library(weechat_ncurses_fake STATIC ${LIB_WEECHAT_NCURSES_FAKE_SRC})

set(LIB_GUI_CURSES_SRC
  ../gui-curses.h
  ../gui-curses-bar-window.c ../gui-curses-bar-window.h
  ../gui-curses-chat.c ../gui-curses-chat.h
  ../gui-curses-color.c ../gui-curses-color.h
  ../gui-curses-key.c ../gui-curses-key.h
  ../gui-curses-main.c ../gui-curses-main.h
  ../gui-curses-mouse.c ../gui-curses-mouse.h
  ../gui-curses-term.c
  ../gui-curses-window.c ../gui-curses-window.h
)

include_directories(. .. ../../core ../../plugins)

add_library(weechat_gui_headless STATIC ${LIB_GUI_CURSES_SRC})

set(WEECHAT_CURSES_MAIN_HEADLESS_SRC main.c)

set(EXECUTABLE weechat-headless)

add_executable(${EXECUTABLE}
  ${WEECHAT_CURSES_MAIN_HEADLESS_SRC}
)

add_dependencies(${EXECUTABLE}
  weechat_gui_headless
  weechat_ncurses_fake
)

target_link_libraries(${EXECUTABLE}
  weechat_core
  weechat_plugins
  weechat_gui_common
  weechat_gui_headless
  weechat_ncurses_fake
  ${EXTRA_LIBS}
  coverage_config
)

# Install executable
install(TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin)
