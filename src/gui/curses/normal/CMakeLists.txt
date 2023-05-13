#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

find_package(Ncurses)
if(NCURSES_FOUND)
  check_include_files(ncursesw/ncurses.h NCURSESW_HEADERS)
  if(NCURSESW_HEADERS)
    add_definitions(-DHAVE_NCURSESW_CURSES_H)
  else()
    check_include_files(ncurses.h NCURSES_HEADERS)
    if(NCURSES_HEADERS)
      add_definitions(-DHAVE_NCURSES_H)
    endif()
  endif()
else()
  message(SEND_ERROR "Ncurses not found")
endif()

include_directories(.. ../../core ../../plugins ${NCURSES_INCLUDE_PATH})

add_library(weechat_gui_curses_normal STATIC ${LIB_GUI_CURSES_SRC})

set(WEECHAT_CURSES_MAIN_SRC main.c)

set(EXECUTABLE weechat)

add_executable(${EXECUTABLE} ${WEECHAT_CURSES_MAIN_SRC})

add_dependencies(${EXECUTABLE} weechat_gui_curses_normal)

list(APPEND EXTRA_LIBS ${NCURSES_LIBRARY})

# Due to circular references, we must link two times with libweechat_core.a and libweechat_gui_common.a
target_link_libraries(${EXECUTABLE} ${STATIC_LIBS} weechat_gui_curses_normal ${EXTRA_LIBS} ${STATIC_LIBS} coverage_config)

# Create a symbolic link weechat-curses -> weechat
# This link is created for compatibility with old versions on /upgrade.
# It may be removed in future.
add_custom_command(
  TARGET ${EXECUTABLE}
  COMMAND ${CMAKE_COMMAND} -E remove -f "weechat-curses${CMAKE_EXECUTABLE_SUFFIX}"
  COMMAND ${CMAKE_COMMAND} -E create_symlink "weechat${CMAKE_EXECUTABLE_SUFFIX}" "weechat-curses${CMAKE_EXECUTABLE_SUFFIX}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  COMMENT "Creating symbolic link ${EXECUTABLE}-curses${CMAKE_EXECUTABLE_SUFFIX}"
)
list(APPEND CMAKE_INSTALL_MANIFEST_FILES "${CMAKE_INSTALL_PREFIX}/bin/weechat-curses${CMAKE_EXECUTABLE_SUFFIX}")

# Install executable and symbolic link
install(TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE}-curses${CMAKE_EXECUTABLE_SUFFIX}" DESTINATION bin)
