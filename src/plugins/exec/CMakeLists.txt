# SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(exec MODULE
  exec.c exec.h
  exec-buffer.c exec-buffer.h
  exec-command.c exec-command.h
  exec-completion.c exec-completion.h
  exec-config.c exec-config.h
  exec-theme.c exec-theme.h
)
set_target_properties(exec PROPERTIES PREFIX "")

target_link_libraries(exec coverage_config)

install(TARGETS exec LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
