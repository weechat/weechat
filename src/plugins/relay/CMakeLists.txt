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

set(RELAY_SRC
  relay.c relay.h
  relay-auth.c relay-auth.h
  relay-bar-item.c relay-bar-item.h
  relay-buffer.c relay-buffer.h
  relay-client.c relay-client.h
  relay-command.c relay-command.h
  relay-completion.c relay-completion.h
  relay-config.c relay-config.h
  relay-http.c relay-http.h
  relay-info.c relay-info.h
  relay-network.c relay-network.h
  relay-raw.c relay-raw.h
  relay-remote.c relay-remote.h
  relay-server.c relay-server.h
  relay-upgrade.c relay-upgrade.h
  relay-websocket.c relay-websocket.h
  # irc relay
  irc/relay-irc.c irc/relay-irc.h
  # weechat relay
  weechat/relay-weechat.c weechat/relay-weechat.h
  weechat/relay-weechat-msg.c weechat/relay-weechat-msg.h
  weechat/relay-weechat-nicklist.c weechat/relay-weechat-nicklist.h
  weechat/relay-weechat-protocol.c weechat/relay-weechat-protocol.h

)

if(ENABLE_CJSON)
  list(APPEND RELAY_SRC
    # API relay
    api/relay-api.c api/relay-api.h
    api/relay-api-msg.c api/relay-api-msg.h
    api/relay-api-protocol.c api/relay-api-protocol.h
    # API relay remote
    api/remote/relay-remote-event.c api/remote/relay-remote-event.h
    api/remote/relay-remote-network.c api/remote/relay-remote-network.h
  )
endif()

add_library(relay MODULE ${RELAY_SRC})
set_target_properties(relay PROPERTIES PREFIX "")

set(LINK_LIBS)

list(APPEND LINK_LIBS ${GNUTLS_LDFLAGS})

list(APPEND LINK_LIBS ${LIBGCRYPT_LDFLAGS})

list(APPEND LINK_LIBS ${ZLIB_LIBRARY})

if(ENABLE_ZSTD)
  include_directories(${LIBZSTD_INCLUDE_DIRS})
  list(APPEND LINK_LIBS ${LIBZSTD_LDFLAGS})
endif()

if(ENABLE_CJSON)
  include_directories(${LIBCJSON_INCLUDE_DIRS})
  list(APPEND LINK_LIBS ${LIBCJSON_LDFLAGS})
endif()

target_link_libraries(relay ${LINK_LIBS} coverage_config)

install(TARGETS relay LIBRARY DESTINATION "${WEECHAT_LIBDIR}/plugins")
