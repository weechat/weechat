#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

docdir = $(datadir)/doc/$(PACKAGE)

EXTRA_DIST = CMakeLists.txt \
             weechat_user.it.txt \
             weechat_plugin_api.it.txt \
             weechat_scripting.it.txt \
             weechat_faq.it.txt \
             weechat_quickstart.it.txt \
             weechat_tester.it.txt \
             $(wildcard autogen/user/*.txt) \
             $(wildcard autogen/plugin_api/*.txt)

all-local: weechat_user.it.html \
		weechat_plugin_api.it.html \
		weechat_scripting.it.html \
		weechat_faq.it.html \
		weechat_quickstart.it.html \
		weechat_tester.it.html

# user's guide
weechat_user.it.html: weechat_user.it.txt $(wildcard autogen/user/*.txt)
	$(ASCIIDOC) -a lang=it -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_user.it.html $(abs_top_srcdir)/doc/it/weechat_user.it.txt

# plugin API reference
weechat_plugin_api.it.html: weechat_plugin_api.it.txt $(wildcard autogen/plugin_api/*.txt)
	$(ASCIIDOC) -a lang=it -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_plugin_api.it.html $(abs_top_srcdir)/doc/it/weechat_plugin_api.it.txt

# scripting guide
weechat_scripting.it.html: weechat_scripting.it.txt
	$(ASCIIDOC) -a lang=it -a toc -a toclevels=3 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_scripting.it.html $(abs_top_srcdir)/doc/it/weechat_scripting.it.txt

# FAQ
weechat_faq.it.html: weechat_faq.it.txt
	$(ASCIIDOC) -a lang=it -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_faq.it.html $(abs_top_srcdir)/doc/it/weechat_faq.it.txt

# quickstart
weechat_quickstart.it.html: weechat_quickstart.it.txt
	$(ASCIIDOC) -a lang=it -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_quickstart.it.html $(abs_top_srcdir)/doc/it/weechat_quickstart.it.txt

# tester's guide
weechat_tester.it.html: weechat_tester.it.txt
	$(ASCIIDOC) -a lang=it -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_tester.it.html $(abs_top_srcdir)/doc/it/weechat_tester.it.txt

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall docs

uninstall-hook:
	$(RM) $(DESTDIR)$(docdir)/*.it.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f *.html
