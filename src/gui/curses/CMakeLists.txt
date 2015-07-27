#
# Copyright (C) 2003-2015 Sébastien Helleu <flashcode@flashtux.org>
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

set(LIB_GUI_CURSES_SRC
gui-curses.h
gui-curses-bar-window.c
gui-curses-chat.c
gui-curses-color.c
gui-curses-key.c
gui-curses-main.c
gui-curses-mouse.c
gui-curses-term.c
gui-curses-window.c)

set(WEECHAT_CURSES_MAIN_SRC
main.c)

set(EXECUTABLE weechat)


find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_search_module(NCURSES ncursesw ncurses)
endif(PKG_CONFIG_FOUND)

if(NOT NCURSES_FOUND)
  find_package(Ncurses)
endif(NCURSES_FOUND)

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
  list(APPEND EXTRA_LIBS ${NCURSES_LIBRARY} ${NCURSES_LIBRARIES} )
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  if(HAVE_BACKTRACE)
    list(APPEND EXTRA_LIBS "execinfo")
  endif()
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "SunOS")
  list(APPEND EXTRA_LIBS "socket" "nsl")
endif()

list(APPEND EXTRA_LIBS "pthread")

if(ICONV_LIBRARY)
  list(APPEND EXTRA_LIBS ${ICONV_LIBRARY})
endif()

if(LIBINTL_LIBRARY)
  list(APPEND EXTRA_LIBS ${LIBINTL_LIBRARY})
endif()

list(APPEND EXTRA_LIBS "m")

list(APPEND EXTRA_LIBS ${CURL_LIBRARIES})

include_directories(.. ../../core ../../plugins ${NCURSES_INCLUDE_PATH})

add_library(weechat_gui_curses STATIC ${LIB_GUI_CURSES_SRC})

add_executable(${EXECUTABLE} ${WEECHAT_CURSES_MAIN_SRC})

add_dependencies(${EXECUTABLE} weechat_gui_curses)

# Due to circular references, we must link two times with libweechat_core.a
target_link_libraries(${EXECUTABLE} ${STATIC_LIBS} weechat_gui_curses ${EXTRA_LIBS} ${STATIC_LIBS})

# Create a symbolic link weechat-curses -> weechat
# This link is created for compatibility with old versions on /upgrade.
# It may be removed in future.
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE}-curses${CMAKE_EXECUTABLE_SUFFIX}
  COMMAND ${CMAKE_COMMAND} -E remove -f "weechat-curses${CMAKE_EXECUTABLE_SUFFIX}"
  COMMAND ${CMAKE_COMMAND} -E create_symlink "weechat${CMAKE_EXECUTABLE_SUFFIX}" "weechat-curses${CMAKE_EXECUTABLE_SUFFIX}"
  DEPENDS ${EXECUTABLE}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMENT "Creating symbolic link ${EXECUTABLE}-curses${CMAKE_EXECUTABLE_SUFFIX}"
  )
add_custom_target(${EXECUTABLE}-curses${CMAKE_EXECUTABLE_SUFFIX} ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE}-curses${CMAKE_EXECUTABLE_SUFFIX}
)
add_dependencies(${EXECUTABLE}-curses${CMAKE_EXECUTABLE_SUFFIX} ${EXECUTABLE})
list(APPEND CMAKE_INSTALL_MANIFEST_FILES "${CMAKE_INSTALL_PREFIX}/bin/weechat-curses${CMAKE_EXECUTABLE_SUFFIX}")

# Install executable and symbolic link
install(TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${EXECUTABLE}-curses${CMAKE_EXECUTABLE_SUFFIX} DESTINATION bin)
