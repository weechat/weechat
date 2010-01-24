# Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
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
             weechat_user.it.txt \
             weechat_faq.it.txt \
             weechat_quickstart.it.txt \
             weechat_tester.it.txt \
             $(wildcard autogen/user/*.txt) \
             $(wildcard autogen/plugin_api/*.txt)

all-local: weechat_user.it.html \
		weechat_faq.it.html \
		weechat_quickstart.it.html \
		weechat_tester.it.html

# user's guide
weechat_user.it.html: weechat_user.it.txt $(wildcard autogen/user/*.txt)
	$(ASCIIDOC) -a toc -a toclevels=3 -a toc-title='Indice' -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_user.it.html weechat_user.it.txt

# FAQ
weechat_faq.it.html: weechat_faq.it.txt
	$(ASCIIDOC) -a toc -a toc-title='Indice' -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_faq.it.html weechat_faq.it.txt

# quickstart
weechat_quickstart.it.html: weechat_quickstart.it.txt
	$(ASCIIDOC) -a toc -a toc_title='Indice' -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_quickstart.it.html weechat_quickstart.it.txt

# tester's guide
weechat_tester.it.html: weechat_tester.it.txt
	$(ASCIIDOC) -a toc -a toc_title='Indice' -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_tester.it.html weechat_tester.it.txt

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# clean

clean-local:
	-rm -f *.html
