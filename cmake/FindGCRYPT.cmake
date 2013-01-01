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

# - Find Gcrypt
# This module finds if libgcrypt is installed and determines where
# the include files and libraries are.
#
# This code sets the following variables:
#
#  GCRYPT_CFLAGS = cflags to use to compile
#  GCRYPT_LDFLAGS = ldflags to use to compile
#

FIND_PROGRAM(LIBGCRYPT_CONFIG_EXECUTABLE NAMES libgcrypt-config)

set(GCRYPT_LDFLAGS)
set(GCRYPT_CFLAGS)

IF(LIBGCRYPT_CONFIG_EXECUTABLE)

   EXEC_PROGRAM(${LIBGCRYPT_CONFIG_EXECUTABLE} ARGS --libs RETURN_VALUE _return_VALUE OUTPUT_VARIABLE GCRYPT_LDFLAGS)
   EXEC_PROGRAM(${LIBGCRYPT_CONFIG_EXECUTABLE} ARGS --cflags RETURN_VALUE _return_VALUE OUTPUT_VARIABLE GCRYPT_CFLAGS)

   IF(${GCRYPT_CFLAGS} MATCHES "\n")
      SET(GCRYPT_CFLAGS " ")
   ENDIF(${GCRYPT_CFLAGS} MATCHES "\n")

ENDIF(LIBGCRYPT_CONFIG_EXECUTABLE)

# handle the QUIETLY and REQUIRED arguments and set GCRYPT_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(${CMAKE_HOME_DIRECTORY}/cmake/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GCRYPT REQUIRED_VARS GCRYPT_LDFLAGS GCRYPT_CFLAGS)

IF(GCRYPT_FOUND)
  MARK_AS_ADVANCED(GCRYPT_CFLAGS GCRYPT_LDFLAGS)
ENDIF(GCRYPT_FOUND)
