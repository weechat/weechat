#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

set(LIB_GUI_COMMON_SRC
  gui-bar.c gui-bar.h
  gui-bar-item.c gui-bar-item.h
  gui-bar-item-custom.c gui-bar-item-custom.h
  gui-bar-window.c gui-bar-window.h
  gui-buffer.c gui-buffer.h
  gui-chat.c gui-chat.h
  gui-color.c gui-color.h
  gui-completion.c gui-completion.h
  gui-cursor.c gui-cursor.h
  gui-filter.c gui-filter.h
  gui-focus.c gui-focus.h
  gui-history.c gui-history.h
  gui-hotlist.c gui-hotlist.h
  gui-input.c gui-input.h
  gui-key.c gui-key.h
  gui-layout.c gui-layout.h
  gui-line.c gui-line.h
  gui-main.h
  gui-mouse.c gui-mouse.h
  gui-nick.c gui-nick.h
  gui-nicklist.c gui-nicklist.h
  gui-window.c gui-window.h
)

include_directories("${CMAKE_BINARY_DIR}")
add_library(weechat_gui_common STATIC ${LIB_GUI_COMMON_SRC})
target_link_libraries(weechat_gui_common coverage_config)
list(APPEND STATIC_LIBS weechat_gui_common)

subdirs(curses)
