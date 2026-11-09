# SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
# SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(otr MODULE
  otr.c otr.h
  otr-bar-item.c otr-bar-item.h
  otr-buffer.c otr-buffer.h
  otr-callback.c otr-callback.h
  otr-command.c otr-command.h
  otr-completion.c otr-completion.h
  otr-config.c otr-config.h
  otr-context.c otr-context.h
  otr-echo.c otr-echo.h
  otr-irc.c otr-irc.h
  otr-log.c otr-log.h
  otr-message.c otr-message.h
  otr-modifier.c otr-modifier.h
  otr-policy.c otr-policy.h
  otr-util.c otr-util.h
)
set_target_properties(otr PROPERTIES PREFIX "" LINK_FLAGS "-Wl,-z,nodelete")
find_package(PkgConfig REQUIRED)

find_library(LIBOTR_LIB NAMES otr REQUIRED)
find_library(GCRYPT_LIB NAMES gcrypt REQUIRED)
target_link_libraries(otr ${LIBOTR_LIB} ${GCRYPT_LIB} ${CMAKE_DL_LIBS})

install(TARGETS otr LIBRARY DESTINATION ${WEECHAT_LIBDIR}/plugins)
