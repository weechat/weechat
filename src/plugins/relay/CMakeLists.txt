#
# Copyright (C) 2003-2010 Sebastien Helleu <flashcode@flashtux.org>
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
relay-command.c relay-command.h
relay-completion.c relay-completion.h
relay-config.c relay-config.h
relay-info.c relay-info.h
relay-protocol-irc.c relay-protocol-irc.h
relay-protocol-weechat.c relay-protocol-weechat.h
relay-server.c relay-server.h
relay-upgrade.c relay-upgrade.h)
SET_TARGET_PROPERTIES(relay PROPERTIES PREFIX "")

TARGET_LINK_LIBRARIES(relay)

INSTALL(TARGETS relay LIBRARY DESTINATION ${LIBDIR}/plugins)
