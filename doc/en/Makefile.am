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
             weechat.1.en.adoc \
             cmdline_options.en.adoc \
             weechat_user.en.adoc \
             weechat_plugin_api.en.adoc \
             weechat_scripting.en.adoc \
             weechat_faq.en.adoc \
             weechat_quickstart.en.adoc \
             weechat_tester.en.adoc \
             weechat_relay_protocol.en.adoc \
             weechat_dev.en.adoc \
             $(wildcard autogen/user/*.adoc) \
             $(wildcard autogen/plugin_api/*.adoc)

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
		weechat_relay_protocol.en.html \
		weechat_dev.en.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
weechat.1: weechat.1.en.adoc cmdline_options.en.adoc
	$(ASCIIDOCTOR) -a revision="WeeChat $(VERSION)" -b manpage -o weechat.1 $(abs_top_srcdir)/doc/en/weechat.1.en.adoc

# user's guide
weechat_user.en.html: weechat_user.en.adoc cmdline_options.en.adoc $(wildcard autogen/user/*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_user.en.html $(abs_top_srcdir)/doc/en/weechat_user.en.adoc

# plugin API reference
weechat_plugin_api.en.html: weechat_plugin_api.en.adoc $(wildcard autogen/plugin_api/*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_plugin_api.en.html $(abs_top_srcdir)/doc/en/weechat_plugin_api.en.adoc

# scripting guide
weechat_scripting.en.html: weechat_scripting.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_scripting.en.html $(abs_top_srcdir)/doc/en/weechat_scripting.en.adoc

# FAQ
weechat_faq.en.html: weechat_faq.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_faq.en.html $(abs_top_srcdir)/doc/en/weechat_faq.en.adoc

# quickstart
weechat_quickstart.en.html: weechat_quickstart.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_quickstart.en.html $(abs_top_srcdir)/doc/en/weechat_quickstart.en.adoc

# tester's guide
weechat_tester.en.html: weechat_tester.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_tester.en.html $(abs_top_srcdir)/doc/en/weechat_tester.en.adoc

# relay protocol
weechat_relay_protocol.en.html: weechat_relay_protocol.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_relay_protocol.en.html $(abs_top_srcdir)/doc/en/weechat_relay_protocol.en.adoc

# developer's guide
weechat_dev.en.html: weechat_dev.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_dev.en.html $(abs_top_srcdir)/doc/en/weechat_dev.en.adoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/man1/
	(cd '$(DESTDIR)$(mandir)/man1/' && $(RM) weechat-headless.1 && $(LN_S) weechat.1 weechat-headless.1)

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/man1/weechat.1
	$(RM) $(DESTDIR)$(mandir)/man1/weechat-headless.1
	-rmdir $(DESTDIR)$(mandir)/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.en.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-$(RM) weechat.1 weechat_*.html
