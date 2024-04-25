/*
 * xfer-buffer.c - display xfer list on xfer buffer
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "xfer.h"
#include "xfer-buffer.h"
#include "xfer-config.h"
#include "xfer-network.h"


struct t_gui_buffer *xfer_buffer = NULL;
int xfer_buffer_selected_line = 0;


/*
 * Updates a xfer in buffer and updates hotlist for xfer buffer.
 */

void
xfer_buffer_refresh (const char *hotlist)
{
    struct t_xfer *ptr_xfer, *xfer_selected;
    char str_color[256], suffix[32], status[64], date[128], eta[128];
    char str_ip[128], str_hash[128];
    char *progress_bar, *str_pos, *str_total, *str_bytes_per_sec;
    int i, length, line, progress_bar_size, num_bars;
    unsigned long long pos, pct_complete;
    struct tm *date_tmp;

    if (xfer_buffer)
    {
        weechat_buffer_clear (xfer_buffer);
        line = 0;
        xfer_selected = xfer_search_by_number (xfer_buffer_selected_line);
        weechat_printf_y (xfer_buffer, 0,
                          "%s%s%s%s%s%s%s%s",
                          weechat_color ("green"),
                          _("Actions (letter+enter):"),
                          weechat_color ("lightgreen"),
                          /* accept */
                          (xfer_selected && XFER_IS_RECV(xfer_selected->type)
                           && (xfer_selected->status == XFER_STATUS_WAITING)) ?
                          _("  [A] Accept") : "",
                          /* cancel */
                          (xfer_selected
                           && !XFER_HAS_ENDED(xfer_selected->status)) ?
                          _("  [C] Cancel") : "",
                          /* remove */
                          (xfer_selected
                           && XFER_HAS_ENDED(xfer_selected->status)) ?
                          _("  [R] Remove") : "",
                          /* purge old */
                          _("  [P] Purge finished"),
                          /* quit */
                          _("  [Q] Close this buffer"));
        for (ptr_xfer = xfer_list; ptr_xfer; ptr_xfer = ptr_xfer->next_xfer)
        {
            suffix[0] = '\0';
            if (ptr_xfer->filename_suffix >= 0)
            {
                snprintf (suffix, sizeof (suffix),
                          " (.%d)", ptr_xfer->filename_suffix);
            }

            snprintf (str_color, sizeof (str_color),
                      "%s,%s",
                      (line == xfer_buffer_selected_line) ?
                      weechat_config_string (xfer_config_color_text_selected) :
                      weechat_config_string (xfer_config_color_text),
                      weechat_config_string (xfer_config_color_text_bg));

            str_ip[0] = '\0';
            if (ptr_xfer->remote_address_str)
            {
                snprintf (str_ip, sizeof (str_ip),
                          " (%s)",
                          ptr_xfer->remote_address_str);
            }

            str_hash[0] = '\0';
            if (ptr_xfer->hash_target
                && ptr_xfer->hash_handle
                && (ptr_xfer->hash_status != XFER_HASH_STATUS_UNKNOWN)
                && ((ptr_xfer->status == XFER_STATUS_ACTIVE)
                    || (ptr_xfer->status == XFER_STATUS_DONE)
                    || (ptr_xfer->status == XFER_STATUS_HASHING)))
            {
                snprintf (str_hash, sizeof (str_hash),
                          " (%s)",
                          _(xfer_hash_status_string[ptr_xfer->hash_status]));
            }

            /* display first line with remote nick, filename and plugin name/id */
            weechat_printf_y (xfer_buffer, (line * 2) + 2,
                              "%s%s%-24s %s%s%s%s (%s.%s)%s%s",
                              weechat_color (str_color),
                              (line == xfer_buffer_selected_line) ?
                              "*** " : "    ",
                              ptr_xfer->remote_nick,
                              (XFER_IS_FILE(ptr_xfer->type)) ? "\"" : "",
                              (XFER_IS_FILE(ptr_xfer->type)) ?
                              ptr_xfer->filename : _("xfer chat"),
                              (XFER_IS_FILE(ptr_xfer->type)) ? "\"" : "",
                              suffix,
                              ptr_xfer->plugin_name,
                              ptr_xfer->plugin_id,
                              str_ip,
                              str_hash);

            snprintf (status, sizeof (status),
                      "%s", _(xfer_status_string[ptr_xfer->status]));
            length = weechat_utf8_strlen_screen (status);
            if (length < 20)
            {
                for (i = 0; i < 20 - length; i++)
                {
                    strcat (status, " ");
                }
            }

            if (XFER_IS_CHAT(ptr_xfer->type))
            {
                /* display second line for chat with status and date */
                date[0] = '\0';
                date_tmp = localtime (&(ptr_xfer->start_time));
                if (date_tmp)
                {
                    if (strftime (date, sizeof (date),
                                  "%a, %d %b %Y %H:%M:%S", date_tmp) == 0)
                        date[0] = '\0';
                }
                weechat_printf_y (xfer_buffer, (line * 2) + 3,
                                  "%s%s%s %s%s%s%s%s",
                                  weechat_color (str_color),
                                  (line == xfer_buffer_selected_line) ?
                                  "*** " : "    ",
                                  (XFER_IS_SEND(ptr_xfer->type)) ?
                                  "<<--" : "-->>",
                                  weechat_color (weechat_config_string (xfer_config_color_status[ptr_xfer->status])),
                                  status,
                                  weechat_color ("reset"),
                                  weechat_color (str_color),
                                  date);
            }
            else
            {
                /* build progress bar */
                pos = (ptr_xfer->pos <= ptr_xfer->size) ? ptr_xfer->pos : ptr_xfer->size;
                progress_bar = NULL;
                progress_bar_size = weechat_config_integer (xfer_config_look_progress_bar_size);
                if (progress_bar_size > 0)
                {
                    progress_bar = malloc (1 + progress_bar_size + 1 + 1 + 1);
                    strcpy (progress_bar, "[");
                    if (ptr_xfer->size == 0)
                    {
                        if (ptr_xfer->status == XFER_STATUS_DONE)
                            num_bars = progress_bar_size;
                        else
                            num_bars = 0;
                    }
                    else
                        num_bars = (int)(((float)(pos)/(float)(ptr_xfer->size)) * (float)progress_bar_size);
                    for (i = 0; i < num_bars - 1; i++)
                    {
                        strcat (progress_bar, "=");
                    }
                    if (num_bars > 0)
                        strcat (progress_bar, ">");
                    for (i = 0; i < progress_bar_size - num_bars; i++)
                    {
                        strcat (progress_bar, " ");
                    }
                    strcat (progress_bar, "] ");
                }

                /* computes percentage */
                if (ptr_xfer->size == 0)
                {
                    if (ptr_xfer->status == XFER_STATUS_DONE)
                        pct_complete = 100;
                    else
                        pct_complete = 0;
                }
                else
                    pct_complete = (unsigned long long)(((float)(pos)/(float)(ptr_xfer->size)) * 100);

                /* position, total and bytes per second */
                str_pos = weechat_string_format_size (pos);
                str_total = weechat_string_format_size (ptr_xfer->size);
                str_bytes_per_sec = weechat_string_format_size (ptr_xfer->bytes_per_sec);

                /* ETA */
                eta[0] = '\0';
                if (ptr_xfer->status == XFER_STATUS_ACTIVE)
                {
                    snprintf (eta, sizeof (eta),
                              "%s: %.2llu:%.2llu:%.2llu - ",
                              _("ETA"),
                              ptr_xfer->eta / 3600,
                              (ptr_xfer->eta / 60) % 60,
                              ptr_xfer->eta % 60);
                }

                /* display second line for file with status, progress bar and estimated time */
                weechat_printf_y (xfer_buffer, (line * 2) + 3,
                                  "%s%s%s %s%s%s%s%3llu%%   %s / %s  (%s%s/s)",
                                  weechat_color (str_color),
                                  (line == xfer_buffer_selected_line) ? "*** " : "    ",
                                  (XFER_IS_SEND(ptr_xfer->type)) ? "<<--" : "-->>",
                                  weechat_color (weechat_config_string (xfer_config_color_status[ptr_xfer->status])),
                                  status,
                                  weechat_color (str_color),
                                  (progress_bar) ? progress_bar : "",
                                  pct_complete,
                                  (str_pos) ? str_pos : "?",
                                  (str_total) ? str_total : "?",
                                  eta,
                                  str_bytes_per_sec);
                free (progress_bar);
                free (str_pos);
                free (str_total);
                free (str_bytes_per_sec);
            }
            line++;
        }
        weechat_buffer_set (xfer_buffer, "hotlist", hotlist);
    }
}

/*
 * Callback called when user send data to xfer list buffer.
 */

int
xfer_buffer_input_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer,
                      const char *input_data)
{
    struct t_xfer *xfer, *ptr_xfer, *next_xfer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    xfer = xfer_search_by_number (xfer_buffer_selected_line);

    /* accept xfer */
    if (weechat_strcmp (input_data, "a") == 0)
    {
        if (xfer && XFER_IS_RECV(xfer->type)
            && (xfer->status == XFER_STATUS_WAITING))
        {
            xfer_network_accept (xfer);
        }
    }
    /* cancel xfer */
    else if (weechat_strcmp (input_data, "c") == 0)
    {
        if (xfer && !XFER_HAS_ENDED(xfer->status))
        {
            xfer_close (xfer, XFER_STATUS_ABORTED);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
        }
    }
    /* purge old xfer */
    else if (weechat_strcmp (input_data, "p") == 0)
    {
        ptr_xfer = xfer_list;
        while (ptr_xfer)
        {
            next_xfer = ptr_xfer->next_xfer;
            if (XFER_HAS_ENDED(ptr_xfer->status))
                xfer_free (ptr_xfer);
            ptr_xfer = next_xfer;
        }
        xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
    }
    /* quit xfer buffer (close it) */
    else if (weechat_strcmp (input_data, "q") == 0)
    {
        weechat_buffer_close (buffer);
    }
    /* remove xfer */
    else if (weechat_strcmp (input_data, "r") == 0)
    {
        if (xfer && XFER_HAS_ENDED(xfer->status))
        {
            xfer_free (xfer);
            xfer_buffer_refresh (WEECHAT_HOTLIST_MESSAGE);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback called when xfer buffer is closed.
 */

int
xfer_buffer_close_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    xfer_buffer = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Opens xfer buffer (to display list of xfer).
 */

void
xfer_buffer_open ()
{
    struct t_hashtable *buffer_props;

    if (xfer_buffer)
        return;

    buffer_props = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (buffer_props)
    {
        weechat_hashtable_set (buffer_props, "type", "free");
        weechat_hashtable_set (buffer_props, "title", _("Xfer list"));
        weechat_hashtable_set (buffer_props, "key_bind_up", "/xfer up");
        weechat_hashtable_set (buffer_props, "key_bind_down", "/xfer down");
        weechat_hashtable_set (buffer_props, "localvar_set_type", "xfer");
    }

    xfer_buffer = weechat_buffer_new_props (
        XFER_BUFFER_NAME,
        buffer_props,
        &xfer_buffer_input_cb, NULL, NULL,
        &xfer_buffer_close_cb, NULL, NULL);

    weechat_hashtable_free (buffer_props);
}
