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

add_library(alias MODULE
  alias.c alias.h
  alias-command.c alias-command.h
  alias-completion.c alias-completion.h
  alias-config.c alias-config.h
  alias-info.c alias-info.h
)
set_target_properties(alias PROPERTIES PREFIX "")

target_link_libraries(alias coverage_config)

install(TARGETS alias LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
