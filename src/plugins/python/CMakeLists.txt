#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

ADD_LIBRARY(python MODULE weechat-python.c weechat-python.h
weechat-python-api.c weechat-python-api.h)

SET_TARGET_PROPERTIES(python PROPERTIES PREFIX "")

IF(PYTHON_FOUND)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PYTHON_LFLAGS}")
  TARGET_LINK_LIBRARIES(python ${PYTHON_LIBRARY} weechat_plugins_scripts)
ENDIF(PYTHON_FOUND)

INSTALL(TARGETS python LIBRARY DESTINATION ${LIBDIR}/plugins)
