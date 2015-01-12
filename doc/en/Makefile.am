#
# Copyright (C) 2003-2015 Sébastien Helleu <flashcode@flashtux.org>
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
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/en/weechat.1.en.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/en/cmdline_options.en.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="WeeChat $(VERSION)" -d manpage -f manpage -L weechat.1.en.asciidoc)
	mv -f tmp_man/weechat.1 .
	rm -rf tmp_man

# user's guide
weechat_user.en.html: weechat_user.en.asciidoc cmdline_options.en.asciidoc $(wildcard autogen/user/*.asciidoc)
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_user.en.html $(abs_top_srcdir)/doc/en/weechat_user.en.asciidoc

# plugin API reference
weechat_plugin_api.en.html: weechat_plugin_api.en.asciidoc $(wildcard autogen/plugin_api/*.asciidoc)
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_plugin_api.en.html $(abs_top_srcdir)/doc/en/weechat_plugin_api.en.asciidoc

# scripting guide
weechat_scripting.en.html: weechat_scripting.en.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_scripting.en.html $(abs_top_srcdir)/doc/en/weechat_scripting.en.asciidoc

# FAQ
weechat_faq.en.html: weechat_faq.en.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_faq.en.html $(abs_top_srcdir)/doc/en/weechat_faq.en.asciidoc

# quickstart
weechat_quickstart.en.html: weechat_quickstart.en.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_quickstart.en.html $(abs_top_srcdir)/doc/en/weechat_quickstart.en.asciidoc

# tester's guide
weechat_tester.en.html: weechat_tester.en.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_tester.en.html $(abs_top_srcdir)/doc/en/weechat_tester.en.asciidoc

# relay protocol
weechat_relay_protocol.en.html: weechat_relay_protocol.en.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_relay_protocol.en.html $(abs_top_srcdir)/doc/en/weechat_relay_protocol.en.asciidoc

# developer's guide
weechat_dev.en.html: weechat_dev.en.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_dev.en.html $(abs_top_srcdir)/doc/en/weechat_dev.en.asciidoc

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
	-rm -f weechat.1 weechat.1.en.asciidoc cmdline_options.en.asciidoc
	-rm -f *.html
