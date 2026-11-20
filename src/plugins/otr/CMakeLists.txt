# SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
#
# SPDX-License-Identifier: GPL-3.0-or-later

add_library(otr MODULE
  otr.c
)
set_target_properties(otr PROPERTIES PREFIX "" LINK_FLAGS "-Wl,-z,nodelete")
find_package(PkgConfig REQUIRED)

find_library(LIBOTR_LIB NAMES otr REQUIRED)
find_library(GCRYPT_LIB NAMES gcrypt REQUIRED)
target_link_libraries(otr ${LIBOTR_LIB} ${GCRYPT_LIB} ${CMAKE_DL_LIBS})

install(TARGETS otr LIBRARY DESTINATION ${WEECHAT_LIBDIR}/plugins)
