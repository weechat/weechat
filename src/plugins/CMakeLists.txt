#
# Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2009 Emmanuel Bouthenot <kolter@openics.org>
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
if(NOT CYGWIN)
  add_definitions(-fPIC)
endif()
add_library(weechat_plugins_scripts STATIC ${LIB_PLUGINS_SCRIPTS_SRC})

include(CheckIncludeFiles)
include(CheckFunctionExists)
include(CheckLibraryExists)

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
  if(ICONV_FOUND)
    add_subdirectory(charset)
  else()
    message(SEND_ERROR "Iconv not found")
  endif()
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
  find_package(Perl)
  if(PERL_FOUND)
    add_subdirectory(perl)
  else()
    message(SEND_ERROR "Perl not found")
  endif()
else()
  add_custom_target(perl COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_PYTHON)
  find_package(Python)
  if(PYTHON_FOUND)
    add_subdirectory(python)
  else()
    message(SEND_ERROR "Python not found")
  endif()
else()
  add_custom_target(python COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_RUBY)
  find_package(Ruby)
  if(RUBY_FOUND)
    add_subdirectory(ruby)
  else()
    message(SEND_ERROR "Ruby not found")
  endif()
else()
  add_custom_target(ruby COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_LUA)
  find_package(Lua)
  if(LUA_FOUND)
    add_subdirectory(lua)
  else()
    message(SEND_ERROR "Lua not found")
  endif()
else()
  add_custom_target(lua COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_TCL)
  find_package(TCL)
  if(TCL_FOUND)
    add_subdirectory(tcl)
  else()
    message(SEND_ERROR "Tcl not found")
  endif()
else()
  add_custom_target(tcl COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_GUILE)
  find_package(Guile)
  if(GUILE_FOUND)
    add_subdirectory(guile)
  else()
    message(SEND_ERROR "Guile not found")
  endif()
else()
  add_custom_target(guile COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_JAVASCRIPT)
  find_package(V8)
  if(V8_FOUND)
    add_subdirectory(javascript)
  else()
    message(SEND_ERROR "V8 (javascript) not found")
  endif()
else()
  add_custom_target(javascript COMMAND true)
endif()

if(ENABLE_SCRIPTS AND ENABLE_PHP)
  find_package(PHP)
  if(PHP_FOUND)
    add_subdirectory(php)
  else()
    message(SEND_ERROR "Php not found")
  endif()
else()
  add_custom_target(php COMMAND true)
endif()

if(ENABLE_SPELL)
  if(ENABLE_ENCHANT)
    find_package(ENCHANT)
    if(ENCHANT_FOUND)
      add_subdirectory(spell)
    else()
      message(SEND_ERROR "Enchant not found")
    endif()
  else()
    find_package(Aspell)
    if(ASPELL_FOUND)
      add_subdirectory(spell)
    else()
      message(SEND_ERROR "Aspell not found")
    endif()
  endif()
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
