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
		weechat_user.fr.txt \
		weechat_faq.fr.txt \
		weechat_quickstart.fr.txt \
		weechat_tester.fr.txt \
		$(wildcard autogen/user/*.txt) \
		$(wildcard autogen/plugin_api/*.txt)

all-local: weechat_user.fr.html \
		weechat_faq.fr.html \
		weechat_quickstart.fr.html \
		weechat_tester.fr.html

# user's guide
weechat_user.fr.html: weechat_user.fr.txt $(wildcard autogen/user/*.txt)
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_user.fr.html weechat_user.fr.txt

# plugin API reference
weechat_plugin_api.fr.html: weechat_plugin_api.fr.txt $(wildcard autogen/plugin_api/*.txt)
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_plugin_api.fr.html weechat_plugin_api.fr.txt

# scripting guide
weechat_scripting.fr.html: weechat_scripting.fr.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_scripting.fr.html weechat_scripting.fr.txt

# FAQ
weechat_faq.fr.html: weechat_faq.fr.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_faq.fr.html weechat_faq.fr.txt

# quickstart
weechat_quickstart.fr.html: weechat_quickstart.fr.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_quickstart.fr.html weechat_quickstart.fr.txt

# tester's guide
weechat_tester.fr.html: weechat_tester.fr.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -n -o weechat_tester.fr.html weechat_tester.fr.txt

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# clean

clean-local:
	-rm -f *.html
