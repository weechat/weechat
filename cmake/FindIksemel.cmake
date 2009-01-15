# Copyright (c) 2003-2009 FlashCode <flashcode@flashtux.org>
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

# - Find Iksemel
# This module finds if libiksemel is installed and determines where 
# the include files and libraries are.
#
# This code sets the following variables:
#
#  IKSEMEL_INCLUDE_PATH = path to where iksemel.h can be found
#  IKSEMEL_LIBRARY = path to where libiksemel.so* can be found

IF (IKSEMEL_FOUND)
  # Already in cache, be silent
  SET(IKSEMEL_FIND_QUIETLY TRUE)
ENDIF (IKSEMEL_FOUND)

FIND_PATH(IKSEMEL_INCLUDE_PATH
  NAMES iksemel.h
  PATHS /usr/include /usr/local/include /usr/pkg/include
)

FIND_LIBRARY(IKSEMEL_LIBRARY
  NAMES iksemel
  PATHS /lib /usr/lib /usr/local/lib /usr/pkg/lib
)

IF (IKSEMEL_INCLUDE_PATH AND IKSEMEL_LIBRARY)
   SET(IKSEMEL_FOUND TRUE)
ENDIF (IKSEMEL_INCLUDE_PATH AND IKSEMEL_LIBRARY)

MARK_AS_ADVANCED(
  IKSEMEL_INCLUDE_PATH
  IKSEMEL_LIBRARY
  )
