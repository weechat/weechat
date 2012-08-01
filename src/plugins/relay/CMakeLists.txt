#
# Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
#

ADD_LIBRARY(relay MODULE
relay.c relay.h
relay-buffer.c relay-buffer.h
relay-client.c relay-client.h
irc/relay-irc.c irc/relay-irc.h
weechat/relay-weechat.c weechat/relay-weechat.h
weechat/relay-weechat-msg.c weechat/relay-weechat-msg.h
weechat/relay-weechat-protocol.c weechat/relay-weechat-protocol.h
relay-command.c relay-command.h
relay-completion.c relay-completion.h
relay-config.c relay-config.h
relay-info.c relay-info.h
relay-network.c relay-network.h
relay-raw.c relay-raw.h
relay-server.c relay-server.h
relay-upgrade.c relay-upgrade.h)
SET_TARGET_PROPERTIES(relay PROPERTIES PREFIX "")

SET (LINK_LIBS)

IF(ZLIB_FOUND)
  LIST(APPEND LINK_LIBS ${ZLIB_LIBRARY})
ENDIF(ZLIB_FOUND)

IF(GNUTLS_FOUND)
  INCLUDE_DIRECTORIES(${GNUTLS_INCLUDE_PATH})
  LIST(APPEND LINK_LIBS ${GNUTLS_LIBRARY})
ENDIF(GNUTLS_FOUND)

TARGET_LINK_LIBRARIES(relay ${LINK_LIBS})

INSTALL(TARGETS relay LIBRARY DESTINATION ${LIBDIR}/plugins)
