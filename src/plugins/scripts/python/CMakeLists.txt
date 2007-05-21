# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

ADD_LIBRARY(python MODULE weechat-python.c)

SET_TARGET_PROPERTIES(python PROPERTIES PREFIX "")

IF(PYTHON_FOUND)
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PYTHON_LFLAGS}")
  TARGET_LINK_LIBRARIES(python ${PYTHON_LIBRARY} weechat_scripts)
ENDIF(PYTHON_FOUND)

INSTALL(TARGETS python LIBRARY DESTINATION lib/weechat/plugins)
