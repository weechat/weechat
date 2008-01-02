# Copyright (c) 2003-2008 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

IF(NCURSES_FOUND)
  SET(NCURSES_FIND_QUIETLY TRUE)
ENDIF(NCURSES_FOUND)

FIND_PATH(NCURSES_INCLUDE_PATH
  NAMES ncurses.h curses.h
  PATHS /usr/include /usr/local/include /usr/pkg/include
)

FIND_LIBRARY(NCURSES_LIBRARY
  NAMES ncursesw ncurses
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

IF (NCURSES_INCLUDE_PATH AND NCURSES_LIBRARY)
  SET(NCURSES_FOUND TRUE)
ENDIF(NCURSES_INCLUDE_PATH AND NCURSES_LIBRARY)

MARK_AS_ADVANCED(
  NCURSES_INCLUDE_PATH
  NCURSES_LIBRARY
)
