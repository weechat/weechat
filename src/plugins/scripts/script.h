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
#define WEECHAT_SCRIPT_EXEC_INT    1
#define WEECHAT_SCRIPT_EXEC_STRING 2

#define WEECHAT_SCRIPT_MSG_NOT_INITIALIZED(__language, __function)      \
    weechat_printf (NULL,                                               \
                    weechat_gettext("%s%s: unable to call function "    \
                                    "\"%s\", script is not "            \
                                    "initialized"),                     \
                    weechat_prefix ("error"), __language, __function)
#define WEECHAT_SCRIPT_MSG_WRONG_ARGUMENTS(__language, __function)      \
    weechat_printf (NULL,                                               \
                    weechat_gettext("%s%s: wrong arguments for "        \
                                    "function \"%s\""),                 \
                    weechat_prefix ("error"), __language, __function)

struct t_script_hook
{
    struct t_hook *hook;                 /* pointer to hook                 */
    char *function;                      /* script function called          */
    void *script;                        /* pointer to script               */
    struct t_script_hook *prev_hook;     /* link to next script hook        */
    struct t_script_hook *next_hook;     /* link to previous hook           */
};

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
    
    struct t_script_hook *hooks;         /* hooks for script                */
    
    struct t_plugin_script *prev_script; /* link to previous script         */
    struct t_plugin_script *next_script; /* link to next script             */
};

extern char *weechat_script_pointer_to_string (void *pointer);
extern void *weechat_script_string_to_pointer (char *pointer_str);
extern void weechat_script_auto_load (struct t_weechat_plugin *weechat_plugin,
                                      char *language,
                                      int (*callback)(void *data, char *filename));
extern struct t_plugin_script *weechat_script_search (struct t_weechat_plugin *weechat_plugin,
                                                      struct t_plugin_script **list,
                                                      char *name);
extern char *weechat_script_search_full_name (struct t_weechat_plugin *weechat_plugin,
                                              char *language, char *filename);
extern struct t_plugin_script *weechat_script_add (struct t_weechat_plugin *weechat_plugin,
                                                   struct t_plugin_script **script_list,
                                                   char *filename, char *name,
                                                   char *version,
                                                   char *shutdown_func,
                                                   char *description,
                                                   char *charset);
extern void weechat_script_remove (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script **script_list,
                                   struct t_plugin_script *script);
extern void weechat_script_charset_set (struct t_plugin_script *script,
                                        char *charset);
extern void weechat_script_printf (struct t_weechat_plugin *plugin,
                                   struct t_plugin_script *script,
                                   struct t_gui_buffer *buffer,
                                   char *format, ...);
extern void weechat_script_infobar_printf (struct t_weechat_plugin *weechat_plugin,
                                           struct t_plugin_script *script,
                                           int delay, char *color_name,
                                           char *format, ...);
extern void weechat_script_log_printf (struct t_weechat_plugin *weechat_plugin,
                                       struct t_plugin_script *script,
                                       char *format, ...);
extern int weechat_script_hook_command (struct t_weechat_plugin *weechat_plugin,
                                        struct t_plugin_script *script,
                                        char *command, char *description,
                                        char *args, char *args_description,
                                        char *completion,
                                        int (*callback)(void *data,
                                                        struct t_gui_buffer *buffer,
                                                        int argc, char **argv,
                                                        char **argv_eol),
                                        char *function);
extern int weechat_script_hook_timer (struct t_weechat_plugin *weechat_plugin,
                                      struct t_plugin_script *script,
                                      int interval, int align_second,
                                      int max_calls,
                                      int (*callback)(void *data),
                                      char *function);
extern int weechat_script_hook_fd (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script *script,
                                   int fd, int flag_read, int flag_write,
                                   int flag_exception,
                                   int (*callback)(void *data),
                                   char *function);
extern int weechat_script_hook_print (struct t_weechat_plugin *weechat_plugin,
                                      struct t_plugin_script *script,
                                      struct t_gui_buffer *buffer,
                                      char *message, int strip_colors,
                                      int (*callback)(void *data,
                                                      struct t_gui_buffer *buffer,
                                                      time_t date,
                                                      char *prefix,
                                                      char *message),
                                      char *function);
extern int weechat_script_hook_signal (struct t_weechat_plugin *weechat_plugin,
                                       struct t_plugin_script *script,
                                       char *signal,
                                       int (*callback)(void *data,
                                                       char *signal,
                                                       char *type_data,
                                                       void *signal_data),
                                       char *function);
extern int weechat_script_hook_config (struct t_weechat_plugin *weechat_plugin,
                                       struct t_plugin_script *script,
                                       char *type, char *option,
                                       int (*callback)(void *data, char *type,
                                                       char *option,
                                                       char *value),
                                       char *function);
extern int weechat_script_hook_completion (struct t_weechat_plugin *weechat_plugin,
                                           struct t_plugin_script *script,
                                           char *completion,
                                           int (*callback)(void *data,
                                                           char *completion,
                                                           struct t_gui_buffer *buffer,
                                                           struct t_weelist *list),
                                           char *function);
extern void weechat_script_unhook (struct t_weechat_plugin *weechat_plugin,
                                   struct t_plugin_script *script,
                                   struct t_hook *hook);
extern void weechat_script_command (struct t_weechat_plugin *weechat_plugin,
                                    struct t_plugin_script *script,
                                    struct t_gui_buffer *buffer,
                                    char *command);
extern char *weechat_script_config_get_plugin (struct t_weechat_plugin *weechat_plugin,
                                               struct t_plugin_script *script,
                                               char *option);
extern int weechat_script_config_set_plugin (struct t_weechat_plugin *weechat_plugin,
                                             struct t_plugin_script *script,
                                             char *option, char *value);

#endif /* script.h */
