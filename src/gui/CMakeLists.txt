# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
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

SUBDIRS( curses )

SET(LIB_GUI_COMMON_SRC gui-buffer.c gui-common.c gui-action.c gui-keyboard.c
gui-log.c gui-window.c gui-panel.c gui.h gui-buffer.h gui-color.h gui-keyboard.h
gui-panel.h gui-window.h)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_gui_common STATIC ${LIB_GUI_COMMON_SRC})
