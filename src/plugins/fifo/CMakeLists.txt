#
# Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(fifo MODULE
  fifo.c fifo.h
  fifo-command.c fifo-command.h
  fifo-config.c fifo-config.h
  fifo-info.c fifo-info.h
)
set_target_properties(fifo PROPERTIES PREFIX "")

target_link_libraries(fifo coverage_config)

install(TARGETS fifo LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
