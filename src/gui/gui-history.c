/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005 Emmanuel Bouthenot <kolter@openics.org>
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
 * gui-history.c: memorize commands or text for buffers (used by all GUI)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hdata.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-history.h"
#include "gui-buffer.h"


struct t_gui_history *history_global = NULL;
struct t_gui_history *last_history_global = NULL;
struct t_gui_history *history_global_ptr = NULL;
int num_history_global = 0;


/*
 * gui_history_buffer_add: add a text/command to buffer's history
 */

void
gui_history_buffer_add (struct t_gui_buffer *buffer, const char *string)
{
    struct t_gui_history *new_history, *ptr_history;

    if (!string)
        return;

    if (!buffer->history
        || (buffer->history
            && (strcmp (buffer->history->text, string) != 0)))
    {
        new_history = malloc (sizeof (*new_history));
        if (new_history)
        {
            new_history->text = strdup (string);
            if (buffer->history)
                buffer->history->prev_history = new_history;
            else
                buffer->last_history = new_history;
            new_history->next_history = buffer->history;
            new_history->prev_history = NULL;
            buffer->history = new_history;
            buffer->num_history++;

            /* remove one command if necessary */
            if ((CONFIG_INTEGER(config_history_max_commands) > 0)
                && (buffer->num_history > CONFIG_INTEGER(config_history_max_commands)))
            {
                ptr_history = buffer->last_history->prev_history;
                if (buffer->ptr_history == buffer->last_history)
                    buffer->ptr_history = ptr_history;
                ((buffer->last_history)->prev_history)->next_history = NULL;
                if (buffer->last_history->text)
                    free (buffer->last_history->text);
                free (buffer->last_history);
                buffer->last_history = ptr_history;
                buffer->num_history++;
            }
        }
    }
}

/*
 * history_global_add: add a text/command to global history
 */

void
gui_history_global_add (const char *string)
{
    struct t_gui_history *new_history, *ptr_history;

    if (!string)
        return;

    if (!history_global
        || (history_global
            && (strcmp (history_global->text, string) != 0)))
    {
        new_history = malloc (sizeof (*new_history));
        if (new_history)
        {
            new_history->text = strdup (string);
            if (history_global)
                history_global->prev_history = new_history;
            else
                last_history_global = new_history;
            new_history->next_history = history_global;
            new_history->prev_history = NULL;
            history_global = new_history;
            num_history_global++;

            /* remove one command if necessary */
            if ((CONFIG_INTEGER(config_history_max_commands) > 0)
                && (num_history_global > CONFIG_INTEGER(config_history_max_commands)))
            {
                ptr_history = last_history_global->prev_history;
                if (history_global_ptr == last_history_global)
                    history_global_ptr = ptr_history;
                (last_history_global->prev_history)->next_history = NULL;
                if (last_history_global->text)
                    free (last_history_global->text);
                free (last_history_global);
                last_history_global = ptr_history;
                num_history_global--;
            }
        }
    }
}

/*
 * gui_history_add: add a text/command to buffer's history + global history
 */

void
gui_history_add (struct t_gui_buffer *buffer, const char *string)
{
    char *string2, str_buffer[128];

    snprintf (str_buffer, sizeof (str_buffer),
              "0x%lx", (long unsigned int)(buffer));
    string2 = hook_modifier_exec (NULL, "history_add", str_buffer, string);

    /*
     * if message was NOT dropped by modifier, then we add it to buffer and
     * global history
     */
    if (!string2 || string2[0])
    {
        gui_history_buffer_add (buffer, (string2) ? string2 : string);
        gui_history_global_add ((string2) ? string2 : string);
    }

    if (string2)
        free (string2);
}

/*
 * gui_history_global_free: free global history
 */

void
gui_history_global_free ()
{
    struct t_gui_history *ptr_history;

    while (history_global)
    {
        ptr_history = history_global->next_history;
        if (history_global->text)
            free (history_global->text);
        free (history_global);
        history_global = ptr_history;
    }
    history_global = NULL;
    last_history_global = NULL;
    history_global_ptr = NULL;
    num_history_global = 0;
}


/*
 * gui_history_buffer_free: free history for a buffer
 */

void
gui_history_buffer_free (struct t_gui_buffer *buffer)
{
    struct t_gui_history *ptr_history;

    while (buffer->history)
    {
        ptr_history = buffer->history->next_history;
        if (buffer->history->text)
            free (buffer->history->text);
        free (buffer->history);
        buffer->history = ptr_history;
    }
    buffer->history = NULL;
    buffer->last_history = NULL;
    buffer->ptr_history = NULL;
    buffer->num_history = 0;
}

/*
 * gui_history_hdata_history_cb: return hdata for history
 */

struct t_hdata *
gui_history_hdata_history_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_history", "next_history");
    if (hdata)
    {
        HDATA_VAR(struct t_gui_history, text, STRING, NULL, NULL);
        HDATA_VAR(struct t_gui_history, prev_history, POINTER, NULL, hdata_name);
        HDATA_VAR(struct t_gui_history, next_history, POINTER, NULL, hdata_name);
    }
    return hdata;
}

/*
 * gui_history_add_to_infolist: add history of commands in an infolist
 *                              if buffer is NULL, global history is returned
 *                              return 1 if ok, 0 if error
 */

int
gui_history_add_to_infolist (struct t_infolist *infolist,
                             struct t_gui_history *history)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !history)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_string (ptr_item, "text", history->text))
        return 0;

    return 1;
}
