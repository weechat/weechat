/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* wee-config-option.c: manages WeeChat/protocols options */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "weechat.h"
#include "wee-config-option.h"
#include "wee-hook.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"


/*
 * config_option_get_pos_array_values: return position of a string in an array of values
 *                                     return -1 if not found, otherwise position
 */

int
config_option_get_pos_array_values (char **array, char *string)
{
    int i;
    
    i = 0;
    while (array[i])
    {
        if (string_strcasecmp (array[i], string) == 0)
            return i;
        i++;
    }
    
    /* string not found in array */
    return -1;
}

/*
 * config_option_list_remove: remove an item from a list for an option
 *                            (for options with value like: "abc:1,def:blabla")
 */

void
config_option_list_remove (char **string, char *item)
{
    char *name, *pos, *pos2;
    
    if (!string || !(*string))
        return;
    
    name = (char *) malloc (strlen (item) + 2);
    strcpy (name, item);
    strcat (name, ":");
    pos = string_strcasestr (*string, name);
    free (name);
    if (pos)
    {
        pos2 = pos + strlen (item);
        if (pos2[0] == ':')
        {
            pos2++;
            if (pos2[0])
            {
                while (pos2[0] && (pos2[0] != ','))
                    pos2++;
                if (pos2[0] == ',')
                    pos2++;
                if (!pos2[0] && (pos != (*string)))
                    pos--;
                strcpy (pos, pos2);
                if (!(*string)[0])
                {
                    free (*string);
                    *string = NULL;
                }
                else
                    (*string) = (char *) realloc (*string, strlen (*string) + 1);
            }
        }
    }
}

/*
 * config_option_list_set: set an item from a list for an option
 *                         (for options with value like: "abc:1,def:blabla")
 */

void
config_option_list_set (char **string, char *item, char *value)
{
    config_option_list_remove (string, item);
    
    if (!(*string))
    {
        (*string) = (char *) malloc (strlen (item) + 1 + strlen (value) + 1);
        (*string)[0] = '\0';
    }
    else
        (*string) = (char *) realloc (*string,
                                      strlen (*string) + 1 +
                                      strlen (item) + 1 + strlen (value) + 1);
    
    if ((*string)[0])
        strcat (*string, ",");
    strcat (*string, item);
    strcat (*string, ":");
    strcat (*string, value);
}

/*
 * config_option_list_get_value: return position of item value in the list
 *                               (for options with value like: "abc:1,def:blabla")
 */

void
config_option_list_get_value (char **string, char *item,
                              char **pos_found, int *length)
{
    char *name, *pos, *pos2, *pos_comma;
    
    *pos_found = NULL;
    *length = 0;
    
    if (!string || !(*string))
        return;
    
    name = (char *) malloc (strlen (item) + 2);
    strcpy (name, item);
    strcat (name, ":");
    pos = string_strcasestr (*string, name);
    free (name);
    if (pos)
    {
        pos2 = pos + strlen (item);
        if (pos2[0] == ':')
        {
            pos2++;
            *pos_found = pos2;
            pos_comma = strchr (pos2, ',');
            if (pos_comma)
                *length = pos_comma - pos2;
            else
                *length = strlen (pos2);
        }
    }
}

/*
 * config_option_option_get_boolean_value: get boolean value with user text
 *                                         return: BOOL_FALSE or BOOL_TRUE
 */

int
config_option_option_get_boolean_value (char *text)
{
    if ((string_strcasecmp (text, "on") == 0)
        || (string_strcasecmp (text, "yes") == 0)
        || (string_strcasecmp (text, "y") == 0)
        || (string_strcasecmp (text, "true") == 0)
        || (string_strcasecmp (text, "t") == 0)
        || (string_strcasecmp (text, "1") == 0))
        return BOOL_TRUE;

    if ((string_strcasecmp (text, "off") == 0)
        || (string_strcasecmp (text, "no") == 0)
        || (string_strcasecmp (text, "n") == 0)
        || (string_strcasecmp (text, "false") == 0)
        || (string_strcasecmp (text, "f") == 0)
        || (string_strcasecmp (text, "0") == 0))
        return BOOL_FALSE;
    
    /* invalid text */
    return -1;
}

/*
 * config_option_set: set new value for an option
 *                    return:  0 if success
 *                            -1 if error (bad value)
 */

int
config_option_set (struct t_config_option *option, char *value)
{
    int int_value;
    
    switch (option->type)
    {
        case OPTION_TYPE_BOOLEAN:
            int_value = config_option_option_get_boolean_value (value);
            switch (int_value)
            {
                case BOOL_TRUE:
                    *(option->ptr_int) = BOOL_TRUE;
                    break;
                case BOOL_FALSE:
                    *(option->ptr_int) = BOOL_FALSE;
                    break;
                default:
                    return -1;
            }
            break;
        case OPTION_TYPE_INT:
            int_value = atoi (value);
            if ((int_value < option->min) || (int_value > option->max))
                return -1;
            *(option->ptr_int) = int_value;
            break;
        case OPTION_TYPE_INT_WITH_STRING:
            int_value = config_option_get_pos_array_values (option->array_values,
                                                            value);
            if (int_value < 0)
                return -1;
            *(option->ptr_int) = int_value;
            break;
        case OPTION_TYPE_STRING:
            if ((option->max > 0) && (utf8_strlen (value) > option->max))
                return -1;
            if (*(option->ptr_string))
                free (*(option->ptr_string));
            *(option->ptr_string) = strdup (value);
            break;
        case OPTION_TYPE_COLOR:
            if (!gui_color_assign (option->ptr_int, value))
                return -1;
            break;
    }
    
    hook_config_exec ("weechat", option->name, value);
    
    return 0;
}

/*
 * config_option_search: look for an option in a section and
 *                       return pointer to this option
 *                       If option is not found, NULL is returned
 */

struct t_config_option *
config_option_search (struct t_config_option *config_options, char *option_name)
{
    int i;
    
    for (i = 0; config_options[i].name; i++)
    {
        /* if option found, return pointer */
        if (string_strcasecmp (config_options[i].name, option_name) == 0)
            return &config_options[i];
    }
    
    /* option not found */
    return NULL;
}

/*
 * config_option_section_option_search: look for an option and return pointer to this option
 *                                      if option is not found, NULL is returned
 */

struct t_config_option *
config_option_section_option_search (char **config_sections,
                                     struct t_config_option **config_options,
                                     char *option_name)
{
    int i;
    struct t_config_option *ptr_option;
    
    for (i = 0; config_sections[i]; i++)
    {
        ptr_option = config_option_search (config_options[i],
                                           option_name);
        if (ptr_option)
            return ptr_option;
    }
    
    /* option not found */
    return NULL;
}

/*
 * config_option_section_option_search_get_value: look for type and value of an option
 *                                                if option is not found, NULL is returned
 */

void
config_option_section_option_search_get_value (char **config_sections,
                                               struct t_config_option **config_options,
                                               char *option_name,
                                               struct t_config_option **output_option,
                                               void **output_option_value)
{
    struct t_config_option *ptr_option;
    void *ptr_value;
    
    ptr_option = NULL;
    ptr_value = NULL;
    
    ptr_option = config_option_section_option_search (config_sections,
                                                      config_options,
                                                      option_name);
    if (ptr_option)
    {
        switch (ptr_option->type)
        {
            case OPTION_TYPE_BOOLEAN:
            case OPTION_TYPE_INT:
            case OPTION_TYPE_INT_WITH_STRING:
            case OPTION_TYPE_COLOR:
                ptr_value = (void *)(ptr_option->ptr_int);
                break;
            case OPTION_TYPE_STRING:
                ptr_value = (void *)(ptr_option->ptr_string);
                break;
        }
        if (ptr_option)
        {
            *output_option = ptr_option;
            *output_option_value = ptr_value;
        }
    }
}

/*
 * config_option_section_option_set_value_by_name: set new value for an option (found by name)
 *                                                 return:  0 if success
 *                                                         -1 if bad value for option
 *                                                         -2 if option is not found
 */

int
config_option_section_option_set_value_by_name (char **config_sections,
                                                struct t_config_option **config_options,
                                                char *option_name, char *value)
{
    struct t_config_option *ptr_option;
    
    ptr_option = config_option_section_option_search (config_sections,
                                                      config_options,
                                                      option_name);
    if (ptr_option)
        return config_option_set (ptr_option, value);
    else
        return -2;
}

/*
 * config_option_section_get_index: get section index by name
 *                                  return -1 if section is not found
 */

int
config_option_section_get_index (char **config_sections, char *section_name)
{
    int i;
    
    for (i = 0; config_sections[i]; i++)
    {
        if (string_strcasecmp (config_sections[i], section_name) == 0)
            return i;
    }
    
    /* option not found */
    return -1;
}

/*
 * config_option_section_get_name: get section name by option pointer
 */

char *
config_option_section_get_name (char **config_sections,
                                struct t_config_option **config_options,
                                struct t_config_option *ptr_option)
{
    int i, j;

    for (i = 0; config_sections[i]; i++)
    {
        for (j = 0; config_options[i][j].name; j++)
        {
            /* if option found, return pointer to section name */
            if (ptr_option == &config_options[i][j])
                return config_sections[i];
        }
    }
    
    /* option not found */
    return NULL;
}

/*
 * config_option_section_option_set_default_values: initialize config options
 *                                                  with default values
 */

void
config_option_section_option_set_default_values (char **config_sections,
                                                 struct t_config_option **config_options)
{
    int i, j, int_value;
    
    for (i = 0; config_sections[i]; i++)
    {
        if (config_options[i])
        {
            for (j = 0; config_options[i][j].name; j++)
            {
                switch (config_options[i][j].type)
                {
                    case OPTION_TYPE_BOOLEAN:
                    case OPTION_TYPE_INT:
                        *config_options[i][j].ptr_int =
                            config_options[i][j].default_int;
                        break;
                    case OPTION_TYPE_INT_WITH_STRING:
                        int_value = config_option_get_pos_array_values (
                            config_options[i][j].array_values,
                            config_options[i][j].default_string);
                        if (int_value < 0)
                            gui_chat_printf (NULL,
                                             _("Warning: unable to assign default int with "
                                               "string (\"%s\")\n"),
                                             config_options[i][j].default_string);
                        else
                            *config_options[i][j].ptr_int = int_value;
                        break;
                    case OPTION_TYPE_STRING:
                        *config_options[i][j].ptr_string =
                            strdup (config_options[i][j].default_string);
                        break;
                    case OPTION_TYPE_COLOR:
                        if (!gui_color_assign (config_options[i][j].ptr_int,
                                               config_options[i][j].default_string))
                            gui_chat_printf (NULL,
                                             _("Warning: unable to assign default color "
                                               "(\"%s\")\n"),
                                             config_options[i][j].default_string);
                        break;
                }
            }
        }
    }
}

/*
 * config_option_print_stdout: print options on standard output
 */

void
config_option_print_stdout (char **config_sections,
                            struct t_config_option **config_options)
{
    int i, j, k;
    
    for (i = 0; config_sections[i]; i++)
    {
        if (config_options[i])
        {
            j = 0;
            while (config_options[i][j].name)
            {
                string_iconv_fprintf (stdout,
                                      "* %s:\n",
                                      config_options[i][j].name);
                switch (config_options[i][j].type)
                {
                    case OPTION_TYPE_BOOLEAN:
                        string_iconv_fprintf (stdout, _("  . type: boolean\n"));
                        string_iconv_fprintf (stdout, _("  . values: 'on' or 'off'\n"));
                        string_iconv_fprintf (stdout, _("  . default value: '%s'\n"),
                                              (config_options[i][j].default_int == BOOL_TRUE) ?
                                              "on" : "off");
                        break;
                    case OPTION_TYPE_INT:
                        string_iconv_fprintf (stdout, _("  . type: integer\n"));
                        string_iconv_fprintf (stdout, _("  . values: between %d and %d\n"),
                                              config_options[i][j].min,
                                              config_options[i][j].max);
                        string_iconv_fprintf (stdout, _("  . default value: %d\n"),
                                              config_options[i][j].default_int);
                        break;
                    case OPTION_TYPE_INT_WITH_STRING:
                        string_iconv_fprintf (stdout, _("  . type: string\n"));
                        string_iconv_fprintf (stdout, _("  . values: "));
                        k = 0;
                        while (config_options[i][j].array_values[k])
                        {
                            string_iconv_fprintf (stdout, "'%s'",
                                                  config_options[i][j].array_values[k]);
                            if (config_options[i][j].array_values[k + 1])
                                string_iconv_fprintf (stdout, ", ");
                            k++;
                        }
                        string_iconv_fprintf (stdout, "\n");
                        string_iconv_fprintf (stdout, _("  . default value: '%s'\n"),
                                              (config_options[i][j].default_string) ?
                                              config_options[i][j].default_string : _("empty"));
                        break;
                    case OPTION_TYPE_STRING:
                        switch (config_options[i][j].max)
                        {
                            case 0:
                                string_iconv_fprintf (stdout, _("  . type: string\n"));
                                string_iconv_fprintf (stdout, _("  . values: any string\n"));
                                break;
                            case 1:
                                string_iconv_fprintf (stdout, _("  . type: char\n"));
                                string_iconv_fprintf (stdout, _("  . values: any char\n"));
                                break;
                            default:
                                string_iconv_fprintf (stdout, _("  . type: string\n"));
                                string_iconv_fprintf (stdout, _("  . values: any string (limit: %d chars)\n"),
                                                       config_options[i][j].max);
                                break;
                        }
                        string_iconv_fprintf (stdout, _("  . default value: '%s'\n"),
                                              (config_options[i][j].default_string) ?
                                              config_options[i][j].default_string : _("empty"));
                        break;
                    case OPTION_TYPE_COLOR:
                        string_iconv_fprintf (stdout, _("  . type: color\n"));
                        string_iconv_fprintf (stdout, _("  . values: color (depends on GUI used)\n"));
                        string_iconv_fprintf (stdout, _("  . default value: '%s'\n"),
                                              (config_options[i][j].default_string) ?
                                              config_options[i][j].default_string : _("empty"));
                        break;
                }
                string_iconv_fprintf (stdout, _("  . description: %s\n"),
                                      _(config_options[i][j].description));
                string_iconv_fprintf (stdout, "\n");
                j++;
            }
        }
    }
}
