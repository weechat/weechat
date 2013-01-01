#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

# - Find Aspell
# This module finds if libaspell is installed and determines where
# the include files and libraries are.
#
# This code sets the following variables:
#
#  ASPELL_INCLUDE_PATH = path to where aspell.h can be found
#  ASPELL_LIBRARY = path to where libaspell.so* can be found

IF (ASPELL_FOUND)
  # Already in cache, be silent
  SET(ASPELL_FIND_QUIETLY TRUE)
ENDIF (ASPELL_FOUND)

FIND_PATH(ASPELL_INCLUDE_PATH
  NAMES aspell.h
  PATHS /usr/include /usr/local/include /usr/pkg/include
)

FIND_LIBRARY(ASPELL_LIBRARY
  NAMES aspell aspell-15
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

IF (ASPELL_INCLUDE_PATH AND ASPELL_LIBRARY)
   SET(ASPELL_FOUND TRUE)
ENDIF (ASPELL_INCLUDE_PATH AND ASPELL_LIBRARY)

MARK_AS_ADVANCED(
  ASPELL_INCLUDE_PATH
  ASPELL_LIBRARY
  )
