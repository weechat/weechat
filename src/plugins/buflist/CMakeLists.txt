# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(buflist MODULE
  buflist.c buflist.h
  buflist-bar-item.c buflist-bar-item.h
  buflist-command.c buflist-command.h
  buflist-completion.c buflist-completion.h
  buflist-config.c buflist-config.h
  buflist-info.c buflist-info.h
  buflist-mouse.c buflist-mouse.h
  buflist-theme.c buflist-theme.h
)
set_target_properties(buflist PROPERTIES PREFIX "")

target_link_libraries(buflist coverage_config)

install(TARGETS buflist LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
