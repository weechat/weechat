#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2007 Julien Louis <ptitlouis@sysif.net>
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

SET(WEECHAT_GTK_SRC
gui-gtk.h
gui-gtk-bar-window.c
gui-gtk-chat.c
gui-gtk-color.c
gui-gtk-key.c
gui-gtk-main.c
gui-gtk-mouse.c
gui-gtk-term.c
gui-gtk-window.c)

SET(EXECUTABLE weechat-gtk)

FIND_PACKAGE(PkgConfig)
IF(PKG_CONFIG_FOUND)
  pkg_check_modules(GTK2 gtk+-x11-2.0)
  IF(GTK2_FOUND)
    INCLUDE_DIRECTORIES( ${GTK2_INCLUDE_DIRS} )
    LIST(APPEND EXTRA_LIBS ${GTK2_LIBRARIES})
    #  ELSE(GTK2_FOUND)
    #    MESSAGE(FATAL_ERROR "Missing dependency, aborting configuration phase")
  ENDIF(GTK2_FOUND)
ENDIF(PKG_CONFIG_FOUND)

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

LIST(APPEND EXTRA_LIBS ${CURL_LIBRARIES})

ADD_EXECUTABLE(${EXECUTABLE} ${WEECHAT_GTK_SRC})

INCLUDE_DIRECTORIES(.. ../../core ../../plugins)

# Because of a linker bug, we have to link 2 times with libweechat_core.a
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${STATIC_LIBS} ${EXTRA_LIBS} ${STATIC_LIBS})

INSTALL(TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin)
