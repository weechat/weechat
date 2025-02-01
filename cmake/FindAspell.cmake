#
# Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

# - Find Aspell
# This module finds if libaspell is installed and determines where
# the include files and libraries are.
#
# This code sets the following variables:
#
#  ASPELL_INCLUDE_PATH = path to where aspell.h can be found
#  ASPELL_LIBRARY = path to where libaspell.so* can be found

if(ASPELL_FOUND)
  # Already in cache, be silent
  SET(ASPELL_FIND_QUIETLY TRUE)
endif()

find_path(ASPELL_INCLUDE_PATH
  NAMES aspell.h
  PATHS /usr/include /usr/local/include /usr/pkg/include
)

find_library(ASPELL_LIBRARY
  NAMES aspell aspell-15
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

if(ASPELL_INCLUDE_PATH AND ASPELL_LIBRARY)
  set(ASPELL_FOUND TRUE)

  # check if function aspell_version_string() exists
  set(CMAKE_REQUIRED_INCLUDES ${ASPELL_INCLUDE_PATH})
  set(CMAKE_REQUIRED_LIBRARIES ${ASPELL_LIBRARY})
  check_symbol_exists(aspell_version_string "aspell.h" HAVE_ASPELL_VERSION_STRING)
  set(CMAKE_REQUIRED_INCLUDES)
  set(CMAKE_REQUIRED_LIBRARIES)
endif()

mark_as_advanced(
  ASPELL_INCLUDE_PATH
  ASPELL_LIBRARY
)
