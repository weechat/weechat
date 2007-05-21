# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

SET(LIB_PLUGINS_SRC weechat-plugin.h plugins.h plugins.c plugins-interface.c
plugins-config.h plugins-config.c)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_plugins STATIC ${LIB_PLUGINS_SRC})

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)

IF(NOT DISABLE_ASPELL)
  # Check for aspell libraries
  FIND_PACKAGE(Aspell)
  IF(ASPELL_FOUND)
    ADD_SUBDIRECTORY( aspell )
  ENDIF(ASPELL_FOUND)
ENDIF(NOT DISABLE_ASPELL)

IF (NOT DISABLE_CHARSET)
  # Check for iconv support.
  FIND_PACKAGE(Iconv)
  IF(ICONV_FOUND)
    ADD_DEFINITIONS( -DHAVE_ICONV )
    ADD_SUBDIRECTORY( charset )
  ENDIF(ICONV_FOUND)
ENDIF(NOT DISABLE_CHARSET)

ADD_SUBDIRECTORY( scripts )
