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

docdir = $(datadir)/doc/$(PACKAGE)

EXTRA_DIST = CMakeLists.txt \
		weechat_quickstart.de.txt \
		weechat_tester.de.txt

all-local: weechat_quickstart.de.html \
		weechat_tester.de.html

# quickstart
weechat_quickstart.de.html: weechat_quickstart.de.txt
	$(ASCIIDOC) -a toc -a toc_title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_quickstart.de.html weechat_quickstart.de.txt

# tester's guide
weechat_tester.de.html: weechat_tester.de.txt
	$(ASCIIDOC) -a toc -a toc_title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_tester.de.html weechat_tester.de.txt

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# clean

clean-local:
	-rm -f *.html
