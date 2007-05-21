# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

SET(LIB_IRC_SRC irc.h irc-commands.c irc-send.c irc-recv.c irc-server.c
irc-channel.c irc-nick.c irc-mode.c irc-dcc.c irc-ignore.c irc-display.c)

CHECK_INCLUDE_FILES("regex.h" HAVE_REGEX_H)
CHECK_FUNCTION_EXISTS(regexec HAVE_REGEXEC)
CHECK_FUNCTION_EXISTS(uname HAVE_UNAME)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_irc STATIC ${LIB_IRC_SRC})
