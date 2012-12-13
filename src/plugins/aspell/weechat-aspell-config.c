/*
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * weechat-aspell-config.c: aspell configuration options (file aspell.conf)
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "weechat-aspell.h"
#include "weechat-aspell-config.h"
#include "weechat-aspell-speller.h"


struct t_config_file *weechat_aspell_config_file = NULL;
struct t_config_section *weechat_aspell_config_section_dict = NULL;

/* aspell config, look section */

struct t_config_option *weechat_aspell_config_look_color;

/* aspell config, check section */

struct t_config_option *weechat_aspell_config_check_commands;
struct t_config_option *weechat_aspell_config_check_default_dict;
struct t_config_option *weechat_aspell_config_check_during_search;
struct t_config_option *weechat_aspell_config_check_enabled;
struct t_config_option *weechat_aspell_config_check_real_time;
struct t_config_option *weechat_aspell_config_check_suggestions;
struct t_config_option *weechat_aspell_config_check_word_min_length;


char **weechat_aspell_commands_to_check = NULL;
int weechat_aspell_count_commands_to_check = 0;
int *weechat_aspell_length_commands_to_check = NULL;


/*
 * Callback for changes on option "aspell.check.commands".
 */

void
weechat_aspell_config_change_commands (void *data,
                                       struct t_config_option *option)
{
    const char *value;
    int i;

    /* make C compiler happy */
    (void) data;

    if (weechat_aspell_commands_to_check)
    {
        weechat_string_free_split (weechat_aspell_commands_to_check);
        weechat_aspell_commands_to_check = NULL;
        weechat_aspell_count_commands_to_check = 0;
    }

    if (weechat_aspell_length_commands_to_check)
    {
        free (weechat_aspell_length_commands_to_check);
        weechat_aspell_length_commands_to_check = NULL;
    }

    value = weechat_config_string (option);
    if (value && value[0])
    {
        weechat_aspell_commands_to_check = weechat_string_split (value,
                                                                 ",", 0, 0,
                                                                 &weechat_aspell_count_commands_to_check);
        if (weechat_aspell_count_commands_to_check > 0)
        {
            weechat_aspell_length_commands_to_check = malloc (weechat_aspell_count_commands_to_check *
                                                              sizeof (int));
            for (i = 0; i < weechat_aspell_count_commands_to_check; i++)
            {
                weechat_aspell_length_commands_to_check[i] = strlen (weechat_aspell_commands_to_check[i]);
            }
        }
    }
}

/*
 * Callback for changes on option "aspell.check.default_dict".
 */

void
weechat_aspell_config_change_default_dict (void *data,
                                           struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_aspell_create_spellers (weechat_current_buffer ());
}

/*
 * Callback for changes on option "aspell.check.enabled".
 */

void
weechat_aspell_config_change_enabled (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;

    aspell_enabled = weechat_config_boolean (option);

    /* refresh input and aspell suggestions */
    weechat_bar_item_update ("input_text");
    weechat_bar_item_update ("aspell_suggest");
}

/*
 * Callback for changes on option "aspell.check.suggestions".
 */

void
weechat_aspell_config_change_suggestions (void *data,
                                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_bar_item_update ("aspell_suggest");
}

/*
 * Callback for changes on a dictionary.
 */

void
weechat_aspell_config_dict_change (void *data,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_aspell_create_spellers (weechat_current_buffer ());
}

/*
 * Callback called when an option is deleted in section "dict".
 */

int
weechat_aspell_config_dict_delete_option (void *data,
                                          struct t_config_file *config_file,
                                          struct t_config_section *section,
                                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    weechat_config_option_free (option);

    weechat_aspell_create_spellers (weechat_current_buffer ());

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Creates an option in section "dict".
 */

int
weechat_aspell_config_dict_create_option (void *data,
                                          struct t_config_file *config_file,
                                          struct t_config_section *section,
                                          const char *option_name,
                                          const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (value && value[0])
        weechat_aspell_speller_check_dictionaries (value);

    if (option_name)
    {
        ptr_option = weechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = weechat_config_option_set (ptr_option, value, 1);
            else
            {
                weechat_config_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = weechat_config_new_option (
                    config_file, section,
                    option_name, "string",
                    _("comma separated list of dictionaries to use on this buffer"),
                    NULL, 0, 0, "", value, 0,
                    NULL, NULL,
                    &weechat_aspell_config_dict_change, NULL,
                    NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating aspell dictionary \"%s\" => \"%s\""),
                        weechat_prefix ("error"), ASPELL_PLUGIN_NAME,
                        option_name, value);
    }
    else
        weechat_aspell_create_spellers (weechat_current_buffer ());

    return rc;
}

/*
 * Callback for changes on an aspell option.
 */

void
weechat_aspell_config_option_change (void *data,
                                     struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) option;

    weechat_aspell_speller_free_all ();
    weechat_aspell_create_spellers (weechat_current_buffer ());
}

/*
 * Callback called when an option is deleted in section "option".
 */

int
weechat_aspell_config_option_delete_option (void *data,
                                            struct t_config_file *config_file,
                                            struct t_config_section *section,
                                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    weechat_config_option_free (option);

    weechat_aspell_speller_free_all ();
    weechat_aspell_create_spellers (weechat_current_buffer ());

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Callback called when an option is created in section "option".
 */

int
weechat_aspell_config_option_create_option (void *data,
                                            struct t_config_file *config_file,
                                            struct t_config_section *section,
                                            const char *option_name,
                                            const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = weechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = weechat_config_option_set (ptr_option, value, 1);
            else
            {
                weechat_config_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = weechat_config_new_option (
                    config_file, section,
                    option_name, "string",
                    _("option for aspell (for list of available options and "
                      "format, run command \"aspell config\" in a shell)"),
                    NULL, 0, 0, "", value, 0,
                    NULL, NULL,
                    &weechat_aspell_config_option_change, NULL,
                    NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating aspell option \"%s\" => \"%s\""),
                        weechat_prefix ("error"), ASPELL_PLUGIN_NAME,
                        option_name, value);
    }
    else
    {
        weechat_aspell_speller_free_all ();
        weechat_aspell_create_spellers (weechat_current_buffer ());
    }

    return rc;
}

/*
 * Gets a list of dictionaries for a buffer.
 */

struct t_config_option *
weechat_aspell_config_get_dict (const char *name)
{
    return weechat_config_search_option (weechat_aspell_config_file,
                                         weechat_aspell_config_section_dict,
                                         name);
}

/*
 * Sets a list of dictionaries for a buffer.
 */

int
weechat_aspell_config_set_dict (const char *name, const char *value)
{
    return weechat_aspell_config_dict_create_option (NULL,
                                                     weechat_aspell_config_file,
                                                     weechat_aspell_config_section_dict,
                                                     name,
                                                     value);
}

/*
 * Initializes aspell configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
weechat_aspell_config_init ()
{
    struct t_config_section *ptr_section;

    weechat_aspell_config_file = weechat_config_new (ASPELL_CONFIG_NAME,
                                                     NULL, NULL);
    if (!weechat_aspell_config_file)
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (weechat_aspell_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (weechat_aspell_config_file);
        return 0;
    }

    weechat_aspell_config_look_color = weechat_config_new_option (
        weechat_aspell_config_file, ptr_section,
        "color", "color",
        N_("color used for misspelled words"),
        NULL, 0, 0, "lightred", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* check */
    ptr_section = weechat_config_new_section (weechat_aspell_config_file, "check",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (weechat_aspell_config_file);
        return 0;
    }

    weechat_aspell_config_check_commands = weechat_config_new_option (
        weechat_aspell_config_file, ptr_section,
        "commands", "string",
        N_("comma separated list of commands for which spell checking is "
           "enabled (spell checking is disabled for all other commands)"),
        NULL, 0, 0,
        "ame,amsg,away,command,cycle,kick,kickban,me,msg,notice,part,query,"
        "quit,topic", NULL, 0,
        NULL, NULL, &weechat_aspell_config_change_commands, NULL, NULL, NULL);
    weechat_aspell_config_check_default_dict = weechat_config_new_option (
        weechat_aspell_config_file, ptr_section,
        "default_dict", "string",
        N_("default dictionary (or comma separated list of dictionaries) to "
           "use when buffer has no dictionary defined (leave blank to disable "
           "aspell on buffers for which you didn't explicitly enabled it)"),
        NULL, 0, 0, "", NULL, 0,
        NULL, NULL, &weechat_aspell_config_change_default_dict, NULL, NULL, NULL);
    weechat_aspell_config_check_during_search = weechat_config_new_option (
        weechat_aspell_config_file, ptr_section,
        "during_search", "boolean",
        N_("check words during text search in buffer"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    weechat_aspell_config_check_enabled = weechat_config_new_option (
        weechat_aspell_config_file, ptr_section,
        "enabled", "boolean",
        N_("enable aspell check for command line"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, &weechat_aspell_config_change_enabled, NULL, NULL, NULL);
    weechat_aspell_config_check_real_time = weechat_config_new_option (
        weechat_aspell_config_file, ptr_section,
        "real_time", "boolean",
        N_("real-time spell checking of words (slower, disabled by default: "
           "words are checked only if there's delimiter after)"),
        NULL, 0, 0, "off", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    weechat_aspell_config_check_suggestions = weechat_config_new_option (
        weechat_aspell_config_file, ptr_section,
        "suggestions", "integer",
        N_("number of suggestions to display in bar item \"aspell_suggest\" "
           "for each dictionary set in buffer (-1 = disable suggestions, "
           "0 = display all possible suggestions in all languages)"),
        NULL, -1, INT_MAX, "-1", NULL, 0,
        NULL, NULL, &weechat_aspell_config_change_suggestions, NULL, NULL, NULL);
    weechat_aspell_config_check_word_min_length = weechat_config_new_option (
        weechat_aspell_config_file, ptr_section,
        "word_min_length", "integer",
        N_("minimum length for a word to be spell checked (use 0 to check all "
           "words)"),
        NULL, 0, INT_MAX, "2", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* dict */
    ptr_section = weechat_config_new_section (weechat_aspell_config_file, "dict",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &weechat_aspell_config_dict_create_option, NULL,
                                              &weechat_aspell_config_dict_delete_option, NULL);
    if (!ptr_section)
    {
        weechat_config_free (weechat_aspell_config_file);
        return 0;
    }

    weechat_aspell_config_section_dict = ptr_section;

    /* option */
    ptr_section = weechat_config_new_section (weechat_aspell_config_file, "option",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &weechat_aspell_config_option_create_option, NULL,
                                              &weechat_aspell_config_option_delete_option, NULL);
    if (!ptr_section)
    {
        weechat_config_free (weechat_aspell_config_file);
        return 0;
    }

    return 1;
}

/*
 * Reads aspell configuration file.
 */

int
weechat_aspell_config_read ()
{
    int rc;

    rc = weechat_config_read (weechat_aspell_config_file);
    if (rc == WEECHAT_CONFIG_READ_OK)
    {
        weechat_aspell_config_change_commands (NULL,
                                               weechat_aspell_config_check_commands);
    }

    return rc;
}

/*
 * Writes aspell configuration file.
 */

int
weechat_aspell_config_write ()
{
    return weechat_config_write (weechat_aspell_config_file);
}

/*
 * Frees aspell configuration.
 */

void
weechat_aspell_config_free ()
{
    weechat_config_free (weechat_aspell_config_file);

    if (weechat_aspell_commands_to_check)
        weechat_string_free_split (weechat_aspell_commands_to_check);
    if (weechat_aspell_length_commands_to_check)
        free (weechat_aspell_length_commands_to_check);
}
