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

add_library(trigger MODULE
  trigger.c trigger.h
  trigger-buffer.c trigger-buffer.h
  trigger-callback.c trigger-callback.h
  trigger-command.c trigger-command.h
  trigger-completion.c trigger-completion.h
  trigger-config.c trigger-config.h
)
set_target_properties(trigger PROPERTIES PREFIX "")

target_link_libraries(trigger coverage_config)

install(TARGETS trigger LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
