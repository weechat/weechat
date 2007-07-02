# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
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

SET(LIB_SCRIPTS_SRC weechat-script.c weechat-script.h)

ADD_DEFINITIONS(${CMAKE_SHARED_LIBRARY_C_FLAGS})
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_scripts STATIC ${LIB_SCRIPTS_SRC})

IF(NOT DISABLE_RUBY)
  FIND_PACKAGE(Ruby)
  IF(RUBY_FOUND)
    ADD_SUBDIRECTORY( ruby )
  ENDIF(RUBY_FOUND)
ENDIF(NOT DISABLE_RUBY)

IF(NOT DISABLE_LUA)
  FIND_PACKAGE(Lua)
  IF(LUA_FOUND)
    ADD_SUBDIRECTORY( lua )
  ENDIF(LUA_FOUND)
ENDIF(NOT DISABLE_LUA)

IF(NOT DISABLE_PYTHON)
  FIND_PACKAGE(Python)
  IF(PYTHON_FOUND)
    ADD_SUBDIRECTORY( python )
  ENDIF(PYTHON_FOUND)
ENDIF(NOT DISABLE_PYTHON)

IF(NOT DISABLE_PERL)
  FIND_PACKAGE(Perl)
  IF(PERL_FOUND)
    ADD_SUBDIRECTORY( perl )
  ENDIF(PERL_FOUND)
ENDIF(NOT DISABLE_PERL)
