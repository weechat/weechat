/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2006 Emmanuel Bouthenot <kolter@openics.org>
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
 * wee-config-file.c: configuration files/sections/options management
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <limits.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>

#include "weechat.h"
#include "wee-config-file.h"
#include "wee-hdata.h"
#include "wee-hook.h"
#include "wee-infolist.h"
#include "wee-log.h"
#include "wee-string.h"
#include "../gui/gui-color.h"
#include "../gui/gui-chat.h"
#include "../plugins/plugin.h"


struct t_config_file *config_files = NULL;
struct t_config_file *last_config_file = NULL;

char *config_option_type_string[CONFIG_NUM_OPTION_TYPES] =
{ N_("boolean"), N_("integer"), N_("string"), N_("color") };
char *config_boolean_true[] = { "on", "yes", "y", "true", "t", "1", NULL };
char *config_boolean_false[] = { "off", "no", "n", "false", "f", "0", NULL };


void config_file_option_free_data (struct t_config_option *option);


/*
 * config_file_search: search a configuration file
 */

struct t_config_file *
config_file_search (const char *name)
{
    struct t_config_file *ptr_config;

    if (!name)
        return NULL;

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if (string_strcasecmp (ptr_config->name, name) == 0)
            return ptr_config;
    }

    /* configuration file not found */
    return NULL;
}

/*
 * config_file_new: create new configuration options structure
 */

struct t_config_file *
config_file_new (struct t_weechat_plugin *plugin, const char *name,
                 int (*callback_reload)(void *data,
                                        struct t_config_file *config_file),
                 void *callback_reload_data)
{
    struct t_config_file *new_config_file;
    char *filename;
    int length;

    if (!name)
        return NULL;

    /* it's NOT authorized to create two configuration files with same filename */
    if (config_file_search (name))
        return NULL;

    new_config_file = malloc (sizeof (*new_config_file));
    if (new_config_file)
    {
        new_config_file->plugin = plugin;
        new_config_file->name = strdup (name);
        if (!new_config_file->name)
        {
            free (new_config_file);
            return NULL;
        }
        length = strlen (name) + 8 + 1;
        filename = malloc (length);
        if (filename)
        {
            snprintf (filename, length, "%s.conf", name);
            new_config_file->filename = strdup (filename);
            free (filename);
        }
        else
            new_config_file->filename = strdup (name);
        if (!new_config_file->filename)
        {
            free (new_config_file->name);
            free (new_config_file);
            return NULL;
        }
        new_config_file->file = NULL;
        new_config_file->callback_reload = callback_reload;
        new_config_file->callback_reload_data = callback_reload_data;
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
 * config_file_new_section: create a new section in a config
 */

struct t_config_section *
config_file_new_section (struct t_config_file *config_file, const char *name,
                         int user_can_add_options, int user_can_delete_options,
                         int (*callback_read)(void *data,
                                              struct t_config_file *config_file,
                                              struct t_config_section *section,
                                              const char *option_name,
                                              const char *value),
                         void *callback_read_data,
                         int (*callback_write)(void *data,
                                               struct t_config_file *config_file,
                                               const char *section_name),
                         void *callback_write_data,
                         int (*callback_write_default)(void *data,
                                                       struct t_config_file *config_file,
                                                       const char *section_name),
                         void *callback_write_default_data,
                         int (*callback_create_option)(void *data,
                                                       struct t_config_file *config_file,
                                                       struct t_config_section *section,
                                                       const char *option_name,
                                                       const char *value),
                         void *callback_create_option_data,
                         int (*callback_delete_option)(void *data,
                                                       struct t_config_file *config_file,
                                                       struct t_config_section *section,
                                                       struct t_config_option *option),
                         void *callback_delete_option_data)
{
    struct t_config_section *new_section;

    if (!config_file || !name)
        return NULL;

    if (config_file_search_section (config_file, name))
        return NULL;

    new_section = malloc (sizeof (*new_section));
    if (new_section)
    {
        new_section->config_file = config_file;
        new_section->name = strdup (name);
        if (!new_section->name)
        {
            free (new_section);
            return NULL;
        }
        new_section->user_can_add_options = user_can_add_options;
        new_section->user_can_delete_options = user_can_delete_options;
        new_section->callback_read = callback_read;
        new_section->callback_read_data = callback_read_data;
        new_section->callback_write = callback_write;
        new_section->callback_write_data = callback_write_data;
        new_section->callback_write_default = callback_write_default;
        new_section->callback_write_default_data = callback_write_default_data;
        new_section->callback_create_option = callback_create_option;
        new_section->callback_create_option_data = callback_create_option_data;
        new_section->callback_delete_option = callback_delete_option;
        new_section->callback_delete_option_data = callback_delete_option_data;
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
 * config_file_search_section: search a section in a configuration structure
 */

struct t_config_section *
config_file_search_section (struct t_config_file *config_file,
                            const char *section_name)
{
    struct t_config_section *ptr_section;

    if (!config_file || !section_name)
        return NULL;

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
 * config_file_option_full_name: build full name for an option
 */

char *
config_file_option_full_name (struct t_config_option *option)
{
    int length_option;
    char *option_full_name;

    if (!option)
        return NULL;

    length_option = strlen (option->config_file->name) + 1 +
        strlen (option->section->name) + 1 + strlen (option->name) + 1;
    option_full_name = malloc (length_option);
    if (option_full_name)
    {
        snprintf (option_full_name, length_option,
                  "%s.%s.%s",
                  option->config_file->name,
                  option->section->name,
                  option->name);
    }

    return option_full_name;
}

/*
 * config_file_hook_config_exec: execute hook_config for modified option
 */

void
config_file_hook_config_exec (struct t_config_option *option)
{
    char *option_full_name, str_value[256];

    if (option)
    {
        option_full_name = config_file_option_full_name (option);
        if (option_full_name)
        {
            if (option->value)
            {
                switch (option->type)
                {
                    case CONFIG_OPTION_TYPE_BOOLEAN:
                        hook_config_exec (option_full_name,
                                          (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE) ?
                                          "on" : "off");
                        break;
                    case CONFIG_OPTION_TYPE_INTEGER:
                        if (option->string_values)
                            hook_config_exec (option_full_name,
                                              option->string_values[CONFIG_INTEGER(option)]);
                        else
                        {
                            snprintf (str_value, sizeof (str_value),
                                      "%d", CONFIG_INTEGER(option));
                            hook_config_exec (option_full_name, str_value);
                        }
                        break;
                    case CONFIG_OPTION_TYPE_STRING:
                        hook_config_exec (option_full_name, (char *)option->value);
                        break;
                    case CONFIG_OPTION_TYPE_COLOR:
                        hook_config_exec (option_full_name,
                                          gui_color_get_name (CONFIG_COLOR(option)));
                        break;
                    case CONFIG_NUM_OPTION_TYPES:
                        break;
                }
            }
            else
                hook_config_exec (option_full_name, NULL);

            free (option_full_name);
        }
    }
}

/*
 * config_file_option_find_pos: find position for an option in section
 *                              (for sorting options)
 */

struct t_config_option *
config_file_option_find_pos (struct t_config_section *section, const char *name)
{
    struct t_config_option *ptr_option;

    if (section && name)
    {
        for (ptr_option = section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            if (string_strcasecmp (name, ptr_option->name) < 0)
                return ptr_option;
        }
    }

    /* position not found (we will add to the end of list) */
    return NULL;
}

/*
 * config_file_option_insert_in_section: insert option in section (for sorting
 *                                       options)
 */

void
config_file_option_insert_in_section (struct t_config_option *option)
{
    struct t_config_option *pos_option;

    if (!option || !option->section)
        return;

    if (option->section->options)
    {
        pos_option = config_file_option_find_pos (option->section,
                                                  option->name);
        if (pos_option)
        {
            /* insert option into the list (before option found) */
            option->prev_option = pos_option->prev_option;
            option->next_option = pos_option;
            if (pos_option->prev_option)
                (pos_option->prev_option)->next_option = option;
            else
                (option->section)->options = option;
            pos_option->prev_option = option;
        }
        else
        {
            /* add option to end of section */
            option->prev_option = (option->section)->last_option;
            option->next_option = NULL;
            (option->section)->last_option->next_option = option;
            (option->section)->last_option = option;
        }
    }
    else
    {
        /* first option for section */
        option->prev_option = NULL;
        option->next_option = NULL;
        (option->section)->options = option;
        (option->section)->last_option = option;
    }
}

/*
 * config_file_option_malloc: allocate memory for new option and fill it with 0/NULL
 */

struct t_config_option *
config_file_option_malloc ()
{
    struct t_config_option *new_option;

    new_option = malloc (sizeof (*new_option));
    if (new_option)
    {
        new_option->config_file = NULL;
        new_option->section = NULL;
        new_option->name = NULL;
        new_option->type = 0;
        new_option->description = NULL;
        new_option->string_values = NULL;
        new_option->min = 0;
        new_option->max = 0;
        new_option->default_value = NULL;
        new_option->value = NULL;
        new_option->null_value_allowed = 0;
        new_option->callback_check_value = NULL;
        new_option->callback_check_value_data = NULL;
        new_option->callback_change = NULL;
        new_option->callback_change_data = NULL;
        new_option->callback_delete = NULL;
        new_option->callback_delete_data = NULL;
        new_option->loaded = 0;
        new_option->prev_option = NULL;
        new_option->next_option = NULL;
    }

    return new_option;
}

/*
 * config_file_new_option: create a new option
 */

struct t_config_option *
config_file_new_option (struct t_config_file *config_file,
                        struct t_config_section *section, const char *name,
                        const char *type, const char *description,
                        const char *string_values, int min, int max,
                        const char *default_value,
                        const char *value,
                        int null_value_allowed,
                        int (*callback_check_value)(void *data,
                                                    struct t_config_option *option,
                                                    const char *value),
                        void *callback_check_value_data,
                        void (*callback_change)(void *data,
                                                struct t_config_option *option),
                        void *callback_change_data,
                        void (*callback_delete)(void *data,
                                                struct t_config_option *option),
                        void *callback_delete_data)
{
    struct t_config_option *new_option;
    int var_type, int_value, argc, i, index_value;
    long number;
    char *error;

    if (!name)
        return NULL;

    if (config_file && section
        && config_file_search_option (config_file, section, name))
        return NULL;

    var_type = -1;
    for (i = 0; i < CONFIG_NUM_OPTION_TYPES; i++)
    {
        if (string_strcasecmp (type, config_option_type_string[i]) == 0)
        {
            var_type = i;
            break;
        }
    }
    if (var_type < 0)
    {
        gui_chat_printf (NULL, "%sError: unknown option type \"%s\"",
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         type);
        return NULL;
    }

    if (!null_value_allowed)
    {
        if (default_value && !value)
            value = default_value;
        else if (!default_value && value)
            default_value = value;
        if (!default_value || !value)
            return NULL;
    }

    new_option = config_file_option_malloc ();
    if (new_option)
    {
        new_option->config_file = config_file;
        new_option->section = section;
        new_option->name = strdup (name);
        if (!new_option->name)
            goto error;
        new_option->type = var_type;
        if (description)
        {
            new_option->description = strdup (description);
            if (!new_option->description)
                goto error;
        }
        argc = 0;
        switch (var_type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                new_option->min = CONFIG_BOOLEAN_FALSE;
                new_option->max = CONFIG_BOOLEAN_TRUE;
                if (default_value)
                {
                    int_value = config_file_string_to_boolean (default_value);
                    new_option->default_value = malloc (sizeof (int));
                    if (!new_option->default_value)
                        goto error;
                    CONFIG_INTEGER_DEFAULT(new_option) = int_value;
                }
                if (value)
                {
                    int_value = config_file_string_to_boolean (value);
                    new_option->value = malloc (sizeof (int));
                    if (!new_option->value)
                        goto error;
                    CONFIG_INTEGER(new_option) = int_value;
                }
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                if (string_values && string_values[0])
                {
                    new_option->string_values = string_split (string_values,
                                                              "|", 0, 0,
                                                              &argc);
                    if (!new_option->string_values)
                        goto error;
                }
                if (new_option->string_values)
                {
                    new_option->min = 0;
                    new_option->max = (argc == 0) ? 0 : argc - 1;
                    if (default_value)
                    {
                        index_value = 0;
                        for (i = 0; i < argc; i++)
                        {
                            if (string_strcasecmp (new_option->string_values[i],
                                                   default_value) == 0)
                            {
                                index_value = i;
                                break;
                            }
                        }
                        new_option->default_value = malloc (sizeof (int));
                        if (!new_option->default_value)
                            goto error;
                        CONFIG_INTEGER_DEFAULT(new_option) = index_value;
                    }
                    if (value)
                    {
                        index_value = 0;
                        for (i = 0; i < argc; i++)
                        {
                            if (string_strcasecmp (new_option->string_values[i],
                                                   value) == 0)
                            {
                                index_value = i;
                                break;
                            }
                        }
                        new_option->value = malloc (sizeof (int));
                        if (!new_option->value)
                            goto error;
                        CONFIG_INTEGER(new_option) = index_value;
                    }
                }
                else
                {
                    new_option->min = min;
                    new_option->max = max;
                    if (default_value)
                    {
                        error = NULL;
                        number = strtol (default_value, &error, 10);
                        if (!error || error[0])
                            number = 0;
                        if (number < min)
                            number = min;
                        else if (number > max)
                            number = max;
                        new_option->default_value = malloc (sizeof (int));
                        if (!new_option->default_value)
                            goto error;
                        CONFIG_INTEGER_DEFAULT(new_option) = number;
                    }
                    if (value)
                    {
                        error = NULL;
                        number = strtol (value, &error, 10);
                        if (!error || error[0])
                            number = 0;
                        if (number < min)
                            number = min;
                        else if (number > max)
                            number = max;
                        new_option->value = malloc (sizeof (int));
                        if (!new_option->value)
                            goto error;
                        CONFIG_INTEGER(new_option) = number;
                    }
                }
                break;
            case CONFIG_OPTION_TYPE_STRING:
                new_option->min = min;
                new_option->max = max;
                if (default_value)
                {
                    new_option->default_value = strdup (default_value);
                    if (!new_option->default_value)
                        goto error;
                }
                if (value)
                {
                    new_option->value = strdup (value);
                    if (!new_option->value)
                        goto error;
                }
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                new_option->min = min;
                new_option->max = gui_color_get_weechat_colors_number () - 1;
                if (default_value)
                {
                    new_option->default_value = malloc (sizeof (int));
                    if (!new_option->default_value)
                        goto error;
                    if (!gui_color_assign (new_option->default_value, default_value))
                        CONFIG_INTEGER_DEFAULT(new_option) = 0;
                }
                if (value)
                {
                    new_option->value = malloc (sizeof (int));
                    if (!new_option->value)
                        goto error;
                    if (!gui_color_assign (new_option->value, value))
                        CONFIG_INTEGER(new_option) = 0;
                }
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
        new_option->null_value_allowed = null_value_allowed;
        new_option->callback_check_value = callback_check_value;
        new_option->callback_check_value_data = callback_check_value_data;
        new_option->callback_change = callback_change;
        new_option->callback_change_data = callback_change_data;
        new_option->callback_delete = callback_delete;
        new_option->callback_delete_data = callback_delete_data;
        new_option->loaded = 1;

        if (section)
        {
            config_file_option_insert_in_section (new_option);
        }
        else
        {
            new_option->prev_option = NULL;
            new_option->next_option = NULL;
        }

        /* run config hook(s) */
        if (new_option->config_file && new_option->section)
        {
            config_file_hook_config_exec (new_option);
        }
    }

    return new_option;

error:
    if (new_option)
    {
        config_file_option_free_data (new_option);
        free (new_option);
    }
    return NULL;
}

/*
 * config_file_search_option: search an option in a configuration file or section
 */

struct t_config_option *
config_file_search_option (struct t_config_file *config_file,
                           struct t_config_section *section,
                           const char *option_name)
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
 * config_file_search_section_option: search an option in a configuration file
 *                                    or section and return section/option
 */

void
config_file_search_section_option (struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   const char *option_name,
                                   struct t_config_section **section_found,
                                   struct t_config_option **option_found)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    *section_found = NULL;
    *option_found = NULL;

    if (section)
    {
        for (ptr_option = section->options; ptr_option;
             ptr_option = ptr_option->next_option)
        {
            if (string_strcasecmp (ptr_option->name, option_name) == 0)
            {
                *section_found = section;
                *option_found = ptr_option;
                return;
            }
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
                {
                    *section_found = ptr_section;
                    *option_found = ptr_option;
                }
            }
        }
    }
}

/*
 * config_file_search_with_string: search file/section/option for
 *                                 an option with full string
 *                                 (file.section.option)
 */

void
config_file_search_with_string (const char *option_name,
                                struct t_config_file **config_file,
                                struct t_config_section **section,
                                struct t_config_option **option,
                                char **pos_option_name)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *file_name, *pos_section, *section_name, *pos_option;

    if (config_file)
        *config_file = NULL;
    if (section)
        *section = NULL;
    if (option)
        *option = NULL;
    if (pos_option_name)
        *pos_option_name = NULL;

    ptr_config = NULL;
    ptr_section = NULL;
    ptr_option = NULL;

    file_name = NULL;
    section_name = NULL;
    pos_option = NULL;

    pos_section = strchr (option_name, '.');
    pos_option = (pos_section) ? strchr (pos_section + 1, '.') : NULL;
    if (pos_section && pos_option)
    {
        file_name = string_strndup (option_name, pos_section - option_name);
        section_name = string_strndup (pos_section + 1,
                                       pos_option - pos_section - 1);
        pos_option++;
    }
    if (file_name && section_name && pos_option)
    {
        if (pos_option_name)
            *pos_option_name = pos_option;
        ptr_config = config_file_search (file_name);
        if (ptr_config)
        {
            ptr_section = config_file_search_section (ptr_config,
                                                      section_name);
            if (ptr_section)
            {
                ptr_option = config_file_search_option (ptr_config,
                                                        ptr_section,
                                                        pos_option);
            }
        }
    }

    if (file_name)
        free (file_name);
    if (section_name)
        free (section_name);

    if (config_file)
        *config_file = ptr_config;
    if (section)
        *section = ptr_section;
    if (option)
        *option = ptr_option;
}

/*
 * config_file_string_boolean_is_valid: return 1 if boolean is valid, otherwise 0
 */

int
config_file_string_boolean_is_valid (const char *text)
{
    int i;

    if (!text)
        return 0;

    for (i = 0; config_boolean_true[i]; i++)
    {
        if (string_strcasecmp (text, config_boolean_true[i]) == 0)
            return 1;
    }

    for (i = 0; config_boolean_false[i]; i++)
    {
        if (string_strcasecmp (text, config_boolean_false[i]) == 0)
            return 1;
    }

    /* text is not a boolean */
    return 0;
}

/*
 * config_file_string_to_boolean: return boolean value of string
 *                                (1 for true, 0 for false)
 */

int
config_file_string_to_boolean (const char *text)
{
    int i;

    if (!text)
        return CONFIG_BOOLEAN_FALSE;

    for (i = 0; config_boolean_true[i]; i++)
    {
        if (string_strcasecmp (text, config_boolean_true[i]) == 0)
            return CONFIG_BOOLEAN_TRUE;
    }

    return CONFIG_BOOLEAN_FALSE;
}

/*
 * config_file_option_reset: set default value for an option
 *                           return one of these values:
 *                             WEECHAT_CONFIG_OPTION_SET_OK_CHANGED
 *                             WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE
 *                             WEECHAT_CONFIG_OPTION_SET_ERROR
 */

int
config_file_option_reset (struct t_config_option *option, int run_callback)
{
    int rc, old_value_was_null;

    if (!option)
        return WEECHAT_CONFIG_OPTION_SET_ERROR;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option->default_value)
    {
        old_value_was_null = (option->value == NULL);
        switch (option->type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                if (!option->value)
                {
                    option->value = malloc (sizeof (int));
                    if (option->value)
                    {
                        CONFIG_BOOLEAN(option) = CONFIG_BOOLEAN_DEFAULT(option);
                        rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                    }
                }
                else
                {
                    if (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_DEFAULT(option))
                        rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                    else
                    {
                        CONFIG_BOOLEAN(option) = CONFIG_BOOLEAN_DEFAULT(option);
                        rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                    }
                }
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                if (!option->value)
                {
                    option->value = malloc (sizeof (int));
                    if (option->value)
                        CONFIG_INTEGER(option) = 0;
                }
                if (CONFIG_INTEGER(option) == CONFIG_INTEGER_DEFAULT(option))
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                else
                {
                    CONFIG_INTEGER(option) = CONFIG_INTEGER_DEFAULT(option);
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                }
                break;
            case CONFIG_OPTION_TYPE_STRING:
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                if (!option->value
                    || (strcmp ((char *)option->value,
                                (char *)option->default_value) != 0))
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                if (option->value)
                {
                    free (option->value);
                    option->value = NULL;
                }
                option->value = strdup ((char *)option->default_value);
                if (!option->value)
                    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                if (!option->value)
                {
                    option->value = malloc (sizeof (int));
                    if (option->value)
                        CONFIG_INTEGER(option) = 0;
                }
                if (CONFIG_COLOR(option) == CONFIG_COLOR_DEFAULT(option))
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                else
                {
                    CONFIG_COLOR(option) = CONFIG_COLOR_DEFAULT(option);
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                }
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
        if (old_value_was_null && option->value)
            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
    }
    else
    {
        if (option->null_value_allowed)
        {
            if (option->value)
            {
                free (option->value);
                option->value = NULL;
                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    if ((rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
        && run_callback && option->callback_change)
    {
        (void)(option->callback_change)(option->callback_change_data, option);
    }

    /* run config hook(s) */
    if ((rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
        && option->config_file && option->section)
    {
        config_file_hook_config_exec (option);
    }

    return rc;
}

/*
 * config_file_option_set: set value for an option
 *                         return one of these values:
 *                           WEECHAT_CONFIG_OPTION_SET_OK_CHANGED
 *                           WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE
 *                           WEECHAT_CONFIG_OPTION_SET_ERROR
 */

int
config_file_option_set (struct t_config_option *option, const char *value,
                        int run_callback)
{
    int value_int, i, rc, new_value_ok, old_value_was_null, old_value;
    long number;
    char *error;

    if (!option)
        return WEECHAT_CONFIG_OPTION_SET_ERROR;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option->callback_check_value)
    {
        if (!(int)(option->callback_check_value)
            (option->callback_check_value_data,
             option,
             value))
            return WEECHAT_CONFIG_OPTION_SET_ERROR;
    }

    if (value)
    {
        old_value_was_null = (option->value == NULL);
        switch (option->type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                if (!option->value)
                {
                    option->value = malloc (sizeof (int));
                    if (option->value)
                    {
                        if (string_strcasecmp (value, "toggle") == 0)
                        {
                            CONFIG_BOOLEAN(option) = CONFIG_BOOLEAN_TRUE;
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                        }
                        else
                        {
                            if (config_file_string_boolean_is_valid (value))
                            {
                                value_int = config_file_string_to_boolean (value);
                                CONFIG_BOOLEAN(option) = value_int;
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                            }
                            else
                            {
                                free (option->value);
                                option->value = NULL;
                            }
                        }
                    }
                }
                else
                {
                    if (string_strcasecmp (value, "toggle") == 0)
                    {
                        CONFIG_BOOLEAN(option) =
                            (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE) ?
                            CONFIG_BOOLEAN_FALSE : CONFIG_BOOLEAN_TRUE;
                        rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                    }
                    else
                    {
                        if (config_file_string_boolean_is_valid (value))
                        {
                            value_int = config_file_string_to_boolean (value);
                            if (value_int == CONFIG_BOOLEAN(option))
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                            else
                            {
                                CONFIG_BOOLEAN(option) = value_int;
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                            }
                        }
                    }
                }
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                old_value = 0;
                if (!option->value)
                    option->value = malloc (sizeof (int));
                else
                    old_value = CONFIG_INTEGER(option);
                if (option->value)
                {
                    if (option->string_values)
                    {
                        value_int = -1;
                        if (strncmp (value, "++", 2) == 0)
                        {
                            error = NULL;
                            number = strtol (value + 2, &error, 10);
                            if (error && !error[0])
                            {
                                number = number % (option->max + 1);
                                value_int = (old_value + number) %
                                    (option->max + 1);
                            }
                        }
                        else if (strncmp (value, "--", 2) == 0)
                        {
                            error = NULL;
                            number = strtol (value + 2, &error, 10);
                            if (error && !error[0])
                            {
                                number = number % (option->max + 1);
                                value_int = (old_value + (option->max + 1) - number) %
                                    (option->max + 1);
                            }
                        }
                        else
                        {
                            for (i = 0; option->string_values[i]; i++)
                            {
                                if (string_strcasecmp (option->string_values[i],
                                                       value) == 0)
                                {
                                    value_int = i;
                                    break;
                                }
                            }
                        }
                        if (value_int >= 0)
                        {
                            if (old_value_was_null
                                || (value_int != old_value))
                            {
                                CONFIG_INTEGER(option) = value_int;
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                            }
                            else
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                        }
                        else
                        {
                            if (old_value_was_null)
                            {
                                free (option->value);
                                option->value = NULL;
                            }
                        }
                    }
                    else
                    {
                        new_value_ok = 0;
                        if (strncmp (value, "++", 2) == 0)
                        {
                            error = NULL;
                            number = strtol (value + 2, &error, 10);
                            if (error && !error[0])
                            {
                                value_int = old_value + number;
                                if (value_int <= option->max)
                                    new_value_ok = 1;
                            }
                        }
                        else if (strncmp (value, "--", 2) == 0)
                        {
                            error = NULL;
                            number = strtol (value + 2, &error, 10);
                            if (error && !error[0])
                            {
                                value_int = old_value - number;
                                if (value_int >= option->min)
                                    new_value_ok = 1;
                            }
                        }
                        else
                        {
                            error = NULL;
                            number = strtol (value, &error, 10);
                            if (error && !error[0])
                            {
                                value_int = number;
                                if ((value_int >= option->min)
                                    && (value_int <= option->max))
                                    new_value_ok = 1;
                            }
                        }
                        if (new_value_ok)
                        {
                            if (old_value_was_null
                                || (value_int != old_value))
                            {
                                CONFIG_INTEGER(option) = value_int;
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                            }
                            else
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                        }
                        else
                        {
                            if (old_value_was_null)
                            {
                                free (option->value);
                                option->value = NULL;
                            }
                        }
                    }
                }
                break;
            case CONFIG_OPTION_TYPE_STRING:
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                if (!option->value
                    || (strcmp (CONFIG_STRING(option), value) != 0))
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                if (option->value)
                {
                    free (option->value);
                    option->value = NULL;
                }
                option->value = strdup (value);
                if (!option->value)
                    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                old_value = 0;
                if (!option->value)
                    option->value = malloc (sizeof (int));
                else
                    old_value = CONFIG_COLOR(option);
                if (option->value)
                {
                    value_int = -1;
                    new_value_ok = 0;
                    if (strncmp (value, "++", 2) == 0)
                    {
                        error = NULL;
                        number = strtol (value + 2, &error, 10);
                        if (error && !error[0])
                        {
                            if (gui_color_assign_by_diff (&value_int,
                                                          gui_color_get_name (old_value),
                                                          number))
                                new_value_ok = 1;
                        }
                    }
                    else if (strncmp (value, "--", 2) == 0)
                    {
                        error = NULL;
                        number = strtol (value + 2, &error, 10);
                        if (error && !error[0])
                        {
                            if (gui_color_assign_by_diff (&value_int,
                                                          gui_color_get_name (old_value),
                                                          -1 * number))
                                new_value_ok = 1;
                        }
                    }
                    else
                    {
                        if (gui_color_assign (&value_int, value))
                            new_value_ok = 1;
                    }
                    if (new_value_ok)
                    {
                        if (old_value_was_null
                            || (value_int != old_value))
                        {
                            CONFIG_COLOR(option) = value_int;
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                        }
                        else
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                    }
                }
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
        if (old_value_was_null && option->value)
            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
    }
    else
    {
        if (option->null_value_allowed && option->value)
        {
            free (option->value);
            option->value = NULL;
            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
        }
        else
            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    }

    /* run callback if asked and value was changed */
    if ((rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
        && run_callback && option->callback_change)
    {
        (void)(option->callback_change)(option->callback_change_data, option);
    }

    /* run config hook(s) */
    if ((rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
        && option->config_file && option->section)
    {
        config_file_hook_config_exec (option);
    }

    return rc;
}

/*
 * config_file_option_set_null: set null (undefined) value for an option
 *                              return one of these values:
 *                                WEECHAT_CONFIG_OPTION_SET_OK_CHANGED
 *                                WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE
 *                                WEECHAT_CONFIG_OPTION_SET_ERROR
 */

int
config_file_option_set_null (struct t_config_option *option, int run_callback)
{
    int rc;

    if (!option)
        return WEECHAT_CONFIG_OPTION_SET_ERROR;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    /* null value is authorized only if it's allowed in option */
    if (option->null_value_allowed)
    {
        /* option was already null: do nothing */
        if (!option->value)
            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        else
        {
            /* set option to null */
            free (option->value);
            option->value = NULL;
            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
        }
    }

    /* run callback if asked and value was changed */
    if ((rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
        && run_callback && option->callback_change)
    {
        (void)(option->callback_change)(option->callback_change_data, option);
    }

    /* run config hook(s) */
    if ((rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
        && option->config_file && option->section)
    {
        config_file_hook_config_exec (option);
    }

    return rc;
}

/*
 * config_file_option_unset: unset/reset option
 *                           return one of these values:
 *                             WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET
 *                             WEECHAT_CONFIG_OPTION_UNSET_OK_RESET
 *                             WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED
 *                             WEECHAT_CONFIG_OPTION_UNSET_ERROR
 */

int
config_file_option_unset (struct t_config_option *option)
{
    int rc;
    char *option_full_name;

    if (!option)
        return WEECHAT_CONFIG_OPTION_UNSET_ERROR;

    rc = WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET;

    if (option->section && option->section->user_can_delete_options)
    {
        /* delete option */
        if (option->callback_delete)
        {
            (void)(option->callback_delete)
                (option->callback_delete_data,
                 option);
        }

        option_full_name = config_file_option_full_name (option);

        if (option->section->callback_delete_option)
        {
            rc = (int)(option->section->callback_delete_option)
                (option->section->callback_delete_option_data,
                 option->config_file,
                 option->section,
                 option);
        }
        else
        {
            config_file_option_free (option);
            rc = WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
        }

        if (option_full_name)
        {
            hook_config_exec (option_full_name, NULL);
            free (option_full_name);
        }
    }
    else
    {
        /* reset value */
        switch (config_file_option_reset (option, 1))
        {
            case WEECHAT_CONFIG_OPTION_SET_ERROR:
                rc = WEECHAT_CONFIG_OPTION_UNSET_ERROR;
                break;
            case WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE:
                rc = WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET;
                break;
            case WEECHAT_CONFIG_OPTION_SET_OK_CHANGED:
                rc = WEECHAT_CONFIG_OPTION_UNSET_OK_RESET;
                break;
        }
    }

    return rc;
}

/*
 * config_file_option_rename: rename an option
 */

void
config_file_option_rename (struct t_config_option *option,
                           const char *new_name)
{
    char *str_new_name;

    if (!option || !new_name || !new_name[0]
        || config_file_search_option (option->config_file, option->section, new_name))
        return;

    str_new_name = strdup (new_name);
    if (str_new_name)
    {
        /* remove option from list */
        if (option->section)
        {
            if (option->prev_option)
                (option->prev_option)->next_option = option->next_option;
            if (option->next_option)
                (option->next_option)->prev_option = option->prev_option;
            if (option->section->options == option)
                (option->section)->options = option->next_option;
            if (option->section->last_option == option)
                (option->section)->last_option = option->prev_option;
        }

        /* rename option */
        if (option->name)
            free (option->name);
        option->name = str_new_name;

        /* re-insert option in section */
        if (option->section)
            config_file_option_insert_in_section (option);
    }
}

/*
 * config_file_option_get_pointer: get a pointer of an option property
 */

void *
config_file_option_get_pointer (struct t_config_option *option,
                                const char *property)
{
    if (!option || !property)
        return NULL;

    if (string_strcasecmp (property, "config_file") == 0)
        return option->config_file;
    else if (string_strcasecmp (property, "section") == 0)
        return option->section;
    else if (string_strcasecmp (property, "name") == 0)
        return option->name;
    else if (string_strcasecmp (property, "type") == 0)
        return &option->type;
    else if (string_strcasecmp (property, "description") == 0)
        return option->description;
    else if (string_strcasecmp (property, "string_values") == 0)
        return option->string_values;
    else if (string_strcasecmp (property, "min") == 0)
        return &option->min;
    else if (string_strcasecmp (property, "max") == 0)
        return &option->max;
    else if (string_strcasecmp (property, "default_value") == 0)
        return option->default_value;
    else if (string_strcasecmp (property, "value") == 0)
        return option->value;
    else if (string_strcasecmp (property, "prev_option") == 0)
        return option->prev_option;
    else if (string_strcasecmp (property, "next_option") == 0)
        return option->next_option;

    return NULL;
}

/*
 * config_file_option_is_null: return 1 if value of option is null
 *                                    0 if it is not null
 */

int
config_file_option_is_null (struct t_config_option *option)
{
    if (!option)
        return 1;

    return (option->value) ? 0 : 1;
}

/*
 * config_file_option_default_is_null: return 1 if default value of option is null
 *                                            0 if it is not null
 */

int
config_file_option_default_is_null (struct t_config_option *option)
{
    if (!option)
        return 1;

    return (option->default_value) ? 0 : 1;
}

/*
 * config_file_option_set_with_string: set value for an option (with a string
 *                                     for name of option)
 *                                     return one of these values:
 *                                       WEECHAT_CONFIG_OPTION_SET_OK_CHANGED
 *                                       WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE
 *                                       WEECHAT_CONFIG_OPTION_SET_ERROR
 *                                       WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND
 */

int
config_file_option_set_with_string (const char *option_name, const char *value)
{
    int rc;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *pos_option;

    rc = WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND;

    config_file_search_with_string (option_name, &ptr_config, &ptr_section,
                                    &ptr_option, &pos_option);

    if (ptr_config && ptr_section)
    {
        if (ptr_option)
        {
            rc = (value) ?
                config_file_option_set (ptr_option, value, 1) :
                config_file_option_set_null (ptr_option, 1);
        }
        else
        {
            if (ptr_section->user_can_add_options
                && ptr_section->callback_create_option)
            {
                rc = (int)(ptr_section->callback_create_option)
                    (ptr_section->callback_create_option_data,
                     ptr_config,
                     ptr_section,
                     pos_option,
                     value);
            }
        }
    }

    return rc;
}

/*
 * config_file_option_boolean: return boolean value of an option
 */

int
config_file_option_boolean (struct t_config_option *option)
{
    if (!option)
        return 0;

    if (option->type == CONFIG_OPTION_TYPE_BOOLEAN)
        return CONFIG_BOOLEAN(option);
    else
        return 0;
}

/*
 * config_file_option_boolean_default: return default boolean value of an option
 */

int
config_file_option_boolean_default (struct t_config_option *option)
{
    if (!option)
        return 0;

    if (option->type == CONFIG_OPTION_TYPE_BOOLEAN)
        return CONFIG_BOOLEAN_DEFAULT(option);
    else
        return 0;
}

/*
 * config_file_option_integer: return integer value of an option
 */

int
config_file_option_integer (struct t_config_option *option)
{
    if (!option)
        return 0;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE)
                return 1;
            else
                return 0;
        case CONFIG_OPTION_TYPE_INTEGER:
        case CONFIG_OPTION_TYPE_COLOR:
            return CONFIG_INTEGER(option);
        case CONFIG_OPTION_TYPE_STRING:
            return 0;
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
    return 0;
}

/*
 * config_file_option_integer_default: return default integer value of an option
 */

int
config_file_option_integer_default (struct t_config_option *option)
{
    if (!option)
        return 0;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN_DEFAULT(option) == CONFIG_BOOLEAN_TRUE)
                return 1;
            else
                return 0;
        case CONFIG_OPTION_TYPE_INTEGER:
        case CONFIG_OPTION_TYPE_COLOR:
            return CONFIG_INTEGER_DEFAULT(option);
        case CONFIG_OPTION_TYPE_STRING:
            return 0;
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
    return 0;
}

/*
 * config_file_option_string: return string value of an option
 */

const char *
config_file_option_string (struct t_config_option *option)
{
    if (!option)
        return NULL;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN(option))
                return config_boolean_true[0];
            else
                return config_boolean_false[0];
        case CONFIG_OPTION_TYPE_INTEGER:
            if (option->string_values)
                return option->string_values[CONFIG_INTEGER(option)];
            return NULL;
        case CONFIG_OPTION_TYPE_STRING:
            return CONFIG_STRING(option);
        case CONFIG_OPTION_TYPE_COLOR:
            return gui_color_get_name (CONFIG_COLOR(option));
        case CONFIG_NUM_OPTION_TYPES:
            return NULL;
    }
    return NULL;
}

/*
 * config_file_option_string_default: return default string value of an option
 */

const char *
config_file_option_string_default (struct t_config_option *option)
{
    if (!option)
        return NULL;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN_DEFAULT(option))
                return config_boolean_true[0];
            else
                return config_boolean_false[0];
        case CONFIG_OPTION_TYPE_INTEGER:
            if (option->string_values)
                return option->string_values[CONFIG_INTEGER_DEFAULT(option)];
            return NULL;
        case CONFIG_OPTION_TYPE_STRING:
            return CONFIG_STRING_DEFAULT(option);
        case CONFIG_OPTION_TYPE_COLOR:
            return gui_color_get_name (CONFIG_COLOR_DEFAULT(option));
        case CONFIG_NUM_OPTION_TYPES:
            return NULL;
    }
    return NULL;
}

/*
 * config_file_option_color: return color value of an option
 */

const char *
config_file_option_color (struct t_config_option *option)
{
    if (!option)
        return NULL;

    return gui_color_get_name (CONFIG_COLOR(option));
}

/*
 * config_file_option_color_default: return default color value of an option
 */

const char *
config_file_option_color_default (struct t_config_option *option)
{
    if (!option)
        return NULL;

    return gui_color_get_name (CONFIG_COLOR_DEFAULT(option));
}

/*
 * config_file_option_escape: return "\" if name of option must be escaped,
 *                            or empty string if option must not be escaped
 *                            The option is escaped if it is begining with one
 *                            of these chars: # [ \
 */

const char *
config_file_option_escape (const char *name)
{
    static char str_escaped[2] = "\\", str_not_escaped[1] = { '\0' };

    if (!name)
        return str_escaped;

    if ((name[0] == '#') || (name[0] == '[') || (name[0] == '\\'))
        return str_escaped;

    return str_not_escaped;
}

/*
 * config_file_write_option: write an option in a configuration file
 *                           return 1 if ok, 0 if error
 */

int
config_file_write_option (struct t_config_file *config_file,
                          struct t_config_option *option)
{
    int rc;

    if (!config_file || !config_file->file || !option)
        return 0;

    rc = 1;

    if (option->value)
    {
        switch (option->type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                rc = string_iconv_fprintf (config_file->file, "%s%s = %s\n",
                                           config_file_option_escape (option->name),
                                           option->name,
                                           (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE) ?
                                           "on" : "off");
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                if (option->string_values)
                    rc = string_iconv_fprintf (config_file->file, "%s%s = %s\n",
                                               config_file_option_escape (option->name),
                                               option->name,
                                               option->string_values[CONFIG_INTEGER(option)]);
                else
                    rc = string_iconv_fprintf (config_file->file, "%s%s = %d\n",
                                               config_file_option_escape (option->name),
                                               option->name,
                                               CONFIG_INTEGER(option));
                break;
            case CONFIG_OPTION_TYPE_STRING:
                rc = string_iconv_fprintf (config_file->file, "%s%s = \"%s\"\n",
                                           config_file_option_escape (option->name),
                                           option->name,
                                           (char *)option->value);
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                rc = string_iconv_fprintf (config_file->file, "%s%s = %s\n",
                                           config_file_option_escape (option->name),
                                           option->name,
                                           gui_color_get_name (CONFIG_COLOR(option)));
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
    }
    else
    {
        rc = string_iconv_fprintf (config_file->file, "%s%s\n",
                                   config_file_option_escape (option->name),
                                   option->name);
    }

    return rc;
}

/*
 * config_file_write_line: write a line in a configuration file
 *                         if value is NULL, then write a section with [ ] around
 *                         return 1 if ok, 0 if error
 */

int
config_file_write_line (struct t_config_file *config_file,
                        const char *option_name, const char *value, ...)
{
    int rc;

    if (!config_file || !option_name)
        return 0;

    if (value && value[0])
    {
        weechat_va_format (value);
        if (vbuffer)
        {
            if (vbuffer[0])
            {
                rc = string_iconv_fprintf (config_file->file, "%s%s = %s\n",
                                           config_file_option_escape (option_name),
                                           option_name, vbuffer);
                free (vbuffer);
                return rc;
            }
            free (vbuffer);
        }
    }

    return (string_iconv_fprintf (config_file->file, "\n[%s]\n",
                                  option_name));
}

/*
 * config_file_write_internal: write a configuration file
 *                             (should not be called directly)
 *                             return one of these values:
 *                               WEECHAT_CONFIG_WRITE_OK
 *                               WEECHAT_CONFIG_WRITE_ERROR
 *                               WEECHAT_CONFIG_WRITE_MEMORY_ERROR
 */

int
config_file_write_internal (struct t_config_file *config_file,
                            int default_options)
{
    int filename_length, rc;
    char *filename, *filename2, resolved_path[PATH_MAX];
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    if (!config_file)
        return WEECHAT_CONFIG_WRITE_ERROR;

    /* build filename */
    filename_length = strlen (weechat_home) +
        strlen (config_file->filename) + 2;
    filename = malloc (filename_length);
    if (!filename)
        return WEECHAT_CONFIG_WRITE_MEMORY_ERROR;
    snprintf (filename, filename_length, "%s%s%s",
              weechat_home, DIR_SEPARATOR, config_file->filename);

    /*
     * build temporary filename, this temp file will be renamed to filename
     * after write
     */
    filename2 = malloc (filename_length + 32);
    if (!filename2)
    {
        free (filename);
        return WEECHAT_CONFIG_WRITE_MEMORY_ERROR;
    }
    snprintf (filename2, filename_length + 32, "%s.weechattmp", filename);

    /* if filename is a symbolic link, use target as filename */
    if (realpath (filename, resolved_path))
    {
        if (strcmp (filename, resolved_path) != 0)
        {
            free (filename);
            filename = strdup (resolved_path);
            if (!filename)
            {
                free (filename2);
                return WEECHAT_CONFIG_WRITE_MEMORY_ERROR;
            }
        }
    }

    log_printf (_("Writing configuration file %s %s"),
                config_file->filename,
                (default_options) ? _("(default options)") : "");

    /* open temp file in write mode */
    config_file->file = fopen (filename2, "wb");
    if (!config_file->file)
    {
        gui_chat_printf (NULL,
                         _("%sError: cannot create file \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename2);
        goto error;
    }

    /* write header with name of config file and WeeChat version */
    if (!string_iconv_fprintf (config_file->file, "#\n"))
        goto error;
    if (!string_iconv_fprintf (config_file->file,
                               "# %s -- %s v%s\n#\n",
                               config_file->filename, PACKAGE_NAME, PACKAGE_VERSION))
        goto error;

    /* write all sections */
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        /* call write callback if defined for section */
        if (default_options && ptr_section->callback_write_default)
        {
            if ((ptr_section->callback_write_default) (ptr_section->callback_write_default_data,
                                                       config_file,
                                                       ptr_section->name) != WEECHAT_CONFIG_WRITE_OK)
                goto error;
        }
        else if (!default_options && ptr_section->callback_write)
        {
            if ((ptr_section->callback_write) (ptr_section->callback_write_data,
                                               config_file,
                                               ptr_section->name) != WEECHAT_CONFIG_WRITE_OK)
                goto error;
        }
        else
        {
            /* write all options for section */
            if (!string_iconv_fprintf (config_file->file,
                                       "\n[%s]\n", ptr_section->name))
                goto error;
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if (!config_file_write_option (config_file, ptr_option))
                    goto error;
            }
        }
    }

    if (fflush (config_file->file) != 0)
        goto error;

    /* close temp file */
    fclose (config_file->file);
    config_file->file = NULL;

    /* update file mode */
    chmod (filename2, 0600);

    /* rename temp file to target file */
    rc = rename (filename2, filename);

    free (filename);
    free (filename2);

    if (rc != 0)
        return WEECHAT_CONFIG_WRITE_ERROR;

    return WEECHAT_CONFIG_WRITE_OK;

error:
    gui_chat_printf (NULL,
                     _("%sError writing configuration file \"%s\""),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     filename);
    log_printf (_("%sError writing configuration file \"%s\""),
                "", config_file->filename);
    if (config_file->file)
    {
        fclose (config_file->file);
        config_file->file = NULL;
    }
    unlink (filename2);
    free (filename);
    free (filename2);
    return WEECHAT_CONFIG_WRITE_ERROR;
}

/*
 * config_file_write: write a configuration file
 *                    return one of these values:
 *                      WEECHAT_CONFIG_WRITE_OK
 *                      WEECHAT_CONFIG_WRITE_ERROR
 *                      WEECHAT_CONFIG_WRITE_MEMORY_ERROR
 */

int
config_file_write (struct t_config_file *config_file)
{
    return config_file_write_internal (config_file, 0);
}

/*
 * config_file_read_internal: read a configuration file
 *                            (should not be called directly)
 *                            return one of these values:
 *                              WEECHAT_CONFIG_READ_OK
 *                              WEECHAT_CONFIG_READ_MEMORY_ERROR
 *                              WEECHAT_CONFIG_READ_FILE_NOT_FOUND
 */

int
config_file_read_internal (struct t_config_file *config_file, int reload)
{
    int filename_length, line_number, rc, undefined_value;
    char *filename;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char line[16384], *ptr_line, *ptr_line2, *pos, *pos2, *ptr_option_name;

    if (!config_file)
        return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;

    /* build filename */
    filename_length = strlen (weechat_home) + strlen (config_file->filename) + 2;
    filename = malloc (filename_length);
    if (!filename)
        return WEECHAT_CONFIG_READ_MEMORY_ERROR;
    snprintf (filename, filename_length, "%s%s%s",
              weechat_home, DIR_SEPARATOR, config_file->filename);
    config_file->file = fopen (filename, "r");
    if (!config_file->file)
    {
        config_file_write_internal (config_file, 1);
        config_file->file = fopen (filename, "r");
        if (!config_file->file)
        {
            gui_chat_printf (NULL,
                             _("%sWarning: configuration file \"%s\" not found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             filename);
            free (filename);
            return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
        }
    }

    if (!reload)
        log_printf (_("Reading configuration file %s"), config_file->filename);

    /* read all lines */
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
                snprintf (line, sizeof (line) - 1, "%s", ptr_line2);

            /* skip spaces */
            while (ptr_line[0] == ' ')
            {
                ptr_line++;
            }

            /* not a comment and not an empty line */
            if ((ptr_line[0] != '#') && (ptr_line[0] != '\r')
                && (ptr_line[0] != '\n'))
            {
                /* beginning of section */
                if ((ptr_line[0] == '[') && !strchr (ptr_line, '='))
                {
                    pos = strchr (line, ']');
                    if (!pos)
                    {
                        gui_chat_printf (NULL,
                                         _("%sWarning: %s, line %d: invalid "
                                           "syntax, missing \"]\""),
                                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                         filename, line_number);
                    }
                    else
                    {
                        pos[0] = '\0';
                        pos = ptr_line + 1;
                        ptr_section = config_file_search_section (config_file,
                                                                  pos);
                        if (!ptr_section)
                        {
                            gui_chat_printf (NULL,
                                             _("%sWarning: %s, line %d: unknown "
                                               "section identifier "
                                               "(\"%s\")"),
                                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                             filename, line_number, pos);
                        }
                    }
                }
                else
                {
                    undefined_value = 1;

                    /* remove CR/LF */
                    pos = strchr (line, '\r');
                    if (pos != NULL)
                        pos[0] = '\0';
                    pos = strchr (line, '\n');
                    if (pos != NULL)
                        pos[0] = '\0';

                    pos = strstr (line, " =");
                    if (pos)
                    {
                        pos[0] = '\0';
                        pos += 2;

                        /* remove spaces before '=' */
                        pos2 = pos - 3;
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

                        if (pos[0]
                            && string_strcasecmp (pos, WEECHAT_CONFIG_OPTION_NULL) != 0)
                        {
                            undefined_value = 0;
                            /* remove simple or double quotes and spaces at the end */
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
                        }
                    }

                    ptr_option_name = (line[0] == '\\') ? line + 1 : line;

                    if (ptr_section && ptr_section->callback_read)
                    {
                        ptr_option = NULL;
                        rc = (ptr_section->callback_read)
                            (ptr_section->callback_read_data,
                             config_file,
                             ptr_section,
                             ptr_option_name,
                             (undefined_value) ? NULL : pos);
                    }
                    else
                    {
                        rc = WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND;
                        ptr_option = config_file_search_option (config_file,
                                                                ptr_section,
                                                                ptr_option_name);
                        if (ptr_option)
                        {
                            rc = config_file_option_set (ptr_option,
                                                         (undefined_value) ?
                                                         NULL : pos,
                                                         1);
                            ptr_option->loaded = 1;
                        }
                        else
                        {
                            if (ptr_section
                                && ptr_section->callback_create_option)
                            {
                                rc = (int)(ptr_section->callback_create_option)
                                    (ptr_section->callback_create_option_data,
                                     config_file,
                                     ptr_section,
                                     ptr_option_name,
                                     (undefined_value) ? NULL : pos);
                            }
                        }
                    }

                    switch (rc)
                    {
                        case WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND:
                            if (ptr_section)
                                gui_chat_printf (NULL,
                                                 _("%sWarning: %s, line %d: "
                                                   "unknown option for section "
                                                   "\"%s\": %s"),
                                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                                 filename, line_number,
                                                 ptr_section->name,
                                                 ptr_line2);
                            else
                                gui_chat_printf (NULL,
                                                 _("%sWarning: %s, line %d: "
                                                   "option outside section: "
                                                   "%s"),
                                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                                 filename, line_number,
                                                 ptr_line2);
                            break;
                        case WEECHAT_CONFIG_OPTION_SET_ERROR:
                            gui_chat_printf (NULL,
                                             _("%sWarning: %s, line %d: "
                                               "invalid value for option: "
                                               "%s"),
                                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                             filename, line_number,
                                             ptr_line2);
                            break;
                    }
                }
            }

            if (ptr_line2)
                free (ptr_line2);
        }
    }

    fclose (config_file->file);
    config_file->file = NULL;
    free (filename);

    return WEECHAT_CONFIG_READ_OK;
}

/*
 * config_file_read: read a configuration file
 *                   return one of these values:
 *                     WEECHAT_CONFIG_READ_OK
 *                     WEECHAT_CONFIG_READ_MEMORY_ERROR
 *                     WEECHAT_CONFIG_READ_FILE_NOT_FOUND
 */

int
config_file_read (struct t_config_file *config_file)
{
    return config_file_read_internal (config_file, 0);
}

/*
 * config_file_reload: reload a configuration file
 *                     return one of these values:
 *                       WEECHAT_CONFIG_READ_OK
 *                       WEECHAT_CONFIG_READ_MEMORY_ERROR
 *                       WEECHAT_CONFIG_READ_FILE_NOT_FOUND
 */

int
config_file_reload (struct t_config_file *config_file)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    int rc;

    if (!config_file)
        return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;

    log_printf (_("Reloading configuration file %s"), config_file->filename);

    /* init "loaded" flag for all options */
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        if (!ptr_section->callback_read)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                ptr_option->loaded = 0;
            }
        }
    }

    /* read configuration file */
    rc = config_file_read_internal (config_file, 1);

    /* reset options not found in configuration file */
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        if (!ptr_section->callback_read)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if (!ptr_option->loaded)
                    config_file_option_reset (ptr_option, 1);
            }
        }
    }

    return rc;
}

/*
 * config_file_option_free_data: free data in an option
 */

void
config_file_option_free_data (struct t_config_option *option)
{
    if (option->name)
        free (option->name);
    if (option->description)
        free (option->description);
    if (option->string_values)
        string_free_split (option->string_values);
    if (option->default_value)
        free (option->default_value);
    if (option->value)
        free (option->value);
}

/*
 * config_file_option_free: free an option
 */

void
config_file_option_free (struct t_config_option *option)
{
    struct t_config_section *ptr_section;
    struct t_config_option *new_options;

    if (!option)
        return;

    ptr_section = option->section;

    /* free data */
    config_file_option_free_data (option);

    /* remove option from section */
    if (ptr_section)
    {
        if (ptr_section->last_option == option)
            ptr_section->last_option = option->prev_option;
        if (option->prev_option)
        {
            (option->prev_option)->next_option = option->next_option;
            new_options = ptr_section->options;
        }
        else
            new_options = option->next_option;
        if (option->next_option)
            (option->next_option)->prev_option = option->prev_option;
        ptr_section->options = new_options;
    }

    free (option);
}

/*
 * config_file_section_free_options: free options in a section
 */

void
config_file_section_free_options (struct t_config_section *section)
{
    if (!section)
        return;

    while (section->options)
    {
        config_file_option_free (section->options);
    }
}

/*
 * config_file_section_free: free a section
 */

void
config_file_section_free (struct t_config_section *section)
{
    struct t_config_file *ptr_config;
    struct t_config_section *new_sections;

    if (!section)
        return;

    ptr_config = section->config_file;

    /* free data */
    config_file_section_free_options (section);
    if (section->name)
        free (section->name);

    /* remove section from list */
    if (ptr_config->last_section == section)
        ptr_config->last_section = section->prev_section;
    if (section->prev_section)
    {
        (section->prev_section)->next_section = section->next_section;
        new_sections = ptr_config->sections;
    }
    else
        new_sections = section->next_section;

    if (section->next_section)
        (section->next_section)->prev_section = section->prev_section;

    free (section);

    ptr_config->sections = new_sections;
}

/*
 * config_file_free: free a configuration file
 */

void
config_file_free (struct t_config_file *config_file)
{
    struct t_config_file *new_config_files;

    if (!config_file)
        return;

    /* free data */
    while (config_file->sections)
    {
        config_file_section_free (config_file->sections);
    }
    if (config_file->name)
        free (config_file->name);
    if (config_file->filename)
        free (config_file->filename);

    /* remove configuration file from list */
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

    free (config_file);

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
config_file_free_all_plugin (struct t_weechat_plugin *plugin)
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
 * config_file_hdata_config_file_cb: return hdata for config_file
 */

struct t_hdata *
config_file_hdata_config_file_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_config", "next_config");
    if (hdata)
    {
        HDATA_VAR(struct t_config_file, plugin, POINTER, NULL, "plugin");
        HDATA_VAR(struct t_config_file, name, STRING, NULL, NULL);
        HDATA_VAR(struct t_config_file, filename, STRING, NULL, NULL);
        HDATA_VAR(struct t_config_file, file, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_file, callback_reload, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_file, callback_reload_data, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_file, sections, POINTER, NULL, "config_section");
        HDATA_VAR(struct t_config_file, last_section, POINTER, NULL, "config_section");
        HDATA_VAR(struct t_config_file, prev_config, POINTER, NULL, hdata_name);
        HDATA_VAR(struct t_config_file, next_config, POINTER, NULL, hdata_name);
        HDATA_LIST(config_files);
        HDATA_LIST(last_config_file);
    }
    return hdata;
}

/*
 * config_file_hdata_config_section_cb: return hdata for config_section
 */

struct t_hdata *
config_file_hdata_config_section_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_section", "next_section");
    if (hdata)
    {
        HDATA_VAR(struct t_config_section, config_file, POINTER, NULL, "config_file");
        HDATA_VAR(struct t_config_section, name, STRING, NULL, NULL);
        HDATA_VAR(struct t_config_section, user_can_add_options, INTEGER, NULL, NULL);
        HDATA_VAR(struct t_config_section, user_can_delete_options, INTEGER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_read, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_read_data, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write_data, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write_default, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write_default_data, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_create_option, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_create_option_data, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_delete_option, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_delete_option_data, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_section, options, POINTER, NULL, "config_option");
        HDATA_VAR(struct t_config_section, last_option, POINTER, NULL, "config_option");
        HDATA_VAR(struct t_config_section, prev_section, POINTER, NULL, hdata_name);
        HDATA_VAR(struct t_config_section, next_section, POINTER, NULL, hdata_name);
    }
    return hdata;
}

/*
 * config_file_hdata_config_option_cb: return hdata for config_option
 */

struct t_hdata *
config_file_hdata_config_option_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_option", "next_option");
    if (hdata)
    {
        HDATA_VAR(struct t_config_option, config_file, POINTER, NULL, "config_file");
        HDATA_VAR(struct t_config_option, section, POINTER, NULL, "config_section");
        HDATA_VAR(struct t_config_option, name, STRING, NULL, NULL);
        HDATA_VAR(struct t_config_option, type, INTEGER, NULL, NULL);
        HDATA_VAR(struct t_config_option, description, STRING, NULL, NULL);
        HDATA_VAR(struct t_config_option, string_values, STRING, "*", NULL);
        HDATA_VAR(struct t_config_option, min, INTEGER, NULL, NULL);
        HDATA_VAR(struct t_config_option, max, INTEGER, NULL, NULL);
        HDATA_VAR(struct t_config_option, default_value, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_option, value, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_option, null_value_allowed, INTEGER, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_check_value, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_check_value_data, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_change, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_change_data, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_delete, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_delete_data, POINTER, NULL, NULL);
        HDATA_VAR(struct t_config_option, loaded, INTEGER, NULL, NULL);
        HDATA_VAR(struct t_config_option, prev_option, POINTER, NULL, hdata_name);
        HDATA_VAR(struct t_config_option, next_option, POINTER, NULL, hdata_name);
    }
    return hdata;
}

/*
 * config_file_add_to_infolist: add configuration options in an infolist
 *                              return 1 if ok, 0 if error
 */

int
config_file_add_to_infolist (struct t_infolist *infolist,
                             const char *option_name)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    struct t_infolist_item *ptr_item;
    int length;
    char *option_full_name, value[128], *string_values;

    if (!infolist)
        return 0;

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        for (ptr_section = ptr_config->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                length = strlen (ptr_config->name) + 1 +
                    strlen (ptr_section->name) + 1 +
                    strlen (ptr_option->name) + 1;
                option_full_name = malloc (length);
                if (option_full_name)
                {
                    snprintf (option_full_name, length, "%s.%s.%s",
                              ptr_config->name,
                              ptr_section->name,
                              ptr_option->name);
                    if (!option_name || !option_name[0]
                        || string_match (option_full_name, option_name, 0))
                    {
                        ptr_item = infolist_new_item (infolist);
                        if (!ptr_item)
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "full_name",
                                                      option_full_name))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "config_name",
                                                      ptr_config->name))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "section_name",
                                                      ptr_section->name))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "option_name",
                                                      ptr_option->name))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "description",
                                                      ptr_option->description))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_string (ptr_item,
                                                      "description_nls",
                                                      (ptr_option->description
                                                       && ptr_option->description[0]) ?
                                                      _(ptr_option->description) : ""))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        string_values = string_build_with_split_string ((const char **)ptr_option->string_values,
                                                                        "|");
                        if (!infolist_new_var_string (ptr_item,
                                                      "string_values",
                                                      string_values))
                        {
                            if (string_values)
                                free (string_values);
                            free (option_full_name);
                            return 0;
                        }
                        if (string_values)
                            free (string_values);
                        if (!infolist_new_var_integer (ptr_item,
                                                       "min",
                                                       ptr_option->min))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_integer (ptr_item,
                                                       "max",
                                                       ptr_option->max))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_integer (ptr_item,
                                                       "null_value_allowed",
                                                       ptr_option->null_value_allowed))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_integer (ptr_item,
                                                       "value_is_null",
                                                       (ptr_option->value) ?
                                                       0 : 1))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        if (!infolist_new_var_integer (ptr_item,
                                                       "default_value_is_null",
                                                       (ptr_option->default_value) ?
                                                       0 : 1))
                        {
                            free (option_full_name);
                            return 0;
                        }
                        switch (ptr_option->type)
                        {
                            case CONFIG_OPTION_TYPE_BOOLEAN:
                                if (!infolist_new_var_string (ptr_item,
                                                              "type",
                                                              "boolean"))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (ptr_option->value)
                                {
                                    if (CONFIG_BOOLEAN(ptr_option) == CONFIG_BOOLEAN_TRUE)
                                        snprintf (value, sizeof (value), "on");
                                    else
                                        snprintf (value, sizeof (value), "off");
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "value",
                                                                  value))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                }
                                if (ptr_option->default_value)
                                {
                                    if (CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE)
                                        snprintf (value, sizeof (value), "on");
                                    else
                                        snprintf (value, sizeof (value), "off");
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "default_value",
                                                                  value))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                }
                                break;
                            case CONFIG_OPTION_TYPE_INTEGER:
                                if (!infolist_new_var_string (ptr_item,
                                                              "type",
                                                              "integer"))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (ptr_option->string_values)
                                {
                                    if (ptr_option->value)
                                    {
                                        if (!infolist_new_var_string (ptr_item,
                                                                      "value",
                                                                      ptr_option->string_values[CONFIG_INTEGER(ptr_option)]))
                                        {
                                            free (option_full_name);
                                            return 0;
                                        }
                                    }
                                    if (ptr_option->default_value)
                                    {
                                        if (!infolist_new_var_string (ptr_item,
                                                                      "default_value",
                                                                      ptr_option->string_values[CONFIG_INTEGER_DEFAULT(ptr_option)]))
                                        {
                                            free (option_full_name);
                                            return 0;
                                        }
                                    }
                                }
                                else
                                {
                                    if (ptr_option->value)
                                    {
                                        snprintf (value, sizeof (value), "%d",
                                                  CONFIG_INTEGER(ptr_option));
                                        if (!infolist_new_var_string (ptr_item,
                                                                      "value",
                                                                      value))
                                        {
                                            free (option_full_name);
                                            return 0;
                                        }
                                    }
                                    if (ptr_option->default_value)
                                    {
                                        snprintf (value, sizeof (value), "%d",
                                                  CONFIG_INTEGER_DEFAULT(ptr_option));
                                        if (!infolist_new_var_string (ptr_item,
                                                                      "default_value",
                                                                      value))
                                        {
                                            free (option_full_name);
                                            return 0;
                                        }
                                    }
                                }
                                break;
                            case CONFIG_OPTION_TYPE_STRING:
                                if (!infolist_new_var_string (ptr_item,
                                                              "type",
                                                              "string"))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (ptr_option->value)
                                {
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "value",
                                                                  CONFIG_STRING(ptr_option)))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                }
                                if (ptr_option->default_value)
                                {
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "default_value",
                                                                  CONFIG_STRING_DEFAULT(ptr_option)))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                }
                                break;
                            case CONFIG_OPTION_TYPE_COLOR:
                                if (!infolist_new_var_string (ptr_item,
                                                              "type",
                                                              "color"))
                                {
                                    free (option_full_name);
                                    return 0;
                                }
                                if (ptr_option->value)
                                {
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "value",
                                                                  gui_color_get_name (CONFIG_COLOR(ptr_option))))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                }
                                if (ptr_option->default_value)
                                {
                                    if (!infolist_new_var_string (ptr_item,
                                                                  "default_value",
                                                                  gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option))))
                                    {
                                        free (option_full_name);
                                        return 0;
                                    }
                                }
                                break;
                            case CONFIG_NUM_OPTION_TYPES:
                                break;
                        }
                    }
                    free (option_full_name);
                }
            }
        }
    }

    return 1;
}

/*
 * config_file_print_log: print configuration in log (usually for crash dump)
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
        log_printf ("");
        log_printf ("[config (addr:0x%lx)]", ptr_config_file);
        log_printf ("  plugin . . . . . . . . : 0x%lx ('%s')",
                    ptr_config_file->plugin,
                    plugin_get_name (ptr_config_file->plugin));
        log_printf ("  name . . . . . . . . . : '%s'",  ptr_config_file->name);
        log_printf ("  filename . . . . . . . : '%s'",  ptr_config_file->filename);
        log_printf ("  file . . . . . . . . . : 0x%lx", ptr_config_file->file);
        log_printf ("  callback_reload. . . . : 0x%lx", ptr_config_file->callback_reload);
        log_printf ("  callback_reload_data . : 0x%lx", ptr_config_file->callback_reload_data);
        log_printf ("  sections . . . . . . . : 0x%lx", ptr_config_file->sections);
        log_printf ("  last_section . . . . . : 0x%lx", ptr_config_file->last_section);
        log_printf ("  prev_config. . . . . . : 0x%lx", ptr_config_file->prev_config);
        log_printf ("  next_config. . . . . . : 0x%lx", ptr_config_file->next_config);

        for (ptr_section = ptr_config_file->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            log_printf ("");
            log_printf ("    [section (addr:0x%lx)]", ptr_section);
            log_printf ("      config_file. . . . . . . . : 0x%lx", ptr_section->config_file);
            log_printf ("      name . . . . . . . . . . . : '%s'",  ptr_section->name);
            log_printf ("      callback_read. . . . . . . : 0x%lx", ptr_section->callback_read);
            log_printf ("      callback_read_data . . . . : 0x%lx", ptr_section->callback_read_data);
            log_printf ("      callback_write . . . . . . : 0x%lx", ptr_section->callback_write);
            log_printf ("      callback_write_data. . . . : 0x%lx", ptr_section->callback_write_data);
            log_printf ("      callback_write_default . . : 0x%lx", ptr_section->callback_write_default);
            log_printf ("      callback_write_default_data: 0x%lx", ptr_section->callback_write_default_data);
            log_printf ("      callback_create_option. . .: 0x%lx", ptr_section->callback_create_option);
            log_printf ("      callback_create_option_data: 0x%lx", ptr_section->callback_create_option_data);
            log_printf ("      callback_delete_option. . .: 0x%lx", ptr_section->callback_delete_option);
            log_printf ("      callback_delete_option_data: 0x%lx", ptr_section->callback_delete_option_data);
            log_printf ("      options. . . . . . . . . . : 0x%lx", ptr_section->options);
            log_printf ("      last_option. . . . . . . . : 0x%lx", ptr_section->last_option);
            log_printf ("      prev_section . . . . . . . : 0x%lx", ptr_section->prev_section);
            log_printf ("      next_section . . . . . . . : 0x%lx", ptr_section->next_section);

            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                log_printf ("");
                log_printf ("      [option (addr:0x%lx)]", ptr_option);
                log_printf ("        config_file. . . . . : 0x%lx", ptr_option->config_file);
                log_printf ("        section. . . . . . . : 0x%lx", ptr_option->section);
                log_printf ("        name . . . . . . . . : '%s'",  ptr_option->name);
                log_printf ("        type . . . . . . . . : %d",    ptr_option->type);
                log_printf ("        description. . . . . : '%s'",  ptr_option->description);
                log_printf ("        string_values. . . . : 0x%lx", ptr_option->string_values);
                log_printf ("        min. . . . . . . . . : %d",    ptr_option->min);
                log_printf ("        max. . . . . . . . . : %d",    ptr_option->max);
                switch (ptr_option->type)
                {
                    case CONFIG_OPTION_TYPE_BOOLEAN:
                        log_printf ("        default value. . . . : %s",
                                    (ptr_option->default_value) ?
                                    ((CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                     "on" : "off") : "null");
                        log_printf ("        value (boolean). . . : %s",
                                    (ptr_option->value) ?
                                    ((CONFIG_BOOLEAN(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                     "on" : "off") : "null");
                        break;
                    case CONFIG_OPTION_TYPE_INTEGER:
                        if (ptr_option->string_values)
                        {
                            log_printf ("        default value. . . . : '%s'",
                                        (ptr_option->default_value) ?
                                        ptr_option->string_values[CONFIG_INTEGER_DEFAULT(ptr_option)] : "null");
                            log_printf ("        value (integer/str). : '%s'",
                                        (ptr_option->value) ?
                                        ptr_option->string_values[CONFIG_INTEGER(ptr_option)] : "null");
                        }
                        else
                        {
                            if (ptr_option->default_value)
                                log_printf ("        default value. . . . : %d",
                                            CONFIG_INTEGER_DEFAULT(ptr_option));
                            else
                                log_printf ("        default value. . . . : null");
                            if (ptr_option->value)
                                log_printf ("        value (integer). . . : %d",
                                            CONFIG_INTEGER(ptr_option));
                            else
                                log_printf ("        value (integer). . . : null");
                        }
                        break;
                    case CONFIG_OPTION_TYPE_STRING:
                        if (ptr_option->default_value)
                            log_printf ("        default value. . . . : '%s'",
                                        CONFIG_STRING_DEFAULT(ptr_option));
                        else
                            log_printf ("        default value. . . . : null");
                        if (ptr_option->value)
                            log_printf ("        value (string) . . . : '%s'",
                                        CONFIG_STRING(ptr_option));
                        else
                            log_printf ("        value (string) . . . : null");
                        break;
                    case CONFIG_OPTION_TYPE_COLOR:
                        if (ptr_option->default_value)
                            log_printf ("        default value. . . . : %d ('%s')",
                                        CONFIG_COLOR_DEFAULT(ptr_option),
                                        gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option)));
                        else
                            log_printf ("        default value. . . . : null");
                        if (ptr_option->value)
                            log_printf ("        value (color). . . . : %d ('%s')",
                                        CONFIG_COLOR(ptr_option),
                                        gui_color_get_name (CONFIG_COLOR(ptr_option)));
                        else
                            log_printf ("        value (color). . . . : null");
                        break;
                    case CONFIG_NUM_OPTION_TYPES:
                        break;
                }
                log_printf ("        null_value_allowed . : %d",    ptr_option->null_value_allowed);
                log_printf ("        callback_change. . . : 0x%lx", ptr_option->callback_change);
                log_printf ("        loaded . . . . . . . : %d",    ptr_option->loaded);
                log_printf ("        prev_option. . . . . : 0x%lx", ptr_option->prev_option);
                log_printf ("        next_option. . . . . : 0x%lx", ptr_option->next_option);
            }
        }
    }
}
