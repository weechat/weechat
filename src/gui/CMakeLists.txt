# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

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
add_library(weechat_gui_common OBJECT ${LIB_GUI_COMMON_SRC})
target_link_libraries(weechat_gui_common coverage_config)

subdirs(curses)
