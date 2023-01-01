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
             weechat.1.fr.adoc \
             weechat-headless.1.fr.adoc \
             weechat_user.fr.adoc \
             weechat_plugin_api.fr.adoc \
             weechat_scripting.fr.adoc \
             weechat_faq.fr.adoc \
             weechat_quickstart.fr.adoc \
             weechat_relay_protocol.fr.adoc \
             weechat_dev.fr.adoc \
             includes/autogen_api_completions.fr.adoc \
             includes/autogen_api_hdata.fr.adoc \
             includes/autogen_api_infolists.fr.adoc \
             includes/autogen_api_infos.fr.adoc \
             includes/autogen_api_infos_hashtable.fr.adoc \
             includes/autogen_api_plugins_priority.fr.adoc \
             includes/autogen_api_url_options.fr.adoc \
             includes/autogen_user_commands.fr.adoc \
             includes/autogen_user_default_aliases.fr.adoc \
             includes/autogen_user_irc_colors.fr.adoc \
             includes/autogen_user_options.fr.adoc \
             includes/cmdline_options.fr.adoc \
             includes/man.fr.adoc

if MAN
  man_targets = weechat.1 \
                weechat-headless.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.fr.html \
		weechat_plugin_api.fr.html \
		weechat_scripting.fr.html \
		weechat_faq.fr.html \
		weechat_quickstart.fr.html \
		weechat_relay_protocol.fr.html \
		weechat_dev.fr.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man pages
weechat.1: weechat.1.fr.adoc includes/cmdline_options.fr.adoc includes/man.fr.adoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -b manpage -o weechat.1 $(abs_top_srcdir)/doc/fr/weechat.1.fr.adoc
weechat-headless.1: weechat-headless.1.fr.adoc includes/cmdline_options.fr.adoc includes/man.fr.adoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -b manpage -o weechat-headless.1 $(abs_top_srcdir)/doc/fr/weechat-headless.1.fr.adoc

# user's guide
weechat_user.fr.html: weechat_user.fr.adoc includes/cmdline_options.fr.adoc $(wildcard includes/autogen_user_*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_user.fr.html $(abs_top_srcdir)/doc/fr/weechat_user.fr.adoc

# plugin API reference
weechat_plugin_api.fr.html: weechat_plugin_api.fr.adoc $(wildcard includes/autogen_api_*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_plugin_api.fr.html $(abs_top_srcdir)/doc/fr/weechat_plugin_api.fr.adoc

# scripting guide
weechat_scripting.fr.html: weechat_scripting.fr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_scripting.fr.html $(abs_top_srcdir)/doc/fr/weechat_scripting.fr.adoc

# FAQ
weechat_faq.fr.html: weechat_faq.fr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_faq.fr.html $(abs_top_srcdir)/doc/fr/weechat_faq.fr.adoc

# quickstart
weechat_quickstart.fr.html: weechat_quickstart.fr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_quickstart.fr.html $(abs_top_srcdir)/doc/fr/weechat_quickstart.fr.adoc

# relay protocol
weechat_relay_protocol.fr.html: weechat_relay_protocol.fr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_relay_protocol.fr.html $(abs_top_srcdir)/doc/fr/weechat_relay_protocol.fr.adoc

# developer's guide
weechat_dev.fr.html: weechat_dev.fr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_dev.fr.html $(abs_top_srcdir)/doc/fr/weechat_dev.fr.adoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/fr/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/fr/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/fr/man1/weechat.1
	$(RM) $(DESTDIR)$(mandir)/fr/man1/weechat-headless.1
	-rmdir $(DESTDIR)$(mandir)/fr/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.fr.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-$(RM) weechat.1 weechat-headless.1 weechat_*.html
