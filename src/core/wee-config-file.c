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

/* wee-config-file.c: manages options in config files */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include "weechat.h"
#include "wee-config-file.h"
#include "wee-log.h"
#include "wee-string.h"
#include "../gui/gui-color.h"
#include "../gui/gui-chat.h"


struct t_config_file *config_files = NULL;
struct t_config_file *last_config_file = NULL;


/*
 * config_file_search: search a configuration file
 */

struct t_config_file *
config_file_search (char *filename)
{
    struct t_config_file *ptr_config;

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if (strcmp (ptr_config->filename, filename) == 0)
            return ptr_config;
    }
    
    /* configuration file not found */
    return NULL;
}

/*
 * config_file_new: create new config options structure
 */

struct t_config_file *
config_file_new (void *plugin, char *filename)
{
    struct t_config_file *new_config_file;
    
    if (!filename)
        return NULL;
    
    /* it's NOT authorized to create two config files with same filename */
    if (config_file_search (filename))
        return NULL;
    
    new_config_file = (struct t_config_file *)malloc (sizeof (struct t_config_file));
    if (new_config_file)
    {
        new_config_file->plugin = plugin;
        new_config_file->filename = strdup (filename);
        new_config_file->file = NULL;
        new_config_file->sections = NULL;
        new_config_file->last_section = NULL;
        
        new_config_file->prev_config = last_config_file;
        new_config_file->next_config = NULL;
        if (config_files)
            last_config_file->next_config = new_config_file;
        else
            config_files = new_config_file;
        last_config_file = new_config_file;
    }
    
    return new_config_file;
}

/*
 * config_file_valid_for_plugin: check if a configuration file pointer exists for a plugin
 *                               return 1 if configuration file exists for plugin
 *                                      0 if configuration file is not found for plugin
 */

int
config_file_valid_for_plugin (void *plugin, struct t_config_file *config_file)
{
    struct t_config_file *ptr_config;
    
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if ((ptr_config == config_file)
            && (ptr_config->plugin == (struct t_weechat_plugin *)plugin))
            return 1;
    }
    
    /* configuration file not found */
    return 0;
}

/*
 * config_file_new_section: create a new section in a config
 */

struct t_config_section *
config_file_new_section (struct t_config_file *config_file, char *name,
                         void (*callback_read)(void *, char *, char *),
                         void (*callback_write)(void *),
                         void (*callback_write_default)(void *))
{
    struct t_config_section *new_section;
    
    if (!config_file || !name)
        return NULL;
    
    new_section = (struct t_config_section *)malloc (sizeof (struct t_config_section));
    if (new_section)
    {
        new_section->name = strdup (name);
        new_section->callback_read = callback_read;
        new_section->callback_write = callback_write;
        new_section->callback_write_default = callback_write_default;
        new_section->options = NULL;
        new_section->last_option = NULL;
        
        new_section->prev_section = config_file->last_section;
        new_section->next_section = NULL;
        if (config_file->sections)
            config_file->last_section->next_section = new_section;
        else
            config_file->sections = new_section;
        config_file->last_section = new_section;
    }
    
    return new_section;
}

/*
 * config_file_search_section: search a section in a config structure
 */

struct t_config_section *
config_file_search_section (struct t_config_file *config_file,
                            char *section_name)
{
    struct t_config_section *ptr_section;

    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        if (string_strcasecmp (ptr_section->name, section_name) == 0)
            return ptr_section;
    }
    
    /* section not found */
    return NULL;
}

/*
 * config_file_new_option_boolean: create a new option, type "boolean"
 *                                 in a config section
 */

struct t_config_option *
config_file_new_option_boolean (struct t_config_section *section, char *name,
                                char *description, int default_value,
                                void (*callback_change)())
{
    struct t_config_option *new_option;
    
    if (!section || !name)
        return NULL;
    
    new_option = (struct t_config_option *)malloc (sizeof (struct t_config_option));
    if (new_option)
    {
        new_option->name = strdup (name);
        new_option->type = CONFIG_OPTION_BOOLEAN;
        new_option->description = (description) ? strdup (description) : NULL;
        new_option->string_values = NULL;
        new_option->min = CONFIG_BOOLEAN_FALSE;
        new_option->max = CONFIG_BOOLEAN_TRUE;
        default_value = (default_value) ?
            CONFIG_BOOLEAN_TRUE : CONFIG_BOOLEAN_FALSE;
        new_option->default_value = malloc (sizeof (char));
        *((char *)new_option->default_value) = default_value;
        new_option->value = malloc (sizeof (char));
        *((char *)new_option->value) = default_value;
        new_option->callback_change = callback_change;
        new_option->loaded = 0;
        
        new_option->prev_option = section->last_option;
        new_option->next_option = NULL;
        if (section->options)
            section->last_option->next_option = new_option;
        else
            section->options = new_option;
        section->last_option = new_option;
    }
    
    return new_option;
}

/*
 * config_file_new_option_integer: create a new option, type "integer"
 *                                 in a config section
 */

struct t_config_option *
config_file_new_option_integer (struct t_config_section *section, char *name,
                                char *description, int min, int max,
                                int default_value, void (*callback_change)())
{
    struct t_config_option *new_option;
    
    if (!section || !name)
        return NULL;
    
    new_option = (struct t_config_option *)malloc (sizeof (struct t_config_option));
    if (new_option)
    {
        new_option->name = strdup (name);
        new_option->type = CONFIG_OPTION_INTEGER;
        new_option->description = (description) ? strdup (description) : NULL;
        new_option->string_values = NULL;
        new_option->min = min;
        new_option->max = max;
        new_option->default_value = malloc (sizeof (int));
        *((int *)new_option->default_value) = default_value;
        new_option->value = malloc (sizeof (int));
        *((int *)new_option->value) = default_value;
        new_option->callback_change = callback_change;
        new_option->loaded = 0;
        
        new_option->prev_option = section->last_option;
        new_option->next_option = NULL;
        if (section->options)
            section->last_option->next_option = new_option;
        else
            section->options = new_option;
        section->last_option = new_option;
    }
    
    return new_option;
}

/*
 * config_file_new_option_integer_with_string: create a new option, type "integer"
 *                                             (with string) in a config section
 */

struct t_config_option *
config_file_new_option_integer_with_string (struct t_config_section *section,
                                            char *name, char *description,
                                            char *string_values,
                                            int default_value,
                                            void (*callback_change)())
{
    struct t_config_option *new_option;
    int argc;
    
    if (!section || !name || !string_values)
        return NULL;
    
    new_option = (struct t_config_option *)malloc (sizeof (struct t_config_option));
    if (new_option)
    {
        new_option->name = strdup (name);
        new_option->type = CONFIG_OPTION_INTEGER;
        new_option->description = (description) ? strdup (description) : NULL;
        new_option->string_values = string_explode (string_values, "|", 0, 0,
                                                    &argc);
        new_option->min = 0;
        new_option->max = (argc == 0) ? 0 : argc - 1;
        new_option->default_value = malloc (sizeof (int));
        *((int *)new_option->default_value) = default_value;
        new_option->value = malloc (sizeof (int));
        *((int *)new_option->value) = default_value;
        new_option->callback_change = callback_change;
        new_option->loaded = 0;
        
        new_option->prev_option = section->last_option;
        new_option->next_option = NULL;
        if (section->options)
            section->last_option->next_option = new_option;
        else
            section->options = new_option;
        section->last_option = new_option;
    }
    
    return new_option;
}

/*
 * config_file_new_option_string: create a new option, type "string"
 *                                in a config section
 */

struct t_config_option *
config_file_new_option_string (struct t_config_section *section,
                               char *name, char *description,
                               int min_length, int max_length,
                               char *default_value, void (*callback_change)())
{
    struct t_config_option *new_option;
    
    if (!section || !name)
        return NULL;
    
    new_option = (struct t_config_option *)malloc (sizeof (struct t_config_option));
    if (new_option)
    {
        new_option->name = strdup (name);
        new_option->type = CONFIG_OPTION_STRING;
        new_option->description = (description) ? strdup (description) : NULL;
        new_option->string_values = NULL;
        new_option->min = min_length;
        new_option->max = max_length;
        new_option->default_value = (default_value) ?
            strdup (default_value) : NULL;
        new_option->value = strdup (default_value) ?
            strdup (default_value) : NULL;
        new_option->callback_change = callback_change;
        new_option->loaded = 0;
        
        new_option->prev_option = section->last_option;
        new_option->next_option = NULL;
        if (section->options)
            section->last_option->next_option = new_option;
        else
            section->options = new_option;
        section->last_option = new_option;
    }
    
    return new_option;
}

/*
 * config_file_new_option_color: create a new option, type "color"
 *                               in a config section
 */

struct t_config_option *
config_file_new_option_color (struct t_config_section *section,
                              char *name, char *description, int color_index,
                              char *default_value, void (*callback_change)())
{
    struct t_config_option *new_option;
    
    if (!section || !name || !default_value)
        return NULL;
    
    new_option = (struct t_config_option *)malloc (sizeof (struct t_config_option));
    if (new_option)
    {
        new_option->name = strdup (name);
        new_option->type = CONFIG_OPTION_COLOR;
        new_option->description = (description) ? strdup (description) : NULL;
        new_option->string_values = NULL;
        new_option->min = color_index;
        new_option->max = color_index;
        new_option->default_value = malloc (sizeof (int));
        if (!gui_color_assign (new_option->default_value, default_value))
            new_option->default_value = 0;
        new_option->value = malloc (sizeof (int));
        *((int *)new_option->value) = *((int *)new_option->default_value);
        new_option->callback_change = callback_change;
        new_option->loaded = 0;
        
        new_option->prev_option = section->last_option;
        new_option->next_option = NULL;
        if (section->options)
            section->last_option->next_option = new_option;
        else
            section->options = new_option;
        section->last_option = new_option;
    }
    
    return new_option;
}

/*
 * config_file_search_option: search an option in a config or section
 */

struct t_config_option *
config_file_search_option (struct t_config_file *config_file,
                           struct t_config_section *section,
                           char *option_name)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    if (section)
    {
        for (ptr_option = section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            if (string_strcasecmp (ptr_option->name, option_name) == 0)
                return ptr_option;
        }
    }
    else if (config_file)
    {
        for (ptr_section = config_file->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if (string_strcasecmp (ptr_option->name, option_name) == 0)
                    return ptr_option;
            }
        }
    }
    
    /* option not found */
    return NULL;
}

/*
 * config_file_option_valid_for_plugin: check if an option pointer exists for a plugin
 *                                      return 1 if option exists for plugin
 *                                             0 if option is not found for plugin
 */

int
config_file_option_valid_for_plugin (void *plugin,
                                     struct t_config_option *option)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if (ptr_config->plugin == (struct t_weechat_plugin *)plugin)
        {
            for (ptr_section = ptr_config->sections; ptr_section;
                 ptr_section = ptr_section->next_section)
            {
                for (ptr_option = ptr_section->options; ptr_option;
                     ptr_option = ptr_option->next_option)
                {
                    if (ptr_option == option)
                        return 1;
                }
            }
        }
    }
    
    /* option not found */
    return 0;
}

/*
 * config_file_string_boolean_value: return boolean value of string
 *                                   return -1 if error
 */

int
config_file_string_boolean_value (char *text)
{
    if ((string_strcasecmp (text, "on") == 0)
        || (string_strcasecmp (text, "yes") == 0)
        || (string_strcasecmp (text, "y") == 0)
        || (string_strcasecmp (text, "true") == 0)
        || (string_strcasecmp (text, "t") == 0)
        || (string_strcasecmp (text, "1") == 0))
        return CONFIG_BOOLEAN_TRUE;
    
    if ((string_strcasecmp (text, "off") == 0)
        || (string_strcasecmp (text, "no") == 0)
        || (string_strcasecmp (text, "n") == 0)
        || (string_strcasecmp (text, "false") == 0)
        || (string_strcasecmp (text, "f") == 0)
        || (string_strcasecmp (text, "0") == 0))
        return CONFIG_BOOLEAN_FALSE;
    
    /* invalid text */
    return -1;
}

/*
 * config_file_option_set: set value for an option
 *                         return: 2 if ok (value changed)
 *                                 1 if ok (value is the same)
 *                                 0 if failed
 */

int
config_file_option_set (struct t_config_option *option, char *new_value)
{
    int new_value_int, i, rc;
    long number;
    char *error;
    
    if (!option)
        return 0;
    
    switch (option->type)
    {
        case CONFIG_OPTION_BOOLEAN:
            if (!new_value)
                return 0;
            new_value_int = config_file_string_boolean_value (new_value);
            if (new_value_int < 0)
                return 0;
            if (new_value_int == *((char *)option->value))
                return 1;
            *((char *)option->value) = new_value_int;
            return 2;
        case CONFIG_OPTION_INTEGER:
            if (!new_value)
                return 0;
            if (option->string_values)
            {
                new_value_int = -1;
                for (i = 0; option->string_values[i]; i++)
                {
                    if (string_strcasecmp (option->string_values[i],
                                           new_value) == 0)
                    {
                        new_value_int = i;
                        break;
                    }
                }
                if (new_value_int < 0)
                    return 0;
                if (new_value_int == *((int *)option->value))
                    return 1;
                *((int *)option->value) = new_value_int;
                return 2;
            }
            else
            {
                error = NULL;
                number = strtol (new_value, &error, 10);
                if (error && (error[0] == '\0'))
                {
                    if (number == *((int *)option->value))
                        return 1;
                    *((int *)option->value) = number;
                    return 2;
                }
            }
            break;
        case CONFIG_OPTION_STRING:
            rc = 1;
            if ((!option->value && new_value)
                || (option->value && !new_value)
                || (strcmp ((char *)option->value, new_value) != 0))
                rc = 2;
            if (option->value)
                free (option->value);
            if (new_value)
            {
                option->value = strdup (new_value);
                if (!option->value)
                    return 0;
            }
            else
                option->value = NULL;
            return rc;
        case CONFIG_OPTION_COLOR:
            if (!gui_color_assign (&new_value_int, new_value))
                return 0;
            if (new_value_int == *((int *)option->value))
                return 1;
            *((int *)option->value) = new_value_int;
            return 2;
    }
    
    return 0;
}

/*
 * config_file_option_reset: set default value for an option
 *                           return: 2 if ok (value changed)
 *                                   1 if ok (value is the same)
 *                                   0 if failed
 */

int
config_file_option_reset (struct t_config_option *option)
{
    int rc;
    
    if (!option)
        return 0;
    
    switch (option->type)
    {
        case CONFIG_OPTION_BOOLEAN:
            if (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_DEFAULT(option))
                return 1;
            CONFIG_BOOLEAN(option) = CONFIG_BOOLEAN_DEFAULT(option);
            return 2;
        case CONFIG_OPTION_INTEGER:
            if (CONFIG_INTEGER(option) == CONFIG_INTEGER_DEFAULT(option))
                return 1;
            CONFIG_INTEGER(option) = CONFIG_INTEGER_DEFAULT(option);
            return 2;
        case CONFIG_OPTION_STRING:
            rc = 1;
            if ((!option->value && option->default_value)
                || (option->value && !option->default_value)
                || (strcmp ((char *)option->value,
                            (char *)option->default_value) != 0))
                rc = 2;
            if (option->value)
                free (option->value);
            if (option->default_value)
            {
                option->value = strdup ((char *)option->default_value);
                if (!option->value)
                    return 0;
            }
            else
                option->value = NULL;
            return rc;
        case CONFIG_OPTION_COLOR:
            if (CONFIG_COLOR(option) == CONFIG_COLOR_DEFAULT(option))
                return 1;
            CONFIG_COLOR(option) = CONFIG_COLOR_DEFAULT(option);
            return 2;
    }
    
    return 0;
}

/*
 * config_file_read: read a configuration file
 *                   return:  0 = successful
 *                           -1 = config file file not found
 *                           -2 = error in config file
 */

int
config_file_read (struct t_config_file *config_file)
{
    int filename_length, line_number, rc;
    char *filename;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char line[1024], *ptr_line, *ptr_line2, *pos, *pos2;
    
    if (!config_file)
        return -1;
    
    filename_length = strlen (weechat_home) + strlen (config_file->filename) + 2;
    filename = (char *) malloc (filename_length * sizeof (char));
    if (!filename)
        return -2;
    snprintf (filename, filename_length, "%s%s%s",
              weechat_home, DIR_SEPARATOR, config_file->filename);
    if ((config_file->file = fopen (filename, "r")) == NULL)
    {
        config_file_write (config_file, 1);
        if ((config_file->file = fopen (filename, "r")) == NULL)
        {
            gui_chat_printf (NULL,
                             _("Warning: config file \"%s\" not found.\n"),
                             filename);
            free (filename);
            return -1;
        }
    }
    
    ptr_section = NULL;
    line_number = 0;
    while (!feof (config_file->file))
    {
        ptr_line = fgets (line, sizeof (line) - 1, config_file->file);
        line_number++;
        if (ptr_line)
        {
            /* encode line to internal charset */
            ptr_line2 = string_iconv_to_internal (NULL, ptr_line);
            if (ptr_line2)
            {
                snprintf (line, sizeof (line) - 1, "%s", ptr_line2);
                free (ptr_line2);
            }
            
            /* skip spaces */
            while (ptr_line[0] == ' ')
                ptr_line++;
            /* not a comment and not an empty line */
            if ((ptr_line[0] != '#') && (ptr_line[0] != '\r')
                && (ptr_line[0] != '\n'))
            {
                /* beginning of section */
                if (ptr_line[0] == '[')
                {
                    pos = strchr (line, ']');
                    if (pos == NULL)
                        gui_chat_printf (NULL,
                                         _("Warning: %s, line %d: invalid "
                                           "syntax, missing \"]\"\n"),
                                         filename, line_number);
                    else
                    {
                        pos[0] = '\0';
                        pos = ptr_line + 1;
                        ptr_section = config_file_search_section (config_file,
                                                                  pos);
                        if (!ptr_section)
                            gui_chat_printf (NULL,
                                             _("Warning: %s, line %d: unknown "
                                               "section identifier "
                                               "(\"%s\")\n"),
                                             filename, line_number, pos);
                    }
                }
                else
                {
                    pos = strchr (line, '=');
                    if (pos == NULL)
                        gui_chat_printf (NULL,
                                         _("Warning: %s, line %d: invalid "
                                           "syntax, missing \"=\"\n"),
                                         filename, line_number);
                    else
                    {
                        pos[0] = '\0';
                        pos++;
                        
                        /* remove spaces before '=' */
                        pos2 = pos - 2;
                        while ((pos2 > line) && (pos2[0] == ' '))
                        {
                            pos2[0] = '\0';
                            pos2--;
                        }
                        
                        /* skip spaces after '=' */
                        while (pos[0] && (pos[0] == ' '))
                        {
                            pos++;
                        }
                        
                        /* remove CR/LF */
                        pos2 = strchr (pos, '\r');
                        if (pos2 != NULL)
                            pos2[0] = '\0';
                        pos2 = strchr (pos, '\n');
                        if (pos2 != NULL)
                            pos2[0] = '\0';
                        
                        /* remove simple or double quotes 
                           and spaces at the end */
                        if (strlen(pos) > 1)
                        {
                            pos2 = pos + strlen (pos) - 1;
                            while ((pos2 > pos) && (pos2[0] == ' '))
                            {
                                pos2[0] = '\0';
                                pos2--;
                            }
                            pos2 = pos + strlen (pos) - 1;
                            if (((pos[0] == '\'') &&
                                 (pos2[0] == '\'')) ||
                                ((pos[0] == '"') &&
                                 (pos2[0] == '"')))
                            {
                                pos2[0] = '\0';
                                pos++;
                            }
                        }
                        
                        ptr_option = config_file_search_option (config_file,
                                                                ptr_section,
                                                                line);
                        if (ptr_option)
                        {
                            rc = config_file_option_set (ptr_option, pos);
                            ptr_option->loaded = 1;
                        }
                        else
                        {
                            /* option not found: use read callback */
                            if (ptr_section && ptr_section->callback_read)
                            {
                                (void) (ptr_section->callback_read) (config_file,
                                                                     line, pos);
                                rc = 1;
                            }
                            else
                                rc = -1;
                        }
                        
                        switch (rc)
                        {
                            case -1:
                                if (ptr_section)
                                    gui_chat_printf (NULL,
                                                     _("Warning: %s, line %d: "
                                                       "option \"%s\" "
                                                       "unknown for "
                                                       "section \"%s\"\n"),
                                                     filename, line_number,
                                                     line, ptr_section->name);
                                else
                                    gui_chat_printf (NULL,
                                                     _("Warning: %s, line %d: "
                                                       "unknown option \"%s\" "
                                                       "(outside a section)\n"),
                                                     filename, line_number,
                                                     line);
                                break;
                            case 0:
                                gui_chat_printf (NULL,
                                                 _("Warning: %s, line %d: "
                                                   "invalid value for option "
                                                   "\"%s\"\n"),
                                                 filename, line_number, line);
                                break;
                            case 2:
                                if (ptr_option && ptr_option->callback_change)
                                    (void) (ptr_option->callback_change) ();
                                break;
                        }
                    }
                }
            }
        }
    }
    
    fclose (config_file->file);
    config_file->file = NULL;
    free (filename);
    
    return 0;
}

/*
 * config_file_reload: reload a configuration file
 *                     return:  0 = successful
 *                             -1 = config file file not found
 *                             -2 = error in config file
 */

int
config_file_reload (struct t_config_file *config_file)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    int rc;

    /* init "loaded" flag for all options */
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        for (ptr_option = ptr_section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            ptr_option->loaded = 0;
        }
    }
    
    /* read configuration file */
    rc = config_file_read (config_file);
    
    /* reset options not found in configuration file */
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        for (ptr_option = ptr_section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            if (!ptr_option->loaded)
            {
                if (config_file_option_reset (ptr_option) == 2)
                {
                    if (ptr_option->callback_change)
                        (void) (ptr_option->callback_change) ();
                }
            }
        }
    }
    
    return rc;
}

/*
 * config_file_write_option: write an option in a config file
 */

void
config_file_write_option (struct t_config_file *config_file,
                          struct t_config_option *option,
                          int default_value)
{
    void *value;
    
    if (!config_file || !config_file->file || !option)
        return;
    
    value = (default_value) ? option->default_value : option->value;
    
    switch (option->type)
    {
        case CONFIG_OPTION_BOOLEAN:
            string_iconv_fprintf (config_file->file, "%s = %s\n",
                                  option->name,
                                  (*((int *)value)) == CONFIG_BOOLEAN_TRUE ?
                                  "on" : "off");
            break;
        case CONFIG_OPTION_INTEGER:
            if (option->string_values)
                string_iconv_fprintf (config_file->file, "%s = %s\n",
                                      option->name,
                                      option->string_values[*((int *)value)]);
            else
                string_iconv_fprintf (config_file->file, "%s = %d\n",
                                      option->name,
                                      *((int *)value));
            break;
        case CONFIG_OPTION_STRING:
            string_iconv_fprintf (config_file->file, "%s = \"%s\"\n",
                                  option->name,
                                  (char *)value);
            break;
        case CONFIG_OPTION_COLOR:
            string_iconv_fprintf (config_file->file, "%s = %s\n",
                                  option->name,
                                  gui_color_get_name (*((int *)value)));
            break;
    }
}

/*
 * config_file_write_line: write a line in a config file
 */

void
config_file_write_line (struct t_config_file *config_file,
                        char *option_name, char *value)
{
    string_iconv_fprintf (config_file->file, "%s = %s\n",
                          option_name, value);
}

/*
 * config_file_write: write a configuration file
 *                    return:  0 if ok
 *                            -1 if error writing file
 *                            -2 if not enough memory
 */

int
config_file_write (struct t_config_file *config_file, int default_options)
{
    int filename_length, rc;
    char *filename, *filename2;
    time_t current_time;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    
    if (!config_file)
        return -1;
    
    filename_length = strlen (weechat_home) +
        strlen (config_file->filename) + 2;
    filename =
        (char *) malloc (filename_length * sizeof (char));
    if (!filename)
        return -2;
    snprintf (filename, filename_length, "%s%s%s",
              weechat_home, DIR_SEPARATOR, config_file->filename);
    
    filename2 = (char *) malloc ((filename_length + 32) * sizeof (char));
    if (!filename2)
    {
        free (filename);
        return -2;
    }
    snprintf (filename2, filename_length + 32, "%s.weechattmp", filename);
    
    if ((config_file->file = fopen (filename2, "w")) == NULL)
    {
        gui_chat_printf (NULL,
                         _("Error: cannot create file \"%s\"\n"),
                         filename2);
        free (filename);
        free (filename2);
        return -1;
    }
    
    current_time = time (NULL);
    string_iconv_fprintf (config_file->file,
                          _("#\n# %s configuration file, created by "
                            "%s v%s on %s"),
                          PACKAGE_NAME, PACKAGE_NAME, PACKAGE_VERSION,
                          ctime (&current_time));
    string_iconv_fprintf (config_file->file,
                          _("# WARNING! Be careful when editing this file, "
                            "WeeChat may write it at any time.\n#\n"));
    
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        /* write section name in file */
        string_iconv_fprintf (config_file->file,
                              "\n[%s]\n", ptr_section->name);
        
        /* call write callback if defined for section */
        if (default_options)
        {
            if (ptr_section->callback_write_default)
                (void) (ptr_section->callback_write_default) (config_file);
        }
        else
        {
            if (ptr_section->callback_write)
                (void) (ptr_section->callback_write) (config_file);
        }
        
        /* write all options for section */
        for (ptr_option = ptr_section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            config_file_write_option (config_file, ptr_option,
                                      default_options);
        }
    }
    
    fclose (config_file->file);
    config_file->file = NULL;
    chmod (filename2, 0600);
    unlink (filename);
    rc = rename (filename2, filename);
    free (filename);
    free (filename2);
    if (rc != 0)
        return -1;
    return 0;
}

/*
 * config_file_option_free: free an option
 */

void
config_file_option_free (struct t_config_section *section,
                         struct t_config_option *option)
{
    struct t_config_option *new_options;
    
    /* remove option */
    if (section->last_option == option)
        section->last_option = option->prev_option;
    if (option->prev_option)
    {
        (option->prev_option)->next_option = option->next_option;
        new_options = section->options;
    }
    else
        new_options = option->next_option;
    
    if (option->next_option)
        (option->next_option)->prev_option = option->prev_option;
    
    /* free data */
    if (option->name)
        free (option->name);
    if (option->description)
        free (option->description);
    if (option->string_values)
        string_free_exploded (option->string_values);
    if (option->default_value)
        free (option->default_value);
    if (option->value)
        free (option->value);
    
    section->options = new_options;
}

/*
 * config_file_section_free: free a section
 */

void
config_file_section_free (struct t_config_file *config_file,
                          struct t_config_section *section)
{
    struct t_config_section *new_sections;
    
    /* remove section */
    if (config_file->last_section == section)
        config_file->last_section = section->prev_section;
    if (section->prev_section)
    {
        (section->prev_section)->next_section = section->next_section;
        new_sections = config_file->sections;
    }
    else
        new_sections = section->next_section;
    
    if (section->next_section)
        (section->next_section)->prev_section = section->prev_section;
    
    /* free data */
    while (section->options)
    {
        config_file_option_free (section, section->options);
    }
    if (section->name)
        free (section->name);
    
    config_file->sections = new_sections;
}

/*
 * config_file_free: free a configuration file
 */

void
config_file_free (struct t_config_file *config_file)
{
    struct t_config_file *new_config_files;
    
    /* remove config file */
    if (last_config_file == config_file)
        last_config_file = config_file->prev_config;
    if (config_file->prev_config)
    {
        (config_file->prev_config)->next_config = config_file->next_config;
        new_config_files = config_files;
    }
    else
        new_config_files = config_file->next_config;
    
    if (config_file->next_config)
        (config_file->next_config)->prev_config = config_file->prev_config;
    
    /* free data */
    while (config_file->sections)
    {
        config_file_section_free (config_file, config_file->sections);
    }
    if (config_file->filename)
        free (config_file->filename);
    
    config_files = new_config_files;
}

/*
 * config_file_free_all: free all configuration files
 */

void
config_file_free_all ()
{
    while (config_files)
    {
        config_file_free (config_files);
    }
}

/*
 * config_file_free_all: free all configuration files for a plugin
 */

void
config_file_free_all_plugin (void *plugin)
{
    struct t_config_file *ptr_config, *next_config;
    
    ptr_config = config_files;
    while (ptr_config)
    {
        next_config = ptr_config->next_config;
        
        if (ptr_config->plugin == plugin)
            config_file_free (ptr_config);
        
        ptr_config = next_config;
    }
}

/*
 * config_file_print_stdout: print options on standard output
 */

void
config_file_print_stdout (struct t_config_file *config_file)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *color_name;
    int i;
    
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        for (ptr_option = ptr_section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            string_iconv_fprintf (stdout,
                                  "* %s:\n",
                                  ptr_option->name);
            switch (ptr_option->type)
            {
                case CONFIG_OPTION_BOOLEAN:
                    string_iconv_fprintf (stdout, _("  . type: boolean\n"));
                    string_iconv_fprintf (stdout, _("  . values: 'on' or 'off'\n"));
                    string_iconv_fprintf (stdout, _("  . default value: '%s'\n"),
                                          (CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                          "on" : "off");
                    break;
                case CONFIG_OPTION_INTEGER:
                    if (ptr_option->string_values)
                    {
                        string_iconv_fprintf (stdout, _("  . type: string\n"));
                        string_iconv_fprintf (stdout, _("  . values: "));
                        i = 0;
                        while (ptr_option->string_values[i])
                        {
                            string_iconv_fprintf (stdout, "'%s'",
                                                  ptr_option->string_values[i]);
                            if (ptr_option->string_values[i + 1])
                                string_iconv_fprintf (stdout, ", ");
                            i++;
                        }
                        string_iconv_fprintf (stdout, "\n");
                        string_iconv_fprintf (stdout, _("  . default value: '%s'\n"),
                                              ptr_option->string_values[CONFIG_INTEGER_DEFAULT(ptr_option)]);
                    }
                    else
                    {
                        string_iconv_fprintf (stdout, _("  . type: integer\n"));
                        string_iconv_fprintf (stdout, _("  . values: between %d and %d\n"),
                                              ptr_option->min,
                                              ptr_option->max);
                        string_iconv_fprintf (stdout, _("  . default value: %d\n"),
                                              CONFIG_INTEGER_DEFAULT(ptr_option));
                    }
                    break;
                case CONFIG_OPTION_STRING:
                    switch (ptr_option->max)
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
                                                  ptr_option->max);
                            break;
                    }
                    string_iconv_fprintf (stdout, _("  . default value: '%s'\n"),
                                          CONFIG_STRING_DEFAULT(ptr_option));
                    break;
                case CONFIG_OPTION_COLOR:
                    color_name = gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option));
                    string_iconv_fprintf (stdout, _("  . type: color\n"));
                    string_iconv_fprintf (stdout, _("  . values: color (depends on GUI used)\n"));
                    string_iconv_fprintf (stdout, _("  . default value: '%s'\n"),
                                          color_name);
                    break;
            }
            string_iconv_fprintf (stdout, _("  . description: %s\n"),
                                  _(ptr_option->description));
            string_iconv_fprintf (stdout, "\n");
        }
    }
}

/*
 * config_file_print_log: print config in log (usually for crash dump)
 */

void
config_file_print_log ()
{
    struct t_config_file *ptr_config_file;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    
    for (ptr_config_file = config_files; ptr_config_file;
         ptr_config_file = ptr_config_file->next_config)
    {
        log_printf ("\n");
        log_printf ("[config (addr:0x%X)]\n", ptr_config_file);
        log_printf ("  filename . . . . . . . : '%s'\n", ptr_config_file->filename);
        log_printf ("  sections . . . . . . . : 0x%X\n", ptr_config_file->sections);
        log_printf ("  last_section . . . . . : 0x%X\n", ptr_config_file->last_section);
        log_printf ("  prev_config. . . . . . : 0x%X\n", ptr_config_file->prev_config);
        log_printf ("  next_config. . . . . . : 0x%X\n", ptr_config_file->next_config);
        
        for (ptr_section = ptr_config_file->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            log_printf ("\n");
            log_printf ("    [section (addr:0x%X)]\n", ptr_section);
            log_printf ("      name . . . . . . . . . : '%s'\n", ptr_section->name);
            log_printf ("      callback_read. . . . . : 0x%X\n", ptr_section->callback_read);
            log_printf ("      callback_write . . . . : 0x%X\n", ptr_section->callback_write);
            log_printf ("      options. . . . . . . . : 0x%X\n", ptr_section->options);
            log_printf ("      last_option. . . . . . : 0x%X\n", ptr_section->last_option);
            log_printf ("      prev_item. . . . . . . : 0x%X\n", ptr_section->prev_section);
            log_printf ("      next_item. . . . . . . : 0x%X\n", ptr_section->next_section);
            
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                log_printf ("\n");
                log_printf ("      [option (addr:0x%X)]\n", ptr_option);
                log_printf ("        name . . . . . . . . : '%s'\n", ptr_option->name);
                log_printf ("        type . . . . . . . . : %d\n",   ptr_option->type);
                log_printf ("        string_values. . . . : 0x%X\n", ptr_option->string_values);
                log_printf ("        min. . . . . . . . . : %d\n",   ptr_option->min);
                log_printf ("        max. . . . . . . . . : %d\n",   ptr_option->max);
                switch (ptr_option->type)
                {
                    case CONFIG_OPTION_BOOLEAN:
                        log_printf ("        value (boolean). . . : %s\n",
                                    (CONFIG_BOOLEAN(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                    "true" : "false");
                        log_printf ("        default value. . . . : %s\n",
                                    (CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                    "true" : "false");
                        break;
                    case CONFIG_OPTION_INTEGER:
                        if (ptr_option->string_values)
                        {
                            log_printf ("        value (integer/str). : '%s'\n",
                                        ptr_option->string_values[CONFIG_INTEGER(ptr_option)]);
                            log_printf ("        default value. . . . : '%s'\n",
                                        ptr_option->string_values[CONFIG_INTEGER_DEFAULT(ptr_option)]);
                        }
                        else
                        {
                            log_printf ("        value (integer). . . : %d\n", CONFIG_INTEGER(ptr_option));
                            log_printf ("        default value. . . . : %d\n", CONFIG_INTEGER_DEFAULT(ptr_option));
                        }
                        break;
                    case CONFIG_OPTION_STRING:
                        log_printf ("        value (string) . . . : '%s'\n", CONFIG_STRING(ptr_option));
                        log_printf ("        default value. . . . : '%s'\n", CONFIG_STRING_DEFAULT(ptr_option));
                        break;
                    case CONFIG_OPTION_COLOR:
                        log_printf ("        value (color). . . . : %d ('%s')\n",
                                    CONFIG_COLOR(ptr_option),
                                    gui_color_get_name (CONFIG_COLOR(ptr_option)));
                        log_printf ("        default value. . . . : %d ('%s')\n",
                                    CONFIG_COLOR_DEFAULT(ptr_option),
                                    gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option)));
                        break;
                }
                log_printf ("        callback_change. . . : 0x%X\n", ptr_option->callback_change);
                log_printf ("        loaded . . . . . . . : %d\n",   ptr_option->loaded);
                log_printf ("        prev_option. . . . . : 0x%X\n", ptr_option->prev_option);
                log_printf ("        next_option. . . . . : 0x%X\n", ptr_option->next_option);
            }
        }
    }
}
