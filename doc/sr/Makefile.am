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
             weechat.1.sr.adoc \
             weechat-headless.1.sr.adoc \
             weechat_user.sr.adoc \
             weechat_plugin_api.sr.adoc \
             weechat_scripting.sr.adoc \
             weechat_faq.sr.adoc \
             weechat_quickstart.sr.adoc \
             weechat_relay_protocol.sr.adoc \
             weechat_dev.sr.adoc \
             includes/autogen_api_completions.sr.adoc \
             includes/autogen_api_hdata.sr.adoc \
             includes/autogen_api_infolists.sr.adoc \
             includes/autogen_api_infos.sr.adoc \
             includes/autogen_api_infos_hashtable.sr.adoc \
             includes/autogen_api_plugins_priority.sr.adoc \
             includes/autogen_api_url_options.sr.adoc \
             includes/autogen_user_commands.sr.adoc \
             includes/autogen_user_default_aliases.sr.adoc \
             includes/autogen_user_irc_colors.sr.adoc \
             includes/autogen_user_options.sr.adoc \
             includes/cmdline_options.sr.adoc \
             includes/man.sr.adoc

if MAN
  man_targets = weechat.1 \
                weechat-headless.1
  man_install = install-man
  man_uninstall = uninstall-man
endif
if DOC
  doc_targets = weechat_user.sr.html \
		weechat_plugin_api.sr.html \
		weechat_scripting.sr.html \
		weechat_faq.sr.html \
		weechat_quickstart.sr.html \
		weechat_relay_protocol.sr.html \
		weechat_dev.sr.html
  doc_install = install-doc
  doc_uninstall = uninstall-doc
endif
all-local: $(man_targets) $(doc_targets)

# man pages
weechat.1: weechat.1.sr.adoc includes/cmdline_options.sr.adoc includes/man.sr.adoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -b manpage -o weechat.1 $(abs_top_srcdir)/doc/sr/weechat.1.sr.adoc
weechat-headless.1: weechat-headless.1.sr.adoc includes/cmdline_options.sr.adoc includes/man.sr.adoc
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -b manpage -o weechat-headless.1 $(abs_top_srcdir)/doc/sr/weechat-headless.1.sr.adoc

# user's guide
weechat_user.sr.html: weechat_user.sr.adoc includes/cmdline_options.sr.adoc $(wildcard includes/autogen_user_*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_user.sr.html $(abs_top_srcdir)/doc/sr/weechat_user.sr.adoc

# plugin API reference
weechat_plugin_api.sr.html: weechat_plugin_api.sr.adoc $(wildcard includes/autogen_api_*.adoc) $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_plugin_api.sr.html $(abs_top_srcdir)/doc/sr/weechat_plugin_api.sr.adoc

# scripting guide
weechat_scripting.sr.html: weechat_scripting.sr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_scripting.sr.html $(abs_top_srcdir)/doc/sr/weechat_scripting.sr.adoc

# FAQ
weechat_faq.sr.html: weechat_faq.sr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_faq.sr.html $(abs_top_srcdir)/doc/sr/weechat_faq.sr.adoc

# quickstart
weechat_quickstart.sr.html: weechat_quickstart.sr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_quickstart.sr.html $(abs_top_srcdir)/doc/sr/weechat_quickstart.sr.adoc

# relay protocol
weechat_relay_protocol.sr.html: weechat_relay_protocol.sr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_relay_protocol.sr.html $(abs_top_srcdir)/doc/sr/weechat_relay_protocol.sr.adoc

# developer's guide
weechat_dev.sr.html: weechat_dev.sr.adoc $(abs_top_srcdir)/doc/docinfo.html
	$(ASCIIDOCTOR) $(ASCIIDOCTOR_ARGS) -a revnumber="$(VERSION)" -o weechat_dev.sr.html $(abs_top_srcdir)/doc/sr/weechat_dev.sr.adoc

# install man/docs

install-data-hook: $(man_install) $(doc_install)

install-man:
	$(mkinstalldirs) $(DESTDIR)$(mandir)/sr/man1/
	$(INSTALL_DATA) *.1 $(DESTDIR)$(mandir)/sr/man1/

install-doc:
	$(mkinstalldirs) $(DESTDIR)$(docdir)/
	$(INSTALL_DATA) *.html $(DESTDIR)$(docdir)/

# uninstall man/docs

uninstall-hook: $(man_uninstall) $(doc_uninstall)

uninstall-man:
	$(RM) $(DESTDIR)$(mandir)/sr/man1/weechat.1
	$(RM) $(DESTDIR)$(mandir)/sr/man1/weechat-headless.1
	-rmdir $(DESTDIR)$(mandir)/sr/man1

uninstall-doc:
	$(RM) $(DESTDIR)$(docdir)/*.sr.html
	-rmdir $(DESTDIR)$(docdir)

# clean

clean-local:
	-$(RM) weechat.1 weechat-headless.1 weechat_*.html
