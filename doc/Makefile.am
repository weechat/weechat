#
# Copyright (C) 2003-2015 Sébastien Helleu <flashcode@flashtux.org>
# Copyright (C) 2006 Julien Louis <ptitlouis@sysif.net>
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

BUILT_SOURCES = build-asciidoc-css
build-asciidoc-css:
	cp $(abs_top_srcdir)/doc/asciidoc.css asciidoc_toc.css
	cat $(abs_top_srcdir)/doc/asciidoc.css $(abs_top_srcdir)/doc/toc2.css > asciidoc_toc2.css

SUBDIRS = . en fr it de pl es ru ja

EXTRA_DIST = docgen.py \
             CMakeLists.txt \
             asciidoc.conf \
             asciidoc.css \
             toc2.css

uninstall-hook:
	-rmdir $(DESTDIR)$(datadir)/doc/$(PACKAGE)

clean-local:
	-rm -f asciidoc_toc.css asciidoc_toc2.css
