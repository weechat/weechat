#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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
	-$(abs_top_srcdir)/tools/git-version.sh "$(abs_top_srcdir)" "$(VERSION)" config-git.h

if TESTS
tests_dir = tests
endif

SUBDIRS = po doc intl src $(tests_dir)

EXTRA_DIST = AUTHORS.adoc \
             ChangeLog.adoc \
             Contributing.adoc \
             README.adoc \
             ReleaseNotes.adoc \
             CMakeLists.txt \
             config.rpath \
             config.h.cmake \
             makedist.sh \
             cmake/CMakeParseArguments.cmake \
             cmake/FindAsciidoctor.cmake \
             cmake/FindAspell.cmake \
             cmake/FindCppUTest.cmake \
             cmake/FindENCHANT.cmake \
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
             cmake/FindSocketpair.cmake \
             cmake/FindTCL.cmake \
             cmake/FindV8.cmake \
             cmake/FindZLIB.cmake \
             cmake/cmake_uninstall.cmake.in \
             po/CMakeLists.txt \
             po/srcfiles.cmake \
             tools/build-test.sh \
             tools/git-version.sh \
             version.sh \
             weechat.png \
             weechat.pc.in \
             weechat.cygport.in

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = weechat.pc

icondir   = $(datadir)/icons/hicolor/32x32/apps
icon_DATA = weechat.png

clean-local:
	$(RM) config-git.h
