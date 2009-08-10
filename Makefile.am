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
             cmake/cmake_uninstall.cmake.in \
             cmake/FindAsciidoc.cmake \
             cmake/FindAspell.cmake \
             cmake/FindGettext.cmake \
             cmake/FindGnuTLS.cmake \
             cmake/FindIconv.cmake \
             cmake/FindLua.cmake \
             cmake/FindNcurses.cmake \
             cmake/FindPerl.cmake \
             cmake/FindPkgConfig.cmake \
             cmake/FindPython.cmake \
             cmake/FindRuby.cmake \
             cmake/FindSourcehighlight.cmake \
             cmake/FindTCL.cmake \
             cmake/makedist.sh.in \
             po/CMakeLists.txt \
             po/srcfiles.cmake \
             weechat.spec \
             weechat_icon_32.png \
             weechat.pc.in \
             UPGRADE_0.3

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = weechat.pc
