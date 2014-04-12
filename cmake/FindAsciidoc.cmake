#
# Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
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

if(ASCIIDOC_FOUND)
  # Already in cache, be silent
  set(ASCIIDOC_FIND_QUIETLY TRUE)
endif()

find_program(
  ASCIIDOC_EXECUTABLE asciidoc
  PATHS /bin /usr/bin /usr/local/bin /usr/pkg/bin
)

find_program(
  A2X_EXECUTABLE a2x
  PATHS /bin /usr/bin /usr/local/bin /usr/pkg/bin
)

if(ASCIIDOC_EXECUTABLE AND A2X_EXECUTABLE)
  execute_process(
    COMMAND ${ASCIIDOC_EXECUTABLE} --version
    OUTPUT_VARIABLE ASCIIDOC_VERSION
    )

  if(${ASCIIDOC_VERSION} MATCHES "asciidoc 8.*")
    set(ASCIIDOC_FOUND TRUE)
  endif()

  mark_as_advanced(
    ASCIIDOC_EXECUTABLE
    )
endif()
