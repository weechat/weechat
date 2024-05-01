/*
 * gui-line.c - line functions (used by all GUI)
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
#include <time.h>
#include <regex.h>

#include "../core/weechat.h"
#include "../core/core-config.h"
#include "../core/core-hashtable.h"
#include "../core/core-hdata.h"
#include "../core/core-hook.h"
#include "../core/core-infolist.h"
#include "../core/core-log.h"
#include "../core/core-string.h"
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
gui_line_lines_alloc ()
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
        new_lines->buffer_max_length_refresh = 0;
        new_lines->prefix_max_length = CONFIG_INTEGER(config_look_prefix_align_min);
        new_lines->prefix_max_length_refresh = 0;
    }

    return new_lines;
}

/*
 * Frees a "t_gui_lines" structure.
 */

void
gui_line_lines_free (struct t_gui_lines *lines)
{
    if (!lines)
        return;

    free (lines);
}

/*
 * Allocates array with tags in a line_data.
 */

void
gui_line_tags_alloc (struct t_gui_line_data *line_data, const char *tags)
{
    if (!line_data)
        return;

    if (tags)
    {
        line_data->tags_array = string_split_shared (tags, ",", NULL, 0, 0,
                                                     &line_data->tags_count);
    }
    else
    {
        line_data->tags_count = 0;
        line_data->tags_array = NULL;
    }
}

/*
 * Frees array with tags in a line_data.
 */

void
gui_line_tags_free (struct t_gui_line_data *line_data)
{
    if (!line_data)
        return;

    if (line_data->tags_array)
    {
        string_free_split_shared (line_data->tags_array);
        line_data->tags_count = 0;
        line_data->tags_array = NULL;
    }
}

/*
 * Checks if prefix on line is a nick and is the same as nick on previous/next
 * line (according to direction: if < 0, check if it's the same nick as
 * previous line, otherwise next line).
 *
 * Returns:
 *   1: prefix is a nick and same as nick on previous/next line
 *   0: prefix is not a nick, or different from nick on previous/next line
 */

int
gui_line_prefix_is_same_nick (struct t_gui_line *line, int direction)
{
    const char *nick, *nick_other;
    struct t_gui_line *other_line;

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
    other_line = (direction < 0) ?
        gui_line_get_prev_displayed (line) :
        gui_line_get_next_displayed (line);
    if (!other_line)
        return 0;

    /* buffer is not the same as the other line => display standard prefix */
    if (line->data->buffer != other_line->data->buffer)
        return 0;

    /*
     * the other line does not have a tag beginning with "prefix_nick"
     * => display standard prefix
     */
    if (!gui_line_search_tag_starting_with (other_line, "prefix_nick"))
        return 0;

    /* no nick on previous line => display standard prefix */
    nick_other = gui_line_get_nick_tag (other_line);
    if (!nick_other)
        return 0;

    /* prefix can be hidden/replaced if nicks are equal */
    return (strcmp (nick, nick_other) == 0) ? 1 : 0;
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
                                 char **color, int *prefix_is_nick)
{
    const char *tag_prefix_nick;

    if (CONFIG_STRING(config_look_prefix_same_nick)
        && CONFIG_STRING(config_look_prefix_same_nick)[0]
        && gui_line_prefix_is_same_nick (line, -1))
    {
        if (CONFIG_STRING(config_look_prefix_same_nick_middle)
            && CONFIG_STRING(config_look_prefix_same_nick_middle)[0]
            && gui_line_prefix_is_same_nick (line, 1))
        {
            /* same nick: return empty prefix or value from option */
            if (strcmp (CONFIG_STRING(config_look_prefix_same_nick_middle), " ") == 0)
            {
                /* return empty prefix */
                if (prefix)
                    *prefix = gui_chat_prefix_empty;
                if (length)
                    *length = 0;
                if (color)
                    *color = NULL;
            }
            else
            {
                /* return prefix from option "weechat.look.prefix_same_nick_middle" */
                if (prefix)
                    *prefix = CONFIG_STRING(config_look_prefix_same_nick_middle);
                if (length)
                    *length = config_length_prefix_same_nick_middle;
                if (color)
                {
                    tag_prefix_nick = gui_line_search_tag_starting_with (line,
                                                                         "prefix_nick_");
                    *color = (tag_prefix_nick) ? (char *)(tag_prefix_nick + 12) : NULL;
                }
            }
            if (prefix_is_nick)
                *prefix_is_nick = 0;
        }
        else
        {
            /* same nick: return empty prefix or value from option */
            if (strcmp (CONFIG_STRING(config_look_prefix_same_nick), " ") == 0)
            {
                /* return empty prefix */
                if (prefix)
                    *prefix = gui_chat_prefix_empty;
                if (length)
                    *length = 0;
                if (color)
                    *color = NULL;
            }
            else
            {
                /* return prefix from option "weechat.look.prefix_same_nick" */
                if (prefix)
                    *prefix = CONFIG_STRING(config_look_prefix_same_nick);
                if (length)
                    *length = config_length_prefix_same_nick;
                if (color)
                {
                    tag_prefix_nick = gui_line_search_tag_starting_with (line,
                                                                         "prefix_nick_");
                    *color = (tag_prefix_nick) ? (char *)(tag_prefix_nick + 12) : NULL;
                }
            }
            if (prefix_is_nick)
                *prefix_is_nick = 0;
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
        if (prefix_is_nick)
            *prefix_is_nick = gui_line_search_tag_starting_with (line, "prefix_nick_") ? 1 : 0;
    }
}

/*
 * Gets alignment for a line.
 */

int
gui_line_get_align (struct t_gui_buffer *buffer, struct t_gui_line *line,
                    int with_suffix, int first_line)
{
    int length_time, length_buffer, length_suffix, prefix_length, prefix_is_nick;

    /* return immediately if buffer has free content (no alignment) */
    if (buffer->type == GUI_BUFFER_TYPE_FREE)
        return 0;

    /* return immediately if line has no time (not aligned) */
    if (line->data->date == 0)
        return 0;

    /* return immediately if alignment for end of lines is "time" */
    if (!first_line
        && (CONFIG_ENUM(config_look_align_end_of_lines) == CONFIG_LOOK_ALIGN_END_OF_LINES_TIME))
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
        && (CONFIG_ENUM(config_look_align_end_of_lines) == CONFIG_LOOK_ALIGN_END_OF_LINES_BUFFER))
    {
        return length_time;
    }

    /* length of buffer name (when many buffers are merged) */
    if (buffer->mixed_lines && (buffer->active != 2))
    {
        if ((CONFIG_ENUM(config_look_prefix_buffer_align) == CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE)
            && (CONFIG_ENUM(config_look_prefix_align) == CONFIG_LOOK_PREFIX_ALIGN_NONE))
            length_buffer = gui_chat_strlen_screen (gui_buffer_get_short_name (line->data->buffer)) + 1;
        else
        {
            if (CONFIG_ENUM(config_look_prefix_buffer_align) == CONFIG_LOOK_PREFIX_BUFFER_ALIGN_NONE)
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
        && (CONFIG_ENUM(config_look_align_end_of_lines) == CONFIG_LOOK_ALIGN_END_OF_LINES_PREFIX))
    {
        return length_time + length_buffer;
    }

    /* length of prefix */
    gui_line_get_prefix_for_display (line, NULL, &prefix_length, NULL,
                                     &prefix_is_nick);
    if (prefix_is_nick)
        prefix_length += config_length_nick_prefix_suffix;

    if (CONFIG_ENUM(config_look_prefix_align) == CONFIG_LOOK_PREFIX_ALIGN_NONE)
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
 * Builds a string with prefix and message.
 *
 * Note: result must be freed after use.
 */

char *
gui_line_build_string_prefix_message (const char *prefix, const char *message)
{
    char **string, *string_without_colors;

    string = string_dyn_alloc (256);
    if (!string)
        return NULL;

    if (prefix)
        string_dyn_concat (string, prefix, -1);
    string_dyn_concat (string, "\t", -1);
    if (message)
        string_dyn_concat (string, message, -1);

    string_without_colors = gui_color_decode (*string, NULL);

    string_dyn_free (string, 1);

    return string_without_colors;
}

/*
 * Builds a string with action message and nick with nick offline color.
 *
 * Note: result must be freed after use.
 */

char *
gui_line_build_string_message_nick_offline (const char *message)
{
    const char *ptr_message;
    char *message2;

    if (!message)
        return NULL;

    ptr_message = gui_chat_string_next_char (NULL, NULL,
                                             (unsigned char *)message,
                                             0, 0, 0);
    if (!ptr_message)
        return strdup ("");

    if (string_asprintf (&message2, "%s%s",
                         GUI_COLOR(GUI_COLOR_CHAT_NICK_OFFLINE),
                         ptr_message) >= 0)
    {
        return message2;
    }

    return NULL;
}

/*
 * Builds a string with message and tags.
 *
 * If colors == 1, keep colors in message and use color for delimiters around
 * tags.
 * If colors == 0, strip colors from message and do not use color for
 * delimiters around tags.
 *
 * Note: result must be freed after use.
 */

char *
gui_line_build_string_message_tags (const char *message,
                                    int tags_count, char **tags_array,
                                    int colors)
{
    int i;
    char **string, *message_no_colors, *result;

    if ((tags_count < 0) || ((tags_count > 0) && !tags_array))
        return NULL;

    string = string_dyn_alloc (256);
    if (!string)
        return NULL;

    if (message)
    {
        if (colors)
        {
            string_dyn_concat (string, message, -1);
        }
        else
        {
            message_no_colors = gui_color_decode (message, NULL);
            if (message_no_colors)
            {
                string_dyn_concat (string, message_no_colors, -1);
                free (message_no_colors);
            }
        }
    }
    if (colors)
        string_dyn_concat (string, GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS), -1);
    string_dyn_concat (string, " [", -1);
    if (colors)
        string_dyn_concat (string, GUI_COLOR(GUI_COLOR_CHAT_TAGS), -1);
    for (i = 0; i < tags_count; i++)
    {
        string_dyn_concat (string, tags_array[i], -1);
        if (i < tags_count - 1)
            string_dyn_concat (string, ",", -1);
    }
    if (colors)
        string_dyn_concat (string, GUI_COLOR(GUI_COLOR_CHAT_DELIMITERS), -1);
    string_dyn_concat (string, "]", -1);

    result = strdup (*string);

    string_dyn_free (string, 1);

    return result;
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
    if (!line)
        return 0;

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
gui_line_search_text (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    char *prefix, *message;
    int rc;

    if (!line || !line->data->message
        || !buffer->input_buffer || !buffer->input_buffer[0])
    {
        return 0;
    }

    rc = 0;

    if ((buffer->text_search_where & GUI_BUFFER_SEARCH_IN_PREFIX)
        && line->data->prefix)
    {
        prefix = gui_color_decode (line->data->prefix, NULL);
        if (prefix)
        {
            if (buffer->text_search_regex)
            {
                if (buffer->text_search_regex_compiled)
                {
                    if (regexec (buffer->text_search_regex_compiled,
                                 prefix, 0, NULL, 0) == 0)
                    {
                        rc = 1;
                    }
                }
            }
            else if ((buffer->text_search_exact
                      && (strstr (prefix, buffer->input_buffer)))
                     || (!buffer->text_search_exact
                         && (string_strcasestr (prefix, buffer->input_buffer))))
            {
                rc = 1;
            }
            free (prefix);
        }
    }

    if (!rc && (buffer->text_search_where & GUI_BUFFER_SEARCH_IN_MESSAGE))
    {
        if (gui_chat_display_tags)
        {
            message = gui_line_build_string_message_tags (
                line->data->message,
                line->data->tags_count,
                line->data->tags_array,
                0);
        }
        else
        {
            message = gui_color_decode (line->data->message, NULL);
        }
        if (message)
        {
            if (buffer->text_search_regex)
            {
                if (buffer->text_search_regex_compiled)
                {
                    if (regexec (buffer->text_search_regex_compiled,
                                 message, 0, NULL, 0) == 0)
                    {
                        rc = 1;
                    }
                }
            }
            else if ((buffer->text_search_exact
                      && (strstr (message, buffer->input_buffer)))
                     || (!buffer->text_search_exact
                         && (string_strcasestr (message, buffer->input_buffer))))
            {
                rc = 1;
            }
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
gui_line_match_regex (struct t_gui_line_data *line_data, regex_t *regex_prefix,
                      regex_t *regex_message)
{
    char *prefix, *message;
    int match_prefix, match_message;

    if (!line_data || (!regex_prefix && !regex_message))
        return 0;

    prefix = NULL;
    message = NULL;

    match_prefix = 1;
    match_message = 1;

    if (line_data->prefix)
    {
        prefix = gui_color_decode (line_data->prefix, NULL);
        if (!prefix
            || (regex_prefix && (regexec (regex_prefix, prefix, 0, NULL, 0) != 0)))
            match_prefix = 0;
    }
    else
    {
        if (regex_prefix)
            match_prefix = 0;
    }

    if (line_data->message)
    {
        message = gui_color_decode (line_data->message, NULL);
        if (!message
            || (regex_message && (regexec (regex_message, message, 0, NULL, 0) != 0)))
            match_message = 0;
    }
    else
    {
        if (regex_message)
            match_message = 0;
    }

    free (prefix);
    free (message);

    return (match_prefix && match_message);
}

/*
 * Checks if a line has tag "no_filter" (which means that line should never been
 * filtered: it is always displayed).
 *
 * Returns:
 *   1: line has tag "no_filter"
 *   0: line does not have tag "no_filter"
 */

int
gui_line_has_tag_no_filter (struct t_gui_line_data *line_data)
{
    int i;

    if (!line_data)
        return 0;

    for (i = 0; i < line_data->tags_count; i++)
    {
        if (strcmp (line_data->tags_array[i], GUI_FILTER_TAG_NO_FILTER) == 0)
            return 1;
    }

    /* tag not found, line may be filtered */
    return 0;
}

/*
 * Checks if line matches tags.
 *
 * Returns:
 *   1: line matches tags
 *   0: line does not match tags
 */

int
gui_line_match_tags (struct t_gui_line_data *line_data,
                     int tags_count, char ***tags_array)
{
    int i, j, k, match, tag_found, tag_negated;
    const char *ptr_tag;

    if (!line_data)
        return 0;

    for (i = 0; i < tags_count; i++)
    {
        match = 1;
        for (j = 0; tags_array[i][j]; j++)
        {
            ptr_tag = tags_array[i][j];
            tag_found = 0;
            tag_negated = 0;

            /* check if tag is negated (prefixed with a '!') */
            if ((ptr_tag[0] == '!') && ptr_tag[1])
            {
                ptr_tag++;
                tag_negated = 1;
            }

            if (strcmp (ptr_tag, "*") == 0)
            {
                tag_found = 1;
            }
            else
            {
                for (k = 0; k < line_data->tags_count; k++)
                {
                    if (string_match (line_data->tags_array[k], ptr_tag, 0))
                    {
                        tag_found = 1;
                        break;
                    }
                }
            }
            if (tag_found && tag_negated)
                return 0;
            if ((!tag_found && !tag_negated) || (tag_found && tag_negated))
            {
                match = 0;
                break;
            }
        }
        if (match)
            return 1;
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

    if (!line)
        return NULL;

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
    int rc, rc_regex, i, no_highlight, action, length;
    char *msg_no_color, *ptr_msg_no_color, *highlight_words;
    const char *ptr_nick;
    regmatch_t regex_match;

    /* remove color codes from line message */
    msg_no_color = gui_color_decode (line->data->message, NULL);
    if (!msg_no_color)
    {
        rc = 0;
        goto end;
    }
    ptr_msg_no_color = msg_no_color;

    /*
     * highlights are disabled on this buffer? (special value "-" means that
     * buffer does not want any highlight)
     */
    if (line->data->buffer->highlight_words
        && (strcmp (line->data->buffer->highlight_words, "-") == 0))
    {
        rc = 0;
        goto end;
    }

    /*
     * check if highlight is disabled for line; also check if the line is an
     * action message (for example tag "irc_action") and get pointer on the nick
     * (tag "nick_xxx"), these info will be used later (see below)
     */
    no_highlight = 0;
    action = 0;
    ptr_nick = NULL;
    for (i = 0; i < line->data->tags_count; i++)
    {
        if (strcmp (line->data->tags_array[i], GUI_CHAT_TAG_NO_HIGHLIGHT) == 0)
            no_highlight = 1;
        else if (strncmp (line->data->tags_array[i], "nick_", 5) == 0)
            ptr_nick = line->data->tags_array[i] + 5;
        else
        {
            length = strlen (line->data->tags_array[i]);
            if ((length >= 7)
                && (strcmp (line->data->tags_array[i] + length - 7, "_action") == 0))
            {
                action = 1;
            }
        }
    }
    if (no_highlight)
    {
        rc = 0;
        goto end;
    }

    /*
     * if the line is an action message and that we know the nick, we skip
     * the nick if it is at beginning of message (to not highlight an action
     * from another user if his nick is in our highlight settings)
     */
    if (action && ptr_nick)
    {
        length = strlen (ptr_nick);
        if (strncmp (ptr_msg_no_color, ptr_nick, length) == 0)
        {
            /* skip nick at beginning (for example: "FlashCode") */
            ptr_msg_no_color += length;
        }
        else if (ptr_msg_no_color[0]
                 && (strncmp (ptr_msg_no_color + 1, ptr_nick, length) == 0))
        {
            /* skip prefix and nick at beginning (for example: "@FlashCode") */
            ptr_msg_no_color += length + 1;
        }
    }

    /*
     * check if highlight is disabled by a regex
     * (with global option "weechat.look.highlight_disable_regex")
     */
    if (config_highlight_disable_regex)
    {
        rc_regex = regexec (config_highlight_disable_regex,
                            ptr_msg_no_color, 1, &regex_match, 0);
        if ((rc_regex == 0) && (regex_match.rm_so >= 0) && (regex_match.rm_eo > 0))
        {
            rc = 0;
            goto end;
        }
    }

    /*
     * check if highlight is disabled by a regex
     * (with buffer property "highlight_disable_regex")
     */
    if (line->data->buffer->highlight_disable_regex_compiled)
    {
        rc_regex = regexec (line->data->buffer->highlight_disable_regex_compiled,
                            ptr_msg_no_color, 1, &regex_match, 0);
        if ((rc_regex == 0) && (regex_match.rm_so >= 0) && (regex_match.rm_eo > 0))
        {
            rc = 0;
            goto end;
        }
    }

    /*
     * check if highlight is forced by a tag
     * (with global option "weechat.look.highlight_tags")
     */
    if (config_highlight_tags
        && gui_line_match_tags (line->data,
                                config_num_highlight_tags,
                                config_highlight_tags))
    {
        rc = 1;
        goto end;
    }

    /*
     * check if highlight is forced by a tag
     * (with buffer property "highlight_tags")
     */
    if (line->data->buffer->highlight_tags
        && gui_line_match_tags (line->data,
                                line->data->buffer->highlight_tags_count,
                                line->data->buffer->highlight_tags_array))
    {
        rc = 1;
        goto end;
    }

    /*
     * check that line matches highlight tags, if any (if no tag is specified,
     * then any tag is allowed)
     */
    if (line->data->buffer->highlight_tags_restrict_count > 0)
    {
        if (!gui_line_match_tags (line->data,
                                  line->data->buffer->highlight_tags_restrict_count,
                                  line->data->buffer->highlight_tags_restrict_array))
        {
            rc = 0;
            goto end;
        }
    }

    /*
     * there is highlight on line if one of buffer highlight words matches line
     * or one of global highlight words matches line
     */
    highlight_words = gui_buffer_string_replace_local_var (line->data->buffer,
                                                           line->data->buffer->highlight_words);
    rc = string_has_highlight (ptr_msg_no_color,
                               (highlight_words) ?
                               highlight_words : line->data->buffer->highlight_words);
    free (highlight_words);
    if (rc)
        goto end;

    highlight_words = gui_buffer_string_replace_local_var (line->data->buffer,
                                                           CONFIG_STRING(config_look_highlight));
    rc = string_has_highlight (ptr_msg_no_color,
                               (highlight_words) ?
                               highlight_words : CONFIG_STRING(config_look_highlight));
    free (highlight_words);
    if (rc)
        goto end;

    if (config_highlight_regex)
    {
        rc = string_has_highlight_regex_compiled (ptr_msg_no_color,
                                                  config_highlight_regex);
    }
    if (rc)
        goto end;

    if (line->data->buffer->highlight_regex_compiled)
    {
        rc = string_has_highlight_regex_compiled (ptr_msg_no_color,
                                                  line->data->buffer->highlight_regex_compiled);
    }

end:
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

    if (!line)
        return 0;

    nick = gui_line_get_nick_tag (line);
    if (nick
        && (line->data->buffer->nicklist_root
            && (line->data->buffer->nicklist_root->nicks
                || line->data->buffer->nicklist_root->children))
        && !gui_nicklist_search_nick (line->data->buffer, NULL, nick))
    {
        return 1;
    }

    return 0;
}

/*
 * Checks if line is an action (eg: `/me` in irc plugin).
 *
 * Returns:
 *   1: line is an action
 *   0: line is not an action
 */

int
gui_line_is_action (struct t_gui_line *line)
{
    int i, length;

    for (i = 0; i < line->data->tags_count; i++)
    {
        length = strlen (line->data->tags_array[i]);
        if ((length >= 7)
            && (strcmp (line->data->tags_array[i] + length - 7, "_action") == 0))
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

    lines->buffer_max_length_refresh = 0;
}

/*
 * Computes "prefix_max_length" for a "t_gui_lines" structure.
 */

void
gui_line_compute_prefix_max_length (struct t_gui_lines *lines)
{
    struct t_gui_line *ptr_line;
    int prefix_length, prefix_is_nick;

    lines->prefix_max_length = CONFIG_INTEGER(config_look_prefix_align_min);

    for (ptr_line = lines->first_line; ptr_line;
         ptr_line = ptr_line->next_line)
    {
        if (ptr_line->data->displayed)
        {
            gui_line_get_prefix_for_display (ptr_line, NULL, &prefix_length,
                                             NULL, &prefix_is_nick);
            if (prefix_is_nick)
                prefix_length += config_length_nick_prefix_suffix;
            if (prefix_length > lines->prefix_max_length)
                lines->prefix_max_length = prefix_length;
        }
    }

    lines->prefix_max_length_refresh = 0;
}

/*
 * Adds a line to a "t_gui_lines" structure.
 */

void
gui_line_add_to_list (struct t_gui_lines *lines,
                      struct t_gui_line *line)
{
    int prefix_length, prefix_is_nick;

    if (lines->last_line)
        (lines->last_line)->next_line = line;
    else
        lines->first_line = line;
    line->prev_line = lines->last_line;
    line->next_line = NULL;
    lines->last_line = line;

    /*
     * adjust "prefix_max_length" if this prefix length is > max
     * (only if the line is displayed
     */
    if (line->data->displayed)
    {
        gui_line_get_prefix_for_display (line, NULL, &prefix_length, NULL,
                                         &prefix_is_nick);
        if (prefix_is_nick)
            prefix_length += config_length_nick_prefix_suffix;
        if (prefix_length > lines->prefix_max_length)
            lines->prefix_max_length = prefix_length;
    }
    else
    {
        /* adjust "lines_hidden" if the line is hidden */
        (lines->lines_hidden)++;
    }

    lines->lines_count++;
}

/*
 * Frees data in a line.
 */

void
gui_line_free_data (struct t_gui_line *line)
{
    free (line->data->str_time);
    gui_line_tags_free (line->data);
    string_shared_free (line->data->prefix);
    free (line->data->message);
    free (line->data);

    line->data = NULL;
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
    int prefix_length, prefix_is_nick;

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
                if (ptr_scroll->start_line)
                {
                    gui_buffer_ask_chat_refresh (buffer, 2);
                }
                else
                {
                    ptr_scroll->first_line_displayed = 1;
                    ptr_scroll->scrolling = 0;
                    ptr_scroll->lines_after = 0;
                    gui_window_ask_refresh (1);
                }
            }

            if (ptr_scroll->text_search_start_line == line)
                ptr_scroll->text_search_start_line = NULL;
        }
        /* remove line from coords */
        gui_window_coords_remove_line (ptr_win, line);
    }

    gui_line_get_prefix_for_display (line, NULL, &prefix_length, NULL,
                                     &prefix_is_nick);
    if (prefix_is_nick)
        prefix_length += config_length_nick_prefix_suffix;
    if (prefix_length == lines->prefix_max_length)
        lines->prefix_max_length_refresh = 1;

    /* move read marker if it was on line we are removing */
    if (lines->last_read_line == line)
    {
        lines->last_read_line = lines->last_read_line->prev_line;
        lines->first_line_not_read = (lines->last_read_line) ? 0 : 1;
        gui_buffer_ask_chat_refresh (buffer, 1);
    }

    /* adjust "lines_hidden" if the line was hidden */
    if (!line->data->displayed && (lines->lines_hidden > 0))
        (lines->lines_hidden)--;

    /* free data */
    if (free_data)
        gui_line_free_data (line);

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

    if (!buffer || !line)
        return;

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
 * Gets max notify level for a line, according to the nick.
 *
 * Returns max notify level, between -1 and GUI_HOTLIST_HIGHLIGHT.
 */

int
gui_line_get_max_notify_level (struct t_gui_line *line)
{
    int max_notify_level, *ptr_max_notify_level;
    const char *nick;

    max_notify_level = GUI_HOTLIST_HIGHLIGHT;

    nick = gui_line_get_nick_tag (line);
    if (nick)
    {
        ptr_max_notify_level = hashtable_get (
            line->data->buffer->hotlist_max_level_nicks,
            nick);
        if (ptr_max_notify_level)
            max_notify_level = *ptr_max_notify_level;
    }

    return max_notify_level;
}

/*
 * Sets the notify level in a line:
 *   -1: no notify at all
 *    0: low (GUI_HOTLIST_LOW)
 *    1: message (GUI_HOTLIST_MESSAGE)
 *    2: private message (GUI_HOTLIST_PRIVATE)
 *    3: message with highlight (GUI_HOTLIST_HIGHLIGHT)
 */

void
gui_line_set_notify_level (struct t_gui_line *line, int max_notify_level)
{
    int i;

    line->data->notify_level = GUI_HOTLIST_LOW;

    for (i = 0; i < line->data->tags_count; i++)
    {
        if (strcmp (line->data->tags_array[i], "notify_none") == 0)
            line->data->notify_level = -1;
        if (strcmp (line->data->tags_array[i], "notify_message") == 0)
            line->data->notify_level = GUI_HOTLIST_MESSAGE;
        if (strcmp (line->data->tags_array[i], "notify_private") == 0)
            line->data->notify_level = GUI_HOTLIST_PRIVATE;
        if (strcmp (line->data->tags_array[i], "notify_highlight") == 0)
            line->data->notify_level = GUI_HOTLIST_HIGHLIGHT;
    }

    if (line->data->notify_level > max_notify_level)
        line->data->notify_level = max_notify_level;
}

/*
 * Sets highlight flag in a line:
 *   0: no highlight
 *   1: highlight
 */

void
gui_line_set_highlight (struct t_gui_line *line, int max_notify_level)
{
    if (line->data->notify_level == GUI_HOTLIST_HIGHLIGHT)
        line->data->highlight = 1;
    else
    {
        line->data->highlight = (max_notify_level == GUI_HOTLIST_HIGHLIGHT) ?
            gui_line_has_highlight (line) : 0;
    }
}

/*
 * Creates a new line for a buffer.
 */

struct t_gui_line *
gui_line_new (struct t_gui_buffer *buffer, int y,
              time_t date, int date_usec,
              time_t date_printed, int date_usec_printed,
              const char *tags,
              const char *prefix, const char *message)
{
    struct t_gui_line *new_line;
    struct t_gui_line_data *new_line_data;
    int max_notify_level;

    if (!buffer)
        return NULL;

    /* create new line */
    new_line = malloc (sizeof (*new_line));
    if (!new_line)
        return NULL;

    /* create data for line */
    new_line_data = malloc (sizeof (*new_line_data));
    if (!new_line_data)
    {
        free (new_line);
        return NULL;
    }
    new_line->data = new_line_data;

    /* fill data in new line */
    new_line->data->buffer = buffer;
    new_line->data->message = (message) ? strdup (message) : strdup ("");

    if (buffer->type == GUI_BUFFER_TYPE_FORMATTED)
    {
        /*
         * the line identifier is almost unique: when reaching INT_MAX, it is
         * reset to 0; it is extremely unlikely all integer are used in the
         * same buffer, that would mean the buffer has a huge number of lines;
         * when searching a line id in a buffer, it is recommended to start
         * from the last line and loop to the first
         */
        new_line->data->id = buffer->next_line_id;
        buffer->next_line_id = (buffer->next_line_id == INT_MAX) ?
            0 : buffer->next_line_id + 1;
        new_line->data->y = -1;
        new_line->data->date = date;
        new_line->data->date_usec = date_usec;
        new_line->data->date_printed = date_printed;
        new_line->data->date_usec_printed = date_usec_printed;
        gui_line_tags_alloc (new_line->data, tags);
        new_line->data->refresh_needed = 0;
        new_line->data->prefix = (prefix) ?
            (char *)string_shared_get (prefix) : ((date != 0) ? (char *)string_shared_get ("") : NULL);
        new_line->data->prefix_length = (prefix) ?
            gui_chat_strlen_screen (prefix) : 0;
        max_notify_level = gui_line_get_max_notify_level (new_line);
        gui_line_set_notify_level (new_line, max_notify_level);
        gui_line_set_highlight (new_line, max_notify_level);
        if (new_line->data->highlight && (new_line->data->notify_level >= 0))
            new_line->data->notify_level = GUI_HOTLIST_HIGHLIGHT;
        new_line->data->str_time = gui_chat_get_time_string (
            date, date_usec, new_line->data->highlight);
    }
    else
    {
        new_line->data->id = y;
        new_line->data->y = y;
        new_line->data->date = date;
        new_line->data->date_usec = date_usec;
        new_line->data->date_printed = date_printed;
        new_line->data->date_usec_printed = date_usec_printed;
        new_line->data->str_time = NULL;
        gui_line_tags_alloc (new_line->data, tags);
        new_line->data->refresh_needed = 1;
        new_line->data->prefix = NULL;
        new_line->data->prefix_length = 0;
        new_line->data->notify_level = 0;
        new_line->data->highlight = 0;
    }

    /* set display flag (check if line is filtered or not) */
    new_line->data->displayed = gui_filter_check_line (new_line->data);

    new_line->prev_line = NULL;
    new_line->next_line = NULL;

    return new_line;
}

/*
 * Updates data in a line via the hook_line.
 */

void
gui_line_hook_update (struct t_gui_line *line,
                      struct t_hashtable *hashtable,
                      struct t_hashtable *hashtable2)
{
    const char *ptr_value, *ptr_value2;
    struct t_gui_buffer *ptr_buffer;
    unsigned long value_pointer;
    long value;
    char *error, *new_message, *pos_newline;
    int rc, tags_updated, notify_level_updated, highlight_updated;
    int max_notify_level;

    tags_updated = 0;
    notify_level_updated = 0;
    highlight_updated = 0;

    ptr_value2 = hashtable_get (hashtable2, "buffer_name");
    if (ptr_value2)
    {
        if (ptr_value2[0])
        {
            ptr_buffer = gui_buffer_search_by_full_name (ptr_value2);
            if (gui_chat_buffer_valid (ptr_buffer, line->data->buffer->type))
                line->data->buffer = ptr_buffer;
        }
        else
        {
            line->data->buffer = NULL;
            return;
        }
    }
    else
    {
        ptr_value2 = hashtable_get (hashtable2, "buffer");
        if (ptr_value2)
        {
            if (ptr_value2[0])
            {
                if ((ptr_value2[0] == '0') && (ptr_value2[1] == 'x'))
                {
                    rc = sscanf (ptr_value2 + 2, "%lx", &value_pointer);
                    ptr_buffer = (struct t_gui_buffer *)value_pointer;
                    if ((rc != EOF) && (rc >= 1)
                        && gui_chat_buffer_valid (ptr_buffer, line->data->buffer->type))
                    {
                        line->data->buffer = ptr_buffer;
                    }
                }
            }
            else
            {
                line->data->buffer = NULL;
                return;
            }
        }
    }

    if (line->data->buffer->type == GUI_BUFFER_TYPE_FREE)
    {
        /* the field "y" can be changed on buffer with free content */
        ptr_value = hashtable_get (hashtable2, "y");
        if (ptr_value)
        {
            error = NULL;
            value = strtol (ptr_value, &error, 10);
            if (error && !error[0] && (value >= 0))
                line->data->y = value;
        }
    }

    ptr_value2 = hashtable_get (hashtable2, "notify_level");
    if (ptr_value2)
    {
        error = NULL;
        value = strtol (ptr_value2, &error, 10);
        if (error && !error[0] && (value >= -1) && (value <= GUI_HOTLIST_MAX))
        {
            notify_level_updated = 1;
            line->data->notify_level = value;
        }
    }

    ptr_value2 = hashtable_get (hashtable2, "highlight");
    if (ptr_value2)
    {
        error = NULL;
        value = strtol (ptr_value2, &error, 10);
        if (error && !error[0])
        {
            highlight_updated = 1;
            line->data->highlight = (value) ? 1 : 0;
        }
    }

    ptr_value2 = hashtable_get (hashtable2, "date");
    if (ptr_value2)
    {
        error = NULL;
        value = strtol (ptr_value2, &error, 10);
        if (error && !error[0] && (value >= 0))
        {
            line->data->date = (time_t)value;
            free (line->data->str_time);
            line->data->str_time = gui_chat_get_time_string (
                line->data->date,
                line->data->date_usec,
                line->data->highlight);
        }
    }

    ptr_value2 = hashtable_get (hashtable2, "date_usec");
    if (ptr_value2)
    {
        error = NULL;
        value = strtol (ptr_value2, &error, 10);
        if (error && !error[0] && (value >= 0) && (value <= 999999))
        {
            line->data->date_usec = (int)value;
            free (line->data->str_time);
            line->data->str_time = gui_chat_get_time_string (
                line->data->date,
                line->data->date_usec,
                line->data->highlight);
        }
    }

    ptr_value2 = hashtable_get (hashtable2, "date_printed");
    if (ptr_value2)
    {
        error = NULL;
        value = strtol (ptr_value2, &error, 10);
        if (error && !error[0] && (value >= 0))
            line->data->date_printed = (time_t)value;
    }

    ptr_value2 = hashtable_get (hashtable2, "date_usec_printed");
    if (ptr_value2)
    {
        error = NULL;
        value = strtol (ptr_value2, &error, 10);
        if (error && !error[0] && (value >= 0) && (value <= 999999))
            line->data->date_usec_printed = (int)value;
    }

    ptr_value = hashtable_get (hashtable, "str_time");
    ptr_value2 = hashtable_get (hashtable2, "str_time");
    if (ptr_value2 && (!ptr_value || (strcmp (ptr_value, ptr_value2) != 0)))
    {
        free (line->data->str_time);
        line->data->str_time = (ptr_value2) ? strdup (ptr_value2) : NULL;
    }

    ptr_value = hashtable_get (hashtable, "tags");
    ptr_value2 = hashtable_get (hashtable2, "tags");
    if (ptr_value2 && (!ptr_value || (strcmp (ptr_value, ptr_value2) != 0)))
    {
        tags_updated = 1;
        gui_line_tags_free (line->data);
        gui_line_tags_alloc (line->data, ptr_value2);
    }

    ptr_value = hashtable_get (hashtable, "prefix");
    ptr_value2 = hashtable_get (hashtable2, "prefix");
    if (ptr_value2 && (!ptr_value || (strcmp (ptr_value, ptr_value2) != 0)))
    {
        string_shared_free (line->data->prefix);
        line->data->prefix = (char *)string_shared_get (
            (ptr_value2) ? ptr_value2 : "");
        line->data->prefix_length = (line->data->prefix) ?
            gui_chat_strlen_screen (line->data->prefix) : 0;
    }

    ptr_value = hashtable_get (hashtable, "message");
    ptr_value2 = hashtable_get (hashtable2, "message");
    if (ptr_value2 && (!ptr_value || (strcmp (ptr_value, ptr_value2) != 0)))
    {
        new_message = strdup (ptr_value2);
        if (new_message && !line->data->buffer->input_multiline)
        {
            /* if input_multiline is not set, keep only first line */
            pos_newline = strchr (new_message, '\n');
            if (pos_newline)
                pos_newline[0] = '\0';
        }
        free (line->data->message);
        line->data->message = (new_message) ? strdup (new_message) : NULL;
        free (new_message);
    }

    max_notify_level = gui_line_get_max_notify_level (line);

    /* if tags were updated but not notify_level, adjust notify level */
    if (tags_updated && !notify_level_updated)
        gui_line_set_notify_level (line, max_notify_level);

    /* adjust flag "displayed" if tags were updated */
    if (tags_updated)
        line->data->displayed = gui_filter_check_line (line->data);

    if ((tags_updated || notify_level_updated) && !highlight_updated)
    {
        gui_line_set_highlight (line, max_notify_level);
        if (line->data->highlight && !notify_level_updated
            && (line->data->notify_level >= 0))
        {
            line->data->notify_level = GUI_HOTLIST_HIGHLIGHT;
        }
    }

    if (!notify_level_updated && highlight_updated && line->data->highlight
        && (line->data->notify_level >= 0))
    {
        line->data->notify_level = GUI_HOTLIST_HIGHLIGHT;
    }
}

/*
 * Adds a new line in a buffer with formatted content.
 */

void
gui_line_add (struct t_gui_line *line)
{
    struct t_gui_window *ptr_win;
    char *message_for_signal;
    int lines_removed;
    time_t current_time;

    /*
     * remove line(s) if necessary, according to history options:
     *   max_lines:   if > 0, keep only N lines in buffer
     *   max_minutes: if > 0, keep only lines from last N minutes
     */
    lines_removed = 0;
    current_time = time (NULL);
    while (line->data->buffer->own_lines->first_line
           && (((CONFIG_INTEGER(config_history_max_buffer_lines_number) > 0)
                && (line->data->buffer->own_lines->lines_count + 1 >
                    CONFIG_INTEGER(config_history_max_buffer_lines_number)))
               || ((CONFIG_INTEGER(config_history_max_buffer_lines_minutes) > 0)
                   && (current_time - line->data->buffer->own_lines->first_line->data->date_printed >
                       CONFIG_INTEGER(config_history_max_buffer_lines_minutes) * 60))))
    {
        gui_line_free (line->data->buffer,
                       line->data->buffer->own_lines->first_line);
        lines_removed++;
    }

    /* add line to lines list */
    gui_line_add_to_list (line->data->buffer->own_lines, line);

    /* update hotlist and/or send signals for line */
    if (line->data->displayed)
    {
        if ((line->data->notify_level >= GUI_HOTLIST_MIN)
            && line->data->highlight)
        {
            (void) gui_hotlist_add (
                line->data->buffer,
                GUI_HOTLIST_HIGHLIGHT,
                NULL,  /* creation_time */
                1);  /* check_conditions */
            if (!weechat_upgrading)
            {
                message_for_signal = gui_line_build_string_prefix_message (
                    line->data->prefix, line->data->message);
                if (message_for_signal)
                {
                    (void) hook_signal_send ("weechat_highlight",
                                             WEECHAT_HOOK_SIGNAL_STRING,
                                             message_for_signal);
                    free (message_for_signal);
                }
            }
        }
        else
        {
            if (!weechat_upgrading
                && (line->data->notify_level == GUI_HOTLIST_PRIVATE))
            {
                message_for_signal = gui_line_build_string_prefix_message (
                    line->data->prefix, line->data->message);
                if (message_for_signal)
                {
                    (void) hook_signal_send ("weechat_pv",
                                             WEECHAT_HOOK_SIGNAL_STRING,
                                             message_for_signal);
                    free (message_for_signal);
                }
            }
            if (line->data->notify_level >= GUI_HOTLIST_MIN)
            {
                (void) gui_hotlist_add (
                    line->data->buffer,
                    line->data->notify_level,
                    NULL,  /* creation_time */
                    1);  /* check_conditions */
            }
        }
    }
    else
    {
        (void) gui_buffer_send_signal (line->data->buffer,
                                       "buffer_lines_hidden",
                                       WEECHAT_HOOK_SIGNAL_POINTER,
                                       line->data->buffer);
    }

    /* add mixed line, if buffer is attached to at least one other buffer */
    if (line->data->buffer->mixed_lines)
    {
        gui_line_mixed_add (line->data->buffer->mixed_lines, line->data);
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
            if ((ptr_win->buffer == line->data->buffer)
                && (line->data->buffer->own_lines->lines_count < ptr_win->win_chat_height))
            {
                gui_buffer_ask_chat_refresh (line->data->buffer, 2);
                break;
            }
        }
    }

    (void) gui_buffer_send_signal (line->data->buffer,
                                   "buffer_line_added",
                                   WEECHAT_HOOK_SIGNAL_POINTER, line);
}

/*
 * Adds or updates a line in a buffer with free content.
 *
 * Ba careful: when replacing an existing line in the buffer, the "line"
 * pointer received as parameter is freed and then becomes invalid.
 * So this pointer must not be used after the call to this function.
 */

void
gui_line_add_y (struct t_gui_line *line)
{
    struct t_gui_line *ptr_line;
    struct t_gui_window *ptr_win;
    int old_line_displayed;

    /* search if line exists for "y" */
    for (ptr_line = line->data->buffer->own_lines->first_line; ptr_line;
         ptr_line = ptr_line->next_line)
    {
        if (ptr_line->data->y >= line->data->y)
            break;
    }

    if (ptr_line && (ptr_line->data->y == line->data->y))
    {
        /* replace line data with the new data */
        old_line_displayed = ptr_line->data->displayed;
        if (ptr_line->data->message)
        {
            /* remove line from coords if the content is changing */
            for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
            {
                gui_window_coords_remove_line (ptr_win, ptr_line);
            }
        }

        /* replace ptr_line by line in list */
        gui_line_free_data (ptr_line);
        ptr_line->data = line->data;
        free (line);
    }
    else
    {
        /* add line to lines list */
        old_line_displayed = 1;
        if (ptr_line)
        {
            /* add before line found */
            line->prev_line = ptr_line->prev_line;
            line->next_line = ptr_line;
            if (ptr_line->prev_line)
                (ptr_line->prev_line)->next_line = line;
            else
                line->data->buffer->own_lines->first_line = line;
            ptr_line->prev_line = line;
        }
        else
        {
            /* add at end of list */
            line->prev_line = line->data->buffer->own_lines->last_line;
            if (line->data->buffer->own_lines->first_line)
                line->data->buffer->own_lines->last_line->next_line = line;
            else
                line->data->buffer->own_lines->first_line = line;
            line->data->buffer->own_lines->last_line = line;
            line->next_line = NULL;
        }
        ptr_line = line;

        line->data->buffer->own_lines->lines_count++;
    }

    /* check if line is filtered or not */
    if (old_line_displayed && !ptr_line->data->displayed)
    {
        (ptr_line->data->buffer->own_lines->lines_hidden)++;
        (void) gui_buffer_send_signal (ptr_line->data->buffer,
                                       "buffer_lines_hidden",
                                       WEECHAT_HOOK_SIGNAL_POINTER,
                                       ptr_line->data->buffer);
    }
    else if (!old_line_displayed && ptr_line->data->displayed)
    {
        if (ptr_line->data->buffer->own_lines->lines_hidden > 0)
            (ptr_line->data->buffer->own_lines->lines_hidden)--;
    }

    ptr_line->data->refresh_needed = 1;

    gui_buffer_ask_chat_refresh (ptr_line->data->buffer, 1);

    (void) gui_buffer_send_signal (ptr_line->data->buffer,
                                   "buffer_line_added",
                                   WEECHAT_HOOK_SIGNAL_POINTER, ptr_line);
}

/*
 * Clears prefix and message on a line (used on buffers with free content only).
 */

void
gui_line_clear (struct t_gui_line *line)
{
    line->data->date = 0;
    line->data->date_usec = 0;
    line->data->date_printed = 0;
    line->data->date_usec_printed = 0;
    if (line->data->str_time)
    {
        free (line->data->str_time);
        line->data->str_time = NULL;
    }
    gui_line_tags_free (line->data);
    if (line->data->prefix)
    {
        string_shared_free (line->data->prefix);
        line->data->prefix = NULL;
    }
    line->data->prefix_length = 0;
    line->data->notify_level = 0;
    line->data->highlight = 0;
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
    new_lines = gui_line_lines_alloc ();
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

    /* ask refresh of prefix/buffer max length for mixed lines */
    new_lines->prefix_max_length_refresh = 1;
    new_lines->buffer_max_length_refresh = 1;

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
gui_line_hdata_lines_cb (const void *pointer, void *data,
                         const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
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
        HDATA_VAR(struct t_gui_lines, buffer_max_length_refresh, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_lines, prefix_max_length, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_lines, prefix_max_length_refresh, INTEGER, 0, NULL, NULL);
    }
    return hdata;
}

/*
 * Returns hdata for line.
 */

struct t_hdata *
gui_line_hdata_line_cb (const void *pointer, void *data,
                        const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
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
    char *new_value, *pos_newline;
    struct t_gui_line_data *line_data;
    struct t_gui_window *ptr_win;
    int rc, update_coords;

    /* make C compiler happy */
    (void) data;

    line_data = (struct t_gui_line_data *)pointer;

    rc = 0;
    update_coords = 0;

    if (hashtable_has_key (hashtable, "date"))
    {
        value = hashtable_get (hashtable, "date");
        if (value)
        {
            hdata_set (hdata, pointer, "date", value);
            free (line_data->str_time);
            line_data->str_time = gui_chat_get_time_string (
                line_data->date,
                line_data->date_usec,
                line_data->highlight);
            rc++;
            update_coords = 1;
        }
    }

    if (hashtable_has_key (hashtable, "date_usec"))
    {
        value = hashtable_get (hashtable, "date_usec");
        if (value)
        {
            hdata_set (hdata, pointer, "date_usec", value);
            free (line_data->str_time);
            line_data->str_time = gui_chat_get_time_string (
                line_data->date,
                line_data->date_usec,
                line_data->highlight);
            rc++;
            update_coords = 1;
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

    if (hashtable_has_key (hashtable, "date_usec_printed"))
    {
        value = hashtable_get (hashtable, "date_usec_printed");
        if (value)
        {
            hdata_set (hdata, pointer, "date_usec_printed", value);
            rc++;
        }
    }

    if (hashtable_has_key (hashtable, "tags_array"))
    {
        value = hashtable_get (hashtable, "tags_array");
        gui_line_tags_free (line_data);
        gui_line_tags_alloc (line_data, value);
        rc++;
    }

    if (hashtable_has_key (hashtable, "prefix"))
    {
        value = hashtable_get (hashtable, "prefix");
        hdata_set (hdata, pointer, "prefix", value);
        line_data->prefix_length = (line_data->prefix) ?
            gui_chat_strlen_screen (line_data->prefix) : 0;
        line_data->buffer->lines->prefix_max_length_refresh = 1;
        rc++;
        update_coords = 1;
    }

    if (hashtable_has_key (hashtable, "message"))
    {
        value = hashtable_get (hashtable, "message");
        new_value = (value) ? strdup (value) : NULL;
        if (new_value && !line_data->buffer->input_multiline)
        {
            /* if input_multiline is not set, keep only first line */
            pos_newline = strchr (new_value, '\n');
            if (pos_newline)
                pos_newline[0] = '\0';
        }
        hdata_set (hdata, pointer, "message", new_value);
        rc++;
        update_coords = 1;
        free (new_value);
    }

    if (rc > 0)
    {
        if (update_coords)
        {
            for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
            {
                gui_window_coords_remove_line_data (ptr_win, line_data);
            }
        }
        gui_filter_buffer (line_data->buffer, line_data);
        gui_buffer_ask_chat_refresh (line_data->buffer, 1);
    }

    return rc;
}

/*
 * Returns hdata for line data.
 */

struct t_hdata *
gui_line_hdata_line_data_cb (const void *pointer, void *data,
                             const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, NULL, NULL,
                       0, 0, &gui_line_hdata_line_data_update_cb, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_line_data, buffer, POINTER, 0, NULL, "buffer");
        HDATA_VAR(struct t_gui_line_data, id, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, y, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, date, TIME, 1, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, date_usec, INTEGER, 1, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, date_printed, TIME, 1, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, date_usec_printed, INTEGER, 1, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, str_time, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, tags_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, tags_array, SHARED_STRING, 1, "*,tags_count", NULL);
        HDATA_VAR(struct t_gui_line_data, displayed, CHAR, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, notify_level, CHAR, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, highlight, CHAR, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, refresh_needed, CHAR, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_line_data, prefix, SHARED_STRING, 1, NULL, NULL);
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

    if (!infolist_new_var_integer (ptr_item, "id", line->data->id))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "y", line->data->y))
        return 0;
    if (!infolist_new_var_time (ptr_item, "date", line->data->date))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "date_usec", line->data->date_usec))
        return 0;
    if (!infolist_new_var_time (ptr_item, "date_printed", line->data->date_printed))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "date_usec_printed", line->data->date_usec_printed))
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
    if (!infolist_new_var_integer (ptr_item, "notify_level", line->data->notify_level))
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
        log_printf ("    first_line . . . . . . . : %p", lines->first_line);
        log_printf ("    last_line. . . . . . . . : %p", lines->last_line);
        log_printf ("    last_read_line . . . . . : %p", lines->last_read_line);
        log_printf ("    lines_count. . . . . . . : %d", lines->lines_count);
        log_printf ("    first_line_not_read. . . : %d", lines->first_line_not_read);
        log_printf ("    lines_hidden . . . . . . : %d", lines->lines_hidden);
        log_printf ("    buffer_max_length. . . . : %d", lines->buffer_max_length);
        log_printf ("    buffer_max_length_refresh: %d", lines->buffer_max_length_refresh);
        log_printf ("    prefix_max_length. . . . : %d", lines->prefix_max_length);
        log_printf ("    prefix_max_length_refresh: %d", lines->prefix_max_length_refresh);
    }
}
