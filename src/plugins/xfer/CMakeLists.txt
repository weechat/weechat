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

add_library(xfer MODULE
  xfer.c xfer.h
  xfer-buffer.c xfer-buffer.h
  xfer-chat.c xfer-chat.h
  xfer-command.c xfer-command.h
  xfer-completion.c xfer-completion.h
  xfer-config.c xfer-config.h
  xfer-dcc.c xfer-dcc.h
  xfer-file.c xfer-file.h
  xfer-info.c xfer-info.h
  xfer-network.c xfer-network.h
  xfer-upgrade.c xfer-upgrade.h
)
set_target_properties(xfer PROPERTIES PREFIX "")

set(LINK_LIBS)

list(APPEND LINK_LIBS ${LIBGCRYPT_LDFLAGS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  # link with resolv lib on macOS
  list(APPEND LINK_LIBS "resolv")
endif()

target_link_libraries(xfer ${LINK_LIBS} coverage_config)

install(TARGETS xfer LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
