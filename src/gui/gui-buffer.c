/*
 * gui-buffer.c - buffer functions (used by all GUI)
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-eval.h"
#include "../core/core-hashtable.h"
#include "../core/core-hdata.h"
#include "../core/core-hook.h"
#include "../core/core-infolist.h"
#include "../core/core-list.h"
#include "../core/core-log.h"
#include "../core/core-secure-buffer.h"
#include "../core/core-string.h"
#include "../core/core-utf8.h"
#include "../plugins/plugin.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-completion.h"
#include "gui-filter.h"
#include "gui-history.h"
#include "gui-hotlist.h"
#include "gui-input.h"
#include "gui-key.h"
#include "gui-layout.h"
#include "gui-line.h"
#include "gui-main.h"
#include "gui-nicklist.h"
#include "gui-window.h"


struct t_gui_buffer *gui_buffers = NULL;           /* first buffer          */
struct t_gui_buffer *last_gui_buffer = NULL;       /* last buffer           */
int gui_buffers_count = 0;                         /* number of buffers     */

/* history of last visited buffers */
struct t_gui_buffer_visited *gui_buffers_visited = NULL;
struct t_gui_buffer_visited *last_gui_buffer_visited = NULL;
int gui_buffers_visited_index = -1;             /* index of pointer in list */
int gui_buffers_visited_count = 0;              /* number of visited buffers*/
int gui_buffers_visited_frozen = 0;             /* 1 to forbid list updates */
struct t_gui_buffer *gui_buffer_last_displayed = NULL; /* last b. displayed */

long long gui_buffer_last_id_assigned = -1;      /* last id assigned         */

char *gui_buffer_reserved_names[] =
{ GUI_BUFFER_MAIN, SECURE_BUFFER_NAME, GUI_COLOR_BUFFER_NAME,
  NULL
};

int gui_buffer_set_signals = 1;        /* 0 to disable signals sent in      */
                                       /* function gui_buffer_set           */

char *gui_buffer_type_string[GUI_BUFFER_NUM_TYPES] =
{ "formatted", "free" };

char *gui_buffer_notify_string[GUI_BUFFER_NUM_NOTIFY] =
{ "none", "highlight", "message", "all" };

char *gui_buffer_properties_get_integer[] =
{ "opening", "number", "layout_number", "layout_number_merge_order", "type",
  "notify", "num_displayed", "active", "hidden", "zoomed",
  "print_hooks_enabled", "day_change", "clear", "filter", "closing",
  "lines_hidden", "prefix_max_length", "next_line_id", "time_for_each_line",
  "nicklist", "nicklist_case_sensitive", "nicklist_max_length",
  "nicklist_display_groups", "nicklist_count", "nicklist_visible_count",
  "nicklist_groups_count", "nicklist_groups_visible_count",
  "nicklist_nicks_count", "nicklist_nicks_visible_count",
  "input", "input_get_any_user_data", "input_get_unknown_commands",
  "input_get_empty", "input_multiline", "input_size", "input_length",
  "input_pos", "input_1st_display", "num_history", "text_search",
  "text_search_direction", "text_search_exact", "text_search_regex",
  "text_search_where", "text_search_history", "text_search_found",
  NULL
};
char *gui_buffer_properties_get_string[] =
{ "id", "plugin", "name", "full_name", "old_full_name", "short_name", "title",
  "modes", "nicklist_last_id_assigned", "input_prompt", "input",
  "text_search_input", "highlight_words", "highlight_disable_regex",
  "highlight_regex", "highlight_tags_restrict", "highlight_tags",
  "hotlist_max_level_nicks",
  NULL
};
char *gui_buffer_properties_get_pointer[] =
{ "plugin", "text_search_regex_compiled", "text_search_ptr_history",
  "highlight_disable_regex_compiled", "highlight_regex_compiled",
  NULL
};
char *gui_buffer_properties_set[] =
{ "hotlist", "unread", "display", "hidden", "print_hooks_enabled", "day_change",
  "clear", "filter", "number", "name", "short_name", "type", "notify", "title",
  "modes", "time_for_each_line", "nicklist", "nicklist_case_sensitive",
  "nicklist_display_groups", "highlight_words", "highlight_words_add",
  "highlight_words_del", "highlight_disable_regex", "highlight_regex",
  "highlight_tags_restrict", "highlight_tags", "hotlist_max_level_nicks",
  "hotlist_max_level_nicks_add", "hotlist_max_level_nicks_del",
  "input_prompt", "input", "input_pos", "input_get_any_user_data",
  "input_get_unknown_commands", "input_get_empty", "input_multiline",
  NULL
};


/*
 * Searches for buffer type.
 *
 * Returns index of buffer type found, -1 if not found.
 */

int
gui_buffer_search_type (const char *type)
{
    int i;

    if (!type)
        return -1;

    for (i = 0; i < GUI_BUFFER_NUM_TYPES; i++)
    {
        if (strcmp (gui_buffer_type_string[i], type) == 0)
            return i;
    }

    return -1;
}

/*
 * Searches for buffer notify level.
 *
 * Returns index of notify level, -1 if not found.
 */

int
gui_buffer_search_notify (const char *notify)
{
    int i;

    if (!notify)
        return -1;

    for (i = 0; i < GUI_BUFFER_NUM_NOTIFY; i++)
    {
        if (strcmp (gui_buffer_notify_string[i], notify) == 0)
            return i;
    }

    return -1;
}

/*
 * Sends a buffer signal (only if the buffer is completely opened.
 */

int
gui_buffer_send_signal (struct t_gui_buffer *buffer,
                        const char *signal,
                        const char *type_data, void *signal_data)
{
    return (buffer->opening) ?
        WEECHAT_RC_OK : hook_signal_send (signal, type_data, signal_data);
}

/*
 * Gets plugin name of buffer.
 *
 * Note: during upgrade process (at startup after /upgrade), the name of plugin
 * is retrieved in temporary variable "plugin_name_for_upgrade".
 */

const char *
gui_buffer_get_plugin_name (struct t_gui_buffer *buffer)
{
    if (!buffer)
        return NULL;

    if (buffer->plugin_name_for_upgrade)
        return buffer->plugin_name_for_upgrade;

    return plugin_get_name (buffer->plugin);
}

/*
 * Get short name of buffer (or name if short_name is NULL).
 *
 * Note: this function never returns NULL.
 */

const char *
gui_buffer_get_short_name (struct t_gui_buffer *buffer)
{
    if (!buffer)
        return NULL;

    return (buffer->short_name) ? buffer->short_name : buffer->name;
}

/*
 * Builds "full_name" of buffer (for example after changing name or
 * plugin_name_for_upgrade).
 */

void
gui_buffer_build_full_name (struct t_gui_buffer *buffer)
{
    int length;

    if (!buffer)
        return;

    free (buffer->full_name);
    length = strlen (gui_buffer_get_plugin_name (buffer)) + 1 +
        strlen (buffer->name) + 1;
    buffer->full_name = malloc (length);
    if (buffer->full_name)
    {
        snprintf (buffer->full_name, length, "%s.%s",
                  gui_buffer_get_plugin_name (buffer), buffer->name);
    }
}

/*
 * Adds a new local variable in a buffer.
 */

void
gui_buffer_local_var_add (struct t_gui_buffer *buffer, const char *name,
                          const char *value)
{
    void *ptr_value;

    if (!buffer || !buffer->local_variables || !name || !value)
        return;

    ptr_value = hashtable_get (buffer->local_variables, name);
    if (string_strcmp (ptr_value, value) == 0)
        return;

    hashtable_set (buffer->local_variables, name, value);

    (void) gui_buffer_send_signal (
        buffer,
        (ptr_value) ? "buffer_localvar_changed" : "buffer_localvar_added",
        WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Removes a local variable in a buffer.
 */

void
gui_buffer_local_var_remove (struct t_gui_buffer *buffer, const char *name)
{
    void *ptr_value;

    if (!buffer || !buffer->local_variables || !name)
        return;

    ptr_value = hashtable_get (buffer->local_variables, name);
    if (ptr_value)
    {
        hashtable_remove (buffer->local_variables, name);
        (void) gui_buffer_send_signal (buffer,
                                       "buffer_localvar_removed",
                                       WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
}

/*
 * Removes all local variables in a buffer.
 */

void
gui_buffer_local_var_remove_all (struct t_gui_buffer *buffer)
{
    if (buffer && buffer->local_variables)
    {
        hashtable_remove_all (buffer->local_variables);
        (void) gui_buffer_send_signal (buffer,
                                       "buffer_localvar_removed",
                                       WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
}

/*
 * Reads a notify level in configuration file.
 *
 * First tries with all arguments, then removes one by one to find notify level
 * (from specific to general notify).
 */

int
gui_buffer_notify_get (struct t_gui_buffer *buffer)
{
    char *option_name, *pos;
    struct t_config_option *ptr_option;

    if (!buffer || !buffer->full_name)
        return CONFIG_ENUM(config_look_buffer_notify_default);

    option_name = strdup (buffer->full_name);
    if (!option_name)
        return CONFIG_ENUM(config_look_buffer_notify_default);

    ptr_option = NULL;

    while (1)
    {
        ptr_option = config_file_search_option (weechat_config_file,
                                                weechat_config_section_notify,
                                                option_name);
        if (ptr_option)
        {
            free (option_name);
            return CONFIG_INTEGER(ptr_option);
        }
        pos = strrchr (option_name, '.');
        if (!pos)
            break;
        pos[0] = '\0';
    }

    ptr_option = config_file_search_option (weechat_config_file,
                                            weechat_config_section_notify,
                                            option_name);

    free (option_name);

    if (ptr_option)
        return CONFIG_INTEGER(ptr_option);

    /* notify level not found */
    return CONFIG_ENUM(config_look_buffer_notify_default);
}

/*
 * Sets notify value on a buffer.
 */

void
gui_buffer_notify_set (struct t_gui_buffer *buffer)
{
    int old_notify, new_notify;

    if (!buffer)
        return;

    old_notify = buffer->notify;
    new_notify = gui_buffer_notify_get (buffer);

    if (new_notify != old_notify)
    {
        buffer->notify = new_notify;
        gui_chat_printf (NULL,
                         _("Notify changed for \"%s%s%s\": \"%s%s%s\" to \"%s%s%s\""),
                         GUI_COLOR(GUI_COLOR_CHAT_BUFFER),
                         buffer->full_name,
                         GUI_COLOR(GUI_COLOR_CHAT),
                         GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                         gui_buffer_notify_string[old_notify],
                         GUI_COLOR(GUI_COLOR_CHAT),
                         GUI_COLOR(GUI_COLOR_CHAT_VALUE),
                         gui_buffer_notify_string[buffer->notify],
                         GUI_COLOR(GUI_COLOR_CHAT));
    }
}

/*
 * Sets notify values on all opened buffers.
 */

void
gui_buffer_notify_set_all ()
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_buffer_notify_set (ptr_buffer);
    }
}

/*
 * Searches for position of buffer in list using its layout number
 * (to keep buffers sorted by number).
 *
 * Returns the pointer to the buffer that will be after the new buffer in list.
 * Returns NULL if position is undefined.
 */

struct t_gui_buffer *
gui_buffer_find_pos (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;

    if (!buffer)
        return NULL;

    /* if no number is asked by layout, position is undefined */
    if (buffer->layout_number < 1)
        return NULL;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((ptr_buffer->layout_number < 1)
            || (buffer->layout_number < ptr_buffer->layout_number)
            || ((buffer->layout_number == ptr_buffer->layout_number)
                && (buffer->layout_number_merge_order <= ptr_buffer->layout_number_merge_order)))
        {
            /* not possible to insert a buffer between 2 merged buffers */
            if (!ptr_buffer->prev_buffer
                || ((ptr_buffer->prev_buffer)->number != ptr_buffer->number))
            {
                return ptr_buffer;
            }
        }
    }

    /* position not found */
    return NULL;
}

/*
 * Shifts number of buffers (number + 1) until we find a gap (if auto renumber
 * is OFF), or until last buffer.
 */

void
gui_buffer_shift_numbers (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = buffer; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->prev_buffer
            && (ptr_buffer->number > ptr_buffer->prev_buffer->number))
        {
            break;
        }
        ptr_buffer->number++;
    }
}

/*
 * Inserts a buffer in good position in list of buffers (keeping sort on
 * number).
 */

void
gui_buffer_insert (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *pos_buffer, *ptr_buffer, *merge_buffer;
    int force_number;

    merge_buffer = NULL;
    force_number = 0;

    pos_buffer = gui_buffer_find_pos (buffer);

    /*
     * if position was not forced by layout and that buffer position is set
     * to "first gap", search for the first available number in the list
     * (if there is not, buffer will be added to the end of list)
     */
    if (!pos_buffer
        && (buffer->layout_number == 0)
        && (CONFIG_ENUM(config_look_buffer_position) == CONFIG_LOOK_BUFFER_POSITION_FIRST_GAP))
    {
        for (ptr_buffer = gui_buffers; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            if (ptr_buffer->prev_buffer)
            {
                if (ptr_buffer->number > ptr_buffer->prev_buffer->number + 1)
                {
                    pos_buffer = ptr_buffer;
                    force_number = ptr_buffer->prev_buffer->number + 1;
                    break;
                }
            }
            else if (ptr_buffer->number > 1)
            {
                pos_buffer = ptr_buffer;
                force_number = 1;
                break;
            }
        }
    }

    /*
     * if position is found in list and that buffer is in layout and must be
     * merged to another buffer (before/after), then force merge with buffer and
     * add buffer to the end (to be sure no other buffer number will be shifted
     * before the merge)
     */
    if (pos_buffer
        && (buffer->layout_number > 0)
        && ((buffer->layout_number == pos_buffer->layout_number)
            || (pos_buffer->prev_buffer
                && (buffer->layout_number == (pos_buffer->prev_buffer)->layout_number))))
    {
        merge_buffer = (buffer->layout_number == pos_buffer->layout_number) ?
            pos_buffer : pos_buffer->prev_buffer;
        pos_buffer = NULL;
    }

    if (pos_buffer)
    {
        /* add buffer into the list (before position found) */
        if (force_number > 0)
        {
            buffer->number = force_number;
        }
        else if (!CONFIG_BOOLEAN(config_look_buffer_auto_renumber)
                 && (buffer->layout_number > 0)
                 && (buffer->layout_number < pos_buffer->number)
                 && (!pos_buffer->prev_buffer
                     || (buffer->layout_number > pos_buffer->prev_buffer->number)))
        {
            buffer->number = buffer->layout_number;
        }
        else
        {
            buffer->number = pos_buffer->number;
        }
        buffer->prev_buffer = pos_buffer->prev_buffer;
        buffer->next_buffer = pos_buffer;
        if (pos_buffer->prev_buffer)
            (pos_buffer->prev_buffer)->next_buffer = buffer;
        else
            gui_buffers = buffer;
        pos_buffer->prev_buffer = buffer;
        if (buffer->number == pos_buffer->number)
            gui_buffer_shift_numbers (pos_buffer);
    }
    else
    {
        /* add buffer to the end */
        if (!CONFIG_BOOLEAN(config_look_buffer_auto_renumber)
            && (buffer->layout_number > 0)
            && (!last_gui_buffer
                || (buffer->layout_number > last_gui_buffer->number)))
        {
            buffer->number = buffer->layout_number;
        }
        else
        {
            buffer->number = (last_gui_buffer) ? last_gui_buffer->number + 1 : 1;
        }
        buffer->prev_buffer = last_gui_buffer;
        buffer->next_buffer = NULL;
        if (last_gui_buffer)
            last_gui_buffer->next_buffer = buffer;
        else
            gui_buffers = buffer;
        last_gui_buffer = buffer;
    }

    if (merge_buffer)
        gui_buffer_merge (buffer, merge_buffer);
    else
    {
        /* merge buffer with previous or next, if they have layout number */
        if (buffer->layout_number > 0)
        {
            if (buffer->prev_buffer
                && (buffer->layout_number == (buffer->prev_buffer)->layout_number))
            {
                gui_buffer_merge (buffer, buffer->prev_buffer);
            }
            else if ((buffer->next_buffer)
                     && (buffer->layout_number == (buffer->next_buffer)->layout_number))
            {
                gui_buffer_merge (buffer->next_buffer, buffer);
            }
        }
    }
}

/*
 * Returns a new unique id for a buffer.
 *
 * The id is the current time with microseconds precision.
 * The same time (including microseconds) can be used only one time, so that
 * all buffer ids are guaranteed to be unique.
 */

long long
gui_buffer_generate_id ()
{
    struct timeval tv;
    long long id;

    gettimeofday (&tv, NULL);

    id = ((long long)tv.tv_sec * 1000000LL) + (long long)(tv.tv_usec);

    /*
     * ensure we never use the same id for two buffers and that the returned
     * id is strictly greater than the last assigned one
     */
    if (id <= gui_buffer_last_id_assigned)
        id = gui_buffer_last_id_assigned + 1;

    return id;
}

/*
 * Initializes input_buffer_* variables in a buffer.
 */

void
gui_buffer_input_buffer_init (struct t_gui_buffer *buffer)
{
    buffer->input_buffer_alloc = GUI_BUFFER_INPUT_BLOCK_SIZE;
    buffer->input_buffer = malloc (GUI_BUFFER_INPUT_BLOCK_SIZE);
    if (buffer->input_buffer)
	buffer->input_buffer[0] = '\0';
    buffer->input_buffer_size = 0;
    buffer->input_buffer_length = 0;
    buffer->input_buffer_pos = 0;
    buffer->input_buffer_1st_display = 0;
}

/*
 * Checks if the name is reserved for WeeChat.
 *
 * Returns:
 *   0: name is not reserved for WeeChat
 *   1: name is reserved for WeeChat
 */

int
gui_buffer_is_reserved_name (const char *name)
{
    int i;

    if (!name)
        return 0;

    for (i = 0; gui_buffer_reserved_names[i]; i++)
    {
        if (strcmp (name, gui_buffer_reserved_names[i]) == 0)
            break;
    }

    return (gui_buffer_reserved_names[i]) ? 1 : 0;
}

/*
 * Applies buffer properties (callback of hashtable_map).
 */

void
gui_buffer_apply_properties_cb (void *data,
                                struct t_hashtable *hashtable,
                                const void *key, const void *value)
{
    /* make C compiler happy */
    (void) hashtable;

    gui_buffer_set ((struct t_gui_buffer *)data,
                    (const char *)key,
                    (const char *)value);
}

/*
 * Applies a buffer property defined in a an option "weechat.buffer.xxx".
 */

void
gui_buffer_apply_config_option_property (struct t_gui_buffer *buffer,
                                         struct t_config_option *option)
{
    const char *pos;
    char *buffer_mask, *value;
    struct t_hashtable *pointers, *extra_vars;

    pos = strrchr (option->name, '.');
    if (!pos)
        return;

    buffer_mask = strndup (option->name, pos - option->name);
    if (!buffer_mask)
        return;

    pos++;

    if (string_match (buffer->full_name, buffer_mask, 1))
    {
        if ((strncmp (pos, "key_bind_", 9) == 0)
            || (strncmp (pos, "key_unbind_", 11) == 0))
        {
            /*
             * no evaluation for local buffer key bindings: this allows to
             * use command /eval without having to escape `${`
             */
            gui_buffer_set (buffer, pos, CONFIG_STRING(option));
        }
        else
        {
            pointers = hashtable_new (
                32,
                WEECHAT_HASHTABLE_STRING,
                WEECHAT_HASHTABLE_POINTER,
                NULL, NULL);
            extra_vars = hashtable_new (
                32,
                WEECHAT_HASHTABLE_STRING,
                WEECHAT_HASHTABLE_STRING,
                NULL, NULL);
            if (pointers && extra_vars)
            {
                hashtable_set (pointers, "buffer", buffer);
                hashtable_set (extra_vars, "property", pos);
                value = eval_expression (CONFIG_STRING(option),
                                         pointers, extra_vars, NULL);
                if (value)
                {
                    gui_buffer_set (buffer, pos, value);
                    free (value);
                }
                hashtable_free (pointers);
                hashtable_free (extra_vars);
            }
        }
    }

    free (buffer_mask);
}

/*
 * Applies buffer properties defined in options "weechat.buffer.*".
 */

void
gui_buffer_apply_config_properties (struct t_gui_buffer *buffer)
{
    struct t_config_option *ptr_option;

    for (ptr_option = weechat_config_section_buffer->options; ptr_option;
         ptr_option = ptr_option->next_option)
    {
        gui_buffer_apply_config_option_property (buffer, ptr_option);
    }
}

/*
 * Creates a new buffer in current window with some optional properties,
 * using id (this function is for internal use only).
 *
 * Returns pointer to new buffer, NULL if error.
 */

struct t_gui_buffer *
gui_buffer_new_props_with_id (long long id,
                              struct t_weechat_plugin *plugin,
                              const char *name,
                              struct t_hashtable *properties,
                              int (*input_callback)(const void *pointer,
                                                    void *data,
                                                    struct t_gui_buffer *buffer,
                                                    const char *input_data),
                              const void *input_callback_pointer,
                              void *input_callback_data,
                              int (*close_callback)(const void *pointer,
                                                    void *data,
                                                    struct t_gui_buffer *buffer),
                              const void *close_callback_pointer,
                              void *close_callback_data)
{
    struct t_gui_buffer *new_buffer;
    int first_buffer_creation;

    if (!name || !name[0])
        return NULL;

    if (gui_buffer_search (plugin_get_name (plugin), name))
    {
        gui_chat_printf (NULL,
                         _("%sA buffer with same name (%s) already exists"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
        return NULL;
    }

    if (gui_buffers_count >= GUI_BUFFERS_MAX)
    {
        gui_chat_printf (NULL,
                         _("%sMaximum number of buffers is reached (%d)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         GUI_BUFFERS_MAX);
        return NULL;
    }

    /* create new buffer */
    new_buffer = malloc (sizeof (*new_buffer));
    if (!new_buffer)
        return NULL;

    /* init buffer */
    new_buffer->id = id;
    if (new_buffer->id > gui_buffer_last_id_assigned)
        gui_buffer_last_id_assigned = new_buffer->id;
    new_buffer->opening = 1;
    new_buffer->plugin = plugin;
    new_buffer->plugin_name_for_upgrade = NULL;

    /* number will be set later (when inserting buffer in list) */
    gui_layout_buffer_get_number (gui_layout_current,
                                  plugin_get_name (plugin),
                                  name,
                                  &(new_buffer->layout_number),
                                  &(new_buffer->layout_number_merge_order));
    new_buffer->name = strdup (name);
    new_buffer->full_name = NULL;
    new_buffer->old_full_name = NULL;
    gui_buffer_build_full_name (new_buffer);
    new_buffer->short_name = NULL;
    new_buffer->type = GUI_BUFFER_TYPE_DEFAULT;
    new_buffer->notify = CONFIG_ENUM(config_look_buffer_notify_default);
    new_buffer->num_displayed = 0;
    new_buffer->active = 1;
    new_buffer->hidden = 0;
    new_buffer->zoomed = 0;
    new_buffer->print_hooks_enabled = 1;
    new_buffer->day_change = 1;
    new_buffer->clear = 1;
    new_buffer->filter = 1;

    /* close callback */
    new_buffer->close_callback = close_callback;
    new_buffer->close_callback_pointer = close_callback_pointer;
    new_buffer->close_callback_data = close_callback_data;
    new_buffer->closing = 0;

    /* title */
    new_buffer->title = NULL;

    /* modes */
    new_buffer->modes = NULL;

    /* chat content */
    new_buffer->own_lines = gui_line_lines_alloc ();
    new_buffer->mixed_lines = NULL;
    new_buffer->lines = new_buffer->own_lines;
    new_buffer->next_line_id = 0;
    new_buffer->time_for_each_line = 1;
    new_buffer->chat_refresh_needed = 2;

    /* nicklist */
    new_buffer->nicklist = 0;
    new_buffer->nicklist_case_sensitive = 0;
    new_buffer->nicklist_root = NULL;
    new_buffer->nicklist_max_length = 0;
    new_buffer->nicklist_display_groups = 1;
    new_buffer->nicklist_count = 0;
    new_buffer->nicklist_visible_count = 0;
    new_buffer->nicklist_groups_count = 0;
    new_buffer->nicklist_groups_visible_count = 0;
    new_buffer->nicklist_nicks_count = 0;
    new_buffer->nicklist_nicks_visible_count = 0;
    new_buffer->nicklist_last_id_assigned = -1;
    new_buffer->nickcmp_callback = NULL;
    new_buffer->nickcmp_callback_pointer = NULL;
    new_buffer->nickcmp_callback_data = NULL;
    gui_nicklist_add_group_with_id (new_buffer, 0, NULL, "root", NULL, 0);

    /* input */
    new_buffer->input = 1;
    new_buffer->input_callback = input_callback;
    new_buffer->input_callback_pointer = input_callback_pointer;
    new_buffer->input_callback_data = input_callback_data;
    new_buffer->input_get_any_user_data = 0;
    new_buffer->input_get_unknown_commands = 0;
    new_buffer->input_get_empty = 0;
    new_buffer->input_multiline = 0;
    new_buffer->input_prompt = NULL;
    gui_buffer_input_buffer_init (new_buffer);

    /* undo for input */
    new_buffer->input_undo_snap = malloc (sizeof (*(new_buffer->input_undo_snap)));
    (new_buffer->input_undo_snap)->data = NULL;
    (new_buffer->input_undo_snap)->pos = 0;
    (new_buffer->input_undo_snap)->prev_undo = NULL; /* not used */
    (new_buffer->input_undo_snap)->next_undo = NULL; /* not used */
    new_buffer->input_undo = NULL;
    new_buffer->last_input_undo = NULL;
    new_buffer->ptr_input_undo = NULL;
    new_buffer->input_undo_count = 0;

    /* init completion */
    new_buffer->completion = gui_completion_new (NULL, new_buffer);

    /* init history */
    new_buffer->history = NULL;
    new_buffer->last_history = NULL;
    new_buffer->ptr_history = NULL;
    new_buffer->num_history = 0;

    /* text search */
    new_buffer->text_search = GUI_BUFFER_SEARCH_DISABLED;
    new_buffer->text_search_direction = GUI_BUFFER_SEARCH_DIR_BACKWARD;
    new_buffer->text_search_exact = 0;
    new_buffer->text_search_regex = 0;
    new_buffer->text_search_regex_compiled = NULL;
    new_buffer->text_search_where = 0;
    new_buffer->text_search_history = GUI_BUFFER_SEARCH_HISTORY_NONE;
    new_buffer->text_search_found = 0;
    new_buffer->text_search_ptr_history = NULL;
    new_buffer->text_search_input = NULL;

    /* highlight */
    new_buffer->highlight_words = NULL;
    new_buffer->highlight_disable_regex = NULL;
    new_buffer->highlight_disable_regex_compiled = NULL;
    new_buffer->highlight_regex = NULL;
    new_buffer->highlight_regex_compiled = NULL;
    new_buffer->highlight_tags_restrict = NULL;
    new_buffer->highlight_tags_restrict_count = 0;
    new_buffer->highlight_tags_restrict_array = NULL;
    new_buffer->highlight_tags = NULL;
    new_buffer->highlight_tags_count = 0;
    new_buffer->highlight_tags_array = NULL;

    /* hotlist */
    new_buffer->hotlist = NULL;
    new_buffer->hotlist_removed = NULL;
    new_buffer->hotlist_max_level_nicks = hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_INTEGER,
        NULL, NULL);

    /* keys */
    new_buffer->keys = NULL;
    new_buffer->last_key = NULL;
    new_buffer->keys_count = 0;

    /* local variables */
    new_buffer->local_variables = hashtable_new (32,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 NULL, NULL);
    hashtable_set (new_buffer->local_variables,
                   "plugin", plugin_get_name (plugin));
    hashtable_set (new_buffer->local_variables, "name", name);

    /* add buffer to buffers list */
    first_buffer_creation = (gui_buffers == NULL);
    gui_buffer_insert (new_buffer);

    gui_buffers_count++;

    /* set notify level */
    new_buffer->notify = gui_buffer_notify_get (new_buffer);

    /* assign this buffer to windows of layout */
    gui_layout_window_assign_buffer (new_buffer);

    /* apply properties (from parameters) */
    if (properties)
        hashtable_map (properties, &gui_buffer_apply_properties_cb, new_buffer);

    /* apply properties (from options weechat.buffer.*) */
    gui_buffer_apply_config_properties (new_buffer);

    new_buffer->opening = 0;

    if (first_buffer_creation)
    {
        gui_buffer_visited_add (new_buffer);
    }
    else
    {
        (void) gui_buffer_send_signal (
            new_buffer,
            "buffer_opened",
            WEECHAT_HOOK_SIGNAL_POINTER, new_buffer);
    }

    return new_buffer;
}

/*
 * Creates a new buffer in current window with some optional properties.
 *
 * Returns pointer to new buffer, NULL if error.
 */

struct t_gui_buffer *
gui_buffer_new_props (struct t_weechat_plugin *plugin,
                      const char *name,
                      struct t_hashtable *properties,
                      int (*input_callback)(const void *pointer,
                                            void *data,
                                            struct t_gui_buffer *buffer,
                                            const char *input_data),
                      const void *input_callback_pointer,
                      void *input_callback_data,
                      int (*close_callback)(const void *pointer,
                                            void *data,
                                            struct t_gui_buffer *buffer),
                      const void *close_callback_pointer,
                      void *close_callback_data)
{
    return gui_buffer_new_props_with_id (
        gui_buffer_generate_id (),
        plugin,
        name,
        properties,
        input_callback,
        input_callback_pointer,
        input_callback_data,
        close_callback,
        close_callback_pointer,
        close_callback_data);
}

/*
 * Creates a new buffer in current window.
 *
 * Returns pointer to new buffer, NULL if error.
 */

struct t_gui_buffer *
gui_buffer_new (struct t_weechat_plugin *plugin,
                const char *name,
                int (*input_callback)(const void *pointer,
                                      void *data,
                                      struct t_gui_buffer *buffer,
                                      const char *input_data),
                const void *input_callback_pointer,
                void *input_callback_data,
                int (*close_callback)(const void *pointer,
                                      void *data,
                                      struct t_gui_buffer *buffer),
                const void *close_callback_pointer,
                void *close_callback_data)
{
    return gui_buffer_new_props (
        plugin,
        name,
        NULL,  /* properties */
        input_callback, input_callback_pointer, input_callback_data,
        close_callback, close_callback_pointer, close_callback_data);
}

/*
 * Input callback for user buffers.
 */

int
gui_buffer_user_input_cb (const void *pointer, void *data,
                          struct t_gui_buffer *buffer, const char *input_data)
{
    char str_signal[1024];
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    snprintf (str_signal, sizeof (str_signal),
              "buffer_user_input_%s",
              buffer->name);
    rc = hook_signal_send (str_signal,
                           WEECHAT_HOOK_SIGNAL_STRING, (void *)input_data);

    if (rc == WEECHAT_RC_OK_EAT)
        return WEECHAT_RC_OK;

    if (string_strcmp (input_data, "q") == 0)
        gui_buffer_close (buffer);

    return WEECHAT_RC_OK;
}

/*
 * Close callback for user buffers.
 */

int
gui_buffer_user_close_cb (const void *pointer, void *data,
                          struct t_gui_buffer *buffer)
{
    char str_signal[1024];

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    snprintf (str_signal, sizeof (str_signal),
              "buffer_user_closing_%s",
              buffer->name);
    hook_signal_send (str_signal,
                      WEECHAT_HOOK_SIGNAL_STRING, NULL);

    return WEECHAT_RC_OK;
}

/*
 * Creates a new user buffer in current window.
 *
 * Returns pointer to new buffer, NULL if error.
 */

struct t_gui_buffer *
gui_buffer_new_user (const char *name, enum t_gui_buffer_type buffer_type)
{
    struct t_hashtable *properties;
    struct t_gui_buffer *new_buffer;

    properties = hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (properties)
    {
        if (buffer_type != GUI_BUFFER_TYPE_DEFAULT)
        {
            hashtable_set (properties,
                           "type", gui_buffer_type_string[buffer_type]);
        }
        hashtable_set (properties, "localvar_set_type", "user");
    }

    new_buffer = gui_buffer_new_props (NULL, name, properties,
                                       &gui_buffer_user_input_cb, NULL, NULL,
                                       &gui_buffer_user_close_cb, NULL, NULL);

    hashtable_free (properties);

    return new_buffer;
}

/*
 * Sets callbacks on user buffers.
 */

void
gui_buffer_user_set_callbacks ()
{
    struct t_gui_buffer *ptr_buffer;
    const char *ptr_type;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (!ptr_buffer->plugin
            && !gui_buffer_is_reserved_name (ptr_buffer->name))
        {
            ptr_type = gui_buffer_get_string (ptr_buffer, "localvar_type");
            if (ptr_type && (strcmp (ptr_type, "user") == 0))
            {
                ptr_buffer->input_callback = &gui_buffer_user_input_cb;
            }
        }
    }
}

/*
 * Checks if a buffer pointer is valid.
 *
 * Returns:
 *   1: buffer exists
 *   0: buffer does not exist
 */

int
gui_buffer_valid (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;

    /* NULL buffer is valid (it's for printing on first buffer) */
    if (!buffer)
        return 1;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer == buffer)
            return 1;
    }

    /* buffer not found */
    return 0;
}

/*
 * Replaces local variables ($var) in a string, using value of local variables.
 *
 * Note: result must be freed after use.
 */

char *
gui_buffer_string_replace_local_var (struct t_gui_buffer *buffer,
                                     const char *string)
{
    int index_string, index_result;
    size_t length, length_var;
    char *result, *result2, *local_var;
    const char *pos_end_name, *ptr_value;

    if (!buffer || !string)
        return NULL;

    length = strlen (string) + 1;
    result = malloc (length);
    if (result)
    {
        index_string = 0;
        index_result = 0;
        while (string[index_string])
        {
            if ((string[index_string] == '$')
                && ((index_string == 0) || (string[index_string - 1] != '\\')))
            {
                pos_end_name = string + index_string + 1;
                while (pos_end_name[0])
                {
                    if (isalnum ((unsigned char)pos_end_name[0])
                        && (pos_end_name[0] != '_')
                        && (pos_end_name[0] != '$'))
                        pos_end_name++;
                    else
                        break;
                }
                if (pos_end_name > string + index_string + 1)
                {
                    local_var = string_strndup (string + index_string + 1,
                                                pos_end_name - (string + index_string + 1));
                    if (local_var)
                    {
                        ptr_value = (const char *)hashtable_get (buffer->local_variables,
                                                                 local_var);
                        if (ptr_value)
                        {
                            length_var = strlen (ptr_value);
                            if (length > SIZE_MAX - length_var)
                            {
                                result2 = NULL;
                            }
                            else
                            {
                                length += length_var;
                                result2 = realloc (result, length);
                            }
                            if (!result2)
                            {
                                free (result);
                                free (local_var);
                                return NULL;
                            }
                            result = result2;
                            strcpy (result + index_result, ptr_value);
                            index_result += length_var;
                            index_string += strlen (local_var) + 1;
                        }
                        else
                            result[index_result++] = string[index_string++];

                        free (local_var);
                    }
                    else
                        result[index_result++] = string[index_string++];
                }
                else
                    result[index_result++] = string[index_string++];
            }
            else
                result[index_result++] = string[index_string++];
        }
        result[index_result] = '\0';
    }

    return result;
}

/*
 * Checks if full name of buffer marches list of buffers.
 *
 * List is a comma-separated list of buffers, where exclusion is possible with
 * char '!', and "*" means all buffers.
 *
 * Comparison is case sensitive.
 *
 * Examples:
 *   "*"
 *   "*,!*#weechat*"
 *   "irc.libera.*"
 *   "irc.libera.*,irc.oftc.#channel"
 *   "irc.libera.#weechat,irc.libera.#other"
 */

int
gui_buffer_match_list (struct t_gui_buffer *buffer, const char *string)
{
    char **buffers;
    int match;

    if (!buffer || !string || !string[0])
        return 0;

    match = 0;

    buffers = string_split (string, ",", NULL,
                            WEECHAT_STRING_SPLIT_STRIP_LEFT
                            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                            0, NULL);
    if (buffers)
    {
        match = string_match_list (buffer->full_name,
                                   (const char **)buffers, 1);
        string_free_split (buffers);
    }

    return match;
}

/*
 * Sets plugin pointer for buffers with a given name (used after /upgrade).
 */

void
gui_buffer_set_plugin_for_upgrade (char *name, struct t_weechat_plugin *plugin)
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->plugin_name_for_upgrade
            && (strcmp (ptr_buffer->plugin_name_for_upgrade, name) == 0))
        {
            free (ptr_buffer->plugin_name_for_upgrade);
            ptr_buffer->plugin_name_for_upgrade = NULL;

            ptr_buffer->plugin = plugin;

            gui_buffer_build_full_name (ptr_buffer);
        }
    }
}

/*
 * Check if a property is in the list of properties.
 *
 * Returns:
 *   1: property is in list
 *   0: property is not in list
 */

int
gui_buffer_property_in_list (char *properties[], char *property)
{
    int i;

    if (!properties || !property)
        return 0;

    for (i = 0; properties[i]; i++)
    {
        if (strcmp (properties[i], property) == 0)
            return 1;
    }

    /* property not found in list */
    return 0;
}

/*
 * Gets a buffer property as integer.
 */

int
gui_buffer_get_integer (struct t_gui_buffer *buffer, const char *property)
{
    if (!buffer || !property)
        return 0;

    if (strcmp (property, "opening") == 0)
        return buffer->opening;
    else if (strcmp (property, "number") == 0)
        return buffer->number;
    else if (strcmp (property, "layout_number") == 0)
        return buffer->layout_number;
    else if (strcmp (property, "layout_number_merge_order") == 0)
        return buffer->layout_number_merge_order;
    else if (strcmp (property, "short_name_is_set") == 0)
        return (buffer->short_name) ? 1 : 0;
    else if (strcmp (property, "type") == 0)
        return buffer->type;
    else if (strcmp (property, "notify") == 0)
        return buffer->notify;
    else if (strcmp (property, "num_displayed") == 0)
        return buffer->num_displayed;
    else if (strcmp (property, "active") == 0)
        return buffer->active;
    else if (strcmp (property, "hidden") == 0)
        return buffer->hidden;
    else if (strcmp (property, "zoomed") == 0)
        return buffer->zoomed;
    else if (strcmp (property, "print_hooks_enabled") == 0)
        return buffer->print_hooks_enabled;
    else if (strcmp (property, "day_change") == 0)
        return buffer->day_change;
    else if (strcmp (property, "clear") == 0)
        return buffer->clear;
    else if (strcmp (property, "filter") == 0)
        return buffer->filter;
    else if (strcmp (property, "closing") == 0)
        return buffer->closing;
    else if (strcmp (property, "lines_hidden") == 0)
        return buffer->lines->lines_hidden;
    else if (strcmp (property, "prefix_max_length") == 0)
        return buffer->lines->prefix_max_length;
    else if (strcmp (property, "next_line_id") == 0)
        return buffer->next_line_id;
    else if (strcmp (property, "time_for_each_line") == 0)
        return buffer->time_for_each_line;
    else if (strcmp (property, "nicklist") == 0)
        return buffer->nicklist;
    else if (strcmp (property, "nicklist_case_sensitive") == 0)
        return buffer->nicklist_case_sensitive;
    else if (strcmp (property, "nicklist_max_length") == 0)
        return buffer->nicklist_max_length;
    else if (strcmp (property, "nicklist_display_groups") == 0)
        return buffer->nicklist_display_groups;
    else if (strcmp (property, "nicklist_count") == 0)
        return buffer->nicklist_count;
    else if (strcmp (property, "nicklist_visible_count") == 0)
        return buffer->nicklist_visible_count;
    else if (strcmp (property, "nicklist_groups_count") == 0)
        return buffer->nicklist_groups_count;
    else if (strcmp (property, "nicklist_groups_visible_count") == 0)
        return buffer->nicklist_groups_visible_count;
    else if (strcmp (property, "nicklist_nicks_count") == 0)
        return buffer->nicklist_nicks_count;
    else if (strcmp (property, "nicklist_nicks_visible_count") == 0)
        return buffer->nicklist_nicks_visible_count;
    else if (strcmp (property, "input") == 0)
        return buffer->input;
    else if (strcmp (property, "input_get_any_user_data") == 0)
        return buffer->input_get_any_user_data;
    else if (strcmp (property, "input_get_unknown_commands") == 0)
        return buffer->input_get_unknown_commands;
    else if (strcmp (property, "input_get_empty") == 0)
        return buffer->input_get_empty;
    else if (strcmp (property, "input_multiline") == 0)
        return buffer->input_multiline;
    else if (strcmp (property, "input_size") == 0)
        return buffer->input_buffer_size;
    else if (strcmp (property, "input_length") == 0)
        return buffer->input_buffer_length;
    else if (strcmp (property, "input_pos") == 0)
        return buffer->input_buffer_pos;
    else if (strcmp (property, "input_1st_display") == 0)
        return buffer->input_buffer_1st_display;
    else if (strcmp (property, "num_history") == 0)
        return buffer->num_history;
    else if (strcmp (property, "text_search") == 0)
        return buffer->text_search;
    else if (strcmp (property, "text_search_direction") == 0)
        return buffer->text_search_direction;
    else if (strcmp (property, "text_search_exact") == 0)
        return buffer->text_search_exact;
    else if (strcmp (property, "text_search_regex") == 0)
        return buffer->text_search_regex;
    else if (strcmp (property, "text_search_where") == 0)
        return buffer->text_search_where;
    else if (strcmp (property, "text_search_history") == 0)
        return buffer->text_search_history;
    else if (strcmp (property, "text_search_found") == 0)
        return buffer->text_search_found;

    return 0;
}

/*
 * Gets a buffer property as string.
 */

const char *
gui_buffer_get_string (struct t_gui_buffer *buffer, const char *property)
{
    const char *ptr_value;
    static char str_value[64];

    if (!buffer || !property)
        return NULL;

    if (strcmp (property, "id") == 0)
    {
        snprintf (str_value, sizeof (str_value), "%lld", buffer->id);
        return str_value;
    }
    else if (strcmp (property, "plugin") == 0)
        return gui_buffer_get_plugin_name (buffer);
    else if (strcmp (property, "name") == 0)
        return buffer->name;
    else if (strcmp (property, "full_name") == 0)
        return buffer->full_name;
    else if (strcmp (property, "old_full_name") == 0)
        return buffer->old_full_name;
    else if (strcmp (property, "short_name") == 0)
        return gui_buffer_get_short_name (buffer);
    else if (strcmp (property, "type") == 0)
       return gui_buffer_type_string[buffer->type];
    else if (strcmp (property, "title") == 0)
        return buffer->title;
    else if (strcmp (property, "modes") == 0)
        return buffer->modes;
    else if (strcmp (property, "nicklist_last_id_assigned") == 0)
    {
        snprintf (str_value, sizeof (str_value),
                  "%lld", buffer->nicklist_last_id_assigned);
        return str_value;
    }
    else if (strcmp (property, "input_prompt") == 0)
        return buffer->input_prompt;
    else if (strcmp (property, "input") == 0)
        return buffer->input_buffer;
    else if (strcmp (property, "text_search_input") == 0)
        return buffer->text_search_input;
    else if (strcmp (property, "highlight_words") == 0)
        return buffer->highlight_words;
    else if (strcmp (property, "highlight_disable_regex") == 0)
        return buffer->highlight_disable_regex;
    else if (strcmp (property, "highlight_regex") == 0)
        return buffer->highlight_regex;
    else if (strcmp (property, "highlight_tags_restrict") == 0)
        return buffer->highlight_tags_restrict;
    else if (strcmp (property, "highlight_tags") == 0)
        return buffer->highlight_tags;
    else if (strcmp (property, "hotlist_max_level_nicks") == 0)
        return hashtable_get_string (buffer->hotlist_max_level_nicks, "keys_values");
    else if (string_strncmp (property, "localvar_", 9) == 0)
    {
        ptr_value = (const char *)hashtable_get (buffer->local_variables,
                                                 property + 9);
        if (ptr_value)
            return ptr_value;
    }

    return NULL;
}

/*
 * Gets a buffer property as pointer.
 */

void *
gui_buffer_get_pointer (struct t_gui_buffer *buffer, const char *property)
{
    if (!buffer || !property)
        return NULL;

    if (strcmp (property, "plugin") == 0)
        return buffer->plugin;
    else if (strcmp (property, "text_search_regex_compiled") == 0)
        return buffer->text_search_regex_compiled;
    else if (strcmp (property, "text_search_ptr_history") == 0)
        return buffer->text_search_ptr_history;
    else if (strcmp (property, "highlight_disable_regex_compiled") == 0)
        return buffer->highlight_disable_regex_compiled;
    else if (strcmp (property, "highlight_regex_compiled") == 0)
        return buffer->highlight_regex_compiled;

    return NULL;
}

/*
 * Sets flag "chat_refresh_needed".
 */

void
gui_buffer_ask_chat_refresh (struct t_gui_buffer *buffer, int refresh)
{
    if (!buffer)
        return;

    if (refresh > buffer->chat_refresh_needed)
        buffer->chat_refresh_needed = refresh;
}

/*
 * Sets day_change for a buffer.
 */

void
gui_buffer_set_day_change (struct t_gui_buffer *buffer, int day_change)
{
    day_change = (day_change) ? 1 : 0;

    if (!buffer || (buffer->day_change == day_change))
        return;

    buffer->day_change = day_change;
    gui_buffer_ask_chat_refresh (buffer, 2);
}

/*
 * Sets filter for a buffer.
 */

void
gui_buffer_set_filter (struct t_gui_buffer *buffer, int filter)
{
    filter = (filter) ? 1 : 0;

    if (!buffer || (buffer->filter == filter))
        return;

    buffer->filter = filter;
    gui_filter_buffer (buffer, NULL);
}

/*
 * Sets name for a buffer.
 */

void
gui_buffer_set_name (struct t_gui_buffer *buffer, const char *name)
{
    if (!buffer || !name || !name[0] || (string_strcmp (buffer->name, name) == 0))
        return;

    /*
     * save the old full name so that hooks on signal "buffer_renamed"
     * can read the old name
     */
    free (buffer->old_full_name);
    buffer->old_full_name = strdup (buffer->full_name);

    free (buffer->name);
    buffer->name = strdup (name);

    gui_buffer_build_full_name (buffer);

    gui_buffer_local_var_add (buffer, "name", name);

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_renamed",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);

    if (buffer->old_full_name)
    {
        free (buffer->old_full_name);
        buffer->old_full_name = NULL;
    }
}

/*
 * Sets short name for a buffer.
 */

void
gui_buffer_set_short_name (struct t_gui_buffer *buffer, const char *short_name)
{
    if (!buffer || (string_strcmp (buffer->short_name, short_name) == 0))
        return;

    free (buffer->short_name);
    buffer->short_name = (short_name && short_name[0]) ?
        strdup (short_name) : NULL;

    if (buffer->mixed_lines)
        buffer->mixed_lines->buffer_max_length_refresh = 1;
    gui_buffer_ask_chat_refresh (buffer, 1);

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_renamed",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sets buffer type.
 */

void
gui_buffer_set_type (struct t_gui_buffer *buffer, enum t_gui_buffer_type type)
{
    if (!buffer || (buffer->type == type))
        return;

    gui_line_free_all (buffer);

    buffer->type = type;

    switch (type)
    {
        case GUI_BUFFER_TYPE_FORMATTED:
            buffer->time_for_each_line = 1;
            break;
        case GUI_BUFFER_TYPE_FREE:
            buffer->clear = 0;
            buffer->time_for_each_line = 0;
            break;
        case GUI_BUFFER_NUM_TYPES:
            break;
    }

    gui_buffer_ask_chat_refresh (buffer, 2);

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_type_changed",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sets notify for a buffer.
 */

void
gui_buffer_set_notify (struct t_gui_buffer *buffer, const char *notify)
{
    const char *ptr_notify;
    char *error;
    long number;
    int mute_old;

    if (!buffer)
        return;

    ptr_notify = NULL;
    error = NULL;
    number = strtol (notify, &error, 10);
    if (error && !error[0])
    {
        if (number < GUI_BUFFER_NUM_NOTIFY)
        {
            if (number < 0)
                number = CONFIG_ENUM(config_look_buffer_notify_default);
            ptr_notify = gui_buffer_notify_string[number];
        }
    }
    else
    {
        ptr_notify = notify;
    }
    if (ptr_notify)
    {
        mute_old = gui_chat_mute;
        gui_chat_mute = GUI_CHAT_MUTE_ALL_BUFFERS;
        config_weechat_notify_set (buffer, ptr_notify);
        gui_chat_mute = mute_old;
    }
}

/*
 * Sets title for a buffer.
 */

void
gui_buffer_set_title (struct t_gui_buffer *buffer, const char *new_title)
{
    if (!buffer || (string_strcmp (buffer->title, new_title) == 0))
        return;

    free (buffer->title);
    buffer->title = (new_title && new_title[0]) ? strdup (new_title) : NULL;

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_title_changed",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sets modes for a buffer.
 */

void
gui_buffer_set_modes (struct t_gui_buffer *buffer, const char *modes)
{
    if (!buffer || (string_strcmp (buffer->modes, modes) == 0))
        return;

    free (buffer->modes);
    buffer->modes = (modes && modes[0]) ? strdup (modes) : NULL;

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_modes_changed",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sets flag "time for each line" for a buffer.
 */

void
gui_buffer_set_time_for_each_line (struct t_gui_buffer *buffer,
                                   int time_for_each_line)
{
    time_for_each_line = (time_for_each_line) ? 1 : 0;

    if (!buffer || (buffer->time_for_each_line == time_for_each_line))
        return;

    buffer->time_for_each_line = time_for_each_line;
    gui_buffer_ask_chat_refresh (buffer, 2);
}

/*
 * Sets nicklist for a buffer.
 */

void
gui_buffer_set_nicklist (struct t_gui_buffer *buffer, int nicklist)
{
    nicklist = (nicklist) ? 1 : 0;

    if (!buffer || (buffer->nicklist == nicklist))
        return;

    buffer->nicklist = nicklist;
    gui_window_ask_refresh (1);
}

/*
 * Sets case_sensitive flag for a buffer.
 */

void
gui_buffer_set_nicklist_case_sensitive (struct t_gui_buffer *buffer,
                                        int case_sensitive)
{
    if (!buffer)
        return;

    buffer->nicklist_case_sensitive = (case_sensitive) ? 1 : 0;
}

/*
 * Sets flag "display_groups" for a buffer.
 */

void
gui_buffer_set_nicklist_display_groups (struct t_gui_buffer *buffer,
                                        int display_groups)
{
    display_groups = (display_groups) ? 1 : 0;

    if (!buffer || (buffer->nicklist_display_groups == display_groups))
        return;

    buffer->nicklist_display_groups = display_groups;
    buffer->nicklist_visible_count = 0;
    buffer->nicklist_groups_visible_count = 0;
    buffer->nicklist_nicks_visible_count = 0;
    gui_nicklist_compute_visible_count (buffer, buffer->nicklist_root);
    gui_window_ask_refresh (1);
}

/*
 * Sets highlight words for a buffer.
 */

void
gui_buffer_set_highlight_words (struct t_gui_buffer *buffer,
                                const char *new_highlight_words)
{
    if (!buffer)
        return;

    free (buffer->highlight_words);
    buffer->highlight_words = (new_highlight_words && new_highlight_words[0]) ?
        strdup (new_highlight_words) : NULL;
}

/*
 * Sets highlight words for a buffer with a list.
 */

void
gui_buffer_set_highlight_words_list (struct t_gui_buffer *buffer,
                                     struct t_weelist *list)
{
    struct t_weelist_item *ptr_list_item;
    const char *ptr_string;
    char **words;

    if (!buffer)
        return;

    words = string_dyn_alloc (64);
    if (!words)
        return;

    if (list)
    {
        for (ptr_list_item = weelist_get (list, 0); ptr_list_item;
             ptr_list_item = weelist_next (ptr_list_item))
        {
            ptr_string = weelist_string (ptr_list_item);
            if (ptr_string)
            {
                if (*words[0])
                    string_dyn_concat (words, ",", -1);
                string_dyn_concat (words, ptr_string, -1);
            }
        }
    }

    gui_buffer_set_highlight_words (buffer, *words);

    string_dyn_free (words, 1);
}

/*
 * Adds highlight words for a buffer.
 */

void
gui_buffer_add_highlight_words (struct t_gui_buffer *buffer,
                                const char *words_to_add)
{
    char **current_words, **add_words;
    int current_count, add_count, i;
    struct t_weelist *list;

    if (!buffer || !words_to_add)
        return;

    list = weelist_new ();
    if (!list)
        return;

    current_words = string_split (buffer->highlight_words, ",", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &current_count);
    add_words = string_split (words_to_add, ",", NULL,
                              WEECHAT_STRING_SPLIT_STRIP_LEFT
                              | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                              | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                              0, &add_count);

    for (i = 0; i < current_count; i++)
    {
        if (!weelist_search (list, current_words[i]))
            weelist_add (list, current_words[i], WEECHAT_LIST_POS_END, NULL);
    }
    for (i = 0; i < add_count; i++)
    {
        if (!weelist_search (list, add_words[i]))
            weelist_add (list, add_words[i], WEECHAT_LIST_POS_END, NULL);
    }

    gui_buffer_set_highlight_words_list (buffer, list);

    weelist_free (list);

    string_free_split (current_words);
    string_free_split (add_words);
}

/*
 * Removes highlight words in a buffer.
 */

void
gui_buffer_remove_highlight_words (struct t_gui_buffer *buffer,
                                   const char *words_to_remove)
{
    char **current_words, **remove_words;
    int current_count, remove_count, i, j, to_remove;
    struct t_weelist *list;

    if (!buffer || !words_to_remove)
        return;

    list = weelist_new ();
    if (!list)
        return;

    current_words = string_split (buffer->highlight_words, ",", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &current_count);
    remove_words = string_split (words_to_remove, ",", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &remove_count);

    for (i = 0; i < current_count; i++)
    {
        /* search if word is to be removed or not */
        to_remove = 0;
        for (j = 0; j < remove_count; j++)
        {
            if (strcmp (current_words[i], remove_words[j]) == 0)
            {
                to_remove = 1;
                break;
            }
        }
        if (!to_remove)
            weelist_add (list, current_words[i], WEECHAT_LIST_POS_END, NULL);
    }

    gui_buffer_set_highlight_words_list (buffer, list);

    weelist_free (list);

    string_free_split (current_words);
    string_free_split (remove_words);
}

/*
 * Sets highlight disable regex for a buffer.
 */

void
gui_buffer_set_highlight_disable_regex (struct t_gui_buffer *buffer,
                                        const char *new_regex)
{
    if (!buffer)
        return;

    if (buffer->highlight_disable_regex)
    {
        free (buffer->highlight_disable_regex);
        buffer->highlight_disable_regex = NULL;
    }
    if (buffer->highlight_disable_regex_compiled)
    {
        regfree (buffer->highlight_disable_regex_compiled);
        free (buffer->highlight_disable_regex_compiled);
        buffer->highlight_disable_regex_compiled = NULL;
    }

    if (new_regex && new_regex[0])
    {
        buffer->highlight_disable_regex = strdup (new_regex);
        if (buffer->highlight_disable_regex)
        {
            buffer->highlight_disable_regex_compiled =
                malloc (sizeof (*buffer->highlight_disable_regex_compiled));
            if (buffer->highlight_disable_regex_compiled)
            {
                if (string_regcomp (buffer->highlight_disable_regex_compiled,
                                    buffer->highlight_disable_regex,
                                    REG_EXTENDED | REG_ICASE) != 0)
                {
                    free (buffer->highlight_disable_regex_compiled);
                    buffer->highlight_disable_regex_compiled = NULL;
                }
            }
        }
    }
}

/*
 * Sets highlight regex for a buffer.
 */

void
gui_buffer_set_highlight_regex (struct t_gui_buffer *buffer,
                                const char *new_regex)
{
    if (!buffer)
        return;

    if (buffer->highlight_regex)
    {
        free (buffer->highlight_regex);
        buffer->highlight_regex = NULL;
    }
    if (buffer->highlight_regex_compiled)
    {
        regfree (buffer->highlight_regex_compiled);
        free (buffer->highlight_regex_compiled);
        buffer->highlight_regex_compiled = NULL;
    }

    if (new_regex && new_regex[0])
    {
        buffer->highlight_regex = strdup (new_regex);
        if (buffer->highlight_regex)
        {
            buffer->highlight_regex_compiled =
                malloc (sizeof (*buffer->highlight_regex_compiled));
            if (buffer->highlight_regex_compiled)
            {
                if (string_regcomp (buffer->highlight_regex_compiled,
                                    buffer->highlight_regex,
                                    REG_EXTENDED | REG_ICASE) != 0)
                {
                    free (buffer->highlight_regex_compiled);
                    buffer->highlight_regex_compiled = NULL;
                }
            }
        }
    }
}

/*
 * Sets tags to restrict highlights for a buffer.
 */

void
gui_buffer_set_highlight_tags_restrict (struct t_gui_buffer *buffer,
                                        const char *new_tags)
{
    if (!buffer)
        return;

    if (buffer->highlight_tags_restrict)
    {
        free (buffer->highlight_tags_restrict);
        buffer->highlight_tags_restrict = NULL;
    }
    if (buffer->highlight_tags_restrict_array)
    {
        string_free_split_tags (buffer->highlight_tags_restrict_array);
        buffer->highlight_tags_restrict_array = NULL;
    }
    buffer->highlight_tags_restrict_count = 0;

    if (!new_tags)
        return;

    buffer->highlight_tags_restrict = strdup (new_tags);
    if (!buffer->highlight_tags_restrict)
        return;

    buffer->highlight_tags_restrict_array = string_split_tags (
        buffer->highlight_tags_restrict,
        &buffer->highlight_tags_restrict_count);
}

/*
 * Sets highlight tags for a buffer.
 */

void
gui_buffer_set_highlight_tags (struct t_gui_buffer *buffer,
                               const char *new_tags)
{
    if (!buffer)
        return;

    if (buffer->highlight_tags)
    {
        free (buffer->highlight_tags);
        buffer->highlight_tags = NULL;
    }
    if (buffer->highlight_tags_array)
    {
        string_free_split_tags (buffer->highlight_tags_array);
        buffer->highlight_tags_array = NULL;
    }
    buffer->highlight_tags_count = 0;

    if (!new_tags)
        return;

    buffer->highlight_tags = strdup (new_tags);
    if (!buffer->highlight_tags)
        return;

    buffer->highlight_tags_array = string_split_tags (
        buffer->highlight_tags,
        &buffer->highlight_tags_count);
}

/*
 * Sets hotlist_max_level_nicks for a buffer.
 */

void
gui_buffer_set_hotlist_max_level_nicks (struct t_gui_buffer *buffer,
                                        const char *new_hotlist_max_level_nicks)
{
    char **nicks, *pos, *error;
    int nicks_count, value, i;
    long number;

    if (!buffer)
        return;

    hashtable_remove_all (buffer->hotlist_max_level_nicks);

    if (new_hotlist_max_level_nicks && new_hotlist_max_level_nicks[0])
    {
        nicks = string_split (new_hotlist_max_level_nicks, ",", NULL,
                              WEECHAT_STRING_SPLIT_STRIP_LEFT
                              | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                              | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                              0, &nicks_count);
        if (nicks)
        {
            for (i = 0; i < nicks_count; i++)
            {
                value = -1;
                pos = strchr (nicks[i], ':');
                if (pos)
                {
                    pos[0] = '\0';
                    pos++;
                    error = NULL;
                    number = strtol (pos, &error, 10);
                    if (error && !error[0])
                        value = (int)number;
                }
                hashtable_set (buffer->hotlist_max_level_nicks, nicks[i],
                               &value);
            }
            string_free_split (nicks);
        }
    }
}

/*
 * Adds nicks to hotlist_max_level_nicks for a buffer.
 */

void
gui_buffer_add_hotlist_max_level_nicks (struct t_gui_buffer *buffer,
                                        const char *nicks_to_add)
{
    char **nicks, *pos, *error;
    int nicks_count, value, i;
    long number;

    if (!buffer || !nicks_to_add)
        return;

    nicks = string_split (nicks_to_add, ",", NULL,
                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                          0, &nicks_count);
    if (nicks)
    {
        for (i = 0; i < nicks_count; i++)
        {
            value = -1;
            pos = strchr (nicks[i], ':');
            if (pos)
            {
                pos[0] = '\0';
                pos++;
                error = NULL;
                number = strtol (pos, &error, 10);
                if (error && !error[0])
                    value = (int)number;
            }
            hashtable_set (buffer->hotlist_max_level_nicks, nicks[i],
                           &value);
        }
        string_free_split (nicks);
    }
}

/*
 * Removes nicks from hotlist_max_level_nicks in a buffer.
 */

void
gui_buffer_remove_hotlist_max_level_nicks (struct t_gui_buffer *buffer,
                                           const char *nicks_to_remove)
{
    char **nicks, *pos;
    int nicks_count, i;

    if (!buffer || !nicks_to_remove)
        return;

    nicks = string_split (nicks_to_remove, ",", NULL,
                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                          0, &nicks_count);
    if (nicks)
    {
        for (i = 0; i < nicks_count; i++)
        {
            pos = strchr (nicks[i], ':');
            if (pos)
                pos[0] = '\0';
            hashtable_remove (buffer->hotlist_max_level_nicks, nicks[i]);
        }
        string_free_split (nicks);
    }
}

/*
 * Sets buffer input prompt.
 */

void
gui_buffer_set_input_prompt (struct t_gui_buffer *buffer,
                             const char *input_prompt)
{
    if (!buffer || (string_strcmp (buffer->input_prompt, input_prompt) == 0))
        return;

    free (buffer->input_prompt);
    buffer->input_prompt = (input_prompt && input_prompt[0]) ?
        strdup (input_prompt) : NULL;

    (void) gui_buffer_send_signal (buffer,
                                   "input_prompt_changed",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Sets buffer input.
 */

void
gui_buffer_set_input (struct t_gui_buffer *buffer, const char *input)
{
    if (!buffer || (string_strcmp (buffer->input_buffer, input) == 0))
        return;

    gui_buffer_undo_snap (buffer);
    gui_input_replace_input (buffer, input);
    gui_input_text_changed_modifier_and_signal (buffer,
                                                1, /* save undo */
                                                1); /* stop completion */
}

/*
 * Sets flag "input_get_any_user_data" for a buffer.
 */

void
gui_buffer_set_input_get_any_user_data (struct t_gui_buffer *buffer,
                                        int input_get_any_user_data)
{
    if (!buffer)
        return;

    buffer->input_get_any_user_data = (input_get_any_user_data) ? 1 : 0;
}

/*
 * Sets flag "input_get_unknown_commands" for a buffer.
 */

void
gui_buffer_set_input_get_unknown_commands (struct t_gui_buffer *buffer,
                                           int input_get_unknown_commands)
{
    if (!buffer)
        return;

    buffer->input_get_unknown_commands = (input_get_unknown_commands) ? 1 : 0;

}
/*
 * Sets flag "input_get_empty" for a buffer.
 */

void
gui_buffer_set_input_get_empty (struct t_gui_buffer *buffer,
                                int input_get_empty)
{
    if (!buffer)
        return;

    buffer->input_get_empty = (input_get_empty) ? 1 : 0;
}

/*
 * Sets flag "input_multiline" for a buffer.
 */

void
gui_buffer_set_input_multiline (struct t_gui_buffer *buffer,
                                int input_multiline)
{
    if (!buffer)
        return;

    buffer->input_multiline = (input_multiline) ? 1 : 0;
}

/*
 * Sets unread marker for a buffer.
 *
 * Argument is a string that can be:
 *   - empty string or NULL: set unread marker after the last line
 *   - "0": remove unread marker from buffer
 *   - "-N" or "+N" (N = integer): move the read marker
 *     (negative number: to older line, positive number: to newer line)
 *   - "N" (N = integer but not 0): mark N latest messages as unread
 */

void
gui_buffer_set_unread (struct t_gui_buffer *buffer, const char *argument)
{
    struct t_gui_line *old_last_read_line;
    int i, old_first_line_not_read;
    long number;
    char *error;

    if (!buffer || (buffer->type != GUI_BUFFER_TYPE_FORMATTED))
        return;

    old_last_read_line = buffer->lines->last_read_line;
    old_first_line_not_read = buffer->lines->first_line_not_read;

    if (!argument || !argument[0])
    {
        /* set unread marker after last line */
        buffer->lines->last_read_line = buffer->lines->last_line;
        buffer->lines->first_line_not_read = (buffer->lines->last_read_line) ? 0 : 1;
    }
    else if (strcmp (argument, "0") == 0)
    {
        /* remove unread marker */
        buffer->lines->last_read_line = NULL;
        buffer->lines->first_line_not_read = 0;
    }
    else if (argument[0] == '-')
    {
        /* move the unread marker N lines towards the first line */
        error = NULL;
        number = strtol (argument, &error, 10);
        if (error && !error[0] && (number < 0))
        {
            for (i = 0; i > number; i--)
            {
                if (!buffer->lines->last_read_line)
                {
                    if (!buffer->lines->last_line
                        || buffer->lines->first_line_not_read)
                    {
                        break;
                    }
                    buffer->lines->last_read_line = buffer->lines->last_line;
                }
                if (!buffer->lines->last_read_line->prev_line)
                {
                    buffer->lines->last_read_line = NULL;
                    buffer->lines->first_line_not_read = 1;
                    break;
                }
                buffer->lines->last_read_line = buffer->lines->last_read_line->prev_line;
            }
        }
    }
    else if (argument[0] == '+')
    {
        /* move the unread marker N lines towards the last line */
        error = NULL;
        number = strtol (argument, &error, 10);
        if (error && !error[0] && (number > 0))
        {
            for (i = 0; i < number; i++)
            {
                if (!buffer->lines->last_read_line)
                {
                    if (!buffer->lines->first_line
                        || !buffer->lines->first_line_not_read)
                    {
                        break;
                    }
                    buffer->lines->last_read_line = buffer->lines->first_line;
                    buffer->lines->first_line_not_read = 0;
                    continue;
                }
                buffer->lines->last_read_line = buffer->lines->last_read_line->next_line;
                if (!buffer->lines->last_read_line)
                    break;
            }
        }
    }
    else
    {
        /* move the unread marker N lines from the end towards the first line */
        error = NULL;
        number = strtol (argument, &error, 10);
        if (error && !error[0] && (number > 0))
        {
            buffer->lines->last_read_line = buffer->lines->last_line;
            buffer->lines->first_line_not_read = 0;
            for (i = 0; i < number; i++)
            {
                if (!buffer->lines->last_read_line)
                    break;
                if (!buffer->lines->last_read_line->prev_line)
                {
                    buffer->lines->last_read_line = NULL;
                    buffer->lines->first_line_not_read = 1;
                    break;
                }
                buffer->lines->last_read_line = buffer->lines->last_read_line->prev_line;
            }
        }
    }

    if ((old_last_read_line != buffer->lines->last_read_line
         || old_first_line_not_read != buffer->lines->first_line_not_read))
    {
        gui_buffer_ask_chat_refresh (buffer, 2);
    }
}

/*
 * Sets a buffer property (string).
 */

void
gui_buffer_set (struct t_gui_buffer *buffer, const char *property,
                const char *value)
{
    long number;
    char *error;

    if (!property || !value)
        return;

    /* properties with optional buffer */
    if (strcmp (property, "hotlist") == 0)
    {
        if (strcmp (value, "-") == 0)
            gui_add_hotlist = 0;
        else if (strcmp (value, "+") == 0)
            gui_add_hotlist = 1;
        else if (buffer)
        {
            error = NULL;
            number = strtol (value, &error, 10);
            if (error && !error[0])
            {
                if (number < 0)
                {
                    gui_hotlist_remove_buffer (buffer, 0);
                }
                else
                {
                    (void) gui_hotlist_add (
                        buffer,
                        number,
                        NULL,  /* creation_time */
                        0);  /* check_conditions */
                }
            }
        }
    }
    else if (strcmp (property, "completion_freeze") == 0)
    {
        gui_completion_freeze = (strcmp (value, "1") == 0) ? 1 : 0;
    }

    if (!buffer)
        return;

    /* properties that need a buffer */
    if (strcmp (property, "unread") == 0)
    {
        gui_buffer_set_unread (buffer, value);
    }
    else if (strcmp (property, "display") == 0)
    {
        /*
         * if it is auto-switch to a buffer, then we don't set read marker,
         * otherwise we reset it (if current buffer is not displayed) after
         * switch
         */
        gui_window_switch_to_buffer (
            gui_current_window,
            buffer,
            (value && (strcmp (value, "auto") == 0)) ? 0 : 1);
    }
    else if (strcmp (property, "hidden") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
        {
            if (number)
                gui_buffer_hide (buffer);
            else
                gui_buffer_unhide (buffer);
        }
    }
    else if (strcmp (property, "print_hooks_enabled") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            buffer->print_hooks_enabled = (number) ? 1 : 0;
    }
    else if (strcmp (property, "day_change") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_day_change (buffer, number);
    }
    else if (strcmp (property, "clear") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            buffer->clear = (number) ? 1 : 0;
    }
    else if (strcmp (property, "filter") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_filter (buffer, number);
    }
    else if (strcmp (property, "number") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0] && (number >= 1))
            gui_buffer_move_to_number (buffer, number);
    }
    else if (strcmp (property, "name") == 0)
    {
        gui_buffer_set_name (buffer, value);
    }
    else if (strcmp (property, "short_name") == 0)
    {
        gui_buffer_set_short_name (buffer, value);
    }
    else if (strcmp (property, "type") == 0)
    {
        if (value && (strcmp (value, "formatted") == 0))
            gui_buffer_set_type (buffer, GUI_BUFFER_TYPE_FORMATTED);
        else if (value && (strcmp (value, "free") == 0))
            gui_buffer_set_type (buffer, GUI_BUFFER_TYPE_FREE);
    }
    else if (strcmp (property, "notify") == 0)
    {
        gui_buffer_set_notify (buffer, value);
    }
    else if (strcmp (property, "title") == 0)
    {
        gui_buffer_set_title (buffer, value);
    }
    else if (strcmp (property, "modes") == 0)
    {
        gui_buffer_set_modes (buffer, value);
    }
    else if (strcmp (property, "time_for_each_line") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_time_for_each_line (buffer, number);
    }
    else if (strcmp (property, "nicklist") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist (buffer, number);
    }
    else if (strcmp (property, "nicklist_case_sensitive") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist_case_sensitive (buffer, number);
    }
    else if (strcmp (property, "nicklist_display_groups") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_nicklist_display_groups (buffer, number);
    }
    else if (strcmp (property, "highlight_words") == 0)
    {
        gui_buffer_set_highlight_words (buffer, value);
    }
    else if (strcmp (property, "highlight_words_add") == 0)
    {
        gui_buffer_add_highlight_words (buffer, value);
    }
    else if (strcmp (property, "highlight_words_del") == 0)
    {
        gui_buffer_remove_highlight_words (buffer, value);
    }
    else if (strcmp (property, "highlight_disable_regex") == 0)
    {
        gui_buffer_set_highlight_disable_regex (buffer, value);
    }
    else if (strcmp (property, "highlight_regex") == 0)
    {
        gui_buffer_set_highlight_regex (buffer, value);
    }
    else if (strcmp (property, "highlight_tags_restrict") == 0)
    {
        gui_buffer_set_highlight_tags_restrict (buffer, value);
    }
    else if (strcmp (property, "highlight_tags") == 0)
    {
        gui_buffer_set_highlight_tags (buffer, value);
    }
    else if (strcmp (property, "hotlist_max_level_nicks") == 0)
    {
        gui_buffer_set_hotlist_max_level_nicks (buffer, value);
    }
    else if (strcmp (property, "hotlist_max_level_nicks_add") == 0)
    {
        gui_buffer_add_hotlist_max_level_nicks (buffer, value);
    }
    else if (strcmp (property, "hotlist_max_level_nicks_del") == 0)
    {
        gui_buffer_remove_hotlist_max_level_nicks (buffer, value);
    }
    else if (strncmp (property, "key_bind_", 9) == 0)
    {
        (void) gui_key_bind (buffer, 0, property + 9, value, 0);
    }
    else if (strncmp (property, "key_unbind_", 11) == 0)
    {
        if (strcmp (property + 11, "*") == 0)
        {
            gui_key_free_all (-1, &buffer->keys, &buffer->last_key,
                              &buffer->keys_count, 0);
        }
        else
        {
            gui_key_unbind (buffer, -1, property + 11);
        }
    }
    else if (strcmp (property, "input_prompt") == 0)
    {
        gui_buffer_set_input_prompt (buffer, value);
    }
    else if (strcmp (property, "input") == 0)
    {
        gui_buffer_set_input (buffer, value);
    }
    else if (strcmp (property, "input_pos") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_input_set_pos (buffer, number);
    }
    else if (strcmp (property, "input_get_any_user_data") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_input_get_any_user_data (buffer, number);
    }
    else if (strcmp (property, "input_get_unknown_commands") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_input_get_unknown_commands (buffer, number);
    }
    else if (strcmp (property, "input_get_empty") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_input_get_empty (buffer, number);
    }
    else if (strcmp (property, "input_multiline") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            gui_buffer_set_input_multiline (buffer, number);
    }
    else if (strncmp (property, "localvar_set_", 13) == 0)
    {
        if (value)
            gui_buffer_local_var_add (buffer, property + 13, value);
    }
    else if (strncmp (property, "localvar_del_", 13) == 0)
    {
        gui_buffer_local_var_remove (buffer, property + 13);
    }
}

/*
 * Sets a buffer property (pointer).
 */

void
gui_buffer_set_pointer (struct t_gui_buffer *buffer, const char *property,
                        void *pointer)
{
    if (!buffer || !property)
        return;

    if (strcmp (property, "close_callback") == 0)
    {
        buffer->close_callback = pointer;
    }
    else if (strcmp (property, "close_callback_pointer") == 0)
    {
        buffer->close_callback_pointer = pointer;
    }
    else if (strcmp (property, "close_callback_data") == 0)
    {
        buffer->close_callback_data = pointer;
    }
    else if (strcmp (property, "nickcmp_callback") == 0)
    {
        buffer->nickcmp_callback = pointer;
    }
    else if (strcmp (property, "nickcmp_callback_pointer") == 0)
    {
        buffer->nickcmp_callback_pointer = pointer;
    }
    else if (strcmp (property, "nickcmp_callback_data") == 0)
    {
        buffer->nickcmp_callback_data = pointer;
    }
    else if (strcmp (property, "input_callback") == 0)
    {
        buffer->input_callback = pointer;
    }
    else if (strcmp (property, "input_callback_pointer") == 0)
    {
        buffer->input_callback_pointer = pointer;
    }
    else if (strcmp (property, "input_callback_data") == 0)
    {
        buffer->input_callback_data = pointer;
    }
}

/*
 * Computes "num_displayed" for all buffers.
 */

void
gui_buffer_compute_num_displayed ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_window *ptr_window;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->num_displayed = 0;
    }

    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer)
        {
            for (ptr_buffer = gui_buffers; ptr_buffer;
                 ptr_buffer = ptr_buffer->next_buffer)
            {
                if ((ptr_buffer->number == ptr_window->buffer->number)
                    && (!ptr_buffer->zoomed || (ptr_buffer->active == 2)))
                {
                    ptr_buffer->num_displayed++;
                }
            }
        }
    }
}

/*
 * Adds value to "num_displayed" variable for a buffer (value can be negative).
 */

void
gui_buffer_add_value_num_displayed (struct t_gui_buffer *buffer, int value)
{
    struct t_gui_buffer *ptr_buffer;

    if (!buffer)
        return;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == buffer->number)
        {
            ptr_buffer->num_displayed += value;
            if (ptr_buffer->num_displayed < 0)
                ptr_buffer->num_displayed = 0;
        }
    }
}

/*
 * Checks if a buffer is the WeeChat core buffer.
 *
 * Returns:
 *   1: buffer is WeeChat core buffer
 *   0: buffer is another buffer
 */

int
gui_buffer_is_main (const char *plugin_name, const char *name)
{
    /* if plugin is set and is not "core", then it's NOT main buffer */
    if (!plugin_name || (strcmp (plugin_name, plugin_get_name (NULL)) != 0))
        return 0;

    /* if name is set and is not "weechat", then it's NOT main buffer */
    if (!name || (strcmp (name, GUI_BUFFER_MAIN) != 0))
        return 0;

    /* it's main buffer */
    return 1;
}

/*
 * Gets main buffer (weechat one, created at startup).
 */

struct t_gui_buffer *
gui_buffer_search_main ()
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((!ptr_buffer->plugin)
            && (ptr_buffer->name)
            && (strcmp (ptr_buffer->name, GUI_BUFFER_MAIN) == 0))
            return ptr_buffer;
    }

    /* buffer not found (should never occur!) */
    return NULL;
}

/*
 * Searches for a buffer by id (unique identifier).
 */

struct t_gui_buffer *
gui_buffer_search_by_id (long long id)
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->id == id)
            return ptr_buffer;
    }

    /* buffer not found */
    return NULL;
}

/*
 * Searches for a buffer by full name (example: "irc.libera.#weechat").
 *
 * If full_name starts with "(?i)", the search starts after this string
 * and the search is case insensitive.
 */

struct t_gui_buffer *
gui_buffer_search_by_full_name (const char *full_name)
{
    struct t_gui_buffer *ptr_buffer;
    int case_sensitive;

    if (!full_name)
        return NULL;

    case_sensitive = 1;
    if (strncmp (full_name, "(?i)", 4) == 0)
    {
        case_sensitive = 0;
        full_name += 4;
    }

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->full_name
            && ((case_sensitive
                 && strcmp (ptr_buffer->full_name, full_name) == 0)
                || (!case_sensitive
                    && string_strcasecmp (ptr_buffer->full_name, full_name) == 0)))
        {
            return ptr_buffer;
        }
    }

    /* buffer not found */
    return NULL;
}

/*
 * Searches for a buffer by plugin and name, full name or id.
 *
 * If plugin is "==", the name is used to search by full name.
 * If plugin is "==id", the name is used to search by id (which must be numeric).
 */

struct t_gui_buffer *
gui_buffer_search (const char *plugin, const char *name)
{
    struct t_gui_buffer *ptr_buffer;
    int plugin_match, plugin_case_sensitive, name_case_sensitive;
    long long id;
    char *error;

    if (!name || !name[0])
        return gui_current_window->buffer;

    if (plugin && (strcmp (plugin, "==") == 0))
        return gui_buffer_search_by_full_name (name);

    if (plugin && (strcmp (plugin, "==id") == 0))
    {
        error = NULL;
        id = strtoll (name, &error, 10);
        return (error && !error[0]) ? gui_buffer_search_by_id (id) : NULL;
    }

    plugin_case_sensitive = 1;
    name_case_sensitive = 1;

    if (plugin && (strncmp (plugin, "(?i)", 4) == 0))
    {
        plugin_case_sensitive = 0;
        plugin += 4;
    }
    if (strncmp (name, "(?i)", 4) == 0)
    {
        name_case_sensitive = 0;
        name += 4;
    }

    if (!name[0])
        return gui_current_window->buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->name)
        {
            plugin_match = 1;
            if (plugin && plugin[0])
            {
                if ((plugin_case_sensitive
                     && (strcmp (plugin, gui_buffer_get_plugin_name (ptr_buffer)) != 0))
                    || (!plugin_case_sensitive
                        && (string_strcasecmp (plugin, gui_buffer_get_plugin_name (ptr_buffer)) != 0)))
                {
                    plugin_match = 0;
                }
            }
            if (plugin_match
                && ((name_case_sensitive
                     && (strcmp (ptr_buffer->name, name) == 0))
                    || (!name_case_sensitive
                        && (string_strcasecmp (ptr_buffer->name, name) == 0))))
            {
                return ptr_buffer;
            }
        }
    }

    /* buffer not found */
    return NULL;
}

/*
 * Searches for a buffer by plugin and partial name.
 *
 * If plugin or name starts with "(?i)", the search starts after this string
 * and the search is case insensitive.
 */

struct t_gui_buffer *
gui_buffer_search_by_partial_name (const char *plugin, const char *name)
{
    struct t_gui_buffer *ptr_start_buffer, *ptr_buffer, *buffer_partial_match[3];
    int plugin_case_sensitive, name_case_sensitive, plugin_match, length_name;
    const char *pos;

    if (!name || !name[0])
        return gui_current_window->buffer;

    plugin_case_sensitive = 1;
    name_case_sensitive = 1;

    if (plugin && (strncmp (plugin, "(?i)", 4) == 0))
    {
        plugin_case_sensitive = 0;
        plugin += 4;
    }
    if (strncmp (name, "(?i)", 4) == 0)
    {
        name_case_sensitive = 0;
        name += 4;
    }

    if (!name[0])
        return gui_current_window->buffer;

    /* 0: matches beginning of buffer name, 1: in the middle, 2: the end */
    buffer_partial_match[0] = NULL;
    buffer_partial_match[1] = NULL;
    buffer_partial_match[2] = NULL;

    length_name = strlen (name);

    ptr_buffer = gui_current_window->buffer->next_buffer;
    if (!ptr_buffer)
        ptr_buffer = gui_buffers;
    ptr_start_buffer = ptr_buffer;

    while (ptr_buffer)
    {
        if (ptr_buffer->name)
        {
            plugin_match = 1;
            if (plugin && plugin[0])
            {
                if ((plugin_case_sensitive
                     && (strcmp (plugin, gui_buffer_get_plugin_name (ptr_buffer)) != 0))
                    || (!plugin_case_sensitive
                        && (string_strcasecmp (plugin, gui_buffer_get_plugin_name (ptr_buffer)) != 0)))
                {
                    plugin_match = 0;
                }
            }
            if (plugin_match)
            {
                if (name_case_sensitive)
                    pos = strstr (ptr_buffer->name, name);
                else
                    pos = string_strcasestr (ptr_buffer->name, name);
                if (pos)
                {
                    if (pos == ptr_buffer->name)
                    {
                        if (!pos[length_name])
                        {
                            /* matches full name, return it immediately */
                            return ptr_buffer;
                        }
                        /* matches beginning of buffer name */
                        if (!buffer_partial_match[0])
                            buffer_partial_match[0] = ptr_buffer;
                    }
                    else
                    {
                        if (pos[length_name])
                        {
                            /* matches middle of buffer name */
                            if (!buffer_partial_match[1])
                                buffer_partial_match[1] = ptr_buffer;
                        }
                        else
                        {
                            /* matches end of buffer name */
                            if (!buffer_partial_match[2])
                                buffer_partial_match[2] = ptr_buffer;
                        }
                    }
                }
            }
        }
        ptr_buffer = ptr_buffer->next_buffer;
        if (!ptr_buffer)
            ptr_buffer = gui_buffers;
        if (ptr_buffer == ptr_start_buffer)
            break;
    }

    /* matches end of name? */
    if (buffer_partial_match[2])
        return buffer_partial_match[2];

    /* matches beginning of name? */
    if (buffer_partial_match[0])
        return buffer_partial_match[0];

    /*
     * return buffer partially matching in name
     * (may be NULL if no buffer was found)
     */
    return buffer_partial_match[1];
}

/*
 * Searches for a buffer by number.
 */

struct t_gui_buffer *
gui_buffer_search_by_number (int number)
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == number)
            return ptr_buffer;
    }

    /* buffer not found */
    return NULL;
}

/*
 * Searches for a buffer by number, full name or partial name.
 */

struct t_gui_buffer *
gui_buffer_search_by_number_or_name (const char *string)
{
    struct t_gui_buffer *ptr_buffer;
    long number;
    char *error;

    if (!string)
        return NULL;

    ptr_buffer = NULL;

    error = NULL;
    number = strtol (string, &error, 10);
    if (error && !error[0])
    {
        ptr_buffer = gui_buffer_search_by_number (number);
    }
    else
    {
        ptr_buffer = gui_buffer_search_by_full_name (string);
        if (!ptr_buffer)
            ptr_buffer = gui_buffer_search_by_partial_name (NULL, string);
    }

    return ptr_buffer;
}

/*
 * Searches a range of buffer between "number1" and  "number2" (both are
 * included in the range).
 *
 * Arguments "ptr_first_buffer" and "ptr_last_buffer" must NOT be NULL.
 * They are set with the first buffer with number >= number1 and the last
 * buffer with number <= number2.
 *
 * If "different_numbers" is not NULL, it is set with the number of
 * "different numbers" from number1 to number2 (including these numbers).
 *
 * Note: first/last buffers returned may have different number than
 * number1/number2.
 * For example with buffers:  1 2 5 7 10, if number1 == 3 and number2 == 8,
 * the function will return:
 *   first: pointer to buffer 5 (first merged buffer in case of merged buffers)
 *   last : pointer to buffer 7 (last merged buffer in case of merged buffers)
 */

void
gui_buffer_search_range (int number1, int number2,
                         struct t_gui_buffer **ptr_first_buffer,
                         struct t_gui_buffer **ptr_last_buffer,
                         int *different_numbers)
{
    struct t_gui_buffer *ptr_buffer;
    int prev_number;

    *ptr_first_buffer = NULL;
    *ptr_last_buffer = NULL;
    if (different_numbers)
        *different_numbers = 0;

    if ((number1 < 1) || (number2 < 1) || (number1 > number2))
        return;

    prev_number = -1;
    for (ptr_buffer = gui_buffers;
         ptr_buffer && (ptr_buffer->number <= number2);
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (!*ptr_first_buffer && (ptr_buffer->number >= number1))
            *ptr_first_buffer = ptr_buffer;

        *ptr_last_buffer = ptr_buffer;

        /* count the number of different numbers between number1 & 2 */
        if (different_numbers && *ptr_first_buffer
            && (ptr_buffer->number != prev_number))
        {
            (*different_numbers)++;
        }
        prev_number = ptr_buffer->number;
    }
}

/*
 * Returns number of merged buffers (buffers with same number).
 */

int
gui_buffer_count_merged_buffers (int number)
{
    struct t_gui_buffer *ptr_buffer;
    int count;

    count = 0;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == number)
            count++;
    }

    return count;
}

/*
 * Clears content of buffer.
 */

void
gui_buffer_clear (struct t_gui_buffer *buffer)
{
    if (!buffer)
        return;

    /* remove all lines */
    gui_line_free_all (buffer);

    gui_hotlist_remove_buffer (buffer, 0);

    gui_buffer_ask_chat_refresh (buffer, 2);

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_cleared",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Clears content of all buffers.
 */

void
gui_buffer_clear_all ()
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->clear)
            gui_buffer_clear (ptr_buffer);
    }
}

/*
 * Gets next active buffer (when many buffers are merged).
 *
 * If "allow_hidden_buffer" == 1, an hidden buffer can be returned.
 * Otherwise an hidden buffer is never returned (if all other merged buffers are
 * hidden, then NULL is returned).
 */

struct t_gui_buffer *
gui_buffer_get_next_active_buffer (struct t_gui_buffer *buffer,
                                   int allow_hidden_buffer)
{
    struct t_gui_buffer *ptr_buffer;

    if (!buffer)
        return NULL;

    /* search after buffer */
    for (ptr_buffer = buffer->next_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number != buffer->number)
            break;
        if (allow_hidden_buffer || !ptr_buffer->hidden)
            return ptr_buffer;
    }

    /* search before buffer */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer == buffer)
            break;
        if ((ptr_buffer->number == buffer->number)
            && (allow_hidden_buffer || !ptr_buffer->hidden))
        {
            return ptr_buffer;
        }
    }

    return NULL;
}

/*
 * Gets previous active buffer (when many buffers are merged).
 *
 * If "allow_hidden_buffer" == 1, an hidden buffer can be returned.
 * Otherwise an hidden buffer is never returned (if all other merged buffers are
 * hidden, then NULL is returned).
 */

struct t_gui_buffer *
gui_buffer_get_previous_active_buffer (struct t_gui_buffer *buffer,
                                       int allow_hidden_buffer)
{
    struct t_gui_buffer *ptr_buffer;

    if (!buffer)
        return NULL;

    /* search before buffer */
    for (ptr_buffer = buffer->prev_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->prev_buffer)
    {
        if (ptr_buffer->number != buffer->number)
            break;
        if (allow_hidden_buffer || !ptr_buffer->hidden)
            return ptr_buffer;
    }

    /* search after buffer */
    for (ptr_buffer = last_gui_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->prev_buffer)
    {
        if (ptr_buffer == buffer)
            break;
        if ((ptr_buffer->number == buffer->number)
            && (allow_hidden_buffer || !ptr_buffer->hidden))
        {
            return ptr_buffer;
        }
    }

    return NULL;
}

/*
 * Gets index for previously visited buffer.
 *
 * Returns -1 if there's no previously buffer in history, starting from current
 * index.
 */

int
gui_buffer_visited_get_index_previous ()
{
    if ((gui_buffers_visited_count < 2) || (gui_buffers_visited_index == 0))
        return -1;

    if (gui_buffers_visited_index < 0)
        return gui_buffers_visited_count - 2;
    else
        return gui_buffers_visited_index - 1;
}

/*
 * Gets index for next visited buffer.
 *
 * Returns -1 if there's no next buffer in history, starting from current index.
 */

int
gui_buffer_visited_get_index_next ()
{
    if ((gui_buffers_visited_count < 2)
        || (gui_buffers_visited_index < 0)
        || (gui_buffers_visited_index >= gui_buffers_visited_count - 1))
        return -1;

    return gui_buffers_visited_index + 1;
}

/*
 * Searches for a visited buffer in list of visited buffers.
 */

struct t_gui_buffer_visited *
gui_buffer_visited_search (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer_visited *ptr_buffer_visited;

    if (!buffer)
        return NULL;

    for (ptr_buffer_visited = gui_buffers_visited; ptr_buffer_visited;
         ptr_buffer_visited = ptr_buffer_visited->next_buffer)
    {
        if (ptr_buffer_visited->buffer == buffer)
            return ptr_buffer_visited;
    }

    /* visited buffer not found */
    return NULL;
}

/*
 * Searches for a visited buffer in list of visited buffers (by number).
 */

struct t_gui_buffer_visited *
gui_buffer_visited_search_by_number (int number)
{
    struct t_gui_buffer_visited *ptr_buffer_visited;
    int i;

    if ((number < 0) || (number >= gui_buffers_visited_count))
        return NULL;

    i = 0;
    for (ptr_buffer_visited = gui_buffers_visited; ptr_buffer_visited;
         ptr_buffer_visited = ptr_buffer_visited->next_buffer)
    {
        if (i == number)
            return ptr_buffer_visited;
        i++;
    }

    /* visited buffer not found (should never be reached) */
    return NULL;
}

/*
 * Removes a visited buffer from list of visited buffers.
 */

void
gui_buffer_visited_remove (struct t_gui_buffer_visited *buffer_visited)
{
    if (!buffer_visited)
        return;

    /* remove visited buffer from list */
    if (buffer_visited->prev_buffer)
        (buffer_visited->prev_buffer)->next_buffer = buffer_visited->next_buffer;
    if (buffer_visited->next_buffer)
        (buffer_visited->next_buffer)->prev_buffer = buffer_visited->prev_buffer;
    if (gui_buffers_visited == buffer_visited)
        gui_buffers_visited = buffer_visited->next_buffer;
    if (last_gui_buffer_visited == buffer_visited)
        last_gui_buffer_visited = buffer_visited->prev_buffer;

    free (buffer_visited);

    if (gui_buffers_visited_count > 0)
        gui_buffers_visited_count--;

    if (gui_buffers_visited_index >= gui_buffers_visited_count)
        gui_buffers_visited_index = -1;
}

/*
 * Removes a visited buffer from list of visited buffers.
 */

void
gui_buffer_visited_remove_by_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer_visited *buffer_visited;

    if (!buffer)
        return;

    buffer_visited = gui_buffer_visited_search (buffer);
    if (buffer_visited)
        gui_buffer_visited_remove (buffer_visited);
}

/*
 * Removes all visited buffers from list.
 */

void
gui_buffer_visited_remove_all ()
{
    while (gui_buffers_visited)
    {
        gui_buffer_visited_remove (gui_buffers_visited);
    }
}

/*
 * Adds a visited buffer to list of visited buffers.
 */

struct t_gui_buffer_visited *
gui_buffer_visited_add (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer_visited *new_buffer_visited;

    if (!buffer)
        return NULL;

    new_buffer_visited = gui_buffer_visited_search (buffer);
    if (new_buffer_visited)
        gui_buffer_visited_remove (new_buffer_visited);

    /* remove old buffer(s) visited if list is too long */
    while (gui_buffers_visited_count > CONFIG_INTEGER(config_history_max_visited_buffers))
    {
        gui_buffer_visited_remove (gui_buffers_visited);
    }

    new_buffer_visited = malloc (sizeof (*new_buffer_visited));
    if (new_buffer_visited)
    {
        new_buffer_visited->buffer = buffer;

        new_buffer_visited->prev_buffer = last_gui_buffer_visited;
        new_buffer_visited->next_buffer = NULL;
        if (last_gui_buffer_visited)
            last_gui_buffer_visited->next_buffer = new_buffer_visited;
        else
            gui_buffers_visited = new_buffer_visited;
        last_gui_buffer_visited = new_buffer_visited;

        gui_buffers_visited_count++;
        gui_buffers_visited_index = -1;
    }

    return new_buffer_visited;
}

/*
 * Closes a buffer.
 */

void
gui_buffer_close (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_window;
    struct t_gui_buffer *ptr_buffer, *ptr_back_to_buffer;
    struct t_gui_buffer *ptr_buffer_visited_buffer;
    int index;
    struct t_gui_buffer_visited *ptr_buffer_visited;

    if (!buffer)
        return;

    buffer->closing = 1;

    (void) hook_signal_send ("buffer_closing",
                             WEECHAT_HOOK_SIGNAL_POINTER, buffer);

    if (buffer->close_callback)
    {
        (void)(buffer->close_callback) (buffer->close_callback_pointer,
                                        buffer->close_callback_data,
                                        buffer);
    }

    ptr_back_to_buffer = NULL;

    /* first unmerge buffer if it is merged to at least one other buffer */
    if (gui_buffer_count_merged_buffers (buffer->number) > 1)
    {
        ptr_back_to_buffer = gui_buffer_get_next_active_buffer (buffer, 0);
        if (!ptr_back_to_buffer)
            ptr_back_to_buffer = gui_buffer_get_next_active_buffer (buffer, 1);
        gui_buffer_unmerge (buffer, last_gui_buffer->number + 1);
    }

    if (!weechat_quit)
    {
        /*
         * find other buffer to display: previously visited buffer if current
         * window is displaying buffer, or buffer # - 1
         */
        ptr_buffer_visited_buffer = NULL;
        if (CONFIG_BOOLEAN(config_look_jump_previous_buffer_when_closing)
            && gui_current_window && (gui_current_window->buffer == buffer))
        {
            index = gui_buffer_visited_get_index_previous ();
            if (index >= 0)
            {
                ptr_buffer_visited = gui_buffer_visited_search_by_number (index);
                if (ptr_buffer_visited
                    && (ptr_buffer_visited->buffer != buffer))
                {
                    ptr_buffer_visited_buffer = ptr_buffer_visited->buffer;
                }
            }
        }

        /* switch to another buffer in each window that displays buffer */
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            if ((buffer == ptr_window->buffer) &&
                ((buffer->next_buffer) || (buffer->prev_buffer)))
            {
                /* switch to previous buffer */
                if (gui_buffers != last_gui_buffer)
                {
                    if (ptr_back_to_buffer)
                    {
                        gui_window_switch_to_buffer (ptr_window,
                                                     ptr_back_to_buffer,
                                                     1);
                    }
                    else if (ptr_buffer_visited_buffer)
                    {
                        gui_window_switch_to_buffer (ptr_window,
                                                     ptr_buffer_visited_buffer,
                                                     1);
                    }
                    else if (ptr_window->buffer->prev_buffer)
                    {
                        gui_window_switch_to_buffer (ptr_window,
                                                     ptr_window->buffer->prev_buffer,
                                                     1);
                    }
                    else
                    {
                        gui_window_switch_to_buffer (ptr_window,
                                                     last_gui_buffer,
                                                     1);
                    }
                }
                gui_window_scroll_remove_buffer (ptr_window, buffer);
            }
        }
    }

    gui_hotlist_remove_buffer (buffer, 1);
    free (buffer->hotlist_removed);
    if (gui_hotlist_initial_buffer == buffer)
        gui_hotlist_initial_buffer = NULL;

    gui_buffer_visited_remove_by_buffer (buffer);

    /* compute "number - 1" on next buffers if auto renumber is ON */
    if (CONFIG_BOOLEAN(config_look_buffer_auto_renumber))
    {
        for (ptr_buffer = buffer->next_buffer; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number--;
        }
    }

    /* free all lines */
    gui_line_free_all (buffer);
    free (buffer->own_lines);
    free (buffer->mixed_lines);

    /* free some data */
    gui_buffer_undo_free_all (buffer);
    gui_history_buffer_free (buffer);
    gui_completion_free (buffer->completion);
    gui_nicklist_remove_all (buffer);
    gui_nicklist_remove_group (buffer, buffer->nicklist_root);
    hashtable_free (buffer->hotlist_max_level_nicks);
    gui_key_free_all (-1, &buffer->keys, &buffer->last_key,
                      &buffer->keys_count, 0);
    gui_buffer_local_var_remove_all (buffer);
    hashtable_free (buffer->local_variables);
    free (buffer->plugin_name_for_upgrade);
    free (buffer->name);
    free (buffer->full_name);
    free (buffer->old_full_name);
    free (buffer->short_name);
    free (buffer->title);
    free (buffer->modes);
    free (buffer->input_prompt);
    free (buffer->input_buffer);
    free (buffer->input_undo_snap);
    free (buffer->text_search_input);
    if (buffer->text_search_regex_compiled)
    {
        regfree (buffer->text_search_regex_compiled);
        free (buffer->text_search_regex_compiled);
    }
    free (buffer->highlight_words);
    free (buffer->highlight_disable_regex);
    if (buffer->highlight_disable_regex_compiled)
    {
        regfree (buffer->highlight_disable_regex_compiled);
        free (buffer->highlight_disable_regex_compiled);
    }
    free (buffer->highlight_regex);
    if (buffer->highlight_regex_compiled)
    {
        regfree (buffer->highlight_regex_compiled);
        free (buffer->highlight_regex_compiled);
    }
    free (buffer->highlight_tags_restrict);
    string_free_split_tags (buffer->highlight_tags_restrict_array);
    free (buffer->highlight_tags);
    string_free_split_tags (buffer->highlight_tags_array);
    free (buffer->input_callback_data);
    free (buffer->close_callback_data);
    free (buffer->nickcmp_callback_data);

    /* remove buffer from buffers list */
    if (buffer->prev_buffer)
        (buffer->prev_buffer)->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        (buffer->next_buffer)->prev_buffer = buffer->prev_buffer;
    if (gui_buffers == buffer)
        gui_buffers = buffer->next_buffer;
    if (last_gui_buffer == buffer)
        last_gui_buffer = buffer->prev_buffer;

    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if (ptr_window->buffer == buffer)
            ptr_window->buffer = gui_buffers;
    }

    if (gui_buffer_last_displayed == buffer)
        gui_buffer_last_displayed = NULL;

    if (gui_buffers_count > 0)
        gui_buffers_count--;

    (void) hook_signal_send ("buffer_closed",
                             WEECHAT_HOOK_SIGNAL_POINTER, buffer);

    free (buffer);
}

/*
 * Switches to another buffer with number.
 */

void
gui_buffer_switch_by_number (struct t_gui_window *window, int number)
{
    struct t_gui_buffer *ptr_buffer;

    if (!window || !window->buffer)
        return;

    /* invalid buffer */
    if ((number < 0) || (number == window->buffer->number))
        return;

    /* search for buffer in the list */
    for (ptr_buffer = gui_buffers; ptr_buffer; ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((ptr_buffer != window->buffer) && (number == ptr_buffer->number)
            && ptr_buffer->active)
        {
            gui_window_switch_to_buffer (window, ptr_buffer, 1);
            return;
        }
    }
}

/*
 * Sets active buffer (when many buffers are merged).
 */

void
gui_buffer_set_active_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    int active;

    if (!buffer)
        return;

    active = 1;
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((ptr_buffer->number == buffer->number) && (ptr_buffer->active))
        {
            active = ptr_buffer->active;
            break;
        }
    }

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == buffer->number)
        {
            if (ptr_buffer == buffer)
            {
                if (active == 2)
                    ptr_buffer->lines = ptr_buffer->own_lines;
                ptr_buffer->active = active;
            }
            else
            {
                if (ptr_buffer->active == 2)
                    ptr_buffer->lines = ptr_buffer->mixed_lines;
                ptr_buffer->active = 0;
            }
        }
    }
}

/*
 * Switches active buffer to next buffer (when many buffers are merged).
 */

void
gui_buffer_switch_active_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_window *window;

    ptr_buffer = gui_buffer_get_next_active_buffer (buffer, 0);
    if (ptr_buffer)
    {
        gui_buffer_set_active_buffer (ptr_buffer);
        window = gui_window_search_with_buffer (buffer);
        if (window)
            gui_window_switch_to_buffer (window, ptr_buffer, 1);
    }
}

/*
 * Switches active buffer to previous buffer (when many buffers are merged).
 */

void
gui_buffer_switch_active_buffer_previous (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_window *window;

    ptr_buffer = gui_buffer_get_previous_active_buffer (buffer, 0);
    if (ptr_buffer)
    {
        gui_buffer_set_active_buffer (ptr_buffer);
        window = gui_window_search_with_buffer (buffer);
        if (window)
            gui_window_switch_to_buffer (window, ptr_buffer, 1);
    }
}

/*
 * Zooms on current active merged buffer, or display all merged buffers if zoom
 * was active.
 */

void
gui_buffer_zoom (struct t_gui_buffer *buffer)
{
    struct t_gui_window *ptr_window;
    struct t_gui_buffer *ptr_buffer;
    int buffer_was_zoomed;

    /* do nothing if current buffer is not merged with another buffer */
    if (gui_buffer_count_merged_buffers (buffer->number) < 2)
        return;

    buffer_was_zoomed = (buffer->active == 2);

    /* reset scroll in all windows displaying this buffer number */
    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if ((ptr_window->buffer->number == buffer->number)
            && ptr_window->scroll && ptr_window->scroll->start_line)
        {
            gui_window_scroll_bottom (ptr_window);
        }
    }

    /* first make buffer active if it is not */
    if (!buffer->active)
    {
        gui_buffer_set_active_buffer (buffer);
        ptr_window = gui_window_search_with_buffer (buffer);
        if (ptr_window)
            gui_window_switch_to_buffer (ptr_window, buffer, 1);
    }

    /*
     * toggle active flag between 1 and 2
     * (1 = active with other merged buffers displayed, 2 = the only active)
     */
    if (buffer->active == 1)
    {
        buffer->active = 2;
        buffer->lines = buffer->own_lines;
    }
    else if (buffer->active == 2)
    {
        buffer->active = 1;
        buffer->lines = buffer->mixed_lines;
    }

    /* set "zoomed" in merged buffers */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number > buffer->number)
            break;
        if (ptr_buffer->number == buffer->number)
        {
            ptr_buffer->zoomed = (buffer->active == 2) ? 1 : 0;
        }
    }

    gui_buffer_compute_num_displayed ();

    gui_buffer_ask_chat_refresh (buffer, 2);

    (void) gui_buffer_send_signal (
        buffer,
        (buffer_was_zoomed) ? "buffer_unzoomed" : "buffer_zoomed",
        WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Renumbers buffers with consecutive numbers between the range
 * number1 -> number2, sarting with new number "start_number".
 *
 * If number1 < 1, then renumber starts at first buffer in list (with lowest
 * number).
 * If number2 < 1, then renumber ends at last buffer in list (with highest
 * number).
 * If start_number is < 1, then the numbers will start with the first number
 * seen in range number1-number2.
 *
 * Note: the start_number must be strictly greater than the buffer before
 * "number1".
 *
 * Examples (^^^ == the buffers affected/renumbered):
 *
 *   renumber(4, 10, 4):        1  2  6  8  10  15
 *                         -->  1  2  4  5   6  15
 *                                    ^^^^^^^^
 *
 *   renumber(4, 10, -1):       1  2  6  8  10  15
 *                         -->  1  2  6  7   8  15
 *                                    ^^^^^^^^
 *
 *   renumber(3, -1, 10):       1  2   6   8  10  15
 *                         -->  1  2  10  11  12  13
 *                                    ^^^^^^^^^^^^^^
 *
 *   renumber(-1, -1, -1):      1  2  6  8  10  15
 *                         -->  1  2  3  4   5   6
 *                              ^^^^^^^^^^^^^^^^^^
 *
 *   renumber(-1, -1, 5):       1  2  6  8  10  15
 *                         -->  5  6  7  8  9   10
 *                              ^^^^^^^^^^^^^^^^^^
 */

void
gui_buffer_renumber (int number1, int number2, int start_number)
{
    struct t_gui_buffer *ptr_buffer, *ptr_buffer2, *ptr_buffer_moved;
    struct t_gui_buffer *ptr_first_buffer, *ptr_last_buffer;
    int different_numbers, current_number, number;

    /* if numbers are >= 1, from_number must be <= to_number */
    if ((number1 >= 1) && (number2 >= 1) && (number1 > number2))
        return;

    /* replace negative buffer numbers */
    if (number1 < 1)
        number1 = gui_buffers->number;
    if (number2 < 1)
        number2 = last_gui_buffer->number;

    /* search for first and last buffers affected by renumbering */
    gui_buffer_search_range (number1, number2,
                             &ptr_first_buffer, &ptr_last_buffer,
                             &different_numbers);
    if (!ptr_first_buffer || !ptr_last_buffer)
        return;

    if (start_number < 1)
        start_number = ptr_first_buffer->number;
    else if (start_number > GUI_BUFFER_NUMBER_MAX)
        start_number = GUI_BUFFER_NUMBER_MAX;

    /* the start number must be greater than buffer before first buffer */
    if (ptr_first_buffer->prev_buffer
        && (start_number <= ptr_first_buffer->prev_buffer->number))
    {
        return;
    }

    /*
     * start_number + different_numbers must be lower than buffer after
     * last buffer
     */
    if (ptr_last_buffer->next_buffer
        && (start_number + different_numbers - 1 >= ptr_last_buffer->next_buffer->number))
    {
        return;
    }

    /* let's go for the renumbering! */
    current_number = start_number;
    ptr_buffer = ptr_first_buffer;
    while (ptr_buffer)
    {
        number = ptr_buffer->number;
        ptr_buffer_moved = (number != current_number) ? ptr_buffer : NULL;
        ptr_buffer2 = ptr_buffer;
        while (ptr_buffer && (ptr_buffer->number == number))
        {
            ptr_buffer->number = current_number;
            ptr_buffer2 = ptr_buffer;
            ptr_buffer = ptr_buffer->next_buffer;
        }
        if (ptr_buffer_moved)
        {
            (void) gui_buffer_send_signal (ptr_buffer_moved,
                                           "buffer_moved",
                                           WEECHAT_HOOK_SIGNAL_POINTER,
                                           ptr_buffer_moved);
        }
        ptr_buffer = ptr_buffer2;
        if (ptr_buffer == ptr_last_buffer)
            break;
        ptr_buffer = ptr_buffer->next_buffer;
        current_number++;
    }
}

/*
 * Moves a buffer to another number.
 */

void
gui_buffer_move_to_number (struct t_gui_buffer *buffer, int number)
{
    struct t_gui_buffer *ptr_first_buffer, *ptr_last_buffer, *ptr_buffer;
    struct t_gui_buffer *ptr_buffer_pos;
    int auto_renumber;

    auto_renumber = CONFIG_BOOLEAN(config_look_buffer_auto_renumber);

    if (!buffer)
        return;

    /* nothing to do if auto renumber is ON and that there is only one buffer */
    if (auto_renumber && (gui_buffers == last_gui_buffer))
        return;

    if (number < 1)
        number = 1;

    if (number > GUI_BUFFER_NUMBER_MAX)
        number = GUI_BUFFER_NUMBER_MAX;

    /* buffer number is already OK ? */
    if (number == buffer->number)
        return;

    /* search for first and last buffer to move */
    gui_buffer_search_range (buffer->number, buffer->number,
                             &ptr_first_buffer,
                             &ptr_last_buffer,
                             NULL);
    if (!ptr_first_buffer || !ptr_last_buffer)
        return;

    /*
     * if group of buffers found is all buffers, then we can't move buffers!
     * (allowed only if auto renumber is OFF)
     */
    if (auto_renumber
        && (ptr_first_buffer == gui_buffers)
        && (ptr_last_buffer == last_gui_buffer))
    {
        return;
    }

    /* remove buffer(s) from list */
    if (ptr_first_buffer->prev_buffer)
        (ptr_first_buffer->prev_buffer)->next_buffer = ptr_last_buffer->next_buffer;
    if (ptr_last_buffer->next_buffer)
        (ptr_last_buffer->next_buffer)->prev_buffer = ptr_first_buffer->prev_buffer;
    if (gui_buffers == ptr_first_buffer)
        gui_buffers = ptr_last_buffer->next_buffer;
    if (last_gui_buffer == ptr_last_buffer)
        last_gui_buffer = ptr_first_buffer->prev_buffer;

    /* compute "number - 1" on next buffers if auto renumber is ON */
    if (auto_renumber)
    {
        for (ptr_buffer = ptr_last_buffer->next_buffer; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number--;
        }
    }

    /* search for new position in the list */
    for (ptr_buffer_pos = gui_buffers; ptr_buffer_pos;
         ptr_buffer_pos = ptr_buffer_pos->next_buffer)
    {
        if (ptr_buffer_pos->number >= number)
            break;
    }

    if (ptr_buffer_pos)
    {
        /* insert before buffer found */
        for (ptr_buffer = ptr_first_buffer; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number = number;
            if (ptr_buffer == ptr_last_buffer)
                break;
        }
        ptr_first_buffer->prev_buffer = ptr_buffer_pos->prev_buffer;
        if (!ptr_first_buffer->prev_buffer)
            gui_buffers = ptr_first_buffer;
        ptr_last_buffer->next_buffer = ptr_buffer_pos;
        if (!ptr_last_buffer->next_buffer)
            last_gui_buffer = ptr_last_buffer;
        if (ptr_buffer_pos->prev_buffer)
            (ptr_buffer_pos->prev_buffer)->next_buffer = ptr_first_buffer;
        ptr_buffer_pos->prev_buffer = ptr_last_buffer;

        /* compute "number + 1" on next buffers */
        if (ptr_last_buffer->next_buffer
            && (ptr_last_buffer->next_buffer->number == number))
        {
            gui_buffer_shift_numbers (ptr_last_buffer->next_buffer);
        }
    }
    else
    {
        /* number not found (too big)? => add to the end */
        for (ptr_buffer = ptr_first_buffer; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number = (auto_renumber) ?
                last_gui_buffer->number + 1 : number;
            if (ptr_buffer == ptr_last_buffer)
                break;
        }
        ptr_first_buffer->prev_buffer = last_gui_buffer;
        ptr_last_buffer->next_buffer = NULL;
        if (!gui_buffers)
            gui_buffers = ptr_first_buffer;
        if (last_gui_buffer)
            last_gui_buffer->next_buffer = ptr_first_buffer;
        last_gui_buffer = ptr_last_buffer;
    }

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_moved",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Swaps two buffers (or merged buffers).
 */

void
gui_buffer_swap (int number1, int number2)
{
    struct t_gui_buffer *ptr_buffer, *ptr_first_buffer[2], *ptr_last_buffer[2];
    int number;

    /* swap buffer with itself? nothing to do! */
    if (number1 == number2)
        return;

    /* ensure we have number1 < number2: if not, exchange numbers */
    if (number1 > number2)
    {
        number = number1;
        number1 = number2;
        number2 = number;
    }

    /* search for first and last buffers, for each number */
    gui_buffer_search_range (number1, number1,
                             &ptr_first_buffer[0],
                             &ptr_last_buffer[0],
                             NULL);
    if (!ptr_first_buffer[0] || !ptr_last_buffer[0])
        return;
    gui_buffer_search_range (number2, number2,
                             &ptr_first_buffer[1],
                             &ptr_last_buffer[1],
                             NULL);
    if (!ptr_first_buffer[1] || !ptr_last_buffer[1])
        return;

    /* first set gui_buffers/last_gui_buffers if they are affected by the swap */
    if (gui_buffers == ptr_first_buffer[0])
        gui_buffers = ptr_first_buffer[1];
    if (last_gui_buffer == ptr_last_buffer[1])
        last_gui_buffer = ptr_last_buffer[0];

    /* swap the pointer outside elements in the linked list */
    if (ptr_first_buffer[0]->prev_buffer)
        (ptr_first_buffer[0]->prev_buffer)->next_buffer = ptr_first_buffer[1];
    if (ptr_last_buffer[1]->next_buffer)
        (ptr_last_buffer[1]->next_buffer)->prev_buffer = ptr_last_buffer[0];
    if (ptr_last_buffer[0]->next_buffer == ptr_first_buffer[1])
    {
        /* special case: adjacent buffers */
        ptr_first_buffer[1]->prev_buffer = ptr_first_buffer[0]->prev_buffer;
        ptr_first_buffer[0]->prev_buffer = ptr_last_buffer[1];
        ptr_last_buffer[0]->next_buffer = ptr_last_buffer[1]->next_buffer;
        ptr_last_buffer[1]->next_buffer = ptr_first_buffer[0];
    }
    else
    {
        if (ptr_first_buffer[1]->prev_buffer)
            (ptr_first_buffer[1]->prev_buffer)->next_buffer = ptr_first_buffer[0];
        if (ptr_last_buffer[0]->next_buffer)
            (ptr_last_buffer[0]->next_buffer)->prev_buffer = ptr_last_buffer[1];
        /* swap first[0]->prev <-> first[1]->prev */
        ptr_buffer = ptr_first_buffer[0]->prev_buffer;
        ptr_first_buffer[0]->prev_buffer = ptr_first_buffer[1]->prev_buffer;
        ptr_first_buffer[1]->prev_buffer = ptr_buffer;
        /* swap last[0]->next <-> last[1]->next */
        ptr_buffer = ptr_last_buffer[0]->next_buffer;
        ptr_last_buffer[0]->next_buffer = ptr_last_buffer[1]->next_buffer;
        ptr_last_buffer[1]->next_buffer = ptr_buffer;
    }

    /* swap the numbers inside buffers */
    for (ptr_buffer = ptr_first_buffer[0]; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number = number2;
        if (ptr_buffer == ptr_last_buffer[0])
            break;
    }
    for (ptr_buffer = ptr_first_buffer[1]; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number = number1;
        if (ptr_buffer == ptr_last_buffer[1])
            break;
    }

    /* send signals */
    (void) gui_buffer_send_signal (
        ptr_first_buffer[0],
        "buffer_moved",
        WEECHAT_HOOK_SIGNAL_POINTER, ptr_first_buffer[0]);
    (void) gui_buffer_send_signal (
        ptr_first_buffer[1],
        "buffer_moved",
        WEECHAT_HOOK_SIGNAL_POINTER, ptr_first_buffer[1]);
}

/*
 * Merges a buffer into another buffer.
 */

void
gui_buffer_merge (struct t_gui_buffer *buffer,
                  struct t_gui_buffer *target_buffer)
{
    struct t_gui_buffer *ptr_buffer, *ptr_first_buffer[2], *ptr_last_buffer[2];

    if (!buffer || !target_buffer)
        return;

    /*
     * nothing to do if:
     * - there is only one buffer
     * - buffer and target are the same
     * - numbers of buffers are the same (already merged)
     */
    if ((gui_buffers == last_gui_buffer)
        || (buffer == target_buffer)
        || (buffer->number == target_buffer->number))
    {
        return;
    }

    /* check if current buffer and target buffers have "formatted" content */
    if ((buffer->type != GUI_BUFFER_TYPE_FORMATTED)
        || (target_buffer->type != GUI_BUFFER_TYPE_FORMATTED))
    {
        gui_chat_printf (NULL,
                         _("%sIt is only possible to merge buffers with "
                           "formatted content"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
        return;
    }

    /* search for first and last buffer to merge */
    gui_buffer_search_range (buffer->number, buffer->number,
                             &ptr_first_buffer[0],
                             &ptr_last_buffer[0],
                             NULL);
    if (!ptr_first_buffer[0] || !ptr_last_buffer[0])
        return;

    /* search for first and last target buffer */
    gui_buffer_search_range (target_buffer->number, target_buffer->number,
                             &ptr_first_buffer[1],
                             &ptr_last_buffer[1],
                             NULL);
    if (!ptr_first_buffer[1] || !ptr_last_buffer[1])
        return;

    /* remove buffer(s) to merge from list */
    if (ptr_first_buffer[0]->prev_buffer)
        (ptr_first_buffer[0]->prev_buffer)->next_buffer = ptr_last_buffer[0]->next_buffer;
    if (ptr_last_buffer[0]->next_buffer)
        (ptr_last_buffer[0]->next_buffer)->prev_buffer = ptr_first_buffer[0]->prev_buffer;
    if (gui_buffers == ptr_first_buffer[0])
        gui_buffers = ptr_last_buffer[0]->next_buffer;
    if (last_gui_buffer == ptr_last_buffer[0])
        last_gui_buffer = ptr_first_buffer[0]->prev_buffer;

    /* compute "number - 1" on next buffers if auto renumber is ON */
    if (CONFIG_BOOLEAN(config_look_buffer_auto_renumber))
    {
        for (ptr_buffer = ptr_last_buffer[0]->next_buffer; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            ptr_buffer->number--;
        }
    }

    /* insert new merged buffers after target buffer */
    if (ptr_last_buffer[1] == last_gui_buffer)
        last_gui_buffer = ptr_last_buffer[0];
    if (ptr_last_buffer[1]->next_buffer)
        (ptr_last_buffer[1]->next_buffer)->prev_buffer = ptr_last_buffer[0];
    ptr_last_buffer[0]->next_buffer = ptr_last_buffer[1]->next_buffer;
    ptr_last_buffer[1]->next_buffer = ptr_first_buffer[0];
    ptr_first_buffer[0]->prev_buffer = ptr_last_buffer[1];

    /* change number in new merged buffer(s) */
    for (ptr_buffer = ptr_first_buffer[0]; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        ptr_buffer->number = target_buffer->number;
        if (ptr_buffer == ptr_last_buffer[0])
            break;
    }

    /* mix lines */
    gui_line_mix_buffers (buffer);

    /* set buffer as active in merged buffers group */
    gui_buffer_set_active_buffer (buffer);

    gui_buffer_compute_num_displayed ();

    gui_window_ask_refresh (1);

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_merged",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Unmerges a buffer from group of merged buffers.
 *
 * If number >= 1, then moves buffer to this number, otherwise moves buffer to
 * buffer->number + 1.
 */

void
gui_buffer_unmerge (struct t_gui_buffer *buffer, int number)
{
    int num_merged;
    struct t_gui_buffer *ptr_buffer, *ptr_new_active_buffer;

    if (!buffer)
        return;

    /* nothing to do if there is only one buffer */
    if (gui_buffers == last_gui_buffer)
        return;

    /* nothing to do if buffer is not merged with at least one buffer */
    num_merged = gui_buffer_count_merged_buffers (buffer->number);
    if (num_merged < 2)
        return;

    ptr_new_active_buffer = NULL;

    if (number > GUI_BUFFER_NUMBER_MAX)
        number = GUI_BUFFER_NUMBER_MAX;

    /* by default, we move buffer to buffer->number + 1 */
    if ((number < 1)  || (number == buffer->number))
    {
        number = buffer->number + 1;
    }
    else
    {
        /* if auto renumber is ON, max number is last buffer + 1 */
        if (CONFIG_BOOLEAN(config_look_buffer_auto_renumber)
            && (number > last_gui_buffer->number + 1))
        {
            number = last_gui_buffer->number + 1;
        }
    }

    if (num_merged == 2)
    {
        /* only one buffer will remain, so it will not be merged any more */
        gui_line_mixed_free_all (buffer);
        gui_line_lines_free (buffer->mixed_lines);
        for (ptr_buffer = gui_buffers; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            if (ptr_buffer->number == buffer->number)
            {
                ptr_buffer->active = 1;
                ptr_buffer->mixed_lines = NULL;
                ptr_buffer->lines = ptr_buffer->own_lines;
            }
        }
    }
    else
    {
        /* remove this buffer from mixed_lines, but keep other buffers merged */
        ptr_new_active_buffer = gui_buffer_get_next_active_buffer (buffer, 0);
        if (!ptr_new_active_buffer)
            ptr_new_active_buffer = gui_buffer_get_next_active_buffer (buffer, 1);
        if (ptr_new_active_buffer)
            gui_buffer_set_active_buffer (ptr_new_active_buffer);
        gui_line_mixed_free_buffer (buffer);
        buffer->mixed_lines = NULL;
        buffer->lines = buffer->own_lines;
    }

    /* remove buffer from list */
    if (buffer->prev_buffer)
        (buffer->prev_buffer)->next_buffer = buffer->next_buffer;
    if (buffer->next_buffer)
        (buffer->next_buffer)->prev_buffer = buffer->prev_buffer;
    if (gui_buffers == buffer)
        gui_buffers = buffer->next_buffer;
    if (last_gui_buffer == buffer)
        last_gui_buffer = buffer->prev_buffer;

    /* move buffer */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number >= number)
            break;
    }
    if (ptr_buffer)
    {
        /* insert buffer into the list (before buffer found) */
        buffer->prev_buffer = ptr_buffer->prev_buffer;
        buffer->next_buffer = ptr_buffer;
        if (ptr_buffer->prev_buffer)
            (ptr_buffer->prev_buffer)->next_buffer = buffer;
        else
            gui_buffers = buffer;
        ptr_buffer->prev_buffer = buffer;
    }
    else
    {
        /* add buffer to the end */
        buffer->prev_buffer = last_gui_buffer;
        buffer->next_buffer = NULL;
        last_gui_buffer->next_buffer = buffer;
        last_gui_buffer = buffer;
    }
    buffer->active = 1;
    buffer->number = number;

    /* compute "number + 1" on next buffers */
    if (buffer->next_buffer
        && (buffer->next_buffer->number == number))
    {
        gui_buffer_shift_numbers (buffer->next_buffer);
    }

    gui_buffer_compute_num_displayed ();

    if (ptr_new_active_buffer)
    {
        ptr_new_active_buffer->mixed_lines->prefix_max_length_refresh = 1;
        ptr_new_active_buffer->mixed_lines->buffer_max_length_refresh = 1;
    }

    gui_window_ask_refresh (1);

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_unmerged",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Unmerges all merged buffers.
 */

void
gui_buffer_unmerge_all ()
{
    struct t_gui_buffer *ptr_buffer, *ptr_next_buffer;
    int number;

    ptr_buffer = gui_buffers;
    while (ptr_buffer)
    {
        number = ptr_buffer->number;
        while (gui_buffer_count_merged_buffers (number) > 1)
        {
            ptr_next_buffer = ptr_buffer->next_buffer;
            gui_buffer_unmerge (ptr_buffer, -1);
            ptr_buffer = ptr_next_buffer;
        }
        /* go to the next number */
        while (ptr_buffer && (ptr_buffer->number == number))
        {
            ptr_buffer = ptr_buffer->next_buffer;
        }
    }
}

/*
 * Hides a buffer.
 */

void
gui_buffer_hide (struct t_gui_buffer *buffer)
{
    if (!buffer || buffer->hidden)
        return;

    buffer->hidden = 1;

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_hidden",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Hides all buffers.
 */

void
gui_buffer_hide_all ()
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_buffer_hide (ptr_buffer);
    }
}

/*
 * Unhides a buffer.
 */

void
gui_buffer_unhide (struct t_gui_buffer *buffer)
{
    if (!buffer || !buffer->hidden)
        return;

    buffer->hidden = 0;

    (void) gui_buffer_send_signal (buffer,
                                   "buffer_unhidden",
                                   WEECHAT_HOOK_SIGNAL_POINTER, buffer);
}

/*
 * Unhides all buffers.
 */

void
gui_buffer_unhide_all ()
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_buffer_unhide (ptr_buffer);
    }
}

/*
 * Sorts buffers by layout number.
 */

void
gui_buffer_sort_by_layout_number ()
{
    struct t_gui_buffer *ptr_buffer, *ptr_next_buffer;
    struct t_gui_buffer *extra_buffers, *last_extra_buffer;

    ptr_buffer = gui_buffers;

    gui_buffers = NULL;
    last_gui_buffer = NULL;

    /* list with buffers that are NOT in layout (layout_number == 0) */
    extra_buffers = NULL;
    last_extra_buffer = NULL;

    while (ptr_buffer)
    {
        ptr_next_buffer = ptr_buffer->next_buffer;

        /*
         * add buffer if it is in layout (if not, it's stored in a temporary
         * list and will be added later)
         */
        if (ptr_buffer->layout_number > 0)
        {
            /* insert buffer now in the final list */
            gui_buffer_insert (ptr_buffer);
        }
        else
        {
            /* move the buffer in a temporary list */
            ptr_buffer->prev_buffer = last_extra_buffer;
            ptr_buffer->next_buffer = NULL;
            if (last_extra_buffer)
                last_extra_buffer->next_buffer = ptr_buffer;
            else
                extra_buffers = ptr_buffer;
            last_extra_buffer = ptr_buffer;
        }

        ptr_buffer = ptr_next_buffer;
    }

    /* add buffers that are NOT in layout */
    ptr_buffer = extra_buffers;
    while (ptr_buffer)
    {
        ptr_next_buffer = ptr_buffer->next_buffer;
        gui_buffer_insert (ptr_buffer);
        ptr_buffer = ptr_next_buffer;
    }
}

/*
 * Makes a "snapshot" of buffer input (save content and position).
 */

void
gui_buffer_undo_snap (struct t_gui_buffer *buffer)
{
    if (!buffer)
        return;

    if ((buffer->input_undo_snap)->data)
    {
        free ((buffer->input_undo_snap)->data);
        (buffer->input_undo_snap)->data = NULL;
    }
    (buffer->input_undo_snap)->pos = 0;

    if (CONFIG_INTEGER(config_look_input_undo_max) > 0)
    {
        (buffer->input_undo_snap)->data = (buffer->input_buffer) ?
            strdup (buffer->input_buffer) : NULL;
        (buffer->input_undo_snap)->pos = buffer->input_buffer_pos;
    }
}

/*
 * Frees "snapshot" of buffer input.
 */

void
gui_buffer_undo_snap_free (struct t_gui_buffer *buffer)
{
    if (!buffer)
        return;

    if ((buffer->input_undo_snap)->data)
    {
        free ((buffer->input_undo_snap)->data);
        (buffer->input_undo_snap)->data = NULL;
    }
    (buffer->input_undo_snap)->pos = 0;
}

/*
 * Adds undo in list, with current input buffer + position.
 *
 * If before_undo is not NULL, then adds undo before this undo, otherwise adds
 * to the end of list.
 */

void
gui_buffer_undo_add (struct t_gui_buffer *buffer)
{
    struct t_gui_input_undo *new_undo;

    if (!buffer)
        return;

    /* undo disabled by configuration */
    if (CONFIG_INTEGER(config_look_input_undo_max) == 0)
        goto end;

    /*
     * if nothing has changed since snapshot of input buffer, then do nothing
     * (just discard snapshot)
     */
    if ((buffer->input_undo_snap)->data
        && (strcmp (buffer->input_buffer, (buffer->input_undo_snap)->data) == 0))
    {
        goto end;
    }

    /* max number of undo reached? */
    if ((buffer->input_undo_count > 0)
        && (buffer->input_undo_count >= CONFIG_INTEGER(config_look_input_undo_max) + 1))
    {
        /* remove older undo in buffer (first in list) */
        gui_buffer_undo_free (buffer, buffer->input_undo);
    }

    /* remove all undos after current undo */
    if (buffer->ptr_input_undo)
    {
        while (buffer->ptr_input_undo->next_undo)
        {
            gui_buffer_undo_free (buffer, buffer->ptr_input_undo->next_undo);
        }
    }

    /* if input is the same as current undo, then do not add it */
    if (buffer->ptr_input_undo
        && (buffer->ptr_input_undo)->data
        && (buffer->input_undo_snap)->data
        && (strcmp ((buffer->input_undo_snap)->data, (buffer->ptr_input_undo)->data) == 0))
    {
        goto end;
    }

    new_undo = (struct t_gui_input_undo *)malloc (sizeof (*new_undo));
    if (!new_undo)
        goto end;

    if ((buffer->input_undo_snap)->data)
    {
        new_undo->data = strdup ((buffer->input_undo_snap)->data);
        new_undo->pos = (buffer->input_undo_snap)->pos;
    }
    else
    {
        new_undo->data = (buffer->input_buffer) ?
            strdup (buffer->input_buffer) : NULL;
        new_undo->pos = buffer->input_buffer_pos;
    }

    /* add undo to the list */
    new_undo->prev_undo = buffer->last_input_undo;
    new_undo->next_undo = NULL;
    if (buffer->last_input_undo)
        (buffer->last_input_undo)->next_undo = new_undo;
    else
        buffer->input_undo = new_undo;
    buffer->last_input_undo = new_undo;

    buffer->ptr_input_undo = new_undo;
    buffer->input_undo_count++;

end:
    gui_buffer_undo_snap_free (buffer);
}

/*
 * Frees undo and removes it from list.
 */

void
gui_buffer_undo_free (struct t_gui_buffer *buffer,
                      struct t_gui_input_undo *undo)
{
    if (!buffer || !undo)
        return;

    /* update current undo if needed */
    if (buffer->ptr_input_undo == undo)
    {
        if ((buffer->ptr_input_undo)->next_undo)
            buffer->ptr_input_undo = (buffer->ptr_input_undo)->next_undo;
        else
            buffer->ptr_input_undo = (buffer->ptr_input_undo)->prev_undo;
    }

    /* free data */
    free (undo->data);

    /* remove undo from list */
    if (undo->prev_undo)
        (undo->prev_undo)->next_undo = undo->next_undo;
    if (undo->next_undo)
        (undo->next_undo)->prev_undo = undo->prev_undo;
    if (buffer->input_undo == undo)
        buffer->input_undo = undo->next_undo;
    if (buffer->last_input_undo == undo)
        buffer->last_input_undo = undo->prev_undo;

    free (undo);

    buffer->input_undo_count--;
}

/*
 * Frees all undos of a buffer.
 */

void
gui_buffer_undo_free_all (struct t_gui_buffer *buffer)
{
    if (!buffer)
        return;

    gui_buffer_undo_snap_free (buffer);

    while (buffer->input_undo)
    {
        gui_buffer_undo_free (buffer, buffer->input_undo);
    }
}

/*
 * Moves buffer input content and undo data to another buffer.
 */

void
gui_buffer_input_move_to_buffer (struct t_gui_buffer *from_buffer,
                                 struct t_gui_buffer *to_buffer)
{
    int is_command;

    /*
     * move of input is allowed if:
     * - 2 buffers are different
     * - input_share is not set to "none"
     * - input buffer in first buffer is not empty
     */
    if (!from_buffer || !to_buffer || (from_buffer == to_buffer)
        || (CONFIG_ENUM(config_look_input_share) == CONFIG_LOOK_INPUT_SHARE_NONE)
        || !from_buffer->input_buffer || !from_buffer->input_buffer[0])
        return;

    /*
     * if input is command and that only text is allowed,
     * or if input is text and that only command is allowed,
     * then do nothing
     */
    is_command = (string_input_for_buffer (from_buffer->input_buffer) == NULL) ? 1 : 0;
    if ((is_command && (CONFIG_ENUM(config_look_input_share) == CONFIG_LOOK_INPUT_SHARE_TEXT))
        || (!is_command && (CONFIG_ENUM(config_look_input_share) == CONFIG_LOOK_INPUT_SHARE_COMMANDS)))
        return;

    /*
     * if overwrite is off and that input of target buffer is not empty,
     * then do nothing
     */
    if ((!CONFIG_BOOLEAN(config_look_input_share_overwrite))
        && to_buffer->input_buffer && to_buffer->input_buffer[0])
        return;

    /* move input_buffer */
    free (to_buffer->input_buffer);
    to_buffer->input_buffer = from_buffer->input_buffer;
    to_buffer->input_buffer_alloc = from_buffer->input_buffer_alloc;
    to_buffer->input_buffer_size = from_buffer->input_buffer_size;
    to_buffer->input_buffer_length = from_buffer->input_buffer_length;
    to_buffer->input_buffer_pos = from_buffer->input_buffer_pos;
    to_buffer->input_buffer_1st_display = from_buffer->input_buffer_1st_display;
    gui_buffer_input_buffer_init (from_buffer);

    /* move undo data */
    gui_buffer_undo_free_all (to_buffer);
    (to_buffer->input_undo_snap)->data = (from_buffer->input_undo_snap)->data;
    (to_buffer->input_undo_snap)->pos = (from_buffer->input_undo_snap)->pos;
    to_buffer->input_undo = from_buffer->input_undo;
    to_buffer->last_input_undo = from_buffer->last_input_undo;
    to_buffer->ptr_input_undo = from_buffer->ptr_input_undo;
    to_buffer->input_undo_count = from_buffer->input_undo_count;
    (from_buffer->input_undo_snap)->data = NULL;
    (from_buffer->input_undo_snap)->pos = 0;
    from_buffer->input_undo = NULL;
    from_buffer->last_input_undo = NULL;
    from_buffer->ptr_input_undo = NULL;
    from_buffer->input_undo_count = 0;

    gui_completion_stop (from_buffer->completion);
}

/*
 * Jumps to buffer with activity.
 */

void
gui_buffer_jump_smart (struct t_gui_window *window)
{
    int scroll_to_bottom;

    if (!window)
        return;

    scroll_to_bottom = 0;

    if (gui_hotlist)
    {
        if (!gui_hotlist_initial_buffer)
            gui_hotlist_initial_buffer = window->buffer;
        gui_window_switch_to_buffer (window, gui_hotlist->buffer, 1);
        gui_hotlist_remove_buffer (window->buffer, 0);
        scroll_to_bottom = 1;
    }
    else
    {
        if (gui_hotlist_initial_buffer)
        {
            if (CONFIG_BOOLEAN(config_look_jump_smart_back_to_buffer))
            {
                gui_window_switch_to_buffer (window,
                                             gui_hotlist_initial_buffer, 1);
                scroll_to_bottom = 1;
            }
            gui_hotlist_initial_buffer = NULL;
        }
        else
        {
            gui_hotlist_initial_buffer = NULL;
        }
    }

    /*
     * scroll to bottom if window was scrolled (except if scrolled
     * beyond the end)
     */
    if (scroll_to_bottom
        && window->scroll
        && window->scroll->start_line
        && (window->scroll->start_line_pos >= 0))
    {
        gui_window_scroll_bottom (window);
    }
}

/*
 * Jumps to last buffer that is visible (not hidden).
 */

void
gui_buffer_jump_last_visible_number (struct t_gui_window *window)
{
    struct t_gui_buffer *ptr_buffer;
    int number;

    if (!window)
        return;

    /* search last non-hidden buffer */
    for (ptr_buffer = last_gui_buffer; ptr_buffer;
         ptr_buffer = ptr_buffer->prev_buffer)
    {
        if (!ptr_buffer->hidden)
            break;
    }

    number = (ptr_buffer) ?
        ptr_buffer->number : ((last_gui_buffer) ? last_gui_buffer->number : -1);
    if (number > 0)
        gui_buffer_switch_by_number (gui_current_window, number);
}

/*
 * Jumps to last buffer displayed (before last jump to a buffer).
 */

void
gui_buffer_jump_last_buffer_displayed (struct t_gui_window *window)
{
    if (window && gui_buffer_last_displayed)
    {
        gui_buffer_switch_by_number (window,
                                     gui_buffer_last_displayed->number);
    }
}

/*
 * Jumps to a visited buffer by index (called by functions to jump to
 * previously / next visited buffer).
 */

void
gui_buffer_jump_visited_by_index (struct t_gui_window *window, int index)
{
    struct t_gui_buffer_visited *ptr_buffer_visited;

    if (!window || (index < 0))
        return;

    gui_buffers_visited_index = index;

    ptr_buffer_visited =
        gui_buffer_visited_search_by_number (gui_buffers_visited_index);
    if (ptr_buffer_visited)
    {
        gui_buffers_visited_frozen = 1;
        gui_buffer_switch_by_number (window,
                                     ptr_buffer_visited->buffer->number);
        gui_buffers_visited_frozen = 0;
    }
}

/*
 * Jumps to previously visited buffer (buffer displayed before current one).
 */

void
gui_buffer_jump_previously_visited_buffer (struct t_gui_window *window)
{
    if (!window)
        return;

    gui_buffer_jump_visited_by_index (
        window,
        gui_buffer_visited_get_index_previous ());
}

/*
 * Jumps to next visited buffer (buffer displayed after current one).
 */

void
gui_buffer_jump_next_visited_buffer (struct t_gui_window *window)
{
    if (!window)
        return;

    gui_buffer_jump_visited_by_index (
        window,
        gui_buffer_visited_get_index_next ());
}

/*
 * Returns hdata for buffer.
 */

struct t_hdata *
gui_buffer_hdata_buffer_cb (const void *pointer, void *data,
                            const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_buffer", "next_buffer",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_buffer, id, LONGLONG, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, opening, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, plugin, POINTER, 0, NULL, "plugin");
        HDATA_VAR(struct t_gui_buffer, plugin_name_for_upgrade, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, number, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, layout_number, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, layout_number_merge_order, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, full_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, old_full_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, short_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, type, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, notify, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, num_displayed, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, active, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, hidden, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, zoomed, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, print_hooks_enabled, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, day_change, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, clear, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, filter, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, close_callback, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, close_callback_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, close_callback_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, closing, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, title, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, modes, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, own_lines, POINTER, 0, NULL, "lines");
        HDATA_VAR(struct t_gui_buffer, mixed_lines, POINTER, 0, NULL, "lines");
        HDATA_VAR(struct t_gui_buffer, lines, POINTER, 0, NULL, "lines");
        HDATA_VAR(struct t_gui_buffer, next_line_id, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, time_for_each_line, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, chat_refresh_needed, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_case_sensitive, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_root, POINTER, 0, NULL, "nick_group");
        HDATA_VAR(struct t_gui_buffer, nicklist_max_length, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_display_groups, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_visible_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_groups_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_groups_visible_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_nicks_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_nicks_visible_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nicklist_last_id_assigned, LONGLONG, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nickcmp_callback, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nickcmp_callback_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, nickcmp_callback_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_callback, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_callback_pointer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_callback_data, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_get_any_user_data, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_get_unknown_commands, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_get_empty, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_multiline, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_prompt, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_buffer, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_buffer_alloc, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_buffer_size, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_buffer_length, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_buffer_pos, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_buffer_1st_display, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, input_undo_snap, POINTER, 0, NULL, "input_undo");
        HDATA_VAR(struct t_gui_buffer, input_undo, POINTER, 0, NULL, "input_undo");
        HDATA_VAR(struct t_gui_buffer, last_input_undo, POINTER, 0, NULL, "input_undo");
        HDATA_VAR(struct t_gui_buffer, ptr_input_undo, POINTER, 0, NULL, "input_undo");
        HDATA_VAR(struct t_gui_buffer, input_undo_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, completion, POINTER, 0, NULL, "completion");
        HDATA_VAR(struct t_gui_buffer, history, POINTER, 0, NULL, "history");
        HDATA_VAR(struct t_gui_buffer, last_history, POINTER, 0, NULL, "history");
        HDATA_VAR(struct t_gui_buffer, ptr_history, POINTER, 0, NULL, "history");
        HDATA_VAR(struct t_gui_buffer, num_history, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, text_search, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, text_search_direction, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, text_search_exact, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, text_search_regex, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, text_search_regex_compiled, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, text_search_where, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, text_search_history, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, text_search_found, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, text_search_ptr_history, POINTER, 0, NULL, "history");
        HDATA_VAR(struct t_gui_buffer, text_search_input, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_words, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_disable_regex, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_disable_regex_compiled, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_regex, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_regex_compiled, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_tags_restrict, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_tags_restrict_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_tags_restrict_array, POINTER, 0, "*,highlight_tags_restrict_count", NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_tags, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_tags_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, highlight_tags_array, POINTER, 0, "*,highlight_tags_count", NULL);
        HDATA_VAR(struct t_gui_buffer, hotlist, POINTER, 0, NULL, "hotlist");
        HDATA_VAR(struct t_gui_buffer, hotlist_max_level_nicks, HASHTABLE, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, keys, POINTER, 0, NULL, "key");
        HDATA_VAR(struct t_gui_buffer, last_key, POINTER, 0, NULL, "key");
        HDATA_VAR(struct t_gui_buffer, keys_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, local_variables, HASHTABLE, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_buffer, prev_buffer, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_buffer, next_buffer, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(gui_buffers, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_gui_buffer, 0);
        HDATA_LIST(gui_buffer_last_displayed, 0);
    }
    return hdata;
}

/*
 * Returns hdata for input undo.
 */

struct t_hdata *
gui_buffer_hdata_input_undo_cb (const void *pointer, void *data,
                                const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_undo", "next_undo",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_input_undo, data, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_input_undo, pos, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_input_undo, prev_undo, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_input_undo, next_undo, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Returns hdata for buffer visited.
 */

struct t_hdata *
gui_buffer_hdata_buffer_visited_cb (const void *pointer, void *data,
                                    const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_buffer", "next_buffer",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_buffer_visited, buffer, POINTER, 0, NULL, "buffer");
        HDATA_VAR(struct t_gui_buffer_visited, prev_buffer, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_buffer_visited, next_buffer, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(gui_buffers_visited, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_gui_buffer_visited, 0);
    }
    return hdata;
}

/*
 * Adds a buffer in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_buffer_add_to_infolist (struct t_infolist *infolist,
                            struct t_gui_buffer *buffer)
{
    struct t_infolist_item *ptr_item;
    struct t_gui_key *ptr_key;
    char option_name[64], str_value[64];
    int i;

    if (!infolist || !buffer)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_pointer (ptr_item, "pointer", buffer))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "current_buffer",
                                   (gui_current_window->buffer == buffer) ? 1 : 0))
        return 0;
    snprintf (str_value, sizeof (str_value), "%lld", buffer->id);
    if (!infolist_new_var_string (ptr_item, "id", str_value))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "opening", buffer->opening))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "plugin", buffer->plugin))
        return 0;
    if (!infolist_new_var_string (ptr_item, "plugin_name",
                                  gui_buffer_get_plugin_name (buffer)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "number", buffer->number))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "layout_number", buffer->layout_number))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "layout_number_merge_order", buffer->layout_number_merge_order))
        return 0;
    if (!infolist_new_var_string (ptr_item, "name", buffer->name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "full_name", buffer->full_name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "old_full_name", buffer->old_full_name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "short_name", gui_buffer_get_short_name (buffer)))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "type", buffer->type))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "notify", buffer->notify))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "num_displayed", buffer->num_displayed))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "active", buffer->active))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "hidden", buffer->hidden))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "zoomed", buffer->zoomed))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "print_hooks_enabled", buffer->print_hooks_enabled))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "day_change", buffer->day_change))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "clear", buffer->clear))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "filter", buffer->filter))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "closing", buffer->closing))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "first_line_not_read", buffer->lines->first_line_not_read))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "lines_hidden", buffer->lines->lines_hidden))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "prefix_max_length", buffer->lines->prefix_max_length))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "next_line_id", buffer->next_line_id))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "time_for_each_line", buffer->time_for_each_line))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_case_sensitive", buffer->nicklist_case_sensitive))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_display_groups", buffer->nicklist_display_groups))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_max_length", buffer->nicklist_max_length))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_count", buffer->nicklist_count))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_visible_count", buffer->nicklist_visible_count))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_groups_count", buffer->nicklist_groups_count))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_groups_visible_count", buffer->nicklist_groups_visible_count))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_nicks_count", buffer->nicklist_nicks_count))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "nicklist_nicks_visible_count", buffer->nicklist_nicks_visible_count))
        return 0;
    snprintf (str_value, sizeof (str_value), "%lld", buffer->nicklist_last_id_assigned);
    if (!infolist_new_var_string (ptr_item, "nicklist_last_id_assigned", str_value))
        return 0;
    if (!infolist_new_var_string (ptr_item, "title", buffer->title))
        return 0;
    if (!infolist_new_var_string (ptr_item, "modes", buffer->modes))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input", buffer->input))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_get_any_user_data", buffer->input_get_any_user_data))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_get_unknown_commands", buffer->input_get_unknown_commands))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_get_empty", buffer->input_get_empty))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_multiline", buffer->input_multiline))
        return 0;
    if (!infolist_new_var_string (ptr_item, "input_prompt", buffer->input_prompt))
        return 0;
    if (!infolist_new_var_string (ptr_item, "input_buffer", buffer->input_buffer))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_alloc", buffer->input_buffer_alloc))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_size", buffer->input_buffer_size))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_length", buffer->input_buffer_length))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_pos", buffer->input_buffer_pos))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "input_buffer_1st_display", buffer->input_buffer_1st_display))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "num_history", buffer->num_history))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search", buffer->text_search))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search_direction", buffer->text_search_direction))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search_exact", buffer->text_search_exact))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search_regex", buffer->text_search_regex))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "text_search_regex_compiled", buffer->text_search_regex_compiled))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search_where", buffer->text_search_where))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search_history", buffer->text_search_history))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "text_search_found", buffer->text_search_found))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "text_search_ptr_history", buffer->text_search_ptr_history))
        return 0;
    if (!infolist_new_var_string (ptr_item, "text_search_input", buffer->text_search_input))
        return 0;
    if (!infolist_new_var_string (ptr_item, "highlight_words", buffer->highlight_words))
        return 0;
    if (!infolist_new_var_string (ptr_item, "highlight_disable_regex", buffer->highlight_disable_regex))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "highlight_disable_regex_compiled", buffer->highlight_disable_regex_compiled))
        return 0;
    if (!infolist_new_var_string (ptr_item, "highlight_regex", buffer->highlight_regex))
        return 0;
    if (!infolist_new_var_pointer (ptr_item, "highlight_regex_compiled", buffer->highlight_regex_compiled))
        return 0;
    if (!infolist_new_var_string (ptr_item, "highlight_tags_restrict", buffer->highlight_tags_restrict))
        return 0;
    if (!infolist_new_var_string (ptr_item, "highlight_tags", buffer->highlight_tags))
        return 0;
    if (!infolist_new_var_string (ptr_item, "hotlist_max_level_nicks", hashtable_get_string (buffer->hotlist_max_level_nicks, "keys_values")))
        return 0;
    i = 0;
    for (ptr_key = buffer->keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        snprintf (option_name, sizeof (option_name), "key_%05d", i);
        if (!infolist_new_var_string (ptr_item, option_name, ptr_key->key))
            return 0;
        snprintf (option_name, sizeof (option_name), "key_command_%05d", i);
        if (!infolist_new_var_string (ptr_item, option_name, ptr_key->command))
            return 0;
        i++;
    }
    if (!infolist_new_var_integer (ptr_item, "keys_count", buffer->keys_count))
        return 0;
    if (!hashtable_add_to_infolist (buffer->local_variables, ptr_item, "localvar"))
        return 0;

    return 1;
}

/*
 * Dumps content of buffer as hexa data in WeeChat log file.
 */

void
gui_buffer_dump_hexa (struct t_gui_buffer *buffer)
{
    struct t_gui_line *ptr_line;
    int num_line;
    char *prefix_without_colors, *message_without_colors, *tags;
    char buf[256];

    log_printf ("[buffer dump hexa (addr:%p)]", buffer);
    num_line = 1;
    for (ptr_line = buffer->lines->first_line; ptr_line;
         ptr_line = ptr_line->next_line)
    {
        /* display line without colors */
        prefix_without_colors = (ptr_line->data->prefix) ?
            gui_color_decode (ptr_line->data->prefix, NULL) : NULL;
        message_without_colors = (ptr_line->data->message) ?
            gui_color_decode (ptr_line->data->message, NULL) : NULL;
        log_printf ("");
        log_printf ("  line %d: %s | %s",
                    num_line,
                    (prefix_without_colors) ? prefix_without_colors : "(null)",
                    (message_without_colors) ? message_without_colors : "(null)");
        free (prefix_without_colors);
        free (message_without_colors);
        tags = string_rebuild_split_string ((const char **)ptr_line->data->tags_array,
                                            ",",
                                            0, -1);
        log_printf ("  tags: '%s', displayed: %d, highlight: %d",
                    (tags) ? tags : "(none)",
                    ptr_line->data->displayed,
                    ptr_line->data->highlight);
        free (tags);
        snprintf (buf, sizeof (buf), "%s", ctime (&ptr_line->data->date));
        buf[strlen (buf) - 1] = '\0';
        log_printf ("  date: %lld = %s",
                    (long long)(ptr_line->data->date), buf);
        log_printf ("  date_usec: %d", ptr_line->data->date_usec);
        snprintf (buf, sizeof (buf), "%s", ctime (&ptr_line->data->date_printed));
        buf[strlen (buf) - 1] = '\0';
        log_printf ("  date_printed: %lld = %s",
                    (long long)ptr_line->data->date_printed, buf);
        log_printf ("  date_usec_printed: %d", ptr_line->data->date_usec_printed);

        /* display raw message for line */
        if (ptr_line->data->message)
        {
            log_printf ("");
            if (ptr_line->data->prefix && ptr_line->data->prefix[0])
            {
                log_printf ("  raw prefix for line %d:", num_line);
                log_printf_hexa ("    ", ptr_line->data->prefix);
            }
            else
            {
                log_printf ("  no prefix for line %d", num_line);
            }
            if (ptr_line->data->message && ptr_line->data->message[0])
            {
                log_printf ("  raw message for line %d:", num_line);
                log_printf_hexa ("    ", ptr_line->data->message);
            }
            else
            {
                log_printf ("  no message for line %d", num_line);
            }
        }

        num_line++;
    }
}

/*
 * Prints buffer infos in WeeChat log file (usually for crash dump).
 */

void
gui_buffer_print_log ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_line *ptr_line;
    struct t_gui_buffer_visited *ptr_buffer_visited;
    struct t_gui_input_undo *ptr_undo;
    char *tags;
    int num;

    log_printf ("");
    log_printf ("gui_buffers . . . . . . . . . : %p", gui_buffers);
    log_printf ("last_gui_buffer . . . . . . . : %p", last_gui_buffer);
    log_printf ("gui_buffers_count . . . . . . : %d", gui_buffers_count);
    log_printf ("gui_buffers_visited . . . . . : %p", gui_buffers_visited);
    log_printf ("last_gui_buffer_visited . . . : %p", last_gui_buffer_visited);
    log_printf ("gui_buffers_visited_index . . : %d", gui_buffers_visited_index);
    log_printf ("gui_buffers_visited_count . . : %d", gui_buffers_visited_count);
    log_printf ("gui_buffers_visited_frozen. . : %d", gui_buffers_visited_frozen);
    log_printf ("gui_buffer_last_displayed . . : %p", gui_buffer_last_displayed);

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        log_printf ("");
        log_printf ("[buffer (addr:%p)]", ptr_buffer);
        log_printf ("  id. . . . . . . . . . . : %lld", ptr_buffer->id);
        log_printf ("  opening . . . . . . . . : %d", ptr_buffer->opening);
        log_printf ("  plugin. . . . . . . . . : %p ('%s')",
                    ptr_buffer->plugin, gui_buffer_get_plugin_name (ptr_buffer));
        log_printf ("  plugin_name_for_upgrade : '%s'", ptr_buffer->plugin_name_for_upgrade);
        log_printf ("  number. . . . . . . . . : %d", ptr_buffer->number);
        log_printf ("  layout_number . . . . . : %d", ptr_buffer->layout_number);
        log_printf ("  layout_number_merge_order: %d", ptr_buffer->layout_number_merge_order);
        log_printf ("  name. . . . . . . . . . : '%s'", ptr_buffer->name);
        log_printf ("  full_name . . . . . . . : '%s'", ptr_buffer->full_name);
        log_printf ("  old_full_name . . . . . : '%s'", ptr_buffer->old_full_name);
        log_printf ("  short_name. . . . . . . : '%s'", ptr_buffer->short_name);
        log_printf ("  type. . . . . . . . . . : %d", ptr_buffer->type);
        log_printf ("  notify. . . . . . . . . : %d", ptr_buffer->notify);
        log_printf ("  num_displayed . . . . . : %d", ptr_buffer->num_displayed);
        log_printf ("  active. . . . . . . . . : %d", ptr_buffer->active);
        log_printf ("  hidden. . . . . . . . . : %d", ptr_buffer->hidden);
        log_printf ("  zoomed. . . . . . . . . : %d", ptr_buffer->zoomed);
        log_printf ("  print_hooks_enabled . . : %d", ptr_buffer->print_hooks_enabled);
        log_printf ("  day_change. . . . . . . : %d", ptr_buffer->day_change);
        log_printf ("  clear . . . . . . . . . : %d", ptr_buffer->clear);
        log_printf ("  filter. . . . . . . . . : %d", ptr_buffer->filter);
        log_printf ("  close_callback. . . . . : %p", ptr_buffer->close_callback);
        log_printf ("  close_callback_pointer. : %p", ptr_buffer->close_callback_pointer);
        log_printf ("  close_callback_data . . : %p", ptr_buffer->close_callback_data);
        log_printf ("  closing . . . . . . . . : %d", ptr_buffer->closing);
        log_printf ("  title . . . . . . . . . : '%s'", ptr_buffer->title);
        log_printf ("  modes . . . . . . . . . : '%s'", ptr_buffer->modes);
        log_printf ("  own_lines . . . . . . . : %p", ptr_buffer->own_lines);
        gui_lines_print_log (ptr_buffer->own_lines);
        log_printf ("  mixed_lines . . . . . . : %p", ptr_buffer->mixed_lines);
        gui_lines_print_log (ptr_buffer->mixed_lines);
        log_printf ("  lines . . . . . . . . . : %p", ptr_buffer->lines);
        log_printf ("  next_line_id. . . . . . : %d", ptr_buffer->next_line_id);
        log_printf ("  time_for_each_line. . . : %d", ptr_buffer->time_for_each_line);
        log_printf ("  chat_refresh_needed . . : %d", ptr_buffer->chat_refresh_needed);
        log_printf ("  nicklist. . . . . . . . : %d", ptr_buffer->nicklist);
        log_printf ("  nicklist_case_sensitive : %d", ptr_buffer->nicklist_case_sensitive);
        log_printf ("  nicklist_root . . . . . : %p", ptr_buffer->nicklist_root);
        log_printf ("  nicklist_max_length . . : %d", ptr_buffer->nicklist_max_length);
        log_printf ("  nicklist_display_groups : %d", ptr_buffer->nicklist_display_groups);
        log_printf ("  nicklist_count. . . . . : %d", ptr_buffer->nicklist_count);
        log_printf ("  nicklist_visible_count. : %d", ptr_buffer->nicklist_visible_count);
        log_printf ("  nicklist_groups_count . : %d", ptr_buffer->nicklist_groups_count);
        log_printf ("  nicklist_groups_vis_cnt : %d", ptr_buffer->nicklist_groups_visible_count);
        log_printf ("  nicklist_nicks_count. . : %d", ptr_buffer->nicklist_nicks_count);
        log_printf ("  nicklist_nicks_vis_cnt. : %d", ptr_buffer->nicklist_nicks_visible_count);
        log_printf ("  nicklist_last_id_assigned: %lld", ptr_buffer->nicklist_last_id_assigned);
        log_printf ("  nickcmp_callback. . . . : %p", ptr_buffer->nickcmp_callback);
        log_printf ("  nickcmp_callback_pointer: %p", ptr_buffer->nickcmp_callback_pointer);
        log_printf ("  nickcmp_callback_data . : %p", ptr_buffer->nickcmp_callback_data);
        log_printf ("  input . . . . . . . . . : %d", ptr_buffer->input);
        log_printf ("  input_callback. . . . . : %p", ptr_buffer->input_callback);
        log_printf ("  input_callback_pointer. : %p", ptr_buffer->input_callback_pointer);
        log_printf ("  input_callback_data . . : %p", ptr_buffer->input_callback_data);
        log_printf ("  input_get_any_user_data : %d", ptr_buffer->input_get_any_user_data);
        log_printf ("  input_get_unknown_cmd . : %d", ptr_buffer->input_get_unknown_commands);
        log_printf ("  input_get_empty . . . . : %d", ptr_buffer->input_get_empty);
        log_printf ("  input_multiline . . . . : %d", ptr_buffer->input_multiline);
        log_printf ("  input_prompt. . . . . . : '%s'", ptr_buffer->input_prompt);
        log_printf ("  input_buffer. . . . . . : '%s'", ptr_buffer->input_buffer);
        log_printf ("  input_buffer_alloc. . . : %d", ptr_buffer->input_buffer_alloc);
        log_printf ("  input_buffer_size . . . : %d", ptr_buffer->input_buffer_size);
        log_printf ("  input_buffer_length . . : %d", ptr_buffer->input_buffer_length);
        log_printf ("  input_buffer_pos. . . . : %d", ptr_buffer->input_buffer_pos);
        log_printf ("  input_buffer_1st_disp . : %d", ptr_buffer->input_buffer_1st_display);
        log_printf ("  input_undo_snap->data . : '%s'", (ptr_buffer->input_undo_snap)->data);
        log_printf ("  input_undo_snap->pos. . : %d", (ptr_buffer->input_undo_snap)->pos);
        log_printf ("  input_undo. . . . . . . : %p", ptr_buffer->input_undo);
        log_printf ("  last_input_undo . . . . : %p", ptr_buffer->last_input_undo);
        log_printf ("  ptr_input_undo. . . . . : %p", ptr_buffer->ptr_input_undo);
        log_printf ("  input_undo_count. . . . : %d", ptr_buffer->input_undo_count);
        num = 0;
        for (ptr_undo = ptr_buffer->input_undo; ptr_undo;
             ptr_undo = ptr_undo->next_undo)
        {
            log_printf ("    undo[%04d]. . . . . . : %p ('%s' / %d)",
                        num, ptr_undo, ptr_undo->data, ptr_undo->pos);
            num++;
        }
        log_printf ("  completion. . . . . . . . . . . : %p", ptr_buffer->completion);
        log_printf ("  history . . . . . . . . . . . . : %p", ptr_buffer->history);
        log_printf ("  last_history. . . . . . . . . . : %p", ptr_buffer->last_history);
        log_printf ("  ptr_history . . . . . . . . . . : %p", ptr_buffer->ptr_history);
        log_printf ("  num_history . . . . . . . . . . : %d", ptr_buffer->num_history);
        log_printf ("  text_search . . . . . . . . . . : %d", ptr_buffer->text_search);
        log_printf ("  text_search_direction . . . . . : %d", ptr_buffer->text_search_direction);
        log_printf ("  text_search_exact . . . . . . . : %d", ptr_buffer->text_search_exact);
        log_printf ("  text_search_regex . . . . . . . : %d", ptr_buffer->text_search_regex);
        log_printf ("  text_search_regex_compiled. . . : %p", ptr_buffer->text_search_regex_compiled);
        log_printf ("  text_search_where . . . . . . . : %d", ptr_buffer->text_search_where);
        log_printf ("  text_search_history . . . . . . : %d", ptr_buffer->text_search_history);
        log_printf ("  text_search_found . . . . . . . : %d", ptr_buffer->text_search_found);
        log_printf ("  text_search_ptr_history . . . . : %p", ptr_buffer->text_search_ptr_history);
        log_printf ("  text_search_input . . . . . . . : '%s'", ptr_buffer->text_search_input);
        log_printf ("  highlight_words . . . . . . . . : '%s'", ptr_buffer->highlight_words);
        log_printf ("  highlight_disable_regex . . . . : '%s'", ptr_buffer->highlight_disable_regex);
        log_printf ("  highlight_disable_regex_compiled: %p", ptr_buffer->highlight_disable_regex_compiled);
        log_printf ("  highlight_regex . . . . . . . . : '%s'", ptr_buffer->highlight_regex);
        log_printf ("  highlight_regex_compiled. . . . : %p", ptr_buffer->highlight_regex_compiled);
        log_printf ("  highlight_tags_restrict . . . . : '%s'", ptr_buffer->highlight_tags_restrict);
        log_printf ("  highlight_tags_restrict_count . : %d", ptr_buffer->highlight_tags_restrict_count);
        log_printf ("  highlight_tags_restrict_array . : %p", ptr_buffer->highlight_tags_restrict_array);
        log_printf ("  highlight_tags. . . . . . . . . : '%s'", ptr_buffer->highlight_tags);
        log_printf ("  highlight_tags_count. . . . . . : %d", ptr_buffer->highlight_tags_count);
        log_printf ("  highlight_tags_array. . . . . . : %p", ptr_buffer->highlight_tags_array);
        log_printf ("  hotlist . . . . . . . . . . . . : %p", ptr_buffer->hotlist);
        log_printf ("  hotlist_removed . . . . . . . . : %p", ptr_buffer->hotlist_removed);
        log_printf ("  keys. . . . . . . . . . . . . . : %p", ptr_buffer->keys);
        log_printf ("  last_key. . . . . . . . . . . . : %p", ptr_buffer->last_key);
        log_printf ("  keys_count. . . . . . . . . . . : %d", ptr_buffer->keys_count);
        log_printf ("  local_variables . . . . . . . . : %p", ptr_buffer->local_variables);
        log_printf ("  prev_buffer . . . . . . . . . . : %p", ptr_buffer->prev_buffer);
        log_printf ("  next_buffer . . . . . . . . . . : %p", ptr_buffer->next_buffer);

        if (ptr_buffer->hotlist_max_level_nicks)
        {
            hashtable_print_log (ptr_buffer->hotlist_max_level_nicks,
                                 "hotlist_max_level_nicks");
        }

        if (ptr_buffer->keys)
        {
            log_printf ("");
            log_printf ("  => keys:");
            gui_key_print_log (ptr_buffer);
        }

        if (ptr_buffer->local_variables)
        {
            hashtable_print_log (ptr_buffer->local_variables,
                                 "local_variables");
        }

        log_printf ("");
        log_printf ("  => nicklist:");
        gui_nicklist_print_log (ptr_buffer->nicklist_root, 0);

        log_printf ("");
        log_printf ("  => last 100 lines:");
        num = 0;
        ptr_line = ptr_buffer->own_lines->last_line;
        while (ptr_line && (num < 100))
        {
            num++;
            ptr_line = ptr_line->prev_line;
        }
        if (!ptr_line)
            ptr_line = ptr_buffer->own_lines->first_line;
        else
            ptr_line = ptr_line->next_line;

        while (ptr_line)
        {
            num--;
            tags = string_rebuild_split_string ((const char **)ptr_line->data->tags_array,
                                                ",",
                                                0, -1);
            log_printf ("       line N-%05d: y:%d, str_time:'%s', tags:'%s', "
                        "displayed:%d, highlight:%d, refresh_needed:%d, "
                        "prefix:'%s'",
                        num, ptr_line->data->y, ptr_line->data->str_time,
                        (tags) ? tags  : "",
                        (int)(ptr_line->data->displayed),
                        (int)(ptr_line->data->highlight),
                        (int)(ptr_line->data->refresh_needed),
                        ptr_line->data->prefix);
            log_printf ("                     data: '%s'",
                        ptr_line->data->message);
            free (tags);

            ptr_line = ptr_line->next_line;
        }
    }

    log_printf ("");
    log_printf ("[visited buffers]");
    num = 1;
    for (ptr_buffer_visited = gui_buffers_visited; ptr_buffer_visited;
         ptr_buffer_visited = ptr_buffer_visited->next_buffer)
    {
        log_printf ("  #%d:", num);
        log_printf ("    buffer . . . . . . . . : %p", ptr_buffer_visited->buffer);
        log_printf ("    prev_buffer. . . . . . : %p", ptr_buffer_visited->prev_buffer);
        log_printf ("    next_buffer. . . . . . : %p", ptr_buffer_visited->next_buffer);
        num++;
    }
}
