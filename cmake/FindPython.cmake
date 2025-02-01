#
# Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2009 Julien Louis <ptitlouis@sysif.net>
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

# - Find Python
# This module finds if Python is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
#  PYTHON_EXECUTABLE   = full path to the python binary
#  PYTHON_INCLUDE_DIRS = path to where python.h can be found
#  PYTHON_LIBRARIES = path to where libpython.so* can be found
#  PYTHON_LDFLAGS = python compiler options for linking

pkg_check_modules(PYTHON python3-embed IMPORTED_TARGET GLOBAL)
if(NOT PYTHON_FOUND)
  pkg_check_modules(PYTHON python3 IMPORTED_TARGET GLOBAL)
endif()
