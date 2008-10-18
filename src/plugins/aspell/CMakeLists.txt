# Copyright (c) 2003-2008 FlashCode <flashcode@flashtux.org>
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

ADD_LIBRARY(aspell MODULE
aspell.c aspell.h
aspell-config.c aspell-config.h
aspell-speller.c aspell-speller.h)
SET_TARGET_PROPERTIES(aspell PROPERTIES PREFIX "")

IF(ASPELL_FOUND)
  INCLUDE_DIRECTORIES(${ASPELL_INCLUDE_PATH})
  TARGET_LINK_LIBRARIES(aspell ${ASPELL_LIBRARY})
ENDIF(ASPELL_FOUND)

INSTALL(TARGETS aspell LIBRARY DESTINATION lib/${PROJECT_NAME}/plugins)
