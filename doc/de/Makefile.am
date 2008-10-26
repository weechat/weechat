# Copyright (c) 2003-2008 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

LANGCODE = de
BOOK = weechat.$(LANGCODE)
BOOK_INCLUDE = authors.$(LANGCODE).xml \
               intro.$(LANGCODE).xml \
               install.$(LANGCODE).xml \
               usage.$(LANGCODE).xml \
               plugins.$(LANGCODE).xml \
               plugin_api.$(LANGCODE).xml \
               plugin_charset.$(LANGCODE).xml \
               plugin_fifo.$(LANGCODE).xml \
               plugin_irc.$(LANGCODE).xml \
               plugin_scripts.$(LANGCODE).xml \
               plugin_xfer.$(LANGCODE).xml \
               ../include_autogen.xml \
               autogen/weechat_commands.xml \
               autogen/weechat_options.xml \
               autogen/weechat_infos.xml \
               autogen/irc_commands.xml \
               autogen/irc_options.xml \
               autogen/irc_infos.xml
QUICKSTART = weechat_quickstart.$(LANGCODE).txt

EXTRA_DIST = $(BOOK).xml $(BOOK_INCLUDE) $(QUICKSTART)

if DBLATEX_FOUND
PDF = pdf-stamp
endif

docdir = $(datadir)/doc/$(PACKAGE)

all-local: html-stamp $(PDF)

# HTML output with chunks (many pages)

html: html-stamp

html-stamp: $(BOOK).xml $(BOOK_INCLUDE) ../weechat-html.xsl ../weechat-doc.css ../include_autogen.xml
	echo "<pubdate>`date '+%F %T'`</pubdate>" >date.xml
	cp ../include_autogen.xml .
	mkdir -p html/
	xsltproc -o html/ ../weechat-html.xsl $(BOOK).xml || true
	rm -f include_autogen.xml
	cp ../weechat-doc.css html/
	touch html-stamp

# HTML output, all in one page

html1: html1-stamp

html1-stamp: $(BOOK).xml $(BOOK_INCLUDE) ../weechat-html-one.xsl ../weechat-doc.css ../include_autogen.xml
	echo "<pubdate>`date '+%F %T'`</pubdate>" >date.xml
	cp ../include_autogen.xml .
	mkdir -p html1/
	xsltproc -o html1/$(BOOK).html ../weechat-html-one.xsl $(BOOK).xml || true
	rm -f include_autogen.xml
	cp ../weechat-doc.css html1/
	touch html1-stamp

# PDF output

pdf: pdf-stamp

pdf-stamp: $(BOOK).xml $(BOOK_INCLUDE) ../include_autogen.xml
	echo "<pubdate>`date '+%F %T'`</pubdate>" >date.xml
	cp ../include_autogen.xml .
	dblatex -P draft.mode=no -c ../dblatex.conf $(BOOK).xml || true
	rm -f include_autogen.xml
	touch pdf-stamp

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/html/$(LANGCODE)/
	$(INSTALL_DATA) html/* $(DESTDIR)$(docdir)/html/$(LANGCODE)/
	$(INSTALL_DATA) $(QUICKSTART) $(DESTDIR)$(docdir)/
if DBLATEX_FOUND
	$(INSTALL_DATA) $(BOOK).pdf $(DESTDIR)$(docdir)/
endif

# clean

clean-local:
	-rm -f $(BOOK).html $(BOOK).pdf $(BOOK).txt date.xml include_autogen.xml
	-rm -rf html/ html1/
	-rm -f html-stamp html1-stamp pdf-stamp
