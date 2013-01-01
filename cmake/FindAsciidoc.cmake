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

# - Find Asciidoc
# This module finds if asciidoc (version 8 or newer) is installed.

IF (ASCIIDOC_FOUND)
  # Already in cache, be silent
  SET(ASCIIDOC_FIND_QUIETLY TRUE)
ENDIF (ASCIIDOC_FOUND)

FIND_PROGRAM(
  ASCIIDOC_EXECUTABLE asciidoc
  PATHS /bin /usr/bin /usr/local/bin /usr/pkg/bin
)

IF(ASCIIDOC_EXECUTABLE)
  EXECUTE_PROCESS(
    COMMAND ${ASCIIDOC_EXECUTABLE} --version
    OUTPUT_VARIABLE ASCIIDOC_VERSION
    )

  IF(${ASCIIDOC_VERSION} MATCHES "asciidoc 8.*")
    SET(ASCIIDOC_FOUND TRUE)
  ENDIF(${ASCIIDOC_VERSION} MATCHES "asciidoc 8.*")

  MARK_AS_ADVANCED(
    ASCIIDOC_EXECUTABLE
    )
ENDIF(ASCIIDOC_EXECUTABLE)
