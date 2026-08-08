/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_HOOK_FOCUS_H
#define WEECHAT_HOOK_FOCUS_H

struct t_weechat_plugin;
struct t_hashtable;
struct t_infolist_item;

#define HOOK_FOCUS(hook, var) (((struct t_hook_focus *)hook->hook_data)->var)

typedef struct t_hashtable *(t_hook_callback_focus)(const void *pointer,
                                                    void *data,
                                                    struct t_hashtable *info);

struct t_hook_focus
{
    t_hook_callback_focus *callback;    /* focus callback                   */
    char *area;                         /* "chat" or bar item name          */
};

extern char *hook_focus_get_description (struct t_hook *hook);
extern struct t_hook *hook_focus (struct t_weechat_plugin *plugin,
                                  const char *area,
                                  t_hook_callback_focus *callback,
                                  const void *callback_pointer,
                                  void *callback_data);
extern struct t_hashtable *hook_focus_get_data (struct t_hashtable *hashtable_focus1,
                                                struct t_hashtable *hashtable_focus2);
extern void hook_focus_free_data (struct t_hook *hook);
extern struct t_hdata *hook_focus_hdata_hook_focus_cb (const void *pointer,
                                                       void *data,
                                                       const char *hdata_name);
extern int hook_focus_add_to_infolist (struct t_infolist_item *item,
                                       struct t_hook *hook);
extern void hook_focus_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_FOCUS_H */
