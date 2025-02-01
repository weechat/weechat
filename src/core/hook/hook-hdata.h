/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
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
extern int hook_hdata_add_to_infolist (struct t_infolist_item *item,
                                       struct t_hook *hook);
extern void hook_hdata_print_log (struct t_hook *hook);

#endif /* WEECHAT_HOOK_HDATA_H */
