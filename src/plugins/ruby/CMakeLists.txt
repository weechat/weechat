#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(ruby MODULE weechat-ruby.c weechat-ruby.h weechat-ruby-api.c
weechat-ruby-api.h)

set_target_properties(ruby PROPERTIES PREFIX "")

if(RUBY_FOUND)
  include_directories(${RUBY_INCLUDE_DIRS})
  set(LINK_LIBS)
  list(APPEND LINK_LIBS ${RUBY_LDFLAGS})
  target_link_libraries(ruby ${LINK_LIBS} ${RUBY_LIB} weechat_plugins_scripts)
endif(RUBY_FOUND)

install(TARGETS ruby LIBRARY DESTINATION ${LIBDIR}/plugins)
