# Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

SET(LIB_PLUGINS_SRC
weechat-plugin.h
plugin.c plugin.h
plugin-api.c plugin-api.h
plugin-config.h plugin-config.c)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_plugins STATIC ${LIB_PLUGINS_SRC})

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)

IF(NOT DISABLE_ALIAS)
  ADD_SUBDIRECTORY( alias )
ENDIF(NOT DISABLE_ALIAS)

IF(NOT DISABLE_ASPELL)
  # Check for aspell libraries
  FIND_PACKAGE(Aspell)
  IF(ASPELL_FOUND)
    ADD_SUBDIRECTORY( aspell )
  ENDIF(ASPELL_FOUND)
ENDIF(NOT DISABLE_ASPELL)

IF(NOT DISABLE_CHARSET)
  # Check for iconv support.
  IF(ICONV_FOUND)
    ADD_SUBDIRECTORY( charset )
  ENDIF(ICONV_FOUND)
ENDIF(NOT DISABLE_CHARSET)

IF(ENABLE_DEMO)
  ADD_SUBDIRECTORY( demo )
ENDIF(ENABLE_DEMO)

IF(NOT DISABLE_FIFO)
  ADD_SUBDIRECTORY( fifo )
ENDIF(NOT DISABLE_FIFO)

IF(NOT DISABLE_IRC)
  ADD_SUBDIRECTORY( irc )
ENDIF(NOT DISABLE_IRC)

IF(NOT DISABLE_LOGGER)
  ADD_SUBDIRECTORY( logger )
ENDIF(NOT DISABLE_LOGGER)

IF(ENABLE_RELAY)
  ADD_SUBDIRECTORY( relay )
ENDIF(ENABLE_RELAY)

IF(NOT DISABLE_SCRIPTS OR NOT DISABLE_PERL OR NOT DISABLE_PYTHON OR NOT DISABLE_RUBY OR NOT DISABLE_LUA OR NOT DISABLE_TCL)
  ADD_SUBDIRECTORY( scripts )
ENDIF(NOT DISABLE_SCRIPTS OR NOT DISABLE_PERL OR NOT DISABLE_PYTHON OR NOT DISABLE_RUBY OR NOT DISABLE_LUA OR NOT DISABLE_TCL)

IF(NOT DISABLE_XFER)
  ADD_SUBDIRECTORY( xfer )
ENDIF(NOT DISABLE_XFER)

INSTALL(FILES weechat-plugin.h DESTINATION ${INCLUDEDIR})
