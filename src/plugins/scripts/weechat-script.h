/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* This header is designed to be distributed with WeeChat plugins, if scripts
   management is needed */

#ifndef __WEECHAT_WEECHAT_SCRIPT_H
#define __WEECHAT_WEECHAT_SCRIPT_H 1

typedef struct t_plugin_script t_plugin_script;

struct t_plugin_script
{
    /* script variables */
    char *filename;                     /* name of script on disk              */
    void *interpreter;                  /* interpreter for script              */
    char *name;                         /* script name                         */
    char *description;                  /* plugin description                  */
    char *version;                      /* plugin version                      */
    char *shutdown_func;                /* function when script is unloaded    */
    
    t_plugin_script *prev_script;       /* link to previous script             */
    t_plugin_script *next_script;       /* link to next script                 */
};

extern void weechat_script_auto_load (t_weechat_plugin *, char *,
                                      int (*)(t_weechat_plugin *, char *));
extern t_plugin_script *weechat_script_search (t_weechat_plugin *,
                                               t_plugin_script **, char *);
extern char *weechat_script_search_full_name (t_weechat_plugin *,
                                              char *, char *);
extern t_plugin_script *weechat_script_add (t_weechat_plugin *,
                                            t_plugin_script **, char *, char *,
                                            char *, char *, char *);
extern void weechat_script_remove (t_weechat_plugin *,
                                   t_plugin_script **, t_plugin_script *);
extern void weechat_script_remove_handler (t_weechat_plugin *,
                                           t_plugin_script *,
                                           char *, char *);
extern void weechat_script_remove_timer_handler (t_weechat_plugin *,
                                                 t_plugin_script *,
                                                 char *);
extern char *weechat_script_get_plugin_config (t_weechat_plugin *,
                                               t_plugin_script *,
                                               char *);
extern int weechat_script_set_plugin_config (t_weechat_plugin *,
                                             t_plugin_script *,
                                             char *, char *);

#endif /* weechat-script.h */
