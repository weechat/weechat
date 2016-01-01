#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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

if(RUBY_FOUND)
   # Already in cache, be silent
   set(RUBY_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_search_module(RUBY ruby-2.2 ruby-2.1 ruby-2.0 ruby-1.9 ruby-1.8)
endif()

if(RUBY_FOUND)
  set(RUBY_LIB "")
  mark_as_advanced(RUBY_LIB)
else()
  find_program(RUBY_EXECUTABLE
    NAMES ruby2.2.3 ruby223 ruby2.2.2 ruby222 ruby2.2.1 ruby221 ruby2.2.0 ruby220 ruby2.2 ruby22 ruby2.1.7 ruby217 ruby2.1.6 ruby216 ruby2.1.5 ruby215 ruby2.1.4 ruby214 ruby2.1.3 ruby213 ruby2.1.2 ruby212 ruby2.1.1 ruby211 ruby2.1.0 ruby210 ruby2.1 ruby21 ruby2.0 ruby20 ruby1.9.3 ruby193 ruby1.9.2 ruby192 ruby1.9.1 ruby191 ruby1.9 ruby19 ruby1.8 ruby18 ruby
    PATHS /usr/bin /usr/local/bin /usr/pkg/bin
    )
  if(RUBY_EXECUTABLE)
    execute_process(
      COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print RbConfig::CONFIG['rubyhdrdir'] || RbConfig::CONFIG['archdir']"
      OUTPUT_VARIABLE RUBY_ARCH_DIR
      )
    execute_process(
      COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print RbConfig::CONFIG['arch']"
      OUTPUT_VARIABLE RUBY_ARCH
      )
    execute_process(
      COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print RbConfig::CONFIG['libdir']"
      OUTPUT_VARIABLE RUBY_POSSIBLE_LIB_PATH
      )
    execute_process(
      COMMAND ${RUBY_EXECUTABLE} -r rbconfig -e "print RbConfig::CONFIG['rubylibdir']"
      OUTPUT_VARIABLE RUBY_RUBY_LIB_PATH
      )
    find_path(RUBY_INCLUDE_DIRS
      NAMES ruby.h
      PATHS ${RUBY_ARCH_DIR}
      )
    set(RUBY_INCLUDE_ARCH "${RUBY_INCLUDE_DIRS}/${RUBY_ARCH}")
    find_library(RUBY_LIB
      NAMES ruby-1.9.3 ruby1.9.3 ruby193 ruby-1.9.2 ruby1.9.2 ruby192 ruby-1.9.1 ruby1.9.1 ruby191 ruby1.9 ruby19 ruby1.8 ruby18 ruby
      PATHS ${RUBY_POSSIBLE_LIB_PATH} ${RUBY_RUBY_LIB_PATH}
      )
    if(RUBY_LIB AND RUBY_INCLUDE_DIRS)
      set(RUBY_FOUND TRUE)
    endif()
    set(RUBY_INCLUDE_DIRS "${RUBY_INCLUDE_DIRS};${RUBY_INCLUDE_ARCH}")
    mark_as_advanced(
      RUBY_INCLUDE_DIRS
      RUBY_LIBRARY_DIRS
      RUBY_LIB
      )
  endif()
endif()
