# Copyright (c) 2003-2009 FlashCode <flashcode@flashtux.org>
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
                       autogen/aspell_commands.xml  \
                       autogen/aspell_options.xml   \
                       autogen/charset_commands.xml \
                       autogen/charset_options.xml  \
                       autogen/completions.xml      \
                       autogen/demo_commands.xml    \
                       autogen/irc_commands.xml     \
                       autogen/irc_options.xml      \
                       autogen/infos.xml            \
                       autogen/infolists.xml        \
                       autogen/jabber_commands.xml  \
                       autogen/jabber_options.xml   \
                       autogen/logger_commands.xml  \
                       autogen/logger_options.xml   \
                       autogen/lua_commands.xml     \
                       autogen/perl_commands.xml    \
                       autogen/python_commands.xml  \
                       autogen/ruby_commands.xml    \
                       autogen/tcl_commands.xml     \
                       autogen/weechat_commands.xml \
                       autogen/weechat_options.xml  \
                       autogen/xfer_commands.xml    \
                       autogen/xfer_options.xml

BOOK_COMMON_INCLUDE  = authors.$(LANGCODE).xml \
                       intro.$(LANGCODE).xml

BOOK_USER            = weechat_user.$(LANGCODE)
BOOK_USER_INCLUDE    = user/install.$(LANGCODE).xml          \
                       user/usage.$(LANGCODE).xml            \
                       user/plugins.$(LANGCODE).xml          \
                       user/plugin_alias.$(LANGCODE).xml     \
                       user/plugin_aspell.$(LANGCODE).xml    \
                       user/plugin_charset.$(LANGCODE).xml   \
                       user/plugin_fifo.$(LANGCODE).xml      \
                       user/plugin_irc.$(LANGCODE).xml       \
                       user/plugin_jabber.$(LANGCODE).xml    \
                       user/plugin_logger.$(LANGCODE).xml    \
                       user/plugin_scripts.$(LANGCODE).xml   \
                       user/plugin_xfer.$(LANGCODE).xml

BOOK_DEV             = weechat_dev.$(LANGCODE)
BOOK_DEV_INCLUDE     = dev/plugins.$(LANGCODE).xml           \
                       dev/plugin_api.$(LANGCODE).xml        \
                       dev/plugin_c_api.$(LANGCODE).xml      \
                       dev/plugin_script_api.$(LANGCODE).xml

QUICKSTART = weechat_quickstart.$(LANGCODE).txt

EXTRA_DIST = $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE)  \
             user/$(BOOK_USER).xml $(BOOK_USER_INCLUDE)      \
             dev/$(BOOK_DEV).xml $(BOOK_DEV_INCLUDE)         \
             $(QUICKSTART)

if DBLATEX_FOUND
PDF = user-pdf-stamp dev-pdf-stamp
endif

docdir = $(datadir)/doc/$(PACKAGE)

all-local: user-html-stamp dev-html-stamp $(PDF)

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
	rm -rf html/user/
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
	rm -rf html1/user/
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
	rm -rf pdf/user/
	mkdir -p pdf/user/
	cd build/user-pdf/ ; dblatex -o ../../pdf/user/$(BOOK_USER).pdf -P draft.mode=no -c ../../../dblatex.conf $(BOOK_USER).xml || true
	touch user-pdf-stamp

# dev guide, HTML output with chunks (many pages)

dev-html: dev-html-stamp

dev-html-stamp: $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) dev/$(BOOK_DEV).xml $(BOOK_DEV_INCLUDE) ../weechat-html.xsl ../weechat-doc.css ../include_autogen.xml
	rm -rf build/dev-html/autogen/
	mkdir -p build/dev-html/autogen/
	cp $(BOOK_AUTOGEN_INCLUDE) build/dev-html/autogen/
	cp $(BOOK_COMMON_INCLUDE) build/dev-html/
	cp dev/$(BOOK_DEV).xml build/dev-html/
	cp $(BOOK_DEV_INCLUDE) build/dev-html/
	cp ../include_autogen.xml build/dev-html/
	echo "<pubdate>`date '+%F %T'`</pubdate>" >build/dev-html/date.xml
	rm -rf html/dev/
	mkdir -p html/dev/
	cd build/dev-html/ ; xsltproc -o ../../html/dev/ ../../../weechat-html.xsl $(BOOK_DEV).xml || true
	cp ../weechat-doc.css html/dev/
	touch dev-html-stamp

# dev guide, HTML output, all in one page

dev-html1: dev-html1-stamp

dev-html1-stamp: $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) dev/$(BOOK_DEV).xml $(BOOK_DEV_INCLUDE) ../weechat-html-one.xsl ../weechat-doc.css ../include_autogen.xml
	rm -rf build/dev-html1/autogen/
	mkdir -p build/dev-html1/autogen/
	cp $(BOOK_AUTOGEN_INCLUDE) build/dev-html1/autogen/
	cp $(BOOK_COMMON_INCLUDE) build/dev-html1/
	cp dev/$(BOOK_DEV).xml build/dev-html1/
	cp $(BOOK_DEV_INCLUDE) build/dev-html1/
	cp ../include_autogen.xml build/dev-html1/
	echo "<pubdate>`date '+%F %T'`</pubdate>" >build/dev-html1/date.xml
	rm -rf html1/dev/
	mkdir -p html1/dev/
	cd build/dev-html1/ ; xsltproc -o ../../html1/dev/$(BOOK_DEV).html ../../../weechat-html-one.xsl $(BOOK_DEV).xml || true
	cp ../weechat-doc.css html1/dev/
	touch dev-html1-stamp

# dev guide, PDF output

dev-pdf: dev-pdf-stamp

dev-pdf-stamp: $(BOOK_AUTOGEN_INCLUDE) $(BOOK_COMMON_INCLUDE) dev/$(BOOK_DEV).xml $(BOOK_DEV_INCLUDE) ../include_autogen.xml
	rm -rf build/dev-pdf/autogen/
	mkdir -p build/dev-pdf/autogen/
	cp $(BOOK_AUTOGEN_INCLUDE) build/dev-pdf/autogen/
	cp $(BOOK_COMMON_INCLUDE) build/dev-pdf/
	cp dev/$(BOOK_DEV).xml build/dev-pdf/
	cp $(BOOK_DEV_INCLUDE) build/dev-pdf/
	cp ../include_autogen.xml build/dev-pdf/
	echo "<pubdate>`date '+%F %T'`</pubdate>" >build/dev-pdf/date.xml
	rm -rf pdf/dev/
	mkdir -p pdf/dev/
	cd build/dev-pdf/ ; dblatex -o ../../pdf/dev/$(BOOK_DEV).pdf -P draft.mode=no -c ../../../dblatex.conf $(BOOK_DEV).xml || true
	touch dev-pdf-stamp

# install docs

install-data-hook:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/html/$(LANGCODE)/user/ $(DESTDIR)$(docdir)/html/$(LANGCODE)/dev/
	$(INSTALL_DATA) html/user/* $(DESTDIR)$(docdir)/html/$(LANGCODE)/user/
	$(INSTALL_DATA) html/dev/* $(DESTDIR)$(docdir)/html/$(LANGCODE)/dev/
	$(INSTALL_DATA) $(QUICKSTART) $(DESTDIR)$(docdir)/
if DBLATEX_FOUND
	$(INSTALL_DATA) pdf/user/$(BOOK_USER).pdf $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) pdf/dev/$(BOOK_DEV).pdf $(DESTDIR)$(docdir)/
endif

# clean

clean-local:
	-rm -rf html/ html1/ pdf/ build/
	-rm -f user-html-stamp user-html1-stamp user-pdf-stamp dev-html-stamp dev-html1-stamp dev-pdf-stamp
