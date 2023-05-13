#
# Copyright (C) 2014-2023 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(exec MODULE
  exec.c exec.h
  exec-buffer.c exec-buffer.h
  exec-command.c exec-command.h
  exec-completion.c exec-completion.h
  exec-config.c exec-config.h
)
set_target_properties(exec PROPERTIES PREFIX "")

target_link_libraries(exec coverage_config)

install(TARGETS exec LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
