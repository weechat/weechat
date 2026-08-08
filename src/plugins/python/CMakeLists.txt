# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

if(CMAKE_VERSION VERSION_LESS "3.18.0")
  find_package(Python 3.0 REQUIRED COMPONENTS Interpreter Development)
else()
  find_package(Python 3.0 REQUIRED COMPONENTS Interpreter Development.Embed)
endif()

add_library(python MODULE
  weechat-python.c weechat-python.h
  weechat-python-api.c weechat-python-api.h
)
set_target_properties(python PROPERTIES PREFIX "")

if(Python_FOUND)
  target_link_libraries(python Python::Python weechat_plugins_scripts coverage_config)
endif()

install(TARGETS python LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
