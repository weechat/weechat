# Copyright (c) 2003-2005 FlashCode <flashcode@flashtux.org>
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
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

all: weechat_doc_fr.texi weechat_doc_en.texi weechat_doc_pt.texi weechat_doc_es.texi
	export LANG=fr_FR && makeinfo --html --no-split --number-sections weechat_doc_fr.texi
	export LANG=fr_FR && makeinfo --no-headers --no-warn --no-split --number-sections weechat_doc_fr.texi >weechat_doc_fr.txt
	export LANG=en_EN && makeinfo --html --no-split --number-sections weechat_doc_en.texi
	export LANG=en_EN && makeinfo --no-headers --no-warn --no-split --number-sections weechat_doc_en.texi >weechat_doc_en.txt
	export LANG=es_ES && makeinfo --html --no-split --number-sections weechat_doc_es.texi
	export LANG=es_ES && makeinfo --no-headers --no-warn --no-split --number-sections weechat_doc_es.texi >weechat_doc_es.txt
	export LANG=pt_PT && makeinfo --html --no-split --number-sections weechat_doc_pt.texi
	export LANG=pt_PT && makeinfo --no-headers --no-warn --no-split --number-sections weechat_doc_pt.texi >weechat_doc_pt.txt
	export LANG=fr_FR && texi2pdf weechat_doc_fr.texi
	export LANG=en_EN && texi2pdf weechat_doc_en.texi
	export LANG=es_ES && texi2pdf weechat_doc_es.texi
	export LANG=pt_PT && texi2pdf weechat_doc_pt.texi

clean:
	rm -rf weechat_doc_*.aux weechat_doc_*.cp weechat_doc_*.fn \
        weechat_doc_*.html weechat_doc_*.ky weechat_doc_*.log \
        weechat_doc_*.pdf weechat_doc_*.pg weechat_doc_*.toc \
        weechat_doc_*.tp weechat_doc_*.txt weechat_doc_*.vr
