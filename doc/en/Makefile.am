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
             weechat.1.en.adoc \
             weechat-headless.1.en.adoc \
             weechat_user.en.adoc \
             weechat_plugin_api.en.adoc \
             weechat_scripting.en.adoc \
             weechat_faq.en.adoc \
             weechat_quickstart.en.adoc \
             weechat_relay_protocol.en.adoc \
             weechat_dev.en.adoc \
             includes/autogen_api_completions.en.adoc \
             includes/autogen_api_hdata.en.adoc \
             includes/autogen_api_infolists.en.adoc \
             includes/autogen_api_infos.en.adoc \
             includes/autogen_api_infos_hashtable.en.adoc \
             includes/autogen_api_plugins_priority.en.adoc \
             includes/autogen_api_url_options.en.adoc \
             includes/autogen_user_commands.en.adoc \
             includes/autogen_user_default_aliases.en.adoc \
             includes/autogen_user_irc_colors.en.adoc \
             includes/autogen_user_options.en.adoc \
             includes/cmdline_options.en.adoc \
             includes/man.en.adoc

if MAN
  man_targets = weechat.1 \
                weechat-headless.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.en.html \
		weechat_plugin_api.en.html \
		weechat_scripting.en.html \
		weechat_faq.en.html \
		weechat_quickstart.en.html \
		weechat_relay_protocol.en.html \
		weechat_dev.en.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man pages
weechat.1: weechat.1.en.adoc includes/cmdline_options.en.adoc includes/man.en.adoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -b manpage -o weechat.1 $(abs_top_srcdir)/doc/en/weechat.1.en.adoc
weechat-headless.1: weechat-headless.1.en.adoc includes/cmdline_options.en.adoc includes/man.en.adoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -b manpage -o weechat-headless.1 $(abs_top_srcdir)/doc/en/weechat-headless.1.en.adoc

# user's guide
weechat_user.en.html: weechat_user.en.adoc includes/cmdline_options.en.adoc $(wildcard includes/autogen_user_*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_user.en.html $(abs_top_srcdir)/doc/en/weechat_user.en.adoc

# plugin API reference
weechat_plugin_api.en.html: weechat_plugin_api.en.adoc $(wildcard includes/autogen_api_*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_plugin_api.en.html $(abs_top_srcdir)/doc/en/weechat_plugin_api.en.adoc

# scripting guide
weechat_scripting.en.html: weechat_scripting.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_scripting.en.html $(abs_top_srcdir)/doc/en/weechat_scripting.en.adoc

# FAQ
weechat_faq.en.html: weechat_faq.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_faq.en.html $(abs_top_srcdir)/doc/en/weechat_faq.en.adoc

# quickstart
weechat_quickstart.en.html: weechat_quickstart.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_quickstart.en.html $(abs_top_srcdir)/doc/en/weechat_quickstart.en.adoc

# relay protocol
weechat_relay_protocol.en.html: weechat_relay_protocol.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_relay_protocol.en.html $(abs_top_srcdir)/doc/en/weechat_relay_protocol.en.adoc

# developer's guide
weechat_dev.en.html: weechat_dev.en.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_dev.en.html $(abs_top_srcdir)/doc/en/weechat_dev.en.adoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/man1/weechat.1
	$(RM) $(DESTDIR)$(mandir)/man1/weechat-headless.1
	-rmdir $(DESTDIR)$(mandir)/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.en.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-$(RM) weechat.1 weechat-headless.1 weechat_*.html
