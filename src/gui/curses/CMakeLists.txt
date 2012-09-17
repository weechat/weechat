#
# Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2007-2008 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2009 Emmanuel Bouthenot <kolter@openics.org>
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

SET(WEECHAT_CURSES_SRC
gui-curses.h
gui-curses-bar-window.c
gui-curses-chat.c
gui-curses-color.c
gui-curses-key.c
gui-curses-main.c
gui-curses-mouse.c
gui-curses-term.c
gui-curses-window.c)

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

IF(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  IF(HAVE_BACKTRACE)
    LIST(APPEND EXTRA_LIBS "execinfo")
  ENDIF(HAVE_BACKTRACE)
ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")

IF(${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
  LIST(APPEND EXTRA_LIBS "pthread")
ENDIF(${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")

IF(ICONV_LIBRARY)
  LIST(APPEND EXTRA_LIBS ${ICONV_LIBRARY})
ENDIF(ICONV_LIBRARY)

IF(LIBINTL_LIBRARY)
  LIST(APPEND EXTRA_LIBS ${LIBINTL_LIBRARY})
ENDIF(LIBINTL_LIBRARY)

LIST(APPEND EXTRA_LIBS "m")

LIST(APPEND EXTRA_LIBS ${CURL_LIBRARIES})

ADD_EXECUTABLE(${EXECUTABLE} ${WEECHAT_CURSES_SRC})

INCLUDE_DIRECTORIES(.. ../../core ../../plugins)

# Because of a linker bug, we have to link 2 times with libweechat_core.a
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${STATIC_LIBS} ${EXTRA_LIBS} ${STATIC_LIBS})

INSTALL(TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin)
