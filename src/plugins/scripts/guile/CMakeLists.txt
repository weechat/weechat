#
# Copyright (C) 2011-2012 Sebastien Helleu <flashcode@flashtux.org>
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

ADD_LIBRARY(guile MODULE weechat-guile.c weechat-guile.h
weechat-guile-api.c weechat-guile-api.h)

SET_TARGET_PROPERTIES(guile PROPERTIES PREFIX "")

IF(GUILE_FOUND)
  INCLUDE_DIRECTORIES(${GUILE_INCLUDE_DIRS})
  SET(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${GUILE_LFLAGS}")
  TARGET_LINK_LIBRARIES(guile ${GUILE_LIBRARY} weechat_scripts)
ENDIF(GUILE_FOUND)

INSTALL(TARGETS guile LIBRARY DESTINATION ${LIBDIR}/plugins)
