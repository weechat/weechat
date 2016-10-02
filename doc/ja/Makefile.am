#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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
             weechat.1.ja.adoc \
             cmdline_options.ja.adoc \
             weechat_user.ja.adoc \
             weechat_plugin_api.ja.adoc \
             weechat_scripting.ja.adoc \
             weechat_faq.ja.adoc \
             weechat_quickstart.ja.adoc \
             weechat_tester.ja.adoc \
             weechat_relay_protocol.ja.adoc \
             weechat_dev.ja.adoc \
             $(wildcard autogen/user/*.adoc) \
             $(wildcard autogen/plugin_api/*.adoc)

if MAN
  man_targets = weechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.ja.html \
		weechat_plugin_api.ja.html \
		weechat_scripting.ja.html \
		weechat_faq.ja.html \
		weechat_quickstart.ja.html \
		weechat_tester.ja.html \
		weechat_relay_protocol.ja.html \
		weechat_dev.ja.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
weechat.1: weechat.1.ja.adoc cmdline_options.ja.adoc
	$(ASCIIDOCTOR) -a revision="WeeChat $(VERSION)" -b manpage -o weechat.1 $(abs_top_srcdir)/doc/ja/weechat.1.ja.adoc

# user's guide
weechat_user.ja.html: weechat_user.ja.adoc cmdline_options.ja.adoc $(wildcard autogen/user/*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_user.ja.html $(abs_top_srcdir)/doc/ja/weechat_user.ja.adoc

# plugin API reference
weechat_plugin_api.ja.html: weechat_plugin_api.ja.adoc $(wildcard autogen/plugin_api/*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_plugin_api.ja.html $(abs_top_srcdir)/doc/ja/weechat_plugin_api.ja.adoc

# scripting guide
weechat_scripting.ja.html: weechat_scripting.ja.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_scripting.ja.html $(abs_top_srcdir)/doc/ja/weechat_scripting.ja.adoc

# FAQ
weechat_faq.ja.html: weechat_faq.ja.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_faq.ja.html $(abs_top_srcdir)/doc/ja/weechat_faq.ja.adoc

# quickstart
weechat_quickstart.ja.html: weechat_quickstart.ja.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_quickstart.ja.html $(abs_top_srcdir)/doc/ja/weechat_quickstart.ja.adoc

# tester's guide
weechat_tester.ja.html: weechat_tester.ja.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_tester.ja.html $(abs_top_srcdir)/doc/ja/weechat_tester.ja.adoc

# relay protocol
weechat_relay_protocol.ja.html: weechat_relay_protocol.ja.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_relay_protocol.ja.html $(abs_top_srcdir)/doc/ja/weechat_relay_protocol.ja.adoc

# developer's guide
weechat_dev.ja.html: weechat_dev.ja.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_dev.ja.html $(abs_top_srcdir)/doc/ja/weechat_dev.ja.adoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/ja/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/ja/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/ja/man1/weechat.1
	-rmdir $(DESTDIR)$(mandir)/ja/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.ja.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f weechat.1 weechat_*.html
