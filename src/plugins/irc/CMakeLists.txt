# Copyright (c) 2003-2008 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

SET(LIB_PROTOCOL_IRC_SRC irc.c irc.h irc-buffer.c irc-buffer.h irc-channel.c
irc-channel.h irc-color.c irc-color.h irc-command.c irc-command.h
irc-completion.c irc-completion.h irc-config.c irc-config.h irc-debug.c
irc-debug.h irc-display.c irc-display.h irc-input.c irc-mode.c irc-mode.h
irc-nick.c irc-nick.h irc-protocol.c irc-protocol.h irc-server.c irc-server.h)

CHECK_INCLUDE_FILES("regex.h" HAVE_REGEX_H)
CHECK_FUNCTION_EXISTS(regexec HAVE_REGEXEC)
CHECK_FUNCTION_EXISTS(uname HAVE_UNAME)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_protocol_irc STATIC ${LIB_PROTOCOL_IRC_SRC})
