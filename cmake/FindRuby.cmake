#
# Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
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

# - Find Ruby
# This module finds if Ruby is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
#  RUBY_INCLUDE_DIRS = C flags to compile with ruby
#  RUBY_LIBRARY_DIRS = linker flags to compile with ruby (found with pkg-config)
#  RUBY_LIB          = ruby library (found without pkg-config)

IF(RUBY_FOUND)
   # Already in cache, be silent
   SET(RUBY_FIND_QUIETLY TRUE)
ENDIF(RUBY_FOUND)

FIND_PACKAGE(PkgConfig)
IF(PKG_CONFIG_FOUND)
  pkg_search_module(RUBY ruby-2.0 ruby-1.9 ruby-1.8)
ENDIF(PKG_CONFIG_FOUND)

IF(RUBY_FOUND)

  SET(RUBY_LIB "")
  MARK_AS_ADVANCED(RUBY_LIB)

ELSE(RUBY_FOUND)

  FIND_PROGRAM(RUBY_EXECUTABLE
    NAMES ruby1.9.3 ruby193 ruby1.9.2 ruby192 ruby1.9.1 ruby191 ruby1.9 ruby19 ruby1.8 ruby18 ruby
    PATHS /usr/bin /usr/local/bin /usr/pkg/bin
    )

  IF(RUBY_EXECUTABLE)

    EXECUTE_PROCESS(
      COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print RbConfig::CONFIG['rubyhdrdir'] || RbConfig::CONFIG['archdir']"
      OUTPUT_VARIABLE RUBY_ARCH_DIR
      )

    EXECUTE_PROCESS(
      COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print RbConfig::CONFIG['arch']"
      OUTPUT_VARIABLE RUBY_ARCH
      )

    EXECUTE_PROCESS(
      COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print RbConfig::CONFIG['libdir']"
      OUTPUT_VARIABLE RUBY_POSSIBLE_LIB_PATH
      )

    EXECUTE_PROCESS(
      COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print RbConfig::CONFIG['rubylibdir']"
      OUTPUT_VARIABLE RUBY_RUBY_LIB_PATH
      )

    FIND_PATH(RUBY_INCLUDE_DIRS
      NAMES ruby.h
      PATHS ${RUBY_ARCH_DIR}
      )

    SET(RUBY_INCLUDE_ARCH "${RUBY_INCLUDE_DIRS}/${RUBY_ARCH}")

    FIND_LIBRARY(RUBY_LIB
      NAMES ruby-1.9.3 ruby1.9.3 ruby193 ruby-1.9.2 ruby1.9.2 ruby192 ruby-1.9.1 ruby1.9.1 ruby191 ruby1.9 ruby19 ruby1.8 ruby18 ruby
      PATHS ${RUBY_POSSIBLE_LIB_PATH} ${RUBY_RUBY_LIB_PATH}
      )

    IF(RUBY_LIB AND RUBY_INCLUDE_DIRS)
      SET(RUBY_FOUND TRUE)
    ENDIF(RUBY_LIB AND RUBY_INCLUDE_DIRS)

    SET(RUBY_INCLUDE_DIRS "${RUBY_INCLUDE_DIRS};${RUBY_INCLUDE_ARCH}")

    MARK_AS_ADVANCED(
      RUBY_INCLUDE_DIRS
      RUBY_LIBRARY_DIRS
      RUBY_LIB
      )

  ENDIF(RUBY_EXECUTABLE)

ENDIF(RUBY_FOUND)
