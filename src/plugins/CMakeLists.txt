#
# SPDX-FileCopyrightText: 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
# SPDX-FileCopyrightText: 2009 Emmanuel Bouthenot <kolter@openics.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# This file is part of WeeChat, the extensible chat client.
#
# WeeChat is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# WeeChat is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

set(LIB_PLUGINS_SRC
  weechat-plugin.h
  plugin.c plugin.h
  plugin-api.c plugin-api.h
  plugin-api-info.c plugin-api-info.h
  plugin-config.h plugin-config.c
)

set(LIB_PLUGINS_SCRIPTS_SRC
  plugin-script.c plugin-script.h
  plugin-script-api.c plugin-script-api.h
  plugin-script-config.c plugin-script-config.h
)

include_directories("${CMAKE_BINARY_DIR}")
add_library(weechat_plugins STATIC ${LIB_PLUGINS_SRC})
target_link_libraries(weechat_plugins coverage_config)

add_definitions(${CMAKE_SHARED_LIBRARY_C_FLAGS})
add_library(weechat_plugins_scripts STATIC ${LIB_PLUGINS_SCRIPTS_SRC})

if(ENABLE_ALIAS)
  add_subdirectory(alias)
else()
  add_custom_target(alias COMMAND true)
endif()

if(ENABLE_BUFLIST)
  add_subdirectory(buflist)
else()
  add_custom_target(buflist COMMAND true)
endif()

if(ENABLE_CHARSET)
  add_subdirectory(charset)
else()
  add_custom_target(charset COMMAND true)
endif()

if(ENABLE_EXEC)
  add_subdirectory(exec)
else()
  add_custom_target(exec COMMAND true)
endif()

if(ENABLE_FIFO)
  add_subdirectory(fifo)
else()
  add_custom_target(fifo COMMAND true)
endif()

if(ENABLE_FSET)
  add_subdirectory(fset)
else()
  add_custom_target(fset COMMAND true)
endif()

if(ENABLE_IRC)
  add_subdirectory(irc)
else()
  add_custom_target(irc COMMAND true)
endif()

if(ENABLE_LOGGER)
  add_subdirectory(logger)
else()
  add_custom_target(logger COMMAND true)
endif()

if(ENABLE_RELAY)
  add_subdirectory(relay)
else()
  add_custom_target(relay COMMAND true)
endif()

if(ENABLE_SCRIPT)
  add_subdirectory(script)
else()
  add_custom_target(script COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_PERL)
  add_subdirectory(perl)
else()
  add_custom_target(perl COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_PYTHON)
  add_subdirectory(python)
else()
  add_custom_target(python COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_RUBY)
  add_subdirectory(ruby)
else()
  add_custom_target(ruby COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_LUA)
  add_subdirectory(lua)
else()
  add_custom_target(lua COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_TCL)
  add_subdirectory(tcl)
else()
  add_custom_target(tcl COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_GUILE)
  add_subdirectory(guile)
else()
  add_custom_target(guile COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_JAVASCRIPT)
  add_subdirectory(javascript)
else()
  add_custom_target(javascript COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_PHP)
  add_subdirectory(php)
else()
  add_custom_target(php COMMAND true)
endif()

if(ENABLE_SPELL)
  add_subdirectory(spell)
else()
  add_custom_target(spell COMMAND true)
endif()

if(ENABLE_TRIGGER)
  add_subdirectory(trigger)
else()
  add_custom_target(trigger COMMAND true)
endif()

if(ENABLE_TYPING)
  add_subdirectory(typing)
else()
  add_custom_target(typing COMMAND true)
endif()

if(ENABLE_XFER)
  add_subdirectory(xfer)
else()
  add_custom_target(xfer COMMAND true)
endif()

install(FILES weechat-plugin.h DESTINATION ${INCLUDEDIR})
