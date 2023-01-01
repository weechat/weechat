#
# Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
# along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
#

docdir = $(datadir)/doc/$(PACKAGE)

EXTRA_DIST = CMakeLists.txt \
             docinfo.html \
             weechat.1.pl.adoc \
             weechat-headless.1.pl.adoc \
             weechat_user.pl.adoc \
             weechat_scripting.pl.adoc \
             weechat_faq.pl.adoc \
             weechat_quickstart.pl.adoc \
             includes/autogen_api_completions.pl.adoc \
             includes/autogen_api_hdata.pl.adoc \
             includes/autogen_api_infolists.pl.adoc \
             includes/autogen_api_infos_hashtable.pl.adoc \
             includes/autogen_api_infos.pl.adoc \
             includes/autogen_api_plugins_priority.pl.adoc \
             includes/autogen_api_url_options.pl.adoc \
             includes/autogen_user_commands.pl.adoc \
             includes/autogen_user_default_aliases.pl.adoc \
             includes/autogen_user_irc_colors.pl.adoc \
             includes/autogen_user_options.pl.adoc \
             includes/cmdline_options.pl.adoc \
             includes/man.pl.adoc

if MAN
  man_targets = weechat.1 \
                weechat-headless.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.pl.html \
		weechat_scripting.pl.html \
		weechat_faq.pl.html \
		weechat_quickstart.pl.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man pages
weechat.1: weechat.1.pl.adoc includes/cmdline_options.pl.adoc includes/man.pl.adoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -b manpage -o weechat.1 $(abs_top_srcdir)/doc/pl/weechat.1.pl.adoc
weechat-headless.1: weechat-headless.1.pl.adoc includes/cmdline_options.pl.adoc includes/man.pl.adoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -b manpage -o weechat-headless.1 $(abs_top_srcdir)/doc/pl/weechat-headless.1.pl.adoc

# user's guide
weechat_user.pl.html: weechat_user.pl.adoc includes/cmdline_options.pl.adoc $(wildcard includes/autogen_user_*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_user.pl.html $(abs_top_srcdir)/doc/pl/weechat_user.pl.adoc

# scripting guide
weechat_scripting.pl.html: weechat_scripting.pl.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_scripting.pl.html $(abs_top_srcdir)/doc/pl/weechat_scripting.pl.adoc

# FAQ
weechat_faq.pl.html: weechat_faq.pl.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_faq.pl.html $(abs_top_srcdir)/doc/pl/weechat_faq.pl.adoc

# quickstart
weechat_quickstart.pl.html: weechat_quickstart.pl.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_quickstart.pl.html $(abs_top_srcdir)/doc/pl/weechat_quickstart.pl.adoc

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
	$(RM) $(DESTDIR)$(mandir)/pl/man1/weechat-headless.1
	-rmdir $(DESTDIR)$(mandir)/pl/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.pl.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-$(RM) weechat.1 weechat-headless.1 weechat_*.html
