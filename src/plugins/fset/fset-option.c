/*
 * fset-option.c - manage options displayed by Fast Set buffer
 *
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-option.h"
#include "fset-buffer.h"
#include "fset-config.h"


struct t_arraylist *fset_options = NULL;
struct t_hashtable *fset_option_max_length_field = NULL;
char *fset_option_filter = NULL;


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

    num_options = weechat_arraylist_size (fset_options);
    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (strcmp (ptr_fset_option->name, name) == 0)
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
 * Checks if the option value is different from the default value.
 *
 * Returns:
 *   1: value is different from default value
 *   0: value is the same as default value
 */

int
fset_option_value_different_from_default (struct t_fset_option *fset_option)
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
                              const char *type, void *value,
                              int default_value,
                              char **value_string)
{
    char str_value[64];
    void *ptr_string_values;

    if (!value)
    {
        *value_string = NULL;
    }
    else if (strcmp (type, "boolean") == 0)
    {
        *value_string = strdup (*((int *)value) ? "on" : "off");
    }
    else if (strcmp (type, "integer") == 0)
    {
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
    }
    else if (strcmp (type, "string") == 0)
    {
        *value_string = strdup (
            (default_value) ? weechat_config_string_default (option) : weechat_config_string (option));
    }
    else if (strcmp (type, "color") == 0)
    {
        *value_string = strdup (
            (default_value) ? weechat_config_color_default (option) : weechat_config_color (option));
    }
    else
    {
        *value_string = strdup ("");
    }
}

/*
 * Checks if an option is matching current filter(s).
 *
 * Returns:
 *   1: option is matching filter(s)
 *   0: option does not match filter(s)
 */

int
fset_option_match_filters (const char *option_name)
{
    if (fset_option_filter
        && !weechat_strcasestr (option_name, fset_option_filter))
    {
        return 0;
    }

    return 1;
}

/*
 * Sets (or sets again) values (except name) in an fset option.
 */

void
fset_option_set_values (struct t_fset_option *fset_option,
                        struct t_config_option *option)
{
    const char *ptr_parent_name, *ptr_type;
    void *ptr_default_value, *ptr_value;
    struct t_config_option *ptr_parent_option;

    /* parent name */
    if (fset_option->parent_name)
    {
        free (fset_option->parent_name);
        fset_option->parent_name = NULL;
    }
    ptr_parent_name = weechat_config_option_get_string (option, "parent_name");
    fset_option->parent_name = (ptr_parent_name) ? strdup (ptr_parent_name) : NULL;

    /* type */
    if (fset_option->type)
    {
        free (fset_option->type);
        fset_option->type = NULL;
    }
    ptr_type = weechat_config_option_get_string (option, "type");
    fset_option->type = strdup ((ptr_type) ? ptr_type : "");

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
}

/*
 * Sets max length for a field in hashtable "fset_option_max_length_field".
 */

void
fset_option_set_max_length_field (const char *field, int length)
{
    int *value;

    value = weechat_hashtable_get (fset_option_max_length_field, field);
    if (!value || (length > *value))
        weechat_hashtable_set (fset_option_max_length_field, field, &length);
}

/*
 * Sets max length for fields, for one option.
 */

void
fset_option_set_max_length_fields_option (struct t_fset_option *fset_option)
{
    int length, length_value, length_parent_value;

    /* name */
    fset_option_set_max_length_field (
        "name", weechat_strlen_screen (fset_option->name));

    /* parent_name */
    fset_option_set_max_length_field (
        "parent_name",
        (fset_option->parent_name) ? weechat_strlen_screen (fset_option->parent_name) : 0);

    /* type */
    fset_option_set_max_length_field (
        "type", weechat_strlen_screen (fset_option->type));

    /* default_value */
    if (fset_option->default_value)
    {
        length = weechat_strlen_screen (fset_option->default_value);
        if (strcmp (fset_option->type, "string") == 0)
            length += 2;
    }
    else
    {
        length = weechat_strlen_screen (FSET_OPTION_VALUE_NULL);
    }
    fset_option_set_max_length_field ("default_value", length);

    /* value */
    if (fset_option->value)
    {
        length_value = weechat_strlen_screen (fset_option->value);
        if (strcmp (fset_option->type, "string") == 0)
            length_value += 2;
    }
    else
    {
        length_value = weechat_strlen_screen (FSET_OPTION_VALUE_NULL);
    }
    fset_option_set_max_length_field ("value", length_value);

    /* parent_value */
    if (fset_option->parent_value)
    {
        length_parent_value = weechat_strlen_screen (fset_option->parent_value);
        if (strcmp (fset_option->type, "string") == 0)
            length_parent_value += 2;
    }
    else
    {
        length_parent_value = weechat_strlen_screen (FSET_OPTION_VALUE_NULL);
    }
    fset_option_set_max_length_field ("parent_value", length_parent_value);

    /* value2 */
    length = length_value;
    if (!fset_option->value)
        length += 4 + length_parent_value;
    fset_option_set_max_length_field ("value2", length);
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
    weechat_hashtable_remove_all (fset_option_max_length_field);

    /* set max length for fields, for all options */
    num_options = weechat_arraylist_size (fset_options);
    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        fset_option_set_max_length_fields_option (ptr_fset_option);
    }
}

/*
 * Allocates an fset option structure using a pointer to a
 * WeeChat/plugin option.
 *
 * Returns pointer to new fset option, NULL if the option does not match
 * filters or if error.
 */

struct t_fset_option *
fset_option_alloc (struct t_config_file *config_file,
                   struct t_config_section *section,
                   struct t_config_option *option)
{
    struct t_fset_option *new_fset_option;
    const char *ptr_config_name, *ptr_section_name, *ptr_option_name;
    char *option_name;
    int length;

    new_fset_option = NULL;
    option_name = NULL;

    ptr_config_name = weechat_hdata_string (fset_hdata_config_file,
                                            config_file, "name");
    ptr_section_name = weechat_hdata_string (fset_hdata_config_section,
                                             section, "name");
    ptr_option_name = weechat_hdata_string (fset_hdata_config_option,
                                            option, "name");
    length = strlen (ptr_config_name) + 1 +
        strlen (ptr_section_name) + 1 +
        strlen (ptr_option_name) + 1;
    option_name = malloc (length);
    if (!option_name)
        goto end;
    snprintf (option_name, length, "%s.%s.%s",
              ptr_config_name,
              ptr_section_name,
              ptr_option_name);

    if (!fset_option_match_filters (option_name))
    {
        /* option does not match filters, ignore it */
        goto end;
    }

    new_fset_option = malloc (sizeof (*new_fset_option));
    if (new_fset_option)
    {
        new_fset_option->name = option_name;
        new_fset_option->parent_name = NULL;
        new_fset_option->type = NULL;
        new_fset_option->default_value = NULL;
        new_fset_option->value = NULL;
        new_fset_option->parent_value = NULL;
        fset_option_set_values (new_fset_option, option);
        fset_option_set_max_length_fields_option (new_fset_option);
    }

end:
    if (!new_fset_option)
    {
        if (option_name)
            free (option_name);
    }
    return new_fset_option;
}

/*
 * Compares two options to sort them by name.
 */

int
fset_option_compare_options_old_cb (void *data, struct t_arraylist *arraylist,
                                    void *pointer1, void *pointer2)
{
    struct t_config_option *ptr_option1, *ptr_option2;
    struct t_config_file *ptr_config1, *ptr_config2;
    struct t_config_section *ptr_section1, *ptr_section2;
    const char *ptr_config_name1, *ptr_config_name2;
    const char *ptr_section_name1, *ptr_section_name2;
    const char *ptr_option_name1, *ptr_option_name2;
    int rc;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    ptr_option1 = (struct t_config_option *)pointer1;
    ptr_option2 = (struct t_config_option *)pointer2;

    /* compare config file name */
    ptr_config1 = weechat_hdata_pointer (fset_hdata_config_option,
                                         ptr_option1, "config_file");
    ptr_config_name1 = weechat_hdata_string (fset_hdata_config_file,
                                             ptr_config1, "name");
    ptr_config2 = weechat_hdata_pointer (fset_hdata_config_option,
                                         ptr_option2, "config_file");
    ptr_config_name2 = weechat_hdata_string (fset_hdata_config_file,
                                             ptr_config2, "name");
    rc = weechat_strcasecmp (ptr_config_name1, ptr_config_name2);
    if (rc != 0)
        return rc;

    /* compare section name */
    ptr_section1 = weechat_hdata_pointer (fset_hdata_config_option,
                                          ptr_option1, "section");
    ptr_section_name1 = weechat_hdata_string (fset_hdata_config_section,
                                              ptr_section1, "name");
    ptr_section2 = weechat_hdata_pointer (fset_hdata_config_option,
                                          ptr_option2, "section");
    ptr_section_name2 = weechat_hdata_string (fset_hdata_config_section,
                                              ptr_section2, "name");
    rc = weechat_strcasecmp (ptr_section_name1, ptr_section_name2);
    if (rc != 0)
        return rc;

    /* compare option name */
    ptr_option_name1 = weechat_hdata_string (fset_hdata_config_option,
                                             ptr_option1, "name");
    ptr_option_name2 = weechat_hdata_string (fset_hdata_config_option,
                                             ptr_option2, "name");
    return weechat_strcasecmp (ptr_option_name1, ptr_option_name2);
}

/*
 * Compares two options to sort them by name.
 */

int
fset_option_compare_options_cb (void *data, struct t_arraylist *arraylist,
                                void *pointer1, void *pointer2)
{
    struct t_fset_option *ptr_fset_option1, *ptr_fset_option2;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    ptr_fset_option1 = (struct t_fset_option *)pointer1;
    ptr_fset_option2 = (struct t_fset_option *)pointer2;

    return weechat_strcasecmp (ptr_fset_option1->name,
                               ptr_fset_option2->name);
}

/*
 * Frees an fset option.
 */

void
fset_option_free_cb (void *data, struct t_arraylist *arraylist, void *pointer)
{
    struct t_fset_option *fset_option;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    fset_option = (struct t_fset_option *)pointer;

    if (fset_option->name)
        free (fset_option->name);
    if (fset_option->parent_name)
        free (fset_option->parent_name);
    if (fset_option->type)
        free (fset_option->type);
    if (fset_option->default_value)
        free (fset_option->default_value);
    if (fset_option->value)
        free (fset_option->value);
    if (fset_option->parent_value)
        free (fset_option->parent_value);

    free (fset_option);
}


/*
 * Gets all options to display in fset buffer.
 */

void
fset_option_get_options ()
{
    struct t_fset_option *new_fset_option;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    int num_options;

    weechat_arraylist_clear (fset_options);
    weechat_hashtable_remove_all (fset_option_max_length_field);

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
                new_fset_option = fset_option_alloc (ptr_config, ptr_section,
                                                     ptr_option);
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
    if (num_options == 0)
        fset_buffer_selected_line = 0;
    else if (fset_buffer_selected_line >= num_options)
        fset_buffer_selected_line = num_options - 1;
}

/*
 * Sets filter for fset options.
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
fset_option_filter_options (const char *search)
{
    fset_buffer_selected_line = 0;
    fset_option_set_filter (search);
    fset_option_get_options ();
    fset_buffer_refresh (1);
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
    const char *ptr_info;
    struct t_fset_option *ptr_fset_option;
    struct t_config_option *ptr_option;
    int line, num_options, full_refresh;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) value;

    /* do nothing if fset buffer is not opened */
    if (!fset_buffer)
        return WEECHAT_RC_OK;

    /* do nothing if WeeChat is upgrading */
    ptr_info = weechat_info_get ("weechat_upgrading", NULL);
    if (ptr_info && (strcmp (ptr_info, "1") == 0))
        return WEECHAT_RC_OK;

    full_refresh = 0;

    ptr_fset_option = fset_option_search_by_name (option, &line);
    if (ptr_fset_option)
    {
        ptr_option = weechat_config_get (ptr_fset_option->name);
        if (ptr_option)
        {
            fset_option_set_values (ptr_fset_option, ptr_option);
            fset_buffer_display_line (line, ptr_fset_option);
        }
        else
        {
            /* option removed: get options and refresh the whole buffer */
            full_refresh = 1;
        }
    }
    else
    {
        /* option added: get options and refresh the whole buffer */
        full_refresh = 1;
    }

    if (full_refresh)
    {
        fset_option_get_options ();
        fset_buffer_refresh (1);
    }
    else
    {
        num_options = weechat_arraylist_size (fset_options);
        for (line = 0; line < num_options; line++)
        {
            ptr_fset_option = weechat_arraylist_get (fset_options, line);
            if (ptr_fset_option->parent_name
                && (strcmp (ptr_fset_option->parent_name, option) == 0))
            {
                ptr_option = weechat_config_get (ptr_fset_option->name);
                if (ptr_option)
                {
                    fset_option_set_values (ptr_fset_option, ptr_option);
                    fset_buffer_display_line (line, ptr_fset_option);
                }
            }
        }
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
        WEECHAT_HDATA_VAR(struct t_fset_option, name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, parent_name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, type, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, default_value, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, value, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_fset_option, parent_value, STRING, 0, NULL, NULL);
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

    if (!weechat_infolist_new_var_string (ptr_item, "name", fset_option->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "parent_name", fset_option->parent_name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "type", fset_option->type))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "default_value", fset_option->default_value))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "value", fset_option->value))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "parent_value", fset_option->parent_value))
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
        weechat_log_printf ("");
        weechat_log_printf ("[fset option (addr:0x%lx)]", ptr_fset_option);
        weechat_log_printf ("  name. . . . . . . . . : '%s'",  ptr_fset_option->name);
        weechat_log_printf ("  parent_name . . . . . : '%s'",  ptr_fset_option->parent_name);
        weechat_log_printf ("  type. . . . . . . . . : '%s'",  ptr_fset_option->type);
        weechat_log_printf ("  default_value . . . . : '%s'",  ptr_fset_option->default_value);
        weechat_log_printf ("  value . . . . . . . . : '%s'",  ptr_fset_option->value);
        weechat_log_printf ("  parent_value. . . . . : '%s'",  ptr_fset_option->parent_value);
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
    fset_option_max_length_field = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_INTEGER,
        NULL, NULL);

    fset_options = weechat_arraylist_new (100, 1, 0,
                                          &fset_option_compare_options_cb, NULL,
                                          &fset_option_free_cb, NULL);

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
    if (fset_option_max_length_field)
    {
        weechat_hashtable_free (fset_option_max_length_field);
        fset_option_max_length_field = NULL;
    }
}
