#
# Copyright (C) 2003-2014 Sébastien Helleu <flashcode@flashtux.org>
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

ADD_LIBRARY(ruby MODULE weechat-ruby.c weechat-ruby.h weechat-ruby-api.c
weechat-ruby-api.h)

SET_TARGET_PROPERTIES(ruby PROPERTIES PREFIX "")

IF(RUBY_FOUND)
  INCLUDE_DIRECTORIES(${RUBY_INCLUDE_DIRS})
  SET(LINK_LIBS)
  LIST(APPEND LINK_LIBS ${RUBY_LDFLAGS})
  TARGET_LINK_LIBRARIES(ruby ${LINK_LIBS} ${RUBY_LIB} weechat_plugins_scripts)
ENDIF(RUBY_FOUND)

INSTALL(TARGETS ruby LIBRARY DESTINATION ${LIBDIR}/plugins)
