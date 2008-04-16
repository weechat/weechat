/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* gui-filter.c: filter functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <regex.h>

#include "../core/weechat.h"
#include "../core/wee-hook.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-filter.h"
#include "gui-buffer.h"
#include "gui-chat.h"


struct t_gui_filter *gui_filters = NULL;           /* first filter          */
struct t_gui_filter *last_gui_filter = NULL;       /* last filter           */
int gui_filters_enabled = 1;                       /* filters enabled?      */


/*
 * gui_filter_check_line: return 1 if a line should be displayed, or
 *                        0 if line is hidden (tag or regex found)
 */

int
gui_filter_check_line (struct t_gui_buffer *buffer, struct t_gui_line *line)
{
    struct t_gui_filter *ptr_filter;
    
    /* make C compiler happy */
    (void) buffer;
    
    /* line is always displayed if filters are disabled */
    if (!gui_filters_enabled)
        return 1;
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        /* check buffer name */
        if (gui_buffer_match_category_name (buffer,
                                            ptr_filter->buffer, 0))
        {
            if ((strcmp (ptr_filter->tags, "*") == 0)
                || (gui_chat_line_match_tags (line,
                                              ptr_filter->tags_count,
                                              ptr_filter->tags_array)))
            {
                /* check line with regex */
                if (!ptr_filter->regex_prefix && !ptr_filter->regex_message)
                    return 0;
                
                if (gui_chat_line_match_regex (line,
                                               ptr_filter->regex_prefix,
                                               ptr_filter->regex_message))
                    return 0;
            }
        }
    }
    
    /* no tag or regex matching, then line is displayed */
    return 1;
}

/*
 * gui_filter_buffer: filter a buffer, using message filters
 */

void
gui_filter_buffer (struct t_gui_buffer *buffer)
{
    struct t_gui_line *ptr_line;
    int line_displayed, lines_hidden;
    
    lines_hidden = 0;
    
    for (ptr_line = buffer->lines; ptr_line;
         ptr_line = ptr_line->next_line)
    {
        line_displayed = gui_filter_check_line (buffer, ptr_line);
        
        /* force chat refresh if at least one line changed */
        if (ptr_line->displayed != line_displayed)
            gui_buffer_ask_chat_refresh (buffer, 2);
        
        ptr_line->displayed = line_displayed;
        
        if (!line_displayed)
            lines_hidden = 1;
    }
    
    if (buffer->lines_hidden != lines_hidden)
    {
        buffer->lines_hidden = lines_hidden;
        hook_signal_send ("buffer_lines_hidden",
                          WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
}

/*
 * gui_filter_all_buffers: filter all buffers, using message filters
 */

void
gui_filter_all_buffers ()
{
    struct t_gui_buffer *ptr_buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_filter_buffer (ptr_buffer);
    }
}

/*
 * gui_filter_enable: enable filters
 */

void
gui_filter_enable ()
{
    if (!gui_filters_enabled)
    {
        gui_filters_enabled = 1;
        gui_filter_all_buffers ();
        hook_signal_send ("filters_enabled",
                          WEECHAT_HOOK_SIGNAL_STRING, NULL);
    }
}

/*
 * gui_filter_disable: disable filters
 */

void
gui_filter_disable ()
{
    if (gui_filters_enabled)
    {
        gui_filters_enabled = 0;
        gui_filter_all_buffers ();
        hook_signal_send ("filters_disabled",
                          WEECHAT_HOOK_SIGNAL_STRING, NULL);
    }
}

/*
 * gui_filter_search: search a filter
 */

struct t_gui_filter *
gui_filter_search (char *buffer, char *tags, char *regex)
{
    struct t_gui_filter *ptr_filter;
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if ((strcmp (ptr_filter->buffer, buffer) == 0)
            && (strcmp (ptr_filter->tags, tags) == 0)
            && (strcmp (ptr_filter->regex, regex) == 0))
            return ptr_filter;
    }
    
    /* no filter found */
    return NULL;
}

/*
 * gui_filter_search_by_number: search a filter by number (first is #1)
 */

struct t_gui_filter *
gui_filter_search_by_number (int number)
{
    struct t_gui_filter *ptr_filter;
    int i;

    i = 1;
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (i == number)
            return ptr_filter;
        i++;
    }
    
    /* no filter found */
    return NULL;
}

/*
 * gui_filter_new: create a new filter
 */

struct t_gui_filter *
gui_filter_new (char *buffer, char *tags, char *regex)
{
    struct t_gui_filter *new_filter;
    regex_t *regex1, *regex2;
    char *pos_tab, *regex_prefix, *pos_regex_message;

    if (!buffer || !tags || !regex)
        return NULL;
    
    regex1 = NULL;
    regex2 = NULL;
    if (strcmp (regex, "*") != 0)
    {
        pos_tab = strstr (regex, "\\t");
        if (pos_tab)
        {
            regex_prefix = string_strndup (regex, pos_tab - regex);
            pos_regex_message = pos_tab + 2;
        }
        else
        {
            regex_prefix = NULL;
            pos_regex_message = regex;
        }
        
        if (regex_prefix)
        {
            regex1 = malloc (sizeof (*regex1));
            if (regex1)
            {
                if (regcomp (regex1, regex_prefix,
                             REG_NOSUB | REG_ICASE) != 0)
                {
                    free (regex_prefix);
                    free (regex1);
                    return NULL;
                }
            }
        }
        
        regex2 = malloc (sizeof (*regex2));
        if (regex2)
        {
            if (regcomp (regex2, pos_regex_message,
                         REG_NOSUB | REG_ICASE) != 0)
            {
                if (regex_prefix)
                    free (regex_prefix);
                if (regex1)
                    free (regex1);
                free (regex2);
                return NULL;
            }
        }
        
        if (regex_prefix)
            free (regex_prefix);
    }
    
    /* create new filter */
    new_filter = (malloc (sizeof (*new_filter)));
    if (new_filter)
    {
        /* init filter */
        new_filter->buffer = (buffer) ? strdup (buffer) : strdup ("*");
        if (tags)
        {
            new_filter->tags = (tags) ? strdup (tags) : NULL;
            new_filter->tags_array = string_explode (tags, ",", 0, 0,
                                                     &new_filter->tags_count);
        }
        else
        {
            new_filter->tags = NULL;
            new_filter->tags_count = 0;
            new_filter->tags_array = NULL;
        }
        new_filter->regex = strdup (regex);
        new_filter->regex_prefix = regex1;
        new_filter->regex_message = regex2;
        
        /* add filter to filters list */
        new_filter->prev_filter = last_gui_filter;
        if (gui_filters)
            last_gui_filter->next_filter = new_filter;
        else
            gui_filters = new_filter;
        last_gui_filter = new_filter;
        new_filter->next_filter = NULL;
        
        gui_filter_all_buffers ();
        
        hook_signal_send ("filter_added",
                          WEECHAT_HOOK_SIGNAL_POINTER, new_filter);
    }
    
    return new_filter;
}

/*
 * gui_filter_free: remove a filter
 */

void
gui_filter_free (struct t_gui_filter *filter)
{
    hook_signal_send ("filter_removing",
                      WEECHAT_HOOK_SIGNAL_POINTER, filter);
    
    /* free data */
    if (filter->buffer)
        free (filter->buffer);
    if (filter->tags)
        free (filter->tags);
    if (filter->regex)
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
    
    /* remove filter from filters list */
    if (filter->prev_filter)
        filter->prev_filter->next_filter = filter->next_filter;
    if (filter->next_filter)
        filter->next_filter->prev_filter = filter->prev_filter;
    if (gui_filters == filter)
        gui_filters = filter->next_filter;
    if (last_gui_filter == filter)
        last_gui_filter = filter->prev_filter;
    
    free (filter);
    
    gui_filter_all_buffers ();
    
    hook_signal_send ("filter_removed", WEECHAT_HOOK_SIGNAL_STRING, NULL);
}

/*
 * gui_filter_free_all: remove all filters
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
 * gui_filter_print_log: print filter infos in log (usually for crash dump)
 */

void
gui_filter_print_log ()
{
    struct t_gui_filter *ptr_filter;
    
    log_printf ("");
    log_printf ("gui_filters_enabled = %d", gui_filters_enabled);
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        log_printf ("");
        log_printf ("[filter (addr:0x%x)]", ptr_filter);
        log_printf ("  buffer . . . . . . . . : '%s'", ptr_filter->buffer);
        log_printf ("  tags . . . . . . . . . : '%s'", ptr_filter->tags);
        log_printf ("  regex. . . . . . . . . : '%s'", ptr_filter->regex);
        log_printf ("  regex_prefix . . . . . : 0x%x", ptr_filter->regex_prefix);
        log_printf ("  regex_message. . . . . : 0x%x", ptr_filter->regex_message);
        log_printf ("  prev_filter. . . . . . : 0x%x", ptr_filter->prev_filter);
        log_printf ("  next_filter. . . . . . : 0x%x", ptr_filter->next_filter);
    }
}
