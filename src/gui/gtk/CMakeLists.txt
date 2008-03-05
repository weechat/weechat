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

SET(WEECHAT_GTK_SRC gui-gtk-bar.c gui-gtk-chat.c gui-gtk-color.c
gui-gtk-infobar.c gui-gtk-input.c gui-gtk-keyboard.c gui-gtk-main.c
gui-gtk-nicklist.c gui-gtk-status.c gui-gtk-window.c gui-gtk.h)

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

ADD_EXECUTABLE(${EXECUTABLE} ${WEECHAT_GTK_SRC}
../../core/weechat.o
../../core/wee-backtrace.o
../../core/wee-command.o
../../core/wee-config.o
../../core/wee-config-file.o
../../core/wee-debug.o
../../core/wee-hook.o
../../core/wee-input.o
../../core/wee-list.o
../../core/wee-log.o
../../core/wee-upgrade.o
../../core/wee-string.o
../../core/wee-utf8.o
../../core/wee-util.o)
INCLUDE_DIRECTORIES(.. ../../core ../../plugins)
TARGET_LINK_LIBRARIES(${EXECUTABLE} ${STATIC_LIBS} ${EXTRA_LIBS})

INSTALL(TARGETS ${EXECUTABLE} RUNTIME DESTINATION bin)
