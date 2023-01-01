#
# Copyright (C) 2012-2023 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

docdir = $(datadir)/doc/$(PACKAGE)

EXTRA_DIST = CMakeLists.txt \
             docinfo.html \
             weechat_faq.es.adoc \
             weechat_quickstart.es.adoc

if MAN
  man_targets =
  man_install =
  man_uninstall =
endif
if DOC
  doc_targets = weechat_faq.es.html \
		weechat_quickstart.es.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# FAQ
weechat_faq.es.html: weechat_faq.es.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_faq.es.html $(abs_top_srcdir)/doc/es/weechat_faq.es.adoc

# quickstart
weechat_quickstart.es.html: weechat_quickstart.es.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_quickstart.es.html $(abs_top_srcdir)/doc/es/weechat_quickstart.es.adoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/es/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/es/man1/
	(cd '$(DESTDIR)$(mandir)/es/man1/' && $(RM) weechat-headless.1 && $(LN_S) weechat.1 weechat-headless.1)

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/es/man1/weechat.1
	$(RM) $(DESTDIR)$(mandir)/es/man1/weechat-headless.1
	-rmdir $(DESTDIR)$(mandir)/es/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.es.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-$(RM) weechat_*.html
