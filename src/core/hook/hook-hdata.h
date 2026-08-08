/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_HOOK_HDATA_H
#define WEECHAT_HOOK_HDATA_H

struct t_weechat_plugin;
struct t_infolist_item;

#define HOOK_HDATA(hook, var) (((struct t_hook_hdata *)hook->hook_data)->var)

typedef struct t_hdata *(t_hook_callback_hdata)(const void *pointer,
                                                void *data,
                                                const char *hdata_name);

struct t_hook_hdata
{
    t_hook_callback_hdata *callback;    /* hdata callback                   */
    char *hdata_name;                   /* hdata name                       */
    char *description;                  /* description                      */
};

extern char *hook_hdata_get_description (struct t_hook *hook);
extern struct t_hook *hook_hdata (struct t_weechat_plugin *plugin,
                                  const char *hdata_name,
                                  const char *description,
                                  t_hook_callback_hdata *callback,
                                  const void *callback_pointer,
                                  void *callback_data);
extern struct t_hdata *hook_hdata_get (struct t_weechat_plugin *plugin,
                                       const char *hdata_name);
extern void hook_hdata_free_data (struct t_hook *hook);
extern struct t_hdata *hook_hdata_hdata_hook_hdata_cb (const void *pointer,
                                                       void *data,
                                                       const char *hdata_name);
extern int hook_hdata_add_to_infolist (struct t_infolist_item *item,
                                       struct t_hook *hook);
extern void hook_hdata_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_HDATA_H */
