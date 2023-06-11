#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(ruby MODULE
  weechat-ruby.c weechat-ruby.h
  weechat-ruby-api.c weechat-ruby-api.h
)
set_target_properties(ruby PROPERTIES PREFIX "")

if(RUBY_FOUND)
  # temporary fix: ignore all warnings on unused parameters due to warnings
  # caused by Ruby headers (with Ruby ≥ 3.1.0)
  # see: https://github.com/ruby/ruby/pull/7085
  add_definitions(-Wno-unused-parameter)
  include_directories(${RUBY_INCLUDE_DIRS})
  target_link_libraries(ruby ${RUBY_LDFLAGS} weechat_plugins_scripts coverage_config)
endif(RUBY_FOUND)

install(TARGETS ruby LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
