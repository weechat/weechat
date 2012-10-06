#
# Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
# Copyright (C) 2006-2012 Sebastien Helleu <flashcode@flashtux.org>
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

ADD_LIBRARY(aspell MODULE
weechat-aspell.c weechat-aspell.h
weechat-aspell-bar-item.c weechat-aspell-bar-item.h
weechat-aspell-config.c weechat-aspell-config.h
weechat-aspell-speller.c weechat-aspell-speller.h)
SET_TARGET_PROPERTIES(aspell PROPERTIES PREFIX "")

IF(ASPELL_FOUND)
  INCLUDE_DIRECTORIES(${ASPELL_INCLUDE_PATH})
  TARGET_LINK_LIBRARIES(aspell ${ASPELL_LIBRARY})
ENDIF(ASPELL_FOUND)

INSTALL(TARGETS aspell LIBRARY DESTINATION ${LIBDIR}/plugins)
