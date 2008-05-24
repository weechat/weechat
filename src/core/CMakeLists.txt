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

SET(LIB_CORE_SRC
weechat.c weechat.h
wee-backtrace.c wee-backtrace.h
wee-command.c wee-command.h
wee-config.c wee-config.h
wee-config-file.c wee-config-file.h
wee-debug.c wee-debug.h
wee-hook.c wee-hook.h
wee-input.c wee-input.h
wee-list.c wee-list.h
wee-log.c wee-log.h
wee-network.c wee-network.h
wee-string.c wee-string.h
wee-upgrade.c wee-upgrade.h
wee-utf8.c wee-utf8.h
wee-util.c wee-util.h)

# Check for flock support
INCLUDE(CheckSymbolExists)
CHECK_INCLUDE_FILES("sys/file.h" HAVE_SYS_FILE_H)
CHECK_SYMBOL_EXISTS(flock "sys/file.h" HAVE_FLOCK)

IF(GNUTLS_FOUND)
  INCLUDE_DIRECTORIES(${GNUTLS_INCLUDE_PATH})
ENDIF(GNUTLS_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_core STATIC ${LIB_CORE_SRC})
