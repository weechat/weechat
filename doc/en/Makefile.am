#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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

docdir = $(datadir)/doc/$(PACKAGE)

EXTRA_DIST = CMakeLists.txt \
             weechat.1.en.txt \
             cmdline_options.en.txt \
             weechat_user.en.txt \
             weechat_plugin_api.en.txt \
             weechat_scripting.en.txt \
             weechat_faq.en.txt \
             weechat_quickstart.en.txt \
             weechat_tester.en.txt \
             weechat_dev.en.txt \
             weechat_relay_protocol.en.txt \
             $(wildcard autogen/user/*.txt) \
             $(wildcard autogen/plugin_api/*.txt)

if MAN
  man_targets = weechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.en.html \
		weechat_plugin_api.en.html \
		weechat_scripting.en.html \
		weechat_faq.en.html \
		weechat_quickstart.en.html \
		weechat_tester.en.html \
		weechat_dev.en.html \
		weechat_relay_protocol.en.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
weechat.1: weechat.1.en.txt cmdline_options.en.txt
	$(A2X) -a revision="WeeChat $(VERSION)" -d manpage -f manpage -L -D . $(abs_top_srcdir)/doc/en/weechat.1.en.txt

# user's guide
weechat_user.en.html: weechat_user.en.txt cmdline_options.en.txt $(wildcard autogen/user/*.txt)
	$(ASCIIDOC) -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_user.en.html $(abs_top_srcdir)/doc/en/weechat_user.en.txt

# plugin API reference
weechat_plugin_api.en.html: weechat_plugin_api.en.txt $(wildcard autogen/plugin_api/*.txt)
	$(ASCIIDOC) -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_plugin_api.en.html $(abs_top_srcdir)/doc/en/weechat_plugin_api.en.txt

# scripting guide
weechat_scripting.en.html: weechat_scripting.en.txt
	$(ASCIIDOC) -a toc -a toclevels=3 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_scripting.en.html $(abs_top_srcdir)/doc/en/weechat_scripting.en.txt

# FAQ
weechat_faq.en.html: weechat_faq.en.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_faq.en.html $(abs_top_srcdir)/doc/en/weechat_faq.en.txt

# quickstart
weechat_quickstart.en.html: weechat_quickstart.en.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_quickstart.en.html $(abs_top_srcdir)/doc/en/weechat_quickstart.en.txt

# tester's guide
weechat_tester.en.html: weechat_tester.en.txt
	$(ASCIIDOC) -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_tester.en.html $(abs_top_srcdir)/doc/en/weechat_tester.en.txt

# developer's guide
weechat_dev.en.html: weechat_dev.en.txt
	$(ASCIIDOC) -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_dev.en.html $(abs_top_srcdir)/doc/en/weechat_dev.en.txt

# relay protocol
weechat_relay_protocol.en.html: weechat_relay_protocol.en.txt
	$(ASCIIDOC) -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_relay_protocol.en.html $(abs_top_srcdir)/doc/en/weechat_relay_protocol.en.txt

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/man1/weechat.1
	-rmdir $(DESTDIR)$(mandir)/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.en.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f weechat.1
	-rm -f *.html
