#
# Copyright (C) 2021-2024 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(typing MODULE
  typing.c typing.h
  typing-bar-item.c typing-bar-item.h
  typing-config.c typing-config.h
  typing-status.c typing-status.h
)
set_target_properties(typing PROPERTIES PREFIX "")

target_link_libraries(typing coverage_config)

install(TARGETS typing LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
