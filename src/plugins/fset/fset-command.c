/*
 * fset-command.c - Fast Set command
 *
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-command.h"
#include "fset-bar-item.h"
#include "fset-buffer.h"
#include "fset-option.h"


/*
 * Callback for command "/fset".
 */

int
fset_command_fset (const void *pointer, void *data,
                   struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    int num_options, line;
    long value;
    char *error;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if (argc == 1)
    {
        if (weechat_arraylist_size (fset_options) == 0)
        {
            fset_option_get_options ();
        }
        if (!fset_buffer)
        {
            fset_buffer_open ();
            fset_buffer_refresh (1);
        }
        weechat_buffer_set (fset_buffer, "display", "1");
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "-refresh") == 0)
    {
        fset_bar_item_update ();
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "-bar") == 0)
    {
        fset_bar_item_update ();
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "-up") == 0)
    {
        if (fset_buffer)
        {
            value = 1;
            if (argc > 2)
            {
                error = NULL;
                value = strtol (argv[2], &error, 10);
                if (!error || error[0])
                    value = 1;
            }
            num_options = weechat_arraylist_size (fset_options);
            if ((fset_buffer_selected_line >= 0)
                && (num_options > 0))
            {
                line = fset_buffer_selected_line - value;
                if (line < 0)
                    line = 0;
                if (line != fset_buffer_selected_line)
                {
                    fset_buffer_set_current_line (line);
                    fset_buffer_check_line_outside_window ();
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "-down") == 0)
    {
        if (fset_buffer)
        {
            value = 1;
            if (argc > 2)
            {
                error = NULL;
                value = strtol (argv[2], &error, 10);
                if (!error || error[0])
                    value = 1;
            }
            num_options = weechat_arraylist_size (fset_options);
            if ((fset_buffer_selected_line >= 0)
                && (num_options > 0))
            {
                line = fset_buffer_selected_line + value;
                if (line >= num_options)
                    line = num_options - 1;
                if (line != fset_buffer_selected_line)
                {
                    fset_buffer_set_current_line (line);
                    fset_buffer_check_line_outside_window ();
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Hooks fset commands.
 */

void
fset_command_init ()
{
    weechat_hook_command (
        "fset",
        N_("fast set WeeChat and plugins options"),
        "-bar || -refresh",
        N_("    -bar: add the fset bar\n"
           "-refresh: force the refresh of the \"fset\" bar item"),
        "-bar || -refresh",
        &fset_command_fset, NULL, NULL);
}
