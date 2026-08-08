# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(fset MODULE
  fset.c fset.h
  fset-bar-item.c fset-bar-item.h
  fset-buffer.c fset-buffer.h
  fset-command.c fset-command.h
  fset-completion.c fset-completion.h
  fset-config.c fset-config.h
  fset-info.c fset-info.h
  fset-mouse.c fset-mouse.h
  fset-option.c fset-option.h
  fset-theme.c fset-theme.h
)
set_target_properties(fset PROPERTIES PREFIX "")

target_link_libraries(fset coverage_config)

install(TARGETS fset LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
