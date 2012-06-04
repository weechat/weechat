/*
 * Copyright (C) 2010-2012 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * rmodifier.c: alter modifier strings with regular expressions
 *              (useful for hiding passwords in input or command history)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "../weechat-plugin.h"
#include "rmodifier.h"
#include "rmodifier-command.h"
#include "rmodifier-completion.h"
#include "rmodifier-config.h"
#include "rmodifier-debug.h"
#include "rmodifier-info.h"


WEECHAT_PLUGIN_NAME(RMODIFIER_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Regex modifier plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_rmodifier_plugin = NULL;

struct t_rmodifier *rmodifier_list = NULL;
struct t_rmodifier *last_rmodifier = NULL;
int rmodifier_count = 0;
struct t_weelist *rmodifier_hook_list = NULL;


/*
 * rmodifier_valid: check if a rmodifier pointer exists
 *                  return 1 if rmodifier exists
 *                         0 if rmodifier is not found
 */

int
rmodifier_valid (struct t_rmodifier *rmodifier)
{
    struct t_rmodifier *ptr_rmodifier;

    if (!rmodifier)
        return 0;

    for (ptr_rmodifier = rmodifier_list; ptr_rmodifier;
         ptr_rmodifier = ptr_rmodifier->next_rmodifier)
    {
        if (ptr_rmodifier == rmodifier)
            return 1;
    }

    /* rmodifier not found */
    return 0;
}

/*
 * rmodifier_search: search a rmodifier
 */

struct t_rmodifier *
rmodifier_search (const char *name)
{
    struct t_rmodifier *ptr_rmodifier;

    for (ptr_rmodifier = rmodifier_list; ptr_rmodifier;
         ptr_rmodifier = ptr_rmodifier->next_rmodifier)
    {
        if (strcmp (name, ptr_rmodifier->name) == 0)
            return ptr_rmodifier;
    }
    return NULL;
}

/*
 * rmodifier_hide_string: hide a string (using char defined in option
 *                        "rmodifier.look.hide_char")
 */

char *
rmodifier_hide_string (const char *string)
{
    int length, i;
    char *result;

    if (!string || !string[0])
        return NULL;

    length = weechat_utf8_strlen (string);
    result = malloc ((length * strlen (weechat_config_string (rmodifier_config_look_hide_char))) + 1);
    if (!result)
        return NULL;

    result[0] = '\0';
    for (i = 0; i < length; i++)
    {
        strcat (result, weechat_config_string (rmodifier_config_look_hide_char));
    }

    return result;
}

/*
 * rmodifier_replace_groups: replace groups in a string, using regex_match
 *                           found by call to regexec()
 */

char *
rmodifier_replace_groups (const char *string, regmatch_t regex_match[],
                          const char *groups)
{
    char *result, *result2, *str_group, *string_to_add;
    const char *ptr_groups;
    int length, num_group;

    length = 1;
    result = malloc (length);
    if (!result)
        return NULL;

    result[0] = '\0';
    ptr_groups = groups;
    while (ptr_groups && ptr_groups[0])
    {
        if ((ptr_groups[0] >= '1') && (ptr_groups[0] <= '9'))
        {
            num_group = ptr_groups[0] - '0';
            if (regex_match[num_group].rm_so >= 0)
            {
                str_group = weechat_strndup (string + regex_match[num_group].rm_so,
                                             regex_match[num_group].rm_eo -regex_match[num_group].rm_so);
                if (str_group)
                {
                    string_to_add = NULL;
                    if (ptr_groups[1] == '*')
                        string_to_add = rmodifier_hide_string (str_group);
                    else
                        string_to_add = strdup (str_group);

                    if (string_to_add)
                    {
                        length += strlen (string_to_add);
                        result2 = realloc (result, length);
                        if (!result2)
                        {
                            if (result)
                                free (result);
                            return NULL;
                        }
                        result = result2;
                        strcat (result, string_to_add);
                        free (string_to_add);
                    }
                    free (str_group);
                }
            }
        }
        ptr_groups = weechat_utf8_next_char (ptr_groups);
    }

    return result;
}

/*
 * rmodifier_modifier_cb: callback for a modifier
 */

char *
rmodifier_modifier_cb (void *data, const char *modifier,
                       const char *modifier_data, const char *string)
{
    struct t_rmodifier *rmodifier;
    regmatch_t regex_match[9];
    int i;

    /* make C compiler happy */
    (void) modifier;
    (void) modifier_data;

    rmodifier = (struct t_rmodifier *)data;

    /* reset matching groups */
    for (i = 0; i < 9; i++)
    {
        regex_match[i].rm_so = -1;
    }

    /* execute regex and return modified string if matching */
    if (regexec (rmodifier->regex, string, 9, regex_match, 0) == 0)
    {
        return rmodifier_replace_groups (string, regex_match,
                                         rmodifier->groups);
    }

    /* regex not matching */
    return NULL;
}

/*
 * rmodifier_hook_modifiers: hook modifiers for a rmodifier
 */

void
rmodifier_hook_modifiers (struct t_rmodifier *rmodifier)
{
    char **argv, str_modifier[128];
    int argc, i;

    argv = weechat_string_split (rmodifier->modifiers, ",", 0, 0, &argc);

    if (argv)
    {
        rmodifier->hooks = malloc (sizeof (*rmodifier->hooks) * argc);
        if (rmodifier->hooks)
        {
            for (i = 0; i < argc; i++)
            {
                /*
                 * we use a high priority here, so that other modifiers
                 * (from other plugins) will be called after this one
                 */
                snprintf (str_modifier, sizeof (str_modifier) - 1,
                          "5000|%s", argv[i]);
                rmodifier->hooks[i] = weechat_hook_modifier (str_modifier,
                                                             &rmodifier_modifier_cb,
                                                             rmodifier);
            }
            rmodifier->hooks_count = argc;
        }
        weechat_string_free_split (argv);
    }
}

/*
 * rmodifier_new: create new rmodifier and add it to rmodifier list
 */

struct t_rmodifier *
rmodifier_new (const char *name, const char *modifiers, const char *str_regex,
               const char *groups)
{
    struct t_rmodifier *new_rmodifier, *ptr_rmodifier;
    regex_t *regex;

    if (!name || !name[0] || !modifiers || !modifiers[0]
        || !str_regex || !str_regex[0])
    {
        return NULL;
    }

    regex = malloc (sizeof (*regex));
    if (!regex)
        return NULL;

    if (weechat_string_regcomp (regex, str_regex,
                                REG_EXTENDED | REG_ICASE) != 0)
    {
        weechat_printf (NULL,
                        _("%s%s: error compiling regular expression \"%s\""),
                        weechat_prefix ("error"), RMODIFIER_PLUGIN_NAME,
                        str_regex);
        free (regex);
        return NULL;
    }

    ptr_rmodifier = rmodifier_search (name);
    if (ptr_rmodifier)
        rmodifier_free (ptr_rmodifier);

    new_rmodifier = malloc (sizeof (*new_rmodifier));
    if (new_rmodifier)
    {
        new_rmodifier->name = strdup (name);
        new_rmodifier->hooks = NULL;
        new_rmodifier->modifiers = strdup (modifiers);
        new_rmodifier->str_regex = strdup (str_regex);
        new_rmodifier->regex = regex;
        new_rmodifier->groups = strdup ((groups) ? groups : "");

        /* create modifiers */
        rmodifier_hook_modifiers (new_rmodifier);

        if (rmodifier_list)
        {
            /* add rmodifier to end of list */
            new_rmodifier->prev_rmodifier = last_rmodifier;
            new_rmodifier->next_rmodifier = NULL;
            last_rmodifier->next_rmodifier = new_rmodifier;
            last_rmodifier = new_rmodifier;
        }
        else
        {
            new_rmodifier->prev_rmodifier = NULL;
            new_rmodifier->next_rmodifier = NULL;
            rmodifier_list = new_rmodifier;
            last_rmodifier = new_rmodifier;
        }

        rmodifier_count++;
    }

    return new_rmodifier;
}

/*
 * rmodifier_new_with_string: create a rmodifier with a single string, which
 *                            contains: "modifiers;regex;groups"
 */

struct t_rmodifier *
rmodifier_new_with_string (const char *name, const char *value)
{
    struct t_rmodifier *new_rmodifier;
    char *pos1, *pos2, *modifiers, *str_regex;

    new_rmodifier = NULL;

    pos1 = strchr (value, ';');
    pos2 = rindex (value, ';');
    if (pos1 && pos2 && (pos2 > pos1))
    {
        modifiers = weechat_strndup (value, pos1 - value);
        str_regex = weechat_strndup (pos1 + 1, pos2 - (pos1 + 1));
        if (modifiers && str_regex)
        {
            new_rmodifier = rmodifier_new (name,
                                           modifiers, str_regex, pos2 + 1);
        }
        if (modifiers)
            free (modifiers);
        if (str_regex)
            free (str_regex);
    }

    return new_rmodifier;
}

/*
 * rmodifer_create_default: create default rmodifiers
 */

void
rmodifier_create_default ()
{
    int i;

    for (i = 0; rmodifier_config_default_list[i][0]; i++)
    {
        if (rmodifier_new (rmodifier_config_default_list[i][0],
                           rmodifier_config_default_list[i][1],
                           rmodifier_config_default_list[i][2],
                           rmodifier_config_default_list[i][3]))
        {
            rmodifier_config_modifier_new_option (rmodifier_config_default_list[i][0],
                                                  rmodifier_config_default_list[i][1],
                                                  rmodifier_config_default_list[i][2],
                                                  rmodifier_config_default_list[i][3]);
        }
    }
}

/*
 * rmodifier_free: free a rmodifier and remove it from list
 */

void
rmodifier_free (struct t_rmodifier *rmodifier)
{
    struct t_rmodifier *new_rmodifier_list;
    int i;

    /* remove rmodifier from list */
    if (last_rmodifier == rmodifier)
        last_rmodifier = rmodifier->prev_rmodifier;
    if (rmodifier->prev_rmodifier)
    {
        (rmodifier->prev_rmodifier)->next_rmodifier = rmodifier->next_rmodifier;
        new_rmodifier_list = rmodifier_list;
    }
    else
        new_rmodifier_list = rmodifier->next_rmodifier;
    if (rmodifier->next_rmodifier)
        (rmodifier->next_rmodifier)->prev_rmodifier = rmodifier->prev_rmodifier;

    /* free data */
    if (rmodifier->name)
        free (rmodifier->name);
    if (rmodifier->modifiers)
        free (rmodifier->modifiers);
    if (rmodifier->hooks)
    {
        for (i = 0; i < rmodifier->hooks_count; i++)
        {
            weechat_unhook (rmodifier->hooks[i]);
        }
        free (rmodifier->hooks);
    }
    if (rmodifier->str_regex)
        free (rmodifier->str_regex);
    if (rmodifier->regex)
    {
        regfree (rmodifier->regex);
        free (rmodifier->regex);
    }
    if (rmodifier->groups)
        free (rmodifier->groups);
    free (rmodifier);

    rmodifier_count--;

    rmodifier_list = new_rmodifier_list;
}

/*
 * rmodifier_free_all: free all rmodifier
 */

void
rmodifier_free_all ()
{
    while (rmodifier_list)
    {
        rmodifier_free (rmodifier_list);
    }
}

/*
 * rmodifier_add_to_infolist: add a rmodifier in an infolist
 *                            return 1 if ok, 0 if error
 */

int
rmodifier_add_to_infolist (struct t_infolist *infolist,
                           struct t_rmodifier *rmodifier)
{
    struct t_infolist_item *ptr_item;
    char option_name[64];
    int i;

    if (!infolist || !rmodifier)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "name", rmodifier->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "modifiers", rmodifier->modifiers))
        return 0;
    for (i = 0; i < rmodifier->hooks_count; i++)
    {
        snprintf (option_name, sizeof (option_name), "hook_%05d", i + 1);
        if (!weechat_infolist_new_var_pointer (ptr_item, option_name,
                                       rmodifier->hooks[i]))
            return 0;
    }
    if (!weechat_infolist_new_var_integer (ptr_item, "hooks_count", rmodifier->hooks_count))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "str_regex", rmodifier->str_regex))
        return 0;
    if (!weechat_infolist_new_var_pointer (ptr_item, "regex", rmodifier->regex))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "groups", rmodifier->groups))
        return 0;

    return 1;
}

/*
 * rmodifier_print_log: print rmodifiers in log (usually for crash dump)
 */

void
rmodifier_print_log ()
{
    struct t_rmodifier *ptr_rmodifier;
    int i;

    for (ptr_rmodifier = rmodifier_list; ptr_rmodifier;
         ptr_rmodifier = ptr_rmodifier->next_rmodifier)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[rmodifier %s (addr:0x%lx)]", ptr_rmodifier->name, ptr_rmodifier);
        weechat_log_printf ("  modifiers. . . . . . : '%s'",  ptr_rmodifier->modifiers);
        weechat_log_printf ("  hooks. . . . . . . . : 0x%lx", ptr_rmodifier->hooks);
        for (i = 0; i < ptr_rmodifier->hooks_count; i++)
        {
            weechat_log_printf ("    hooks[%03d] . . . . : 0x%lx", i, ptr_rmodifier->hooks[i]);
        }
        weechat_log_printf ("  hooks_count. . . . . : %d",    ptr_rmodifier->hooks_count);
        weechat_log_printf ("  str_regex. . . . . . : '%s'",  ptr_rmodifier->str_regex);
        weechat_log_printf ("  regex. . . . . . . . : 0x%lx", ptr_rmodifier->regex);
        weechat_log_printf ("  groups . . . . . . . : '%s'",  ptr_rmodifier->groups);
        weechat_log_printf ("  prev_rmodifier . . . : 0x%lx", ptr_rmodifier->prev_rmodifier);
        weechat_log_printf ("  next_rmodifier . . . : 0x%lx", ptr_rmodifier->next_rmodifier);
    }
}

/*
 * weechat_plugin_init: initialize rmodifier plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    rmodifier_count = 0;

    rmodifier_hook_list = weechat_list_new ();

    if (!rmodifier_config_init ())
    {
        weechat_printf (NULL,
                        _("%s%s: error creating configuration file"),
                        weechat_prefix("error"), RMODIFIER_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }
    rmodifier_config_read ();

    rmodifier_command_init ();
    rmodifier_completion_init ();

    rmodifier_info_init ();

    rmodifier_debug_init ();

    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end rmodifier plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    rmodifier_config_write ();
    rmodifier_free_all ();
    weechat_list_free (rmodifier_hook_list);
    weechat_config_free (rmodifier_config_file);

    return WEECHAT_RC_OK;
}
