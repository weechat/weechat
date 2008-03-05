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

IF(NOT DISABLE_NCURSES)
  SUBDIRS( curses )
ENDIF(NOT DISABLE_NCURSES)

SET(LIB_GUI_COMMON_SRC gui-action.c gui-action.h gui-bar.c gui-bar.h
gui-bar-item.c gui-bar-item.h gui-buffer.c gui-buffer.h gui-chat.c gui-chat.h
gui-color.c gui-color.h gui-completion.c gui-completion.h gui-history.c
gui-history.h gui-hotlist.c gui-hotlist.h gui-infobar.c gui-infobar.h
gui-input.c gui-input.h gui-keyboard.c gui-keyboard.h gui-main.h
gui-nicklist.c gui-nicklist.h gui-status.h gui-window.c gui-window.h)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_gui_common STATIC ${LIB_GUI_COMMON_SRC})

IF(ENABLE_GTK)
  ADD_SUBDIRECTORY( gtk )
ENDIF(ENABLE_GTK)
