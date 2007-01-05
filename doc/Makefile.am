# Copyright (c) 2003-2007 FlashCode <flashcode@flashtux.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

SUBDIRS = . en fr de ru pl cs

man_MANS = weechat-curses.1

EXTRA_DIST = $(man_MANS) weechat-doc.css weechat-html-one.xsl.in weechat-html.xsl.in weechat-pdf.xsl dblatex.conf

all-local: weechat-html.xsl weechat-html-one.xsl

weechat-html.xsl: weechat-html.xsl.in
	sed -e 's,\@DOC_XSL_PREFIX\@,$(DOC_XSL_PREFIX),g' < weechat-html.xsl.in > weechat-html.xsl

weechat-html-one.xsl: weechat-html-one.xsl.in
	sed -e 's,\@DOC_XSL_PREFIX\@,$(DOC_XSL_PREFIX),g' < weechat-html-one.xsl.in > weechat-html-one.xsl

clean-local:
	-rm -f weechat-html.xsl weechat-html-one.xsl
