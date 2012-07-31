#
# Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

IF(NCURSES_FOUND)
  SET(NCURSES_FIND_QUIETLY TRUE)
ENDIF(NCURSES_FOUND)

FIND_PATH(NCURSES_INCLUDE_PATH
  NAMES ncurses.h curses.h
  PATHS /usr/include/ncursesw /usr/include/ncurses /usr/include
  /usr/local/include/ncursesw /usr/local/include/ncurses /usr/local/include
  /usr/pkg/include/ncursesw /usr/pkg/include/ncurses /usr/pkg/include
)

FIND_LIBRARY(NCURSESW_LIBRARY
  NAMES ncursesw
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

IF (NCURSESW_LIBRARY)
  SET(NCURSES_LIBRARY ${NCURSESW_LIBRARY})
ELSE(NCURSESW_LIBRARY)
  FIND_LIBRARY(NCURSES_LIBRARY
    NAMES ncurses
    PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
  )
  IF (NCURSES_LIBRARY)
    MESSAGE("*** WARNING:\n"
      "*** ncursesw library not found! Falling back to \"ncurses\"\n"
      "*** Be careful, UTF-8 display may not work properly if your locale is UTF-8.")
  ENDIF(NCURSES_LIBRARY)
ENDIF(NCURSESW_LIBRARY)

IF (NCURSES_INCLUDE_PATH AND NCURSES_LIBRARY)
  SET(NCURSES_FOUND TRUE)
ENDIF(NCURSES_INCLUDE_PATH AND NCURSES_LIBRARY)

MARK_AS_ADVANCED(
  NCURSES_INCLUDE_PATH
  NCURSES_LIBRARY
)
