/*
 * fset-option.c - manage options displayed by Fast Set buffer
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <sys/stat.h>

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-option.h"
#include "fset-buffer.h"
#include "fset-config.h"


/* options */
struct t_arraylist *fset_options = NULL;
int fset_option_count_marked = 0;
struct t_fset_option_max_length *fset_option_max_length = NULL;

/* filters */
char *fset_option_filter = NULL;
struct t_hashtable *fset_option_filter_hashtable_pointers = NULL;
struct t_hashtable *fset_option_filter_hashtable_extra_vars = NULL;
struct t_hashtable *fset_option_filter_hashtable_options = NULL;

/* refresh */
struct t_hashtable *fset_option_timer_options_changed = NULL;
struct t_hook *fset_option_timer_hook = NULL;

/* types */
char *fset_option_type_string[FSET_OPTION_NUM_TYPES] =
{ N_("boolean"), N_("integer"), N_("string"), N_("color") };
char *fset_option_type_string_short[FSET_OPTION_NUM_TYPES] =
{ "bool", "int", "str", "col" };
char *fset_option_type_string_tiny[FSET_OPTION_NUM_TYPES] =
{ "b", "i", "s", "c" };


/*
 * Checks if a fset option pointer is valid.
 *
 * Returns:
 *   1: option exists
 *   0: option does not exist
 */

int
fset_option_valid (struct t_fset_option *fset_option)
{
    struct t_fset_option *ptr_fset_option;
    int num_options, i;

    if (!fset_option)
        return 0;

    num_options = weechat_arraylist_size (fset_options);
    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (ptr_fset_option == fset_option)
            return 1;
    }

    /* fset option not found */
    return 0;
}

/*
 * Searches for an option by name.
 *
 * If line is not NULL, *line is set with the line number of option found
 * (-1 if line is not found).
 *
 * Returns pointer to option found, NULL if not found.
 */

struct t_fset_option *
fset_option_search_by_name (const char *name, int *line)
{
    struct t_fset_option *ptr_fset_option;
    int num_options, i;

    if (line)
        *line = -1;

    if (!name)
        return NULL;

    num_options = weechat_arraylist_size (fset_options);
    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (ptr_fset_option && (strcmp (ptr_fset_option->name, name) == 0))
        {
            if (line)
                *line = i;
            return ptr_fset_option;
        }
    }

    /* fset option not found */
    return NULL;
}

/*
 * Checks if the option value is changed (different from the default value).
 *
 * Returns:
 *   1: value has been changed
 *   0: value is the same as default value
 */

int
fset_option_value_is_changed (struct t_fset_option *fset_option)
{
    if (!fset_option->value && !fset_option->default_value)
        return 0;

    if ((fset_option->value && !fset_option->default_value)
        || (!fset_option->value && fset_option->default_value))
    {
        return 1;
    }

    return (strcmp (fset_option->value,
                    fset_option->default_value) != 0) ? 1 : 0;
}

/*
 * Sets the value in option, according to its type.
 */

void
fset_option_set_value_string (struct t_config_option *option,
                              enum t_fset_option_type type, void *value,
                              int default_value,
                              char **value_string)
{
    char str_value[64];
    void *ptr_string_values;

    if (!value)
    {
        *value_string = NULL;
    }
    else
    {
        switch (type)
        {
            case FSET_OPTION_TYPE_BOOLEAN:
                *value_string = strdup (*((int *)value) ? "on" : "off");
                break;
            case FSET_OPTION_TYPE_INTEGER:
                ptr_string_values = weechat_config_option_get_pointer (
                    option, "string_values");
                if (ptr_string_values)
                {
                    *value_string = strdup (
                        (default_value) ? weechat_config_string_default (option) : weechat_config_string (option));
                }
                else
                {
                    snprintf (str_value, sizeof (str_value), "%d", *((int *)value));
                    *value_string = strdup (str_value);
                }
                break;
            case FSET_OPTION_TYPE_STRING:
                *value_string = strdup (
                    (default_value) ? weechat_config_string_default (option) : weechat_config_string (option));
                break;
            case FSET_OPTION_TYPE_COLOR:
                *value_string = strdup (
                    (default_value) ? weechat_config_color_default (option) : weechat_config_color (option));
                break;
            case FSET_OPTION_NUM_TYPES:
                break;
        }
    }
}

/*
 * Checks if a string matches a mask.
 *
 * If mask has no "*" inside, it just checks if "mask" is inside the "string".
 * If mask has at least one "*" inside, the function weechat_string_match is
 * used.
 *
 * Returns:
 *   1: string matches mask
 *   0: string does not match mask
 */

int
fset_option_string_match (const char *string, const char *mask)
{
    if (strchr (mask, '*'))
        return weechat_string_match (string, mask, 0);
    else
        return (weechat_strcasestr (string, mask)) ? 1 : 0;
}

/*
 * Adds the properties of an fset option in a hashtable
 * (keys and values must be strings).
 */

void
fset_option_add_option_in_hashtable (struct t_hashtable *hashtable,
                                     struct t_fset_option *fset_option)
{
    int length;
    char *value;

    weechat_hashtable_set (hashtable, "file", fset_option->file);
    weechat_hashtable_set (hashtable, "section", fset_option->section);
    weechat_hashtable_set (hashtable, "option", fset_option->option);
    weechat_hashtable_set (hashtable, "name", fset_option->name);
    weechat_hashtable_set (hashtable, "parent_name", fset_option->parent_name);
    weechat_hashtable_set (hashtable,
                           "type", _(fset_option_type_string[fset_option->type]));
    weechat_hashtable_set (hashtable,
                           "type_en", fset_option_type_string[fset_option->type]);
    weechat_hashtable_set (hashtable,
                           "type_short", fset_option_type_string_short[fset_option->type]);
    weechat_hashtable_set (hashtable,
                           "type_tiny", fset_option_type_string_tiny[fset_option->type]);
    weechat_hashtable_set (hashtable,
                           "default_value", fset_option->default_value);
    weechat_hashtable_set (hashtable, "value", fset_option->value);
    if (fset_option->value && (fset_option->type == FSET_OPTION_TYPE_STRING))
    {
        length = 1 + strlen (fset_option->value) + 1 + 1;
        value = malloc (length);
        if (value)
        {
            snprintf (value, length, "\"%s\"", fset_option->value);
            weechat_hashtable_set (hashtable, "quoted_value", value);
            free (value);
        }
        else
        {
            weechat_hashtable_set (hashtable,
                                   "quoted_value", fset_option->value);
        }
    }
    else
    {
        weechat_hashtable_set (hashtable, "quoted_value", fset_option->value);
    }
    weechat_hashtable_set (hashtable,
                           "parent_value", fset_option->parent_value);
    weechat_hashtable_set (hashtable, "min", fset_option->min);
    weechat_hashtable_set (hashtable, "max", fset_option->max);
    weechat_hashtable_set (hashtable,
                           "description",
                           (fset_option->description && fset_option->description[0]) ?
                           _(fset_option->description) : "");
    weechat_hashtable_set (hashtable,
                           "description2",
                           (fset_option->description && fset_option->description[0]) ?
                           _(fset_option->description) : _("(no description)"));
    weechat_hashtable_set (hashtable,
                           "description_en", fset_option->description);
    weechat_hashtable_set (hashtable,
                           "description_en2",
                           (fset_option->description && fset_option->description[0]) ?
                           fset_option->description : "(no description)");
    weechat_hashtable_set (hashtable,
                           "string_values", fset_option->string_values);
    weechat_hashtable_set (hashtable,
                           "default_value_undef",
                           (fset_option->default_value == NULL) ? "1" : "0");
    weechat_hashtable_set (hashtable,
                           "value_undef",
                           (fset_option->value == NULL) ? "1" : "0");
    weechat_hashtable_set (hashtable,
                           "value_changed",
                           (fset_option_value_is_changed (fset_option)) ? "1" : "0");
}

/*
 * Checks if an option is matching current filter(s).
 *
 * Returns:
 *   1: option is matching filter(s)
 *   0: option does not match filter(s)
 */

int
fset_option_match_filter (struct t_fset_option *fset_option, const char *filter)
{
    int match;
    char *result;

    if (!filter || !filter[0])
        return 1;

    if (strncmp (filter, "c:", 2) == 0)
    {
        /* filter by evaluated condition */
        weechat_hashtable_set (fset_option_filter_hashtable_pointers,
                               "fset_option", fset_option);
        fset_option_add_option_in_hashtable (
            fset_option_filter_hashtable_extra_vars,
            fset_option);
        result = weechat_string_eval_expression (
            filter + 2,
            fset_option_filter_hashtable_pointers,
            fset_option_filter_hashtable_extra_vars,
            fset_option_filter_hashtable_options);
        match = (result && (strcmp (result, "1") == 0)) ? 1 : 0;
        if (result)
            free (result);
        return match;
    }
    else if (strncmp (filter, "f:", 2) == 0)
    {
        /* filter by config name */
        return (weechat_strcasecmp (fset_option->file,
                                    filter + 2) == 0) ? 1 : 0;
    }
    else if (strncmp (filter, "t:", 2) == 0)
    {
        /* filter by type */
        return (weechat_strcasecmp (
                    fset_option_type_string_short[fset_option->type],
                    filter + 2) == 0) ? 1 : 0;
    }
    else if (strncmp (filter, "d==", 3) == 0)
    {
        /* filter by modified values (on exact value) */
        if (!fset_option_value_is_changed (fset_option))
            return 0;
        return (weechat_strcasecmp (
                    (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
                    filter + 3) == 0) ? 1 : 0;
    }
    else if (strncmp (filter, "d=", 2) == 0)
    {
        /* filter by modified values (on value) */
        if (!fset_option_value_is_changed (fset_option))
            return 0;
        return (fset_option_string_match (
                    (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
                    filter + 2)) ? 1 : 0;
    }
    else if (strncmp (filter, "d:", 2) == 0)
    {
        /* filter by modified values (on name) */
        if (!fset_option_value_is_changed (fset_option))
            return 0;
        return fset_option_string_match (fset_option->name,
                                         filter + 2) ? 1 : 0;
    }
    else if (strcmp (filter, "d") == 0)
    {
        /* filter by modified values */
        return (fset_option_value_is_changed (fset_option)) ? 1 : 0;
    }
    else if (strncmp (filter, "h=", 2) == 0)
    {
        /* filter by help text (translated) */
        return (fset_option_string_match (
                    (fset_option->description && fset_option->description[0]) ?
                    _(fset_option->description) : "",
                    filter + 2)) ? 1 : 0;
    }
    else if (strncmp (filter, "he=", 3) == 0)
    {
        /* filter by help text (in English) */
        return (fset_option_string_match (
                    (fset_option->description && fset_option->description[0]) ?
                    fset_option->description : "",
                    filter + 3)) ? 1 : 0;
    }
    else if (strncmp (filter, "==", 2) == 0)
    {
        /* filter by exact value */
        return (weechat_strcasecmp (
                    (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
                    filter + 2) == 0) ? 1 : 0;
    }
    else if (filter[0] == '=')
    {
        /* filter by value */
        return (fset_option_string_match (
                    (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
                    filter + 1)) ? 1 : 0;
    }
    else
    {
        /* filter by option name */
        return (fset_option_string_match (fset_option->name, filter)) ? 1 : 0;
    }
}

/*
 * Sets (or sets again) values (except name) in an fset option.
 */

void
fset_option_set_values (struct t_fset_option *fset_option,
                        struct t_config_option *option)
{
    const char *ptr_config_name, *ptr_section_name, *ptr_option_name;
    const char *ptr_parent_name, *ptr_description;
    const char **ptr_string_values;
    void *ptr_default_value, *ptr_value;
    struct t_config_option *ptr_parent_option;
    int length, *ptr_type, *ptr_min, *ptr_max;
    char str_value[64];

    /* file */
    if (fset_option->file)
    {
        free (fset_option->file);
        fset_option->file = NULL;
    }
    ptr_config_name = weechat_config_option_get_string (option, "config_name");
    fset_option->file = strdup (ptr_config_name);

    /* section */
    if (fset_option->section)
    {
        free (fset_option->section);
        fset_option->section = NULL;
    }
    ptr_section_name = weechat_config_option_get_string (option, "section_name");
    fset_option->section = strdup (ptr_section_name);

    /* option */
    if (fset_option->option)
    {
        free (fset_option->option);
        fset_option->option = NULL;
    }
    ptr_option_name = weechat_config_option_get_string (option, "name");
    fset_option->option = strdup (ptr_option_name);

    /* name */
    if (fset_option->name)
    {
        free (fset_option->name);
        fset_option->name = NULL;
    }
    length = strlen (ptr_config_name) + 1 +
        strlen (ptr_section_name) + 1 +
        strlen (ptr_option_name) + 1;
    fset_option->name = malloc (length);
    if (fset_option->name)
    {
        snprintf (fset_option->name, length, "%s.%s.%s",
                  ptr_config_name,
                  ptr_section_name,
                  ptr_option_name);
    }

    /* parent name */
    if (fset_option->parent_name)
    {
        free (fset_option->parent_name);
        fset_option->parent_name = NULL;
    }
    ptr_parent_name = weechat_config_option_get_string (option, "parent_name");
    fset_option->parent_name = (ptr_parent_name) ? strdup (ptr_parent_name) : NULL;

    /* type */
    ptr_type = weechat_config_option_get_pointer (option, "type");
    fset_option->type = *ptr_type;

    /* default value */
    if (fset_option->default_value)
    {
        free (fset_option->default_value);
        fset_option->default_value = NULL;
    }
    ptr_default_value = weechat_config_option_get_pointer (option,
                                                           "default_value");
    fset_option_set_value_string (option,
                                  fset_option->type,
                                  ptr_default_value,
                                  1,
                                  &fset_option->default_value);

    /* value */
    if (fset_option->value)
    {
        free (fset_option->value);
        fset_option->value = NULL;
    }
    ptr_value = weechat_config_option_get_pointer (option, "value");
    fset_option_set_value_string (option,
                                  fset_option->type,
                                  ptr_value,
                                  0,
                                  &fset_option->value);

    /* parent_value */
    if (fset_option->parent_value)
    {
        free (fset_option->parent_value);
        fset_option->parent_value = NULL;
    }
    if (ptr_parent_name)
    {
        ptr_parent_option = weechat_config_get (ptr_parent_name);
        if (ptr_parent_option)
        {
            ptr_value = weechat_config_option_get_pointer (ptr_parent_option,
                                                           "value");
            fset_option_set_value_string (ptr_parent_option,
                                          fset_option->type,
                                          ptr_value,
                                          0,
                                          &fset_option->parent_value);
        }
    }

    /* min value */
    if (fset_option->min)
    {
        free (fset_option->min);
        fset_option->min = NULL;
    }
    ptr_min = weechat_config_option_get_pointer (option, "min");
    snprintf (str_value, sizeof (str_value), "%d", *ptr_min);
    fset_option->min = strdup (str_value);

    /* max value */
    if (fset_option->max)
    {
        free (fset_option->max);
        fset_option->max = NULL;
    }
    ptr_max = weechat_config_option_get_pointer (option, "max");
    snprintf (str_value, sizeof (str_value), "%d", *ptr_max);
    fset_option->max = strdup (str_value);

    /* description */
    if (fset_option->description)
    {
        free (fset_option->description);
        fset_option->description = NULL;
    }
    ptr_description = weechat_config_option_get_string (option, "description");
    fset_option->description = strdup ((ptr_description) ? ptr_description : "");

    /* string_values */
    if (fset_option->string_values)
    {
        free (fset_option->string_values);
        fset_option->string_values = NULL;
    }
    ptr_string_values = weechat_config_option_get_pointer (option, "string_values");
    if (ptr_string_values)
    {
        fset_option->string_values = weechat_string_rebuild_split_string (
            ptr_string_values, ",", 0, -1);
    }
    else
    {
        fset_option->string_values = strdup ("");
    }
}

/*
 * Sets max length for fields, for one option.
 */

void
fset_option_set_max_length_fields_option (struct t_fset_option *fset_option)
{
    int length, length_value, length_parent_value;

    /* file */
    length = weechat_utf8_strlen_screen (fset_option->file);
    if (length > fset_option_max_length->file)
        fset_option_max_length->file = length;

    /* section */
    length = weechat_utf8_strlen_screen (fset_option->section);
    if (length > fset_option_max_length->section)
        fset_option_max_length->section = length;

    /* option */
    length = weechat_utf8_strlen_screen (fset_option->option);
    if (length > fset_option_max_length->option)
        fset_option_max_length->option = length;

    /* name */
    length = weechat_utf8_strlen_screen (fset_option->name);
    if (length > fset_option_max_length->name)
        fset_option_max_length->name = length;

    /* parent_name */
    length = (fset_option->parent_name) ?
        weechat_utf8_strlen_screen (fset_option->name) : 0;
    if (length > fset_option_max_length->parent_name)
        fset_option_max_length->parent_name = length;

    /* type */
    length = weechat_utf8_strlen_screen (_(fset_option_type_string[fset_option->type]));
    if (length > fset_option_max_length->type)
        fset_option_max_length->type = length;

    /* type_en */
    length = weechat_utf8_strlen_screen (fset_option_type_string[fset_option->type]);
    if (length > fset_option_max_length->type_en)
        fset_option_max_length->type_en = length;

    /* type_short */
    length = weechat_utf8_strlen_screen (fset_option_type_string_short[fset_option->type]);
    if (length > fset_option_max_length->type_short)
        fset_option_max_length->type_short = length;

    /* type_tiny */
    length = weechat_utf8_strlen_screen (fset_option_type_string_tiny[fset_option->type]);
    if (length > fset_option_max_length->type_tiny)
        fset_option_max_length->type_tiny = length;

    /* default_value */
    if (fset_option->default_value)
    {
        length = weechat_utf8_strlen_screen (fset_option->default_value);
        if (fset_option->type == FSET_OPTION_TYPE_STRING)
            length += 2;
    }
    else
    {
        length = weechat_utf8_strlen_screen (FSET_OPTION_VALUE_NULL);
    }
    if (length > fset_option_max_length->default_value)
        fset_option_max_length->default_value = length;

    /* value */
    if (fset_option->value)
    {
        length_value = weechat_utf8_strlen_screen (fset_option->value);
        if (fset_option->type == FSET_OPTION_TYPE_STRING)
            length_value += 2;
    }
    else
    {
        length_value = weechat_utf8_strlen_screen (FSET_OPTION_VALUE_NULL);
    }
    if (length_value > fset_option_max_length->value)
        fset_option_max_length->value = length_value;

    /* parent_value */
    if (fset_option->parent_value)
    {
        length_parent_value = weechat_utf8_strlen_screen (fset_option->parent_value);
        if (fset_option->type == FSET_OPTION_TYPE_STRING)
            length_parent_value += 2;
    }
    else
    {
        length_parent_value = weechat_utf8_strlen_screen (FSET_OPTION_VALUE_NULL);
    }
    if (length_parent_value > fset_option_max_length->parent_value)
        fset_option_max_length->parent_value = length_parent_value;

    /* value2 */
    length = length_value;
    if (!fset_option->value)
        length += 4 + length_parent_value;
    if (length > fset_option_max_length->value2)
        fset_option_max_length->value2 = length;

    /* min */
    length = weechat_utf8_strlen_screen (fset_option->min);
    if (length > fset_option_max_length->min)
        fset_option_max_length->min = length;

    /* max */
    length = weechat_utf8_strlen_screen (fset_option->max);
    if (length > fset_option_max_length->max)
        fset_option_max_length->max = length;

    /* description */
    length = (fset_option->description && fset_option->description[0]) ?
        weechat_utf8_strlen_screen (_(fset_option->description)) : 0;
    if (length > fset_option_max_length->description)
        fset_option_max_length->description = length;

    /* description2 */
    length = weechat_utf8_strlen_screen (
        (fset_option->description && fset_option->description[0]) ?
        _(fset_option->description) : _("(no description)"));
    if (length > fset_option_max_length->description2)
        fset_option_max_length->description2 = length;

    /* description_en */
    length = weechat_utf8_strlen_screen (fset_option->description);
    if (length > fset_option_max_length->description_en)
        fset_option_max_length->description_en = length;

    /* description_en2 */
    length = weechat_utf8_strlen_screen (
        (fset_option->description && fset_option->description[0]) ?
        fset_option->description : _("(no description)"));
    if (length > fset_option_max_length->description_en2)
        fset_option_max_length->description_en2 = length;

    /* string_values */
    length = weechat_utf8_strlen_screen (fset_option->string_values);
    if (length > fset_option_max_length->string_values)
        fset_option_max_length->string_values = length;

    /* marked */
    length = weechat_utf8_strlen_screen (weechat_config_string (fset_config_look_marked_string));
    if (length > fset_option_max_length->marked)
        fset_option_max_length->marked = length;
    length = weechat_utf8_strlen_screen (weechat_config_string (fset_config_look_unmarked_string));
    if (length > fset_option_max_length->marked)
        fset_option_max_length->marked = length;
}

/*
 * Initializes max length for fields.
 */

void
fset_option_init_max_length (struct t_fset_option_max_length *max_length)
{
    memset (max_length, 0, sizeof (*max_length));
}

/*
 * Sets max length for fields, for all options.
 */

void
fset_option_set_max_length_fields_all ()
{
    int i, num_options;
    struct t_fset_option *ptr_fset_option;

    /* first clear all max lengths */
    fset_option_init_max_length (fset_option_max_length);

    /* set max length for fields, for all options */
    num_options = weechat_arraylist_size (fset_options);
    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (ptr_fset_option)
            fset_option_set_max_length_fields_option (ptr_fset_option);
    }
}

/*
 * Allocates an fset option structure using a pointer to a
 * WeeChat/plugin option.
 *
 * Returns pointer to new fset option, NULL if error.
 */

struct t_fset_option *
fset_option_alloc (struct t_config_option *option)
{
    struct t_fset_option *new_fset_option;

    new_fset_option = malloc (sizeof (*new_fset_option));
    if (!new_fset_option)
        return NULL;

    new_fset_option->index = 0;
    new_fset_option->file = NULL;
    new_fset_option->section = NULL;
    new_fset_option->option = NULL;
    new_fset_option->name = NULL;
    new_fset_option->parent_name = NULL;
    new_fset_option->type = 0;
    new_fset_option->default_value = NULL;
    new_fset_option->value = NULL;
    new_fset_option->parent_value = NULL;
    new_fset_option->min = NULL;
    new_fset_option->max = NULL;
    new_fset_option->description = NULL;
    new_fset_option->string_values = NULL;
    new_fset_option->marked = 0;

    fset_option_set_values (new_fset_option, option);

    return new_fset_option;
}

/*
 * Allocates an fset option structure using a pointer to a
 * WeeChat/plugin option.
 *
 * Returns pointer to new fset option, NULL if the option does not match
 * filters or if error.
 */

struct t_fset_option *
fset_option_add (struct t_config_option *option)
{
    struct t_fset_option *new_fset_option;

    new_fset_option = fset_option_alloc (option);
    if (!new_fset_option)
        return NULL;

    if (!weechat_config_boolean (fset_config_look_show_plugins_desc)
        && (strcmp (new_fset_option->file, "plugins") == 0)
        && (strcmp (new_fset_option->section, "desc") == 0))
    {
        fset_option_free (new_fset_option);
        return NULL;
    }

    /* check if option match filters (if not, ignore it) */
    if (!fset_option_match_filter (new_fset_option, fset_option_filter))
    {
        fset_option_free (new_fset_option);
        return NULL;
    }

    fset_option_set_max_length_fields_option (new_fset_option);

    return new_fset_option;
}

/*
 * Compares two options to sort them by name.
 */

int
fset_option_compare_options_cb (void *data, struct t_arraylist *arraylist,
                                void *pointer1, void *pointer2)
{
    int i, reverse, case_sensitive, rc;
    const char *ptr_field;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    if (!fset_hdata_fset_option)
        return 1;

    for (i = 0; i < fset_config_sort_fields_count; i++)
    {
        reverse = 1;
        case_sensitive = 1;
        ptr_field = fset_config_sort_fields[i];
        while ((ptr_field[0] == '-') || (ptr_field[0] == '~'))
        {
            if (ptr_field[0] == '-')
                reverse *= -1;
            else if (ptr_field[0] == '~')
                case_sensitive ^= 1;
            ptr_field++;
        }
        rc = weechat_hdata_compare (fset_hdata_fset_option,
                                    pointer1, pointer2,
                                    ptr_field,
                                    case_sensitive);
        rc *= reverse;
        if (rc != 0)
            return rc;
    }

    return 1;
}

/*
 * Frees an fset option.
 */

void
fset_option_free (struct t_fset_option *fset_option)
{
    if (!fset_option)
        return;

    if (fset_option->file)
        free (fset_option->file);
    if (fset_option->section)
        free (fset_option->section);
    if (fset_option->option)
        free (fset_option->option);
    if (fset_option->name)
        free (fset_option->name);
    if (fset_option->parent_name)
        free (fset_option->parent_name);
    if (fset_option->default_value)
        free (fset_option->default_value);
    if (fset_option->value)
        free (fset_option->value);
    if (fset_option->parent_value)
        free (fset_option->parent_value);
    if (fset_option->min)
        free (fset_option->min);
    if (fset_option->max)
        free (fset_option->max);
    if (fset_option->description)
        free (fset_option->description);
    if (fset_option->string_values)
        free (fset_option->string_values);

    free (fset_option);
}

/*
 * Frees an fset option (arraylist callback).
 */

void
fset_option_free_cb (void *data, struct t_arraylist *arraylist, void *pointer)
{
    struct t_fset_option *fset_option;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    fset_option = (struct t_fset_option *)pointer;

    fset_option_free (fset_option);
}

/*
 * Allocates and returns the arraylist to store options.
 */

struct t_arraylist *
fset_option_get_arraylist_options ()
{
    return weechat_arraylist_new (100, 1, 0,
                                  &fset_option_compare_options_cb, NULL,
                                  &fset_option_free_cb, NULL);
}

/*
 * Allocates and returns the structure to store max length of fields.
 */

struct t_fset_option_max_length *
fset_option_get_max_length ()
{
    struct t_fset_option_max_length *max_length;

    max_length = malloc (sizeof (*fset_option_max_length));
    if (max_length)
        fset_option_init_max_length (max_length);

    return max_length;
}

/*
 * Gets all options to display in fset buffer.
 */

void
fset_option_get_options ()
{
    struct t_fset_option *new_fset_option, *ptr_fset_option;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    struct t_hashtable *marked_options;
    int i, num_options;

    /* save marked options in a hashtable */
    if (!weechat_config_boolean (fset_config_look_auto_unmark))
    {
        marked_options = weechat_hashtable_new (256,
                                                WEECHAT_HASHTABLE_STRING,
                                                WEECHAT_HASHTABLE_POINTER,
                                                NULL, NULL);
        num_options = weechat_arraylist_size (fset_options);
        for (i = 0; i < num_options; i++)
        {
            ptr_fset_option = weechat_arraylist_get (fset_options, i);
            if (ptr_fset_option && ptr_fset_option->marked)
                weechat_hashtable_set (marked_options, ptr_fset_option->name, NULL);
        }
    }
    else
    {
        marked_options = NULL;
    }

    /* clear options */
    weechat_arraylist_clear (fset_options);
    fset_option_count_marked = 0;
    fset_option_init_max_length (fset_option_max_length);

    /* get options */
    ptr_config = weechat_hdata_get_list (fset_hdata_config_file,
                                         "config_files");
    while (ptr_config)
    {
        ptr_section = weechat_hdata_pointer (fset_hdata_config_file,
                                             ptr_config, "sections");
        while (ptr_section)
        {
            ptr_option = weechat_hdata_pointer (fset_hdata_config_section,
                                                ptr_section, "options");
            while (ptr_option)
            {
                new_fset_option = fset_option_add (ptr_option);
                if (new_fset_option)
                    weechat_arraylist_add (fset_options, new_fset_option);
                ptr_option = weechat_hdata_move (fset_hdata_config_option,
                                                 ptr_option, 1);
            }
            ptr_section = weechat_hdata_move (fset_hdata_config_section,
                                              ptr_section, 1);
        }
        ptr_config = weechat_hdata_move (fset_hdata_config_file,
                                         ptr_config, 1);
    }

    num_options = weechat_arraylist_size (fset_options);

    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (ptr_fset_option)
            ptr_fset_option->index = i;
    }

    /* check selected line */
    if (num_options == 0)
        fset_buffer_selected_line = 0;
    else if (fset_buffer_selected_line >= num_options)
        fset_buffer_selected_line = num_options - 1;

    /* restore marked options */
    if (marked_options)
    {
        for (i = 0; i < num_options; i++)
        {
            ptr_fset_option = weechat_arraylist_get (fset_options, i);
            if (ptr_fset_option
                && weechat_hashtable_has_key (marked_options,
                                              ptr_fset_option->name))
            {
                ptr_fset_option->marked = 1;
                fset_option_count_marked++;
            }
        }
        weechat_hashtable_free (marked_options);
    }
}

/*
 * Sets the filter.
 */

void
fset_option_set_filter (const char *filter)
{
    if (fset_option_filter)
        free (fset_option_filter);
    fset_option_filter = (filter && (strcmp (filter, "*") != 0)) ?
        strdup (filter) : NULL;
}

/*
 * Filters options.
 */

void
fset_option_filter_options (const char *filter)
{
    fset_buffer_selected_line = 0;

    fset_option_set_filter (filter);

    fset_buffer_set_localvar_filter ();

    fset_option_get_options ();

    fset_buffer_refresh (1);
}

/*
 * Toggles a boolean option.
 */

void
fset_option_toggle_value (struct t_fset_option *fset_option,
                          struct t_config_option *option)
{
    if (!fset_option || !option
        || (fset_option->type != FSET_OPTION_TYPE_BOOLEAN))
        return;

    weechat_config_option_set (option, "toggle", 1);
}

/*
 * Adds a value to an integer/color option.
 */

void
fset_option_add_value (struct t_fset_option *fset_option,
                       struct t_config_option *option,
                       int value)
{
    char str_value[128];

    if (!fset_option || !option
        || ((fset_option->type != FSET_OPTION_TYPE_INTEGER)
            && (fset_option->type != FSET_OPTION_TYPE_COLOR)))
        return;

    snprintf (str_value, sizeof (str_value),
              "%s%d",
              (value > 0) ? "++" : "--",
              (value > 0) ? value : value * -1);
    weechat_config_option_set (option, str_value, 1);
}

/*
 * Resets the value of an option.
 */

void
fset_option_reset_value (struct t_fset_option *fset_option,
                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) fset_option;

    if (!option)
        return;

    weechat_config_option_reset (option, 1);
}

/*
 * Unsets the value of an option.
 */

void
fset_option_unset_value (struct t_fset_option *fset_option,
                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) fset_option;

    if (!option)
        return;

    weechat_config_option_unset (option);
}

/*
 * Sets the value of an option.
 *
 * If set_mode == -1, edit an empty value.
 * If set_mode == 0, edit the current value.
 * If set_mode == 1, append to the current value (move the cursor at the end of
 * value)
 */

void
fset_option_set (struct t_fset_option *fset_option,
                 struct t_config_option *option,
                 struct t_gui_buffer *buffer,
                 int set_mode)
{
    int use_mute, add_quotes, length_input, input_pos;
    char *ptr_value, *str_input, str_pos[32];
    char empty_value[1] = { '\0' };

    /* make C compiler happy */
    (void) option;

    if (!fset_option)
        return;

    if (set_mode == -1)
        ptr_value = empty_value;
    else
        ptr_value = (fset_option->value) ? fset_option->value : empty_value;

    length_input = 64 + strlen (fset_option->name) + strlen (ptr_value) + 1;
    str_input = malloc (length_input);
    if (!str_input)
        return;

    use_mute = weechat_config_boolean (fset_config_look_use_mute);
    add_quotes = (fset_option->type == FSET_OPTION_TYPE_STRING) ? 1 : 0;
    snprintf (str_input, length_input,
              "%s/set %s %s%s%s",
              (use_mute) ? "/mute " : "",
              fset_option->name,
              (add_quotes) ? "\"" : "",
              ptr_value,
              (add_quotes) ? "\"" : "");
    weechat_buffer_set (buffer, "input", str_input);
    input_pos = ((use_mute) ? 6 : 0) +  /* "/mute " */
        5 +  /* "/set " */
        weechat_utf8_strlen (fset_option->name) + 1 +
        ((add_quotes) ? 1 : 0) +
        ((set_mode == 1) ? ((fset_option->value) ?
                            weechat_utf8_strlen (fset_option->value) : 0) : 0);
    snprintf (str_pos, sizeof (str_pos), "%d", input_pos);
    weechat_buffer_set (buffer, "input_pos", str_pos);

    free (str_input);
}

/*
 * Marks/unmarks an option.
 */

void
fset_option_toggle_mark (struct t_fset_option *fset_option,
                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) option;

    if (!fset_option)
        return;

    fset_option->marked ^= 1;
    fset_option_count_marked += (fset_option->marked) ? 1 : -1;

    (void) fset_buffer_display_option (fset_option);
}

/*
 * Mark/unmark options matching a filter.
 */

void
fset_option_mark_options_matching_filter (const char *filter, int mark)
{
    int num_options, i, mark_old, matching, set_title;
    struct t_fset_option *ptr_fset_option;

    set_title = 0;

    num_options = weechat_arraylist_size (fset_options);
    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (ptr_fset_option)
        {
            mark_old = ptr_fset_option->marked;
            matching = fset_option_match_filter (ptr_fset_option, filter);
            if (matching)
            {
                if (!mark_old && mark)
                {
                    ptr_fset_option->marked = 1;
                    fset_option_count_marked++;
                    (void) fset_buffer_display_option (ptr_fset_option);
                    set_title = 1;
                }
                else if (mark_old && !mark)
                {
                    ptr_fset_option->marked = 0;
                    fset_option_count_marked--;
                    (void) fset_buffer_display_option (ptr_fset_option);
                    set_title = 1;
                }
            }
        }
    }

    if (set_title)
        fset_buffer_set_title ();
}

/*
 * Unmarks all options.
 */

void
fset_option_unmark_all ()
{
    int num_options, marked, set_title, i;
    struct t_fset_option *ptr_fset_option;

    set_title = 0;

    num_options = weechat_arraylist_size (fset_options);
    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (ptr_fset_option)
        {
            marked = ptr_fset_option->marked;
            ptr_fset_option->marked = 0;
            if (marked)
            {
                (void) fset_buffer_display_option (ptr_fset_option);
                set_title = 1;
            }
        }
    }
    fset_option_count_marked = 0;

    if (set_title)
        fset_buffer_set_title ();
}

/*
 * Exports options currently displayed in fset buffer.
 *
 * If with_help == 1, the help is displayed above each option
 * and options are separated by an empty line.
 *
 * Returns:
 *   1: export OK
 *   0: error
 */

int
fset_option_export (const char *filename, int with_help)
{
    int num_options, i;
    char *line;
    FILE *file;
    struct t_fset_option *ptr_fset_option;
    struct t_hashtable *hashtable_pointers, *hashtable_extra_vars;

    file = fopen (filename, "w");
    if (!file)
        return 0;

    chmod (filename, 0600);

    hashtable_pointers = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL, NULL);
    hashtable_extra_vars = weechat_hashtable_new (
        128,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);

    num_options = weechat_arraylist_size (fset_options);
    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (ptr_fset_option)
        {
            weechat_hashtable_set (hashtable_pointers,
                                   "fset_option", ptr_fset_option);
            fset_option_add_option_in_hashtable (hashtable_extra_vars,
                                                 ptr_fset_option);
            if (with_help)
            {
                if (i > 0)
                    fprintf (file, "\n");
                line = weechat_string_eval_expression (
                    weechat_config_string (fset_config_format_export_help),
                    hashtable_pointers,
                    hashtable_extra_vars,
                    NULL);
                if (line && line[0])
                    fprintf (file, "%s\n", line);
                if (line)
                    free (line);
            }
            line = weechat_string_eval_expression (
                (ptr_fset_option->value) ?
                weechat_config_string (fset_config_format_export_option) :
                weechat_config_string (fset_config_format_export_option_null),
                hashtable_pointers,
                hashtable_extra_vars,
                NULL);
            if (line && line[0])
                fprintf (file, "%s\n", line);
            if (line)
                free (line);
        }
    }

    fclose (file);

    if (hashtable_pointers)
        weechat_hashtable_free (hashtable_pointers);
    if (hashtable_extra_vars)
        weechat_hashtable_free (hashtable_extra_vars);

    return 1;
}

/*
 * Refreshes the fset buffer after the change of an option.
 */

void
fset_option_config_changed (const char *option_name)
{
    struct t_fset_option *ptr_fset_option, *new_fset_option;
    struct t_config_option *ptr_option;
    int option_removed, option_added, line, num_options;
    char *old_name_selected;

    if (!fset_buffer)
        return;

    option_removed = 0;
    option_added = 0;

    ptr_fset_option = weechat_arraylist_get (fset_options,
                                             fset_buffer_selected_line);
    old_name_selected = (ptr_fset_option) ?
        strdup (ptr_fset_option->name) : NULL;

    ptr_fset_option = (option_name) ?
        fset_option_search_by_name (option_name, &line) : NULL;
    ptr_option = (option_name) ? weechat_config_get (option_name) : NULL;


    if (ptr_fset_option)
    {
        if (ptr_option)
        {
            fset_option_set_values (ptr_fset_option, ptr_option);
            (void) fset_buffer_display_option (ptr_fset_option);
        }
        else
        {
            /* option removed: get options and refresh the whole buffer */
            option_removed = 1;
            if (ptr_fset_option->index < fset_buffer_selected_line)
                fset_buffer_selected_line--;
        }
    }
    else if (ptr_option)
    {
        new_fset_option = fset_option_alloc (ptr_option);
        if (fset_option_match_filter (new_fset_option, fset_option_filter))
        {
            /* option added: get options and refresh the whole buffer */
            option_added = 1;
        }
        fset_option_free (new_fset_option);
    }

    if (option_removed || option_added)
    {
        fset_option_get_options ();
        /*
         * in case of option added, we move to the next one if is was the
         * selected one
         */
        if (option_added && old_name_selected)
        {
            ptr_fset_option = weechat_arraylist_get (
                fset_options, fset_buffer_selected_line + 1);
            if (ptr_fset_option
                && (strcmp (old_name_selected, ptr_fset_option->name) == 0))
            {
                fset_buffer_selected_line++;
            }
        }
        fset_buffer_refresh (0);
        fset_buffer_check_line_outside_window ();
    }
    else
    {
        num_options = weechat_arraylist_size (fset_options);
        for (line = 0; line < num_options; line++)
        {
            ptr_fset_option = weechat_arraylist_get (fset_options, line);
            if (ptr_fset_option
                && ptr_fset_option->parent_name
                && option_name
                && (strcmp (ptr_fset_option->parent_name, option_name) == 0))
            {
                ptr_option = weechat_config_get (ptr_fset_option->name);
                if (ptr_option)
                    fset_option_set_values (ptr_fset_option, ptr_option);
            }
        }
        fset_option_set_max_length_fields_all ();
        fset_buffer_refresh (0);
    }

    if (old_name_selected)
        free (old_name_selected);
}

/*
 * Callback called by the timer for each option changed.
 */

void
fset_option_timer_option_changed_cb (void *data,
                                     struct t_hashtable *hashtable,
                                     const void *key,
                                     const void *value)
{
    /* make C compiler happy */
    (void) data;
    (void) hashtable;
    (void) value;

    fset_option_config_changed (key);
}

/*
 * Callback for timer after an option is changed.
 */

int
fset_option_config_timer_cb (const void *pointer,
                             void *data,
                             int remaining_calls)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    if (weechat_hashtable_get_integer (
            fset_option_timer_options_changed,
            "items_count") >= FSET_OPTION_TIMER_MAX_OPTIONS_CHANGED)
    {
        fset_option_get_options ();
        fset_buffer_refresh (1);
    }
    else
    {
        weechat_hashtable_map (fset_option_timer_options_changed,
                               &fset_option_timer_option_changed_cb,
                               NULL);
    }

    weechat_hashtable_remove_all (fset_option_timer_options_changed);

    fset_option_timer_hook = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Callback for config option changed.
 */

int
fset_option_config_cb (const void *pointer,
                       void *data,
                       const char *option,
                       const char *value)
{
    char *info;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) value;

    /* do nothing if fset buffer is not opened */
    if (!fset_buffer)
        return WEECHAT_RC_OK;

    /* do nothing if auto-refresh is disabled for this option */
    if (!weechat_string_match_list (option,
                                    (const char **)fset_config_auto_refresh,
                                    0))
        return WEECHAT_RC_OK;

    /* do nothing if WeeChat is upgrading */
    info = weechat_info_get ("weechat_upgrading", NULL);
    if (info && (strcmp (info, "1") == 0))
    {
        free (info);
        return WEECHAT_RC_OK;
    }
    if (info)
        free (info);

    /*
     * we limit the number of options to display with the timer; for example
     * on /reload, many options are changed, so we'll get all options and
     * display them, instead of change them one by one, which is very slow
     */
    if (weechat_hashtable_get_integer (
            fset_option_timer_options_changed,
            "items_count") < FSET_OPTION_TIMER_MAX_OPTIONS_CHANGED)
    {
        weechat_hashtable_set (fset_option_timer_options_changed,
                               option, NULL);
    }

    if (!fset_option_timer_hook)
    {
        fset_option_timer_hook = weechat_hook_timer (
            1, 0, 1,
            &fset_option_config_timer_cb, NULL, NULL);
    }

    return WEECHAT_RC_OK;
}

/*
 * Returns hdata for option.
 */

struct t_hdata *
fset_option_hdata_option_cb (const void *pointer, void *data,
                             const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, NULL, NULL, 0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_fset_option, index, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, file, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, section, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, option, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, parent_name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, type, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, default_value, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, value, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, parent_value, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, min, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, max, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, description, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, string_values, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, marked, INTEGER, 0, NULL, NULL);
    }
    return hdata;
}

/*
 * Adds a fset option in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
fset_option_add_to_infolist (struct t_infolist *infolist,
                             struct t_fset_option *fset_option)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !fset_option)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_integer (ptr_item, "index", fset_option->index))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "file", fset_option->file))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "section", fset_option->section))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "option", fset_option->option))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "name", fset_option->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "parent_name", fset_option->parent_name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "type", _(fset_option_type_string[fset_option->type])))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "type_en", fset_option_type_string[fset_option->type]))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "default_value", fset_option->default_value))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "value", fset_option->value))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "parent_value", fset_option->parent_value))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "min", fset_option->min))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "max", fset_option->max))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "description",
                                          (fset_option->description && fset_option->description[0]) ?
                                          _(fset_option->description) : ""))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "description_en", fset_option->description))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "string_values", fset_option->description))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "marked", fset_option->marked))
        return 0;

    return 1;
}

/*
 * Prints fset options in WeeChat log file (usually for crash dump).
 */

void
fset_option_print_log ()
{
    struct t_fset_option *ptr_fset_option;
    int num_options, i;

    num_options = weechat_arraylist_size (fset_options);
    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (!ptr_fset_option)
            continue;
        weechat_log_printf ("");
        weechat_log_printf ("[fset option (addr:0x%lx)]", ptr_fset_option);
        weechat_log_printf ("  index . . . . . . . . : %d",    ptr_fset_option->index);
        weechat_log_printf ("  file. . . . . . . . . : '%s'",  ptr_fset_option->file);
        weechat_log_printf ("  section . . . . . . . : '%s'",  ptr_fset_option->section);
        weechat_log_printf ("  option. . . . . . . . : '%s'",  ptr_fset_option->option);
        weechat_log_printf ("  name. . . . . . . . . : '%s'",  ptr_fset_option->name);
        weechat_log_printf ("  parent_name . . . . . : '%s'",  ptr_fset_option->parent_name);
        weechat_log_printf ("  type. . . . . . . . . : %d ('%s')",
                            ptr_fset_option->type,
                            fset_option_type_string[ptr_fset_option->type]);
        weechat_log_printf ("  default_value . . . . : '%s'",  ptr_fset_option->default_value);
        weechat_log_printf ("  value . . . . . . . . : '%s'",  ptr_fset_option->value);
        weechat_log_printf ("  parent_value. . . . . : '%s'",  ptr_fset_option->parent_value);
        weechat_log_printf ("  min . . . . . . . . . : '%s'",  ptr_fset_option->min);
        weechat_log_printf ("  max . . . . . . . . . : '%s'",  ptr_fset_option->max);
        weechat_log_printf ("  description . . . . . : '%s'",  ptr_fset_option->description);
        weechat_log_printf ("  string_values . . . . : '%s'",  ptr_fset_option->string_values);
        weechat_log_printf ("  marked. . . . . . . . : %d",    ptr_fset_option->marked);
    }
}

/*
 * Initializes fset list of options.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
fset_option_init ()
{
    fset_options = fset_option_get_arraylist_options ();
    if (!fset_options)
        return 0;
    fset_option_count_marked = 0;

    fset_option_max_length = fset_option_get_max_length ();
    if (!fset_option_max_length)
    {
        weechat_arraylist_free (fset_options);
        return 0;
    }

    fset_option_filter_hashtable_pointers = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL, NULL);
    if (!fset_option_filter_hashtable_pointers)
    {
        weechat_arraylist_free (fset_options);
        free (fset_option_max_length);
        return 0;
    }
    fset_option_filter_hashtable_extra_vars = weechat_hashtable_new (
        128,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (!fset_option_filter_hashtable_extra_vars)
    {
        weechat_arraylist_free (fset_options);
        free (fset_option_max_length);
        weechat_hashtable_free (fset_option_filter_hashtable_pointers);
        return 0;
    }
    fset_option_filter_hashtable_options = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (!fset_option_filter_hashtable_options)
    {
        weechat_arraylist_free (fset_options);
        free (fset_option_max_length);
        weechat_hashtable_free (fset_option_filter_hashtable_pointers);
        weechat_hashtable_free (fset_option_filter_hashtable_extra_vars);
        return 0;
    }
    weechat_hashtable_set (fset_option_filter_hashtable_options,
                           "type", "condition");

    fset_option_timer_options_changed = weechat_hashtable_new (
        128,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL, NULL);
    if (!fset_option_timer_options_changed)
    {
        weechat_arraylist_free (fset_options);
        free (fset_option_max_length);
        weechat_hashtable_free (fset_option_filter_hashtable_pointers);
        weechat_hashtable_free (fset_option_filter_hashtable_extra_vars);
        weechat_hashtable_free (fset_option_filter_hashtable_options);
        return 0;
    }

    return 1;
}

/*
 * Ends fset list of options.
 */

void
fset_option_end ()
{
    if (fset_options)
    {
        weechat_arraylist_free (fset_options);
        fset_options = NULL;
    }
    fset_option_count_marked = 0;
    if (fset_option_max_length)
    {
        free (fset_option_max_length);
        fset_option_max_length = NULL;
    }
    if (fset_option_filter)
    {
        free (fset_option_filter);
        fset_option_filter = NULL;
    }
    if (fset_option_filter_hashtable_pointers)
    {
        weechat_hashtable_free (fset_option_filter_hashtable_pointers);
        fset_option_filter_hashtable_pointers = NULL;
    }
    if (fset_option_filter_hashtable_extra_vars)
    {
        weechat_hashtable_free (fset_option_filter_hashtable_extra_vars);
        fset_option_filter_hashtable_extra_vars = NULL;
    }
    if (fset_option_filter_hashtable_options)
    {
        weechat_hashtable_free (fset_option_filter_hashtable_options);
        fset_option_filter_hashtable_options = NULL;
    }
    if (fset_option_timer_options_changed)
    {
        weechat_hashtable_free (fset_option_timer_options_changed);
        fset_option_timer_options_changed = NULL;
    }
}
