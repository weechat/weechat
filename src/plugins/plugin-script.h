/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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

#ifndef __WEECHAT_PLUGIN_SCRIPT_H
#define __WEECHAT_PLUGIN_SCRIPT_H 1

/* constants which defines return types for weechat_<lang>_exec functions */

enum t_weechat_script_exec_type
{
    WEECHAT_SCRIPT_EXEC_INT = 0,
    WEECHAT_SCRIPT_EXEC_STRING,
    WEECHAT_SCRIPT_EXEC_HASHTABLE,
};

#define WEECHAT_SCRIPT_HASHTABLE_DEFAULT_SIZE 16

#define WEECHAT_SCRIPT_MSG_NOT_INIT(__current_script,                   \
                                    __function)                         \
    weechat_printf (NULL,                                               \
                    weechat_gettext("%s%s: unable to call function "    \
                                    "\"%s\", script is not "            \
                                    "initialized (script: %s)"),        \
                    weechat_prefix ("error"), weechat_plugin->name,     \
                    __function,                                         \
                    (__current_script) ? __current_script : "-");
#define WEECHAT_SCRIPT_MSG_WRONG_ARGS(__current_script,                 \
                                      __function)                       \
    weechat_printf (NULL,                                               \
                    weechat_gettext("%s%s: wrong arguments for "        \
                                    "function \"%s\" (script: %s)"),    \
                    weechat_prefix ("error"), weechat_plugin->name,     \
                    __function,                                         \
                    (__current_script) ? __current_script : "-");

struct t_plugin_script
{
    /* script variables */
    char *filename;                      /* name of script on disk          */
    void *interpreter;                   /* interpreter for script          */
    char *name;                          /* script name                     */
    char *author;                        /* author name/mail                */
    char *version;                       /* plugin version                  */
    char *license;                       /* script license                  */
    char *description;                   /* plugin description              */
    char *shutdown_func;                 /* function when script is unloaded*/
    char *charset;                       /* script charset                  */
    struct t_script_callback *callbacks; /* callbacks for script            */
    int unloading;                       /* script is being unloaded        */
    struct t_plugin_script *prev_script; /* link to previous script         */
    struct t_plugin_script *next_script; /* link to next script             */
};

struct t_plugin_script_init
{
    int (*callback_command)(void *data, struct t_gui_buffer *buffer,
                            int argc, char **argv, char **argv_eol);
    int (*callback_completion)(void *data, const char *completion_item,
                               struct t_gui_buffer *buffer,
                               struct t_gui_completion *completion);
    struct t_infolist *(*callback_infolist)(void *data,
                                            const char *infolist_name,
                                            void *pointer,
                                            const char *arguments);
    int (*callback_signal_debug_dump)(void *data, const char *signal,
                                      const char *type_data,
                                      void *signal_data);
    int (*callback_signal_buffer_closed)(void *data, const char *signal,
                                         const char *type_data,
                                         void *signal_data);
    int (*callback_signal_script_action)(void *data, const char *signal,
                                         const char *type_data,
                                         void *signal_data);
    void (*callback_load_file)(void *data, const char *filename);
};

extern void plugin_script_init (struct t_weechat_plugin *weechat_plugin,
                                int argc, char *argv[],
                                struct t_plugin_script_init *init);
extern int plugin_script_valid (struct t_plugin_script *scripts,
                                struct t_plugin_script *script);
extern char *plugin_script_ptr2str (void *pointer);
extern void *plugin_script_str2ptr (struct t_weechat_plugin *weechat_plugin,
                                    const char *script_name,
                                    const char *function_name,
                                    const char *pointer_str);
extern void plugin_script_auto_load (struct t_weechat_plugin *weechat_plugin,
                                     void (*callback)(void *data, const char *filename));
extern struct t_plugin_script *plugin_script_search (struct t_weechat_plugin *weechat_plugin,
                                                     struct t_plugin_script *scripts,
                                                     const char *name);
extern char *plugin_script_search_path (struct t_weechat_plugin *weechat_plugin,
                                        const char *filename);
extern struct t_plugin_script *plugin_script_add (struct t_weechat_plugin *weechat_plugin,
                                                  struct t_plugin_script **scripts,
                                                  struct t_plugin_script **last_script,
                                                  const char *filename, const char *name,
                                                  const char *author, const char *version,
                                                  const char *license, const char *description,
                                                  const char *shutdown_func, const char *charset);
extern void plugin_script_set_buffer_callbacks (struct t_weechat_plugin *weechat_plugin,
                                                struct t_plugin_script *scripts,
                                                struct t_plugin_script *script,
                                                int (*callback_buffer_input) (void *data,
                                                                              struct t_gui_buffer *buffer,
                                                                              const char *input_data),
                                                int (*callback_buffer_close) (void *data,
                                                                              struct t_gui_buffer *buffer));
extern void plugin_script_remove_buffer_callbacks (struct t_plugin_script *scripts,
                                                   struct t_gui_buffer *buffer);
extern void plugin_script_remove (struct t_weechat_plugin *weechat_plugin,
                                  struct t_plugin_script **scripts,
                                  struct t_plugin_script **last_script,
                                  struct t_plugin_script *script);
extern void plugin_script_completion (struct t_weechat_plugin *weechat_plugin,
                                      struct t_gui_completion *completion,
                                      struct t_plugin_script *scripts);
extern void plugin_script_action_add (char **action_list, const char *name);
extern void plugin_script_action_install (struct t_weechat_plugin *weechat_plugin,
                                          struct t_plugin_script *scripts,
                                          void (*script_unload)(struct t_plugin_script *script),
                                          int (*script_load)(const char *filename),
                                          char **list);
extern void plugin_script_action_remove (struct t_weechat_plugin *weechat_plugin,
                                         struct t_plugin_script *scripts,
                                         void (*script_unload)(struct t_plugin_script *script),
                                         char **list);
extern void plugin_script_display_list (struct t_weechat_plugin *weechat_plugin,
                                        struct t_plugin_script *scripts,
                                        const char *name, int full);
extern void plugin_script_display_short_list (struct t_weechat_plugin *weechat_plugin,
                                              struct t_plugin_script *scripts);
extern int plugin_script_add_to_infolist (struct t_weechat_plugin *weechat_plugin,
                                          struct t_infolist *infolist,
                                          struct t_plugin_script *script);
extern struct t_infolist *plugin_script_infolist_list_scripts (struct t_weechat_plugin *weechat_plugin,
                                                               struct t_plugin_script *scripts,
                                                               void *pointer,
                                                               const char *arguments);
extern void plugin_script_end (struct t_weechat_plugin *weechat_plugin,
                               struct t_plugin_script **scripts,
                               void (*callback_unload_all)());
extern void plugin_script_print_log (struct t_weechat_plugin *weechat_plugin,
                                     struct t_plugin_script *scripts);

#endif /* __WEECHAT_PLUGIN_SCRIPT_H */
