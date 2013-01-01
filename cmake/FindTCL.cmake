#
# Copyright (C) 2008 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2008-2013 Sebastien Helleu <flashcode@flashtux.org>
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

# - Find Tcl includes and libraries.
# This module finds if Tcl is installed and determines where the
# include files and libraries are. It also determines what the name of
# the library is. This code sets the following variables:
#  TCL_FOUND              = Tcl was found
#  TK_FOUND               = Tk was found
#  TCLTK_FOUND            = Tcl and Tk were found
#  TCL_LIBRARY            = path to Tcl library (tcl tcl80)
#  TCL_INCLUDE_PATH       = path to where tcl.h can be found
#  TCL_TCLSH              = path to tclsh binary (tcl tcl80)
#  TK_LIBRARY             = path to Tk library (tk tk80 etc)
#  TK_INCLUDE_PATH        = path to where tk.h can be found
#  TK_WISH                = full path to the wish executable
#
# In an effort to remove some clutter and clear up some issues for people
# who are not necessarily Tcl/Tk gurus/developpers, some variables were
# moved or removed. Changes compared to CMake 2.4 are:
# - The stub libraries are now found in FindTclStub.cmake
#   => they were only useful for people writing Tcl/Tk extensions.
# - TCL_LIBRARY_DEBUG and TK_LIBRARY_DEBUG were removed.
#   => these libs are not packaged by default with Tcl/Tk distributions.
#      Even when Tcl/Tk is built from source, several flavors of debug libs
#      are created and there is no real reason to pick a single one
#      specifically (say, amongst tcl84g, tcl84gs, or tcl84sgx).
#      Let's leave that choice to the user by allowing him to assign
#      TCL_LIBRARY to any Tcl library, debug or not.
# - TK_INTERNAL_PATH was removed.
#   => this ended up being only a Win32 variable, and there is a lot of
#      confusion regarding the location of this file in an installed Tcl/Tk
#      tree anyway (see 8.5 for example). If you need the internal path at
#      this point it is safer you ask directly where the *source* tree is
#      and dig from there.

IF(TCL_FOUND)
  SET(TCL_FIND_QUIETLY TRUE)
ENDIF(TCL_FOUND)

INCLUDE(CMakeFindFrameworks)
INCLUDE(FindTclsh)

GET_FILENAME_COMPONENT(TCL_TCLSH_PATH "${TCL_TCLSH}" PATH)
GET_FILENAME_COMPONENT(TCL_TCLSH_PATH_PARENT "${TCL_TCLSH_PATH}" PATH)
STRING(REGEX REPLACE
  "^.*tclsh([0-9]\\.*[0-9]).*$" "\\1" TCL_TCLSH_VERSION "${TCL_TCLSH}")

GET_FILENAME_COMPONENT(TCL_INCLUDE_PATH_PARENT "${TCL_INCLUDE_PATH}" PATH)

GET_FILENAME_COMPONENT(TCL_LIBRARY_PATH "${TCL_LIBRARY}" PATH)
GET_FILENAME_COMPONENT(TCL_LIBRARY_PATH_PARENT "${TCL_LIBRARY_PATH}" PATH)
STRING(REGEX REPLACE
  "^.*tcl([0-9]\\.*[0-9]).*$" "\\1" TCL_VERSION "${TCL_LIBRARY}")

SET(TCL_POSSIBLE_LIB_PATHS
  "${TCL_INCLUDE_PATH_PARENT}/lib"
  "${TCL_INCLUDE_PATH_PARENT}/lib64"
  "${TCL_LIBRARY_PATH}"
  "${TCL_TCLSH_PATH_PARENT}/lib"
  "${TCL_TCLSH_PATH_PARENT}/lib64"
  /usr/lib
  /usr/lib64
  /usr/local/lib
  /usr/local/lib64
  )

IF(WIN32)
  GET_FILENAME_COMPONENT(
    ActiveTcl_CurrentVersion
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActiveTcl;CurrentVersion]"
    NAME)
  SET(TCLTK_POSSIBLE_LIB_PATHS ${TCLTK_POSSIBLE_LIB_PATHS}
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActiveTcl\\${ActiveTcl_CurrentVersion}]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.6;Root]/lib"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.5;Root]/lib"
    "$ENV{ProgramFiles}/Tcl/Lib"
    "C:/Program Files/Tcl/lib"
    "C:/Tcl/lib"
    )
ENDIF(WIN32)

FIND_LIBRARY(TCL_LIBRARY
  NAMES
  tcl86 tcl8.6
  tcl85 tcl8.5
  tcl
  tcl${TCL_VERSION} tcl${TCL_TCLSH_VERSION}
  PATHS ${TCL_POSSIBLE_LIB_PATHS}
  )

CMAKE_FIND_FRAMEWORKS(Tcl)

SET(TCL_FRAMEWORK_INCLUDES)
IF(Tcl_FRAMEWORKS)
  IF(NOT TCL_INCLUDE_PATH)
    FOREACH(dir ${Tcl_FRAMEWORKS})
      SET(TCL_FRAMEWORK_INCLUDES ${TCL_FRAMEWORK_INCLUDES} ${dir}/Headers)
    ENDFOREACH(dir)
  ENDIF(NOT TCL_INCLUDE_PATH)
ENDIF(Tcl_FRAMEWORKS)

SET(TCL_POSSIBLE_INCLUDE_PATHS
  "${TCL_LIBRARY_PATH_PARENT}/include"
  "${TCL_INCLUDE_PATH}"
  ${TCL_FRAMEWORK_INCLUDES}
  "${TCL_TCLSH_PATH_PARENT}/include"
  /usr/include/tcl8.6
  /usr/include/tcl8.5
  /usr/include
  /usr/local/include
  /usr/include/tcl${TCL_VERSION}
  /usr/local/include/tcl${TCL_VERSION}
  /usr/local/include/tcl8.6
  /usr/local/include/tcl8.5
  )


IF(WIN32)
  SET(TCLTK_POSSIBLE_INCLUDE_PATHS ${TCLTK_POSSIBLE_INCLUDE_PATHS}
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\ActiveState\\ActiveTcl\\${ActiveTcl_CurrentVersion}]/include"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.6;Root]/include"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Scriptics\\Tcl\\8.5;Root]/include"
    "$ENV{ProgramFiles}/Tcl/include"
    "C:/Program Files/Tcl/include"
    "C:/Tcl/include"
    )
ENDIF(WIN32)

FIND_PATH(TCL_INCLUDE_PATH
  NAMES tcl.h
  PATHS ${TCL_POSSIBLE_INCLUDE_PATHS}
  )

IF(TCL_LIBRARY AND TCL_INCLUDE_PATH)
  SET(TCL_VERSION ${TCL_VERSION})
  SET(TCL_LIBARY ${TCL_LIBRARY})
  SET(TCL_INCLUDE_PATH ${TCL_INCLUDE_PATH})
  SET(TCL_FOUND TRUE)
ENDIF(TCL_LIBRARY AND TCL_INCLUDE_PATH)

MARK_AS_ADVANCED(
  TCL_INCLUDE_PATH
  TCL_LIBRARY
  TCL_VERSION
  )
