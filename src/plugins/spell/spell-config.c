/*
 * spell-config.c - spell checker configuration options (file spell.conf)
 *
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "spell.h"
#include "spell-config.h"
#include "spell-speller.h"


struct t_config_file *spell_config_file = NULL;

/* sections */

struct t_config_section *spell_config_section_color = NULL;
struct t_config_section *spell_config_section_check = NULL;
struct t_config_section *spell_config_section_dict = NULL;
struct t_config_section *spell_config_section_look = NULL;
struct t_config_section *spell_config_section_option = NULL;

/* spell config, color section */

struct t_config_option *spell_config_color_misspelled = NULL;
struct t_config_option *spell_config_color_suggestion = NULL;
struct t_config_option *spell_config_color_suggestion_delimiter_dict = NULL;
struct t_config_option *spell_config_color_suggestion_delimiter_word = NULL;

/* spell config, check section */

struct t_config_option *spell_config_check_commands = NULL;
struct t_config_option *spell_config_check_default_dict = NULL;
struct t_config_option *spell_config_check_during_search = NULL;
struct t_config_option *spell_config_check_enabled = NULL;
struct t_config_option *spell_config_check_real_time = NULL;
struct t_config_option *spell_config_check_suggestions = NULL;
struct t_config_option *spell_config_check_word_min_length = NULL;

/* spell config, look section */

struct t_config_option *spell_config_look_suggestion_delimiter_dict = NULL;
struct t_config_option *spell_config_look_suggestion_delimiter_word = NULL;


int spell_config_loading = 0;
char **spell_commands_to_check = NULL;
int spell_count_commands_to_check = 0;
int *spell_length_commands_to_check = NULL;


/*
 * Callback for changes on option "spell.check.commands".
 */

void
spell_config_change_commands (const void *pointer, void *data,
                              struct t_config_option *option)
{
    const char *value;
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (spell_commands_to_check)
    {
        weechat_string_free_split (spell_commands_to_check);
        spell_commands_to_check = NULL;
        spell_count_commands_to_check = 0;
    }

    if (spell_length_commands_to_check)
    {
        free (spell_length_commands_to_check);
        spell_length_commands_to_check = NULL;
    }

    value = weechat_config_string (option);
    if (value && value[0])
    {
        spell_commands_to_check = weechat_string_split (
            value,
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &spell_count_commands_to_check);
        if (spell_count_commands_to_check > 0)
        {
            spell_length_commands_to_check = malloc (spell_count_commands_to_check *
                                                     sizeof (int));
            for (i = 0; i < spell_count_commands_to_check; i++)
            {
                spell_length_commands_to_check[i] = strlen (spell_commands_to_check[i]);
            }
        }
    }
}

/*
 * Callback for changes on option "spell.check.default_dict".
 */

void
spell_config_change_default_dict (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    weechat_hashtable_remove_all (spell_speller_buffer);
    if (!spell_config_loading)
        spell_speller_remove_unused ();
}

/*
 * Callback for changes on option "spell.check.enabled".
 */

void
spell_config_change_enabled (const void *pointer, void *data,
                             struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    spell_enabled = weechat_config_boolean (option);

    /* refresh input and spell suggestions */
    weechat_bar_item_update ("input_text");
    weechat_bar_item_update ("spell_suggest");
}

/*
 * Callback for changes on option "spell.check.suggestions".
 */

void
spell_config_change_suggestions (const void *pointer, void *data,
                                 struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    weechat_bar_item_update ("spell_suggest");
}

/*
 * Callback for changes on a dictionary.
 */

void
spell_config_dict_change (const void *pointer, void *data,
                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    weechat_hashtable_remove_all (spell_speller_buffer);
    if (!spell_config_loading)
        spell_speller_remove_unused ();
}

/*
 * Callback called when an option is deleted in section "dict".
 */

int
spell_config_dict_delete_option (const void *pointer, void *data,
                                 struct t_config_file *config_file,
                                 struct t_config_section *section,
                                 struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    weechat_config_option_free (option);

    weechat_hashtable_remove_all (spell_speller_buffer);
    if (!spell_config_loading)
        spell_speller_remove_unused ();

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Creates an option in section "dict".
 */

int
spell_config_dict_create_option (const void *pointer, void *data,
                                 struct t_config_file *config_file,
                                 struct t_config_section *section,
                                 const char *option_name,
                                 const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (value && value[0])
        spell_speller_check_dictionaries (value);

    if (option_name)
    {
        ptr_option = weechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = weechat_config_option_set (ptr_option, value, 0);
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
                    _("comma separated list of dictionaries to use on this "
                      "buffer (special value \"-\" disables spell checking "
                      "on this buffer)"),
                    NULL, 0, 0, "", value, 0,
                    NULL, NULL, NULL,
                    &spell_config_dict_change, NULL, NULL,
                    NULL, NULL, NULL);
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
                        _("%s%s: error creating spell dictionary \"%s\" => \"%s\""),
                        weechat_prefix ("error"), SPELL_PLUGIN_NAME,
                        option_name, value);
    }
    else
    {
        weechat_hashtable_remove_all (spell_speller_buffer);
        if (!spell_config_loading)
            spell_speller_remove_unused ();
    }

    return rc;
}

/*
 * Callback for changes on an spell option.
 */

void
spell_config_option_change (const void *pointer, void *data,
                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    weechat_hashtable_remove_all (spell_speller_buffer);
    if (!spell_config_loading)
        spell_speller_remove_unused ();
}

/*
 * Callback called when an option is deleted in section "option".
 */

int
spell_config_option_delete_option (const void *pointer, void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    weechat_config_option_free (option);

    weechat_hashtable_remove_all (spell_speller_buffer);
    if (!spell_config_loading)
        spell_speller_remove_unused ();

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Callback called when an option is created in section "option".
 */

int
spell_config_option_create_option (const void *pointer, void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   const char *option_name,
                                   const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) pointer;
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
                    NULL, NULL, NULL,
                    &spell_config_option_change, NULL, NULL,
                    NULL, NULL, NULL);
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
                        _("%s%s: error creating spell option \"%s\" => \"%s\""),
                        weechat_prefix ("error"), SPELL_PLUGIN_NAME,
                        option_name, value);
    }
    else
    {
        weechat_hashtable_remove_all (spell_speller_buffer);
        if (!spell_config_loading)
            spell_speller_remove_unused ();
    }

    return rc;
}

/*
 * Gets a list of dictionaries for a buffer.
 */

struct t_config_option *
spell_config_get_dict (const char *name)
{
    return weechat_config_search_option (spell_config_file,
                                         spell_config_section_dict,
                                         name);
}

/*
 * Sets a list of dictionaries for a buffer.
 */

int
spell_config_set_dict (const char *name, const char *value)
{
    return spell_config_dict_create_option (NULL, NULL,
                                            spell_config_file,
                                            spell_config_section_dict,
                                            name,
                                            value);
}

/*
 * Initializes spell configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
spell_config_init ()
{
    spell_config_file = weechat_config_new (SPELL_CONFIG_PRIO_NAME,
                                            NULL, NULL, NULL);
    if (!spell_config_file)
        return 0;

    /* color */
    spell_config_section_color = weechat_config_new_section (
        spell_config_file, "color",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (spell_config_section_color)
    {
        spell_config_color_misspelled = weechat_config_new_option (
            spell_config_file, spell_config_section_color,
            "misspelled", "color",
            N_("text color for misspelled words (input bar)"),
            NULL, 0, 0, "lightred", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        spell_config_color_suggestion = weechat_config_new_option (
            spell_config_file, spell_config_section_color,
            "suggestion", "color",
            N_("text color for suggestion on a misspelled word in bar item "
               "\"spell_suggest\""),
            NULL, 0, 0, "default", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        spell_config_color_suggestion_delimiter_dict = weechat_config_new_option (
            spell_config_file, spell_config_section_color,
            "suggestion_delimiter_dict", "color",
            N_("text color for delimiters displayed between two dictionaries "
               "in bar item \"spell_suggest\""),
            NULL, 0, 0, "cyan", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        spell_config_color_suggestion_delimiter_word = weechat_config_new_option (
            spell_config_file, spell_config_section_color,
            "suggestion_delimiter_word", "color",
            N_("text color for delimiters displayed between two words in bar "
               "item \"spell_suggest\""),
            NULL, 0, 0, "cyan", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* check */
    spell_config_section_check = weechat_config_new_section (
        spell_config_file, "check",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (spell_config_section_check)
    {
        spell_config_check_commands = weechat_config_new_option (
            spell_config_file, spell_config_section_check,
            "commands", "string",
            N_("comma separated list of commands for which spell checking is "
               "enabled (spell checking is disabled for all other commands)"),
            NULL, 0, 0,
            "away,command,cycle,kick,kickban,me,msg,notice,part,query,"
            "quit,topic", NULL, 0,
            NULL, NULL, NULL,
            &spell_config_change_commands, NULL, NULL,
            NULL, NULL, NULL);
        spell_config_check_default_dict = weechat_config_new_option (
            spell_config_file, spell_config_section_check,
            "default_dict", "string",
            N_("default dictionary (or comma separated list of dictionaries) to "
               "use when buffer has no dictionary defined (leave blank to "
               "disable spell checker on buffers for which you didn't "
               "explicitly enabled it)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &spell_config_change_default_dict, NULL, NULL,
            NULL, NULL, NULL);
        spell_config_check_during_search = weechat_config_new_option (
            spell_config_file, spell_config_section_check,
            "during_search", "boolean",
            N_("check words during text search in buffer"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        spell_config_check_enabled = weechat_config_new_option (
            spell_config_file, spell_config_section_check,
            "enabled", "boolean",
            N_("enable spell checker for command line"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &spell_config_change_enabled, NULL, NULL,
            NULL, NULL, NULL);
        spell_config_check_real_time = weechat_config_new_option (
            spell_config_file, spell_config_section_check,
            "real_time", "boolean",
            N_("real-time spell checking of words (slower, disabled by default: "
               "words are checked only if there's delimiter after)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        spell_config_check_suggestions = weechat_config_new_option (
            spell_config_file, spell_config_section_check,
            "suggestions", "integer",
            N_("number of suggestions to display in bar item \"spell_suggest\" "
               "for each dictionary set in buffer (-1 = disable suggestions, "
               "0 = display all possible suggestions in all languages)"),
            NULL, -1, INT_MAX, "-1", NULL, 0,
            NULL, NULL, NULL,
            &spell_config_change_suggestions, NULL, NULL,
            NULL, NULL, NULL);
        spell_config_check_word_min_length = weechat_config_new_option (
            spell_config_file, spell_config_section_check,
            "word_min_length", "integer",
            N_("minimum length for a word to be spell checked (use 0 to check "
               "all words)"),
            NULL, 0, INT_MAX, "2", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* dict */
    spell_config_section_dict = weechat_config_new_section (
        spell_config_file, "dict",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &spell_config_dict_create_option, NULL, NULL,
        &spell_config_dict_delete_option, NULL, NULL);

    /* look */
    spell_config_section_look = weechat_config_new_section (
        spell_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (spell_config_section_look)
    {
        spell_config_look_suggestion_delimiter_dict = weechat_config_new_option (
            spell_config_file, spell_config_section_look,
            "suggestion_delimiter_dict", "string",
            N_("delimiter displayed between two dictionaries in bar item "
               "\"spell_suggest\""),
            NULL, 0, 0, " / ", NULL, 0,
            NULL, NULL, NULL,
            &spell_config_change_suggestions, NULL, NULL,
            NULL, NULL, NULL);
        spell_config_look_suggestion_delimiter_word = weechat_config_new_option (
            spell_config_file, spell_config_section_look,
            "suggestion_delimiter_word", "string",
            N_("delimiter displayed between two words in bar item "
               "\"spell_suggest\""),
            NULL, 0, 0, ",", NULL, 0,
            NULL, NULL, NULL,
            &spell_config_change_suggestions, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* option */
    spell_config_section_option = weechat_config_new_section (
        spell_config_file, "option",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &spell_config_option_create_option, NULL, NULL,
        &spell_config_option_delete_option, NULL, NULL);

    return 1;
}

/*
 * Reads spell configuration file.
 */

int
spell_config_read ()
{
    int rc;

    spell_config_loading = 1;
    rc = weechat_config_read (spell_config_file);
    spell_config_loading = 0;
    if (rc == WEECHAT_CONFIG_READ_OK)
        spell_config_change_commands (NULL, NULL, spell_config_check_commands);
    spell_speller_remove_unused ();

    return rc;
}

/*
 * Writes spell configuration file.
 */

int
spell_config_write ()
{
    return weechat_config_write (spell_config_file);
}

/*
 * Frees spell configuration.
 */

void
spell_config_free ()
{
    weechat_config_free (spell_config_file);
    spell_config_file = NULL;

    if (spell_commands_to_check)
    {
        weechat_string_free_split (spell_commands_to_check);
        spell_commands_to_check = NULL;
    }
    if (spell_length_commands_to_check)
    {
        free (spell_length_commands_to_check);
        spell_length_commands_to_check = NULL;
    }
}
