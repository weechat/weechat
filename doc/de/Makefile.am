#
# Copyright (C) 2003-2010 Sebastien Helleu <flashcode@flashtux.org>
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
             weechat_user.de.txt \
             weechat_scripting.de.txt \
             weechat_faq.de.txt \
             weechat_quickstart.de.txt \
             weechat_tester.de.txt \
             $(wildcard autogen/user/*.txt)

all-local: weechat_user.de.html \
		weechat_scripting.de.html \
		weechat_faq.de.html \
		weechat_quickstart.de.html \
		weechat_tester.de.html

# user's guide
weechat_user.de.html: weechat_user.de.txt $(wildcard autogen/user/*.txt)
	$(ASCIIDOC) -a toc -a toclevels=3 -a toc-title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=`pwd`/../asciidoc.css -n -o weechat_user.de.html weechat_user.de.txt

# scripting guide
weechat_scripting.de.html: weechat_scripting.de.txt
	$(ASCIIDOC) -a toc -a toclevels=3 -a toc-title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=`pwd`/../asciidoc.css -n -o weechat_scripting.de.html weechat_scripting.de.txt

# FAQ
weechat_faq.de.html: weechat_faq.de.txt
	$(ASCIIDOC) -a toc -a toc-title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=`pwd`/../asciidoc.css -n -o weechat_faq.de.html weechat_faq.de.txt

# quickstart
weechat_quickstart.de.html: weechat_quickstart.de.txt
	$(ASCIIDOC) -a toc -a toc-title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=`pwd`/../asciidoc.css -n -o weechat_quickstart.de.html weechat_quickstart.de.txt

# tester's guide
weechat_tester.de.html: weechat_tester.de.txt
	$(ASCIIDOC) -a toc -a toc-title='Inhaltsverzeichnis' -a date=`date "+%F"` -a revision="$(VERSION)" -a stylesheet=`pwd`/../asciidoc.css -n -o weechat_tester.de.html weechat_tester.de.txt

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# clean

clean-local:
	-rm -f *.html
