/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_UPGRADE_H
#define WEECHAT_PLUGIN_OTR_UPGRADE_H

#define OTR_UPGRADE_FILENAME "otr"

/* For developers: please add new values ONLY AT THE END of enums */

enum t_otr_upgrade_type
{
    OTR_UPGRADE_TYPE_PEER = 0,
};

extern int otr_upgrade_keep_sessions;
extern char **otr_upgrade_queries;
extern struct t_hook *otr_upgrade_timer;
extern struct t_hook *otr_upgrade_hook_connected;

extern int otr_upgrade_signal_cb (const void *pointer, void *data,
                                  const char *signal, const char *type_data,
                                  void *signal_data);
extern int otr_upgrade_cancel_query (const char *server, const char *peer,
                                     int buffer_closing);
extern int otr_upgrade_load (void);
extern void otr_upgrade_end (void);

#endif /* WEECHAT_PLUGIN_OTR_UPGRADE_H */
