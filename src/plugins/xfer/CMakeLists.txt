# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(xfer MODULE
  xfer.c xfer.h
  xfer-buffer.c xfer-buffer.h
  xfer-chat.c xfer-chat.h
  xfer-command.c xfer-command.h
  xfer-completion.c xfer-completion.h
  xfer-config.c xfer-config.h
  xfer-dcc.c xfer-dcc.h
  xfer-file.c xfer-file.h
  xfer-info.c xfer-info.h
  xfer-network.c xfer-network.h
  xfer-theme.c xfer-theme.h
  xfer-upgrade.c xfer-upgrade.h
)
set_target_properties(xfer PROPERTIES PREFIX "")

set(LINK_LIBS)

list(APPEND LINK_LIBS ${LIBGCRYPT_LDFLAGS})

if(LIBRESOLV_HAS_RES_INIT)
  list(APPEND LINK_LIBS ${RESOLV_LIBRARY})
endif()

target_link_libraries(xfer ${LINK_LIBS} coverage_config)

install(TARGETS xfer LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
