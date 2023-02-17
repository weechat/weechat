#
# Copyright (C) 2011-2023 Sébastien Helleu <flashcode@flashtux.org>
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

# - Find Guile
# This module finds if Guile is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
# GUILE_FOUND        = Guile is installed
# GUILE_INCLUDE_DIRS = Guile include directory
# GUILE_LIBRARIES    = Link options to compile Guile

if(GUILE_FOUND)
  # Already in cache, be silent
  set(GUILE_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_search_module(GUILE guile-3.0 guile-2.2 guile-2.0)
  if(GUILE_FOUND)
    # check if variable "scm_install_gmp_memory_functions" exists
    set(CMAKE_REQUIRED_INCLUDES ${GUILE_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_LIBRARIES ${GUILE_LDFLAGS})
    check_symbol_exists(scm_install_gmp_memory_functions "libguile.h" HAVE_GUILE_GMP_MEMORY_FUNCTIONS)
    set(CMAKE_REQUIRED_INCLUDES)
    set(CMAKE_REQUIRED_LIBRARIES)
  endif()
endif()
