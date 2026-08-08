# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(alias MODULE
  alias.c alias.h
  alias-command.c alias-command.h
  alias-completion.c alias-completion.h
  alias-config.c alias-config.h
  alias-info.c alias-info.h
)
set_target_properties(alias PROPERTIES PREFIX "")

target_link_libraries(alias coverage_config)

install(TARGETS alias LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
