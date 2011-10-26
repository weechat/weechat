#
# Copyright (C) 2003-2011 Sebastien Helleu <flashcode@flashtux.org>
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

SET(LIB_SCRIPTS_SRC script.c script.h script-callback.c script-callback.h
script-api.c script-api.h)

ADD_DEFINITIONS(${CMAKE_SHARED_LIBRARY_C_FLAGS})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_scripts STATIC ${LIB_SCRIPTS_SRC})

IF(ENABLE_PERL)
  FIND_PACKAGE(Perl)
  IF(PERL_FOUND)
    ADD_SUBDIRECTORY( perl )
  ENDIF(PERL_FOUND)
ENDIF(ENABLE_PERL)

IF(ENABLE_PYTHON)
  FIND_PACKAGE(Python)
  IF(PYTHON_FOUND)
    ADD_SUBDIRECTORY( python )
  ENDIF(PYTHON_FOUND)
ENDIF(ENABLE_PYTHON)

IF(ENABLE_RUBY)
  FIND_PACKAGE(Ruby)
  IF(RUBY_FOUND)
    ADD_SUBDIRECTORY( ruby )
  ENDIF(RUBY_FOUND)
ENDIF(ENABLE_RUBY)

IF(ENABLE_LUA)
  FIND_PACKAGE(Lua)
  IF(LUA_FOUND)
    ADD_SUBDIRECTORY( lua )
  ENDIF(LUA_FOUND)
ENDIF(ENABLE_LUA)

IF(ENABLE_TCL)
  FIND_PACKAGE(TCL)
  IF(TCL_FOUND)
    ADD_SUBDIRECTORY( tcl )
  ENDIF(TCL_FOUND)
ENDIF(ENABLE_TCL)

IF(ENABLE_GUILE)
  FIND_PACKAGE(Guile)
  IF(GUILE_FOUND)
    ADD_SUBDIRECTORY( guile )
  ENDIF(GUILE_FOUND)
ENDIF(ENABLE_GUILE)
