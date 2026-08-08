# SPDX-FileCopyrightText: 2021-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(typing MODULE
  typing.c typing.h
  typing-bar-item.c typing-bar-item.h
  typing-config.c typing-config.h
  typing-status.c typing-status.h
)
set_target_properties(typing PROPERTIES PREFIX "")

target_link_libraries(typing coverage_config)

install(TARGETS typing LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
