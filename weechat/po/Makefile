# Copyright (c) 2003 FlashCode <flashcode@flashtux.org>
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

all:
	@echo "Usage: make init    => create new template (weechat.pot)"
	@echo "       make update  => update translations with new messages from sources"
	@echo "       make install => compile and install all translations"
	@echo "       make clean   => delete all binary and backup files"

init:
	@cd ../src; \
	xgettext -o ../po/weechat.pot --keyword=_ --keyword=N_ --msgid-bugs-address=flashcode@flashtux.org --msgstr-prefix `find -name "*.h"` `find -name "*.c"`

update:
	cd ../src; \
	xgettext -o /tmp/temp1.po --keyword=_ --keyword=N_ --msgid-bugs-address=flashcode@flashtux.org `find -name "*.h"` `find -name "*.c"`; \
	sed s/charset=CHARSET/charset=iso-8859-1/ /tmp/temp1.po >/tmp/temp2.po; \
	cd ../po; \
	for fichier in *.po; do \
		msgmerge -U $$fichier /tmp/temp2.po; \
	done

install:
	@for fichier in *.po; do \
		echo "Creating binary: $$fichier => weechat.mo..."; \
		msgfmt -o weechat.mo $$fichier; \
		REP=$${fichier%%.po}; \
		echo "Copying weechat.mo (from $$fichier) to /usr/share/locale/$$REP/LC_MESSAGES"; \
		cp -f weechat.mo /usr/share/locale/$$REP/LC_MESSAGES/ ; \
	done
	rm -f ./weechat.mo

clean:
	rm -f ./*.mo ./*.po~
