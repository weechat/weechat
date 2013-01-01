#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2009 Emmanuel Bouthenot <kolter@openics.org>
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

# - Find Source-Highlight
# This module finds if source-highlight is installed.

IF (SOURCEHIGHLIGHT_FOUND)
  # Already in cache, be silent
  SET(SOURCEHIGHLIGHT_FIND_QUIETLY TRUE)
ENDIF (SOURCEHIGHLIGHT_FOUND)

FIND_PROGRAM(
  SOURCEHIGHLIGHT_EXECUTABLE source-highlight
  PATHS /bin /usr/bin /usr/local/bin /usr/pkg/bin
)

IF(SOURCEHIGHLIGHT_EXECUTABLE)
  SET(SOURCEHIGHLIGHT_FOUND TRUE)
  MARK_AS_ADVANCED(
    SOURCEHIGHLIGHT_EXECUTABLE
    )
ENDIF(SOURCEHIGHLIGHT_EXECUTABLE)
