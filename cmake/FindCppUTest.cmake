# SPDX-FileCopyrightText: 2014-2026 Sébastien Helleu <flashcode@flashtux.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# - Find CppUTest
# This module finds if CppUTest is installed and determines where the include
# files and libraries are. It also determines what the name of the library is.
# This code sets the following variables:
#
# CPPUTEST_FOUND        = CppUTest is installed
# CPPUTEST_INCLUDE_DIRS = CppUTest include directory
# CPPUTEST_LIBRARIES    = Link options to compile with CppUTest

if(CPPUTEST_FOUND)
  # Already in cache, be silent.
  set(CPPUTEST_FIND_QUIETLY TRUE)
endif()

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_search_module(CPPUTEST cpputest)
endif()
