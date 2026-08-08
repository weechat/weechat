# SPDX-FileCopyrightText: 2011-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(Guile REQUIRED)

add_library(guile MODULE
  weechat-guile.c weechat-guile.h
  weechat-guile-api.c weechat-guile-api.h
)
set_target_properties(guile PROPERTIES PREFIX "")

include_directories(${GUILE_INCLUDE_DIRS})
target_link_libraries(guile ${GUILE_LDFLAGS} weechat_plugins_scripts coverage_config)

install(TARGETS guile LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
