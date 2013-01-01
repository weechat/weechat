/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_PLUGIN_SCRIPT_CALLBACK_H
#define __WEECHAT_PLUGIN_SCRIPT_CALLBACK_H 1

struct t_plugin_script_cb
{
    void *script;                            /* pointer to script           */
    char *function;                          /* script function called      */
    char *data;                              /* data string for callback    */
    struct t_config_file *config_file;       /* not NULL for config file    */
    struct t_config_section *config_section; /* not NULL for config section */
    struct t_config_option *config_option;   /* not NULL for config option  */
    struct t_hook *hook;                     /* not NULL for hook           */
    struct t_gui_buffer *buffer;             /* not NULL for buffer         */
    struct t_gui_bar_item *bar_item;         /* not NULL for bar item       */
    struct t_upgrade_file *upgrade_file;     /* not NULL for upgrade file   */
    struct t_plugin_script_cb *prev_callback; /* link to next callback      */
    struct t_plugin_script_cb *next_callback; /* link to previous callback  */
};

extern struct t_plugin_script_cb *plugin_script_callback_add (struct t_plugin_script *script,
                                                              const char *function,
                                                              const char *data);
extern void plugin_script_callback_remove (struct t_plugin_script *script,
                                           struct t_plugin_script_cb *script_callback);
extern void plugin_script_callback_remove_all (struct t_plugin_script *script);
extern void plugin_script_callback_print_log (struct t_weechat_plugin *weechat_plugin,
                                              struct t_plugin_script_cb *script_callback);

#endif /* __WEECHAT_PLUGIN_SCRIPT_CALLBACK_H */
