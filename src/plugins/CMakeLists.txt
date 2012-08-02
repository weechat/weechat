#
# Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

SET(LIB_PLUGINS_SRC
weechat-plugin.h
plugin.c plugin.h
plugin-api.c plugin-api.h
plugin-config.h plugin-config.c)

SET(LIB_PLUGINS_SCRIPTS_SRC
plugin-script.c plugin-script.h
plugin-script-api.c plugin-script-api.h
plugin-script-callback.c plugin-script-callback.h)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_plugins STATIC ${LIB_PLUGINS_SRC})

ADD_DEFINITIONS(${CMAKE_SHARED_LIBRARY_C_FLAGS})
ADD_LIBRARY(weechat_plugins_scripts STATIC ${LIB_PLUGINS_SCRIPTS_SRC})

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)

IF(ENABLE_ALIAS)
  ADD_SUBDIRECTORY( alias )
ENDIF(ENABLE_ALIAS)

IF(ENABLE_ASPELL)
  # Check for aspell libraries
  FIND_PACKAGE(Aspell)
  IF(ASPELL_FOUND)
    ADD_SUBDIRECTORY( aspell )
  ENDIF(ASPELL_FOUND)
ENDIF(ENABLE_ASPELL)

IF(ENABLE_CHARSET)
  # Check for iconv support.
  IF(ICONV_FOUND)
    ADD_SUBDIRECTORY( charset )
  ENDIF(ICONV_FOUND)
ENDIF(ENABLE_CHARSET)

IF(ENABLE_DEMO)
  ADD_SUBDIRECTORY( demo )
ENDIF(ENABLE_DEMO)

IF(ENABLE_FIFO)
  ADD_SUBDIRECTORY( fifo )
ENDIF(ENABLE_FIFO)

IF(ENABLE_IRC)
  ADD_SUBDIRECTORY( irc )
ENDIF(ENABLE_IRC)

IF(ENABLE_LOGGER)
  ADD_SUBDIRECTORY( logger )
ENDIF(ENABLE_LOGGER)

IF(ENABLE_RELAY)
  ADD_SUBDIRECTORY( relay )
ENDIF(ENABLE_RELAY)

IF(ENABLE_RMODIFIER)
  ADD_SUBDIRECTORY( rmodifier )
ENDIF(ENABLE_RMODIFIER)

IF(ENABLE_XFER)
  ADD_SUBDIRECTORY( xfer )
ENDIF(ENABLE_XFER)

IF(ENABLE_SCRIPTS AND ENABLE_PERL)
  FIND_PACKAGE(Perl)
  IF(PERL_FOUND)
    ADD_SUBDIRECTORY( perl )
  ENDIF(PERL_FOUND)
ENDIF(ENABLE_SCRIPTS AND ENABLE_PERL)

IF(ENABLE_SCRIPTS AND ENABLE_PYTHON)
  FIND_PACKAGE(Python)
  IF(PYTHON_FOUND)
    ADD_SUBDIRECTORY( python )
  ENDIF(PYTHON_FOUND)
ENDIF(ENABLE_SCRIPTS AND ENABLE_PYTHON)

IF(ENABLE_SCRIPTS AND ENABLE_RUBY)
  FIND_PACKAGE(Ruby)
  IF(RUBY_FOUND)
    ADD_SUBDIRECTORY( ruby )
  ENDIF(RUBY_FOUND)
ENDIF(ENABLE_SCRIPTS AND ENABLE_RUBY)

IF(ENABLE_SCRIPTS AND ENABLE_LUA)
  FIND_PACKAGE(Lua)
  IF(LUA_FOUND)
    ADD_SUBDIRECTORY( lua )
  ENDIF(LUA_FOUND)
ENDIF(ENABLE_SCRIPTS AND ENABLE_LUA)

IF(ENABLE_SCRIPTS AND ENABLE_TCL)
  FIND_PACKAGE(TCL)
  IF(TCL_FOUND)
    ADD_SUBDIRECTORY( tcl )
  ENDIF(TCL_FOUND)
ENDIF(ENABLE_SCRIPTS AND ENABLE_TCL)

IF(ENABLE_SCRIPTS AND ENABLE_GUILE)
  FIND_PACKAGE(Guile)
  IF(GUILE_FOUND)
    ADD_SUBDIRECTORY( guile )
  ENDIF(GUILE_FOUND)
ENDIF(ENABLE_SCRIPTS AND ENABLE_GUILE)

INSTALL(FILES weechat-plugin.h DESTINATION ${INCLUDEDIR})
