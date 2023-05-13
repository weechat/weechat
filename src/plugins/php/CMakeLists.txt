#
# Copyright (C) 2006-2017 Adam Saponara <as@php.net>
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

add_library(php MODULE
  weechat-php.c weechat-php.h
  weechat-php-api.c weechat-php-api.h
)
set_target_properties(php PROPERTIES PREFIX "")

if(PHP_FOUND)
  include_directories(${PHP_INCLUDE_DIRS})
  set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PHP_LDFLAGS}")
  target_link_libraries(php ${PHP_LIB} weechat_plugins_scripts coverage_config)
endif()

install(TARGETS php LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
