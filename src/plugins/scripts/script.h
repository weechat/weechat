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

/* This header is designed to be distributed with WeeChat plugins, if scripts
   management is needed */

#ifndef __WEECHAT_SCRIPT_H
#define __WEECHAT_SCRIPT_H 1

/* constants which defines return types for weechat_<lang>_exec functions */
#define SCRIPT_EXEC_INT    1
#define SCRIPT_EXEC_STRING 2

struct t_plugin_script
{
    /* script variables */
    char *filename;                      /* name of script on disk          */
    void *interpreter;                   /* interpreter for script          */
    char *name;                          /* script name                     */
    char *description;                   /* plugin description              */
    char *version;                       /* plugin version                  */
    char *shutdown_func;                 /* function when script is unloaded*/
    char *charset;                       /* script charset                  */
    
    struct t_plugin_script *prev_script; /* link to previous script         */
    struct t_plugin_script *next_script; /* link to next script             */
};

extern void weechat_script_auto_load (struct t_weechat_plugin *plugin, char *language,
                                      int (*callback)(void *data, char *filename));
extern struct t_plugin_script *weechat_script_search (struct t_weechat_plugin *plugin,
                                                      struct t_plugin_script **list,
                                                      char *name);
extern char *weechat_script_search_full_name (struct t_weechat_plugin *plugin,
                                              char *language, char *filename);
extern struct t_plugin_script *weechat_script_add (struct t_weechat_plugin *plugin,
                                                   struct t_plugin_script **script_list,
                                                   char *filename, char *name,
                                                   char *version,
                                                   char *shutdown_func,
                                                   char *description,
                                                   char *charset);
extern void weechat_script_remove (struct t_weechat_plugin *plugin,
                                   struct t_plugin_script **script_list,
                                   struct t_plugin_script *script);
extern void weechat_script_print (struct t_weechat_plugin *plugin,
                                  struct t_plugin_script *script,
                                  char *server, char *channel,
                                  char *message, ...);
extern void weechat_script_print_server (struct t_weechat_plugin *plugin,
                                         struct t_plugin_script *script,
                                         char *message, ...);
extern void weechat_script_print_infobar (struct t_weechat_plugin *plugin,
                                          struct t_plugin_script *script,
                                          int delay, char *message, ...);
extern void weechat_script_log (struct t_weechat_plugin *plugin,
                                struct t_plugin_script *script,
                                char *server, char *channel,
                                char *message, ...);
extern void weechat_script_exec_command (struct t_weechat_plugin *plugin,
                                         struct t_plugin_script *script,
                                         char *server, char *channel,
                                         char *command);
extern void weechat_script_remove_handler (struct t_weechat_plugin *plugin,
                                           struct t_plugin_script *script,
                                           char *arg1, char *arg2);
extern void weechat_script_remove_timer_handler (struct t_weechat_plugin *plugin,
                                                 struct t_plugin_script *script,
                                                 char *function);
extern void weechat_script_remove_keyboard_handler (struct t_weechat_plugin *plugin,
                                                    struct t_plugin_script *script,
                                                    char *function);
extern void weechat_script_remove_event_handler (struct t_weechat_plugin *plugin,
                                                 struct t_plugin_script *script,
                                                 char *function);
extern void weechat_script_remove_modifier (struct t_weechat_plugin *plugin,
                                            struct t_plugin_script *script,
                                            char *arg1, char *arg2,
                                            char *arg3);
extern char *weechat_script_get_plugin_config (struct t_weechat_plugin *plugin,
                                               struct t_plugin_script *script,
                                               char *option);
extern int weechat_script_set_plugin_config (struct t_weechat_plugin *plugin,
                                             struct t_plugin_script *script,
                                             char *option, char *value);
extern void weechat_script_set_charset (struct t_weechat_plugin *plugin,
                                        struct t_plugin_script *script,
                                        char *charset);

#endif /* script.h */
