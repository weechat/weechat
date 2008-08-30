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

if DOC
doc_dir = doc
endif

SUBDIRS = po $(doc_dir) intl src

EXTRA_DIST = config.rpath BUGS FAQ FAQ.fr \
             debian/changelog debian/control \
             debian/copyright debian/compat \
             debian/rules debian/weechat-common.docs \
             debian/lintian-override \
             debian/weechat-common.install debian/weechat-curses.dirs \
             debian/weechat-curses.install debian/weechat-plugins.install \
             debian/weechat-curses.menu debian/NEWS debian/weechat.xpm \
             weechat.spec weechat_icon_32.png

ACLOCAL_AMFLAGS = -I m4
