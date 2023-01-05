#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

# This target will update file config-git.h with output of command "git describe"
# (if needed, and only for a devel/rc version).
BUILT_SOURCES = build-config-git.h
build-config-git.h:
	-$(abs_top_srcdir)/tools/set_git_version.sh "$(abs_top_srcdir)" "$(VERSION)" config-git.h

if TESTS
tests_dir = tests
endif

SUBDIRS = icons po doc intl src $(tests_dir)

EXTRA_DIST = AUTHORS.adoc \
             ChangeLog.adoc \
             Contributing.adoc \
             README.adoc \
             ReleaseNotes.adoc \
             CMakeLists.txt \
             config.rpath \
             config.h.cmake \
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
             cmake/FindPerl.cmake \
             cmake/FindPkgConfig.cmake \
             cmake/FindPython.cmake \
             cmake/FindRuby.cmake \
             cmake/FindV8.cmake \
             cmake/FindZLIB.cmake \
             cmake/cmake_uninstall.cmake.in \
             po/CMakeLists.txt \
             po/srcfiles.cmake \
             tools/build_test.sh \
             tools/set_git_version.sh \
             tools/makedist.sh \
             version.sh \
             weechat.desktop \
             weechat.pc.in \
             weechat.cygport.in

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = weechat.pc

desktopdir = $(datadir)/applications
desktop_DATA = weechat.desktop

clean-local:
	$(RM) config-git.h
