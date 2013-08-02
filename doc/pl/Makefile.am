#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
             weechat_scripting.pl.txt \
             weechat_faq.pl.txt \
             weechat_quickstart.pl.txt \
             weechat_tester.pl.txt

if MAN
  man_targets =
  man_install =
  man_uninstall =
endif
if DOC
  doc_targets = weechat_scripting.pl.html \
		weechat_faq.pl.html \
		weechat_quickstart.pl.html \
		weechat_tester.pl.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# scripting guide
weechat_scripting.pl.html: weechat_scripting.pl.txt
	$(ASCIIDOC) -a toc -a toclevels=3 -a toc-title='Spis treści' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_scripting.pl.html $(abs_top_srcdir)/doc/pl/weechat_scripting.pl.txt

# FAQ
weechat_faq.pl.html: weechat_faq.pl.txt
	$(ASCIIDOC) -a toc -a toc-title='Spis treści' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_faq.pl.html $(abs_top_srcdir)/doc/pl/weechat_faq.pl.txt

# quickstart
weechat_quickstart.pl.html: weechat_quickstart.pl.txt
	$(ASCIIDOC) -a toc -a toc-title='Spis treści' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_quickstart.pl.html $(abs_top_srcdir)/doc/pl/weechat_quickstart.pl.txt

# tester's guide
weechat_tester.pl.html: weechat_tester.pl.txt
	$(ASCIIDOC) -a toc -a toc-title='Spis treści' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_tester.pl.html $(abs_top_srcdir)/doc/pl/weechat_tester.pl.txt

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/pl/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/pl/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/pl/man1/weechat.1
	-rmdir $(DESTDIR)$(mandir)/pl/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.pl.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f *.html
