#
# Copyright (C) 2015-2025 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

enable_language(CXX)

add_library(javascript MODULE
  weechat-js.cpp weechat-js.h
  weechat-js-v8.cpp weechat-js-v8.h
  weechat-js-api.cpp weechat-js-api.h
)
set_target_properties(javascript PROPERTIES PREFIX "")

if(V8_FOUND)
  include_directories(${V8_INCLUDE_DIR})
  target_link_libraries(javascript ${V8_LIBRARY} weechat_plugins_scripts coverage_config)
endif()

install(TARGETS javascript LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
