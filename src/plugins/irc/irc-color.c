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

/* irc-color.c: IRC color decoding/encidong in messages */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "irc.h"
#include "irc-color.h"


/*
 * irc_color_decode: parses a message (coming from IRC server),
 *                   if keep_colors == 0: remove any color/style in message
 *                     otherwise change colors by internal WeeChat color codes
 *                   if keep_eechat_attr == 0: remove any weechat color/style attribute
 *                   After use, string returned has to be free()
 */

unsigned char *
irc_color_decode (unsigned char *string, int keep_irc_colors, int keep_weechat_attr)
{
    /*unsigned char *out;
    int out_length, out_pos, length;
    char str_fg[3], str_bg[3];
    int fg, bg, attr;*/

    (void) string;
    (void) keep_irc_colors;
    (void) keep_weechat_attr;

    return NULL;
    
    /*out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string && string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case IRC_COLOR_BOLD_CHAR:
            case IRC_COLOR_RESET_CHAR:
            case IRC_COLOR_FIXED_CHAR:
            case IRC_COLOR_REVERSE_CHAR:
            case IRC_COLOR_REVERSE2_CHAR:
            case IRC_COLOR_ITALIC_CHAR:
            case IRC_COLOR_UNDERLINE_CHAR:
                if (keep_irc_colors)
                    out[out_pos++] = string[0];
                string++;
                break;
            case IRC_COLOR_COLOR_CHAR:
                string++;
                str_fg[0] = '\0';
                str_bg[0] = '\0';
                if (isdigit (string[0]))
                {
                    str_fg[0] = string[0];
                    str_fg[1] = '\0';
                    string++;
                    if (isdigit (string[0]))
                    {
                        str_fg[1] = string[0];
                        str_fg[2] = '\0';
                        string++;
                    }
                }
                if (string[0] == ',')
                {
                    string++;
                    if (isdigit (string[0]))
                    {
                        str_bg[0] = string[0];
                        str_bg[1] = '\0';
                        string++;
                        if (isdigit (string[0]))
                        {
                            str_bg[1] = string[0];
                            str_bg[2] = '\0';
                            string++;
                        }
                    }
                }
                if (keep_irc_colors)
                {
                    if (!str_fg[0] && !str_bg[0])
                        out[out_pos++] = IRC_COLOR_COLOR_CHAR;
                    else
                    {
                        attr = 0;
                        if (str_fg[0])
                        {
                            sscanf (str_fg, "%d", &fg);
                            fg %= GUI_NUM_IRC_COLORS;
                            attr |= gui_irc_colors[fg][1];
                        }
                        if (str_bg[0])
                        {
                            sscanf (str_bg, "%d", &bg);
                            bg %= GUI_NUM_IRC_COLORS;
                            attr |= gui_irc_colors[bg][1];
                        }
                        if (attr & A_BOLD)
                        {
                            out[out_pos++] = GUI_ATTR_WEECHAT_SET_CHAR;
                            out[out_pos++] = IRC_COLOR_BOLD_CHAR;
                        }
                        else
                        {
                            out[out_pos++] = GUI_ATTR_WEECHAT_REMOVE_CHAR;
                            out[out_pos++] = IRC_COLOR_BOLD_CHAR;
                        }
                        out[out_pos++] = IRC_COLOR_COLOR_CHAR;
                        if (str_fg[0])
                        {
                            out[out_pos++] = (gui_irc_colors[fg][0] / 10) + '0';
                            out[out_pos++] = (gui_irc_colors[fg][0] % 10) + '0';
                        }
                        if (str_bg[0])
                        {
                            out[out_pos++] = ',';
                            out[out_pos++] = (gui_irc_colors[bg][0] / 10) + '0';
                            out[out_pos++] = (gui_irc_colors[bg][0] % 10) + '0';
                        }
                    }
                }
                break;
            case GUI_ATTR_WEECHAT_COLOR_CHAR:
                if (keep_weechat_attr)
                    out[out_pos++] = string[0];
                string++;
                if (isdigit (string[0]) && isdigit (string[1]))
                {
                    if (keep_weechat_attr)
                    {
                        out[out_pos++] = string[0];
                        out[out_pos++] = string[1];
                    }
                    string += 2;
                }
                break;
            case GUI_ATTR_WEECHAT_SET_CHAR:
            case GUI_ATTR_WEECHAT_REMOVE_CHAR:
                if (keep_weechat_attr)
                    out[out_pos++] = string[0];
                string++;
                if (string[0])
                {
                    if (keep_weechat_attr)
                        out[out_pos++] = string[0];
                    string++;
                }
                break;
            case GUI_ATTR_WEECHAT_RESET_CHAR:
                if (keep_weechat_attr)
                    out[out_pos++] = string[0];
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
    return out;*/
}

/*
 * irc_color_decode_for_user_entry: parses a message (coming from IRC server),
 *                                  and replaces colors/bold/.. by ^C, ^B, ..
 *                                  After use, string returned has to be free()
 */

unsigned char *
irc_color_decode_for_user_entry (unsigned char *string)
{
    /*unsigned char *out;
      int out_length, out_pos, length;*/

    (void) string;

    return NULL;
    
    /*out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string && string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case IRC_COLOR_BOLD_CHAR:
                out[out_pos++] = 0x02; // ^B
                string++;
                break;
            case IRC_COLOR_FIXED_CHAR:
                string++;
                break;
            case IRC_COLOR_RESET_CHAR:
                out[out_pos++] = 0x0F; // ^O
                string++;
                break;
            case IRC_COLOR_REVERSE_CHAR:
            case IRC_COLOR_REVERSE2_CHAR:
                out[out_pos++] = 0x12; // ^R
                string++;
                break;
            case IRC_COLOR_ITALIC_CHAR:
                string++;
                break;
            case IRC_COLOR_UNDERLINE_CHAR:
                out[out_pos++] = 0x15; // ^U
                string++;
                break;
            case IRC_COLOR_COLOR_CHAR:
                out[out_pos++] = 0x03; // ^C
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
    return out;*/
}

/*
 * irc_color_encode: parses a message (entered by user), and
 *                   encode special chars (^Cb, ^Cc, ..) in IRC colors
 *                   if keep_colors == 0: remove any color/style in message
 *                   otherwise: keep colors
 *                   After use, string returned has to be free()
 */

unsigned char *
irc_color_encode (unsigned char *string, int keep_colors)
{
    /*unsigned char *out;
      int out_length, out_pos, length;*/

    (void) string;
    (void) keep_colors;

    return NULL;
    
    /*out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string && string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case 0x02: // ^B
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_BOLD_CHAR;
                string++;
                break;
            case 0x03: // ^C
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_COLOR_CHAR;
                string++;
                if (isdigit (string[0]))
                {
                    if (keep_colors)
                        out[out_pos++] = string[0];
                    string++;
                    if (isdigit (string[0]))
                    {
                        if (keep_colors)
                            out[out_pos++] = string[0];
                        string++;
                    }
                }
                if (string[0] == ',')
                {
                    if (keep_colors)
                        out[out_pos++] = ',';
                    string++;
                    if (isdigit (string[0]))
                    {
                        if (keep_colors)
                            out[out_pos++] = string[0];
                        string++;
                        if (isdigit (string[0]))
                        {
                            if (keep_colors)
                                out[out_pos++] = string[0];
                            string++;
                        }
                    }
                }
                break;
            case 0x0F: // ^O
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_RESET_CHAR;
                string++;
                break;
            case 0x12: // ^R
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_REVERSE_CHAR;
                string++;
                break;
            case 0x15: // ^U
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_UNDERLINE_CHAR;
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
    return out;*/
}
