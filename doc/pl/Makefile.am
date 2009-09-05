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
		weechat_faq.pl.txt \
		weechat_quickstart.pl.txt \
		weechat_tester.pl.txt

all-local: weechat_faq.pl.html \
		weechat_quickstart.pl.html \
		weechat_tester.pl.html

# FAQ
weechat_faq.pl.html: weechat_faq.pl.txt
	$(ASCIIDOC) -a toc -a toc_title='Spis treści' -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_faq.pl.html weechat_faq.pl.txt

# quickstart
weechat_quickstart.pl.html: weechat_quickstart.pl.txt
	$(ASCIIDOC) -a toc -a toc_title='Spis treści' -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_quickstart.pl.html weechat_quickstart.pl.txt

# tester's guide
weechat_tester.pl.html: weechat_tester.pl.txt
	$(ASCIIDOC) -a toc -a toc_title='Spis treści' -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_tester.pl.html weechat_tester.pl.txt

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# clean

clean-local:
	-rm -f *.html
