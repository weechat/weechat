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

CC=gcc

OPTIONS=-Wall -W -pipe -O2

OUTPUT=../gui.a
OBJS=gui-common.o

# WeeChat with Curses interface
ifeq ($(GUI), curses)
DEFINES=WEE_CURSES
curses: curses/gui.a $(OBJS)
curses/gui.a:
	cd curses && make
endif

# WeeChat with Gtk+ interface
ifeq ($(GUI), gtk)
INCLUDES=`pkg-config --cflags gtk+-2.0`
DEFINES=WEE_GTK
gtk: gtk/gui.a $(OBJS)
gtk/gui.a:
	cd gtk && make
endif

# WeeChat with Qt interface
ifeq ($(GUI), qt)
DEFINES=WEE_QT
qt: qt/gui.a $(OBJS)
qt/gui.a:
	cd qt && make
endif


all:
	make curses GUI=curses

$(OBJS):
	$(CC) $(OPTIONS) -o $@ -c $< $(INCLUDES) -D$(DEFINES)

clean:
	rm -f *.o *.a *~ core
	cd curses && make clean
	cd gtk && make clean
	cd qt && make clean

gui-common.o: gui-common.c ../weechat.h gui.h ../completion.h \
  ../history.h ../irc/irc.h ../gui/gui.h
