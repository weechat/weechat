# SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(trigger MODULE
  trigger.c trigger.h
  trigger-buffer.c trigger-buffer.h
  trigger-callback.c trigger-callback.h
  trigger-command.c trigger-command.h
  trigger-completion.c trigger-completion.h
  trigger-config.c trigger-config.h
  trigger-theme.c trigger-theme.h
)
set_target_properties(trigger PROPERTIES PREFIX "")

target_link_libraries(trigger coverage_config)

install(TARGETS trigger LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
