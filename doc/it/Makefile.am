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
             weechat.1.it.asciidoc \
             cmdline_options.it.asciidoc \
             weechat_user.it.asciidoc \
             weechat_plugin_api.it.asciidoc \
             weechat_scripting.it.asciidoc \
             weechat_faq.it.asciidoc \
             weechat_quickstart.it.asciidoc \
             weechat_tester.it.asciidoc \
             $(wildcard autogen/user/*.asciidoc) \
             $(wildcard autogen/plugin_api/*.asciidoc)

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
weechat.1: weechat.1.it.asciidoc cmdline_options.it.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/it/weechat.1.it.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/it/cmdline_options.it.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="WeeChat $(VERSION)" -d manpage -f manpage -L weechat.1.it.asciidoc)
	mv -f tmp_man/weechat.1 .
	rm -rf tmp_man

# user's guide
weechat_user.it.html: weechat_user.it.asciidoc cmdline_options.it.asciidoc $(wildcard autogen/user/*.asciidoc)
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_user.it.html $(abs_top_srcdir)/doc/it/weechat_user.it.asciidoc

# plugin API reference
weechat_plugin_api.it.html: weechat_plugin_api.it.asciidoc $(wildcard autogen/plugin_api/*.asciidoc)
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_plugin_api.it.html $(abs_top_srcdir)/doc/it/weechat_plugin_api.it.asciidoc

# scripting guide
weechat_scripting.it.html: weechat_scripting.it.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_scripting.it.html $(abs_top_srcdir)/doc/it/weechat_scripting.it.asciidoc

# FAQ
weechat_faq.it.html: weechat_faq.it.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_faq.it.html $(abs_top_srcdir)/doc/it/weechat_faq.it.asciidoc

# quickstart
weechat_quickstart.it.html: weechat_quickstart.it.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_quickstart.it.html $(abs_top_srcdir)/doc/it/weechat_quickstart.it.asciidoc

# tester's guide
weechat_tester.it.html: weechat_tester.it.asciidoc
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_builddir)/../asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -o weechat_tester.it.html $(abs_top_srcdir)/doc/it/weechat_tester.it.asciidoc

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
	-rm -f weechat.1 weechat.1.it.asciidoc cmdline_options.it.asciidoc
	-rm -f *.html
