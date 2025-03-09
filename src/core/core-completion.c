/*
 * core-completion.c - completion for WeeChat commands
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
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
#include "core-arraylist.h"
#include "core-config.h"
#include "core-dir.h"
#include "core-eval.h"
#include "core-hashtable.h"
#include "core-hook.h"
#include "core-list.h"
#include "core-proxy.h"
#include "core-secure.h"
#include "core-string.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-bar-item-custom.h"
#include "../gui/gui-bar-window.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-color.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-key.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


extern char **environ;


/*
 * Adds a word with quotes around to completion list.
 */

void
completion_list_add_quoted_word (struct t_gui_completion *completion,
                                 const char *word)
{
    char *temp;

    if (string_asprintf (&temp, "\"%s\"", word) >= 0)
    {
        gui_completion_list_add (completion, temp, 0, WEECHAT_LIST_POS_END);
        free (temp);
    }
}

/*
 * Adds bar names to completion list.
 */

int
completion_list_add_bars_names_cb (const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) pointer;
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
 * Adds bar items to completion list.
 */

int
completion_list_add_bars_items_cb (const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    struct t_gui_bar_item *ptr_bar_item;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_bar_item = gui_bar_items; ptr_bar_item;
         ptr_bar_item = ptr_bar_item->next_item)
    {
        gui_completion_list_add (completion, ptr_bar_item->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds custom bar items names to completion list.
 */

int
completion_list_add_custom_bar_items_names_cb (const void *pointer, void *data,
                                               const char *completion_item,
                                               struct t_gui_buffer *buffer,
                                               struct t_gui_completion *completion)
{
    struct t_gui_bar_item_custom *ptr_item;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_item = gui_custom_bar_items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        gui_completion_list_add (completion, ptr_item->bar_item->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds custom bar item conditions to completion list.
 */

int
completion_list_add_custom_bar_item_conditions_cb (const void *pointer, void *data,
                                                   const char *completion_item,
                                                   struct t_gui_buffer *buffer,
                                                   struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    gui_completion_list_add (completion,
                             "\"" GUI_BAR_ITEM_CUSTOM_DEFAULT_CONDITIONS "\"",
                             0,
                             WEECHAT_LIST_POS_END);
    gui_completion_list_add (completion, "\"\"", 0, WEECHAT_LIST_POS_END);

    return WEECHAT_RC_OK;
}

/*
 * Adds custom bar item contents to completion list.
 */

int
completion_list_add_custom_bar_item_contents_cb (const void *pointer, void *data,
                                                 const char *completion_item,
                                                 struct t_gui_buffer *buffer,
                                                 struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    gui_completion_list_add (completion,
                             "\"" GUI_BAR_ITEM_CUSTOM_DEFAULT_CONTENTS "\"",
                             0,
                             WEECHAT_LIST_POS_END);
    gui_completion_list_add (completion, "\"\"", 0, WEECHAT_LIST_POS_END);

    return WEECHAT_RC_OK;
}

/*
 * Adds arguments for commands that add a custom bar item.
 */

int
completion_list_add_custom_bar_item_add_arguments_cb (const void *pointer, void *data,
                                                      const char *completion_item,
                                                      struct t_gui_buffer *buffer,
                                                      struct t_gui_completion *completion)
{
    char **sargv;
    int sargc, arg_complete;
    struct t_gui_bar_item_custom *ptr_item;

    if (!completion->args)
        return WEECHAT_RC_OK;

    sargv = string_split_shell (completion->args, &sargc);
    if (!sargv)
        return WEECHAT_RC_OK;

    ptr_item = (sargc > 1) ? gui_bar_item_custom_search (sargv[1]) : NULL;

    arg_complete = sargc;
    if (completion->base_word && completion->base_word[0])
        arg_complete--;

    switch (arg_complete)
    {
        case 1:
            completion_list_add_custom_bar_items_names_cb (pointer, data,
                                                           completion_item,
                                                           buffer, completion);

            break;
        case 2:
            if (ptr_item)
            {
                completion_list_add_quoted_word (
                    completion,
                    CONFIG_STRING(ptr_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONDITIONS]));
            }
            else
            {
                completion_list_add_custom_bar_item_conditions_cb (
                    pointer, data, completion_item, buffer, completion);
            }
            break;
        case 3:
            if (ptr_item)
            {
                completion_list_add_quoted_word (
                    completion,
                    CONFIG_STRING(ptr_item->options[GUI_BAR_ITEM_CUSTOM_OPTION_CONTENT]));
            }
            else
            {
                completion_list_add_custom_bar_item_contents_cb (
                    pointer, data, completion_item, buffer, completion);
            }
            break;
    }

    string_free_split (sargv);

    return WEECHAT_RC_OK;
}

/*
 * Adds bar options to completion list.
 */

int
completion_list_add_bars_options_cb (const void *pointer, void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_buffers_names_cb (const void *pointer, void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_buffers_numbers_cb (const void *pointer, void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;
    char str_number[32];

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_buffers_plugins_names_cb (const void *pointer, void *data,
                                              const char *completion_item,
                                              struct t_gui_buffer *buffer,
                                              struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) pointer;
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
 * Adds a buffer local variable to completions list.
 */

void
completion_list_map_buffer_local_variable_cb (void *data,
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
 * Adds buffer local variables to completion list.
 */

int
completion_list_add_buffer_local_variables_cb (const void *pointer, void *data,
                                               const char *completion_item,
                                               struct t_gui_buffer *buffer,
                                               struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    hashtable_map (completion->buffer->local_variables,
                   &completion_list_map_buffer_local_variable_cb,
                   completion);

    return WEECHAT_RC_OK;
}

/*
 * Adds buffer local variable value to completion list.
 */

int
completion_list_add_buffer_local_variable_value_cb (const void *pointer, void *data,
                                                    const char *completion_item,
                                                    struct t_gui_buffer *buffer,
                                                    struct t_gui_completion *completion)
{
    char **argv;
    int argc, arg_index;
    const char *ptr_value;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    if (!completion->args)
        return WEECHAT_RC_OK;

    argv = string_split (completion->args, " ", NULL,
                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                         0, &argc);
    if (!argv)
        return WEECHAT_RC_OK;

    if (argc > 0)
    {
        arg_index = completion->base_command_arg_index - 2;
        if ((arg_index < 1) || (arg_index > argc - 1))
            arg_index = argc - 1;
        ptr_value = hashtable_get (completion->buffer->local_variables,
                                   argv[arg_index]);
        if (ptr_value)
        {
            gui_completion_list_add (completion,
                                     ptr_value,
                                     0, WEECHAT_LIST_POS_SORT);
        }
    }
    string_free_split (argv);

    return WEECHAT_RC_OK;
}

/*
 * Adds buffer properties (that can be set) to completion list.
 */

int
completion_list_add_buffer_properties_set_cb (const void *pointer, void *data,
                                              const char *completion_item,
                                              struct t_gui_buffer *buffer,
                                              struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
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
 * Adds a buffer local variable to completions list (for `/buffer setauto`).
 */

void
completion_list_map_buffer_local_variable_setauto_cb (void *data,
                                                      struct t_hashtable *hashtable,
                                                      const void *key,
                                                      const void *value)
{
    char str_localvar[4096];

    /* make C compiler happy */
    (void) hashtable;
    (void) value;

    snprintf (str_localvar, sizeof (str_localvar),
              "localvar_set_%s", (const char *)key);
    gui_completion_list_add ((struct t_gui_completion *)data,
                             str_localvar,
                             0, WEECHAT_LIST_POS_SORT);
}

/*
 * Adds buffer properties (that can be set), local variables and key bindings
 * to completion list.
 */

int
completion_list_add_buffer_properties_setauto_cb (const void *pointer, void *data,
                                                  const char *completion_item,
                                                  struct t_gui_buffer *buffer,
                                                  struct t_gui_completion *completion)
{
    struct t_gui_key *ptr_key;
    char str_key[1024];
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    /* add buffer properties */
    for (i = 0; gui_buffer_properties_set[i]; i++)
    {
        gui_completion_list_add (completion,
                                 gui_buffer_properties_set[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }

    /* add buffer local variables */
    hashtable_map (completion->buffer->local_variables,
                   &completion_list_map_buffer_local_variable_setauto_cb,
                   completion);

    /* add buffer keys */
    for (ptr_key = completion->buffer->keys; ptr_key;
         ptr_key = ptr_key->next_key)
    {
        snprintf (str_key, sizeof (str_key), "key_bind_%s", ptr_key->key);
        gui_completion_list_add (completion, str_key, 0, WEECHAT_LIST_POS_SORT);
        snprintf (str_key, sizeof (str_key), "key_unbind_%s", ptr_key->key);
        gui_completion_list_add (completion, str_key, 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds buffer properties (that can be read) to completion list.
 */

int
completion_list_add_buffer_properties_get_cb (const void *pointer, void *data,
                                              const char *completion_item,
                                              struct t_gui_buffer *buffer,
                                              struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_windows_numbers_cb (const void *pointer, void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    struct t_gui_window *ptr_win;
    char str_number[32];

    /* make C compiler happy */
    (void) pointer;
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
 * Adds colors to completion list.
 */

int
completion_list_add_colors_cb (const void *pointer, void *data,
                               const char *completion_item,
                               struct t_gui_buffer *buffer,
                               struct t_gui_completion *completion)
{
    char str_number[64];
    const char *color_name;
    int i, num_colors;
    struct t_gui_color_palette *color_palette;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

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
completion_list_add_palette_colors_cb (const void *pointer, void *data,
                                       const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_config_files_cb (const void *pointer, void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_config_file *ptr_config_file;

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_filename_cb (const void *pointer, void *data,
                                 const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    char home[3] = { '~', DIR_SEPARATOR_CHAR, '\0' };
    char *ptr_home, *pos, buf[PATH_MAX], *real_prefix, *prefix, *path_dir;
    char *path_base, *dir_name;
    const char *pos_args;
    int length_path_base;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    completion->add_space = 0;

    pos_args = (completion_item) ? strchr (completion_item, ':') : NULL;
    if (pos_args)
        pos_args++;

    ptr_home = getenv ("HOME");

    real_prefix = NULL;
    prefix = NULL;
    path_dir = NULL;
    path_base = NULL;
    dir_name = NULL;

    if (ptr_home && (strncmp (completion->base_word, home, 2) == 0))
    {
        real_prefix = strdup (ptr_home);
        prefix = strdup (home);
    }
    else
    {
        if (!completion->base_word[0]
            || completion->base_word[0] != DIR_SEPARATOR_CHAR)
        {
            real_prefix = NULL;
            if (pos_args && pos_args[0])
            {
                real_prefix = eval_expression (pos_args, NULL, NULL, NULL);
                if (real_prefix && !real_prefix[0])
                {
                    free (real_prefix);
                    real_prefix = NULL;
                }
            }
            if (!real_prefix)
                real_prefix = strdup (weechat_data_dir);
            prefix = strdup ("");
        }
        else
        {
            real_prefix = strdup (DIR_SEPARATOR);
            prefix = strdup (DIR_SEPARATOR);
        }
    }
    if (!real_prefix || !prefix)
        goto end;

    snprintf (buf, sizeof (buf),
              "%s", completion->base_word + strlen (prefix));
    pos = strrchr (buf, DIR_SEPARATOR_CHAR);
    if (pos)
    {
        pos[0] = '\0';
        path_dir = strdup (buf);
        path_base = strdup (pos + 1);
    }
    else
    {
        path_dir = strdup ("");
        path_base = strdup (buf);
    }
    if (!path_dir || !path_base)
        goto end;

    snprintf (buf, sizeof (buf),
              "%s%s%s", real_prefix, DIR_SEPARATOR, path_dir);
    dir_name = strdup (buf);
    if (!dir_name)
        goto end;

    dp = opendir (dir_name);
    if (!dp)
        goto end;

    length_path_base = strlen (path_base);
    while ((entry = readdir (dp)) != NULL)
    {
        if (strncmp (entry->d_name, path_base, length_path_base) != 0)
            continue;

        /* skip "." and ".." */
        if ((strcmp (entry->d_name, ".") == 0)
            || (strcmp (entry->d_name, "..") == 0))
        {
            continue;
        }

        /* skip entry if not accessible */
        snprintf (buf, sizeof (buf), "%s%s%s",
                  dir_name, DIR_SEPARATOR, entry->d_name);
        if (stat (buf, &statbuf) == -1)
            continue;

        /* build full path name */
        snprintf (buf, sizeof (buf),
                  "%s%s%s%s%s%s",
                  prefix,
                  (prefix[0] && !strchr (prefix, DIR_SEPARATOR_CHAR)) ?
                  DIR_SEPARATOR : "",
                  path_dir,
                  (path_dir[0]) ? DIR_SEPARATOR : "",
                  entry->d_name,
                  S_ISDIR(statbuf.st_mode) ? DIR_SEPARATOR : "");

        /* add path to list of completions */
        gui_completion_list_add (completion, buf,
                                 0, WEECHAT_LIST_POS_SORT);
    }
    closedir (dp);

end:
    free (real_prefix);
    free (prefix);
    free (path_dir);
    free (path_base);
    free (dir_name);
    return WEECHAT_RC_OK;
}

/*
 * Adds filter names to completion list.
 */

int
completion_list_add_filters_cb (const void *pointer, void *data,
                                const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    struct t_gui_filter *ptr_filter;

    /* make C compiler happy */
    (void) pointer;
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
 * Adds disabled filter names to completion list.
 */

int
completion_list_add_filters_disabled_cb (const void *pointer, void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion)
{
    struct t_gui_filter *ptr_filter;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (!ptr_filter->enabled)
        {
            gui_completion_list_add (completion, ptr_filter->name,
                                     0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds enabled filter names to completion list.
 */

int
completion_list_add_filters_enabled_cb (const void *pointer, void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    struct t_gui_filter *ptr_filter;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (ptr_filter->enabled)
        {
            gui_completion_list_add (completion, ptr_filter->name,
                                     0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds command hook types to completion list.
 */

int
completion_list_add_hook_types_cb (const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; i < HOOK_NUM_TYPES; i++)
    {
        gui_completion_list_add (completion, hook_type_string[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds command hooks to completion list.
 */

int
completion_list_add_commands_cb (const void *pointer, void *data,
                                 const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    const char *pos;
    char str_command[512];
    struct t_hook *ptr_hook;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    pos = (completion_item) ? strchr (completion_item, ':') : NULL;
    if (pos)
        pos++;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (HOOK_COMMAND(ptr_hook, command))
            && (HOOK_COMMAND(ptr_hook, command)[0]))
        {
            if (pos)
            {
                snprintf (str_command, sizeof (str_command),
                          "%s%s",
                          pos,
                          HOOK_COMMAND(ptr_hook, command));
                gui_completion_list_add (completion, str_command,
                                         0, WEECHAT_LIST_POS_SORT);
            }
            else
            {
                gui_completion_list_add (completion,
                                         HOOK_COMMAND(ptr_hook, command),
                                         0, WEECHAT_LIST_POS_SORT);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds info hooks to completion list.
 */

int
completion_list_add_infos_cb (const void *pointer, void *data,
                              const char *completion_item,
                              struct t_gui_buffer *buffer,
                              struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_infolists_cb (const void *pointer, void *data,
                                  const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_nicks_cb (const void *pointer, void *data,
                              const char *completion_item,
                              struct t_gui_buffer *buffer,
                              struct t_gui_completion *completion)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    int count_before;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    count_before = completion->list->size;
    hook_completion_exec (completion->buffer->plugin,
                          "nick",
                          completion->buffer,
                          completion);
    if (completion->list->size == count_before)
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
completion_list_add_config_options_cb (const void *pointer, void *data,
                                       const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *option_full_name;

    /* make C compiler happy */
    (void) pointer;
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
                if (string_asprintf (&option_full_name,
                                     "%s.%s.%s",
                                     ptr_config->name,
                                     ptr_section->name,
                                     ptr_option->name) >= 0)
                {
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
completion_list_add_plugins_cb (const void *pointer, void *data,
                                const char *completion_item,
                                struct t_gui_buffer *buffer,
                                struct t_gui_completion *completion)
{
    struct t_weechat_plugin *ptr_plugin;

    /* make C compiler happy */
    (void) pointer;
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
 * Adds a plugin installed to completion list.
 */

void
completion_list_add_plugins_installed_exec_cb (void *data,
                                               const char *filename)
{
    struct t_gui_completion *completion;
    const char *pos, *pos2;
    char *name;

    completion = (struct t_gui_completion *)data;

    /* start after last '/' (or '\') in path */
    pos = strrchr (filename, DIR_SEPARATOR_CHAR);
    if (pos)
        pos++;
    else
        pos = filename;

    /* truncate after the last '.' in name */
    pos2 = strrchr (pos, '.');
    if (pos2)
        name = string_strndup (pos, pos2 - pos);
    else
        name = strdup (pos);

    if (name)
    {
        gui_completion_list_add (completion, name, 0, WEECHAT_LIST_POS_SORT);
        free (name);
    }
}

/*
 * Adds plugins installed to completion list.
 */

int
completion_list_add_plugins_installed_cb (const void *pointer, void *data,
                                          const char *completion_item,
                                          struct t_gui_buffer *buffer,
                                          struct t_gui_completion *completion)
{
    char *plugin_path, *dir_name, *extra_libdir;
    struct t_hashtable *options;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    /* plugins in WeeChat extra lib dir */
    extra_libdir = getenv (WEECHAT_EXTRA_LIBDIR);
    if (extra_libdir && extra_libdir[0])
    {
        if (string_asprintf (&dir_name, "%s/plugins", extra_libdir) >= 0)
        {
            dir_exec_on_files (dir_name, 1, 0,
                               &completion_list_add_plugins_installed_exec_cb,
                               completion);
            free (dir_name);
        }
    }

    /* plugins in WeeChat home dir */
    if (CONFIG_STRING(config_plugin_path)
        && CONFIG_STRING(config_plugin_path)[0])
    {
        options = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
        if (options)
            hashtable_set (options, "directory", "data");
        plugin_path = string_eval_path_home (CONFIG_STRING(config_plugin_path),
                                             NULL, NULL, options);
        hashtable_free (options);
        if (plugin_path)
        {
            dir_exec_on_files (plugin_path, 1, 0,
                               &completion_list_add_plugins_installed_exec_cb,
                               completion);
            free (plugin_path);
        }
    }

    /* plugins in WeeChat global lib dir */
    if (string_asprintf (&dir_name, "%s/plugins", WEECHAT_LIBDIR) >= 0)
    {
        dir_exec_on_files (dir_name, 1, 0,
                           &completion_list_add_plugins_installed_exec_cb,
                           completion);
        free (dir_name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds plugin commands to completion list.
 *
 * The plugin name is read in previous argument.
 */

int
completion_list_add_plugins_commands_cb (const void *pointer, void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion)
{
    char **argv, str_command[512];
    const char *pos;
    int argc, arg_index;
    struct t_weechat_plugin *ptr_plugin;
    struct t_hook *ptr_hook;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    if (!completion->args)
        return WEECHAT_RC_OK;

    argv = string_split (completion->args, " ", NULL,
                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                         0, &argc);
    if (!argv)
        return WEECHAT_RC_OK;

    if (argc > 0)
    {
        pos = (completion_item) ? strchr (completion_item, ':') : NULL;
        if (pos)
            pos++;

        arg_index = completion->base_command_arg_index - 2;
        if ((arg_index < 0) || (arg_index > argc - 1))
            arg_index = argc - 1;

        ptr_plugin = NULL;
        if (strcmp (argv[arg_index], PLUGIN_CORE) != 0)
        {
            /*
             * plugin name is different from "core", then search it in
             * plugin list
             */
            ptr_plugin = plugin_search (argv[arg_index]);
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
                if (pos)
                {
                    snprintf (str_command, sizeof (str_command),
                              "%s%s",
                              pos,
                              HOOK_COMMAND(ptr_hook, command));
                    gui_completion_list_add (completion, str_command,
                                             0, WEECHAT_LIST_POS_SORT);
                }
                else
                {
                    gui_completion_list_add (completion,
                                             HOOK_COMMAND(ptr_hook, command),
                                             0, WEECHAT_LIST_POS_SORT);
                }
            }
        }
    }

    string_free_split (argv);

    return WEECHAT_RC_OK;
}

/*
 * Adds value of option to completion list.
 *
 * The option name is read in previous argument.
 */

int
completion_list_add_config_option_values_cb (const void *pointer, void *data,
                                             const char *completion_item,
                                             struct t_gui_buffer *buffer,
                                             struct t_gui_completion *completion)
{
    char *pos_space, *option_full_name, *pos_section, *pos_option;
    char *file, *section, *value_string, **ptr_value;
    const char *color_name;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section, *section_found;
    struct t_config_option *option_found;

    /* make C compiler happy */
    (void) completion_item;
    (void) buffer;

    if (!completion->args)
        return WEECHAT_RC_OK;

    pos_space = strchr (completion->args, ' ');
    if (pos_space)
        option_full_name = string_strndup (completion->args,
                                           pos_space - completion->args);
    else
        option_full_name = strdup (completion->args);

    if (!option_full_name)
        return WEECHAT_RC_OK;

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
                            if (option_found->value && CONFIG_INTEGER(option_found) > option_found->min)
                                gui_completion_list_add (completion, "--1",
                                                         0, WEECHAT_LIST_POS_BEGINNING);
                            if (option_found->value && CONFIG_INTEGER(option_found) < option_found->max)
                                gui_completion_list_add (completion, "++1",
                                                         0, WEECHAT_LIST_POS_BEGINNING);
                            if (option_found->value)
                            {
                                if (string_asprintf (
                                        &value_string,
                                        "%d",
                                        CONFIG_INTEGER(option_found)) >= 0)
                                {
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
                        case CONFIG_OPTION_TYPE_STRING:
                            gui_completion_list_add (completion,
                                                     "\"\"",
                                                     0, WEECHAT_LIST_POS_BEGINNING);
                            if (option_found->value)
                            {
                                if (string_asprintf (
                                        &value_string,
                                        "\"%s\"",
                                        CONFIG_STRING(option_found)) >= 0)
                                {
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
                            completion_list_add_colors_cb (
                                pointer, data, completion_item, buffer,
                                completion);
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
                        case CONFIG_OPTION_TYPE_ENUM:
                            for (ptr_value = option_found->string_values;
                                 *ptr_value; ptr_value++)
                            {
                                gui_completion_list_add (completion,
                                                         *ptr_value,
                                                         0, WEECHAT_LIST_POS_SORT);
                            }
                            gui_completion_list_add (completion, "++1",
                                                     0, WEECHAT_LIST_POS_END);
                            gui_completion_list_add (completion, "--1",
                                                     0, WEECHAT_LIST_POS_END);
                            if (option_found->value)
                            {
                                gui_completion_list_add (completion,
                                                         option_found->string_values[CONFIG_ENUM(option_found)],
                                                         0, WEECHAT_LIST_POS_BEGINNING);
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

    free (file);
    free (section);
    free (option_full_name);

    return WEECHAT_RC_OK;
}

/*
 * Adds WeeChat commands to completion list.
 */

int
completion_list_add_weechat_commands_cb (const void *pointer, void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    const char *pos;
    char str_command[512];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    pos = (completion_item) ? strchr (completion_item, ':') : NULL;
    if (pos)
        pos++;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && !ptr_hook->plugin
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0])
        {
            if (pos)
            {
                snprintf (str_command, sizeof (str_command),
                          "%s%s",
                          pos,
                          HOOK_COMMAND(ptr_hook, command));
                gui_completion_list_add (completion, str_command,
                                         0, WEECHAT_LIST_POS_SORT);
            }
            else
            {
                gui_completion_list_add (completion,
                                         HOOK_COMMAND(ptr_hook, command),
                                         0, WEECHAT_LIST_POS_SORT);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds proxy names to completion list.
 */

int
completion_list_add_proxies_names_cb (const void *pointer, void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    struct t_proxy *ptr_proxy;

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_proxies_options_cb (const void *pointer, void *data,
                                        const char *completion_item,
                                        struct t_gui_buffer *buffer,
                                        struct t_gui_completion *completion)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_keys_contexts_cb (const void *pointer, void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    int context;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
    {
        gui_completion_list_add (completion, gui_key_context_string[context],
                                 0, WEECHAT_LIST_POS_END);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds keys to completion list.
 */

int
completion_list_add_keys_codes_cb (const void *pointer, void *data,
                                   const char *completion_item,
                                   struct t_gui_buffer *buffer,
                                   struct t_gui_completion *completion)
{
    int context;
    struct t_gui_key *ptr_key;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
    {
        for (ptr_key = gui_keys[context]; ptr_key; ptr_key = ptr_key->next_key)
        {
            gui_completion_list_add (completion, ptr_key->key,
                                     0, WEECHAT_LIST_POS_SORT);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds keys that can be reset (keys added, redefined or removed) to completion
 * list.
 */

int
completion_list_add_keys_codes_for_reset_cb (const void *pointer, void *data,
                                             const char *completion_item,
                                             struct t_gui_buffer *buffer,
                                             struct t_gui_completion *completion)
{
    int context;
    struct t_gui_key *ptr_key, *ptr_default_key;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
    {
        /* keys added or redefined */
        for (ptr_key = gui_keys[context]; ptr_key; ptr_key = ptr_key->next_key)
        {
            ptr_default_key = gui_key_search (gui_default_keys[context],
                                              ptr_key->key);
            if (!ptr_default_key
                || (strcmp (ptr_default_key->command, ptr_key->command) != 0))
            {
                gui_completion_list_add (completion, ptr_key->key,
                                         0, WEECHAT_LIST_POS_SORT);
            }
        }

        /* keys deleted */
        for (ptr_default_key = gui_default_keys[context]; ptr_default_key;
             ptr_default_key = ptr_default_key->next_key)
        {
            ptr_key = gui_key_search (gui_keys[context],
                                      ptr_default_key->key);
            if (!ptr_key)
            {
                gui_completion_list_add (completion, ptr_default_key->key,
                                         0, WEECHAT_LIST_POS_SORT);
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
completion_list_add_cursor_areas_cb (const void *pointer, void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_gui_bar_window *ptr_bar_win;
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) pointer;
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
completion_list_add_layouts_names_cb (const void *pointer, void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    struct t_gui_layout *ptr_layout;

    /* make C compiler happy */
    (void) pointer;
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
 * Adds a secured data to completion list.
 */

void
completion_list_map_add_secured_data_cb (void *data,
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
 * Adds secured data to completion list.
 */

int
completion_list_add_secured_data_cb (const void *pointer, void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    hashtable_map (secure_hashtable_data,
                   &completion_list_map_add_secured_data_cb,
                   completion);

    return WEECHAT_RC_OK;
}

/*
 * Adds environment variables to completion list.
 */

int
completion_list_add_env_vars_cb (const void *pointer, void *data,
                                 const char *completion_item,
                                 struct t_gui_buffer *buffer,
                                 struct t_gui_completion *completion)
{
    char *pos, *name, **ptr_environ;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (ptr_environ = environ; *ptr_environ; ptr_environ++)
    {
        pos = strchr (*ptr_environ, '=');
        if (pos)
        {
            name = string_strndup (*ptr_environ, pos - *ptr_environ);
            if (name)
            {
                gui_completion_list_add (completion, name,
                                         0, WEECHAT_LIST_POS_SORT);
                free (name);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds value of an environment variable to completion list.
 */

int
completion_list_add_env_value_cb (const void *pointer, void *data,
                                  const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion)
{
    char **argv, *value;
    int argc, arg_index;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    if (completion->args)
    {
        argv = string_split (completion->args, " ", NULL,
                             WEECHAT_STRING_SPLIT_STRIP_LEFT
                             | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                             | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                             0, &argc);
        if (!argv)
            return WEECHAT_RC_OK;

        if (argc > 0)
        {
            arg_index = completion->base_command_arg_index - 2;
            if ((arg_index < 1) || (arg_index > argc - 1))
                arg_index = argc - 1;
            value = getenv (argv[arg_index]);
            if (value)
            {
                gui_completion_list_add (completion, value,
                                         0, WEECHAT_LIST_POS_END);
            }
        }
        string_free_split (argv);
    }

    return WEECHAT_RC_OK;
}

/*
 * Adds a buffer local variable for /eval to completions list.
 */

void
completion_list_map_eval_buffer_local_variable_cb (void *data,
                                                   struct t_hashtable *hashtable,
                                                   const void *key, const void *value)
{
    char *name;

    /* make C compiler happy */
    (void) hashtable;
    (void) value;

    if (string_asprintf (&name, "${%s}", (const char *)key) >= 0)
    {
        gui_completion_list_add ((struct t_gui_completion *)data,
                                 name, 0, WEECHAT_LIST_POS_SORT);
        free (name);
    }
}

/*
 * Adds /eval variables to completion list.
 */

int
completion_list_add_eval_variables_cb (const void *pointer, void *data,
                                       const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    char *eval_variables[] = {
        "${\\string}",
        "${base_decode:16,string}",
        "${base_decode:32,string}",
        "${base_decode:64,string}",
        "${base_decode:64url,string}",
        "${base_encode:16,string}",
        "${base_encode:32,string}",
        "${base_encode:64,string}",
        "${base_encode:64url,string}",
        "${calc:expression}",
        "${chars:alpha}",
        "${chars:alnum}",
        "${chars:c1-c2}",
        "${chars:digit}",
        "${chars:lower}",
        "${chars:upper}",
        "${chars:xdigit}",
        "${color:name}",
        "${cut:+max,suffix,string}",
        "${cut:max,suffix,string}",
        "${cutscr:+max,suffix,string}",
        "${cutscr:max,suffix,string}",
        "${date:format}",
        "${date}",
        "${define:name,value}",
        "${env:NAME}",
        "${esc:string}",
        "${eval:string}",
        "${eval_cond:string}",
        "${file.section.option}",
        "${hdata.var1.var2}",
        "${hdata[list].var1.var2}",
        "${hdata[ptr].var1.var2}",
        "${hdata[ptr_name].var1.var2}",
        "${hdata_count:name[list]}",
        "${hdata_count:name[ptr]}",
        "${hide:char,string}",
        "${hl:string}",
        "${if:condition?value_if_true:value_if_false}",
        "${info:name,arguments}",
        "${length:string}",
        "${lengthscr:string}",
        "${lower:string}",
        "${modifier:name,data,string}",
        "${random:min,max}",
        "${raw:string}",
        "${raw_hl:string}",
        "${re:+}",
        "${re:N}",
        "${repeat:count,string}",
        "${rev:string}",
        "${revscr:string}",
        "${sec.data.xxx}",
        "${split:count,separators,flags,string}",
        "${split:N,separators,flags,string}",
        "${split:random,separators,flags,string}",
        "${split_shell:count,string}",
        "${split_shell:N,string}",
        "${split_shell:random,string}",
        "${translate:string}",
        "${upper:string}",
        "${weechat_cache_dir}",
        "${weechat_config_dir}",
        "${weechat_data_dir}",
        "${weechat_runtime_dir}",
        "${weechat_state_dir}",
        "${window}",
        "${window.buffer}",
        "${window.buffer.xxx}",
        NULL,
    };
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    for (i = 0; eval_variables[i]; i++)
    {
        gui_completion_list_add (completion, eval_variables[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }

    hashtable_map (completion->buffer->local_variables,
                   &completion_list_map_eval_buffer_local_variable_cb,
                   completion);

    return WEECHAT_RC_OK;
}

/*
 * Adds hooks for completions done by WeeChat core.
 */

void
completion_init (void)
{
    hook_completion (NULL, "buffers_names", /* formerly "%b" */
                     N_("names of buffers"),
                     &completion_list_add_buffers_names_cb, NULL, NULL);
    hook_completion (NULL, "buffers_numbers",
                     N_("numbers of buffers"),
                     &completion_list_add_buffers_numbers_cb, NULL, NULL);
    hook_completion (NULL, "buffers_plugins_names", /* formerly "%B" */
                     N_("names of buffers (including plugins names)"),
                     &completion_list_add_buffers_plugins_names_cb, NULL, NULL);
    hook_completion (NULL, "buffer_local_variables",
                     N_("buffer local variables"),
                     &completion_list_add_buffer_local_variables_cb, NULL, NULL);
    hook_completion (NULL, "buffer_local_variable_value",
                     N_("value of a buffer local variable"),
                     &completion_list_add_buffer_local_variable_value_cb, NULL, NULL);
    hook_completion (NULL, "buffer_properties_set",
                     N_("properties that can be set on a buffer"),
                     &completion_list_add_buffer_properties_set_cb, NULL, NULL);
    hook_completion (NULL, "buffer_properties_setauto",
                     N_("properties that can be automatically set on a buffer"),
                     &completion_list_add_buffer_properties_setauto_cb, NULL, NULL);
    hook_completion (NULL, "buffer_properties_get",
                     N_("properties that can be read on a buffer"),
                     &completion_list_add_buffer_properties_get_cb, NULL, NULL);
    hook_completion (NULL, "windows_numbers",
                     N_("numbers of windows"),
                     &completion_list_add_windows_numbers_cb, NULL, NULL);
    hook_completion (NULL, "colors",
                     N_("color names"),
                     &completion_list_add_colors_cb, NULL, NULL);
    hook_completion (NULL, "palette_colors",
                     N_("palette colors"),
                     &completion_list_add_palette_colors_cb, NULL, NULL);
    hook_completion (NULL, "config_files", /* formerly "%c" */
                     N_("configuration files"),
                     &completion_list_add_config_files_cb, NULL, NULL);
    hook_completion (NULL, "filename", /* formerly "%f" */
                     N_("filename; "
                        "optional argument: default path (evaluated, "
                        "see /help eval)"),
                     &completion_list_add_filename_cb, NULL, NULL);
    hook_completion (NULL, "filters_names", /* formerly "%F" */
                     N_("names of filters"),
                     &completion_list_add_filters_cb, NULL, NULL);
    hook_completion (NULL, "filters_names_disabled",
                     N_("names of disabled filters"),
                     &completion_list_add_filters_disabled_cb, NULL, NULL);
    hook_completion (NULL, "filters_names_enabled",
                     N_("names of enabled filters"),
                     &completion_list_add_filters_enabled_cb, NULL, NULL);
    hook_completion (NULL, "hook_types",
                     N_("hook types"),
                     &completion_list_add_hook_types_cb, NULL, NULL);
    hook_completion (NULL, "commands", /* formerly "%h" */
                     N_("commands (weechat and plugins); "
                        "optional argument: prefix to add before the commands"),
                     &completion_list_add_commands_cb, NULL, NULL);
    hook_completion (NULL, "infos", /* formerly "%i" */
                     N_("names of infos hooked"),
                     &completion_list_add_infos_cb, NULL, NULL);
    hook_completion (NULL, "infolists", /* formerly "%I" */
                     N_("names of infolists hooked"),
                     &completion_list_add_infolists_cb, NULL, NULL);
    hook_completion (NULL, "nicks", /* formerly "%n" */
                     N_("nicks in nicklist of current buffer"),
                     &completion_list_add_nicks_cb, NULL, NULL);
    hook_completion (NULL, "config_options", /* formerly "%o" */
                     N_("configuration options"),
                     &completion_list_add_config_options_cb, NULL, NULL);
    hook_completion (NULL, "plugins_names", /* formerly "%p" */
                     N_("names of plugins"),
                     &completion_list_add_plugins_cb, NULL, NULL);
    hook_completion (NULL, "plugins_installed",
                     N_("names of plugins installed"),
                     &completion_list_add_plugins_installed_cb, NULL, NULL);
    hook_completion (NULL, "plugins_commands", /* formerly "%P" */
                     N_("commands defined by plugins; "
                        "optional argument: prefix to add before the commands"),
                     &completion_list_add_plugins_commands_cb, NULL, NULL);
    hook_completion (NULL, "bars_names", /* formerly "%r" */
                     N_("names of bars"),
                     &completion_list_add_bars_names_cb, NULL, NULL);
    hook_completion (NULL, "bars_items",
                     N_("names of bar items"),
                     &completion_list_add_bars_items_cb, NULL, NULL);
    hook_completion (NULL, "custom_bar_items_names",
                     N_("names of custom bar items"),
                     &completion_list_add_custom_bar_items_names_cb, NULL, NULL);
    hook_completion (NULL, "custom_bar_item_conditions",
                     N_("conditions for custom bar item"),
                     &completion_list_add_custom_bar_item_conditions_cb, NULL, NULL);
    hook_completion (NULL, "custom_bar_item_contents",
                     N_("contents for custom bar item"),
                     &completion_list_add_custom_bar_item_contents_cb, NULL, NULL);
    hook_completion (NULL, "custom_bar_item_add_arguments",
                     N_("arguments for command that adds a custom bar item: "
                        "item name, conditions, content"),
                     &completion_list_add_custom_bar_item_add_arguments_cb, NULL, NULL);
    hook_completion (NULL, "config_option_values", /* formerly "%v" */
                     N_("values for a configuration option"),
                     &completion_list_add_config_option_values_cb, NULL, NULL);
    hook_completion (NULL, "weechat_commands", /* formerly "%w" */
                     N_("weechat commands; "
                        "optional argument: prefix to add before the commands"),
                     &completion_list_add_weechat_commands_cb, NULL, NULL);
    hook_completion (NULL, "proxies_names", /* formerly "%y" */
                     N_("names of proxies"),
                     &completion_list_add_proxies_names_cb, NULL, NULL);
    hook_completion (NULL, "proxies_options",
                     N_("options for proxies"),
                     &completion_list_add_proxies_options_cb, NULL, NULL);
    hook_completion (NULL, "bars_options",
                     N_("options for bars"),
                     &completion_list_add_bars_options_cb, NULL, NULL);
    hook_completion (NULL, "keys_contexts",
                     /* TRANSLATORS: "key" means "key on the keyboard" */
                     N_("key contexts"),
                     &completion_list_add_keys_contexts_cb, NULL, NULL);
    hook_completion (NULL, "keys_codes",
                     /* TRANSLATORS: "key" means "key on the keyboard" */
                     N_("key codes"),
                     &completion_list_add_keys_codes_cb, NULL, NULL);
    hook_completion (NULL, "keys_codes_for_reset",
                     /* TRANSLATORS: "key" means "key on the keyboard" */
                     N_("key codes that can be reset (keys added, redefined "
                        "or removed)"),
                     &completion_list_add_keys_codes_for_reset_cb, NULL, NULL);
    hook_completion (NULL, "cursor_areas",
                     N_("areas (\"chat\" or bar name) for free cursor "
                        "movement"),
                     &completion_list_add_cursor_areas_cb, NULL, NULL);
    hook_completion (NULL, "layouts_names",
                     N_("names of layouts"),
                     &completion_list_add_layouts_names_cb, NULL, NULL);
    hook_completion (NULL, "secured_data",
                     N_("names of secured data (file sec.conf, section data)"),
                     &completion_list_add_secured_data_cb, NULL, NULL);
    hook_completion (NULL, "env_vars",
                     N_("environment variables"),
                     &completion_list_add_env_vars_cb, NULL, NULL);
    hook_completion (NULL, "env_value",
                     N_("value of an environment variable"),
                     &completion_list_add_env_value_cb, NULL, NULL);
    hook_completion (NULL, "eval_variables",
                     N_("variables that can be used in /eval command"),
                     &completion_list_add_eval_variables_cb, NULL, NULL);
}
