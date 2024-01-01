#
# Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

add_library(script MODULE
  script.c script.h
  script-action.c script-action.h
  script-buffer.c script-buffer.h
  script-command.c script-command.h
  script-completion.c script-completion.h
  script-config.c script-config.h
  script-info.c script-info.h
  script-mouse.c script-mouse.h
  script-repo.c script-repo.h
)
set_target_properties(script PROPERTIES PREFIX "")

set(LINK_LIBS)

list(APPEND LINK_LIBS ${ZLIB_LIBRARY})

target_link_libraries(script ${LINK_LIBS} coverage_config)

install(TARGETS script LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
