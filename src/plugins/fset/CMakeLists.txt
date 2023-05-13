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

add_library(fset MODULE
  fset.c fset.h
  fset-bar-item.c fset-bar-item.h
  fset-buffer.c fset-buffer.h
  fset-command.c fset-command.h
  fset-completion.c fset-completion.h
  fset-config.c fset-config.h
  fset-info.c fset-info.h
  fset-mouse.c fset-mouse.h
  fset-option.c fset-option.h
)
set_target_properties(fset PROPERTIES PREFIX "")

target_link_libraries(fset coverage_config)

install(TARGETS fset LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
