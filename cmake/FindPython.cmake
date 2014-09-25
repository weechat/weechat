#
# Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

# - Find Python
# This module finds if Python is installed and determines where the include files
# and libraries are. It also determines what the name of the library is. This
# code sets the following variables:
#
#  PYTHON_EXECUTABLE   = full path to the python binary
#  PYTHON_INCLUDE_PATH = path to where python.h can be found
#  PYTHON_LIBRARY = path to where libpython.so* can be found
#  PYTHON_LFLAGS = python compiler options for linking

if(PYTHON_FOUND)
   # Already in cache, be silent
   set(PYTHON_FIND_QUIETLY TRUE)
endif()

if(ENABLE_PYTHON3)
  find_program(PYTHON_EXECUTABLE
    NAMES python3.4 python3.3 python3.2 python3.1 python3.0 python3 python2.7 python2.6 python2.5 python
    PATHS /usr/bin /usr/local/bin /usr/pkg/bin
    )
else()
  find_program(PYTHON_EXECUTABLE
    NAMES python2.7 python2.6 python2.5 python
    PATHS /usr/bin /usr/local/bin /usr/pkg/bin
    )
endif()

if(PYTHON_EXECUTABLE)
  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import sys; from distutils.sysconfig import *; sys.stdout.write(get_config_var('INCLUDEPY'))"
    OUTPUT_VARIABLE PYTHON_INC_DIR
    )

  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import sys; from distutils.sysconfig import *; sys.stdout.write(get_config_var('LIBPL'))"
    OUTPUT_VARIABLE PYTHON_POSSIBLE_LIB_PATH
    )

  execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import sys; from distutils.sysconfig import *; sys.stdout.write(get_config_var('LINKFORSHARED'))"
    OUTPUT_VARIABLE PYTHON_LFLAGS
    )

  find_path(PYTHON_INCLUDE_PATH
    NAMES Python.h
    HINTS ${PYTHON_INC_DIR}
    )
  if(ENABLE_PYTHON3)
    find_library(PYTHON_LIBRARY
      NAMES python3.4 python3.3 python3.2 python3.1 python3.0 python3 python2.7 python2.6 python2.5 python
      HINTS ${PYTHON_POSSIBLE_LIB_PATH}
      )
  else()
    find_library(PYTHON_LIBRARY
      NAMES python2.7 python2.6 python2.5 python
      HINTS ${PYTHON_POSSIBLE_LIB_PATH}
      )
  endif()

  if(PYTHON_LIBRARY AND PYTHON_INCLUDE_PATH)
    execute_process(
      COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write(sys.version[:3])"
      OUTPUT_VARIABLE PYTHON_VERSION
      )
    execute_process(
      COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write(str(sys.version_info < (2,5)))"
      OUTPUT_VARIABLE PYTHON_OLD_VERSION
      )
    if(${PYTHON_OLD_VERSION} STREQUAL "True")
      message("Python >= 2.5 is needed to build python plugin, version found: ${PYTHON_VERSION}")
    else()
      set(PYTHON_FOUND TRUE)
    endif()
  endif()

  mark_as_advanced(
    PYTHON_EXECUTABLE
    PYTHON_INCLUDE_PATH
    PYTHON_LIBRARY
    PYTHON_LFLAGS
    )

endif()
