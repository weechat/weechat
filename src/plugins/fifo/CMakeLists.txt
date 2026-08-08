# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(fifo MODULE
  fifo.c fifo.h
  fifo-command.c fifo-command.h
  fifo-config.c fifo-config.h
  fifo-info.c fifo-info.h
)
set_target_properties(fifo PROPERTIES PREFIX "")

target_link_libraries(fifo coverage_config)

install(TARGETS fifo LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
