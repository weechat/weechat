#
# Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2005 Julien Louis <ptitlouis@sysif.net>
# Copyright (C) 2006-2009 Emmanuel Bouthenot <kolter@openics.org>
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

# This target will update file config-git.h with output of command "git describe"
# (if needed, and only for a devel/rc version).
BUILT_SOURCES = build-config-git.h
build-config-git.h:
	-$(abs_top_srcdir)/git-version.sh "$(abs_top_srcdir)" "$(VERSION)" config-git.h

if DOC
doc_dir = doc
endif

SUBDIRS = po $(doc_dir) intl src

EXTRA_DIST = CMakeLists.txt \
             config.rpath \
             config.h.cmake \
             git-version.sh \
             makedist.sh \
             cmake/CMakeParseArguments.cmake \
             cmake/FindAsciidoc.cmake \
             cmake/FindAspell.cmake \
             cmake/FindGCRYPT.cmake \
             cmake/FindGettext.cmake \
             cmake/FindGnuTLS.cmake \
             cmake/FindGuile.cmake \
             cmake/FindIconv.cmake \
             cmake/FindLua.cmake \
             cmake/FindNcurses.cmake \
             cmake/FindPackageHandleStandardArgs.cmake \
             cmake/FindPerl.cmake \
             cmake/FindPkgConfig.cmake \
             cmake/FindPython.cmake \
             cmake/FindRuby.cmake \
             cmake/FindSourcehighlight.cmake \
             cmake/FindTCL.cmake \
             cmake/FindZLIB.cmake \
             cmake/cmake_uninstall.cmake.in \
             po/CMakeLists.txt \
             po/srcfiles.cmake \
             weechat.spec \
             weechat_icon_32.png \
             weechat.pc.in

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = weechat.pc
