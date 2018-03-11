#
# Copyright (C) 2003-2018 Sébastien Helleu <flashcode@flashtux.org>
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
             docinfo.html \
             weechat.1.de.adoc \
             cmdline_options.de.adoc \
             weechat_user.de.adoc \
             weechat_scripting.de.adoc \
             weechat_faq.de.adoc \
             weechat_quickstart.de.adoc \
             weechat_tester.de.adoc \
             $(wildcard autogen/user/*.adoc)

if MAN
  man_targets = weechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.de.html \
		weechat_scripting.de.html \
		weechat_faq.de.html \
		weechat_quickstart.de.html \
		weechat_tester.de.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
weechat.1: weechat.1.de.adoc cmdline_options.de.adoc
	$(ASCIIDOCTOR) -a revision="WeeChat $(VERSION)" -b manpage -o weechat.1 $(abs_top_srcdir)/doc/de/weechat.1.de.adoc

# user's guide
weechat_user.de.html: weechat_user.de.adoc cmdline_options.de.adoc $(wildcard autogen/user/*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_user.de.html $(abs_top_srcdir)/doc/de/weechat_user.de.adoc

# scripting guide
weechat_scripting.de.html: weechat_scripting.de.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_scripting.de.html $(abs_top_srcdir)/doc/de/weechat_scripting.de.adoc

# FAQ
weechat_faq.de.html: weechat_faq.de.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_faq.de.html $(abs_top_srcdir)/doc/de/weechat_faq.de.adoc

# quickstart
weechat_quickstart.de.html: weechat_quickstart.de.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_quickstart.de.html $(abs_top_srcdir)/doc/de/weechat_quickstart.de.adoc

# tester's guide
weechat_tester.de.html: weechat_tester.de.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_tester.de.html $(abs_top_srcdir)/doc/de/weechat_tester.de.adoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/de/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/de/man1/
	(cd '$(DESTDIR)$(mandir)/de/man1/' && $(RM) weechat-headless.1 && $(LN_S) weechat.1 weechat-headless.1)

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/de/man1/weechat.1
	$(RM) $(DESTDIR)$(mandir)/de/man1/weechat-headless.1
	-rmdir $(DESTDIR)$(mandir)/de/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.de.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f weechat.1 weechat_*.html
