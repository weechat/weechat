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

OUTPUT=weechat

OBJS=weechat.o config.o command.o completion.o history.o
OBJS_IRC=irc/irc.a
OBJS_GUI=gui/gui.a


# WeeChat with Curses interface
ifeq ($(GUI), curses)
LIBS_CURSES=-lcurses
DEFINES=WEE_CURSES

curses: $(OBJS) $(OBJS_IRC) $(OBJS_GUI)
	$(CC) $(OPTIONS) $(OBJS) $(OBJS_IRC) $(OBJS_GUI) -o $(OUTPUT) $(LIBS_CURSES)
endif

# WeeChat with Gtk+ interface
ifeq ($(GUI), gtk)
OBJS_GTK=gui-gtk.o
LIBS_GTK=
DEFINES=WEE_GTK
gtk: $(OBJS) $(OBJS_IRC) $(OBJS_GUI)
	$(CC) $(OPTIONS) $(OBJS) $(OBJS_IRC) $(OBJS_GUI) -o $(OUTPUT) $(LIBS_GTK)
endif

# WeeChat with Qt interface
ifeq ($(GUI), qt)
OBJS_QT=gui-qt.o
LIBS_QT=
DEFINES=WEE_QT
qt: $(OBJS) $(OBJS_IRC) $(OBJS_GUI)
	$(CC) $(OPTIONS) $(OBJS) $(OBJS_IRC) $(OBJS_GUI) -o $(OUTPUT) $(LIBS_QT)
endif

# WeeChat with Text interface
ifeq ($(GUI), text)
OBJS_TEXT=gui-text.o
LIBS_TEXT=
DEFINES=WEE_TEXT
text: $(OBJS) $(OBJS_IRC) $(OBJS_GUI)
	$(CC) $(OPTIONS) $(OBJS) $(OBJS_IRC) $(OBJS_GUI) -o $(OUTPUT) $(LIBS_TEXT)
endif


all:
	make curses GUI=curses

$(OBJS):
	$(CC) $(OPTIONS) -o $@ -c $< $(INCLUDES) -D$(DEFINES)

irc/irc.a:
	cd irc && make

gui/gui.a:
	cd gui && make $(GUI) GUI=$(GUI)

clean:
	rm -f *.o *.a *~ core $(OUTPUT)
	cd irc && make clean
	cd gui && make clean

command.o: command.c weechat.h command.h irc/irc.h gui/gui.h completion.h \
  history.h config.h
completion.o: completion.c weechat.h completion.h irc/irc.h gui/gui.h \
  history.h command.h
config.o: config.c weechat.h config.h irc/irc.h gui/gui.h completion.h \
  history.h
history.o: history.c weechat.h history.h gui/gui.h completion.h
weechat.o: weechat.c weechat.h config.h command.h irc/irc.h gui/gui.h \
  completion.h history.h
