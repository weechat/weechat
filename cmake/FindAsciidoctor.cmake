#
# Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

# - Find Asciidoctor
# This module finds if asciidoctor (version 1.5.4 or newer) is installed.

if(ASCIIDOCTOR_FOUND)
  # Already in cache, be silent
  set(ASCIIDOCTOR_FIND_QUIETLY TRUE)
endif()

find_program(
  ASCIIDOCTOR_EXECUTABLE asciidoctor
  PATHS /bin /usr/bin /usr/local/bin /usr/pkg/bin
)

if(ASCIIDOCTOR_EXECUTABLE)
  execute_process(
    COMMAND ${ASCIIDOCTOR_EXECUTABLE} --version
    OUTPUT_VARIABLE ASCIIDOCTOR_VERSION
  )

  string(REGEX REPLACE "^Asciidoctor ([^ ]+) .*" "\\1" ASCIIDOCTOR_VERSION "${ASCIIDOCTOR_VERSION}")

  if(ASCIIDOCTOR_VERSION VERSION_EQUAL "1.5.4" OR ASCIIDOCTOR_VERSION VERSION_GREATER "1.5.4")
    set(ASCIIDOCTOR_FOUND TRUE)
  endif()

  mark_as_advanced(
    ASCIIDOCTOR_EXECUTABLE
    ASCIIDOCTOR_VERSION
  )
endif()
