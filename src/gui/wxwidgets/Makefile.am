#
# Copyright (C) 2005 Benoit Papillault <benoit.papillault@free.fr>
# Copyright (C) 2005-2011 Sebastien Helleu <flashcode@flashtux.org>
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

INCLUDES = -DLOCALEDIR=\"$(datadir)/locale\"

bin_PROGRAMS = weechat-wxwidgets

weechat_wxwidgets_LDADD = ../lib_weechat_gui_common.a \
                       ../../common/lib_weechat_main.a \
                       ../../irc/lib_weechat_irc.a \
                       ../../plugins/lib_weechat_plugins.a $(PLUGINS_LIBS)

weechat_wxwidgets_SOURCES = gui-display.c \
                            gui-input.c
