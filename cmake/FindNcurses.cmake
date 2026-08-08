# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

if(NCURSES_FOUND)
  set(NCURSES_FIND_QUIETLY TRUE)
endif()

find_path(NCURSES_INCLUDE_PATH
  NAMES ncurses.h curses.h
  PATH_SUFFIXES ncursesw ncurses
  PATHS /usr/include /usr/local/include /usr/pkg/include
)

find_library(NCURSESW_LIBRARY
  NAMES ncursesw
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

if(NCURSESW_LIBRARY)
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_search_module(NCURSES ncursesw)
    set(NCURSESW_LIBRARY ${NCURSES_LIBRARIES} ${NCURSES_CFLAGS_OTHER})
  endif()
  set(NCURSES_LIBRARY ${NCURSESW_LIBRARY})
else()
  find_library(NCURSES_LIBRARY
    NAMES ncurses
    PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
  )
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_search_module(NCURSES ncurses)
    set(NCURSES_LIBRARY ${NCURSES_LIBRARIES} ${NCURSES_CFLAGS_OTHER})
  endif()
  if(NCURSES_LIBRARY)
    message("*** WARNING:\n"
      "*** ncursesw library not found! Falling back to \"ncurses\"\n"
      "*** Be careful, UTF-8 display may not work properly if your locale is UTF-8.")
  endif()
endif()

if(NCURSES_INCLUDE_PATH AND NCURSES_LIBRARY)
  set(NCURSES_FOUND TRUE)
endif()

mark_as_advanced(
  NCURSES_INCLUDE_PATH
  NCURSES_LIBRARY
)
