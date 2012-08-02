#
# Copyright (C) 2008-2010 Dmitry Kobylin <fnfal@academ.tsc.ru>
# Copyright (C) 2008 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2008-2012 Sebastien Helleu <flashcode@flashtux.org>
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

ADD_LIBRARY(tcl MODULE weechat-tcl.c weechat-tcl.h
weechat-tcl-api.c weechat-tcl-api.h)

SET_TARGET_PROPERTIES(tcl PROPERTIES PREFIX "")

IF(TCL_FOUND)
  SET(CMAKE_REQUIRED_INCLUDES "${TCL_INCLUDE_PATH}")
  SET(CMAKE_REQUIRED_LIBRARIES "${TCL_LIBRARY}")

  INCLUDE_DIRECTORIES(${TCL_INCLUDE_PATH})
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${TCL_LFLAGS}")
  TARGET_LINK_LIBRARIES(tcl ${TCL_LIBRARY} weechat_plugins_scripts)

ENDIF(TCL_FOUND)

INSTALL(TARGETS tcl LIBRARY DESTINATION ${LIBDIR}/plugins)
