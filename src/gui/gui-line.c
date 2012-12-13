/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * gui-line.c: line functions (used by all GUI)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hdata.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-line.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-filter.h"
#include "gui-hotlist.h"
#include "gui-nicklist.h"
#include "gui-window.h"


/*
 * Allocates structure "t_gui_lines" and initializes it.
 *
 * Returns pointer to new lines, NULL if error.
 */

struct t_gui_lines *
gui_lines_alloc ()
{
    struct t_gui_lines *new_lines;

    new_lines = malloc (sizeof (*new_lines));
    if (new_lines)
    {
        new_lines->first_line = NULL;
        new_lines->last_line = NULL;
        new_lines->last_read_line = NULL;
        new_lines->lines_count = 0;
        new_lines->first_line_not_read = 0;
        new_lines->lines_hidden = 0;
        new_lines->buffer_max_length = 0;
        new_lines->prefix_max_length = CONFIG_INTEGER(config_look_prefix_align_min);
    }

    return new_lines;
}

/*
 * Frees a "t_gui_lines" structure.
 */

void
gui_lines_free (struct t_gui_lines *lines)
{
    free (lines);
}

/*
 * Checks if prefix on line is a nick and is the same as nick on previous line.
 *
 * Returns:
 *   1: prefix is a nick and same as nick on previous line
 *   0: prefix is not a nick, or different from nick on previous line
 */

int
gui_line_prefix_is_same_nick_as_previous (struct t_gui_line *line)
{
    const char *nick, *nick_previous;
    struct t_gui_line *prev_line;

    /*
     * if line is not displayed, has a highlight, or does not have a tag
     * beginning with "prefix_nick" => display standard prefix
     */
    if (!line->data->displayed || line->data->highlight
        || !gui_line_search_tag_starting_with (line, "prefix_nick"))
        return 0;

    /* no nick on line => display standard prefix */
    nick = gui_line_get_nick_tag (line);
    if (!nick)
        return 0;

    /*
     * previous line is not found => display standard prefix
     */
    prev_line = gui_line_get_prev_displayed (line);
    if (!prev_line)
        return 0;

    /* buffer is not the same as previous line => display standard prefix */
    if (line->data->buffer != prev_line->data->buffer)
        return 0;

    /*
     * previous line does not have a tag beginning with "prefix_nick"
     * => display standard prefix
     */
    if (!gui_line_search_tag_starting_with (prev_line, "prefix_nick"))
        return 0;

    /* no nick on previous line => display standard prefix */
    nick_previous = gui_line_get_nick_tag (prev_line);
    if (!nick_previous)
        return 0;

    /* prefix can be hidden/replaced if nicks are equal */
    return (strcmp (nick, nick_previous) == 0) ? 1 : 0;
}

/*
 * Gets prefix and its length (for display only).
 *
 * If the prefix can be hidden (same nick as previous message), and if the
 * option is enabled (not empty string), then returns empty prefix or prefix
 * from option.
 */

void
gui_line_get_prefix_for_display (struct t_gui_line *line,
                                 char **prefix, int *length,
                                 char **color)
{
    const char *tag_prefix_nick;

    if (CONFIG_STRING(config_look_prefix_same_nick)
        && CONFIG_STRING(config_look_prefix_same_nick)[0]
        && gui_line_prefix_is_same_nick_as_previous (line))
    {
        /* same nick: return empty prefix or value from option */
        if (strcmp (CONFIG_STRING(config_look_prefix_same_nick), " ") == 0)
        {
            if (prefix)
                *prefix = gui_chat_prefix_empty;
            if (length)
                *length = 0;
            if (color)
                *color = NULL;
        }
        else
        {
            if (prefix)
                *prefix = CONFIG_STRING(config_look_prefix_same_nick);
            if (length)
                *length = config_length_prefix_same_nick;
            if (color)
            {
                *color = NULL;
                tag_prefix_nick = gui_line_search_tag_starting_with (line,
                                                                     "prefix_nick_");
                if (tag_prefix_nick)
                    *color = (char *)(tag_prefix_nick + 12);
            }
        }
    }
    else
    {
        /* not same nick: return prefix from line */
        if (prefix)
            *prefix = line->data->prefix;
        if (length)
            *length = line->data->prefix_length;
        if (color)
            *color = NULL;
    }
}

/*
 * Gets alignment for a line.
 */

int
gui_line_get_align (struct t_gui_buffer *buffer, struct t_gui_line *line,
                    int with_suffix, int first_line)
{
    int length_time, length_buffer, length_suffix, prefix_length;

    /* return immediately if alignment for end of lines is "time" */
    if (!first_line
        && (CONFIG_INTEGER(config_look_align_end_of_lines) == CONFIG_LOOK_ALIGN_END_OF_LINES_TIME))
    {
        return 0;
    }

    /* length of time */
    if (buffer->time_for_each_line)
    {
        length_time = (gui_chat_time_length == 0) ? 0 : gui_chat_time_length + 1;
    }
    else
        length_time = 0;

    /* return immediately if alignment for end of lines is "buffer" */
    if (!first_line
        && (CONFIG_INTEGER(config_look_align_end_of_lines) == CONFIG_LOOK_ALIGN_END_OF_LINES_BUFFER))
    {
        return length_time;
    }

    /* length of buffer name (when many buffers are merged) */
    if (buffer->mixed_lines && (buffer->active != 2))
    {
        if ((CONFIG_INTEGER(config_look_prefix_buffer_align) == CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE)
            && (CONFIG_INTEGER(config_look_prefix_align) == CONFIG_LOOK_PREFIX_ALIGN_NONE))
            length_buffer = gui_chat_strlen_screen (gui_buffer_get_short_name (buffer)) + 1;
        else
        {
            if (CONFIG_INTEGER(config_look_prefix_buffer_align) == CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE)
                length_buffer = buffer->mixed_lines->buffer_max_length + 1;
            else
                length_buffer = ((CONFIG_INTEGER(config_look_prefix_buffer_align_max) > 0)
                                 && (buffer->mixed_lines->buffer_max_length > CONFIG_INTEGER(config_look_prefix_buffer_align_max))) ?
                    CONFIG_INTEGER(config_look_prefix_buffer_align_max) + 1 : buffer->mixed_lines->buffer_max_length + 1;
        }
    }
    else
        length_buffer = 0;

    /* return immediately if alignment for end of lines is "prefix" */
    if (!first_line
        && (CONFIG_INTEGER(config_look_align_end_of_lines) == CONFIG_LOOK_ALIGN_END_OF_LINES_PREFIX))
    {
        return length_time + length_buffer;
    }

    gui_line_get_prefix_for_display (line, NULL, &prefix_length, NULL);

    if (CONFIG_INTEGER(config_look_prefix_align) == CONFIG_LOOK_PREFIX_ALIGN_NONE)
    {
        return length_time + length_buffer + prefix_length + ((prefix_length > 0) ? 1 : 0);
    }

    length_suffix = 0;
    if (with_suffix)
    {
        if (CONFIG_STRING(config_look_prefix_suffix)
            && CONFIG_STRING(config_look_prefix_suffix)[0])
            length_suffix = gui_chat_strlen_screen (CONFIG_STRING(config_look_prefix_suffix)) + 1;
    }

    return length_time + ((buffer->lines->prefix_max_length > 0) ? 1 : 0)
        + length_buffer
        + (((CONFIG_INTEGER(config_look_prefix_align_max) > 0)
            && (buffer->lines->prefix_max_length > CONFIG_INTEGER(config_look_prefix_align_max))) ?
           CONFIG_INTEGER(config_look_prefix_align_max) : buffer->lines->prefix_max_length)
        + length_suffix;
}

/*
 * Checks if a line is displayed (no filter on line or filters disabled).
 *
 * Returns:
 *   1: line is displayed
 *   0: line is hidden
 */

int
gui_line_is_displayed (struct t_gui_line *line)
{
    /* line is hidden if filters are enabled and flag "displayed" is not set */
    if (gui_filters_enabled && !line->data->displayed)
        return 0;

    /* in all other cases, line is displayed */
    return 1;
}

/*
 * Gets the first line displayed of a buffer.
 *
 * Returns pointer to first line displayed, NULL if not found.
 */

struct t_gui_line *
gui_line_get_first_displayed (struct t_gui_buffer *buffer)
{
    struct t_gui_line *ptr_line;

    ptr_line = buffer->lines->first_line;
    while (ptr_line && !gui_line_is_displayed (ptr_line))
    {
        ptr_line = ptr_line->next_line;
    }

    return ptr_line;
}

/*
 * Gets the last line displayed of a buffer.
 *
 * Returns pointer to last line displayed, NULL if not found.
 */

struct t_gui_line *
gui_line_get_last_displayed (struct t_gui_buffer *buffer)
{
    struct t_gui_line *ptr_line;

    ptr_line = buffer->lines->last_line;
    while (ptr_line && !gui_line_is_displayed (ptr_line))
    {
        ptr_line = ptr_line->prev_line;
    }

    return ptr_line;
}

/*
 * Gets previous line displayed.
 *
 * Returns pointer to previous line displayed, NULL if not found.
 */

struct t_gui_line *
gui_line_get_prev_displayed (struct t_gui_line *line)
{
    if (line)
    {
        line = line->prev_line;
        while (line && !gui_line_is_displayed (line))
        {
            line = line->prev_line;
        }
    }
    return line;
}

/*
 * Gets next line displayed.
 *
 * Returns pointer to next line displayed, NULL if not found.
 */

struct t_gui_line *
gui_line_get_next_displayed (struct t_gui_line *line)
{
    if (line)
    {
        line = line->next_line;
        while (line && !gui_line_is_displayed (line))
        {
            line = line->next_line;
        }
    }
    return line;
}

/*
 * Searches for text in a line.
 *
 * Returns:
 *   1: text found in line
 *   0: text not found in line
 */

int
gui_line_search_text (struct t_gui_line *line, const char *text,
                      int case_sensitive)
{
    char *prefix, *message;
    int rc;

    if (!line || !line->data->message || !text || !text[0])
        return 0;

    rc = 0;

    if (line->data->prefix)
    {
        prefix = gui_color_decode (line->data->prefix, NULL);
        if (prefix)
        {
            if ((case_sensitive && (strstr (prefix, text)))
                || (!case_sensitive && (string_strcasestr (prefix, text))))
                rc = 1;
            free (prefix);
        }
    }

    if (!rc)
    {
        message = gui_color_decode (line->data->message, NULL);
        if (message)
        {
            if ((case_sensitive && (strstr (message, text)))
                || (!case_sensitive && (string_strcasestr (message, text))))
                rc = 1;
            free (message);
        }
    }

    return rc;
}

/*
 * Checks if a line matches regex.
 *
 * Returns:
 *   1: line matches regex
 *   0: line does not match regex
 */

int
gui_line_match_regex (struct t_gui_line *line, regex_t *regex_prefix,
                      regex_t *regex_message)
{
    char *prefix, *message;
    int match_prefix, match_message;

    if (!line || (!regex_prefix && !regex_message))
        return 0;

    prefix = NULL;
    message = NULL;

    match_prefix = 1;
    match_message = 1;

    if (line->data->prefix)
    {
        prefix = gui_color_decode (line->data->prefix, NULL);
        if (!prefix
            || (regex_prefix && (regexec (regex_prefix, prefix, 0, NULL, 0) != 0)))
            match_prefix = 0;
    }
    else
    {
        if (regex_prefix)
            match_prefix = 0;
    }

    if (line->data->message)
    {
        message = gui_color_decode (line->data->message, NULL);
        if (!message
            || (regex_message && (regexec (regex_message, message, 0, NULL, 0) != 0)))
            match_message = 0;
    }
    else
    {
        if (regex_message)
            match_message = 0;
    }

    if (prefix)
        free (prefix);
    if (message)
        free (message);

    return (match_prefix && match_message);
}

/*
 * Checks if line matches tags.
 *
 * Returns:
 *   1: line matches tags
 *   0: line does not match tags
 */

int
gui_line_match_tags (struct t_gui_line *line, int tags_count,
                     char **tags_array)
{
    int i, j;

    if (!line)
        return 0;

    if (line->data->tags_count == 0)
        return 0;

    for (i = 0; i < tags_count; i++)
    {
        for (j = 0; j < line->data->tags_count; j++)
        {
            /* check tag */
            if (string_match (line->data->tags_array[j],
                              tags_array[i],
                              0))
                return 1;
        }
    }

    return 0;
}

/*
 * Returns pointer on tag starting with "tag", NULL if such tag is not found.
 */

const char *
gui_line_search_tag_starting_with (struct t_gui_line *line, const char *tag)
{
    int i, length;

    if (!line || !tag)
        return NULL;

    length = strlen (tag);

    for (i = 0; i < line->data->tags_count; i++)
    {
        if (strncmp (line->data->tags_array[i], tag, length) == 0)
            return line->data->tags_array[i];
    }

    /* tag not found */
    return NULL;
}

/*
 * Gets nick in tags: returns "xxx" if tag "nick_xxx" is found.
 */

const char *
gui_line_get_nick_tag (struct t_gui_line *line)
{
    const char *tag;

    tag = gui_line_search_tag_starting_with (line, "nick_");
    if (!tag)
        return NULL;

    return tag + 5;
}

/*
 * Checks if a line has highlight (with a string in global highlight or buffer
 * highlight).
 *
 * Returns:
 *   1: line has highlight
 *   0: line has no highlight
 */

int
gui_line_has_highlight (struct t_gui_line *line)
{
    int rc, i, j, no_highlight;
    char *msg_no_color, *highlight_words;

    /*
     * highlights are disabled on this buffer? (special value "-" means that
     * buffer does not want any highlight)
     */
    if (line->data->buffer->highlight_words
        && (strcmp (line->data->buffer->highlight_words, "-") == 0))
        return 0;

    /*
     * check if highlight is forced by a tag (with option highlight_tags) or
     * disabled for line
     */
    no_highlight = 0;
    for (i = 0; i < line->data->tags_count; i++)
    {
        if (config_highlight_tags)
        {
            for (j = 0; j < config_num_highlight_tags; j++)
            {
                if (string_strcasecmp (line->data->tags_array[i],
                                       config_highlight_tags[j]) == 0)
                    return 1;
            }
        }
        if (strcmp (line->data->tags_array[i], GUI_CHAT_TAG_NO_HIGHLIGHT) == 0)
            no_highlight = 1;
    }
    if (no_highlight)
        return 0;

    /*
     * check that line matches highlight tags, if any (if no tag is specified,
     * then any tag is allowed)
     */
    if (line->data->buffer->highlight_tags_count > 0)
    {
        if (!gui_line_match_tags (line,
                                  line->data->buffer->highlight_tags_count,
                                  line->data->buffer->highlight_tags_array))
            return 0;
    }

    /* remove color codes from line message */
    msg_no_color = gui_color_decode (line->data->message, NULL);
    if (!msg_no_color)
        return 0;

    /*
     * there is highlight on line if one of buffer highlight words matches line
     * or one of global highlight words matches line
     */
    highlight_words = gui_buffer_string_replace_local_var (line->data->buffer,
                                                           line->data->buffer->highlight_words);
    rc = string_has_highlight (msg_no_color,
                               (highlight_words) ?
                               highlight_words : line->data->buffer->highlight_words);
    if (highlight_words)
        free (highlight_words);

    if (!rc)
    {
        highlight_words = gui_buffer_string_replace_local_var (line->data->buffer,
                                                               CONFIG_STRING(config_look_highlight));
        rc = string_has_highlight (msg_no_color,
                                   (highlight_words) ?
                                   highlight_words : CONFIG_STRING(config_look_highlight));
        if (highlight_words)
            free (highlight_words);
    }

    if (!rc && config_highlight_regex)
    {
        rc = string_has_highlight_regex_compiled (msg_no_color,
                                                  config_highlight_regex);
    }

    if (!rc && line->data->buffer->highlight_regex_compiled)
    {
        rc = string_has_highlight_regex_compiled (msg_no_color,
                                                  line->data->buffer->highlight_regex_compiled);
    }

    free (msg_no_color);

    return rc;
}

/*
 * Checks if nick of line is offline (not in nicklist any more).
 *
 * Returns:
 *   1: nick is offline
 *   0: nick is still there (in nicklist)
 */

int
gui_line_has_offline_nick (struct t_gui_line *line)
{
    const char *nick;

    if (line && gui_line_search_tag_starting_with (line, "prefix_nick"))
    {
        nick = gui_line_get_nick_tag (line);
        if (nick
            && (line->data->buffer->nicklist_root
                && (line->data->buffer->nicklist_root->nicks
                    || line->data->buffer->nicklist_root->children))
            && !gui_nicklist_search_nick (line->data->buffer, NULL, nick))
        {
            return 1;
        }
    }

    return 0;
}

/*
 * Computes "buffer_max_length" for a "t_gui_lines" structure.
 */

void
gui_line_compute_buffer_max_length (struct t_gui_buffer *buffer,
                                    struct t_gui_lines *lines)
{
    struct t_gui_buffer *ptr_buffer;
    int length;
    const char *short_name;

    lines->buffer_max_length = 0;
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        short_name = gui_buffer_get_short_name (ptr_buffer);
        if (ptr_buffer->number == buffer->number)
        {
            length = gui_chat_strlen_screen (short_name);
            if (length > lines->buffer_max_length)
                lines->buffer_max_length = length;
        }
    }
}

/*
 * Computes "prefix_max_length" for a "t_gui_lines" structure.
 */

void
gui_line_compute_prefix_max_length (struct t_gui_lines *lines)
{
    struct t_gui_line *ptr_line;
    int prefix_length;

    lines->prefix_max_length = CONFIG_INTEGER(config_look_prefix_align_min);
    for (ptr_line = lines->first_line; ptr_line;
         ptr_line = ptr_line->next_line)
    {
        gui_line_get_prefix_for_display (ptr_line, NULL, &prefix_length, NULL);
        if (prefix_length > lines->prefix_max_length)
            lines->prefix_max_length = prefix_length;
    }
}

/*
 * Adds a line to a "t_gui_lines" structure.
 */

void
gui_line_add_to_list (struct t_gui_lines *lines,
                      struct t_gui_line *line)
{
    int prefix_length;

    if (!lines->first_line)
        lines->first_line = line;
    else
        (lines->last_line)->next_line = line;
    line->prev_line = lines->last_line;
    line->next_line = NULL;
    lines->last_line = line;

    /* adjust "prefix_max_length" if this prefix length is > max */
    gui_line_get_prefix_for_display (line, NULL, &prefix_length, NULL);
    if (prefix_length > lines->prefix_max_length)
        lines->prefix_max_length = prefix_length;

    lines->lines_count++;
}

/*
 * Removes a line from a "t_gui_lines" structure.
 */

void
gui_line_remove_from_list (struct t_gui_buffer *buffer,
                           struct t_gui_lines *lines,
                           struct t_gui_line *line,
                           int free_data)
{
    struct t_gui_window *ptr_win;
    struct t_gui_window_scroll *ptr_scroll;
    int i, update_prefix_max_length, prefix_length;

    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        /* reset scroll for any window scroll starting with this line */
        for (ptr_scroll = ptr_win->scroll; ptr_scroll;
             ptr_scroll = ptr_scroll->next_scroll)
        {
            if (ptr_scroll->start_line == line)
            {
                ptr_scroll->start_line = ptr_scroll->start_line->next_line;
                ptr_scroll->start_line_pos = 0;
                gui_buffer_ask_chat_refresh (buffer, 2);
            }
        }
        /* remove line from coords */
        if (ptr_win->coords)
        {
            for (i = 0; i < ptr_win->coords_size; i++)
            {
                if (ptr_win->coords[i].line == line)
                    gui_window_coords_init_line (ptr_win, i);
            }
        }
    }

    gui_line_get_prefix_for_display (line, NULL, &prefix_length, NULL);
    update_prefix_max_length =
        (prefix_length == lines->prefix_max_length);

    /* move read marker if it was on line we are removing */
    if (lines->last_read_line == line)
    {
        lines->last_read_line = lines->last_read_line->prev_line;
        lines->first_line_not_read = (lines->last_read_line) ? 0 : 1;
        gui_buffer_ask_chat_refresh (buffer, 1);
    }

    /* free data */
    if (free_data)
    {
        if (line->data->str_time)
            free (line->data->str_time);
        if (line->data->tags_array)
            string_free_split (line->data->tags_array);
        if (line->data->prefix)
            free (line->data->prefix);
        if (line->data->message)
            free (line->data->message);
        free (line->data);
    }

    /* remove line from list */
    if (line->prev_line)
        (line->prev_line)->next_line = line->next_line;
    if (line->next_line)
        (line->next_line)->prev_line = line->prev_line;
    if (lines->first_line == line)
        lines->first_line = line->next_line;
    if (lines->last_line == line)
        lines->last_line = line->prev_line;

    lines->lines_count--;

    free (line);

    /* compute "prefix_max_length" if needed */
    if (update_prefix_max_length)
        gui_line_compute_prefix_max_length (lines);
}

/*
 * Adds line to mixed lines for a buffer.
 */

void
gui_line_mixed_add (struct t_gui_lines *lines,
                    struct t_gui_line_data *line_data)
{
    struct t_gui_line *new_line;

    new_line = malloc (sizeof (*new_line));
    if (new_line)
    {
        new_line->data = line_data;
        gui_line_add_to_list (lines, new_line);
    }
}

/*
 * Frees all mixed lines matching a buffer.
 */

void
gui_line_mixed_free_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_line *ptr_line, *ptr_next_line;

    if (buffer->mixed_lines)
    {
        ptr_line = buffer->mixed_lines->first_line;
        while (ptr_line)
        {
            ptr_next_line = ptr_line->next_line;

            if (ptr_line->data->buffer == buffer)
            {
                gui_line_remove_from_list (buffer,
                                           buffer->mixed_lines,
                                           ptr_line,
                                           0);
            }

            ptr_line = ptr_next_line;
        }
    }
}

/*
 * Frees all mixed lines in a buffer.
 */

void
gui_line_mixed_free_all (struct t_gui_buffer *buffer)
{
    if (buffer->mixed_lines)
    {
        while (buffer->mixed_lines->first_line)
        {
            gui_line_remove_from_list (buffer,
                                       buffer->mixed_lines,
                                       buffer->mixed_lines->first_line,
                                       0);
        }
    }
}

/*
 * Deletes a line from a buffer.
 */

void
gui_line_free (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    struct t_gui_line *ptr_line;

    /* first remove mixed line if it exists */
    if (buffer->mixed_lines)
    {
        for (ptr_line = buffer->mixed_lines->first_line; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            if (ptr_line->data == line->data)
            {
                gui_line_remove_from_list (buffer,
                                           buffer->mixed_lines,
                                           ptr_line,
                                           0);
                break;
            }
        }
    }

    /* remove line from lines list */
    gui_line_remove_from_list (buffer, buffer->own_lines, line, 1);
}

/*
 * Deletes all formatted lines from a buffer.
 */

void
gui_line_free_all (struct t_gui_buffer *buffer)
{
    while (buffer->own_lines->first_line)
    {
        gui_line_free (buffer, buffer->own_lines->first_line);
    }
}

/*
 * Gets notify level for a line.
 *
 * Returns notify level of line, -1 if tag "notify_none" is found (meaning no
 * notify at all for line).
 */

int
gui_line_get_notify_level (struct t_gui_line *line)
{
    int i;

    for (i = 0; i < line->data->tags_count; i++)
    {
        if (string_strcasecmp (line->data->tags_array[i], "notify_none") == 0)
            return -1;
        if (string_strcasecmp (line->data->tags_array[i], "notify_highlight") == 0)
            return GUI_HOTLIST_HIGHLIGHT;
        if (string_strcasecmp (line->data->tags_array[i], "notify_private") == 0)
            return GUI_HOTLIST_PRIVATE;
        if (string_strcasecmp (line->data->tags_array[i], "notify_message") == 0)
            return GUI_HOTLIST_MESSAGE;
    }
    return GUI_HOTLIST_LOW;
}

/*
 * Adds a new line for a buffer.
 */

struct t_gui_line *
gui_line_add (struct t_gui_buffer *buffer, time_t date,
              time_t date_printed, const char *tags,
              const char *prefix, const char *message)
{
    struct t_gui_line *new_line;
    struct t_gui_line_data *new_line_data;
    struct t_gui_window *ptr_win;
    char *message_for_signal;
    const char *nick;
    int notify_level, *max_notify_level, lines_removed;
    time_t current_time;

    /*
     * remove line(s) if necessary, according to history options:
     *   max_lines:   if > 0, keep only N lines in buffer
     *   max_minutes: if > 0, keep only lines from last N minutes
     */
    lines_removed = 0;
    current_time = time (NULL);
    while (buffer->own_lines->first_line
           && (((CONFIG_INTEGER(config_history_max_buffer_lines_number) > 0)
                && (buffer->own_lines->lines_count + 1 >
                    CONFIG_INTEGER(config_history_max_buffer_lines_number)))
               || ((CONFIG_INTEGER(config_history_max_buffer_lines_minutes) > 0)
                   && (current_time - buffer->own_lines->first_line->data->date_printed >
                       CONFIG_INTEGER(config_history_max_buffer_lines_minutes) * 60))))
    {
        gui_line_free (buffer, buffer->own_lines->first_line);
        lines_removed++;
    }

    /* create new line */
    new_line = malloc (sizeof (*new_line));
    if (!new_line)
    {
        log_printf (_("Not enough memory for new line"));
        return NULL;
    }

    /* create data for line */
    new_line_data = malloc (sizeof (*(new_line->data)));
    if (!new_line_data)
    {
        free (new_line);
        log_printf (_("Not enough memory for new line"));
        return NULL;
    }
    new_line->data = new_line_data;

    /* fill data in new line */
    new_line->data->buffer = buffer;
    new_line->data->y = -1;
    new_line->data->date = date;
    new_line->data->date_printed = date_printed;
    new_line->data->str_time = gui_chat_get_time_string (date);
    if (tags)
    {
        new_line->data->tags_array = string_split (tags, ",", 0, 0,
                                                   &new_line->data->tags_count);
    }
    else
    {
        new_line->data->tags_count = 0;
        new_line->data->tags_array = NULL;
    }
    new_line->data->refresh_needed = 0;
    new_line->data->prefix = (prefix) ?
        strdup (prefix) : ((date != 0) ? strdup ("") : NULL);
    new_line->data->prefix_length = (prefix) ?
        gui_chat_strlen_screen (prefix) : 0;
    new_line->data->message = (message) ? strdup (message) : strdup ("");

    /* get notify level and max notify level for nick in buffer */
    notify_level = gui_line_get_notify_level (new_line);
    max_notify_level = NULL;
    nick = gui_line_get_nick_tag (new_line);
    if (nick)
        max_notify_level = hashtable_get (buffer->hotlist_max_level_nicks, nick);
    if (max_notify_level
        && (*max_notify_level < notify_level))
        notify_level = *max_notify_level;

    if (notify_level == GUI_HOTLIST_HIGHLIGHT)
        new_line->data->highlight = 1;
    else if (max_notify_level && (*max_notify_level < GUI_HOTLIST_HIGHLIGHT))
        new_line->data->highlight = 0;
    else
        new_line->data->highlight = gui_line_has_highlight (new_line);

    /* check if line is filtered or not */
    new_line->data->displayed = gui_filter_check_line (new_line);

    /* add line to lines list */
    gui_line_add_to_list (buffer->own_lines, new_line);

    /* update hotlist and/or send signals for line */
    if (new_line->data->displayed)
    {
        if (new_line->data->highlight)
        {
            (void) gui_hotlist_add (buffer, GUI_HOTLIST_HIGHLIGHT, NULL);
            if (!weechat_upgrading)
            {
                message_for_signal = gui_chat_build_string_prefix_message (new_line);
                if (message_for_signal)
                {
                    hook_signal_send ("weechat_highlight",
                                      WEECHAT_HOOK_SIGNAL_STRING,
                                      message_for_signal);
                    free (message_for_signal);
                }
            }
        }
        else
        {
            if (!weechat_upgrading && (notify_level == GUI_HOTLIST_PRIVATE))
            {
                message_for_signal = gui_chat_build_string_prefix_message (new_line);
                if (message_for_signal)
                {
                    hook_signal_send ("weechat_pv",
                                      WEECHAT_HOOK_SIGNAL_STRING,
                                      message_for_signal);
                    free (message_for_signal);
                }
            }
            if (notify_level >= GUI_HOTLIST_MIN)
                (void) gui_hotlist_add (buffer, notify_level, NULL);
        }
    }
    else
    {
        if (!buffer->own_lines->lines_hidden)
        {
            buffer->own_lines->lines_hidden = 1;
            if (buffer->mixed_lines)
                buffer->mixed_lines->lines_hidden = 1;
            hook_signal_send ("buffer_lines_hidden",
                              WEECHAT_HOOK_SIGNAL_POINTER, buffer);
        }
    }

    /* add mixed line, if buffer is attched to at least one other buffer */
    if (buffer->mixed_lines)
    {
        gui_line_mixed_add (buffer->mixed_lines, new_line->data);
    }

    /*
     * if some lines were removed, force a full refresh if at least one window
     * is displaying buffer and that number of lines in buffer is lower than
     * window height
     */
    if (lines_removed > 0)
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if ((ptr_win->buffer == buffer)
                && (buffer->own_lines->lines_count < ptr_win->win_chat_height))
            {
                gui_buffer_ask_chat_refresh (buffer, 2);
                break;
            }
        }
    }

    hook_signal_send ("buffer_line_added",
                      WEECHAT_HOOK_SIGNAL_POINTER, new_line);

    return new_line;
}

/*
 * Adds or updates a line for a buffer with free content.
 */

void
gui_line_add_y (struct t_gui_buffer *buffer, int y, const char *message)
{
    struct t_gui_line *ptr_line, *new_line;
    struct t_gui_line_data *new_line_data;

    /* search if line exists for "y" */
    for (ptr_line = buffer->own_lines->first_line; ptr_line;
         ptr_line = ptr_line->next_line)
    {
        if (ptr_line->data->y >= y)
            break;
    }

    if (!ptr_line || (ptr_line->data->y > y))
    {
        new_line = malloc (sizeof (*new_line));
        if (!new_line)
        {
            log_printf (_("Not enough memory for new line"));
            return;
        }

        new_line_data = malloc (sizeof (*(new_line->data)));
        if (!new_line_data)
        {
            free (new_line);
            log_printf (_("Not enough memory for new line"));
            return;
        }
        new_line->data = new_line_data;

        buffer->own_lines->lines_count++;

        /* fill data in new line */
        new_line->data->buffer = buffer;
        new_line->data->y = y;
        new_line->data->date = 0;
        new_line->data->date_printed = 0;
        new_line->data->str_time = NULL;
        new_line->data->tags_count = 0;
        new_line->data->tags_array = NULL;
        new_line->data->refresh_needed = 1;
        new_line->data->prefix = NULL;
        new_line->data->prefix_length = 0;
        new_line->data->message = NULL;
        new_line->data->highlight = 0;

        /* add line to lines list */
        if (ptr_line)
        {
            /* add before line found */
            new_line->prev_line = ptr_line->prev_line;
            new_line->next_line = ptr_line;
            if (ptr_line->prev_line)
                (ptr_line->prev_line)->next_line = new_line;
            else
                buffer->own_lines->first_line = new_line;
            ptr_line->prev_line = new_line;
        }
        else
        {
            /* add at end of list */
            new_line->prev_line = buffer->own_lines->last_line;
            if (buffer->own_lines->first_line)
                buffer->own_lines->last_line->next_line = new_line;
            else
                buffer->own_lines->first_line = new_line;
            buffer->own_lines->last_line = new_line;
            new_line->next_line = NULL;
        }

        ptr_line = new_line;
    }

    /* set message for line */
    if (ptr_line->data->message)
        free (ptr_line->data->message);
    ptr_line->data->message = (message) ? strdup (message) : strdup ("");

    /* check if line is filtered or not */
    ptr_line->data->displayed = gui_filter_check_line (ptr_line);
    if (!ptr_line->data->displayed)
    {
        if (!buffer->own_lines->lines_hidden)
        {
            buffer->own_lines->lines_hidden = 1;
            hook_signal_send ("buffer_lines_hidden",
                              WEECHAT_HOOK_SIGNAL_POINTER, buffer);
        }
    }

    ptr_line->data->refresh_needed = 1;
}

/*
 * Clears prefix and message on a line (used on buffers with free content only).
 */

void
gui_line_clear (struct t_gui_line *line)
{
    if (line->data->prefix)
        free (line->data->prefix);
    line->data->prefix = strdup ("");

    if (line->data->message)
        free (line->data->message);
    line->data->message = strdup ("");
}

/*
 * Mixes lines of a buffer (or group of buffers) with a new buffer.
 */

void
gui_line_mix_buffers (struct t_gui_buffer *buffer)
{
    struct t_gui_buffer *ptr_buffer, *ptr_buffer_found;
    struct t_gui_lines *new_lines;
    struct t_gui_line *ptr_line1, *ptr_line2;

    /* search first other buffer with same number */
    ptr_buffer_found = NULL;
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if ((ptr_buffer != buffer) && (ptr_buffer->number == buffer->number))
        {
            ptr_buffer_found = ptr_buffer;
            break;
        }
    }
    if (!ptr_buffer_found)
        return;

    /* mix all lines (sorting by date) to a new structure "new_lines" */
    new_lines = gui_lines_alloc ();
    if (!new_lines)
        return;
    ptr_line1 = ptr_buffer_found->lines->first_line;
    ptr_line2 = buffer->lines->first_line;
    while (ptr_line1 || ptr_line2)
    {
        if (!ptr_line1)
        {
            gui_line_mixed_add (new_lines, ptr_line2->data);
            ptr_line2 = ptr_line2->next_line;
        }
        else
        {
            if (!ptr_line2)
            {
                gui_line_mixed_add (new_lines, ptr_line1->data);
                ptr_line1 = ptr_line1->next_line;
            }
            else
            {
                /* look for older line by comparing time */
                if (ptr_line1->data->date <= ptr_line2->data->date)
                {
                    while (ptr_line1
                           && (ptr_line1->data->date <= ptr_line2->data->date))
                    {
                        gui_line_mixed_add (new_lines, ptr_line1->data);
                        ptr_line1 = ptr_line1->next_line;
                    }
                }
                else
                {
                    while (ptr_line2
                           && (ptr_line1->data->date > ptr_line2->data->date))
                    {
                        gui_line_mixed_add (new_lines, ptr_line2->data);
                        ptr_line2 = ptr_line2->next_line;
                    }
                }
            }
        }
    }

    /* compute "prefix_max_length" for mixed lines */
    gui_line_compute_prefix_max_length (new_lines);

    /* compute "buffer_max_length" for mixed lines */
    gui_line_compute_buffer_max_length (buffer, new_lines);

    /* free old mixed lines */
    if (ptr_buffer_found->mixed_lines)
    {
        gui_line_mixed_free_all (ptr_buffer_found);
        free (ptr_buffer_found->mixed_lines);
    }

    /* use new structure with mixed lines in all buffers with correct number */
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->number == buffer->number)
        {
            ptr_buffer->mixed_lines = new_lines;
            ptr_buffer->lines = ptr_buffer->mixed_lines;
        }
    }
}

/*
 * Returns hdata for lines.
 */

struct t_hdata *
gui_line_hdata_lines_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, NULL, NULL, 0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_lines, first_line, POINTER, 0, NULL, "line");
        HDATA_VAR(struct t_gui_lines, last_line, POINTER, 0, NULL, "line");
        HDATA_VAR(struct t_gui_lines, last_read_line, POINTER, 0, NULL, "line");
        HDATA_VAR(struct t_gui_lines, lines_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_lines, first_line_not_read, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_lines, lines_hidden, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_lines, buffer_max_length, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_lines, prefix_max_length, INTEGER, 0, NULL, NULL);
    }
    return hdata;
}

/*
 * Returns hdata for line.
 */

struct t_hdata *
gui_line_hdata_line_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_line", "next_line",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_line, data, POINTER, 0, NULL, "line_data");
        HDATA_VAR(struct t_gui_line, prev_line, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_line, next_line, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Callback for updating data of a line.
 */

int
gui_line_hdata_line_data_update_cb (void *data,
                                    struct t_hdata *hdata,
                                    void *pointer,
                                    struct t_hashtable *hashtable)
{
    const char *value;
    struct t_gui_line_data *line_data;
    int rc;

    /* make C compiler happy */
    (void) data;

    line_data = (struct t_gui_line_data *)pointer;

    rc = 0;

    if (hashtable_has_key (hashtable, "date"))
    {
        value = hashtable_get (hashtable, "date");
        if (value)
        {
            hdata_set (hdata, pointer, "date", value);
            if (line_data->str_time)
                free (line_data->str_time);
            line_data->str_time = gui_chat_get_time_string (line_data->date);
            rc++;
        }
    }

    if (hashtable_has_key (hashtable, "date_printed"))
    {
        value = hashtable_get (hashtable, "date_printed");
        if (value)
        {
            hdata_set (hdata, pointer, "date_printed", value);
            rc++;
        }
    }

    if (hashtable_has_key (hashtable, "tags_array"))
    {
        value = hashtable_get (hashtable, "tags_array");
        if (line_data->tags_array)
            string_free_split (line_data->tags_array);
        if (value)
        {
            line_data->tags_array = string_split (value, ",", 0, 0,
                                                  &line_data->tags_count);
        }
        else
        {
            line_data->tags_count = 0;
            line_data->tags_array = NULL;
        }
        rc++;
    }

    if (hashtable_has_key (hashtable, "prefix"))
    {
        value = hashtable_get (hashtable, "prefix");
        hdata_set (hdata, pointer, "prefix", value);
        line_data->prefix_length = (line_data->prefix) ?
            gui_chat_strlen_screen (line_data->prefix) : 0;
        gui_line_compute_prefix_max_length (line_data->buffer->lines);
        rc++;
    }

    if (hashtable_has_key (hashtable, "message"))
    {
        value = hashtable_get (hashtable, "message");
        hdata_set (hdata, pointer, "message", value);
        rc++;
    }

    gui_buffer_ask_chat_refresh (line_data->buffer, 1);

    return rc;
}

/*
 * Returns hdata for line data.
 */

struct t_hdata *
gui_line_hdata_line_data_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, NULL, NULL,
                       0, 0, &gui_line_hdata_line_data_update_cb, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_line_data, buffer, POINTER, 0, NULL, "buffer");
        HDATA_VAR(struct t_gui_line_data, y, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, date, TIME, 1, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, date_printed, TIME, 1, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, str_time, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, tags_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, tags_array, STRING, 1, "tags_count", NULL);
        HDATA_VAR(struct t_gui_line_data, displayed, CHAR, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, highlight, CHAR, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, refresh_needed, CHAR, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, prefix, STRING, 1, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, prefix_length, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, message, STRING, 1, NULL, NULL);
    }
    return hdata;
}

/*
 * Adds a line in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_line_add_to_infolist (struct t_infolist *infolist,
                          struct t_gui_lines *lines,
                          struct t_gui_line *line)
{
    struct t_infolist_item *ptr_item;
    int i, length;
    char option_name[64], *tags;

    if (!infolist || !line)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_integer (ptr_item, "y", line->data->y))
        return 0;
    if (!infolist_new_var_time (ptr_item, "date", line->data->date))
        return 0;
    if (!infolist_new_var_time (ptr_item, "date_printed", line->data->date_printed))
        return 0;
    if (!infolist_new_var_string (ptr_item, "str_time", line->data->str_time))
        return 0;

    /* write tags */
    if (!infolist_new_var_integer (ptr_item, "tags_count", line->data->tags_count))
        return 0;
    length = 0;
    for (i = 0; i < line->data->tags_count; i++)
    {
        snprintf (option_name, sizeof (option_name), "tag_%05d", i + 1);
        if (!infolist_new_var_string (ptr_item, option_name,
                                      line->data->tags_array[i]))
            return 0;
        length += strlen (line->data->tags_array[i]) + 1;
    }
    tags = malloc (length + 1);
    if (!tags)
        return 0;
    tags[0] = '\0';
    for (i = 0; i < line->data->tags_count; i++)
    {
        strcat (tags, line->data->tags_array[i]);
        if (i < line->data->tags_count - 1)
            strcat (tags, ",");
    }
    if (!infolist_new_var_string (ptr_item, "tags", tags))
    {
        free (tags);
        return 0;
    }
    free (tags);

    if (!infolist_new_var_integer (ptr_item, "displayed", line->data->displayed))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "highlight", line->data->highlight))
        return 0;
    if (!infolist_new_var_string (ptr_item, "prefix", line->data->prefix))
        return 0;
    if (!infolist_new_var_string (ptr_item, "message", line->data->message))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "last_read_line",
                                   (lines->last_read_line == line) ? 1 : 0))
        return 0;

    return 1;
}

/*
 * Prints lines structure infos in WeeChat log file (usually for crash dump).
 */

void
gui_lines_print_log (struct t_gui_lines *lines)
{
    if (lines)
    {
        log_printf ("    first_line. . . . . . : 0x%lx", lines->first_line);
        log_printf ("    last_line . . . . . . : 0x%lx", lines->last_line);
        log_printf ("    last_read_line. . . . : 0x%lx", lines->last_read_line);
        log_printf ("    lines_count . . . . . : %d",    lines->lines_count);
        log_printf ("    first_line_not_read . : %d",    lines->first_line_not_read);
        log_printf ("    lines_hidden. . . . . : %d",    lines->lines_hidden);
        log_printf ("    buffer_max_length . . : %d",    lines->buffer_max_length);
        log_printf ("    prefix_max_length . . : %d",    lines->prefix_max_length);
    }
}
