/*
 * alias-config.c - alias configuration options (file alias.conf)
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "alias.h"
#include "alias-config.h"


struct t_config_file *alias_config_file = NULL;
struct t_config_section *alias_config_section_cmd = NULL;
struct t_config_section *alias_config_section_completion = NULL;

char *alias_default[][3] =
{ { "aaway",   "allserv /away",        NULL            },
  { "anick",   "allserv /nick",        NULL            },
  { "beep",    "print -beep",          NULL            },
  { "bye",     "quit",                 NULL            },
  { "c",       "buffer clear",         NULL            },
  { "cl",      "buffer clear",         NULL            },
  { "close",   "buffer close",         NULL            },
  { "chat",    "dcc chat",             NULL            },
  { "exit",    "quit",                 NULL            },
  { "ig",      "ignore",               NULL            },
  { "j",       "join",                 NULL            },
  { "k",       "kick",                 NULL            },
  { "kb",      "kickban",              NULL            },
  { "leave",   "part",                 NULL            },
  { "m",       "msg",                  NULL            },
  { "mub",     "unban *",              NULL            },
  { "msgbuf",  "command -buffer $1 * /input send $2-",
    "%(buffers_plugins_names)"                         },
  { "n",       "names",                NULL            },
  { "q",       "query",                NULL            },
  { "redraw",  "window refresh",       NULL            },
  { "say",     "msg *",                NULL            },
  { "signoff", "quit",                 NULL            },
  { "t",       "topic",                NULL            },
  { "ub",      "unban",                NULL            },
  { "umode",   "mode $nick",           NULL            },
  { "v",       "command core version", NULL            },
  { "w",       "who",                  NULL            },
  { "wc",      "window close",         NULL            },
  { "wi",      "whois",                NULL            },
  { "wii",     "whois $1 $1",          NULL            },
  { "wm",      "window merge",         NULL            },
  { "ww",      "whowas",               NULL            },
  { NULL,      NULL,                   NULL            },
};

/*
 * Callback for changes on options in section "cmd" (command).
 */

void
alias_config_cmd_change_cb (const void *pointer, void *data,
                            struct t_config_option *option)
{
    struct t_config_option *ptr_option_completion;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_option_completion = weechat_config_search_option (alias_config_file,
                                                          alias_config_section_completion,
                                                          weechat_config_option_get_pointer (option, "name"));

    alias_new (weechat_config_option_get_pointer (option, "name"),
               weechat_config_option_get_pointer (option, "value"),
               (ptr_option_completion) ?
               weechat_config_option_get_pointer (ptr_option_completion, "value") : NULL);
}

/*
 * Callback called when an option is deleted in section "cmd" (command).
 */

void
alias_config_cmd_delete_cb (const void *pointer, void *data,
                            struct t_config_option *option)
{
    struct t_config_option *ptr_option_completion;
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_option_completion = weechat_config_search_option (alias_config_file,
                                                          alias_config_section_completion,
                                                          weechat_config_option_get_pointer (option, "name"));

    ptr_alias = alias_search (weechat_config_option_get_pointer (option, "name"));
    alias_free (ptr_alias);

    weechat_config_option_free (ptr_option_completion);
}

/*
 * Callback for changes on options in section "completion".
 */

void
alias_config_completion_change_cb (const void *pointer, void *data,
                                   struct t_config_option *option)
{
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_alias = alias_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_alias && ptr_alias->hook)
    {
        alias_update_completion (ptr_alias,
                                 weechat_config_option_get_pointer (option, "value"));
    }
}

/*
 * Callback called when an option is deleted in section "completion".
 */

void
alias_config_completion_delete_cb (const void *pointer, void *data,
                                   struct t_config_option *option)
{
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_alias = alias_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_alias && ptr_alias->hook && ptr_alias->completion)
    {
        alias_update_completion (ptr_alias, NULL);
    }
}

/*
 * Reloads alias configuration file.
 */

int
alias_config_reload (const void *pointer, void *data,
                     struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    weechat_config_section_free_options (alias_config_section_cmd);
    weechat_config_section_free_options (alias_config_section_completion);
    alias_free_all ();

    return weechat_config_reload (config_file);
}

/*
 * Writes default aliases in configuration file in section "cmd" (command).
 */

int
alias_config_cmd_write_default_cb (const void *pointer, void *data,
                                   struct t_config_file *config_file,
                                   const char *section_name)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!weechat_config_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (i = 0; alias_default[i][0]; i++)
    {
        if (!weechat_config_write_line (config_file,
                                        alias_default[i][0],
                                        "\"%s\"", alias_default[i][1]))
            return WEECHAT_CONFIG_WRITE_ERROR;
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Creates a new option in section "cmd" (command).
 */

void
alias_config_cmd_new_option (const char *name, const char *command)
{
    weechat_config_new_option (alias_config_file, alias_config_section_cmd,
                               name, "string", NULL,
                               NULL, 0, 0, NULL, command, 0,
                               NULL, NULL, NULL,
                               &alias_config_cmd_change_cb, NULL, NULL,
                               &alias_config_cmd_delete_cb, NULL, NULL);
}

/*
 * Callback called when an option is created in section "cmd" (command).
 */

int
alias_config_cmd_create_option_cb (const void *pointer, void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   const char *option_name, const char *value)
{
    struct t_alias *ptr_alias;
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    /* create configuration option */
    alias_config_cmd_new_option (option_name, value);

    /* create alias */
    ptr_alias = alias_search (option_name);
    alias_free (ptr_alias);
    if (value && value[0])
        rc = (alias_new (option_name, value, NULL)) ?
            WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
    else
        rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating alias \"%s\" => \"%s\""),
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        option_name, value);
    }

    return rc;
}

/*
 * Writes default completions in configuration file in section "completion".
 */

int
alias_config_completion_write_default_cb (const void *pointer, void *data,
                                          struct t_config_file *config_file,
                                          const char *section_name)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!weechat_config_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (i = 0; alias_default[i][0]; i++)
    {
        if (alias_default[i][2])
        {
            if (!weechat_config_write_line (config_file,
                                            alias_default[i][0],
                                            "\"%s\"", alias_default[i][2]))
                return WEECHAT_CONFIG_WRITE_ERROR;
        }
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Creates a new option in section "completion".
 */

void
alias_config_completion_new_option (const char *name, const char *completion)
{
    weechat_config_new_option (alias_config_file,
                               alias_config_section_completion,
                               name, "string", NULL,
                               NULL, 0, 0, NULL, completion, 0,
                               NULL, NULL, NULL,
                               &alias_config_completion_change_cb, NULL, NULL,
                               &alias_config_completion_delete_cb, NULL, NULL);
}

/*
 * Callback called when an option is created in section "completion".
 */

int
alias_config_completion_create_option_cb (const void *pointer, void *data,
                                          struct t_config_file *config_file,
                                          struct t_config_section *section,
                                          const char *option_name,
                                          const char *value)
{
    struct t_alias *ptr_alias;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    ptr_alias = alias_search (option_name);
    if (!ptr_alias)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating completion for alias \"%s\": "
                          "alias not found"),
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        option_name);
        return WEECHAT_CONFIG_OPTION_SET_ERROR;
    }

    /* create configuration option */
    alias_config_completion_new_option (option_name, value);

    /* create/update completion in alias */
    alias_update_completion (ptr_alias, value);

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Updates options in configuration file while reading the file.
 */

struct t_hashtable *
alias_config_update_cb (const void *pointer, void *data,
                        struct t_config_file *config_file,
                        int version_read,
                        struct t_hashtable *data_read)
{
    const char *ptr_section, *ptr_option;
    char *new_option;
    int changes;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;

    /* nothing to do if the config file is already up-to-date */
    if (version_read >= ALIAS_CONFIG_VERSION)
        return NULL;

    changes = 0;

    if (version_read < 2)
    {
        /*
         * changes in v2 (WeeChat 4.0.0):
         *   - aliases are in lower case by default
         *     (default aliases and those created by users are automatically
         *     converted to lower case)
         */
        ptr_section = weechat_hashtable_get (data_read, "section");
        ptr_option = weechat_hashtable_get (data_read, "option");
        if (ptr_section
            && ptr_option
            && ((strcmp (ptr_section, "cmd") == 0)
                || (strcmp (ptr_section, "completion") == 0)))
        {
            /* convert alias name to lower case */
            new_option = weechat_string_tolower (ptr_option);
            if (new_option)
            {
                if (strcmp (ptr_option, new_option) != 0)
                {
                    if (strcmp (ptr_section, "cmd") == 0)
                    {
                        /* display message only for alias, not for completion */
                        weechat_printf (
                            NULL,
                            _("Alias converted to lower case: \"%s\" => \"%s\""),
                            ptr_option, new_option);
                    }
                    weechat_hashtable_set (data_read, "option", new_option);
                    changes++;
                }
                free (new_option);
            }
        }
    }

    return (changes) ? data_read : NULL;
}

/*
 * Initializes alias configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
alias_config_init ()
{
    alias_config_file = weechat_config_new (ALIAS_CONFIG_PRIO_NAME,
                                            &alias_config_reload, NULL, NULL);
    if (!alias_config_file)
        return 0;

    if (!weechat_config_set_version (alias_config_file, ALIAS_CONFIG_VERSION,
                                     &alias_config_update_cb, NULL, NULL))
    {
        weechat_config_free (alias_config_file);
        alias_config_file = NULL;
        return 0;
    }

    /* cmd */
    alias_config_section_cmd = weechat_config_new_section (
        alias_config_file, "cmd",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &alias_config_cmd_write_default_cb, NULL, NULL,
        &alias_config_cmd_create_option_cb, NULL, NULL,
        NULL, NULL, NULL);

    /* completion */
    alias_config_section_completion = weechat_config_new_section (
        alias_config_file, "completion",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &alias_config_completion_write_default_cb, NULL, NULL,
        &alias_config_completion_create_option_cb, NULL, NULL,
        NULL, NULL, NULL);

    return 1;
}

/*
 * Reads alias configuration file.
 */

int
alias_config_read ()
{
    return weechat_config_read (alias_config_file);
}

/*
 * Writes alias configuration file.
 */

int
alias_config_write ()
{
    return weechat_config_write (alias_config_file);
}
