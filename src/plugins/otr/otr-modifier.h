/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_OTR_MODIFIER_H
#define WEECHAT_PLUGIN_OTR_MODIFIER_H

extern struct t_hashtable *otr_modifier_removed_batches;
extern struct t_hook *otr_modifier_assembly_timer;

extern int otr_modifier_assembly_timer_cb (const void *pointer, void *data,
                                           int remaining_calls);
extern char *otr_modifier_in_cb (const void *pointer, void *data,
                                 const char *modifier,
                                 const char *modifier_data,
                                 const char *string);
extern char *otr_modifier_out_cb (const void *pointer, void *data,
                                  const char *modifier,
                                  const char *modifier_data,
                                  const char *string);

extern int otr_modifier_policy_changed_cb (const void *pointer, void *data,
                                           const char *option,
                                           const char *value);
extern void otr_modifier_free_removed_batches (void);

#endif /* WEECHAT_PLUGIN_OTR_MODIFIER_H */
