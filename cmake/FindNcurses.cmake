# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# - Find Ncurses
# This module finds ncursesw (or ncurses as fallback) and determines where
# the include files and libraries are.
#
# A custom ncurses can be used with PKG_CONFIG_PATH, or with these options
# (which have precedence over pkg-config):
#
#  -DNCURSES_INCLUDE_PATH=/path/to/include/ncursesw
#  -DNCURSESW_LIBRARY=/path/to/lib/libncursesw.so
#
# This code sets the following variables:
#
#  NCURSES_INCLUDE_PATH = path to where <ncurses.h> can be found
#  NCURSES_LIBRARY = libraries to link with
#  NCURSES_CFLAGS = compiler flags (only if found with pkg-config)
#
#  NCURSES_FOUND = is ncurses usable on system?

if(NCURSES_FOUND)
  # Already in cache, be silent.
  set(NCURSES_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig QUIET)

if(NCURSESW_LIBRARY)
  set(NCURSES_LIBRARY ${NCURSESW_LIBRARY})
elseif(NOT NCURSES_LIBRARY)
  # Libraries found by pkg-config are used with their full path, so that
  # another ncurses in the default linker paths is not used instead.
  if(PKG_CONFIG_FOUND)
    pkg_search_module(NCURSES ncursesw)
  endif()
  if(NCURSES_FOUND)
    set(NCURSES_LIBRARY ${NCURSES_LINK_LIBRARIES})
  else()
    find_library(NCURSESW_LIBRARY
      NAMES ncursesw
      PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
    )
    if(NCURSESW_LIBRARY)
      set(NCURSES_LIBRARY ${NCURSESW_LIBRARY})
    else()
      if(PKG_CONFIG_FOUND)
        pkg_search_module(NCURSES ncurses)
      endif()
      if(NCURSES_FOUND)
        set(NCURSES_LIBRARY ${NCURSES_LINK_LIBRARIES})
      else()
        find_library(NCURSES_LIBRARY
          NAMES ncurses
          PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
        )
      endif()
      if(NCURSES_LIBRARY)
        message("*** WARNING:\n"
          "*** ncursesw library not found! Falling back to \"ncurses\"\n"
          "*** Be careful, UTF-8 display may not work properly if your locale is UTF-8.")
      endif()
    endif()
  endif()
endif()

find_path(NCURSES_INCLUDE_PATH
  NAMES ncurses.h curses.h
  PATH_SUFFIXES ncursesw ncurses
  HINTS ${NCURSES_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /usr/pkg/include
)

if(NCURSES_INCLUDE_PATH AND NCURSES_LIBRARY)
  set(NCURSES_FOUND TRUE)
endif()

mark_as_advanced(
  NCURSES_INCLUDE_PATH
  NCURSES_LIBRARY
  NCURSESW_LIBRARY
)
