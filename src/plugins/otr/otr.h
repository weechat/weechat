/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_H
#define WEECHAT_PLUGIN_OTR_H

#include <gcrypt.h>
/* order matters: message.h needs the types from proto.h, tlv.h, context.h */
#include <libotr/version.h>
#include <libotr/proto.h>
#include <libotr/tlv.h>
#include <libotr/context.h>
#include <libotr/userstate.h>
#include <libotr/privkey.h>
#include <libotr/instag.h>
#include <libotr/message.h>

#define OTR_PROTOCOL           "irc"
#define OTR_BAR_ITEM           "otr"
#define OTR_MAX_ASSEMBLY       (64 * 1024)
#define OTR_LOG_UNTOUCHED      (-2)   /* we have not changed the log level */
#define OTR_LOG_DEFAULT        (-1)   /* buffer used the default log level */
#define OTR_ACTION_PREFIX      "/me "
#define OTR_UNENCRYPTED_PREFIX _("Unencrypted message received: ")

#define weechat_plugin weechat_otr_plugin

extern struct t_weechat_plugin *weechat_otr_plugin;

extern OtrlUserState otr_us;
extern char *otr_dir;
extern char *otr_keyfile;
extern char *otr_fprfile;
extern char *otr_instagfile;
extern struct t_gui_bar_item *otr_bar_item;
extern int otr_injecting;
extern int otr_no_tag;
extern int otr_shutdown;

#endif /* WEECHAT_PLUGIN_OTR_H */
