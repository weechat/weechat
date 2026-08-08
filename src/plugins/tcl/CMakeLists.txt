# SPDX-FileCopyrightText: 2008-2010 Dmitry Kobylin <fnfal@academ.tsc.ru>
# SPDX-FileCopyrightText: 2008 Julien Louis <ptitlouis@sysif.net>
# SPDX-FileCopyrightText: 2008-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

find_package(TCL)
if(TCL_FOUND)
  add_library(tcl MODULE
    weechat-tcl.c weechat-tcl.h
    weechat-tcl-api.c weechat-tcl-api.h
  )
  set_target_properties(tcl PROPERTIES PREFIX "")

  include_directories(${TCL_INCLUDE_PATH})
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${TCL_LFLAGS}")
  target_link_libraries(tcl ${TCL_LIBRARY} weechat_plugins_scripts coverage_config)

  install(TARGETS tcl LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
else()
  message(SEND_ERROR "Tcl not found")
endif()
