# SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
# SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

pkg_check_modules(LIBOTR REQUIRED libotr>=4.0.0)

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
set_target_properties(otr PROPERTIES PREFIX "")

# Keep the plugin (and libotr) in memory after /plugin unload: libotr registers
# handlers in libgcrypt (the plugin also does it at runtime with dlopen, this
# linker option is not available on all platforms, for example macOS).
include(CheckLinkerFlag)
check_linker_flag(C "-Wl,-z,nodelete" HAVE_LINKER_FLAG_Z_NODELETE)
if(HAVE_LINKER_FLAG_Z_NODELETE)
  target_link_options(otr PRIVATE "-Wl,-z,nodelete")
endif()

include_directories(${LIBOTR_INCLUDE_DIRS} ${LIBGCRYPT_INCLUDE_DIRS})
target_link_libraries(otr ${LIBOTR_LDFLAGS} ${LIBGCRYPT_LDFLAGS} ${CMAKE_DL_LIBS} coverage_config)

install(TARGETS otr LIBRARY DESTINATION ${WEECHAT_LIBDIR}/plugins)
