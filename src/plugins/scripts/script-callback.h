/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_SCRIPT_CALLBACK_H
#define __WEECHAT_SCRIPT_CALLBACK_H 1

struct t_script_callback
{
    void *script;                        /* pointer to script               */
    char *function;                      /* script function called          */
    struct t_hook *hook;                 /* not NULL if hook                */
    struct t_gui_buffer *buffer;         /* not NULL if buffer callback     */
    struct t_script_callback *prev_callback; /* link to next callback       */
    struct t_script_callback *next_callback; /* link to previous callback   */
};

extern struct t_script_callback *script_callback_alloc ();
extern void script_callback_add (struct t_plugin_script *script,
                                 struct t_script_callback *callback);
extern void script_callback_remove (struct t_weechat_plugin *weechat_plugin,
                                    struct t_plugin_script *script,
                                    struct t_script_callback *script_callback);
extern void script_callback_remove_all (struct t_weechat_plugin *weechat_plugin,
                                        struct t_plugin_script *script);

#endif /* script-callback.h */
