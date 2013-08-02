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
             weechat.1.it.txt \
             cmdline_options.it.txt \
             weechat_user.it.txt \
             weechat_plugin_api.it.txt \
             weechat_scripting.it.txt \
             weechat_faq.it.txt \
             weechat_quickstart.it.txt \
             weechat_tester.it.txt \
             $(wildcard autogen/user/*.txt) \
             $(wildcard autogen/plugin_api/*.txt)

if MAN
  man_targets = weechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.it.html \
		weechat_plugin_api.it.html \
		weechat_scripting.it.html \
		weechat_faq.it.html \
		weechat_quickstart.it.html \
		weechat_tester.it.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
weechat.1: weechat.1.it.txt cmdline_options.it.txt
	$(A2X) -a lang=it -a revision="WeeChat $(VERSION)" -d manpage -f manpage -L -D . $(abs_top_srcdir)/doc/it/weechat.1.it.txt

# user's guide
weechat_user.it.html: weechat_user.it.txt cmdline_options.it.txt $(wildcard autogen/user/*.txt)
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

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/it/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/it/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/it/man1/weechat.1
	-rmdir $(DESTDIR)$(mandir)/it/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.it.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f weechat.1
	-rm -f *.html
