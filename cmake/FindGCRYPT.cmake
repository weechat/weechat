#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

# - Find Gcrypt
# This module finds if libgcrypt is installed and determines where
# the include files and libraries are.
#
# This code sets the following variables:
#
#  GCRYPT_CFLAGS = cflags to use to compile
#  GCRYPT_LDFLAGS = ldflags to use to compile
#

find_program(LIBGCRYPT_CONFIG_EXECUTABLE NAMES libgcrypt-config)

set(GCRYPT_LDFLAGS)
set(GCRYPT_CFLAGS)

if(LIBGCRYPT_CONFIG_EXECUTABLE)

  execute_process(COMMAND ${LIBGCRYPT_CONFIG_EXECUTABLE} --libs RESULT_VARIABLE _return_VALUE OUTPUT_VARIABLE GCRYPT_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  execute_process(COMMAND ${LIBGCRYPT_CONFIG_EXECUTABLE} --cflags RESULT_VARIABLE _return_VALUE OUTPUT_VARIABLE GCRYPT_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

  if(NOT DEFINED ${GCRYPT_CFLAGS})
    set(GCRYPT_CFLAGS " ")
  endif()

endif()

# handle the QUIETLY and REQUIRED arguments and set GCRYPT_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GCRYPT REQUIRED_VARS GCRYPT_LDFLAGS GCRYPT_CFLAGS)

if(GCRYPT_FOUND)
  mark_as_advanced(GCRYPT_CFLAGS GCRYPT_LDFLAGS)
endif()
