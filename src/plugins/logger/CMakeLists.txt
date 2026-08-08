# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(logger MODULE
  logger.c logger.h
  logger-backlog.c logger-backlog.h
  logger-buffer.c logger-buffer.h
  logger-command.c logger-command.h
  logger-config.c logger-config.h
  logger-info.c logger-info.h
  logger-tail.c logger-tail.h
  logger-theme.c logger-theme.h
)
set_target_properties(logger PROPERTIES PREFIX "")

target_link_libraries(logger coverage_config)

install(TARGETS logger LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
