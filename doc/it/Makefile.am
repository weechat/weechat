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
             weechat.1.it.adoc \
             cmdline_options.it.adoc \
             weechat_user.it.adoc \
             weechat_plugin_api.it.adoc \
             weechat_scripting.it.adoc \
             weechat_faq.it.adoc \
             weechat_quickstart.it.adoc \
             weechat_tester.it.adoc \
             $(wildcard autogen/user/*.adoc) \
             $(wildcard autogen/plugin_api/*.adoc)

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
weechat.1: weechat.1.it.adoc cmdline_options.it.adoc
	$(ASCIIDOCTOR) -a revision="WeeChat $(VERSION)" -b manpage -o weechat.1 $(abs_top_srcdir)/doc/it/weechat.1.it.adoc

# user's guide
weechat_user.it.html: weechat_user.it.adoc cmdline_options.it.adoc $(wildcard autogen/user/*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_user.it.html $(abs_top_srcdir)/doc/it/weechat_user.it.adoc

# plugin API reference
weechat_plugin_api.it.html: weechat_plugin_api.it.adoc $(wildcard autogen/plugin_api/*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_plugin_api.it.html $(abs_top_srcdir)/doc/it/weechat_plugin_api.it.adoc

# scripting guide
weechat_scripting.it.html: weechat_scripting.it.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_scripting.it.html $(abs_top_srcdir)/doc/it/weechat_scripting.it.adoc

# FAQ
weechat_faq.it.html: weechat_faq.it.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_faq.it.html $(abs_top_srcdir)/doc/it/weechat_faq.it.adoc

# quickstart
weechat_quickstart.it.html: weechat_quickstart.it.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_quickstart.it.html $(abs_top_srcdir)/doc/it/weechat_quickstart.it.adoc

# tester's guide
weechat_tester.it.html: weechat_tester.it.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) -a revision="$(VERSION)" -a sectanchors -a source-highlighter=prettify -o weechat_tester.it.html $(abs_top_srcdir)/doc/it/weechat_tester.it.adoc

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
	-rm -f weechat.1 weechat_*.html
