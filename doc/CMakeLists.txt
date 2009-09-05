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

IF(NOT DISABLE_DOC)

  FIND_PACKAGE(Sourcehighlight)
  FIND_PACKAGE(Asciidoc)
  IF(ASCIIDOC_FOUND AND SOURCEHIGHLIGHT_FOUND)
    ADD_SUBDIRECTORY( en )
    ADD_SUBDIRECTORY( fr )
    ADD_SUBDIRECTORY( pl )
    ADD_SUBDIRECTORY( de )
  ENDIF(ASCIIDOC_FOUND AND SOURCEHIGHLIGHT_FOUND)

ENDIF(NOT DISABLE_DOC)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/weechat-curses.1 DESTINATION share/man/man1)
