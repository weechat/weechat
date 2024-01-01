/*
 * Copyright (C) 2014-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_TRIGGER_H
#define WEECHAT_PLUGIN_TRIGGER_H

#include <regex.h>

#define weechat_plugin weechat_trigger_plugin
#define TRIGGER_PLUGIN_NAME "trigger"
#define TRIGGER_PLUGIN_PRIORITY 13000

#define TRIGGER_HOOK_DEFAULT_CONDITIONS "${...}"
#define TRIGGER_HOOK_DEFAULT_REGEX      "/abc/def"
#define TRIGGER_HOOK_DEFAULT_COMMAND    "/cmd"

enum t_trigger_option
{
    TRIGGER_OPTION_ENABLED = 0,        /* true if trigger is enabled        */
    TRIGGER_OPTION_HOOK,               /* hook (signal, modifier, ...)      */
    TRIGGER_OPTION_ARGUMENTS,          /* arguments for hook                */
    TRIGGER_OPTION_CONDITIONS,         /* conditions for trigger            */
    TRIGGER_OPTION_REGEX,              /* replace text with 1 or more regex */
    TRIGGER_OPTION_COMMAND,            /* command run if conditions are OK  */
    TRIGGER_OPTION_RETURN_CODE,        /* return code for hook callback     */
    TRIGGER_OPTION_POST_ACTION,        /* action to take after execution    */
    /* number of trigger options */
    TRIGGER_NUM_OPTIONS,
};

enum t_trigger_hook_type
{
    TRIGGER_HOOK_SIGNAL = 0,
    TRIGGER_HOOK_HSIGNAL,
    TRIGGER_HOOK_MODIFIER,
    TRIGGER_HOOK_LINE,
    TRIGGER_HOOK_PRINT,
    TRIGGER_HOOK_COMMAND,
    TRIGGER_HOOK_COMMAND_RUN,
    TRIGGER_HOOK_TIMER,
    TRIGGER_HOOK_CONFIG,
    TRIGGER_HOOK_FOCUS,
    TRIGGER_HOOK_INFO,
    TRIGGER_HOOK_INFO_HASHTABLE,
    /* number of hook types */
    TRIGGER_NUM_HOOK_TYPES,
};

enum t_trigger_return_code
{
    TRIGGER_RC_OK = 0,
    TRIGGER_RC_OK_EAT,
    TRIGGER_RC_ERROR,
    /* number of return codes */
    TRIGGER_NUM_RETURN_CODES,
};

enum t_trigger_post_action
{
    TRIGGER_POST_ACTION_NONE = 0,
    TRIGGER_POST_ACTION_DISABLE,
    TRIGGER_POST_ACTION_DELETE,
    /* number of post actions */
    TRIGGER_NUM_POST_ACTIONS,
};

enum t_trigger_regex_command
{
    TRIGGER_REGEX_COMMAND_REPLACE = 0,
    TRIGGER_REGEX_COMMAND_TRANSLATE_CHARS,
    /* number of regex commands */
    TRIGGER_NUM_REGEX_COMMANDS,
};

struct t_trigger_regex
{
    enum t_trigger_regex_command command; /* regex command                  */
    char *variable;                    /* the hashtable key used            */
    char *str_regex;                   /* regex to search for replacement   */
    regex_t *regex;                    /* compiled regex                    */
    char *replace;                     /* replacement text                  */
    char *replace_escaped;             /* repl. text (with chars escaped)   */
};

struct t_trigger
{
    /* user choices */
    char *name;                        /* trigger name                      */
    struct t_config_option *options[TRIGGER_NUM_OPTIONS];

    /* internal vars */

    /* hooks */
    int hooks_count;                   /* number of hooks                   */
    struct t_hook **hooks;             /* array of hooks (signal, ...)      */
    unsigned long long hook_count_cb;  /* number of calls made to callback  */
    unsigned long long hook_count_cmd; /* number of commands run in callback*/
    int hook_running;                  /* 1 if one hook callback is running */
    char *hook_print_buffers;          /* buffers (for hook_print only)     */

    /* regular expressions */
    int regex_count;                   /* number of regex                   */
    struct t_trigger_regex *regex;     /* array of regex                    */

    /* commands */
    int commands_count;                /* number of commands                */
    char **commands;                   /* commands                          */

    /* links to other triggers */
    struct t_trigger *prev_trigger;    /* link to previous trigger          */
    struct t_trigger *next_trigger;    /* link to next trigger              */
};

extern struct t_weechat_plugin *weechat_trigger_plugin;
extern char *trigger_option_string[];
extern char *trigger_option_default[];
extern char *trigger_hook_type_string[];
extern char *trigger_hook_option_values;
extern char *trigger_hook_default_arguments[];
extern char *trigger_hook_default_rc[];
extern char trigger_regex_command[];
extern char *trigger_hook_regex_default_var[];
extern char *trigger_return_code_string[];
extern int trigger_return_code[];
extern char *trigger_post_action_string[];
extern struct t_trigger *triggers;
extern struct t_trigger *last_trigger;
extern int triggers_count;
extern struct t_trigger *triggers_temp;
extern struct t_trigger *last_trigger_temp;
extern int trigger_enabled;

extern int trigger_search_option (const char *option_name);
extern int trigger_search_hook_type (const char *type);
extern int trigger_search_regex_command (char command);
extern int trigger_search_return_code (const char *return_code);
extern int trigger_search_post_action (const char *post_action);
extern struct t_trigger *trigger_search (const char *name);
extern struct t_trigger *trigger_search_with_option (struct t_config_option *option);
extern void trigger_regex_free (int *regex_count,
                                struct t_trigger_regex **regex);
extern int trigger_regex_split (const char *str_regex,
                                int *regex_count,
                                struct t_trigger_regex **regex);
extern void trigger_split_command (const char *command,
                                   int *commands_count, char ***commands);
extern void trigger_unhook (struct t_trigger *trigger);
extern void trigger_hook (struct t_trigger *trigger);
extern int trigger_name_valid (const char *name);
extern struct t_trigger *trigger_alloc (const char *name);
extern void trigger_add (struct t_trigger *trigger,
                         struct t_trigger **list_triggers,
                         struct t_trigger **last_list_trigger);
extern struct t_trigger *trigger_new_with_options (const char *name,
                                                   struct t_config_option **options);
extern struct t_trigger *trigger_new (const char *name,
                                      const char *enabled,
                                      const char *hook,
                                      const char *arguments,
                                      const char *conditions,
                                      const char *replace,
                                      const char *command,
                                      const char *return_code,
                                      const char *post_action);
extern void trigger_create_default ();
extern int trigger_rename (struct t_trigger *trigger, const char *name);
extern struct t_trigger *trigger_copy (struct t_trigger *trigger,
                                       const char *name);
extern void trigger_free (struct t_trigger *trigger);
extern void trigger_free_all ();

#endif /* WEECHAT_PLUGIN_TRIGGER_H */
