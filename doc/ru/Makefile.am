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
             weechat.1.ru.asciidoc \
             cmdline_options.ru.asciidoc \
             weechat_quickstart.ru.asciidoc

if MAN
  man_targets = weechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_quickstart.ru.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
weechat.1: weechat.1.ru.asciidoc cmdline_options.ru.asciidoc
	-mkdir tmp_man
	cp -f $(abs_top_srcdir)/doc/ru/weechat.1.ru.asciidoc ./tmp_man/
	cp -f $(abs_top_srcdir)/doc/ru/cmdline_options.ru.asciidoc ./tmp_man/
	(cd tmp_man && $(A2X) -a revision="WeeChat $(VERSION)" -d manpage -f manpage -L weechat.1.ru.asciidoc)
	mv -f tmp_man/weechat.1 .
	rm -rf tmp_man

# quickstart
weechat_quickstart.ru.html: weechat_quickstart.ru.asciidoc $(abs_top_srcdir)/doc/asciidoc_toc2.css
	$(ASCIIDOC) -b html5 -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc_toc2.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_quickstart.ru.html $(abs_top_srcdir)/doc/ru/weechat_quickstart.ru.asciidoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/ru/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/ru/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/ru/man1/weechat.1
	-rmdir $(DESTDIR)$(mandir)/ru/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.ru.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f weechat.1 weechat.1.ru.asciidoc cmdline_options.ru.asciidoc
	-rm -f *.html
