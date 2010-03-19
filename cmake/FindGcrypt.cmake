# Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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
   
   IF(GCRYPT_LDFLAGS AND GCRYPT_CFLAGS)
      SET(GCRYPT_FOUND TRUE)
   ENDIF(GCRYPT_LDFLAGS AND GCRYPT_CFLAGS)

ENDIF(LIBGCRYPT_CONFIG_EXECUTABLE)

MARK_AS_ADVANCED(GCRYPT_CFLAGS GCRYPT_LDFLAGS)
