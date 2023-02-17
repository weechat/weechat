#
# Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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

# - Find CppUTest
# This module finds if CppUTest is installed and determines where the include
# files and libraries are. It also determines what the name of the library is.
# This code sets the following variables:
#
# CPPUTEST_FOUND        = CppUTest is installed
# CPPUTEST_INCLUDE_DIRS = CppUTest include directory
# CPPUTEST_LIBRARIES    = Link options to compile with CppUTest

if(CPPUTEST_FOUND)
  # Already in cache, be silent
  set(CPPUTEST_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_search_module(CPPUTEST cpputest)
endif()
