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

SET(WEECHAT_CURSES_SRC gui-curses-chat.c gui-curses-color.c gui-curses-infobar.c
gui-curses-input.c gui-curses-keyboard.c gui-curses-main.c gui-curses-nicklist.c
gui-curses-status.c gui-curses-window.c gui-curses-panel.c gui-curses.h)

SET(EXECUTABLE weechat-curses)

FIND_PACKAGE(Ncurses)
IF(NCURSES_FOUND)
  CHECK_INCLUDE_FILES(ncursesw/ncurses.h NCURSESW_HEADERS)
  IF(NCURSESW_HEADERS)
    ADD_DEFINITIONS(-DHAVE_NCURSESW_CURSES_H)
  ELSE(NCURSESW_HEADERS)
    CHECK_INCLUDE_FILES(ncurses.h NCURSES_HEADERS)
    IF(NCURSES_HEADERS)
      ADD_DEFINITIONS(-DHAVE_NCURSES_H)
    ENDIF(NCURSES_HEADERS)
  ENDIF(NCURSESW_HEADERS)
  
  LIST(APPEND EXTRA_LIBS ${NCURSES_LIBRARY})
ENDIF(NCURSES_FOUND)

ADD_EXECUTABLE(${EXECUTABLE} ${WEECHAT_CURSES_SRC})
INCLUDE_DIRECTORIES(.. ../../common ../../irc ../../plugins)
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${STATIC_LIBS} ${EXTRA_LIBS})

INSTALL(TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin)
