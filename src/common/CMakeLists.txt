# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
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

SET(LIB_MAIN_SRC weechat.c weechat.h alias.c alias.h backtrace.c backtrace.h
command.c command.h completion.c completion.h weelist.c weelist.h weeconfig.c
weeconfig.h history.c history.h hotlist.c hotlist.h log.c log.h fifo.c fifo.h
session.c session.h utf8.c utf8.h util.c util.h)

# Check for flock support
INCLUDE(CheckSymbolExists)
CHECK_INCLUDE_FILES("sys/file.h" HAVE_SYS_FILE_H)
CHECK_SYMBOL_EXISTS(flock "sys/file.h" HAVE_FLOCK)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})
ADD_LIBRARY(weechat_main STATIC ${LIB_MAIN_SRC})
