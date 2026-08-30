# SPDX-FileCopyrightText: 2011-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# - Find Guile
# This module finds if Guile is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
# GUILE_FOUND        = Guile is installed
# GUILE_INCLUDE_DIRS = Guile include directory
# GUILE_LIBRARIES    = Link options to compile Guile

if(GUILE_FOUND)
  # Already in cache, be silent.
  set(GUILE_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_search_module(GUILE guile-3.0 guile-2.2 guile-2.0)
  if(GUILE_FOUND)
    # Check if variable "scm_install_gmp_memory_functions" exists.
    set(CMAKE_REQUIRED_INCLUDES ${GUILE_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_LIBRARIES ${GUILE_LDFLAGS})
    check_symbol_exists(scm_install_gmp_memory_functions "libguile.h" HAVE_GUILE_GMP_MEMORY_FUNCTIONS)
    set(CMAKE_REQUIRED_INCLUDES)
    set(CMAKE_REQUIRED_LIBRARIES)
  endif()
endif()
