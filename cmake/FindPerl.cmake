#
# Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

# - Find Perl libraries
# This module finds if Perl is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
#  PERL_EXECUTABLE   = full path to the perl binary
#  PERL_INCLUDE_PATH = path to where perl.h can be found
#  PERL_LIBRARY = path to where libperl.so* can be found
#  PERL_CFLAGS = perl compiler options for compiling
#  PERL_LFLAGS = perl compiler options for linking

if(PERL_FOUND)
  # Already in cache, be silent
  set(PERL_FIND_QUIETLY TRUE)
endif()

find_program(PERL_EXECUTABLE
  NAMES perl perl5
  PATHS /usr/bin /usr/local/bin /usr/pkg/bin
)

if(PERL_EXECUTABLE)

  execute_process(
    COMMAND ${PERL_EXECUTABLE} -MConfig -e "print \"\$Config{archlibexp}/CORE\""
    OUTPUT_VARIABLE PERL_INTERNAL_DIR
  )

  execute_process(
    COMMAND ${PERL_EXECUTABLE} -MExtUtils::Embed -e ccopts
    OUTPUT_VARIABLE PERL_CFLAGS
  )

  execute_process(
    COMMAND ${PERL_EXECUTABLE} -MExtUtils::Embed -e ldopts
    OUTPUT_VARIABLE PERL_LFLAGS
  )

  # remove the new lines from the output by replacing them with empty strings
  string(REPLACE "\n" "" PERL_INTERNAL_DIR "${PERL_INTERNAL_DIR}")
  string(REPLACE "\n" "" PERL_CFLAGS "${PERL_CFLAGS}")
  string(REPLACE "\n" "" PERL_LFLAGS "${PERL_LFLAGS}")

  find_path(PERL_INCLUDE_PATH
    NAMES perl.h
    PATHS ${PERL_INTERNAL_DIR}
  )

  find_library(PERL_LIBRARY
    NAMES perl
    PATHS /usr/lib /usr/local/lib /usr/pkg/lib ${PERL_INTERNAL_DIR}
  )

  if(PERL_LIBRARY AND PERL_INCLUDE_PATH)
    set(PERL_FOUND TRUE)
  endif()

  mark_as_advanced(
    PERL_EXECUTABLE
    PERL_INCLUDE_PATH
    PERL_LIBRARY
    PERL_CFLAGS
    PERL_LFLAGS
  )
endif()
