# Copyright (c) 2003 FlashCode <flashcode@flashtux.org>
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

bin_PROGRAMS = weechat-curses

weechat_curses_LDADD = ../../common/lib_weechat_main.a \
                       ../../irc/lib_weechat_irc.a \
                       $(CURSES_LIBS) $(PLUGIN_LIBS)

weechat_curses_SOURCES = ../gui.h \
                         ../gui-common.c \
                         gui-display.c \
			             gui-input.c
