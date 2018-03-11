#
# Copyright (C) 2003-2018 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

set(LIB_PLUGINS_SRC
  weechat-plugin.h
  plugin.c plugin.h
  plugin-api.c plugin-api.h
  plugin-config.h plugin-config.c
)

set(LIB_PLUGINS_SCRIPTS_SRC
  plugin-script.c plugin-script.h
  plugin-script-api.c plugin-script-api.h
  plugin-script-config.c plugin-script-config.h
)

include_directories(${CMAKE_BINARY_DIR})
add_library(weechat_plugins STATIC ${LIB_PLUGINS_SRC})

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
endif()

if(ENABLE_ASPELL)
  if(ENABLE_ENCHANT)
    # Check for enchant libraries
    find_package(ENCHANT)
    if(ENCHANT_FOUND)
      add_subdirectory(aspell)
    else()
      # Check for aspell libraries
      find_package(Aspell)
      if(ASPELL_FOUND)
        add_subdirectory(aspell)
      endif()
    endif()
  else()
    # Check for aspell libraries
    find_package(Aspell)
    if(ASPELL_FOUND)
      add_subdirectory(aspell)
    endif()
  endif()
endif()

if(ENABLE_BUFLIST)
  add_subdirectory(buflist)
endif()

if(ENABLE_CHARSET)
  # Check for iconv support.
  if(ICONV_FOUND)
    add_subdirectory(charset)
  endif()
endif()

if(ENABLE_EXEC)
  add_subdirectory(exec)
endif()

if(ENABLE_FIFO)
  add_subdirectory(fifo)
endif()

if(ENABLE_FSET)
  add_subdirectory(fset)
endif()

if(ENABLE_IRC)
  add_subdirectory(irc)
endif()

if(ENABLE_LOGGER)
  add_subdirectory(logger)
endif()

if(ENABLE_RELAY)
  add_subdirectory(relay)
endif()

if(ENABLE_SCRIPT)
  add_subdirectory(script)
endif()

if(ENABLE_SCRIPTS AND ENABLE_PERL)
  find_package(Perl)
  if(PERL_FOUND)
    add_subdirectory(perl)
  endif()
endif()

if(ENABLE_SCRIPTS AND ENABLE_PYTHON)
  find_package(Python)
  if(PYTHON_FOUND)
    add_subdirectory(python)
  endif()
endif()

if(ENABLE_SCRIPTS AND ENABLE_RUBY)
  find_package(Ruby)
  if(RUBY_FOUND)
    add_subdirectory(ruby)
  endif()
endif()

if(ENABLE_SCRIPTS AND ENABLE_LUA)
  find_package(Lua)
  if(LUA_FOUND)
    add_subdirectory(lua)
  endif()
endif()

if(ENABLE_SCRIPTS AND ENABLE_TCL)
  find_package(TCL)
  if(TCL_FOUND)
    add_subdirectory(tcl)
  endif()
endif()

if(ENABLE_SCRIPTS AND ENABLE_GUILE)
  find_package(Guile)
  if(GUILE_FOUND)
    add_subdirectory(guile)
  endif()
endif()

if(ENABLE_SCRIPTS AND ENABLE_JAVASCRIPT)
  find_package(V8)
  if(V8_FOUND)
    add_subdirectory(javascript)
  endif()
endif()

if(ENABLE_SCRIPTS AND ENABLE_PHP)
  find_package(PHP)
  if(PHP_FOUND)
    add_subdirectory(php)
  endif()
endif()

if(ENABLE_TRIGGER)
  add_subdirectory(trigger)
endif()

if(ENABLE_XFER)
  add_subdirectory(xfer)
endif()

install(FILES weechat-plugin.h DESTINATION ${INCLUDEDIR})
