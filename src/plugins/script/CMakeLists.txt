# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(script MODULE
  script.c script.h
  script-action.c script-action.h
  script-buffer.c script-buffer.h
  script-command.c script-command.h
  script-completion.c script-completion.h
  script-config.c script-config.h
  script-info.c script-info.h
  script-mouse.c script-mouse.h
  script-repo.c script-repo.h
  script-theme.c script-theme.h
)
set_target_properties(script PROPERTIES PREFIX "")

set(LINK_LIBS)

list(APPEND LINK_LIBS ${ZLIB_LIBRARY})

target_link_libraries(script ${LINK_LIBS} coverage_config)

install(TARGETS script LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
