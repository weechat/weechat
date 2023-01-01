#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2007 Julien Louis <ptitlouis@sysif.net>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

# - Find Gettext
# This module finds if gettext is installed and determines where
# the include files and libraries are.
#
# This code sets the following variables:
#
#  GETTEXT_FOUND = is gettext usable on system?

if(GETTEXT_FOUND)
  # Already in cache, be silent
  set(GETTEXT_FIND_QUIETLY TRUE)
endif()

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckFunctionExists)

find_path(LIBINTL_INCLUDE
  NAMES libintl.h
  PATH /usr/local/include /usr/pkg/include /usr/include
)

set(CMAKE_REQUIRED_INCLUDES ${LIBINTL_INCLUDE})

check_include_files(libintl.h HAVE_LIBINTL_H)

if(HAVE_LIBINTL_H)
  check_function_exists(dgettext LIBC_HAS_DGETTEXT)
  if(LIBC_HAS_DGETTEXT)
    set(GETTEXT_FOUND TRUE)
  else()
    find_library(LIBINTL_LIBRARY NAMES intl
      PATHS
      /usr/local/lib
      /usr/lib
    )
    if(LIBINTL_LIBRARY)
      if(${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
        set(CMAKE_REQUIRED_LIBRARIES "iconv")
        check_library_exists(${LIBINTL_LIBRARY} "libintl_dgettext" "" LIBINTL_HAS_DGETTEXT)
      else()
        check_library_exists(${LIBINTL_LIBRARY} "dgettext" "" LIBINTL_HAS_DGETTEXT)
      endif()
      if(LIBINTL_HAS_DGETTEXT)
        set(GETTEXT_FOUND TRUE)
      endif()
    endif()
  endif()
endif()
