#
# Copyright (C) 2003-2015 Sébastien Helleu <flashcode@flashtux.org>
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
             weechat.1.ja.asciidoc \
             cmdline_options.ja.asciidoc \
             weechat_user.ja.asciidoc \
             weechat_plugin_api.ja.asciidoc \
             weechat_scripting.ja.asciidoc \
             weechat_faq.ja.asciidoc \
             weechat_quickstart.ja.asciidoc \
             weechat_tester.ja.asciidoc \
             weechat_relay_protocol.ja.asciidoc \
             weechat_dev.ja.asciidoc \
             $(wildcard autogen/user/*.asciidoc) \
             $(wildcard autogen/plugin_api/*.asciidoc)

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
weechat.1: weechat.1.ja.asciidoc cmdline_options.ja.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/ja/weechat.1.ja.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/ja/cmdline_options.ja.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="WeeChat $(VERSION)" -d manpage -f manpage -L weechat.1.ja.asciidoc)
	mv -f tmp_man/weechat.1 .
	rm -rf tmp_man

# user's guide
weechat_user.ja.html: weechat_user.ja.asciidoc cmdline_options.ja.asciidoc $(wildcard autogen/user/*.asciidoc)
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_user.ja.html $(abs_top_srcdir)/doc/ja/weechat_user.ja.asciidoc

# plugin API reference
weechat_plugin_api.ja.html: weechat_plugin_api.ja.asciidoc $(wildcard autogen/plugin_api/*.asciidoc)
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_plugin_api.ja.html $(abs_top_srcdir)/doc/ja/weechat_plugin_api.ja.asciidoc

# scripting guide
weechat_scripting.ja.html: weechat_scripting.ja.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_scripting.ja.html $(abs_top_srcdir)/doc/ja/weechat_scripting.ja.asciidoc

# FAQ
weechat_faq.ja.html: weechat_faq.ja.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_faq.ja.html $(abs_top_srcdir)/doc/ja/weechat_faq.ja.asciidoc

# quickstart
weechat_quickstart.ja.html: weechat_quickstart.ja.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_quickstart.ja.html $(abs_top_srcdir)/doc/ja/weechat_quickstart.ja.asciidoc

# tester's guide
weechat_tester.ja.html: weechat_tester.ja.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_tester.ja.html $(abs_top_srcdir)/doc/ja/weechat_tester.ja.asciidoc

# relay protocol
weechat_relay_protocol.ja.html: weechat_relay_protocol.ja.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_relay_protocol.ja.html $(abs_top_srcdir)/doc/ja/weechat_relay_protocol.ja.asciidoc

# developer's guide
weechat_dev.ja.html: weechat_dev.ja.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_dev.ja.html $(abs_top_srcdir)/doc/ja/weechat_dev.ja.asciidoc

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
	-rm -f weechat.1 weechat.1.ja.asciidoc cmdline_options.ja.asciidoc
	-rm -f *.html
