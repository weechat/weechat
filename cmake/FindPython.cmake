#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

IF(PYTHON_FOUND)
   # Already in cache, be silent
   SET(PYTHON_FIND_QUIETLY TRUE)
ENDIF(PYTHON_FOUND)

IF(ENABLE_PYTHON3)
  FIND_PROGRAM(PYTHON_EXECUTABLE
    NAMES python3.4 python3.3 python3.2 python3.1 python3.0 python3 python2.7 python2.6 python2.5 python
    PATHS /usr/bin /usr/local/bin /usr/pkg/bin
    )
ELSE(ENABLE_PYTHON3)
  FIND_PROGRAM(PYTHON_EXECUTABLE
    NAMES python2.7 python2.6 python2.5 python
    PATHS /usr/bin /usr/local/bin /usr/pkg/bin
    )
ENDIF(ENABLE_PYTHON3)

IF(PYTHON_EXECUTABLE)
  EXECUTE_PROCESS(
    COMMAND ${PYTHON_EXECUTABLE} -c "import sys; from distutils.sysconfig import *; sys.stdout.write(get_config_var('INCLUDEPY'))"
    OUTPUT_VARIABLE PYTHON_INC_DIR
    )

  EXECUTE_PROCESS(
    COMMAND ${PYTHON_EXECUTABLE} -c "import sys; from distutils.sysconfig import *; sys.stdout.write(get_config_var('LIBPL'))"
    OUTPUT_VARIABLE PYTHON_POSSIBLE_LIB_PATH
    )

  EXECUTE_PROCESS(
    COMMAND ${PYTHON_EXECUTABLE} -c "import sys; from distutils.sysconfig import *; sys.stdout.write(get_config_var('LINKFORSHARED'))"
    OUTPUT_VARIABLE PYTHON_LFLAGS
    )

  FIND_PATH(PYTHON_INCLUDE_PATH
    NAMES Python.h
    PATHS ${PYTHON_INC_DIR}
    )
  IF(ENABLE_PYTHON3)
    FIND_LIBRARY(PYTHON_LIBRARY
      NAMES python3.4 python3.3 python3.2 python3.1 python3.0 python3 python2.7 python2.6 python2.5 python
      PATHS ${PYTHON_POSSIBLE_LIB_PATH}
      )
  ELSE(ENABLE_PYTHON3)
    FIND_LIBRARY(PYTHON_LIBRARY
      NAMES python2.7 python2.6 python2.5 python
      PATHS ${PYTHON_POSSIBLE_LIB_PATH}
      )
  ENDIF(ENABLE_PYTHON3)

  IF(PYTHON_LIBRARY AND PYTHON_INCLUDE_PATH)
    EXECUTE_PROCESS(
      COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write(sys.version[:3])"
      OUTPUT_VARIABLE PYTHON_VERSION
      )
    EXECUTE_PROCESS(
      COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write(str(sys.version_info < (2,5)))"
      OUTPUT_VARIABLE PYTHON_OLD_VERSION
      )
    IF(${PYTHON_OLD_VERSION} STREQUAL "True")
      MESSAGE("Python >= 2.5 is needed to build python plugin, version found: ${PYTHON_VERSION}")
    ELSE()
      SET(PYTHON_FOUND TRUE)
    ENDIF(${PYTHON_OLD_VERSION} STREQUAL "True")
  ENDIF(PYTHON_LIBRARY AND PYTHON_INCLUDE_PATH)

  MARK_AS_ADVANCED(
    PYTHON_EXECUTABLE
    PYTHON_INCLUDE_PATH
    PYTHON_LIBRARY
    PYTHON_LFLAGS
    )

ENDIF(PYTHON_EXECUTABLE)
