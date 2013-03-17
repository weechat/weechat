/*
 * wee-completion.c - completion for WeeChat commands
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>

#include "weechat.h"
#include "wee-config.h"
#include "wee-hashtable.h"
#include "wee-hook.h"
#include "wee-list.h"
#include "wee-proxy.h"
#include "wee-string.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-window.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-color.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-key.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


/*
 * Adds bar names to completion list.
 */

int
completion_list_add_bars_names_cb (void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        gui_completion_list_add (completion, ptr_bar->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds bar options to completion list.
 */

int
completion_list_add_bars_options_cb (void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        gui_completion_list_add (completion, gui_bar_option_string[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds buffer names to completion list.
 */

int
completion_list_add_buffers_names_cb (void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_completion_list_add (completion, ptr_buffer->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds buffer numbers to completion list.
 */

int
completion_list_add_buffers_numbers_cb (void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;
    char str_number[32];

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        snprintf (str_number, sizeof (str_number), "%d", ptr_buffer->number);
        gui_completion_list_add (completion, str_number,
                                 0, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds plugin+buffer names to completion list.
 */

int
completion_list_add_buffers_plugins_names_cb (void *data,
                                              const char *completion_item,
                                              struct t_gui_buffer *buffer,
                                              struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_completion_list_add (completion, ptr_buffer->full_name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds buffer properties (that can be set) to completion list.
 */

int
completion_list_add_buffer_properties_set_cb (void *data,
                                              const char *completion_item,
                                              struct t_gui_buffer *buffer,
                                              struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; gui_buffer_properties_set[i]; i++)
    {
        gui_completion_list_add (completion,
                                 gui_buffer_properties_set[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds buffer properties (that can be read) to completion list.
 */

int
completion_list_add_buffer_properties_get_cb (void *data,
                                              const char *completion_item,
                                              struct t_gui_buffer *buffer,
                                              struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; gui_buffer_properties_get_integer[i]; i++)
    {
        gui_completion_list_add (completion,
                                 gui_buffer_properties_get_integer[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }
    for (i = 0; gui_buffer_properties_get_string[i]; i++)
    {
        gui_completion_list_add (completion,
                                 gui_buffer_properties_get_string[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }
    for (i = 0; gui_buffer_properties_get_pointer[i]; i++)
    {
        gui_completion_list_add (completion,
                                 gui_buffer_properties_get_pointer[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds window numbers to completion list.
 */

int
completion_list_add_windows_numbers_cb (void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    struct t_gui_window *ptr_win;
    char str_number[32];

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        snprintf (str_number, sizeof (str_number), "%d", ptr_win->number);
        gui_completion_list_add (completion, str_number,
                                 0, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds a palette color to completion list.
 */

void
completion_list_map_add_palette_color_cb (void *data,
                                          struct t_hashtable *hashtable,
                                          const void *key, const void *value)
{
    /* make C compiler happy */
    (void) hashtable;
    (void) value;

    gui_completion_list_add ((struct t_gui_completion *)data,
                             (const char *)key,
                             0, WEECHAT_LIST_POS_SORT);
}

/*
 * Adds palette colors to completion list.
 */

int
completion_list_add_palette_colors_cb (void *data,
                                       const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    hashtable_map (gui_color_hash_palette_color,
                   &completion_list_map_add_palette_color_cb,
                   completion);

    return WEECHAT_RC_OK;
}

/*
 * Adds configuration files to completion list.
 */

int
completion_list_add_config_files_cb (void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_config_file *ptr_config_file;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_config_file = config_files; ptr_config_file;
         ptr_config_file = ptr_config_file->next_config)
    {
        gui_completion_list_add (completion, ptr_config_file->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds path/filename to completion list.
 */

int
completion_list_add_filename_cb (void *data,
                                 const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    char *path_d, *path_b, *p, *d_name;
    char *real_prefix, *prefix;
    char *buf;
    int buf_len;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char home[3] = { '~', DIR_SEPARATOR_CHAR, '\0' };

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    buf_len = PATH_MAX;
    buf = malloc (buf_len);
    if (!buf)
        return WEECHAT_RC_OK;

    completion->add_space = 0;

    if ((strncmp (completion->base_word, home, 2) == 0) && getenv("HOME"))
    {
        real_prefix = strdup (getenv("HOME"));
        prefix = strdup (home);
    }
    else
    {
        if ((strncmp (completion->base_word, DIR_SEPARATOR, 1) != 0)
            || (strcmp (completion->base_word, "") == 0))
        {
            real_prefix = strdup (weechat_home);
            prefix = strdup ("");
        }
        else
        {
            real_prefix = strdup (DIR_SEPARATOR);
            prefix = strdup (DIR_SEPARATOR);
        }
    }

    snprintf (buf, buf_len, "%s", completion->base_word + strlen (prefix));
    p = strrchr (buf, DIR_SEPARATOR_CHAR);
    if (p)
    {
        p[0] = '\0';
        path_d = strdup (buf);
        p++;
        path_b = strdup (p);
    }
    else
    {
        path_d = strdup ("");
        path_b = strdup (buf);
    }

    sprintf (buf, "%s%s%s", real_prefix, DIR_SEPARATOR, path_d);
    d_name = strdup (buf);
    dp = opendir (d_name);
    if (dp != NULL)
    {
        while ((entry = readdir (dp)) != NULL)
        {
            if (strncmp (entry->d_name, path_b, strlen (path_b)) == 0)
            {
                if (strcmp (entry->d_name, ".") == 0 || strcmp (entry->d_name, "..") == 0)
                    continue;

                snprintf (buf, buf_len, "%s%s%s",
                          d_name, DIR_SEPARATOR, entry->d_name);
                if (stat (buf, &statbuf) == -1)
                    continue;

                snprintf (buf, buf_len, "%s%s%s%s%s%s",
                          prefix,
                          ((strcmp(prefix, "") == 0)
                           || strchr(prefix, DIR_SEPARATOR_CHAR)) ? "" : DIR_SEPARATOR,
                          path_d,
                          strcmp(path_d, "") == 0 ? "" : DIR_SEPARATOR,
                          entry->d_name,
                          S_ISDIR(statbuf.st_mode) ? DIR_SEPARATOR : "");

                gui_completion_list_add (completion, buf,
                                         0, WEECHAT_LIST_POS_SORT);
            }
        }
        closedir (dp);
    }

    free (d_name);
    free (prefix);
    free (real_prefix);
    free (path_d);
    free (path_b);
    free (buf);

    return WEECHAT_RC_OK;
}

/*
 * Adds filter names to completion list.
 */

int
completion_list_add_filters_cb (void *data,
                                const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    struct t_gui_filter *ptr_filter;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        gui_completion_list_add (completion, ptr_filter->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds command hooks to completion list.
 */

int
completion_list_add_commands_cb (void *data,
                                 const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (HOOK_COMMAND(ptr_hook, command))
            && (HOOK_COMMAND(ptr_hook, command)[0]))
            gui_completion_list_add (completion,
                                     HOOK_COMMAND(ptr_hook, command),
                                     0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds info hooks to completion list.
 */

int
completion_list_add_infos_cb (void *data,
                              const char *completion_item,
                              struct t_gui_buffer *buffer,
                              struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_INFO]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (HOOK_INFO(ptr_hook, info_name))
            && (HOOK_INFO(ptr_hook, info_name)[0]))
            gui_completion_list_add (completion,
                                     HOOK_INFO(ptr_hook, info_name),
                                     0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds infolist hooks to completion list.
 */

int
completion_list_add_infolists_cb (void *data,
                                  const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_INFOLIST]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (HOOK_INFOLIST(ptr_hook, infolist_name))
            && (HOOK_INFOLIST(ptr_hook, infolist_name)[0]))
            gui_completion_list_add (completion,
                                     HOOK_INFOLIST(ptr_hook, infolist_name),
                                     0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds nicks to completion list.
 */

int
completion_list_add_nicks_cb (void *data,
                              const char *completion_item,
                              struct t_gui_buffer *buffer,
                              struct t_gui_completion *completion)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    int count_before;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    count_before = weelist_size (completion->completion_list);
    hook_completion_exec (completion->buffer->plugin,
                          "nick",
                          completion->buffer,
                          completion);
    if (weelist_size (completion->completion_list) == count_before)
    {
        /*
         * no plugin overrides nick completion => use default nick
         * completion, with nicks of nicklist, in order of nicklist
         */
        ptr_group = NULL;
        ptr_nick = NULL;
        gui_nicklist_get_next_item (completion->buffer,
                                    &ptr_group, &ptr_nick);
        while (ptr_group || ptr_nick)
        {
            if (ptr_nick && ptr_nick->visible)
            {
                gui_completion_list_add (completion,
                                         ptr_nick->name,
                                         1, WEECHAT_LIST_POS_END);
            }
            gui_nicklist_get_next_item (completion->buffer,
                                        &ptr_group, &ptr_nick);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds configuration options to completion list.
 */

int
completion_list_add_config_options_cb (void *data,
                                       const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    int length;
    char *option_full_name;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        for (ptr_section = ptr_config->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                length = strlen (ptr_config->name) + 1
                    + strlen (ptr_section->name) + 1
                    + strlen (ptr_option->name) + 1;
                option_full_name = malloc (length);
                if (option_full_name)
                {
                    snprintf (option_full_name, length, "%s.%s.%s",
                              ptr_config->name, ptr_section->name,
                              ptr_option->name);
                    gui_completion_list_add (completion,
                                             option_full_name,
                                             0, WEECHAT_LIST_POS_SORT);
                    free (option_full_name);
                }
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds plugin names to completion list.
 */

int
completion_list_add_plugins_cb (void *data,
                                const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    struct t_weechat_plugin *ptr_plugin;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        gui_completion_list_add (completion, ptr_plugin->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds plugin commands to completion list.
 *
 * The plugin name is read in previous argument.
 */

int
completion_list_add_plugins_commands_cb (void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion)
{
    char *pos_space, *plugin_name;
    struct t_weechat_plugin *ptr_plugin;
    struct t_hook *ptr_hook;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    if (completion->args)
    {
        pos_space = strchr (completion->args, ' ');
        if (pos_space)
            plugin_name = string_strndup (completion->args,
                                          pos_space - completion->args);
        else
            plugin_name = strdup (completion->args);

        if (plugin_name)
        {
            ptr_plugin = NULL;
            if (string_strcasecmp (plugin_name, PLUGIN_CORE) != 0)
            {
                /*
                 * plugin name is different from "core", then search it in
                 * plugin list
                 */
                ptr_plugin = plugin_search (plugin_name);
                if (!ptr_plugin)
                    return WEECHAT_RC_OK;
            }
            for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
                 ptr_hook = ptr_hook->next_hook)
            {
                if (!ptr_hook->deleted
                    && (ptr_hook->plugin == ptr_plugin)
                    && HOOK_COMMAND(ptr_hook, command)
                    && HOOK_COMMAND(ptr_hook, command)[0])
                {
                    gui_completion_list_add (completion,
                                             HOOK_COMMAND(ptr_hook, command),
                                             0, WEECHAT_LIST_POS_SORT);
                }
            }
            free (plugin_name);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds value of option to completion list.
 *
 * The option name is read in previous argument.
 */

int
completion_list_add_config_option_values_cb (void *data,
                                             const char *completion_item,
                                             struct t_gui_buffer *buffer,
                                             struct t_gui_completion *completion)
{
    char *pos_space, *option_full_name, *pos_section, *pos_option;
    char *file, *section, *value_string, str_number[64];
    const char *color_name;
    int length, i, num_colors;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section, *section_found;
    struct t_config_option *option_found;
    struct t_gui_color_palette *color_palette;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    if (completion->args)
    {
        pos_space = strchr (completion->args, ' ');
        if (pos_space)
            option_full_name = string_strndup (completion->args,
                                               pos_space - completion->args);
        else
            option_full_name = strdup (completion->args);

        if (option_full_name)
        {
            file = NULL;
            section = NULL;
            pos_option = NULL;

            pos_section = strchr (option_full_name, '.');
            pos_option = (pos_section) ? strchr (pos_section + 1, '.') : NULL;

            if (pos_section && pos_option)
            {
                file = string_strndup (option_full_name,
                                       pos_section - option_full_name);
                section = string_strndup (pos_section + 1,
                                          pos_option - pos_section - 1);
                pos_option++;
            }
            if (file && section && pos_option)
            {
                ptr_config = config_file_search (file);
                if (ptr_config)
                {
                    ptr_section = config_file_search_section (ptr_config,
                                                              section);
                    if (ptr_section)
                    {
                        config_file_search_section_option (ptr_config,
                                                           ptr_section,
                                                           pos_option,
                                                           &section_found,
                                                           &option_found);
                        if (option_found)
                        {
                            switch (option_found->type)
                            {
                                case CONFIG_OPTION_TYPE_BOOLEAN:
                                    gui_completion_list_add (completion, "on",
                                                             0, WEECHAT_LIST_POS_SORT);
                                    gui_completion_list_add (completion, "off",
                                                             0, WEECHAT_LIST_POS_SORT);
                                    gui_completion_list_add (completion, "toggle",
                                                             0, WEECHAT_LIST_POS_END);
                                    if (option_found->value)
                                    {
                                        if (CONFIG_BOOLEAN(option_found) == CONFIG_BOOLEAN_TRUE)
                                            gui_completion_list_add (completion, "on",
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        else
                                            gui_completion_list_add (completion, "off",
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                    }
                                    else
                                    {
                                        gui_completion_list_add (completion,
                                                                 WEECHAT_CONFIG_OPTION_NULL,
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                    }
                                    break;
                                case CONFIG_OPTION_TYPE_INTEGER:
                                    if (option_found->string_values)
                                    {
                                        for (i = 0; option_found->string_values[i]; i++)
                                        {
                                            gui_completion_list_add (completion,
                                                                     option_found->string_values[i],
                                                                     0, WEECHAT_LIST_POS_SORT);
                                        }
                                        gui_completion_list_add (completion, "++1",
                                                                 0, WEECHAT_LIST_POS_END);
                                        gui_completion_list_add (completion, "--1",
                                                                 0, WEECHAT_LIST_POS_END);
                                        if (option_found->value)
                                        {
                                            gui_completion_list_add (completion,
                                                                     option_found->string_values[CONFIG_INTEGER(option_found)],
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        }
                                        else
                                        {
                                            gui_completion_list_add (completion,
                                                                     WEECHAT_CONFIG_OPTION_NULL,
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        }
                                    }
                                    else
                                    {
                                        if (option_found->value && CONFIG_INTEGER(option_found) > option_found->min)
                                            gui_completion_list_add (completion, "--1",
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        if (option_found->value && CONFIG_INTEGER(option_found) < option_found->max)
                                            gui_completion_list_add (completion, "++1",
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        if (option_found->value)
                                        {
                                            length = 64;
                                            value_string = malloc (length);
                                            if (value_string)
                                            {
                                                snprintf (value_string, length,
                                                          "%d", CONFIG_INTEGER(option_found));
                                                gui_completion_list_add (completion,
                                                                         value_string,
                                                                         0, WEECHAT_LIST_POS_BEGINNING);
                                                free (value_string);
                                            }
                                        }
                                        else
                                        {
                                            gui_completion_list_add (completion,
                                                                     WEECHAT_CONFIG_OPTION_NULL,
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        }
                                    }
                                    break;
                                case CONFIG_OPTION_TYPE_STRING:
                                    gui_completion_list_add (completion,
                                                             "\"\"",
                                                             0, WEECHAT_LIST_POS_BEGINNING);
                                    if (option_found->value)
                                    {
                                        length = strlen (CONFIG_STRING(option_found)) + 2 + 1;
                                        value_string = malloc (length);
                                        if (value_string)
                                        {
                                            snprintf (value_string, length,
                                                      "\"%s\"",
                                                      CONFIG_STRING(option_found));
                                            gui_completion_list_add (completion,
                                                                     value_string,
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                            free (value_string);
                                        }
                                    }
                                    else
                                    {
                                        gui_completion_list_add (completion,
                                                                 WEECHAT_CONFIG_OPTION_NULL,
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                    }
                                    break;
                                case CONFIG_OPTION_TYPE_COLOR:
                                    num_colors = gui_color_get_weechat_colors_number ();
                                    for (i = 0; i < num_colors; i++)
                                    {
                                        color_name = gui_color_get_name (i);
                                        if (color_name)
                                        {
                                            gui_completion_list_add (completion,
                                                                     color_name,
                                                                     0, WEECHAT_LIST_POS_SORT);
                                        }
                                    }
                                    num_colors = gui_color_get_term_colors ();
                                    for (i = 0; i <= num_colors; i++)
                                    {
                                        color_palette = gui_color_palette_get (i);
                                        if (color_palette)
                                        {
                                            gui_completion_list_add (completion,
                                                                     color_palette->alias,
                                                                     0, WEECHAT_LIST_POS_END);
                                        }
                                        else
                                        {
                                            snprintf (str_number,
                                                      sizeof (str_number),
                                                      "%d",
                                                      i);
                                            gui_completion_list_add (completion,
                                                                     str_number,
                                                                     0, WEECHAT_LIST_POS_END);
                                        }
                                    }
                                    gui_completion_list_add (completion, "++1",
                                                             0, WEECHAT_LIST_POS_END);
                                    gui_completion_list_add (completion, "--1",
                                                             0, WEECHAT_LIST_POS_END);
                                    if (option_found->value)
                                    {
                                        color_name = gui_color_get_name (CONFIG_INTEGER(option_found));
                                        if (color_name)
                                        {
                                            gui_completion_list_add (completion,
                                                                     color_name,
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        }
                                    }
                                    else
                                    {
                                        gui_completion_list_add (completion,
                                                                 WEECHAT_CONFIG_OPTION_NULL,
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                    }
                                    break;
                                case CONFIG_NUM_OPTION_TYPES:
                                    break;
                            }
                            if (option_found->value
                                && option_found->null_value_allowed)
                            {
                                gui_completion_list_add (completion,
                                                         WEECHAT_CONFIG_OPTION_NULL,
                                                         0,
                                                         WEECHAT_LIST_POS_END);
                            }
                        }
                    }
                }
            }
            if (file)
                free (file);
            if (section)
                free (section);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds WeeChat commands to completion list.
 */

int
completion_list_add_weechat_commands_cb (void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && !ptr_hook->plugin
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0])
        {
            gui_completion_list_add (completion,
                                     HOOK_COMMAND(ptr_hook, command),
                                     0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds proxy names to completion list.
 */

int
completion_list_add_proxies_names_cb (void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    struct t_proxy *ptr_proxy;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_proxy = weechat_proxies; ptr_proxy;
         ptr_proxy = ptr_proxy->next_proxy)
    {
        gui_completion_list_add (completion, ptr_proxy->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds proxy options to completion list.
 */

int
completion_list_add_proxies_options_cb (void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < PROXY_NUM_OPTIONS; i++)
    {
        gui_completion_list_add (completion, proxy_option_string[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds key contexts to completion list.
 */

int
completion_list_add_keys_contexts_cb (void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        gui_completion_list_add (completion, gui_key_context_string[i],
                                 0, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds keys to completion list.
 */

int
completion_list_add_keys_codes_cb (void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    int i;
    struct t_gui_key *ptr_key;
    char *expanded_name;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        for (ptr_key = gui_keys[i]; ptr_key; ptr_key = ptr_key->next_key)
        {
            expanded_name = gui_key_get_expanded_name (ptr_key->key);
            gui_completion_list_add (completion,
                                     (expanded_name) ? expanded_name : ptr_key->key,
                                     0, WEECHAT_LIST_POS_SORT);
            if (expanded_name)
                free (expanded_name);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds keys that can be reset (keys added, redefined or removed) to completion
 * list.
 */

int
completion_list_add_keys_codes_for_reset_cb (void *data,
                                             const char *completion_item,
                                             struct t_gui_buffer *buffer,
                                             struct t_gui_completion *completion)
{
    int i;
    struct t_gui_key *ptr_key, *ptr_default_key;
    char *expanded_name;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < GUI_KEY_NUM_CONTEXTS; i++)
    {
        /* keys added or redefined */
        for (ptr_key = gui_keys[i]; ptr_key; ptr_key = ptr_key->next_key)
        {
            ptr_default_key = gui_key_search (gui_default_keys[i], ptr_key->key);
            if (!ptr_default_key
                || (strcmp (ptr_default_key->command, ptr_key->command) != 0))
            {
                expanded_name = gui_key_get_expanded_name (ptr_key->key);
                gui_completion_list_add (completion,
                                         (expanded_name) ? expanded_name : ptr_key->key,
                                         0, WEECHAT_LIST_POS_SORT);
                if (expanded_name)
                    free (expanded_name);
            }
        }

        /* keys deleted */
        for (ptr_default_key = gui_default_keys[i]; ptr_default_key;
             ptr_default_key = ptr_default_key->next_key)
        {
            ptr_key = gui_key_search (gui_keys[i], ptr_default_key->key);
            if (!ptr_key)
            {
                expanded_name = gui_key_get_expanded_name (ptr_default_key->key);
                gui_completion_list_add (completion,
                                         (expanded_name) ? expanded_name : ptr_default_key->key,
                                         0, WEECHAT_LIST_POS_SORT);
                if (expanded_name)
                    free (expanded_name);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds areas for free cursor movement ("chat" and bar names) to completion
 * list.
 */

int
completion_list_add_cursor_areas_cb (void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_gui_bar_window *ptr_bar_win;
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    /* add "chat" for chat area */
    gui_completion_list_add (completion, "chat", 0, WEECHAT_LIST_POS_SORT);

    /* add bar windows (of current window) */
    for (ptr_bar_win = gui_current_window->bar_windows; ptr_bar_win;
         ptr_bar_win = ptr_bar_win->next_bar_window)
    {
        gui_completion_list_add (completion, ptr_bar_win->bar->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (ptr_bar->bar_window)
        {
            gui_completion_list_add (completion, ptr_bar->name,
                                     0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds layout names to completion list.
 */

int
completion_list_add_layouts_names_cb (void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    struct t_gui_layout *ptr_layout;

    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_layout = gui_layouts; ptr_layout;
         ptr_layout = ptr_layout->next_layout)
    {
        gui_completion_list_add (completion, ptr_layout->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds hooks for completions done by WeeChat core.
 */

void
completion_init ()
{
    hook_completion (NULL, "buffers_names", /* formerly "%b" */
                     N_("names of buffers"),
                     &completion_list_add_buffers_names_cb, NULL);
    hook_completion (NULL, "buffers_numbers",
                     N_("numbers of buffers"),
                     &completion_list_add_buffers_numbers_cb, NULL);
    hook_completion (NULL, "buffers_plugins_names", /* formerly "%B" */
                     N_("names of buffers (including plugins names)"),
                     &completion_list_add_buffers_plugins_names_cb, NULL);
    hook_completion (NULL, "buffer_properties_set",
                     N_("properties that can be set on a buffer"),
                     &completion_list_add_buffer_properties_set_cb, NULL);
    hook_completion (NULL, "buffer_properties_get",
                     N_("properties that can be read on a buffer"),
                     &completion_list_add_buffer_properties_get_cb, NULL);
    hook_completion (NULL, "windows_numbers",
                     N_("numbers of windows"),
                     &completion_list_add_windows_numbers_cb, NULL);
    hook_completion (NULL, "palette_colors",
                     N_("palette colors"),
                     &completion_list_add_palette_colors_cb, NULL);
    hook_completion (NULL, "config_files", /* formerly "%c" */
                     N_("configuration files"),
                     &completion_list_add_config_files_cb, NULL);
    hook_completion (NULL, "filename", /* formerly "%f" */
                     N_("filename"),
                     &completion_list_add_filename_cb, NULL);
    hook_completion (NULL, "filters_names", /* formerly "%F" */
                     N_("names of filters"),
                     &completion_list_add_filters_cb, NULL);
    hook_completion (NULL, "commands", /* formerly "%h" */
                     N_("commands (weechat and plugins)"),
                     &completion_list_add_commands_cb, NULL);
    hook_completion (NULL, "infos", /* formerly "%i" */
                     N_("names of infos hooked"),
                     &completion_list_add_infos_cb, NULL);
    hook_completion (NULL, "infolists", /* formerly "%I" */
                     N_("names of infolists hooked"),
                     &completion_list_add_infolists_cb, NULL);
    hook_completion (NULL, "nicks", /* formerly "%n" */
                     N_("nicks in nicklist of current buffer"),
                     &completion_list_add_nicks_cb, NULL);
    hook_completion (NULL, "config_options", /* formerly "%o" */
                     N_("configuration options"),
                     &completion_list_add_config_options_cb, NULL);
    hook_completion (NULL, "plugins_names", /* formerly "%p" */
                     N_("names of plugins"),
                     &completion_list_add_plugins_cb, NULL);
    hook_completion (NULL, "plugins_commands", /* formerly "%P" */
                     N_("commands defined by plugins"),
                     &completion_list_add_plugins_commands_cb, NULL);
    hook_completion (NULL, "bars_names", /* formerly "%r" */
                     N_("names of bars"),
                     &completion_list_add_bars_names_cb, NULL);
    hook_completion (NULL, "config_option_values", /* formerly "%v" */
                     N_("values for a configuration option"),
                     &completion_list_add_config_option_values_cb, NULL);
    hook_completion (NULL, "weechat_commands", /* formerly "%w" */
                     N_("weechat commands"),
                     &completion_list_add_weechat_commands_cb, NULL);
    hook_completion (NULL, "proxies_names", /* formerly "%y" */
                     N_("names of proxies"),
                     &completion_list_add_proxies_names_cb, NULL);
    hook_completion (NULL, "proxies_options",
                     N_("options for proxies"),
                     &completion_list_add_proxies_options_cb, NULL);
    hook_completion (NULL, "bars_options",
                     N_("options for bars"),
                     &completion_list_add_bars_options_cb, NULL);
    hook_completion (NULL, "keys_contexts",
                     /* TRANSLATORS: "key" means "key on the keyboard" */
                     N_("key contexts"),
                     &completion_list_add_keys_contexts_cb, NULL);
    hook_completion (NULL, "keys_codes",
                     /* TRANSLATORS: "key" means "key on the keyboard" */
                     N_("key codes"),
                     &completion_list_add_keys_codes_cb, NULL);
    hook_completion (NULL, "keys_codes_for_reset",
                     /* TRANSLATORS: "key" means "key on the keyboard" */
                     N_("key codes that can be reset (keys added, redefined "
                        "or removed)"),
                     &completion_list_add_keys_codes_for_reset_cb, NULL);
    hook_completion (NULL, "cursor_areas",
                     N_("areas (\"chat\" or bar name) for free cursor movement"),
                     &completion_list_add_cursor_areas_cb, NULL);
    hook_completion (NULL, "layouts_names",
                     N_("names of layouts"),
                     &completion_list_add_layouts_names_cb, NULL);
}
