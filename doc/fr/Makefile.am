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

LANGCODE = fr

BOOK_AUTOGEN_INCLUDE = ../include_autogen.xml       \
                       autogen/alias_commands.xml   \
                       autogen/alias_infos.xml      \
                       autogen/aspell_commands.xml  \
                       autogen/aspell_options.xml   \
                       autogen/charset_commands.xml \
                       autogen/demo_commands.xml    \
                       autogen/fifo_infos.xml       \
                       autogen/irc_commands.xml     \
                       autogen/irc_infos.xml        \
                       autogen/irc_options.xml      \
                       autogen/logger_commands.xml  \
                       autogen/logger_infos.xml     \
                       autogen/logger_options.xml   \
                       autogen/lua_commands.xml     \
                       autogen/notify_commands.xml  \
                       autogen/perl_commands.xml    \
                       autogen/python_commands.xml  \
                       autogen/ruby_commands.xml    \
                       autogen/tcl_commands.xml     \
                       autogen/weechat_commands.xml \
                       autogen/weechat_infos.xml    \
                       autogen/weechat_options.xml  \
                       autogen/xfer_commands.xml    \
                       autogen/xfer_infos.xml       \
                       autogen/xfer_options.xml

BOOK_COMMON_INCLUDE  = authors.$(LANGCODE).xml \
                       intro.$(LANGCODE).xml

BOOK_USER            = weechat_user.$(LANGCODE)
BOOK_USER_INCLUDE    = user/install.$(LANGCODE).xml        \
                       user/plugin_charset.$(LANGCODE).xml \
                       user/plugin_fifo.$(LANGCODE).xml    \
                       user/plugin_irc.$(LANGCODE).xml     \
                       user/plugin_xfer.$(LANGCODE).xml    \
                       user/usage.$(LANGCODE).xml

BOOK_DEVEL           = weechat_devel.$(LANGCODE)
BOOK_DEVEL_INCLUDE   = devel/plugin_api.$(LANGCODE).xml     \
                       devel/plugin_scripts.$(LANGCODE).xml \
                       devel/plugins.$(LANGCODE).xml        \
                       devel/weechat_devel.$(LANGCODE).xml

QUICKSTART = weechat_quickstart.$(LANGCODE).txt

EXTRA_DIST = $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) \
             user/$(BOOK_USER).xml $(BOOK_USER_INCLUDE)          \
             devel/$(BOOK_DEVEL).xml $(BOOK_DEVEL_INCLUDE)        \
             $(QUICKSTART)

if DBLATEX_FOUND
PDF = user-pdf-stamp devel-pdf-stamp
endif

docdir = $(datadir)/doc/$(PACKAGE)

all-local: user-html-stamp devel-html-stamp $(PDF)

# user guide, HTML output with chunks (many pages)

user-html: user-html-stamp

user-html-stamp: $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) user/$(BOOK_USER).xml $(BOOK_USER_INCLUDE) ../weechat-html.xsl ../weechat-doc.css ../include_autogen.xml
	rm -rf build/user-html/autogen/
	mkdir -p build/user-html/autogen/
	cp $(BOOK_AUTOGEN_INCLUDE) build/user-html/autogen/
	cp $(BOOK_COMMON_INCLUDE) build/user-html/
	cp user/$(BOOK_USER).xml build/user-html/
	cp $(BOOK_USER_INCLUDE) build/user-html/
	cp ../include_autogen.xml build/user-html/
	echo "<pubdate>`date '+%F %T'`</pubdate>" >build/user-html/date.xml
	mkdir -p html/user/
	cd build/user-html/ ; xsltproc -o ../../html/user/ ../../../weechat-html.xsl $(BOOK_USER).xml || true
	cp ../weechat-doc.css html/user/
	touch user-html-stamp

# user guide, HTML output, all in one page

user-html1: user-html1-stamp

user-html1-stamp: $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) user/$(BOOK_USER).xml $(BOOK_USER_INCLUDE) ../weechat-html-one.xsl ../weechat-doc.css ../include_autogen.xml
	rm -rf build/user-html1/autogen/
	mkdir -p build/user-html1/autogen/
	cp $(BOOK_AUTOGEN_INCLUDE) build/user-html1/autogen/
	cp $(BOOK_COMMON_INCLUDE) build/user-html1/
	cp user/$(BOOK_USER).xml build/user-html1/
	cp $(BOOK_USER_INCLUDE) build/user-html1/
	cp ../include_autogen.xml build/user-html1/
	echo "<pubdate>`date '+%F %T'`</pubdate>" >build/user-html1/date.xml
	mkdir -p html1/user/
	cd build/user-html1/ ; xsltproc -o ../../html1/user/$(BOOK_USER).html ../../../weechat-html-one.xsl $(BOOK_USER).xml || true
	cp ../weechat-doc.css html1/user/
	touch user-html1-stamp

# user guide, PDF output

user-pdf: user-pdf-stamp

user-pdf-stamp: $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) user/$(BOOK_USER).xml $(BOOK_USER_INCLUDE) ../include_autogen.xml
	rm -rf build/user-pdf/autogen/
	mkdir -p build/user-pdf/autogen/
	cp $(BOOK_AUTOGEN_INCLUDE) build/user-pdf/autogen/
	cp $(BOOK_COMMON_INCLUDE) build/user-pdf/
	cp user/$(BOOK_USER).xml build/user-pdf/
	cp $(BOOK_USER_INCLUDE) build/user-pdf/
	cp ../include_autogen.xml build/user-pdf/
	echo "<pubdate>`date '+%F %T'`</pubdate>" >build/user-pdf/date.xml
	mkdir -p pdf/user/
	cd build/user-pdf/ ; dblatex -o ../../pdf/user/$(BOOK_USER).pdf -P draft.mode=no -c ../../../dblatex.conf $(BOOK_USER).xml || true
	touch user-pdf-stamp

# devel guide, HTML output with chunks (many pages)

devel-html: devel-html-stamp

devel-html-stamp: $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) devel/$(BOOK_DEVEL).xml $(BOOK_DEVEL_INCLUDE) ../weechat-html.xsl ../weechat-doc.css ../include_autogen.xml
	rm -rf build/devel-html/autogen/
	mkdir -p build/devel-html/autogen/
	cp $(BOOK_AUTOGEN_INCLUDE) build/devel-html/autogen/
	cp $(BOOK_COMMON_INCLUDE) build/devel-html/
	cp devel/$(BOOK_DEVEL).xml build/devel-html/
	cp $(BOOK_DEVEL_INCLUDE) build/devel-html/
	cp ../include_autogen.xml build/devel-html/
	echo "<pubdate>`date '+%F %T'`</pubdate>" >build/devel-html/date.xml
	mkdir -p html/devel/
	cd build/devel-html/ ; xsltproc -o ../../html/devel/ ../../../weechat-html.xsl $(BOOK_DEVEL).xml || true
	cp ../weechat-doc.css html/devel/
	touch devel-html-stamp

# devel guide, HTML output, all in one page

devel-html1: devel-html1-stamp

devel-html1-stamp: $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) devel/$(BOOK_DEVEL).xml $(BOOK_DEVEL_INCLUDE) ../weechat-html-one.xsl ../weechat-doc.css ../include_autogen.xml
	rm -rf build/devel-html1/autogen/
	mkdir -p build/devel-html1/autogen/
	cp $(BOOK_AUTOGEN_INCLUDE) build/devel-html1/autogen/
	cp $(BOOK_COMMON_INCLUDE) build/devel-html1/
	cp devel/$(BOOK_DEVEL).xml build/devel-html1/
	cp $(BOOK_DEVEL_INCLUDE) build/devel-html1/
	cp ../include_autogen.xml build/devel-html1/
	echo "<pubdate>`date '+%F %T'`</pubdate>" >build/devel-html1/date.xml
	mkdir -p html1/devel/
	cd build/devel-html1/ ; xsltproc -o ../../html1/devel/$(BOOK_DEVEL).html ../../../weechat-html-one.xsl $(BOOK_DEVEL).xml || true
	cp ../weechat-doc.css html1/devel/
	touch devel-html1-stamp

# devel guide, PDF output

devel-pdf: devel-pdf-stamp

devel-pdf-stamp: $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) devel/$(BOOK_DEVEL).xml $(BOOK_DEVEL_INCLUDE) ../include_autogen.xml
	rm -rf build/devel-pdf/autogen/
	mkdir -p build/devel-pdf/autogen/
	cp $(BOOK_AUTOGEN_INCLUDE) build/devel-pdf/autogen/
	cp $(BOOK_COMMON_INCLUDE) build/devel-pdf/
	cp devel/$(BOOK_DEVEL).xml build/devel-pdf/
	cp $(BOOK_DEVEL_INCLUDE) build/devel-pdf/
	cp ../include_autogen.xml build/devel-pdf/
	echo "<pubdate>`date '+%F %T'`</pubdate>" >build/devel-pdf/date.xml
	mkdir -p pdf/devel/
	cd build/devel-pdf/ ; dblatex -o ../../pdf/devel/$(BOOK_DEVEL).pdf -P draft.mode=no -c ../../../dblatex.conf $(BOOK_DEVEL).xml || true
	touch devel-pdf-stamp

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/html/$(LANGCODE)/user/ $(DESTDIR)$(docdir)/html/$(LANGCODE)/devel/
	$(INSTALL_DATA) html/user/* $(DESTDIR)$(docdir)/html/$(LANGCODE)/user/
	$(INSTALL_DATA) html/devel/* $(DESTDIR)$(docdir)/html/$(LANGCODE)/devel/
	$(INSTALL_DATA) $(QUICKSTART) $(DESTDIR)$(docdir)/
if DBLATEX_FOUND
	$(INSTALL_DATA) pdf/user/$(BOOK_USER).pdf $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) pdf/devel/$(BOOK_DEVEL).pdf $(DESTDIR)$(docdir)/
endif

# clean

clean-local:
	-rm -rf html/ html1/ pdf/ build/
	-rm -f user-html-stamp user-html1-stamp user-pdf-stamp devel-html-stamp devel-html1-stamp devel-pdf-stamp
