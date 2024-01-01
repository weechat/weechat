#
# Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # set specific search path for macOS
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:/usr/local/opt/ruby/lib/pkgconfig")
  endif()
  pkg_search_module(RUBY ruby-3.3 ruby-3.2 ruby-3.1 ruby-3.0 ruby-2.7 ruby-2.6 ruby-2.5 ruby-2.4 ruby-2.3 ruby-2.2 ruby-2.1 ruby-2.0 ruby-1.9 ruby)
  if(RUBY_FOUND AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # FIXME: weird hack: hardcoding the Ruby lib location on macOS
    set(RUBY_LDFLAGS "${RUBY_LDFLAGS} -L/usr/local/opt/ruby/lib")
  endif()
endif()
