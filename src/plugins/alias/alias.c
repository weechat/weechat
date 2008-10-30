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

/* alias.c: Alias plugin for WeeChat */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "alias.h"
#include "alias-info.h"


WEECHAT_PLUGIN_NAME(ALIAS_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Alias plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_WEECHAT_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL3");

struct t_weechat_plugin *weechat_alias_plugin = NULL;

struct t_config_file *alias_config_file = NULL;
struct t_config_section *alias_config_section_cmd = NULL;

struct t_alias *alias_list = NULL;
struct t_alias *last_alias = NULL;


/*
 * alias_valid: check if an alias pointer exists
 *              return 1 if alias exists
 *                     0 if alias is not found
 */

int
alias_valid (struct t_alias *alias)
{
    struct t_alias *ptr_alias;
    
    if (!alias)
        return 0;
    
    for (ptr_alias = alias_list; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        if (ptr_alias == alias)
            return 1;
    }
    
    /* alias not found */
    return 0;
}

/*
 * alias_search: search an alias
 */

struct t_alias *
alias_search (const char *alias_name)
{
    struct t_alias *ptr_alias;
    
    for (ptr_alias = alias_list; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        if (weechat_strcasecmp (alias_name, ptr_alias->name) == 0)
            return ptr_alias;
    }
    return NULL;
}

/*
 * alias_add_word: add word to string and increment length
 *                 This function should NOT be called directly.
 */

void
alias_add_word (char **alias, int *length, const char *word)
{
    int length_word;

    if (!word)
        return;
    
    length_word = strlen (word);
    if (length_word == 0)
        return;
    
    if (*alias == NULL)
    {
        *alias = malloc (length_word + 1);
        strcpy (*alias, word);
    }
    else
    {
        *alias = realloc (*alias, strlen (*alias) + length_word + 1);
        strcat (*alias, word);
    }
    *length += length_word;
}

/*
 * alias_replace_args: replace arguments ($1, $2, .. or $*) in alias arguments
 */

char *
alias_replace_args (const char *alias_args, const char *user_args)
{
    char **argv, *res, *word;
    const char *start, *pos;
    int argc, length_res, args_count;
    
    argv = weechat_string_explode (user_args, " ", 0, 0, &argc);
    
    res = NULL;
    length_res = 0;
    args_count = 0;
    start = alias_args;
    pos = start;
    while (pos && pos[0])
    {
        if ((pos[0] == '\\') && (pos[1] == '$'))
        {
            word = weechat_strndup (start, pos - start);
            if (word)
            {
                alias_add_word (&res, &length_res, word);
                free (word);
            }
            alias_add_word (&res, &length_res, "$");
            start = pos + 2;
            pos = start;
        }
        else
        {
            if (pos[0] == '$')
            {
                if (pos[1] == '*')
                {
                    args_count++;
                    if (pos > start)
                    {
                        word = weechat_strndup (start, pos - start);
                        if (word)
                        {
                            alias_add_word (&res, &length_res, word);
                            free (word);
                        }
                    }
                    alias_add_word (&res, &length_res, user_args);
                    start = pos + 2;
                    pos = start;
                }
                else
                {
                    if ((pos[1] >= '1') && (pos[1] <= '9'))
                    {
                        args_count++;
                        if (pos > start)
                        {
                            word = weechat_strndup (start, pos - start);
                            if (word)
                            {
                                alias_add_word (&res, &length_res, word);
                                free (word);
                            }
                        }
                        if (pos[1] - '1' < argc)
                            alias_add_word (&res, &length_res, argv[pos[1] - '1']);
                        start = pos + 2;
                        pos = start;
                    }
                    else
                        pos++;
                }
            }
            else
                pos++;
        }
    }
    
    if (start < pos)
        alias_add_word (&res, &length_res, start);
    
    if ((args_count == 0) && user_args && user_args[0])
    {
        alias_add_word (&res, &length_res, " ");
        alias_add_word (&res, &length_res, user_args);
    }
    
    if (argv)
        weechat_string_free_exploded (argv);
    
    return res;
}

/*
 * alias_run_command: replace local buffer variables in string, then run
 *                    command on buffer
 */

void
alias_run_command (struct t_gui_buffer *buffer, const char *command)
{
    char *string;
    
    string = weechat_buffer_string_replace_local_var (buffer, command);
    weechat_command (buffer,
                     (string) ? string : command);
    if (string)
        free (string);
}

/*
 * alias_cb: callback for alias (called when user uses an alias)
 */

int
alias_cb (void *data, struct t_gui_buffer *buffer, int argc, char **argv,
          char **argv_eol)
{
    struct t_alias *ptr_alias;
    char **commands, **ptr_cmd, **ptr_next_cmd;
    char *args_replaced, *alias_command;
    int some_args_replaced, length1, length2;
    
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    ptr_alias = (struct t_alias *)data;
    
    if (ptr_alias->running)
    {
        weechat_printf (NULL,
                        _("%s%s: error, circular reference when calling "
                          "alias \"%s\""),
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        ptr_alias->name);
        return WEECHAT_RC_ERROR;
    }
    else
    {		
        /* an alias can contain many commands separated by ';' */
        commands = weechat_string_split_command (ptr_alias->command, ';');
        if (commands)
        {
            some_args_replaced = 0;
            ptr_alias->running = 1;
            for (ptr_cmd = commands; *ptr_cmd; ptr_cmd++)
            {
                ptr_next_cmd = ptr_cmd;
                ptr_next_cmd++;

                args_replaced = (argc > 1) ?
                    alias_replace_args (*ptr_cmd, argv_eol[1]) : NULL;
                if (args_replaced)
                {
                    some_args_replaced = 1;
                    if (*ptr_cmd[0] == '/')
                        alias_run_command (buffer, args_replaced);
                    else
                    {
                        alias_command = malloc (1 + strlen(args_replaced) + 1);
                        if (alias_command)
                        {
                            strcpy (alias_command, "/");
                            strcat (alias_command, args_replaced);
                            alias_run_command (buffer, alias_command);
                            free (alias_command);
                        }
                    }
                    free (args_replaced);
                }
                else
                {
                    /* if alias has arguments, they are now
                       arguments of the last command in the list (if no $1,$2,..$*) was found */
                    if ((*ptr_next_cmd == NULL) && argv_eol[1] && (!some_args_replaced))
                    {
                        length1 = strlen (*ptr_cmd);
                        length2 = strlen (argv_eol[1]);
                        
                        alias_command = malloc (1 + length1 + 1 + length2 + 1);
                        if (alias_command)
                        {
                            if (*ptr_cmd[0] != '/')
                                strcpy (alias_command, "/");
                            else
                                strcpy (alias_command, "");
                            
                            strcat (alias_command, *ptr_cmd);
                            strcat (alias_command, " ");
                            strcat (alias_command, argv_eol[1]);
                            
                            alias_run_command (buffer, alias_command);
                            free (alias_command);
                        }
                    }
                    else
                    {
                        if (*ptr_cmd[0] == '/')
                            alias_run_command (buffer, *ptr_cmd);
                        else
                        {
                            alias_command = malloc (1 + strlen (*ptr_cmd) + 1);
                            if (alias_command)
                            {
                                strcpy (alias_command, "/");
                                strcat (alias_command, *ptr_cmd);
                                alias_run_command (buffer, alias_command);
                                free (alias_command);
                            }
                        }
                    }
                }
            }
            ptr_alias->running = 0;
            weechat_string_free_splitted_command (commands);
        }
    }
    return WEECHAT_RC_OK;
}

/*
 * alias_find_pos: find position for an alias (for sorting aliases)
 */

struct t_alias *
alias_find_pos (const char *name)
{
    struct t_alias *ptr_alias;
    
    for (ptr_alias = alias_list; ptr_alias; ptr_alias = ptr_alias->next_alias)
    {
        if (weechat_strcasecmp (name, ptr_alias->name) < 0)
            return ptr_alias;
    }
    
    /* position not found (we will add to the end of list) */
    return NULL;
}

/*
 * alias_new: create new alias and add it to alias list
 */

struct t_alias *
alias_new (const char *name, const char *command)
{
    struct t_alias *new_alias, *ptr_alias, *pos_alias;
    struct t_hook *new_hook;
    
    if (!name || !name[0] || !command || !command[0])
        return NULL;
    
    while (name[0] == '/')
    {
	name++;
    }
    
    ptr_alias = alias_search (name);
    if (ptr_alias)
    {
	if (ptr_alias->command)
	    free (ptr_alias->command);
	ptr_alias->command = strdup (command);
	return ptr_alias;
    }
    
    new_alias = malloc (sizeof (*new_alias));
    if (new_alias)
    {
        new_hook = weechat_hook_command (name, "[alias]", NULL, NULL, NULL,
                                         alias_cb, new_alias);
        if (!new_hook)
        {
            free (new_alias);
            return NULL;
        }
        
        new_alias->hook = new_hook;
        new_alias->name = strdup (name);
        new_alias->command = strdup (command);
	new_alias->running = 0;

        if (alias_list)
        {
            pos_alias = alias_find_pos (name);
            if (pos_alias)
            {
                /* insert alias into the list (before alias found) */
                new_alias->prev_alias = pos_alias->prev_alias;
                new_alias->next_alias = pos_alias;
                if (pos_alias->prev_alias)
                    (pos_alias->prev_alias)->next_alias = new_alias;
                else
                    alias_list = new_alias;
                pos_alias->prev_alias = new_alias;
            }
            else
            {
                /* add alias to end of list */
                new_alias->prev_alias = last_alias;
                new_alias->next_alias = NULL;
                last_alias->next_alias = new_alias;
                last_alias = new_alias;
            }
        }
        else
        {
            new_alias->prev_alias = NULL;
            new_alias->next_alias = NULL;
            alias_list = new_alias;
            last_alias = new_alias;
        }
    }
    
    return new_alias;
}

/*
 * alias_get_final_command: get final command pointed by an alias
 */

char *
alias_get_final_command (struct t_alias *alias)
{
    struct t_alias *ptr_alias;
    char *result;
    
    if (alias->running)
    {
        weechat_printf (NULL,
                        _("%s%s: error, circular reference when calling "
                          "alias \"%s\""),
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        alias->name);
        return NULL;
    }
    
    ptr_alias = alias_search ((alias->command[0] == '/') ?
                             alias->command + 1 : alias->command);
    if (ptr_alias)
    {
        alias->running = 1;
        result = alias_get_final_command (ptr_alias);
        alias->running = 0;
        return result;
    }
    return (alias->command[0] == '/') ?
        alias->command + 1 : alias->command;
}

/*
 * alias_free: free an alias and remove it from list
 */

void
alias_free (struct t_alias *alias)
{
    struct t_alias *new_alias_list;
    
    /* remove alias from list */
    if (last_alias == alias)
        last_alias = alias->prev_alias;
    if (alias->prev_alias)
    {
        (alias->prev_alias)->next_alias = alias->next_alias;
        new_alias_list = alias_list;
    }
    else
        new_alias_list = alias->next_alias;
    if (alias->next_alias)
        (alias->next_alias)->prev_alias = alias->prev_alias;

    /* free data */
    if (alias->hook)
        weechat_unhook (alias->hook);
    if (alias->name)
        free (alias->name);
    if (alias->command)
        free (alias->command);
    free (alias);
    
    alias_list = new_alias_list;
}

/*
 * alias_free_all: free all alias
 */

void
alias_free_all ()
{
    while (alias_list)
    {
        alias_free (alias_list);
    }
}

/*
 * alias_config_change_cb: callback called when alias option is modified
 */

void
alias_config_change_cb (void *data, struct t_config_option *option)
{
    struct t_alias *ptr_alias;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_alias = alias_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_alias)
        alias_free (ptr_alias);
    alias_new (weechat_config_option_get_pointer (option, "name"),
               weechat_config_option_get_pointer (option, "value"));
}

/*
 * alias_config_delete_cb: callback called when alias option is deleted
 */

void
alias_config_delete_cb (void *data, struct t_config_option *option)
{
    struct t_alias *ptr_alias;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_alias = alias_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_alias)
        alias_free (ptr_alias);
}

/*
 * alias_config_reload: reload alias configuration file
 */

int
alias_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;

    weechat_config_section_free_options (alias_config_section_cmd);
    alias_free_all ();
    
    return weechat_config_reload (config_file);
}

/*
 * alias_config_write_default: write default aliases in configuration file
 */

void
alias_config_write_default (void *data,
                            struct t_config_file *config_file,
                            const char *section_name)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_config_write_line (config_file, section_name, NULL);
    
    weechat_config_write_line (config_file, "BYE", "%s", "\"quit\"");
    weechat_config_write_line (config_file, "C", "%s", "\"buffer clear\"");
    weechat_config_write_line (config_file, "CL", "%s", "\"buffer clear\"");
    weechat_config_write_line (config_file, "CLOSE", "%s", "\"buffer close\"");
    weechat_config_write_line (config_file, "CHAT", "%s", "\"dcc chat\"");
    weechat_config_write_line (config_file, "EXIT", "%s", "\"quit\"");
    weechat_config_write_line (config_file, "IG", "%s", "\"ignore\"");
    weechat_config_write_line (config_file, "J", "%s", "\"join\"");
    weechat_config_write_line (config_file, "K", "%s", "\"kick\"");
    weechat_config_write_line (config_file, "KB", "%s", "\"kickban\"");
    weechat_config_write_line (config_file, "LEAVE", "%s", "\"part\"");
    weechat_config_write_line (config_file, "M", "%s", "\"msg\"");
    weechat_config_write_line (config_file, "MUB", "%s", "\"unban *\"");
    weechat_config_write_line (config_file, "N", "%s", "\"names\"");
    weechat_config_write_line (config_file, "Q", "%s", "\"query\"");
    weechat_config_write_line (config_file, "REDRAW", "%s", "\"window refresh\"");
    weechat_config_write_line (config_file, "SAY", "%s", "\"msg *\"");
    weechat_config_write_line (config_file, "SIGNOFF", "%s", "\"quit\"");
    weechat_config_write_line (config_file, "T", "%s", "\"topic\"");
    weechat_config_write_line (config_file, "UB", "%s", "\"unban\"");
    weechat_config_write_line (config_file, "UNIG", "%s", "\"unignore\"");
    weechat_config_write_line (config_file, "W", "%s", "\"who\"");
    weechat_config_write_line (config_file, "WC", "%s", "\"window merge\"");
    weechat_config_write_line (config_file, "WI", "%s", "\"whois\"");
    weechat_config_write_line (config_file, "WII", "%s", "\"whois $1 $1\"");
    weechat_config_write_line (config_file, "WW", "%s", "\"whowas\"");
}

/*
 * alias_config_create_option: create an alias
 */

int
alias_config_create_option (void *data, struct t_config_file *config_file,
                            struct t_config_section *section,
                            const char *option_name, const char *value)
{
    struct t_alias *ptr_alias;
    int rc;
    
    /* make C compiler happy */
    (void) data;
    
    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    
    /* create config option */
    weechat_config_new_option (
        config_file, section,
        option_name, "string", NULL,
        NULL, 0, 0, "", value, NULL, NULL,
        &alias_config_change_cb, NULL,
        &alias_config_delete_cb, NULL);
    
    /* create alias */
    ptr_alias = alias_search (option_name);
    if (ptr_alias)
        alias_free (ptr_alias);
    if (value && value[0])
        rc = (alias_new (option_name, value)) ?
            WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
    else
        rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    
    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        "%s%s: error creating alias \"%s\" => \"%s\"",
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        option_name, value);
    }
    
    return rc;
}

/*
 * alias_config_init: init alias configuration file
 *                    return: 1 if ok, 0 if error
 */

int
alias_config_init ()
{
    struct t_config_section *ptr_section;
    
    alias_config_file = weechat_config_new (ALIAS_CONFIG_NAME,
                                            &alias_config_reload, NULL);
    if (!alias_config_file)
        return 0;
    
    ptr_section = weechat_config_new_section (alias_config_file, "cmd",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &alias_config_write_default, NULL,
                                              &alias_config_create_option, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (alias_config_file);
        return 0;
    }
    
    alias_config_section_cmd = ptr_section;
    
    return 1;
}

/*
 * alias_config_read: read alias configuration file
 */

int
alias_config_read ()
{
    return weechat_config_read (alias_config_file);
}

/*
 * alias_config_write: write alias configuration file
 */

int
alias_config_write ()
{
    return weechat_config_write (alias_config_file);
}

/*
 * alias_command_cb: display or create alias
 */

int
alias_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                  char **argv, char **argv_eol)
{
    char *alias_name;
    struct t_alias *ptr_alias;
    struct t_config_option *ptr_option;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    
    if (argc > 1)
    {
        alias_name = (argv[1][0] == '/') ? argv[1] + 1 : argv[1];
        if (argc > 2)
        {
            /* Define new alias */
            if (!alias_new (alias_name, argv_eol[2]))
            {
                weechat_printf (NULL,
                                _("%s%s: error creating alias \"%s\" "
                                  "=> \"%s\""),
                                weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                                alias_name, argv_eol[2]);
                return WEECHAT_RC_ERROR;
            }
            
            /* create config option */
            ptr_option = weechat_config_search_option (alias_config_file,
                                                       alias_config_section_cmd,
                                                       alias_name);
            if (ptr_option)
                weechat_config_option_free (ptr_option);
            weechat_config_new_option (
                alias_config_file, alias_config_section_cmd,
                alias_name, "string", NULL,
                NULL, 0, 0, "", argv_eol[2], NULL, NULL,
                &alias_config_change_cb, NULL,
                &alias_config_delete_cb, NULL);
            
            weechat_printf (NULL,
                            _("Alias \"%s\" => \"%s\" created"),
                            alias_name, argv_eol[2]);
        }
        else
        {
            /* Display one alias */
            ptr_alias = alias_search (alias_name);
	    if (ptr_alias)
	    {
		weechat_printf (NULL, "");
		weechat_printf (NULL, _("Alias:"));
		weechat_printf (NULL, "  %s %s=>%s %s",
                                ptr_alias->name,
                                weechat_color ("chat_delimiters"),
                                weechat_color ("chat"),
                                ptr_alias->command);
	    }
            else
                weechat_printf (NULL,
                                _("No alias found"));
        }
    }
    else
    {
        /* List all aliases */
        if (alias_list)
        {
            weechat_printf (NULL, "");
            weechat_printf (NULL, _("List of aliases:"));
            for (ptr_alias = alias_list; ptr_alias;
                 ptr_alias = ptr_alias->next_alias)
            {
                weechat_printf (NULL,
                                "  %s %s=>%s %s",
                                ptr_alias->name,
                                weechat_color ("chat_delimiters"),
                                weechat_color ("chat"),
                                ptr_alias->command);
            }
        }
        else
            weechat_printf (NULL, _("No alias defined"));
    }
    
    return WEECHAT_RC_OK;
}

/*
 * unalias_command_cb: remove an alias
 */

int
unalias_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                    char **argv, char **argv_eol)
{
    char *alias_name;
    struct t_alias *ptr_alias;
    struct t_config_option *ptr_option;
    
    /* make C compiler happy */
    (void) data;
    (void) buffer;
    (void) argv_eol;
    
    if (argc > 1)
    {
        alias_name = (argv[1][0] == '/') ? argv[1] + 1 : argv[1];
        ptr_alias = alias_search (alias_name);
        if (!ptr_alias)
        {
            weechat_printf (NULL,
                            _("%sAlias \"%s\" not found"),
                            weechat_prefix ("error"),
                            alias_name);
            return WEECHAT_RC_ERROR;
        }
        
        /* remove alias */
        alias_free (ptr_alias);
        
        /* remove option */
        ptr_option = weechat_config_search_option (alias_config_file,
                                                   alias_config_section_cmd,
                                                   alias_name);
        if (ptr_option)
            weechat_config_option_free (ptr_option);
        
        weechat_printf (NULL,
                        _("Alias \"%s\" removed"),
                        alias_name);
    }
    return WEECHAT_RC_OK;
}

/*
 * alias_completion_cb: callback for completion with list of aliases
 */

int
alias_completion_cb (void *data, const char *completion_item,
                     struct t_gui_buffer *buffer,
                     struct t_gui_completion *completion)
{
    struct t_alias *ptr_alias;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_alias = alias_list; ptr_alias;
         ptr_alias = ptr_alias->next_alias)
    {
        weechat_hook_completion_list_add (completion, ptr_alias->name,
                                          0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * alias_add_to_infolist: add an alias in an infolist
 *                        return 1 if ok, 0 if error
 */

int
alias_add_to_infolist (struct t_infolist *infolist, struct t_alias *alias)
{
    struct t_infolist_item *ptr_item;
    
    if (!infolist || !alias)
        return 0;
    
    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;
    
    if (!weechat_infolist_new_var_pointer (ptr_item, "hook", alias->hook))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "name", alias->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "command", alias->command))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "running", alias->running))
        return 0;
    
    return 1;
}

/*
 * weechat_plugin_init: initialize alias plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    weechat_plugin = plugin;
    
    if (!alias_config_init ())
    {
        weechat_printf (NULL,
                        "%s%s: error creating configuration file",
                        weechat_prefix("error"), ALIAS_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }
    alias_config_read ();
    
    weechat_hook_command ("alias",
                          N_("create an alias for a command"),
                          N_("[alias_name [command [arguments]]]"),
                          N_("alias_name: name of alias\n"
                             "   command: command name (many commands can be "
                             "separated by semicolons)\n"
                             " arguments: arguments for command\n\n"
                             "Note: in command, special variables "
                             "$1, $2,..,$9 are replaced by arguments given "
                             "by user, and $* is replaced by all arguments.\n"
                             "Variables $nick, $channel and $server are "
                             "replaced by current nick/channel/server."),
                          "%(alias) %h",
                          &alias_command_cb, NULL);
    
    weechat_hook_command ("unalias", N_("remove an alias"),
                          N_("alias_name"),
                          N_("alias_name: name of alias to remove"),
                          "%(alias)",
                          &unalias_command_cb, NULL);
    
    weechat_hook_completion ("alias", &alias_completion_cb, NULL);
    
    alias_info_init ();
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end alias plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    alias_config_write ();
    alias_free_all ();    
    weechat_config_free (alias_config_file);
    
    return WEECHAT_RC_OK;
}
