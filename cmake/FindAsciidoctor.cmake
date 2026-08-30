# SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# - Find Asciidoctor
# This module finds if asciidoctor (version 1.5.4 or newer) is installed.

if(ASCIIDOCTOR_FOUND)
  # Already in cache, be silent.
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
