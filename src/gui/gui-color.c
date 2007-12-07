/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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

/* gui-color.c: color functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "gui-color.h"


/*
 * gui_color_search_config: search a color with configuration option name
 *                          return color found (number >= 0), -1 if not found
 */

int
gui_color_search_config (char *color_name)
{
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    if (color_name)
    {
        ptr_section = config_file_search_section (weechat_config_file,
                                                  "colors");
        if (ptr_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if (string_strcasecmp (ptr_option->name, color_name) == 0)
                    return ptr_option->min;
            }
        }
    }
    
    /* color not found */
    return -1;
}

/*
 * gui_color_decode: parses a message and remove WeeChat color codes
 *                   After use, string returned has to be free()
 */

unsigned char *
gui_color_decode (unsigned char *string)
{
    unsigned char *out;
    int out_length, out_pos, length;
    
    out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string && string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case GUI_COLOR_COLOR_CHAR:
                string++;
                if (isdigit (string[0]) && isdigit (string[1]))
                    string += 2;
                break;
            case GUI_COLOR_SET_CHAR:
            case GUI_COLOR_REMOVE_CHAR:
                string++;
                if (string[0])
                    string++;
                break;
            case GUI_COLOR_RESET_CHAR:
                string++;
                break;
            default:
                length = utf8_char_size ((char *)string);
                if (length == 0)
                    length = 1;
                memcpy (out + out_pos, string, length);
                out_pos += length;
                string += length;
        }
    }
    out[out_pos] = '\0';
    return out;
}
