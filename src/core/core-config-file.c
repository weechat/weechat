/*
 * core-config-file.c - configuration files/sections/options management
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
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
#include <errno.h>

#include "weechat.h"
#include "core-config-file.h"
#include "core-arraylist.h"
#include "core-config.h"
#include "core-dir.h"
#include "core-hashtable.h"
#include "core-hdata.h"
#include "core-hook.h"
#include "core-infolist.h"
#include "core-log.h"
#include "core-string.h"
#include "core-version.h"
#include "../gui/gui-color.h"
#include "../gui/gui-chat.h"
#include "../plugins/plugin.h"


struct t_config_file *config_files = NULL;
struct t_config_file *last_config_file = NULL;

char *config_option_type_string[CONFIG_NUM_OPTION_TYPES] =
{ N_("boolean"), N_("integer"), N_("string"), N_("color"), N_("enum") };
char *config_boolean_true[] = { "on", "yes", "y", "true", "t", "1", NULL };
char *config_boolean_false[] = { "off", "no", "n", "false", "f", "0", NULL };


void config_file_option_free_data (struct t_config_option *option);


/*
 * Checks if a configuration file pointer is valid.
 *
 * Returns:
 *   1: configuration file exists
 *   0: configuration file does not exist
 */

int
config_file_valid (struct t_config_file *config_file)
{
    struct t_config_file *ptr_config;

    if (!config_file)
        return 0;

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if (ptr_config == config_file)
            return 1;
    }

    /* configuration file not found */
    return 0;
}

/*
 * Searches for a configuration file.
 */

struct t_config_file *
config_file_search (const char *name)
{
    struct t_config_file *ptr_config;
    int rc;

    if (!name)
        return NULL;

    for (ptr_config = last_config_file; ptr_config;
         ptr_config = ptr_config->prev_config)
    {
        rc = strcmp (ptr_config->name, name);
        if (rc == 0)
            return ptr_config;
        else if (rc < 0)
            break;
    }

    /* configuration file not found */
    return NULL;
}

/*
 * Searches for position of configuration file (to keep configuration files
 * sorted by name).
 */

struct t_config_file *
config_file_find_pos (const char *name)
{
    struct t_config_file *ptr_config;

    if (!name)
        return NULL;

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        if (string_strcmp (name, ptr_config->name) < 0)
            return ptr_config;
    }

    /* position not found (we will add to the end of list) */
    return NULL;
}

/*
 * Inserts a configuration file (keeping files sorted by name).
 */

void
config_file_config_insert (struct t_config_file *config_file)
{
    struct t_config_file *pos_config;

    if (!config_file)
        return;

    if (config_files)
    {
        pos_config = config_file_find_pos (config_file->name);
        if (pos_config)
        {
            /* insert config into the list (before config found) */
            config_file->prev_config = pos_config->prev_config;
            config_file->next_config = pos_config;
            if (pos_config->prev_config)
                (pos_config->prev_config)->next_config = config_file;
            else
                config_files = config_file;
            pos_config->prev_config = config_file;
        }
        else
        {
            /* add config to the end */
            config_file->prev_config = last_config_file;
            config_file->next_config = NULL;
            last_config_file->next_config = config_file;
            last_config_file = config_file;
        }
    }
    else
    {
        /* first config */
        config_file->prev_config = NULL;
        config_file->next_config = NULL;
        config_files = config_file;
        last_config_file = config_file;
    }
}

/*
 * Creates a new configuration file.
 *
 * Returns pointer to new configuration file, NULL if error.
 */

struct t_config_file *
config_file_new (struct t_weechat_plugin *plugin, const char *name,
                 int (*callback_reload)(const void *pointer,
                                        void *data,
                                        struct t_config_file *config_file),
                 const void *callback_reload_pointer,
                 void *callback_reload_data)
{
    struct t_config_file *new_config_file;
    const char *ptr_name;
    int priority;

    string_get_priority_and_name (name, &priority, &ptr_name,
                                  CONFIG_PRIORITY_DEFAULT);

    if (!ptr_name || !ptr_name[0])
        return NULL;

    /* two configuration files cannot have same name */
    if (config_file_search (ptr_name))
        return NULL;

    new_config_file = malloc (sizeof (*new_config_file));
    if (new_config_file)
    {
        new_config_file->plugin = plugin;
        new_config_file->priority = priority;
        new_config_file->name = strdup (ptr_name);
        if (!new_config_file->name)
        {
            free (new_config_file);
            return NULL;
        }
        if (string_asprintf (&new_config_file->filename, "%s.conf", ptr_name) < 0)
        {
            free (new_config_file->name);
            free (new_config_file);
            return NULL;
        }
        new_config_file->file = NULL;
        new_config_file->version = 1;
        new_config_file->callback_update = NULL;
        new_config_file->callback_update_pointer = NULL;
        new_config_file->callback_update_data = NULL;
        new_config_file->callback_reload = callback_reload;
        new_config_file->callback_reload_pointer = callback_reload_pointer;
        new_config_file->callback_reload_data = callback_reload_data;
        new_config_file->sections = NULL;
        new_config_file->last_section = NULL;

        config_file_config_insert (new_config_file);
    }

    return new_config_file;
}

/*
 * Sets configuration file version and a callback to update config
 * sections/options on-the-fly when the config is read.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
config_file_set_version (struct t_config_file *config_file,
                         int version,
                         struct t_hashtable *(*callback_update)(const void *pointer,
                                                                void *data,
                                                                struct t_config_file *config_file,
                                                                int version_read,
                                                                struct t_hashtable *data_read),
                         const void *callback_update_pointer,
                         void *callback_update_data)
{
    if (version < 1)
        return 0;

    config_file->version = version;

    config_file->callback_update = callback_update;
    config_file->callback_update_pointer = callback_update_pointer;
    config_file->callback_update_data = callback_update_data;

    return 1;
}

/*
 * Compares two configuration files to sort them by priority (highest priority
 * at beginning of list).
 *
 * Returns:
 *   -1: config1 has higher priority than config2
 *    1: config1 has same or lower priority than config2
 */

int
config_file_arraylist_cmp_config_cb (void *data,
                                     struct t_arraylist *arraylist,
                                     void *pointer1, void *pointer2)
{
    struct t_config_file *ptr_config1, *ptr_config2;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    ptr_config1 = (struct t_config_file *)pointer1;
    ptr_config2 = (struct t_config_file *)pointer2;

    return (ptr_config1->priority > ptr_config2->priority) ? -1 : 1;
}

/*
 * Returns an arraylist with pointers to configuration files, sorted by
 * priority (from highest to lowest).
 */

struct t_arraylist *
config_file_get_configs_by_priority (void)
{
    struct t_arraylist *list;
    struct t_config_file *ptr_config;

    /*
     * build a list of pointers to configs sorted by priority,
     * so that configs with high priority are reloaded first
     */
    list = arraylist_new (
        32, 1, 1,
        &config_file_arraylist_cmp_config_cb, NULL,
        NULL, NULL);
    if (!list)
        return NULL;

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        arraylist_add (list, ptr_config);
    }

    return list;
}

/*
 * Searches for position of section in configuration file (to keep sections
 * sorted by name).
 */

struct t_config_section *
config_file_section_find_pos (struct t_config_file *config_file,
                              const char *name)
{
    struct t_config_section *ptr_section;

    if (!config_file || !name)
        return NULL;

    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        if (string_strcmp (name, ptr_section->name) < 0)
            return ptr_section;
    }

    /* position not found (we will add to the end of list) */
    return NULL;
}

/*
 * Creates a new section in a configuration file.
 *
 * Returns pointer to new section, NULL if error.
 */

struct t_config_section *
config_file_new_section (struct t_config_file *config_file, const char *name,
                         int user_can_add_options, int user_can_delete_options,
                         int (*callback_read)(const void *pointer,
                                              void *data,
                                              struct t_config_file *config_file,
                                              struct t_config_section *section,
                                              const char *option_name,
                                              const char *value),
                         const void *callback_read_pointer,
                         void *callback_read_data,
                         int (*callback_write)(const void *pointer,
                                               void *data,
                                               struct t_config_file *config_file,
                                               const char *section_name),
                         const void *callback_write_pointer,
                         void *callback_write_data,
                         int (*callback_write_default)(const void *pointer,
                                                       void *data,
                                                       struct t_config_file *config_file,
                                                       const char *section_name),
                         const void *callback_write_default_pointer,
                         void *callback_write_default_data,
                         int (*callback_create_option)(const void *pointer,
                                                       void *data,
                                                       struct t_config_file *config_file,
                                                       struct t_config_section *section,
                                                       const char *option_name,
                                                       const char *value),
                         const void *callback_create_option_pointer,
                         void *callback_create_option_data,
                         int (*callback_delete_option)(const void *pointer,
                                                       void *data,
                                                       struct t_config_file *config_file,
                                                       struct t_config_section *section,
                                                       struct t_config_option *option),
                         const void *callback_delete_option_pointer,
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
        new_section->callback_read_pointer = callback_read_pointer;
        new_section->callback_read_data = callback_read_data;
        new_section->callback_write = callback_write;
        new_section->callback_write_pointer = callback_write_pointer;
        new_section->callback_write_data = callback_write_data;
        new_section->callback_write_default = callback_write_default;
        new_section->callback_write_default_pointer = callback_write_default_pointer;
        new_section->callback_write_default_data = callback_write_default_data;
        new_section->callback_create_option = callback_create_option;
        new_section->callback_create_option_pointer = callback_create_option_pointer;
        new_section->callback_create_option_data = callback_create_option_data;
        new_section->callback_delete_option = callback_delete_option;
        new_section->callback_delete_option_pointer = callback_delete_option_pointer;
        new_section->callback_delete_option_data = callback_delete_option_data;
        new_section->options = NULL;
        new_section->last_option = NULL;

        new_section->prev_section = config_file->last_section;
        new_section->next_section = NULL;
        if (config_file->last_section)
            config_file->last_section->next_section = new_section;
        else
            config_file->sections = new_section;
        config_file->last_section = new_section;
    }

    return new_section;
}

/*
 * Searches for a section in a configuration file.
 *
 * Returns pointer to section found, NULL if not found.
 */

struct t_config_section *
config_file_search_section (struct t_config_file *config_file,
                            const char *name)
{
    struct t_config_section *ptr_section;

    if (!config_file || !name)
        return NULL;

    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        if (strcmp (ptr_section->name, name) == 0)
            return ptr_section;
    }

    /* section not found */
    return NULL;
}

/*
 * Builds full name for an option, using format: "file.section.option".
 *
 * Note: result must be freed after use.
 */

char *
config_file_option_full_name (struct t_config_option *option)
{
    char *option_full_name;

    if (!option)
        return NULL;

    string_asprintf (&option_full_name,
                     "%s.%s.%s",
                     option->config_file->name,
                     option->section->name,
                     option->name);

    return option_full_name;
}

/*
 * Executes hook_config for modified option.
 */

void
config_file_hook_config_exec (struct t_config_option *option)
{
    char *option_full_name, str_value[256];

    if (!option || !option->config_file || !option->section)
        return;

    option_full_name = config_file_option_full_name (option);
    if (!option_full_name)
        return;

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
                snprintf (str_value, sizeof (str_value),
                          "%d", CONFIG_INTEGER(option));
                hook_config_exec (option_full_name, str_value);
                break;
            case CONFIG_OPTION_TYPE_STRING:
                hook_config_exec (option_full_name, (char *)option->value);
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                hook_config_exec (option_full_name,
                                  gui_color_get_name (CONFIG_COLOR(option)));
                break;
            case CONFIG_OPTION_TYPE_ENUM:
                hook_config_exec (option_full_name,
                                  option->string_values[CONFIG_ENUM(option)]);
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
    }
    else
    {
        hook_config_exec (option_full_name, NULL);
    }

    free (option_full_name);
}

/*
 * Searches for position of option in section (to keep options sorted by name).
 */

struct t_config_option *
config_file_option_find_pos (struct t_config_section *section, const char *name)
{
    struct t_config_option *ptr_option;

    if (!section || !name)
        return NULL;

    for (ptr_option = section->last_option; ptr_option;
         ptr_option = ptr_option->prev_option)
    {
        if (string_strcmp (name, ptr_option->name) >= 0)
            return ptr_option->next_option;
    }

    return section->options;
}

/*
 * Inserts an option in section (keeping options sorted by name).
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
 * Allocates memory for a new option and initializes it.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
config_file_option_malloc (void)
{
    struct t_config_option *new_option;

    new_option = malloc (sizeof (*new_option));
    if (new_option)
    {
        new_option->config_file = NULL;
        new_option->section = NULL;
        new_option->name = NULL;
        new_option->parent_name = NULL;
        new_option->type = 0;
        new_option->description = NULL;
        new_option->string_values = NULL;
        new_option->min = 0;
        new_option->max = 0;
        new_option->default_value = NULL;
        new_option->value = NULL;
        new_option->null_value_allowed = 0;
        new_option->callback_check_value = NULL;
        new_option->callback_check_value_pointer = NULL;
        new_option->callback_check_value_data = NULL;
        new_option->callback_change = NULL;
        new_option->callback_change_pointer = NULL;
        new_option->callback_change_data = NULL;
        new_option->callback_delete = NULL;
        new_option->callback_delete_pointer = NULL;
        new_option->callback_delete_data = NULL;
        new_option->loaded = 0;
        new_option->prev_option = NULL;
        new_option->next_option = NULL;
    }

    return new_option;
}

/*
 * Creates a new option.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
config_file_new_option (struct t_config_file *config_file,
                        struct t_config_section *section, const char *name,
                        const char *type, const char *description,
                        const char *string_values, int min, int max,
                        const char *default_value,
                        const char *value,
                        int null_value_allowed,
                        int (*callback_check_value)(const void *pointer,
                                                    void *data,
                                                    struct t_config_option *option,
                                                    const char *value),
                        const void *callback_check_value_pointer,
                        void *callback_check_value_data,
                        void (*callback_change)(const void *pointer,
                                                void *data,
                                                struct t_config_option *option),
                        const void *callback_change_pointer,
                        void *callback_change_data,
                        void (*callback_delete)(const void *pointer,
                                                void *data,
                                                struct t_config_option *option),
                        const void *callback_delete_pointer,
                        void *callback_delete_data)
{
    struct t_config_option *new_option;
    int var_type, int_value, argc, i, index_value;
    long number;
    char *error, *pos, *option_name, *parent_name;

    new_option = NULL;
    option_name = NULL;
    parent_name = NULL;

    if (!name || !type)
        goto error;

    pos = strstr (name, " << ");
    if (pos)
    {
        option_name = string_strndup (name, pos - name);
        parent_name = strdup (pos + 4);
    }
    else
    {
        option_name = strdup (name);
    }

    if (config_file && section
        && config_file_search_option (config_file, section, option_name))
    {
        goto error;
    }

    var_type = -1;
    for (i = 0; i < CONFIG_NUM_OPTION_TYPES; i++)
    {
        if (strcmp (type, config_option_type_string[i]) == 0)
        {
            var_type = i;
            break;
        }
    }
    if (var_type < 0)
    {
        gui_chat_printf (NULL, "%sUnknown option type \"%s\"",
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         type);
        goto error;
    }

    /*
     * compatibility with versions < 4.1.0: force enum type for an integer
     * with string values
     */
    if ((var_type == CONFIG_OPTION_TYPE_INTEGER)
        && string_values && string_values[0])
    {
        var_type = CONFIG_OPTION_TYPE_ENUM;
    }

    if ((var_type == CONFIG_OPTION_TYPE_ENUM)
        && (!string_values || !string_values[0]))
    {
        goto error;
    }

    if (!null_value_allowed)
    {
        if (default_value && !value)
            value = default_value;
        else if (!default_value && value)
            default_value = value;
        if (!default_value || !value)
            goto error;
    }

    new_option = config_file_option_malloc ();
    if (new_option)
    {
        new_option->config_file = config_file;
        new_option->section = section;
        new_option->name = strdup (option_name);
        if (!new_option->name)
            goto error;
        new_option->parent_name = (parent_name) ? strdup (parent_name) : NULL;
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
                    CONFIG_BOOLEAN_DEFAULT(new_option) = int_value;
                }
                if (value)
                {
                    int_value = config_file_string_to_boolean (value);
                    new_option->value = malloc (sizeof (int));
                    if (!new_option->value)
                        goto error;
                    CONFIG_BOOLEAN(new_option) = int_value;
                }
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
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
                        CONFIG_COLOR_DEFAULT(new_option) = 0;
                }
                if (value)
                {
                    new_option->value = malloc (sizeof (int));
                    if (!new_option->value)
                        goto error;
                    if (!gui_color_assign (new_option->value, value))
                        CONFIG_COLOR(new_option) = 0;
                }
                break;
            case CONFIG_OPTION_TYPE_ENUM:
                new_option->string_values = string_split (
                    string_values,
                    "|",
                    NULL,
                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                    0,
                    &argc);
                if (!new_option->string_values)
                    goto error;
                new_option->min = 0;
                new_option->max = (argc == 0) ? 0 : argc - 1;
                if (default_value)
                {
                    index_value = 0;
                    for (i = 0; i < argc; i++)
                    {
                        if (strcmp (new_option->string_values[i],
                                    default_value) == 0)
                        {
                            index_value = i;
                            break;
                        }
                    }
                    new_option->default_value = malloc (sizeof (int));
                    if (!new_option->default_value)
                        goto error;
                    CONFIG_ENUM_DEFAULT(new_option) = index_value;
                }
                if (value)
                {
                    index_value = 0;
                    for (i = 0; i < argc; i++)
                    {
                        if (strcmp (new_option->string_values[i],
                                    value) == 0)
                        {
                            index_value = i;
                            break;
                        }
                    }
                    new_option->value = malloc (sizeof (int));
                    if (!new_option->value)
                        goto error;
                    CONFIG_ENUM(new_option) = index_value;
                }
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
        new_option->null_value_allowed = null_value_allowed;
        new_option->callback_check_value = callback_check_value;
        new_option->callback_check_value_pointer = callback_check_value_pointer;
        new_option->callback_check_value_data = callback_check_value_data;
        new_option->callback_change = callback_change;
        new_option->callback_change_pointer = callback_change_pointer;
        new_option->callback_change_data = callback_change_data;
        new_option->callback_delete = callback_delete;
        new_option->callback_delete_pointer = callback_delete_pointer;
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

        config_file_hook_config_exec (new_option);
    }

    goto end;

error:
    if (new_option)
    {
        config_file_option_free_data (new_option);
        free (new_option);
        new_option = NULL;
    }

end:
    free (option_name);
    free (parent_name);
    return new_option;
}

/*
 * Searches for an option in a configuration file or section.
 *
 * Returns pointer to option found, NULL if error.
 */

struct t_config_option *
config_file_search_option (struct t_config_file *config_file,
                           struct t_config_section *section,
                           const char *option_name)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    int rc;

    if (!option_name)
        return NULL;

    if (section)
    {
        for (ptr_option = section->last_option; ptr_option;
             ptr_option = ptr_option->prev_option)
        {
            rc = strcmp (ptr_option->name, option_name);
            if (rc == 0)
                return ptr_option;
            else if (rc < 0)
                break;
        }
    }
    else if (config_file)
    {
        for (ptr_section = config_file->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->last_option; ptr_option;
                 ptr_option = ptr_option->prev_option)
            {
                rc = strcmp (ptr_option->name, option_name);
                if (rc == 0)
                    return ptr_option;
                else if (rc < 0)
                    break;
            }
        }
    }

    /* option not found */
    return NULL;
}

/*
 * Searches for an option in a configuration file or section.
 *
 * Returns section/option found (in section_found/option_found), NULL if not
 * found.
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
    int rc;

    *section_found = NULL;
    *option_found = NULL;

    if (!option_name)
        return;

    if (section)
    {
        for (ptr_option = section->last_option; ptr_option;
             ptr_option = ptr_option->prev_option)
        {
            rc = strcmp (ptr_option->name, option_name);
            if (rc == 0)
            {
                *section_found = section;
                *option_found = ptr_option;
                return;
            }
            else if (rc < 0)
                break;
        }
    }
    else if (config_file)
    {
        for (ptr_section = config_file->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->last_option; ptr_option;
                 ptr_option = ptr_option->prev_option)
            {
                rc = strcmp (ptr_option->name, option_name);
                if (rc == 0)
                {
                    *section_found = ptr_section;
                    *option_found = ptr_option;
                    return;
                }
                else if (rc < 0)
                    break;
            }
        }
    }
}

/*
 * Searches for a file/section/option using a full name of option (format:
 * "file.section.option").
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

    if (!option_name)
        return;

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

    free (file_name);
    free (section_name);

    if (config_file)
        *config_file = ptr_config;
    if (section)
        *section = ptr_section;
    if (option)
        *option = ptr_option;
}

/*
 * Gets pointer to parent option, NULL if the option has no parent.
 */

struct t_config_option *
config_file_get_parent_option (struct t_config_option *option)
{
    struct t_config_option *ptr_parent_option;

    if (!option || !option->parent_name)
        return NULL;

    config_file_search_with_string (
        option->parent_name,
        NULL,  /* config_file */
        NULL,  /* section */
        &ptr_parent_option,
        NULL);  /* pos_option_name */

    return ptr_parent_option;
}

/*
 * Checks if a string with boolean value is valid.
 *
 * Returns:
 *   1: boolean value is valid
 *   0: boolean value is NOT valid
 */

int
config_file_string_boolean_is_valid (const char *text)
{
    int i;

    if (!text)
        return 0;

    for (i = 0; config_boolean_true[i]; i++)
    {
        if (strcmp (text, config_boolean_true[i]) == 0)
            return 1;
    }

    for (i = 0; config_boolean_false[i]; i++)
    {
        if (strcmp (text, config_boolean_false[i]) == 0)
            return 1;
    }

    /* text is not a boolean */
    return 0;
}

/*
 * Converts string to boolean value.
 *
 * Returns:
 *   1: boolean value is true
 *   0: boolean value is false
 */

int
config_file_string_to_boolean (const char *text)
{
    int i;

    if (!text)
        return CONFIG_BOOLEAN_FALSE;

    for (i = 0; config_boolean_true[i]; i++)
    {
        if (strcmp (text, config_boolean_true[i]) == 0)
            return CONFIG_BOOLEAN_TRUE;
    }

    return CONFIG_BOOLEAN_FALSE;
}

/*
 * Resets an option to its default value.
 *
 * Returns:
 *   WEECHAT_CONFIG_OPTION_SET_OK_CHANGED: OK, value has been changed
 *   WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE: OK, value not changed
 *   WEECHAT_CONFIG_OPTION_SET_ERROR: error
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
                    else
                        break;
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
                        CONFIG_COLOR(option) = 0;
                    else
                        break;
                }
                if (CONFIG_COLOR(option) == CONFIG_COLOR_DEFAULT(option))
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                else
                {
                    CONFIG_COLOR(option) = CONFIG_COLOR_DEFAULT(option);
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                }
                break;
            case CONFIG_OPTION_TYPE_ENUM:
                if (!option->value)
                {
                    option->value = malloc (sizeof (int));
                    if (option->value)
                        CONFIG_ENUM(option) = 0;
                    else
                        break;
                }
                if (CONFIG_ENUM(option) == CONFIG_ENUM_DEFAULT(option))
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                else
                {
                    CONFIG_ENUM(option) = CONFIG_ENUM_DEFAULT(option);
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

    /* run callback and config hook(s) if value was changed */
    if (rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
    {
        if (run_callback && option->callback_change)
        {
            (void) (option->callback_change) (
                option->callback_change_pointer,
                option->callback_change_data,
                option);
        }
        config_file_hook_config_exec (option);
    }

    return rc;
}

/*
 * Sets the value for an option.
 *
 * Returns:
 *   WEECHAT_CONFIG_OPTION_SET_OK_CHANGED: OK, value has been changed
 *   WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE: OK, value not changed
 *   WEECHAT_CONFIG_OPTION_SET_ERROR: error
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
        if (!(int)(option->callback_check_value) (
                option->callback_check_value_pointer,
                option->callback_check_value_data,
                option,
                value))
        {
            return WEECHAT_CONFIG_OPTION_SET_ERROR;
        }
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
                        if (strcmp (value, "toggle") == 0)
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
                    if (strcmp (value, "toggle") == 0)
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
                    new_value_ok = 0;
                    if (strncmp (value, "++", 2) == 0)
                    {
                        error = NULL;
                        number = strtol (value + 2, &error, 10);
                        if (error && !error[0]
                            && (long)old_value + number <= (long)(option->max))
                        {
                            value_int = old_value + number;
                            new_value_ok = 1;
                        }
                    }
                    else if (strncmp (value, "--", 2) == 0)
                    {
                        error = NULL;
                        number = strtol (value + 2, &error, 10);
                        if (error && !error[0]
                            && (long)old_value - number >= (long)(option->min))
                        {
                            value_int = old_value - number;
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
                    else
                    {
                        if (old_value_was_null)
                        {
                            free (option->value);
                            option->value = NULL;
                        }
                    }
                }
                break;
            case CONFIG_OPTION_TYPE_ENUM:
                old_value = 0;
                if (!option->value)
                    option->value = malloc (sizeof (int));
                else
                    old_value = CONFIG_ENUM(option);
                if (option->value)
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
                            if (strcmp (option->string_values[i], value) == 0)
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
                            CONFIG_ENUM(option) = value_int;
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

    /* run callback and config hook(s) if value was changed */
    if (rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
    {
        if (run_callback && option->callback_change)
        {
            (void) (option->callback_change) (
                option->callback_change_pointer,
                option->callback_change_data,
                option);
        }
        config_file_hook_config_exec (option);
    }

    return rc;
}

/*
 * Toggles value of an option.
 *
 * Returns:
 *   WEECHAT_CONFIG_OPTION_SET_OK_CHANGED: OK, value has been changed
 *   WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE: OK, value not changed
 *   WEECHAT_CONFIG_OPTION_SET_ERROR: error
 */

int
config_file_option_toggle (struct t_config_option *option,
                           const char **values, int num_values,
                           int run_callback)
{
    char *current_value;
    const char *ptr_new_value, *empty_string = "";
    int i, rc, index_found, value_is_null, reset_value;

    if (!option || (num_values < 0))
        return WEECHAT_CONFIG_OPTION_SET_ERROR;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    ptr_new_value = NULL;
    reset_value = 0;

    value_is_null = (option->value == NULL);
    current_value = config_file_option_value_to_string (option, 0, 0, 0);

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (!values)
            {
                ptr_new_value = (option->value && CONFIG_BOOLEAN(option)) ?
                    config_boolean_false[0] : config_boolean_true[0];
            }
            break;
        case CONFIG_OPTION_TYPE_INTEGER:
            if (!values)
                goto end;
            break;
        case CONFIG_OPTION_TYPE_STRING:
            if (!values)
            {
                if (option->value && (strcmp (CONFIG_STRING(option), "") == 0))
                    ptr_new_value = CONFIG_STRING_DEFAULT(option);
                else
                    ptr_new_value = empty_string;
            }
            break;
        case CONFIG_OPTION_TYPE_COLOR:
            if (!values)
                goto end;
            break;
        case CONFIG_OPTION_TYPE_ENUM:
            if (!values)
                goto end;
            break;
        case CONFIG_NUM_OPTION_TYPES:
            /* make C compiler happy */
            break;
    }

    /* search new value to use with the provided list of values */
    if (!ptr_new_value && values)
    {
        index_found = -1;
        for (i = 0; i < num_values; i++)
        {
            if ((value_is_null && !values[i])
                || (!value_is_null && current_value && values[i]
                    && strcmp (current_value, values[i]) == 0))
            {
                index_found = i;
                break;
            }
        }
        if (index_found >= 0)
        {
            if (index_found + 1 < num_values)
            {
                ptr_new_value =  values[index_found + 1];
            }
            else
            {
                if (num_values < 2)
                    reset_value = 1;
                else
                    ptr_new_value = values[0];
            }
        }
        else
        {
            ptr_new_value = values[0];
        }
    }

    if (reset_value)
        rc = config_file_option_reset (option, run_callback);
    else
        rc = config_file_option_set (option, ptr_new_value, run_callback);

end:
    free (current_value);
    return rc;
}

/*
 * Sets null (undefined) value for an option.
 *
 * Returns:
 *   WEECHAT_CONFIG_OPTION_SET_OK_CHANGED: OK, value has been changed
 *   WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE: OK, value not changed
 *   WEECHAT_CONFIG_OPTION_SET_ERROR: error
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

    /* run callback and config hook(s) if value was changed */
    if (rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
    {
        if (run_callback && option->callback_change)
        {
            (void) (option->callback_change) (
                option->callback_change_pointer,
                option->callback_change_data,
                option);
        }
        config_file_hook_config_exec (option);
    }

    return rc;
}

/*
 * Sets the default value for an option.
 *
 * Returns:
 *   WEECHAT_CONFIG_OPTION_SET_OK_CHANGED: OK, default value has been changed
 *   WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE: OK, default value not changed
 *   WEECHAT_CONFIG_OPTION_SET_ERROR: error
 */

int
config_file_option_set_default (struct t_config_option *option,
                                const char *value,
                                int run_callback)
{
    int value_int, i, rc, new_value_ok, old_value_was_null, old_value;
    long number;
    char *error;

    if (!option)
        return WEECHAT_CONFIG_OPTION_SET_ERROR;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (value)
    {
        old_value_was_null = (option->default_value == NULL);
        switch (option->type)
        {
            case CONFIG_OPTION_TYPE_BOOLEAN:
                if (!option->default_value)
                {
                    option->default_value = malloc (sizeof (int));
                    if (option->default_value)
                    {
                        if (strcmp (value, "toggle") == 0)
                        {
                            CONFIG_BOOLEAN_DEFAULT(option) = CONFIG_BOOLEAN_TRUE;
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                        }
                        else
                        {
                            if (config_file_string_boolean_is_valid (value))
                            {
                                value_int = config_file_string_to_boolean (value);
                                CONFIG_BOOLEAN_DEFAULT(option) = value_int;
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                            }
                            else
                            {
                                free (option->default_value);
                                option->default_value = NULL;
                            }
                        }
                    }
                }
                else
                {
                    if (strcmp (value, "toggle") == 0)
                    {
                        CONFIG_BOOLEAN_DEFAULT(option) =
                            (CONFIG_BOOLEAN_DEFAULT(option) == CONFIG_BOOLEAN_TRUE) ?
                            CONFIG_BOOLEAN_FALSE : CONFIG_BOOLEAN_TRUE;
                        rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                    }
                    else
                    {
                        if (config_file_string_boolean_is_valid (value))
                        {
                            value_int = config_file_string_to_boolean (value);
                            if (value_int == CONFIG_BOOLEAN_DEFAULT(option))
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                            else
                            {
                                CONFIG_BOOLEAN_DEFAULT(option) = value_int;
                                rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                            }
                        }
                    }
                }
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                old_value = 0;
                if (!option->default_value)
                    option->default_value = malloc (sizeof (int));
                else
                    old_value = CONFIG_INTEGER_DEFAULT(option);
                if (option->default_value)
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
                            CONFIG_INTEGER_DEFAULT(option) = value_int;
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                        }
                        else
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                    }
                    else
                    {
                        if (old_value_was_null)
                        {
                            free (option->default_value);
                            option->default_value = NULL;
                        }
                    }
                }
                break;
            case CONFIG_OPTION_TYPE_STRING:
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                if (!option->default_value
                    || (strcmp (CONFIG_STRING_DEFAULT(option), value) != 0))
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                if (option->default_value)
                {
                    free (option->default_value);
                    option->default_value = NULL;
                }
                option->default_value = strdup (value);
                if (!option->default_value)
                    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                old_value = 0;
                if (!option->default_value)
                    option->default_value = malloc (sizeof (int));
                else
                    old_value = CONFIG_COLOR_DEFAULT(option);
                if (option->default_value)
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
                            CONFIG_COLOR_DEFAULT(option) = value_int;
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                        }
                        else
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                    }
                    else
                    {
                        if (old_value_was_null)
                        {
                            free (option->default_value);
                            option->default_value = NULL;
                        }
                    }
                }
                break;
            case CONFIG_OPTION_TYPE_ENUM:
                old_value = 0;
                if (!option->default_value)
                    option->default_value = malloc (sizeof (int));
                else
                    old_value = CONFIG_ENUM_DEFAULT(option);
                if (option->default_value)
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
                            if (strcmp (option->string_values[i], value) == 0)
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
                            CONFIG_ENUM_DEFAULT(option) = value_int;
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
                        }
                        else
                            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                    }
                    else
                    {
                        if (old_value_was_null)
                        {
                            free (option->default_value);
                            option->default_value = NULL;
                        }
                    }
                }
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
        if (old_value_was_null && option->default_value)
            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
    }
    else
    {
        if (option->null_value_allowed && option->default_value)
        {
            free (option->default_value);
            option->default_value = NULL;
            rc = WEECHAT_CONFIG_OPTION_SET_OK_CHANGED;
        }
        else
            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    }

    /* run callback and config hook(s) if default value was changed */
    if (rc == WEECHAT_CONFIG_OPTION_SET_OK_CHANGED)
    {
        if (run_callback && option->callback_change)
        {
            (void) (option->callback_change) (
                option->callback_change_pointer,
                option->callback_change_data,
                option);
        }
        config_file_hook_config_exec (option);
    }

    return rc;
}

/*
 * Unsets/resets an option.
 *
 * Returns:
 *   WEECHAT_CONFIG_OPTION_UNSET_OK_NO_RESET: OK, value has not been reset
 *   WEECHAT_CONFIG_OPTION_UNSET_OK_RESET: OK, value has been reset
 *   WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED: OK, value has been removed
 *   WEECHAT_CONFIG_OPTION_UNSET_ERROR: error
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
            (void) (option->callback_delete) (
                option->callback_delete_pointer,
                option->callback_delete_data,
                option);
        }

        option_full_name = config_file_option_full_name (option);

        if (option->section->callback_delete_option)
        {
            rc = (int) (option->section->callback_delete_option) (
                option->section->callback_delete_option_pointer,
                option->section->callback_delete_option_data,
                option->config_file,
                option->section,
                option);
        }
        else
        {
            config_file_option_free (option, 0);
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
 * Renames an option.
 */

void
config_file_option_rename (struct t_config_option *option,
                           const char *new_name)
{
    char *str_new_name, *full_old_name, *full_new_name;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    if (!option || !new_name || !new_name[0]
        || config_file_search_option (option->config_file, option->section, new_name))
        return;

    full_old_name = config_file_option_full_name (option);

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
        free (option->name);
        option->name = str_new_name;

        /* re-insert option in section */
        if (option->section)
            config_file_option_insert_in_section (option);
    }

    full_new_name = config_file_option_full_name (option);

    /* rename "parent_name" in any option using the old option name */
    if (full_old_name && full_new_name)
    {
        for (ptr_config = config_files; ptr_config;
             ptr_config = ptr_config->next_config)
        {
            for (ptr_section = ptr_config->sections; ptr_section;
                 ptr_section = ptr_section->next_section)
            {
                for (ptr_option = ptr_section->options; ptr_option;
                     ptr_option = ptr_option->next_option)
                {
                    if (ptr_option->parent_name
                        && (strcmp (ptr_option->parent_name, full_old_name) == 0))
                    {
                        free (ptr_option->parent_name);
                        ptr_option->parent_name = strdup (full_new_name);
                    }
                }
            }
        }
    }

    free (full_old_name);
    free (full_new_name);

    config_file_hook_config_exec (option);
}

/*
 * Builds a string with the value or default value of option,
 * depending on the type of option.
 *
 * According to default_value:
 *   0: value of option is returned
 *   1: default value of option is returned
 *
 * Note: result must be freed after use.
 */

char *
config_file_option_value_to_string (struct t_config_option *option,
                                    int default_value,
                                    int use_colors,
                                    int use_delimiters)
{
    char *value;
    const char *ptr_value;
    int enabled;

    if (!option)
        return NULL;

    if ((default_value && !option->default_value)
        || (!default_value && !option->value))
    {
        string_asprintf (
            &value,
            "%s%s",
            (use_colors) ? GUI_COLOR(GUI_COLOR_CHAT_VALUE_NULL) : "",
            "null");
        return value;
    }

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            enabled = (default_value) ?
                CONFIG_BOOLEAN_DEFAULT(option) : CONFIG_BOOLEAN(option);
            string_asprintf (
                &value,
                "%s%s",
                (use_colors) ? GUI_COLOR(GUI_COLOR_CHAT_VALUE) : "",
                (enabled) ? "on" : "off");
            return value;
        case CONFIG_OPTION_TYPE_INTEGER:
            string_asprintf (
                &value,
                "%s%d",
                (use_colors) ? GUI_COLOR(GUI_COLOR_CHAT_VALUE) : "",
                (default_value) ? CONFIG_INTEGER_DEFAULT(option) : CONFIG_INTEGER(option));
            return value;
        case CONFIG_OPTION_TYPE_STRING:
            ptr_value = (default_value) ? CONFIG_STRING_DEFAULT(option) : CONFIG_STRING(option);
            string_asprintf (
                &value,
                "%s%s%s%s%s%s",
                (use_colors && use_delimiters) ? GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS) : "",
                (use_delimiters) ? "\"" : "",
                (use_colors) ? GUI_COLOR(GUI_COLOR_CHAT_VALUE) : "",
                ptr_value,
                (use_colors && use_delimiters) ? GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS) : "",
                (use_delimiters) ? "\"" : "");
            return value;
        case CONFIG_OPTION_TYPE_COLOR:
            ptr_value = gui_color_get_name (
                (default_value) ? CONFIG_COLOR_DEFAULT(option) : CONFIG_COLOR(option));
            if (!ptr_value)
                return NULL;
            string_asprintf (
                &value,
                "%s%s",
                (use_colors) ? GUI_COLOR(GUI_COLOR_CHAT_VALUE) : "",
                ptr_value);
            return value;
        case CONFIG_OPTION_TYPE_ENUM:
            ptr_value = (default_value) ?
                option->string_values[CONFIG_ENUM_DEFAULT(option)] :
                option->string_values[CONFIG_ENUM(option)];
            string_asprintf (
                &value,
                "%s%s",
                (use_colors) ? GUI_COLOR(GUI_COLOR_CHAT_VALUE) : "",
                ptr_value);
            return value;
        case CONFIG_NUM_OPTION_TYPES:
            /* make C compiler happy */
            break;
    }

    /* make C static analyzer happy (never executed) */
    return NULL;
}

/*
 * Gets a string value of an option property.
 */

const char *
config_file_option_get_string (struct t_config_option *option,
                               const char *property)
{
    if (!option || !property)
        return NULL;

    if (strcmp (property, "config_name") == 0)
        return option->config_file->name;
    else if (strcmp (property, "section_name") == 0)
        return option->section->name;
    else if (strcmp (property, "name") == 0)
        return option->name;
    else if (strcmp (property, "parent_name") == 0)
        return option->parent_name;
    else if (strcmp (property, "type") == 0)
        return config_option_type_string[option->type];
    else if (strcmp (property, "description") == 0)
        return option->description;

    return NULL;
}

/*
 * Gets a pointer on an option property.
 */

void *
config_file_option_get_pointer (struct t_config_option *option,
                                const char *property)
{
    if (!option || !property)
        return NULL;

    if (strcmp (property, "config_file") == 0)
        return option->config_file;
    else if (strcmp (property, "section") == 0)
        return option->section;
    else if (strcmp (property, "name") == 0)
        return option->name;
    else if (strcmp (property, "parent_name") == 0)
        return option->parent_name;
    else if (strcmp (property, "type") == 0)
        return &option->type;
    else if (strcmp (property, "description") == 0)
        return option->description;
    else if (strcmp (property, "string_values") == 0)
        return option->string_values;
    else if (strcmp (property, "min") == 0)
        return &option->min;
    else if (strcmp (property, "max") == 0)
        return &option->max;
    else if (strcmp (property, "default_value") == 0)
        return option->default_value;
    else if (strcmp (property, "value") == 0)
        return option->value;
    else if (strcmp (property, "prev_option") == 0)
        return option->prev_option;
    else if (strcmp (property, "next_option") == 0)
        return option->next_option;

    return NULL;
}

/*
 * Checks if an option has a null value.
 *
 * Returns:
 *   1: value of option is null
 *   0: value of option is not null
 */

int
config_file_option_is_null (struct t_config_option *option)
{
    if (!option)
        return 1;

    return (option->value) ? 0 : 1;
}

/*
 * Checks if an option has a null default value.
 *
 * Returns:
 *   1: default value of option is null
 *   0: default value of option is not null
 */

int
config_file_option_default_is_null (struct t_config_option *option)
{
    if (!option)
        return 1;

    return (option->default_value) ? 0 : 1;
}

/*
 * Checks if an option has changed (current value different from default value).
 *
 * Returns:
 *   1: option has changed
 *   0: option has default value
 */

int config_file_option_has_changed (struct t_config_option *option)
{
    /* both default and current value are null => not changed */
    if (!option->default_value && !option->value)
        return 0;

    /* default is null and current value is not null => changed! */
    if (!option->default_value && option->value)
        return 1;

    /* default is not null and current value is null => changed! */
    if (option->default_value && !option->value)
        return 1;

    /* both default and current value are not null, compare their values */
    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            return CONFIG_BOOLEAN(option) != CONFIG_BOOLEAN_DEFAULT(option);
        case CONFIG_OPTION_TYPE_INTEGER:
            return CONFIG_INTEGER(option) != CONFIG_INTEGER_DEFAULT(option);
        case CONFIG_OPTION_TYPE_STRING:
            return strcmp (CONFIG_STRING(option), CONFIG_STRING_DEFAULT(option)) != 0;
        case CONFIG_OPTION_TYPE_COLOR:
            return CONFIG_COLOR(option) != CONFIG_COLOR_DEFAULT(option);
        case CONFIG_OPTION_TYPE_ENUM:
            return CONFIG_ENUM(option) != CONFIG_ENUM_DEFAULT(option);
        case CONFIG_NUM_OPTION_TYPES:
            /* make C compiler happy */
            break;
    }

    return 0;
}

/*
 * Sets the value for an option using a full name of option (format:
 * "file.section.option").
 *
 * Returns:
 *   WEECHAT_CONFIG_OPTION_SET_OK_CHANGED: OK, value has been changed
 *   WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE: OK, value not changed
 *   WEECHAT_CONFIG_OPTION_SET_ERROR: error
 *   WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND: option not found
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
                rc = (int) (ptr_section->callback_create_option) (
                    ptr_section->callback_create_option_pointer,
                    ptr_section->callback_create_option_data,
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
 * Returns boolean value of an option.
 *
 * Returns 1 if value is true, 0 if it is false.
 */

int
config_file_option_boolean (struct t_config_option *option)
{
    if (!option || !option->value || (option->type != CONFIG_OPTION_TYPE_BOOLEAN))
        return 0;

    return CONFIG_BOOLEAN(option);
}

/*
 * Returns default boolean value of an option.
 *
 * Returns 1 if default value is true, 0 if it is false.
 */

int
config_file_option_boolean_default (struct t_config_option *option)
{
    if (!option || !option->default_value || (option->type != CONFIG_OPTION_TYPE_BOOLEAN))
        return 0;

    return CONFIG_BOOLEAN_DEFAULT(option);
}

/*
 * Returns inherited boolean value of an option: value of option if not NULL,
 * or value of the parent option (if option inherits from another option).
 *
 * If the parent option is not found, returns the default value of the option.
 * If the parent value is NULL, returns the default value of the parent option.
 */

int
config_file_option_boolean_inherited (struct t_config_option *option)
{
    struct t_config_option *ptr_parent_option;

    if (option && option->value)
    {
        return config_file_option_boolean (option);
    }
    else
    {
        ptr_parent_option = config_file_get_parent_option (option);
        if (!ptr_parent_option)
            return config_file_option_boolean_default (option);
        if (!ptr_parent_option->value)
            return config_file_option_boolean_default (ptr_parent_option);
        return config_file_option_boolean (ptr_parent_option);
    }
}

/*
 * Returns integer value of an option.
 */

int
config_file_option_integer (struct t_config_option *option)
{
    if (!option || !option->value)
        return 0;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE)
                return 1;
            else
                return 0;
        case CONFIG_OPTION_TYPE_INTEGER:
            return CONFIG_INTEGER(option);
        case CONFIG_OPTION_TYPE_STRING:
            return 0;
        case CONFIG_OPTION_TYPE_COLOR:
            return CONFIG_COLOR(option);
        case CONFIG_OPTION_TYPE_ENUM:
            return CONFIG_ENUM(option);
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
    return 0;
}

/*
 * Returns default integer value of an option.
 */

int
config_file_option_integer_default (struct t_config_option *option)
{
    if (!option || !option->default_value)
        return 0;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN_DEFAULT(option) == CONFIG_BOOLEAN_TRUE)
                return 1;
            else
                return 0;
        case CONFIG_OPTION_TYPE_INTEGER:
            return CONFIG_INTEGER_DEFAULT(option);
        case CONFIG_OPTION_TYPE_STRING:
            return 0;
        case CONFIG_OPTION_TYPE_COLOR:
            return CONFIG_COLOR_DEFAULT(option);
        case CONFIG_OPTION_TYPE_ENUM:
            return CONFIG_ENUM_DEFAULT(option);
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
    return 0;
}

/*
 * Returns inherited integer value of an option: value of option if not NULL,
 * or value of the parent option (if option inherits from another option).
 *
 * If the parent option is not found, returns the default value of the option.
 * If the parent value is NULL, returns the default value of the parent option.
 */

int
config_file_option_integer_inherited (struct t_config_option *option)
{
    struct t_config_option *ptr_parent_option;

    if (option && option->value)
    {
        return config_file_option_integer (option);
    }
    else
    {
        ptr_parent_option = config_file_get_parent_option (option);
        if (!ptr_parent_option)
            return config_file_option_integer_default (option);
        if (!ptr_parent_option->value)
            return config_file_option_integer_default (ptr_parent_option);
        return config_file_option_integer (ptr_parent_option);
    }
}

/*
 * Returns string value of an option.
 */

const char *
config_file_option_string (struct t_config_option *option)
{
    if (!option || !option->value)
        return NULL;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN(option))
                return config_boolean_true[0];
            else
                return config_boolean_false[0];
        case CONFIG_OPTION_TYPE_INTEGER:
            return NULL;
        case CONFIG_OPTION_TYPE_STRING:
            return CONFIG_STRING(option);
        case CONFIG_OPTION_TYPE_COLOR:
            return gui_color_get_name (CONFIG_COLOR(option));
        case CONFIG_OPTION_TYPE_ENUM:
            return option->string_values[CONFIG_ENUM(option)];
        case CONFIG_NUM_OPTION_TYPES:
            return NULL;
    }
    return NULL;
}

/*
 * Returns default string value of an option.
 */

const char *
config_file_option_string_default (struct t_config_option *option)
{
    if (!option || !option->default_value)
        return NULL;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN_DEFAULT(option))
                return config_boolean_true[0];
            else
                return config_boolean_false[0];
        case CONFIG_OPTION_TYPE_INTEGER:
            return NULL;
        case CONFIG_OPTION_TYPE_STRING:
            return CONFIG_STRING_DEFAULT(option);
        case CONFIG_OPTION_TYPE_COLOR:
            return gui_color_get_name (CONFIG_COLOR_DEFAULT(option));
        case CONFIG_OPTION_TYPE_ENUM:
            return option->string_values[CONFIG_ENUM_DEFAULT(option)];
        case CONFIG_NUM_OPTION_TYPES:
            return NULL;
    }
    return NULL;
}

/*
 * Returns inherited string value of an option: value of option if not NULL,
 * or value of the parent option (if option inherits from another option).
 *
 * If the parent option is not found, returns the default value of the option.
 * If the parent value is NULL, returns the default value of the parent option.
 */

const char *
config_file_option_string_inherited (struct t_config_option *option)
{
    struct t_config_option *ptr_parent_option;

    if (option && option->value)
    {
        return config_file_option_string (option);
    }
    else
    {
        ptr_parent_option = config_file_get_parent_option (option);
        if (!ptr_parent_option)
            return config_file_option_string_default (option);
        if (!ptr_parent_option->value)
            return config_file_option_string_default (ptr_parent_option);
        return config_file_option_string (ptr_parent_option);
    }
}

/*
 * Returns color value of an option.
 */

const char *
config_file_option_color (struct t_config_option *option)
{
    if (!option || !option->value || (option->type != CONFIG_OPTION_TYPE_COLOR))
        return NULL;

    return gui_color_get_name (CONFIG_COLOR(option));
}

/*
 * Returns default color value of an option.
 */

const char *
config_file_option_color_default (struct t_config_option *option)
{
    if (!option || !option->default_value || (option->type != CONFIG_OPTION_TYPE_COLOR))
        return NULL;

    return gui_color_get_name (CONFIG_COLOR_DEFAULT(option));
}

/*
 * Returns inherited color value of an option: value of option if not NULL,
 * or value of the parent option (if option inherits from another option).
 *
 * If the parent option is not found, returns the default value of the option.
 * If the parent value is NULL, returns the default value of the parent option.
 */

const char *
config_file_option_color_inherited (struct t_config_option *option)
{
    struct t_config_option *ptr_parent_option;

    if (option && option->value)
        return config_file_option_color (option);
    else
    {
        ptr_parent_option = config_file_get_parent_option (option);
        if (!ptr_parent_option)
            return config_file_option_color_default (option);
        if (!ptr_parent_option->value)
            return config_file_option_color_default (ptr_parent_option);
        return config_file_option_color (ptr_parent_option);
    }
}

/*
 * Returns enum value of an option.
 */

int
config_file_option_enum (struct t_config_option *option)
{
    if (!option || !option->value)
        return 0;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE)
                return 1;
            else
                return 0;
        case CONFIG_OPTION_TYPE_INTEGER:
            return CONFIG_INTEGER(option);
        case CONFIG_OPTION_TYPE_STRING:
            return 0;
        case CONFIG_OPTION_TYPE_COLOR:
            return CONFIG_COLOR(option);
        case CONFIG_OPTION_TYPE_ENUM:
            return CONFIG_ENUM(option);
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
    return 0;
}

/*
 * Returns default enum value of an option.
 */

int
config_file_option_enum_default (struct t_config_option *option)
{
    if (!option || !option->default_value)
        return 0;

    switch (option->type)
    {
        case CONFIG_OPTION_TYPE_BOOLEAN:
            if (CONFIG_BOOLEAN_DEFAULT(option) == CONFIG_BOOLEAN_TRUE)
                return 1;
            else
                return 0;
        case CONFIG_OPTION_TYPE_INTEGER:
            return CONFIG_INTEGER_DEFAULT(option);
        case CONFIG_OPTION_TYPE_STRING:
            return 0;
        case CONFIG_OPTION_TYPE_COLOR:
            return CONFIG_COLOR_DEFAULT(option);
        case CONFIG_OPTION_TYPE_ENUM:
            return CONFIG_ENUM_DEFAULT(option);
        case CONFIG_NUM_OPTION_TYPES:
            break;
    }
    return 0;
}

/*
 * Returns inherited enum value of an option: value of option if not NULL,
 * or value of the parent option (if option inherits from another option).
 *
 * If the parent option is not found, returns the default value of the option.
 * If the parent value is NULL, returns the default value of the parent option.
 */

int
config_file_option_enum_inherited (struct t_config_option *option)
{
    struct t_config_option *ptr_parent_option;

    if (option && option->value)
    {
        return config_file_option_enum (option);
    }
    else
    {
        ptr_parent_option = config_file_get_parent_option (option);
        if (!ptr_parent_option)
            return config_file_option_enum_default (option);
        if (!ptr_parent_option->value)
            return config_file_option_enum_default (ptr_parent_option);
        return config_file_option_enum (ptr_parent_option);
    }
}

/*
 * Returns a char to add before the name of option to escape it.
 *
 * Returns:
 *   "\": name must be escaped with "\" (if names begins with # [ \)
 *   "": name must not be escaped
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
 * Writes an option in a configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
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
                rc = string_fprintf (config_file->file, "%s%s = %s\n",
                                     config_file_option_escape (option->name),
                                     option->name,
                                     (CONFIG_BOOLEAN(option) == CONFIG_BOOLEAN_TRUE) ?
                                     "on" : "off");
                break;
            case CONFIG_OPTION_TYPE_INTEGER:
                rc = string_fprintf (config_file->file, "%s%s = %d\n",
                                     config_file_option_escape (option->name),
                                     option->name,
                                     CONFIG_INTEGER(option));
                break;
            case CONFIG_OPTION_TYPE_STRING:
                rc = string_fprintf (config_file->file, "%s%s = \"%s\"\n",
                                     config_file_option_escape (option->name),
                                     option->name,
                                     (char *)option->value);
                break;
            case CONFIG_OPTION_TYPE_COLOR:
                rc = string_fprintf (config_file->file, "%s%s = %s\n",
                                     config_file_option_escape (option->name),
                                     option->name,
                                     gui_color_get_name (CONFIG_COLOR(option)));
                break;
            case CONFIG_OPTION_TYPE_ENUM:
                rc = string_fprintf (config_file->file, "%s%s = %s\n",
                                     config_file_option_escape (option->name),
                                     option->name,
                                     option->string_values[CONFIG_ENUM(option)]);
                break;
            case CONFIG_NUM_OPTION_TYPES:
                break;
        }
    }
    else
    {
        rc = string_fprintf (config_file->file, "%s%s\n",
                             config_file_option_escape (option->name),
                             option->name);
    }

    return rc;
}

/*
 * Writes a line in a configuration file.
 *
 * If value is NULL, then writes a section with [ ] around.
 *
 * Returns:
 *   1: OK
 *   0: error
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
                rc = string_fprintf (config_file->file, "%s%s = %s\n",
                                     config_file_option_escape (option_name),
                                     option_name, vbuffer);
                free (vbuffer);
                return rc;
            }
            free (vbuffer);
        }
    }

    return (string_fprintf (config_file->file, "\n[%s]\n",
                            option_name));
}

/*
 * Writes a configuration file (this function must not be called directly).
 *
 * Returns:
 *   WEECHAT_CONFIG_WRITE_OK: OK
 *   WEECHAT_CONFIG_WRITE_ERROR: error
 *   WEECHAT_CONFIG_WRITE_MEMORY_ERROR: not enough memory
 */

int
config_file_write_internal (struct t_config_file *config_file,
                            int default_options)
{
    int rc;
    long file_perms;
    char *filename, *filename2, resolved_path[PATH_MAX], *error;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    if (!config_file)
        return WEECHAT_CONFIG_WRITE_ERROR;

    /* build filename */
    if (string_asprintf (&filename,
                         "%s%s%s",
                         weechat_config_dir,
                         DIR_SEPARATOR,
                         config_file->filename) < 0)
    {
        return WEECHAT_CONFIG_WRITE_MEMORY_ERROR;
    }

    /*
     * build temporary filename, this temp file will be renamed to filename
     * after write
     */
    if (string_asprintf (&filename2, "%s.weechattmp", filename) < 0)
    {
        free (filename);
        return WEECHAT_CONFIG_WRITE_MEMORY_ERROR;
    }

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

    log_printf (_("Writing configuration file %s%s%s"),
                config_file->filename,
                (default_options) ? " " : "",
                (default_options) ? _("(default options)") : "");

    /* open temp file in write mode */
    config_file->file = fopen (filename2, "wb");
    if (!config_file->file)
    {
        gui_chat_printf (NULL,
                         _("%sCannot create file \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename2);
        goto error;
    }

    /* write header with name of config file and WeeChat version */
    if (!string_fprintf (
            config_file->file,
            "#\n"
            "# %s -- %s\n"
            "#\n"
            "# WARNING: It is NOT recommended to edit this file by hand,\n"
            "# especially if WeeChat is running.\n"
            "#\n"
            "# Use commands like /set or /fset to change settings in WeeChat.\n"
            "#\n"
            "# For more info, see: https://weechat.org/doc/weechat/quickstart/\n"
            "#\n",
            version_get_name (),
            config_file->filename))
    {
        goto error;
    }

    /* write config version (if different from 1) */
    if (config_file->version > 1)
    {
        if (!string_fprintf (config_file->file,
                             "\nconfig_version = %d\n",
                             config_file->version))
        {
            goto error;
        }
    }

    /* write all sections */
    for (ptr_section = config_file->sections; ptr_section;
         ptr_section = ptr_section->next_section)
    {
        /* call write callback if defined for section */
        if (default_options && ptr_section->callback_write_default)
        {
            if ((ptr_section->callback_write_default) (
                    ptr_section->callback_write_default_pointer,
                    ptr_section->callback_write_default_data,
                    config_file,
                    ptr_section->name) != WEECHAT_CONFIG_WRITE_OK)
                goto error;
        }
        else if (!default_options && ptr_section->callback_write)
        {
            if ((ptr_section->callback_write) (
                    ptr_section->callback_write_pointer,
                    ptr_section->callback_write_data,
                    config_file,
                    ptr_section->name) != WEECHAT_CONFIG_WRITE_OK)
                goto error;
        }
        else
        {
            /* write all options for section */
            if (!string_fprintf (config_file->file,
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

    /*
     * ensure the file is really written on the storage device;
     * this is disabled by default because it is really slow
     * (about 20 to 200x slower)
     */
    if (CONFIG_BOOLEAN(config_look_save_config_with_fsync))
    {
        if (fsync (fileno (config_file->file)) != 0)
            goto error;
    }

    /* close temp file */
    fclose (config_file->file);
    config_file->file = NULL;

    /* update file mode */
    error = NULL;
    file_perms = strtol (CONFIG_STRING(config_look_config_permissions), &error, 8);
    if (!error || error[0])
        file_perms = 0600;
    if (chmod (filename2, file_perms) < 0)
    {
        gui_chat_printf (
            NULL,
            _("%sWARNING: failed to set permissions on configuration file "
              "\"%s\" (%s)"),
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
            filename2,
            strerror (errno));
    }

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
 * Writes a configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_WRITE_OK: OK
 *   WEECHAT_CONFIG_WRITE_ERROR: error
 *   WEECHAT_CONFIG_WRITE_MEMORY_ERROR: not enough memory
 */

int
config_file_write (struct t_config_file *config_file)
{
    return config_file_write_internal (config_file, 0);
}

/*
 * Parses configuration version.
 *
 * Returns:
 *   >= 1: configuration version
 *     -1: error
 */

int
config_file_parse_version (const char *version)
{
    long number;
    char *error;

    if (!version)
        return -1;

    error = NULL;
    number = strtoll (version, &error, 10);
    if (!error || error[0])
        return -1;

    return (number < 1) ? -1 : (int)number;
}

/*
 * Backups a configuration file if its version is unsupported and cannot be
 * loaded.
 */

void
config_file_backup (const char *filename)
{
    char *filename_backup, str_time[32], str_index[32];
    int length, index;
    struct tm *local_time;
    time_t date;

    if (!filename)
        return;

    length = strlen (filename) + 128;

    filename_backup = malloc (length);
    if (!filename_backup)
        return;

    date = time (NULL);
    local_time = localtime (&date);
    if (strftime (str_time, sizeof (str_time), ".%Y%m%d.%H%M%S", local_time) == 0)
        str_time[0] = '\0';

    index = 1;
    while (1)
    {
        if (index == 1)
            str_index[0] = '\0';
        else
            snprintf (str_index, sizeof (str_index), ".%d", index);
        snprintf (filename_backup, length,
                  "%s.backup%s%s",
                  filename, str_time, str_index);
        if (access (filename_backup, F_OK) != 0)
            break;
        index++;
    }

    if (dir_file_copy (filename, filename_backup))
    {
        gui_chat_printf (NULL,
                         _("%sFile %s has been backed up as %s"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename, filename_backup);
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sError: unable to backup file %s"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename);
    }

    free (filename_backup);
}

/*
 * Updates data read from config file: either section or option + value.
 * The update callback (if defined in config) is called if the config version
 * read in file is less than to the current config version.
 *
 * Parameters "section", "option" and "value" are updated in place: if the
 * callback gives a new value, they are first freed and allocated again with
 * the new value (or set to NULL for the value if the callback returns
 * special key "value_null").
 *
 * Section can be updated only if option and value are NULL (ie if we are
 * reading a section line like "[section]").
 *
 * Integer warning_update_displayed is set to 1 if a warning is displayed,
 * when the file is updated to a newer version (then it's not compatible any
 * more with previous versions).
 */

void
config_file_update_data_read (struct t_config_file *config_file,
                              const char *filename,
                              const char *section,
                              const char *option,
                              const char *value,
                              char **ret_section,
                              char **ret_option,
                              char **ret_value,
                              int *warning_update_displayed)
{
    struct t_hashtable *data_read, *hashtable;
    const char *ptr_section, *ptr_option, *ptr_value;
    int value_null;

    /* do nothing if config is already the latest version */
    if (config_file->version_read >= config_file->version)
        return;

    if (!*warning_update_displayed
        && (config_file->version_read < config_file->version))
    {
        gui_chat_printf (
            NULL,
            _("%sImportant: file %s has been updated from version %d to %d, "
              "it is not compatible and cannot be loaded anymore with any "
              "older version"),
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
            filename,
            config_file->version_read,
            config_file->version);
        *warning_update_displayed = 1;
    }

    /* do nothing if there's no update callback */
    if (!config_file->callback_update)
        return;

    value_null = 0;

    data_read = hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (!data_read)
        return;

    hashtable_set (data_read, "config", config_file->name);
    if (section)
        hashtable_set (data_read, "section", section);
    if (option)
    {
        hashtable_set (data_read, "option", option);
        if (value)
        {
            hashtable_set (data_read, "value", value);
        }
        else
        {
            hashtable_set (data_read, "value_null", "1");
            value_null = 1;
        }
    }

    hashtable = (config_file->callback_update)
        (config_file->callback_update_pointer,
         config_file->callback_update_data,
         config_file,
         config_file->version_read,
         data_read);

    if (hashtable)
    {
        /* if reading a section line, we can update its name */
        if (section && !option && ret_section)
        {
            ptr_section = hashtable_get (hashtable, "section");
            if (ptr_section && ptr_section[0])
            {
                free (*ret_section);
                *ret_section = strdup (ptr_section);
            }
        }

        /* if reading an option line, we can update its name and value */
        if (section && option)
        {
            /* option name */
            if (ret_option)
            {
                ptr_option = hashtable_get (hashtable, "option");
                if (ptr_option)
                {
                    free (*ret_option);
                    *ret_option = strdup (ptr_option);
                }
            }
            /* value */
            if (ret_value)
            {
                ptr_value = hashtable_get (hashtable, "value");
                if (!value_null && hashtable_has_key (hashtable, "value_null"))
                    ptr_value = NULL;
                free (*ret_value);
                *ret_value = (ptr_value) ? strdup (ptr_value) : NULL;
            }
        }
    }

    if (hashtable && (hashtable != data_read))
        hashtable_free (hashtable);
    hashtable_free (data_read);
}

/*
 * Reads a configuration file (this function must not be called directly).
 *
 * Returns:
 *   WEECHAT_CONFIG_READ_OK: OK
 *   WEECHAT_CONFIG_READ_MEMORY_ERROR: not enough memory
 *   WEECHAT_CONFIG_READ_FILE_NOT_FOUND: file not found
 */

int
config_file_read_internal (struct t_config_file *config_file, int reload)
{
    int line_number, rc, length, version, warning_update_displayed;
    char *filename, *section, *option, *value;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char line[16384], *ptr_line, *ptr_line2, *pos, *pos2;

    if (!config_file)
        return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;

    config_file->version_read = 1;
    warning_update_displayed = 0;

    /* build filename */
    if (string_asprintf (&filename,
                         "%s%s%s",
                         weechat_config_dir,
                         DIR_SEPARATOR,
                         config_file->filename) < 0)
    {
        return WEECHAT_CONFIG_READ_MEMORY_ERROR;
    }

    /* create file with default options if it does not exist */
    if (access (filename, F_OK) != 0)
    {
        if (strcmp (config_file->name, WEECHAT_CONFIG_NAME) == 0)
            weechat_first_start = 1;
        config_file_write_internal (config_file, 1);
    }

    /* read config file */
    config_file->file = fopen (filename, "r");
    if (!config_file->file)
    {
        gui_chat_printf (NULL,
                         _("%sWARNING: failed to read configuration file "
                           "\"%s\" (%s)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename,
                         strerror (errno));
        gui_chat_printf (NULL,
                         _("%sWARNING: file \"%s\" will be overwritten on exit "
                           "with default values (it is HIGHLY recommended to "
                           "backup this file now)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         filename);
        free (filename);
        return WEECHAT_CONFIG_READ_FILE_NOT_FOUND;
    }

    if (!reload)
        log_printf (_("Reading configuration file %s"), config_file->filename);

    /* read all lines */
    ptr_section = NULL;
    line_number = 0;
    while (!feof (config_file->file))
    {
        line_number++;

        option = NULL;
        value = NULL;

        ptr_line = fgets (line, sizeof (line) - 1, config_file->file);
        if (!ptr_line)
            goto end_line;

        /* encode line to internal charset */
        ptr_line2 = string_iconv_to_internal (NULL, ptr_line);
        if (ptr_line2)
        {
            snprintf (line, sizeof (line), "%s", ptr_line2);
            free (ptr_line2);
        }

        /* skip spaces */
        while (ptr_line[0] == ' ')
        {
            ptr_line++;
        }

        /* remove CR/LF */
        pos = strchr (ptr_line, '\r');
        if (pos)
            pos[0] = '\0';
        pos = strchr (ptr_line, '\n');
        if (pos)
            pos[0] = '\0';

        /* ignore empty line or comment */
        if (!ptr_line[0] || (ptr_line[0] == '#'))
            goto end_line;

        /* beginning of section */
        if ((ptr_line[0] == '[') && !strchr (ptr_line, '='))
        {
            pos = strchr (ptr_line, ']');
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
                section = string_strndup (ptr_line + 1, pos - ptr_line - 1);
                if (section)
                {
                    config_file_update_data_read (config_file, filename,
                                                  section, NULL, NULL,
                                                  &section, NULL, NULL,
                                                  &warning_update_displayed);
                    ptr_section = config_file_search_section (config_file,
                                                              section);
                    if (!ptr_section)
                    {
                        gui_chat_printf (
                            NULL,
                            _("%sWarning: %s, line %d: ignoring unknown "
                              "section identifier (\"%s\")"),
                            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                            filename, line_number, section);
                    }
                    free (section);
                }
            }
            goto end_line;
        }

        /* skip escape char */
        if (ptr_line[0] == '\\')
            ptr_line++;

        pos = strstr (ptr_line, " =");
        if (pos)
        {
            /* skip spaces before '=' */
            pos2 = pos - 1;
            while ((pos2 > ptr_line) && (pos2[0] == ' '))
            {
                pos2--;
            }
            option = string_strndup (ptr_line, pos2 + 1 - ptr_line);
            /* skip spaces after '=' */
            pos += 2;
            while (pos[0] == ' ')
            {
                pos++;
            }
            if (strcmp (pos, WEECHAT_CONFIG_OPTION_NULL) != 0)
            {
                length = strlen (pos);
                if (length > 1)
                {
                    /* remove simple or double quotes and spaces at the end */
                    pos2 = pos + length - 1;
                    while ((pos2 > pos) && (pos2[0] == ' '))
                    {
                        pos2--;
                    }
                    if (((pos[0] == '\'') && (pos2[0] == '\''))
                        || ((pos[0] == '"') && (pos2[0] == '"')))
                    {
                        value = string_strndup (pos + 1, pos2 - pos - 1);
                    }
                    else
                    {
                        value = string_strndup (pos, pos2 + 1 - pos);
                    }
                }
                else
                {
                    value = strdup (pos);
                }
            }
        }
        else
        {
            option = strdup (ptr_line);
        }

        if (!ptr_section && (strcmp (option, CONFIG_VERSION_OPTION) == 0))
        {
            version = config_file_parse_version (pos);
            if (version < 0)
            {
                gui_chat_printf (
                    NULL,
                    _("%sError: %s, line %d: invalid config "
                      "version: \"%s\" => "
                      "rest of file is IGNORED, default options are used"),
                    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                    filename, line_number,
                    line);
                config_file_backup (filename);
                free (option);
                free (value);
                goto end_file;
            }
            else
            {
                config_file->version_read = version;
                if (config_file->version_read > config_file->version)
                {
                    gui_chat_printf (
                        NULL,
                        _("%sError: %s, version read (%d) is newer than "
                          "supported version (%d) => "
                          "rest of file is IGNORED, default options are used"),
                        gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                        filename,
                        config_file->version_read,
                        config_file->version);
                    config_file_backup (filename);
                    free (option);
                    free (value);
                    goto end_file;
                }
            }
            goto end_line;
        }

        if (!ptr_section)
        {
            gui_chat_printf (NULL,
                             _("%sWarning: %s, line %d: "
                               "ignoring option outside section: %s"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             filename, line_number,
                             line);
            goto end_line;
        }

        config_file_update_data_read (config_file, filename,
                                      ptr_section->name, option, value,
                                      NULL, &option, &value,
                                      &warning_update_displayed);

        /* option has been ignored by the update callback? */
        if (!option || !option[0])
            goto end_line;

        if (ptr_section->callback_read)
        {
            ptr_option = NULL;
            rc = (ptr_section->callback_read)
                (ptr_section->callback_read_pointer,
                 ptr_section->callback_read_data,
                 config_file,
                 ptr_section,
                 option,
                 value);
        }
        else
        {
            rc = WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND;
            ptr_option = config_file_search_option (config_file,
                                                    ptr_section,
                                                    option);
            if (ptr_option)
            {
                rc = config_file_option_set (ptr_option, value, 1);
                ptr_option->loaded = 1;
            }
            else
            {
                if (ptr_section->callback_create_option)
                {
                    rc = (int) (ptr_section->callback_create_option) (
                        ptr_section->callback_create_option_pointer,
                        ptr_section->callback_create_option_data,
                        config_file,
                        ptr_section,
                        option,
                        value);
                }
            }
        }

        switch (rc)
        {
            case WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND:
                gui_chat_printf (
                    NULL,
                    _("%sWarning: %s, line %d: "
                      "ignoring unknown option for section \"%s\": %s"),
                    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                    filename, line_number,
                    ptr_section->name,
                    line);
                break;
            case WEECHAT_CONFIG_OPTION_SET_ERROR:
                gui_chat_printf (
                    NULL,
                    _("%sWarning: %s, line %d: "
                      "ignoring invalid value for option in section \"%s\": %s"),
                    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                    filename, line_number,
                    ptr_section->name,
                    line);
                break;
        }

    end_line:
        free (option);
        free (value);
    }

end_file:
    fclose (config_file->file);
    config_file->file = NULL;
    free (filename);

    return WEECHAT_CONFIG_READ_OK;
}

/*
 * Reads a configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_READ_OK: OK
 *   WEECHAT_CONFIG_READ_MEMORY_ERROR: not enough memory
 *   WEECHAT_CONFIG_READ_FILE_NOT_FOUND: file not found
 */

int
config_file_read (struct t_config_file *config_file)
{
    return config_file_read_internal (config_file, 0);
}

/*
 * Reloads a configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_READ_OK: OK
 *   WEECHAT_CONFIG_READ_MEMORY_ERROR: not enough memory
 *   WEECHAT_CONFIG_READ_FILE_NOT_FOUND: file not found
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
 * Frees data in an option.
 */

void
config_file_option_free_data (struct t_config_option *option)
{
    free (option->name);
    free (option->parent_name);
    free (option->description);
    string_free_split (option->string_values);
    free (option->default_value);
    free (option->value);
    free (option->callback_check_value_data);
    free (option->callback_change_data);
    free (option->callback_delete_data);
}

/*
 * Frees an option.
 */

void
config_file_option_free (struct t_config_option *option, int run_callback)
{
    struct t_config_section *ptr_section;
    struct t_config_option *new_options;
    char *option_full_name;

    if (!option)
        return;

    option_full_name = (run_callback) ?
        config_file_option_full_name (option) : NULL;

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

    if (option_full_name)
    {
        hook_config_exec (option_full_name, NULL);
        free (option_full_name);
    }
}

/*
 * Frees options in a section.
 */

void
config_file_section_free_options (struct t_config_section *section)
{
    if (!section)
        return;

    while (section->options)
    {
        config_file_option_free (section->options, 1);
    }
}

/*
 * Frees a section.
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
    free (section->name);
    free (section->callback_read_data);
    free (section->callback_write_data);
    free (section->callback_write_default_data);
    free (section->callback_create_option_data);
    free (section->callback_delete_option_data);

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
 * Frees a configuration file.
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
    free (config_file->name);
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

    /* free data */
    free (config_file->callback_update_data);
    free (config_file->callback_reload_data);

    free (config_file);

    config_files = new_config_files;
}

/*
 * Frees all configuration files.
 */

void
config_file_free_all (void)
{
    while (config_files)
    {
        config_file_free (config_files);
    }
}

/*
 * Frees all configuration files for a plugin.
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
 * Returns hdata for structure t_config_file.
 */

struct t_hdata *
config_file_hdata_config_file_cb (const void *pointer, void *data,
                                  const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_config", "next_config",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_config_file, plugin, POINTER, 0, NULL, "plugin");
        HDATA_VAR(struct t_config_file, priority, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_file, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_config_file, filename, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_config_file, file, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_file, version, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_file, callback_reload, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_file, callback_reload_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_file, callback_reload_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_file, sections, POINTER, 0, NULL, "config_section");
        HDATA_VAR(struct t_config_file, last_section, POINTER, 0, NULL, "config_section");
        HDATA_VAR(struct t_config_file, prev_config, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_config_file, next_config, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(config_files, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_config_file, 0);
    }
    return hdata;
}

/*
 * Returns hdata for structure t_config_section.
 */

struct t_hdata *
config_file_hdata_config_section_cb (const void *pointer, void *data,
                                     const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_section", "next_section",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_config_section, config_file, POINTER, 0, NULL, "config_file");
        HDATA_VAR(struct t_config_section, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, user_can_add_options, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, user_can_delete_options, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_read, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_read_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_read_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write_default, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write_default_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_write_default_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_create_option, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_create_option_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_create_option_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_delete_option, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_delete_option_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, callback_delete_option_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_section, options, POINTER, 0, NULL, "config_option");
        HDATA_VAR(struct t_config_section, last_option, POINTER, 0, NULL, "config_option");
        HDATA_VAR(struct t_config_section, prev_section, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_config_section, next_section, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Returns hdata for structure t_config_option.
 */

struct t_hdata *
config_file_hdata_config_option_cb (const void *pointer, void *data,
                                    const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_option", "next_option",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_config_option, config_file, POINTER, 0, NULL, "config_file");
        HDATA_VAR(struct t_config_option, section, POINTER, 0, NULL, "config_section");
        HDATA_VAR(struct t_config_option, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, parent_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, type, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, description, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, string_values, STRING, 0, "*,*", NULL);
        HDATA_VAR(struct t_config_option, min, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, max, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, default_value, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, value, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, null_value_allowed, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_check_value, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_check_value_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_check_value_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_change, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_change_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_change_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_delete, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_delete_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, callback_delete_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, loaded, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_config_option, prev_option, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_config_option, next_option, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Adds a configuration option in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
config_file_add_option_to_infolist (struct t_infolist *infolist,
                                    struct t_config_file *config_file,
                                    struct t_config_section *section,
                                    struct t_config_option *option,
                                    const char *option_name)
{
    char *option_full_name, *value, *string_values;
    struct t_config_option *ptr_parent_option;
    struct t_infolist_item *ptr_item;
    int rc;

    rc = 1;

    option_full_name = config_file_option_full_name (option);
    if (!option_full_name)
        goto error;

    if (option_name && option_name[0]
        && (!string_match (option_full_name, option_name, 1)))
    {
        goto end;
    }

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        goto error;

    if (!infolist_new_var_string (ptr_item, "full_name", option_full_name))
        goto error;
    if (!infolist_new_var_string (ptr_item, "config_name", config_file->name))
        goto error;
    if (!infolist_new_var_string (ptr_item, "section_name", section->name))
        goto error;
    if (!infolist_new_var_string (ptr_item, "option_name", option->name))
        goto error;
    if (!infolist_new_var_string (ptr_item, "parent_name", option->parent_name))
        goto error;
    if (!infolist_new_var_string (ptr_item, "description", option->description))
        goto error;
    if (!infolist_new_var_string (ptr_item, "description_nls",
                                  (option->description
                                   && option->description[0]) ?
                                  _(option->description) : ""))
    {
        goto error;
    }
    string_values = string_rebuild_split_string (
        (const char **)option->string_values, "|", 0, -1);
    if (!infolist_new_var_string (ptr_item, "string_values", string_values))
    {
        free (string_values);
        goto error;
    }
    free (string_values);
    if (!infolist_new_var_integer (ptr_item, "min", option->min))
        goto error;
    if (!infolist_new_var_integer (ptr_item, "max", option->max))
        goto error;
    if (!infolist_new_var_integer (ptr_item, "null_value_allowed",
                                   option->null_value_allowed))
    {
        goto error;
    }
    if (!infolist_new_var_integer (ptr_item, "value_is_null",
                                   (option->value) ? 0 : 1))
    {
        goto error;
    }
    if (!infolist_new_var_integer (ptr_item,
                                   "default_value_is_null",
                                   (option->default_value) ?
                                   0 : 1))
    {
        goto error;
    }
    if (!infolist_new_var_string (ptr_item, "type",
                                  config_option_type_string[option->type]))
    {
        goto error;
    }
    if (option->value)
    {
        value = config_file_option_value_to_string (option, 0, 0, 0);
        if (!value)
            goto error;
        if (!infolist_new_var_string (ptr_item, "value", value))
        {
            free (value);
            goto error;
        }
        free (value);
    }
    if (option->default_value)
    {
        value = config_file_option_value_to_string (option, 1, 0, 0);
        if (!value)
            goto error;
        if (!infolist_new_var_string (ptr_item, "default_value", value))
        {
            free (value);
            goto error;
        }
        free (value);
    }
    if (option->parent_name)
    {
        config_file_search_with_string (option->parent_name,
                                        NULL, NULL, &ptr_parent_option, NULL);
        if (ptr_parent_option && ptr_parent_option->value)
        {
            value = config_file_option_value_to_string (ptr_parent_option,
                                                        0, 0, 0);
            if (!value)
                goto error;
            if (!infolist_new_var_string (ptr_item, "parent_value", value))
            {
                free (value);
                goto error;
            }
            free (value);
        }
    }

    goto end;

error:
    rc = 0;

end:
    free (option_full_name);
    return rc;
}

/*
 * Adds configuration options in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
config_file_add_to_infolist (struct t_infolist *infolist,
                             const char *option_name)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

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
                if (!config_file_add_option_to_infolist (infolist,
                                                         ptr_config,
                                                         ptr_section,
                                                         ptr_option,
                                                         option_name))
                {
                    return 0;
                }
            }
        }
    }

    return 1;
}

/*
 * Prints configuration file in WeeChat log file (usually for crash dump).
 */

void
config_file_print_log (void)
{
    struct t_config_file *ptr_config_file;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    for (ptr_config_file = config_files; ptr_config_file;
         ptr_config_file = ptr_config_file->next_config)
    {
        log_printf ("");
        log_printf ("[config (addr:%p)]", ptr_config_file);
        log_printf ("  plugin . . . . . . . . : %p ('%s')",
                    ptr_config_file->plugin,
                    plugin_get_name (ptr_config_file->plugin));
        log_printf ("  priority . . . . . . . : %d", ptr_config_file->priority);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_config_file->name);
        log_printf ("  filename . . . . . . . : '%s'", ptr_config_file->filename);
        log_printf ("  file . . . . . . . . . : %p", ptr_config_file->file);
        log_printf ("  callback_reload. . . . : %p", ptr_config_file->callback_reload);
        log_printf ("  callback_reload_pointer: %p", ptr_config_file->callback_reload_pointer);
        log_printf ("  callback_reload_data . : %p", ptr_config_file->callback_reload_data);
        log_printf ("  sections . . . . . . . : %p", ptr_config_file->sections);
        log_printf ("  last_section . . . . . : %p", ptr_config_file->last_section);
        log_printf ("  prev_config. . . . . . : %p", ptr_config_file->prev_config);
        log_printf ("  next_config. . . . . . : %p", ptr_config_file->next_config);

        for (ptr_section = ptr_config_file->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            log_printf ("");
            log_printf ("    [section (addr:%p)]", ptr_section);
            log_printf ("      config_file . . . . . . . . . : %p", ptr_section->config_file);
            log_printf ("      name. . . . . . . . . . . . . : '%s'", ptr_section->name);
            log_printf ("      callback_read . . . . . . . . : %p", ptr_section->callback_read);
            log_printf ("      callback_read_pointer . . . . : %p", ptr_section->callback_read_pointer);
            log_printf ("      callback_read_data. . . . . . : %p", ptr_section->callback_read_data);
            log_printf ("      callback_write. . . . . . . . : %p", ptr_section->callback_write);
            log_printf ("      callback_write_pointer. . . . : %p", ptr_section->callback_write_pointer);
            log_printf ("      callback_write_data . . . . . : %p", ptr_section->callback_write_data);
            log_printf ("      callback_write_default. . . . : %p", ptr_section->callback_write_default);
            log_printf ("      callback_write_default_pointer: %p", ptr_section->callback_write_default_pointer);
            log_printf ("      callback_write_default_data . : %p", ptr_section->callback_write_default_data);
            log_printf ("      callback_create_option. . . . : %p", ptr_section->callback_create_option);
            log_printf ("      callback_create_option_pointer: %p", ptr_section->callback_create_option_pointer);
            log_printf ("      callback_create_option_data . : %p", ptr_section->callback_create_option_data);
            log_printf ("      callback_delete_option. . . . : %p", ptr_section->callback_delete_option);
            log_printf ("      callback_delete_option_pointer: %p", ptr_section->callback_delete_option_pointer);
            log_printf ("      callback_delete_option_data . : %p", ptr_section->callback_delete_option_data);
            log_printf ("      options . . . . . . . . . . . : %p", ptr_section->options);
            log_printf ("      last_option . . . . . . . . . : %p", ptr_section->last_option);
            log_printf ("      prev_section. . . . . . . . . : %p", ptr_section->prev_section);
            log_printf ("      next_section. . . . . . . . . : %p", ptr_section->next_section);

            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                log_printf ("");
                log_printf ("      [option (addr:%p)]", ptr_option);
                log_printf ("        config_file. . . . . . . . . : %p", ptr_option->config_file);
                log_printf ("        section. . . . . . . . . . . : %p", ptr_option->section);
                log_printf ("        name . . . . . . . . . . . . : '%s'", ptr_option->name);
                log_printf ("        parent_name. . . . . . . . . : '%s'", ptr_option->parent_name);
                log_printf ("        type . . . . . . . . . . . . : %d", ptr_option->type);
                log_printf ("        description. . . . . . . . . : '%s'", ptr_option->description);
                log_printf ("        string_values. . . . . . . . : %p", ptr_option->string_values);
                log_printf ("        min. . . . . . . . . . . . . : %d", ptr_option->min);
                log_printf ("        max. . . . . . . . . . . . . : %d", ptr_option->max);
                switch (ptr_option->type)
                {
                    case CONFIG_OPTION_TYPE_BOOLEAN:
                        log_printf ("        default value. . . . . . . . : %s",
                                    (ptr_option->default_value) ?
                                    ((CONFIG_BOOLEAN_DEFAULT(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                     "on" : "off") : "null");
                        log_printf ("        value (boolean). . . . . . . : %s",
                                    (ptr_option->value) ?
                                    ((CONFIG_BOOLEAN(ptr_option) == CONFIG_BOOLEAN_TRUE) ?
                                     "on" : "off") : "null");
                        break;
                    case CONFIG_OPTION_TYPE_INTEGER:
                        if (ptr_option->default_value)
                            log_printf ("        default value. . . . . . . . : %d",
                                        CONFIG_INTEGER_DEFAULT(ptr_option));
                        else
                            log_printf ("        default value. . . . . . . . : null");
                        if (ptr_option->value)
                            log_printf ("        value (integer). . . . . . . : %d",
                                        CONFIG_INTEGER(ptr_option));
                        else
                            log_printf ("        value (integer). . . . . . . : null");
                        break;
                    case CONFIG_OPTION_TYPE_STRING:
                        if (ptr_option->default_value)
                            log_printf ("        default value. . . . . . . . : '%s'",
                                        CONFIG_STRING_DEFAULT(ptr_option));
                        else
                            log_printf ("        default value. . . . . . . . : null");
                        if (ptr_option->value)
                            log_printf ("        value (string) . . . . . . . : '%s'",
                                        CONFIG_STRING(ptr_option));
                        else
                            log_printf ("        value (string) . . . . . . . : null");
                        break;
                    case CONFIG_OPTION_TYPE_COLOR:
                        if (ptr_option->default_value)
                            log_printf ("        default value. . . . . . . . : %d ('%s')",
                                        CONFIG_COLOR_DEFAULT(ptr_option),
                                        gui_color_get_name (CONFIG_COLOR_DEFAULT(ptr_option)));
                        else
                            log_printf ("        default value. . . . . . . . : null");
                        if (ptr_option->value)
                            log_printf ("        value (color). . . . . . . . : %d ('%s')",
                                        CONFIG_COLOR(ptr_option),
                                        gui_color_get_name (CONFIG_COLOR(ptr_option)));
                        else
                            log_printf ("        value (color). . . . . . . . : null");
                        break;
                    case CONFIG_OPTION_TYPE_ENUM:
                        log_printf ("        default value. . . . . . . . : '%s'",
                                    (ptr_option->default_value) ?
                                    ptr_option->string_values[CONFIG_ENUM_DEFAULT(ptr_option)] : "null");
                        log_printf ("        value (integer/str). . . . . : '%s'",
                                    (ptr_option->value) ?
                                    ptr_option->string_values[CONFIG_ENUM(ptr_option)] : "null");
                        break;
                    case CONFIG_NUM_OPTION_TYPES:
                        break;
                }
                log_printf ("        null_value_allowed . . . . . : %d", ptr_option->null_value_allowed);
                log_printf ("        callback_check_value . . . . : %p", ptr_option->callback_check_value);
                log_printf ("        callback_check_value_pointer : %p", ptr_option->callback_check_value_pointer);
                log_printf ("        callback_check_value_data. . : %p", ptr_option->callback_check_value_data);
                log_printf ("        callback_change. . . . . . . : %p", ptr_option->callback_change);
                log_printf ("        callback_change_pointer. . . : %p", ptr_option->callback_change_pointer);
                log_printf ("        callback_change_data . . . . : %p", ptr_option->callback_change_data);
                log_printf ("        callback_delete. . . . . . . : %p", ptr_option->callback_delete);
                log_printf ("        callback_delete_pointer. . . : %p", ptr_option->callback_delete_pointer);
                log_printf ("        callback_delete_data . . . . : %p", ptr_option->callback_delete_data);
                log_printf ("        loaded . . . . . . . . . . . : %d", ptr_option->loaded);
                log_printf ("        prev_option. . . . . . . . . : %p", ptr_option->prev_option);
                log_printf ("        next_option. . . . . . . . . : %p", ptr_option->next_option);
            }
        }
    }
}
