# Copyright (c) 2003-2008 FlashCode <flashcode@flashtux.org>
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

SET(LIB_PLUGINS_SRC weechat-plugin.h plugin.c plugin.h plugin-api.c
plugin-api.h plugin-config.h plugin-config.c plugin-infolist.c
plugin-infolist.h)

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

IF(NOT DISABLE_DEBUG)
  ADD_SUBDIRECTORY( debug )
ENDIF(NOT DISABLE_DEBUG)

IF(NOT DISABLE_FIFO)
  ADD_SUBDIRECTORY( fifo )
ENDIF(NOT DISABLE_FIFO)

IF(NOT DISABLE_IRC)
  ADD_SUBDIRECTORY( irc )
ENDIF(NOT DISABLE_IRC)

IF(NOT DISABLE_LOGGER)
  ADD_SUBDIRECTORY( logger )
ENDIF(NOT DISABLE_LOGGER)

IF(NOT DISABLE_SCRIPTS AND NOT DISABLE_PERL AND NOT DISABLE_PYTHON AND NOT DISABLE_RUBY AND NOT DISABLE_LUA)
  ADD_SUBDIRECTORY( scripts )
ENDIF(NOT DISABLE_SCRIPTS AND NOT DISABLE_PERL AND NOT DISABLE_PYTHON AND NOT DISABLE_RUBY AND NOT DISABLE_LUA)

IF(NOT DISABLE_TRIGGER)
  ADD_SUBDIRECTORY( trigger )
ENDIF(NOT DISABLE_TRIGGER)

IF(NOT DISABLE_XFER)
  ADD_SUBDIRECTORY( xfer )
ENDIF(NOT DISABLE_XFER)
