#
# Copyright (C) 2003-2016 Sébastien Helleu <flashcode@flashtux.org>
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
             weechat.1.en.asciidoc \
             cmdline_options.en.asciidoc \
             weechat_user.en.asciidoc \
             weechat_plugin_api.en.asciidoc \
             weechat_scripting.en.asciidoc \
             weechat_faq.en.asciidoc \
             weechat_quickstart.en.asciidoc \
             weechat_tester.en.asciidoc \
             weechat_relay_protocol.en.asciidoc \
             weechat_dev.en.asciidoc \
             $(wildcard autogen/user/*.asciidoc) \
             $(wildcard autogen/plugin_api/*.asciidoc)

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
weechat.1: weechat.1.en.asciidoc cmdline_options.en.asciidoc
	$(ASCIIDOCTOR) -a revision="WeeChat $(VERSION)" -b manpage -o weechat.1 $(abs_top_srcdir)/doc/en/weechat.1.en.asciidoc

# user's guide
weechat_user.en.html: weechat_user.en.asciidoc cmdline_options.en.asciidoc $(wildcard autogen/user/*.asciidoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=highlight.js -o weechat_user.en.html $(abs_top_srcdir)/doc/en/weechat_user.en.asciidoc

# plugin API reference
weechat_plugin_api.en.html: weechat_plugin_api.en.asciidoc $(wildcard autogen/plugin_api/*.asciidoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=highlight.js -o weechat_plugin_api.en.html $(abs_top_srcdir)/doc/en/weechat_plugin_api.en.asciidoc

# scripting guide
weechat_scripting.en.html: weechat_scripting.en.asciidoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=highlight.js -o weechat_scripting.en.html $(abs_top_srcdir)/doc/en/weechat_scripting.en.asciidoc

# FAQ
weechat_faq.en.html: weechat_faq.en.asciidoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=highlight.js -o weechat_faq.en.html $(abs_top_srcdir)/doc/en/weechat_faq.en.asciidoc

# quickstart
weechat_quickstart.en.html: weechat_quickstart.en.asciidoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=highlight.js -o weechat_quickstart.en.html $(abs_top_srcdir)/doc/en/weechat_quickstart.en.asciidoc

# tester's guide
weechat_tester.en.html: weechat_tester.en.asciidoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=highlight.js -o weechat_tester.en.html $(abs_top_srcdir)/doc/en/weechat_tester.en.asciidoc

# relay protocol
weechat_relay_protocol.en.html: weechat_relay_protocol.en.asciidoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=highlight.js -o weechat_relay_protocol.en.html $(abs_top_srcdir)/doc/en/weechat_relay_protocol.en.asciidoc

# developer's guide
weechat_dev.en.html: weechat_dev.en.asciidoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=highlight.js -o weechat_dev.en.html $(abs_top_srcdir)/doc/en/weechat_dev.en.asciidoc

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
	-rm -f weechat.1 weechat_*.html
