# Copyright (c) 2003-2009 FlashCode <flashcode@flashtux.org>
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

EXTRA_DIST = CMakeLists.txt \
             config.rpath \
             config.h.cmake \
             debian/NEWS \
             debian/changelog \
             debian/compat \
             debian/control \
             debian/copyright \
             debian/rules \
             debian/watch \
             debian/weechat-core.docs \
             debian/weechat-core.install \
             debian/weechat-curses.dirs \
             debian/weechat-curses.install \
             debian/weechat-curses.lintian-overrides \
             debian/weechat-curses.manpages \
             debian/weechat-curses.menu \
             debian/weechat-dev.dirs \
             debian/weechat-dev.install \
             debian/weechat-doc.docs \
             debian/weechat-doc.install \
             debian/weechat-plugins.dirs \
             debian/weechat-plugins.install \
             debian/weechat.xpm \
             cmake/cmake_uninstall.cmake.in \
             cmake/FindAspell.cmake \
             cmake/FindGettext.cmake \
             cmake/FindGnuTLS.cmake \
             cmake/FindIconv.cmake \
             cmake/FindIksemel.cmake \
             cmake/FindLua.cmake \
             cmake/FindNcurses.cmake \
             cmake/FindPerl.cmake \
             cmake/FindPkgConfig.cmake \
             cmake/FindPython.cmake \
             cmake/FindRuby.cmake \
             cmake/FindTCL.cmake \
             cmake/makedist.sh.in \
             weechat.spec \
             weechat_icon_32.png \
             weechat.pc.in

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = weechat.pc
