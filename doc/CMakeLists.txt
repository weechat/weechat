#
# Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

IF(ENABLE_DOC)

  FIND_PACKAGE(Sourcehighlight)
  FIND_PACKAGE(Asciidoc)
  IF(ASCIIDOC_FOUND AND SOURCEHIGHLIGHT_FOUND)
    ADD_SUBDIRECTORY( en )
    ADD_SUBDIRECTORY( fr )
    ADD_SUBDIRECTORY( it )
    ADD_SUBDIRECTORY( de )
    ADD_SUBDIRECTORY( pl )
    ADD_SUBDIRECTORY( es )
    ADD_SUBDIRECTORY( ru )
    ADD_SUBDIRECTORY( ja )
  ENDIF(ASCIIDOC_FOUND AND SOURCEHIGHLIGHT_FOUND)

ENDIF(ENABLE_DOC)

INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/weechat-curses.1 DESTINATION ${MANDIR}/man1)
