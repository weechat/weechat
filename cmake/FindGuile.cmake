#
# Copyright (C) 2011-2013 Sebastien Helleu <flashcode@flashtux.org>
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

# - Find Guile
# This module finds if Guile is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
# GUILE_FOUND        = Guile is installed
# GUILE_INCLUDE_DIRS = Guile include directory
# GUILE_LIBRARIES    = Link options to compile Guile

IF(GUILE_FOUND)
   # Already in cache, be silent
   SET(GUILE_FIND_QUIETLY TRUE)
ENDIF(GUILE_FOUND)

FIND_PATH(GUILE_INCLUDE_DIR libguile.h /usr/include /usr/local/include /usr/include/libguile /usr/local/include/libguile)

FIND_LIBRARY(GUILE_LIBRARY NAMES guile PATH /usr/lib /usr/local/lib)

IF(GUILE_INCLUDE_DIR AND GUILE_LIBRARY)
  SET(GUILE_FOUND TRUE)
  SET(GUILE_INCLUDE_DIRS ${GUILE_INCLUDE_DIR})
  SET(GUILE_LIBRARIES ${GUILE_LIBRARY})
ELSE(GUILE_INCLUDE_DIR AND GUILE_LIBRARY)
  SET(GUILE_FOUND FALSE)
  SET(GUILE_INCLUDE_DIRS)
  SET(GUILE_LIBRARIES)
ENDIF(GUILE_INCLUDE_DIR AND GUILE_LIBRARY)

MARK_AS_ADVANCED(
  GUILE_INCLUDE_DIR
  GUILE_LIBRARY
  )
