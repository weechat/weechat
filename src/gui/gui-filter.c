/*
 * gui-filter.c - filter functions (used by all GUI)
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
#include <stddef.h>
#include <string.h>
#include <regex.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-hdata.h"
#include "../core/core-hook.h"
#include "../core/core-infolist.h"
#include "../core/core-log.h"
#include "../core/core-string.h"
#include "../plugins/plugin.h"
#include "gui-filter.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-line.h"
#include "gui-window.h"


struct t_gui_filter *gui_filters = NULL;           /* first filter          */
struct t_gui_filter *last_gui_filter = NULL;       /* last filter           */
int gui_filters_enabled = 1;                       /* filters enabled?      */


/*
 * Checks if a line must be displayed or not (filtered).
 *
 * Returns:
 *   1: line must be displayed (not filtered)
 *   0: line must be hidden (filtered)
 */

int
gui_filter_check_line (struct t_gui_line_data *line_data)
{
    struct t_gui_filter *ptr_filter;
    int rc;

    /* line is always displayed if filters are disabled (globally or in buffer) */
    if (!gui_filters_enabled || !line_data->buffer->filter)
        return 1;

    if (gui_line_has_tag_no_filter (line_data))
        return 1;

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (ptr_filter->enabled)
        {
            /* check buffer */
            if (string_match_list (line_data->buffer->full_name,
                                   (const char **)ptr_filter->buffers,
                                   0))
            {
                if ((strcmp (ptr_filter->tags, "*") == 0)
                    || (gui_line_match_tags (line_data,
                                             ptr_filter->tags_count,
                                             ptr_filter->tags_array)))
                {
                    /* check line with regex */
                    rc = 1;
                    if (!ptr_filter->regex_prefix && !ptr_filter->regex_message)
                        rc = 0;
                    if (gui_line_match_regex (line_data,
                                              ptr_filter->regex_prefix,
                                              ptr_filter->regex_message))
                    {
                        rc = 0;
                    }
                    if (ptr_filter->regex && (ptr_filter->regex[0] == '!'))
                        rc ^= 1;
                    if (rc == 0)
                        return 0;
                }
            }
        }
    }

    /* no tag or regex matching, then line is displayed */
    return 1;
}

/*
 * Filters a buffer, using message filters.
 *
 * If line_data is NULL, filters all lines in buffer.
 * If line_data is not NULL, filters only this line_data.
 */

void
gui_filter_buffer (struct t_gui_buffer *buffer,
                   struct t_gui_line_data *line_data)
{
    struct t_gui_line *ptr_line;
    struct t_gui_line_data *ptr_line_data;
    struct t_gui_window *ptr_window;
    int lines_changed, line_displayed, lines_hidden;

    lines_changed = 0;
    lines_hidden = buffer->lines->lines_hidden;

    ptr_line = buffer->lines->first_line;
    while (ptr_line || line_data)
    {
        ptr_line_data = (line_data) ? line_data : ptr_line->data;

        line_displayed = gui_filter_check_line (ptr_line_data);

        if (ptr_line_data->displayed != line_displayed)
        {
            lines_changed = 1;
            lines_hidden += (line_displayed) ? -1 : 1;
        }

        ptr_line_data->displayed = line_displayed;

        if (line_data)
            break;

        ptr_line = ptr_line->next_line;
    }

    if (line_data)
        line_data->buffer->lines->prefix_max_length_refresh = 1;
    else
        buffer->lines->prefix_max_length_refresh = 1;

    if (buffer->lines->lines_hidden != lines_hidden)
    {
        buffer->lines->lines_hidden = lines_hidden;
        (void) gui_buffer_send_signal (buffer,
                                       "buffer_lines_hidden",
                                       WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }

    if (lines_changed)
    {
        /* force a full refresh of buffer */
        gui_buffer_ask_chat_refresh (buffer, 2);

        /*
         * check that a scroll in a window displaying this buffer is not on a
         * hidden line (if this happens, use the previous displayed line as
         * scroll)
         */
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            if ((ptr_window->buffer == buffer)
                && ptr_window->scroll->start_line
                && !ptr_window->scroll->start_line->data->displayed)
            {
                ptr_window->scroll->start_line =
                    gui_line_get_prev_displayed (ptr_window->scroll->start_line);
                ptr_window->scroll->start_line_pos = 0;
            }
        }
    }
}

/*
 * Filters all buffers, using message filters.
 *
 * If filter is NULL, filters all buffers.
 * If filter is not NULL, filters only buffers matched by this filter.
 */

void
gui_filter_all_buffers (struct t_gui_filter *filter)
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (!filter
            || string_match_list (ptr_buffer->full_name,
                                  (const char **)filter->buffers, 0))
        {
            gui_filter_buffer (ptr_buffer, NULL);
        }
    }
}

/*
 * Enables message filtering.
 */

void
gui_filter_global_enable ()
{
    if (!gui_filters_enabled)
    {
        gui_filters_enabled = 1;
        gui_filter_all_buffers (NULL);
        (void) hook_signal_send ("filters_enabled",
                                 WEECHAT_HOOK_SIGNAL_STRING, NULL);
    }
}

/*
 * Disables message filtering.
 */

void
gui_filter_global_disable ()
{
    if (gui_filters_enabled)
    {
        gui_filters_enabled = 0;
        gui_filter_all_buffers (NULL);
        (void) hook_signal_send ("filters_disabled",
                                 WEECHAT_HOOK_SIGNAL_STRING, NULL);
    }
}

/*
 * Searches for a filter by name.
 *
 * Returns pointer to filter found, NULL if not found.
 */

struct t_gui_filter *
gui_filter_search_by_name (const char *name)
{
    struct t_gui_filter *ptr_filter;

    if (!name)
        return NULL;

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (strcmp (ptr_filter->name, name) == 0)
            return ptr_filter;
    }

    /* filter not found */
    return NULL;
}

/*
 * Displays an error when a new filter is created.
 */

void
gui_filter_new_error (const char *name, const char *error)
{
    gui_chat_printf_date_tags (
        NULL, 0, GUI_FILTER_TAG_NO_FILTER,
        _("%sUnable to add filter \"%s\": %s"),
        gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
        (name) ? name : "",
        error);
}

/*
 * Searches for position of filter in list (to keep filters sorted by name).
 */

struct t_gui_filter *
gui_filter_find_pos (struct t_gui_filter *filter)
{
    struct t_gui_filter *ptr_filter;

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (string_strcmp (filter->name, ptr_filter->name) < 0)
            return ptr_filter;
    }

    /* position not found */
    return NULL;
}

/*
 * Adds a filter to the list of filters (sorted by name).
 */

void
gui_filter_add_to_list (struct t_gui_filter *filter)
{
    struct t_gui_filter *pos_filter;

    pos_filter = gui_filter_find_pos (filter);
    if (pos_filter)
    {
        /* add filter before "pos_filter" */
        filter->prev_filter = pos_filter->prev_filter;
        filter->next_filter = pos_filter;
        if (pos_filter->prev_filter)
            (pos_filter->prev_filter)->next_filter = filter;
        else
            gui_filters = filter;
        pos_filter->prev_filter = filter;
    }
    else
    {
        /* add filter to end of list */
        filter->prev_filter = last_gui_filter;
        filter->next_filter = NULL;
        if (last_gui_filter)
            last_gui_filter->next_filter = filter;
        else
            gui_filters = filter;
        last_gui_filter = filter;
    }
}

/*
 * Removes a filter from list of filters.
 */

void
gui_filter_remove_from_list (struct t_gui_filter *filter)
{
    if (filter->prev_filter)
        (filter->prev_filter)->next_filter = filter->next_filter;
    if (filter->next_filter)
        (filter->next_filter)->prev_filter = filter->prev_filter;
    if (gui_filters == filter)
        gui_filters = filter->next_filter;
    if (last_gui_filter == filter)
        last_gui_filter = filter->prev_filter;
}

/*
 * Creates a new filter.
 *
 * Returns pointer to new filter, NULL if error.
 */

struct t_gui_filter *
gui_filter_new (int enabled, const char *name, const char *buffer_name,
                const char *tags, const char *regex)
{
    struct t_gui_filter *new_filter;
    regex_t *regex1, *regex2;
    char *pos_tab, *regex_prefix, buf[512], str_error[1024];
    const char *ptr_start_regex, *pos_regex_message;
    int rc;

    if (!name || !buffer_name || !tags || !regex)
    {
        gui_filter_new_error (name, _("not enough arguments"));
        return NULL;
    }

    if (gui_filter_search_by_name (name))
    {
        gui_filter_new_error (name,
                              _("a filter with same name already exists "
                                "(choose another name or use option "
                                "\"addreplace\" to overwrite it)"));
        return NULL;
    }

    ptr_start_regex = regex;
    if ((ptr_start_regex[0] == '!')
        || ((ptr_start_regex[0] == '\\') && (ptr_start_regex[1] == '!')))
    {
        ptr_start_regex++;
    }

    regex1 = NULL;
    regex2 = NULL;

    if (strcmp (ptr_start_regex, "*") != 0)
    {
        pos_tab = strstr (ptr_start_regex, "\\t");
        if (pos_tab)
        {
            regex_prefix = string_strndup (ptr_start_regex,
                                           pos_tab - ptr_start_regex);
            pos_regex_message = pos_tab + 2;
        }
        else
        {
            regex_prefix = NULL;
            pos_regex_message = ptr_start_regex;
        }

        if (regex_prefix && regex_prefix[0])
        {
            regex1 = malloc (sizeof (*regex1));
            if (regex1)
            {
                rc = string_regcomp (regex1, regex_prefix,
                                     REG_EXTENDED | REG_ICASE | REG_NOSUB);
                if (rc != 0)
                {
                    regerror (rc, regex1, buf, sizeof (buf));
                    snprintf (str_error, sizeof (str_error),
                              /* TRANSLATORS: %s is the error returned by regerror */
                              _("invalid regular expression (%s)"),
                              buf);
                    gui_filter_new_error (name, str_error);
                    free (regex_prefix);
                    free (regex1);
                    return NULL;
                }
            }
        }

        if (pos_regex_message && pos_regex_message[0])
        {
            regex2 = malloc (sizeof (*regex2));
            if (regex2)
            {
                rc = string_regcomp (regex2, pos_regex_message,
                                     REG_EXTENDED | REG_ICASE | REG_NOSUB);
                if (rc != 0)
                {
                    regerror (rc, regex2, buf, sizeof (buf));
                    snprintf (str_error, sizeof (str_error),
                              /* TRANSLATORS: %s is the error returned by regerror */
                              _("invalid regular expression (%s)"),
                              buf);
                    gui_filter_new_error (name, str_error);
                    free (regex_prefix);
                    if (regex1)
                    {
                        regfree (regex1);
                        free (regex1);
                    }
                    free (regex2);
                    return NULL;
                }
            }
        }

        free (regex_prefix);
    }

    /* create new filter */
    new_filter = malloc (sizeof (*new_filter));
    if (new_filter)
    {
        /* init filter */
        new_filter->enabled = enabled;
        new_filter->name = strdup (name);
        new_filter->buffer_name = strdup ((buffer_name) ? buffer_name : "*");
        new_filter->buffers = string_split (
            new_filter->buffer_name,
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &new_filter->num_buffers);
        new_filter->tags = (tags) ? strdup (tags) : NULL;
        new_filter->tags_array = string_split_tags (new_filter->tags,
                                                    &new_filter->tags_count);
        new_filter->regex = strdup (regex);
        new_filter->regex_prefix = regex1;
        new_filter->regex_message = regex2;

        gui_filter_add_to_list (new_filter);

        (void) hook_signal_send ("filter_added",
                                 WEECHAT_HOOK_SIGNAL_POINTER, new_filter);
    }
    else
    {
        gui_filter_new_error (name, _("not enough memory"));
    }

    return new_filter;
}

/*
 * Renames a filter.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_filter_rename (struct t_gui_filter *filter, const char *new_name)
{
    if (!filter || !new_name)
        return 0;

    if (gui_filter_search_by_name (new_name))
        return 0;

    free (filter->name);
    filter->name = strdup (new_name);

    /* resort list of filters */
    gui_filter_remove_from_list (filter);
    gui_filter_add_to_list (filter);

    return 1;
}

/*
 * Removes a filter.
 */

void
gui_filter_free (struct t_gui_filter *filter)
{
    if (!filter)
        return;

    (void) hook_signal_send ("filter_removing",
                             WEECHAT_HOOK_SIGNAL_POINTER, filter);

    /* free data */
    free (filter->name);
    free (filter->buffer_name);
    string_free_split (filter->buffers);
    free (filter->tags);
    string_free_split_tags (filter->tags_array);
    free (filter->regex);
    if (filter->regex_prefix)
    {
        regfree (filter->regex_prefix);
        free (filter->regex_prefix);
    }
    if (filter->regex_message)
    {
        regfree (filter->regex_message);
        free (filter->regex_message);
    }

    gui_filter_remove_from_list (filter);

    free (filter);

    (void) hook_signal_send ("filter_removed", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * Removes all filters.
 */

void
gui_filter_free_all ()
{
    while (gui_filters)
    {
        gui_filter_free (gui_filters);
    }
}

/*
 * Returns hdata for filter.
 */

struct t_hdata *
gui_filter_hdata_filter_cb (const void *pointer, void *data,
                            const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_filter", "next_filter",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_filter, enabled, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, buffer_name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, num_buffers, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, buffers, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, tags, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, tags_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, tags_array, POINTER, 0, "*,tags_count", NULL);
        HDATA_VAR(struct t_gui_filter, regex, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, regex_prefix, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, regex_message, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_filter, prev_filter, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_filter, next_filter, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(gui_filters, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_gui_filter, 0);
    }
    return hdata;
}

/*
 * Adds a filter in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_filter_add_to_infolist (struct t_infolist *infolist,
                            struct t_gui_filter *filter)
{
    struct t_infolist_item *ptr_item;
    char option_name[64], *tags;
    int i;

    if (!infolist || !filter)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_integer (ptr_item, "enabled", filter->enabled))
        return 0;
    if (!infolist_new_var_string (ptr_item, "name", filter->name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "buffer_name", filter->buffer_name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "tags", filter->tags))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "tags_count", filter->tags_count))
        return 0;
    for (i = 0; i < filter->tags_count; i++)
    {
        snprintf (option_name, sizeof (option_name), "tag_%05d", i + 1);
        tags = string_rebuild_split_string ((const char **)filter->tags_array[i],
                                            "+",
                                            0, -1);
        if (tags)
        {
            if (!infolist_new_var_string (ptr_item, option_name, tags))
            {
                free (tags);
                return 0;
            }
            free (tags);
        }
    }
    if (!infolist_new_var_string (ptr_item, "regex", filter->regex))
        return 0;

    return 1;
}

/*
 * Prints filter infos in WeeChat log file (usually for crash dump).
 */

void
gui_filter_print_log ()
{
    struct t_gui_filter *ptr_filter;
    int i;

    log_printf ("");
    log_printf ("gui_filters_enabled = %d", gui_filters_enabled);

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        log_printf ("");
        log_printf ("[filter (addr:%p)]", ptr_filter);
        log_printf ("  enabled. . . . . . . . : %d", ptr_filter->enabled);
        log_printf ("  name . . . . . . . . . : '%s'", ptr_filter->name);
        log_printf ("  buffer_name. . . . . . : '%s'", ptr_filter->buffer_name);
        log_printf ("  num_buffers. . . . . . : %d", ptr_filter->num_buffers);
        log_printf ("  buffers. . . . . . . . : %p", ptr_filter->buffers);
        for (i = 0; i < ptr_filter->num_buffers; i++)
        {
            log_printf ("  buffers[%03d] . . . . . : '%s'", i, ptr_filter->buffers[i]);
        }
        log_printf ("  tags . . . . . . . . . : '%s'", ptr_filter->tags);
        log_printf ("  regex. . . . . . . . . : '%s'", ptr_filter->regex);
        log_printf ("  regex_prefix . . . . . : %p", ptr_filter->regex_prefix);
        log_printf ("  regex_message. . . . . : %p", ptr_filter->regex_message);
        log_printf ("  prev_filter. . . . . . : %p", ptr_filter->prev_filter);
        log_printf ("  next_filter. . . . . . : %p", ptr_filter->next_filter);
    }
}
