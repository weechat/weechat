#
# Copyright (C) 2016 Sébastien Helleu <flashcode@flashtux.org>
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

# - Find Socketpair
# This module finds if socketpair function is available and supports SOCK_DGRAM.
#

if(SOCKETPAIR_FOUND)
   # Already in cache, be silent
   set(SOCKETPAIR_FIND_QUIETLY TRUE)
endif()

include(CheckCSourceRuns)

set(CMAKE_REQUIRED_FLAGS -Werror)
check_c_source_runs("
  #include <sys/types.h>
  #include <sys/socket.h>
  int main() {
        int socket[2];
        int rc = socketpair (AF_LOCAL, SOCK_DGRAM, 0, socket);
        return (rc == 0) ? 0 : 1;
  }
" HAVE_SOCKETPAIR_SOCK_DGRAM)
