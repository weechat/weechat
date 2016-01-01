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
             weechat.1.cs.asciidoc \
             cmdline_options.cs.asciidoc \
             weechat_quickstart.cs.asciidoc \
             $(wildcard autogen/user/*.asciidoc) \
             $(wildcard autogen/plugin_api/*.asciidoc)

if MAN
  man_targets = weechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_quickstart.cs.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
weechat.1: weechat.1.cs.asciidoc cmdline_options.cs.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/cs/weechat.1.cs.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/cs/cmdline_options.cs.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="WeeChat $(VERSION)" -d manpage -f manpage -L weechat.1.cs.asciidoc)
	mv -f tmp_man/weechat.1 .
	rm -rf tmp_man

# quickstart
weechat_quickstart.cs.html: weechat_quickstart.cs.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_quickstart.cs.html $(abs_top_srcdir)/doc/cs/weechat_quickstart.cs.asciidoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/cs/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/cs/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/cs/man1/weechat.1
	-rmdir $(DESTDIR)$(mandir)/cs/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.cs.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f weechat.1 weechat.1.cs.asciidoc cmdline_options.cs.asciidoc
	-rm -f *.html
