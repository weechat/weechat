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
             weechat.1.de.txt \
             cmdline_options.de.txt \
             weechat_user.de.txt \
             weechat_scripting.de.txt \
             weechat_faq.de.txt \
             weechat_quickstart.de.txt \
             weechat_tester.de.txt \
             $(wildcard autogen/user/*.txt)

if MAN
  man_targets = weechat.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.de.html \
		weechat_scripting.de.html \
		weechat_faq.de.html \
		weechat_quickstart.de.html \
		weechat_tester.de.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man page
weechat.1: weechat.1.de.txt cmdline_options.de.txt
	$(A2X) -a lang=de -a revision="WeeChat $(VERSION)" -d manpage -f manpage -L -D . $(abs_top_srcdir)/doc/de/weechat.1.de.txt

# user's guide
weechat_user.de.html: weechat_user.de.txt cmdline_options.de.txt $(wildcard autogen/user/*.txt)
	$(ASCIIDOC) -a lang=de -a toc -a toclevels=4 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_user.de.html $(abs_top_srcdir)/doc/de/weechat_user.de.txt

# scripting guide
weechat_scripting.de.html: weechat_scripting.de.txt
	$(ASCIIDOC) -a lang=de -a toc -a toclevels=3 -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_scripting.de.html $(abs_top_srcdir)/doc/de/weechat_scripting.de.txt

# FAQ
weechat_faq.de.html: weechat_faq.de.txt
	$(ASCIIDOC) -a lang=de -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -f $(abs_top_srcdir)/doc/asciidoc.conf -n -o weechat_faq.de.html $(abs_top_srcdir)/doc/de/weechat_faq.de.txt

# quickstart
weechat_quickstart.de.html: weechat_quickstart.de.txt
	$(ASCIIDOC) -a lang=de -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_quickstart.de.html $(abs_top_srcdir)/doc/de/weechat_quickstart.de.txt

# tester's guide
weechat_tester.de.html: weechat_tester.de.txt
	$(ASCIIDOC) -a lang=de -a toc -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=$(abs_top_srcdir)/doc/asciidoc.css -n -o weechat_tester.de.html $(abs_top_srcdir)/doc/de/weechat_tester.de.txt

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/de/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/de/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/de/man1/weechat.1
	-rmdir $(DESTDIR)$(mandir)/de/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.de.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-rm -f weechat.1
	-rm -f *.html
