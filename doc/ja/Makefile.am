#
# Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
             weechat.1.ja.txt \
             cmdline_options.ja.txt \
             weechat_user.ja.txt \
             weechat_scripting.ja.txt \
             weechat_faq.ja.txt \
             weechat_quickstart.ja.txt \
             weechat_tester.ja.txt \
             $(wildcard autogen/user/*.txt) \
             $(wildcard autogen/plugin_api/*.txt)

if MAN
  man_targets = weechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.ja.html \
		weechat_scripting.ja.html \
		weechat_faq.ja.html \
		weechat_quickstart.ja.html \
		weechat_tester.ja.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
weechat.1: weechat.1.ja.txt cmdline_options.ja.txt
	$(A2X) -a lang=ja -a revision="WeeChat $(VERSION)" -d manpage -f manpage -L -D . $(abs_top_srcdir)/doc/ja/weechat.1.ja.txt

# user's guide
weechat_user.ja.html: weechat_user.ja.txt cmdline_options.ja.txt $(wildcard autogen/user/*.txt)
	$(ASCIIDOC) -a toc -a toclevels=4 -a toc-title='目次' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_user.ja.html $(abs_top_srcdir)/doc/ja/weechat_user.ja.txt

# scripting guide
weechat_scripting.ja.html: weechat_scripting.ja.txt
	$(ASCIIDOC) -a toc -a toclevels=3 -a toc-title='目次' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_scripting.ja.html $(abs_top_srcdir)/doc/ja/weechat_scripting.ja.txt

# FAQ
weechat_faq.ja.html: weechat_faq.ja.txt
	$(ASCIIDOC) -a toc -a toc-title='目次' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_faq.ja.html $(abs_top_srcdir)/doc/ja/weechat_faq.ja.txt

# quickstart
weechat_quickstart.ja.html: weechat_quickstart.ja.txt
	$(ASCIIDOC) -a toc -a toc-title='目次' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_quickstart.ja.html $(abs_top_srcdir)/doc/ja/weechat_quickstart.ja.txt

# tester's guide
weechat_tester.ja.html: weechat_tester.ja.txt
	$(ASCIIDOC) -a toc -a toc-title='目次' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_tester.ja.html $(abs_top_srcdir)/doc/ja/weechat_tester.ja.txt

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
	-rm -f weechat.1
	-rm -f *.html
