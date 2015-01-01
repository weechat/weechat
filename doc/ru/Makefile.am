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
             weechat_quickstart.ru.asciidoc

if MAN
  man_targets =
  man_install =
  man_uninstall =
endif
if DOC
  doc_targets = weechat_quickstart.ru.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# quickstart
weechat_quickstart.ru.html: weechat_quickstart.ru.asciidoc
	$(ASCIIDOC) -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_quickstart.ru.html $(abs_top_srcdir)/doc/ru/weechat_quickstart.ru.asciidoc

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
	-rm -f *.html
