/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* gui-curses-color.c: color functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/util.h"
#include "../../common/weeconfig.h"
#include "gui-curses.h"


t_gui_color gui_weechat_colors[] =
{ { -1,                    0, 0,        "default"      },
  { WEECHAT_COLOR_BLACK,   0, 0,        "black"        },
  { WEECHAT_COLOR_RED,     0, 0,        "red"          },
  { WEECHAT_COLOR_RED,     0, A_BOLD,   "lightred"     },
  { WEECHAT_COLOR_GREEN,   0, 0,        "green"        },
  { WEECHAT_COLOR_GREEN,   0, A_BOLD,   "lightgreen"   },
  { WEECHAT_COLOR_YELLOW,  0, 0,        "brown"        },
  { WEECHAT_COLOR_YELLOW,  0, A_BOLD,   "yellow"       },
  { WEECHAT_COLOR_BLUE,    0, 0,        "blue"         },
  { WEECHAT_COLOR_BLUE,    0, A_BOLD,   "lightblue"    },
  { WEECHAT_COLOR_MAGENTA, 0, 0,        "magenta"      },
  { WEECHAT_COLOR_MAGENTA, 0, A_BOLD,   "lightmagenta" },
  { WEECHAT_COLOR_CYAN,    0, 0,        "cyan"         },
  { WEECHAT_COLOR_CYAN,    0, A_BOLD,   "lightcyan"    },
  { WEECHAT_COLOR_WHITE,   0, A_BOLD,   "white"        },
  { 0,                     0, 0,        NULL           }
};

int gui_irc_colors[GUI_NUM_IRC_COLORS][2] =
{ { /*  0 */ WEECHAT_COLOR_WHITE,   A_BOLD },
  { /*  1 */ WEECHAT_COLOR_BLACK,   0      },
  { /*  2 */ WEECHAT_COLOR_BLUE,    0      },
  { /*  3 */ WEECHAT_COLOR_GREEN,   0      },
  { /*  4 */ WEECHAT_COLOR_RED,     A_BOLD },
  { /*  5 */ WEECHAT_COLOR_RED,     0      },
  { /*  6 */ WEECHAT_COLOR_MAGENTA, 0      },
  { /*  7 */ WEECHAT_COLOR_YELLOW,  0      },
  { /*  8 */ WEECHAT_COLOR_YELLOW,  A_BOLD },
  { /*  9 */ WEECHAT_COLOR_GREEN,   A_BOLD },
  { /* 10 */ WEECHAT_COLOR_CYAN,    0      },
  { /* 11 */ WEECHAT_COLOR_CYAN,    A_BOLD },
  { /* 12 */ WEECHAT_COLOR_BLUE,    A_BOLD },
  { /* 13 */ WEECHAT_COLOR_MAGENTA, A_BOLD },
  { /* 14 */ WEECHAT_COLOR_WHITE,   0      },
  { /* 15 */ WEECHAT_COLOR_WHITE,   A_BOLD }
};

t_gui_color *gui_color[GUI_NUM_COLORS];


/*
 * gui_color_assign: assign a WeeChat color (read from config)
 */

int
gui_color_assign (int *color, char *color_name)
{
    int i;
    
    /* look for curses colors in table */
    i = 0;
    while (gui_weechat_colors[i].string)
    {
        if (ascii_strcasecmp (gui_weechat_colors[i].string, color_name) == 0)
        {
            *color = i;
            return 1;
        }
        i++;
    }
    
    /* color not found */
    return 0;
}

/*
 * gui_color_get_name: get color name
 */

char *
gui_color_get_name (int num_color)
{
    return gui_weechat_colors[num_color].string;
}

/*
 * gui_color_decode: parses a message (coming from IRC server),
 *                   and according:
 *                     - remove any color/style in message
 *                   or:
 *                     - change colors by codes to be compatible with
 *                       other IRC clients
 *                   After use, string returned has to be free()
 */

unsigned char *
gui_color_decode (unsigned char *string, int keep_colors)
{
    unsigned char *out;
    int out_length, out_pos;
    char str_fg[3], str_bg[3];
    int fg, bg, attr;
    
    out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case GUI_ATTR_BOLD_CHAR:
            case GUI_ATTR_RESET_CHAR:
            case GUI_ATTR_FIXED_CHAR:
            case GUI_ATTR_REVERSE_CHAR:
            case GUI_ATTR_REVERSE2_CHAR:
            case GUI_ATTR_ITALIC_CHAR:
            case GUI_ATTR_UNDERLINE_CHAR:
                if (keep_colors)
                    out[out_pos++] = string[0];
                string++;
                break;
            case GUI_ATTR_COLOR_CHAR:
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
                if (keep_colors)
                {
                    if (!str_fg[0] && !str_bg[0])
                        out[out_pos++] = GUI_ATTR_COLOR_CHAR;
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
                            out[out_pos++] = GUI_ATTR_BOLD_CHAR;
                        }
                        else
                        {
                            out[out_pos++] = GUI_ATTR_WEECHAT_REMOVE_CHAR;
                            out[out_pos++] = GUI_ATTR_BOLD_CHAR;
                        }
                        out[out_pos++] = GUI_ATTR_COLOR_CHAR;
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
                string++;
                if (isdigit (string[0]) && isdigit (string[1]))
                {
                    if (keep_colors)
                    {
                        out[out_pos++] = string[0];
                        out[out_pos++] = string[1];
                    }
                    string += 2;
                }
                break;
            case GUI_ATTR_WEECHAT_SET_CHAR:
            case GUI_ATTR_WEECHAT_REMOVE_CHAR:
                string++;
                if (string[0])
                {
                    if (keep_colors)
                    {
                        out[out_pos++] = *(string - 1);
                        out[out_pos++] = string[0];
                    }
                    string++;
                }
                break;
            default:
                out[out_pos++] = string[0];
                string++;
        }
    }
    out[out_pos] = '\0';
    return out;
}

/*
 * gui_color_decode_for_user_entry: parses a message (coming from IRC server),
 *                                  and replaces colors/bold/.. by %C, %B, ..
 *                                  After use, string returned has to be free()
 */

unsigned char *
gui_color_decode_for_user_entry (unsigned char *string)
{
    unsigned char *out;
    int out_length, out_pos;
    
    out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case GUI_ATTR_BOLD_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'B';
                string++;
                break;
            case GUI_ATTR_FIXED_CHAR:
                string++;
                break;
            case GUI_ATTR_RESET_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'O';
                string++;
                break;
            case GUI_ATTR_REVERSE_CHAR:
            case GUI_ATTR_REVERSE2_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'R';
                string++;
                break;
            case GUI_ATTR_ITALIC_CHAR:
                string++;
                break;
            case GUI_ATTR_UNDERLINE_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'R';
                string++;
                break;
            case GUI_ATTR_COLOR_CHAR:
                out[out_pos++] = '%';
                out[out_pos++] = 'C';
                string++;
                break;
            default:
                out[out_pos++] = string[0];
                string++;
        }
    }
    out[out_pos] = '\0';
    return out;
}

/*
 * gui_color_encode: parses a message (entered by user), and
 *                   encode special chars (%B, %C, ..) in IRC colors
 *                   After use, string returned has to be free()
 */

unsigned char *
gui_color_encode (unsigned char *string)
{
    unsigned char *out;
    int out_length, out_pos;
    
    out_length = (strlen ((char *)string) * 2) + 1;
    out = (unsigned char *)malloc (out_length);
    if (!out)
        return NULL;
    
    out_pos = 0;
    while (string[0] && (out_pos < out_length - 1))
    {
        switch (string[0])
        {
            case '%':
                string++;
                switch (string[0])
                {
                    case '\0':
                        out[out_pos++] = '%';
                        break;
                    case '%': /* double '%' replaced by single '%' */
                        out[out_pos++] = string[0];
                        string++;
                        break;
                    case 'B': /* bold */
                        out[out_pos++] = GUI_ATTR_BOLD_CHAR;
                        string++;
                        break;
                    case 'C': /* color */
                        out[out_pos++] = GUI_ATTR_COLOR_CHAR;
                        string++;
                        if (isdigit (string[0]))
                        {
                            out[out_pos++] = string[0];
                            string++;
                            if (isdigit (string[0]))
                            {
                                out[out_pos++] = string[0];
                                string++;
                            }
                        }
                        if (string[0] == ',')
                        {
                            out[out_pos++] = ',';
                            string++;
                            if (isdigit (string[0]))
                            {
                                out[out_pos++] = string[0];
                                string++;
                                if (isdigit (string[0]))
                                {
                                out[out_pos++] = string[0];
                                string++;
                                }
                            }
                        }
                        break;
                    case 'O': /* reset */
                        out[out_pos++] = GUI_ATTR_RESET_CHAR;
                        string++;
                        break;
                    case 'R': /* reverse */
                        out[out_pos++] = GUI_ATTR_REVERSE_CHAR;
                        string++;
                        break;
                    case 'U': /* underline */
                        out[out_pos++] = GUI_ATTR_UNDERLINE_CHAR;
                        string++;
                        break;
                    default:
                        out[out_pos++] = '%';
                        out[out_pos++] = string[0];
                        string++;
                }
                break;
            default:
                out[out_pos++] = string[0];
                string++;
        }
    }
    out[out_pos] = '\0';
    return out;
}

/*
 * gui_color_build: build a WeeChat color with foreground,
 *                  background and attributes (attributes are
 *                  given with foreground color, with a OR)
 */

t_gui_color *
gui_color_build (int number, int foreground, int background)
{
    t_gui_color *new_color;
    
    new_color = (t_gui_color *)malloc (sizeof (t_gui_color));
    if (!new_color)
        return NULL;
    
    new_color->foreground = gui_weechat_colors[foreground].foreground;
    new_color->background = gui_weechat_colors[background].foreground;
    new_color->attributes = gui_weechat_colors[foreground].attributes;
    new_color->string = (char *)malloc (4);
    if (new_color->string)
        snprintf (new_color->string, 4,
                  "%s%02d",
                  GUI_ATTR_WEECHAT_COLOR_STR, number);
    
    return new_color;
}

/*
 * gui_color_get_pair: get color pair with a WeeChat color number
 */

int
gui_color_get_pair (int num_color)
{
    int fg, bg;
    
    if ((num_color < 0) || (num_color > GUI_NUM_COLORS - 1))
        return WEECHAT_COLOR_WHITE;
    
    fg = gui_color[num_color]->foreground;
    bg = gui_color[num_color]->background;
    
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        return 63;
    if ((fg == -1) || (fg == 99))
        fg = WEECHAT_COLOR_WHITE;
    if ((bg == -1) || (bg == 99))
        bg = 0;
    
    return (bg * 8) + fg;
}

/*
 * gui_color_init_pairs: init color pairs
 */

void
gui_color_init_pairs ()
{
    int i;
    char shift_colors[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
    
    if (has_colors ())
    {
        for (i = 1; i < 64; i++)
            init_pair (i, shift_colors[i % 8], (i < 8) ? -1 : shift_colors[i / 8]);
        
        /* disable white on white, replaced by black on white */
        init_pair (63, -1, -1);
        
        /* white on default bg is default (-1) */
        if (!cfg_col_real_white)
            init_pair (WEECHAT_COLOR_WHITE, -1, -1);
    }
}

/*
 * gui_color_init_weechat: init WeeChat colors
 */

void
gui_color_init_weechat ()
{
    int i;
    
    gui_color[COLOR_WIN_SEPARATOR] = gui_color_build (COLOR_WIN_SEPARATOR, cfg_col_separator, cfg_col_separator);
    gui_color[COLOR_WIN_TITLE] = gui_color_build (COLOR_WIN_TITLE, cfg_col_title, cfg_col_title_bg);
    gui_color[COLOR_WIN_TITLE_MORE] = gui_color_build (COLOR_WIN_TITLE_MORE, cfg_col_title_more, cfg_col_title_bg);
    gui_color[COLOR_WIN_CHAT] = gui_color_build (COLOR_WIN_CHAT, cfg_col_chat, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_TIME] = gui_color_build (COLOR_WIN_CHAT_TIME, cfg_col_chat_time, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_TIME_SEP] = gui_color_build (COLOR_WIN_CHAT_TIME_SEP, cfg_col_chat_time_sep, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_PREFIX1] = gui_color_build (COLOR_WIN_CHAT_PREFIX1, cfg_col_chat_prefix1, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_PREFIX2] = gui_color_build (COLOR_WIN_CHAT_PREFIX2, cfg_col_chat_prefix2, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_SERVER] = gui_color_build (COLOR_WIN_CHAT_SERVER, cfg_col_chat_server, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_JOIN] = gui_color_build (COLOR_WIN_CHAT_JOIN, cfg_col_chat_join, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_PART] = gui_color_build (COLOR_WIN_CHAT_PART, cfg_col_chat_part, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_NICK] = gui_color_build (COLOR_WIN_CHAT_NICK, cfg_col_chat_nick, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_HOST] = gui_color_build (COLOR_WIN_CHAT_HOST, cfg_col_chat_host, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_CHANNEL] = gui_color_build (COLOR_WIN_CHAT_CHANNEL, cfg_col_chat_channel, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_DARK] = gui_color_build (COLOR_WIN_CHAT_DARK, cfg_col_chat_dark, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_HIGHLIGHT] = gui_color_build (COLOR_WIN_CHAT_HIGHLIGHT, cfg_col_chat_highlight, cfg_col_chat_bg);
    gui_color[COLOR_WIN_CHAT_READ_MARKER] = gui_color_build (COLOR_WIN_CHAT_READ_MARKER, cfg_col_chat_read_marker, cfg_col_chat_read_marker_bg);
    gui_color[COLOR_WIN_STATUS] = gui_color_build (COLOR_WIN_STATUS, cfg_col_status, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DELIMITERS] = gui_color_build (COLOR_WIN_STATUS_DELIMITERS, cfg_col_status_delimiters, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_CHANNEL] = gui_color_build (COLOR_WIN_STATUS_CHANNEL, cfg_col_status_channel, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DATA_MSG] = gui_color_build (COLOR_WIN_STATUS_DATA_MSG, cfg_col_status_data_msg, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DATA_PRIVATE] = gui_color_build (COLOR_WIN_STATUS_DATA_PRIVATE, cfg_col_status_data_private, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DATA_HIGHLIGHT] = gui_color_build (COLOR_WIN_STATUS_DATA_HIGHLIGHT, cfg_col_status_data_highlight, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_DATA_OTHER] = gui_color_build (COLOR_WIN_STATUS_DATA_OTHER, cfg_col_status_data_other, cfg_col_status_bg);
    gui_color[COLOR_WIN_STATUS_MORE] = gui_color_build (COLOR_WIN_STATUS_MORE, cfg_col_status_more, cfg_col_status_bg);
    gui_color[COLOR_WIN_INFOBAR] = gui_color_build (COLOR_WIN_INFOBAR, cfg_col_infobar, cfg_col_infobar_bg);
    gui_color[COLOR_WIN_INFOBAR_DELIMITERS] = gui_color_build (COLOR_WIN_INFOBAR_DELIMITERS, cfg_col_infobar_delimiters, cfg_col_infobar_bg);
    gui_color[COLOR_WIN_INFOBAR_HIGHLIGHT] = gui_color_build (COLOR_WIN_INFOBAR_HIGHLIGHT, cfg_col_infobar_highlight, cfg_col_infobar_bg);
    gui_color[COLOR_WIN_INPUT] = gui_color_build (COLOR_WIN_INPUT, cfg_col_input, cfg_col_input_bg);
    gui_color[COLOR_WIN_INPUT_SERVER] = gui_color_build (COLOR_WIN_INPUT_SERVER, cfg_col_input_server, cfg_col_input_bg);
    gui_color[COLOR_WIN_INPUT_CHANNEL] = gui_color_build (COLOR_WIN_INPUT_CHANNEL, cfg_col_input_channel, cfg_col_input_bg);
    gui_color[COLOR_WIN_INPUT_NICK] = gui_color_build (COLOR_WIN_INPUT_NICK, cfg_col_input_nick, cfg_col_input_bg);
    gui_color[COLOR_WIN_INPUT_DELIMITERS] = gui_color_build (COLOR_WIN_INPUT_DELIMITERS, cfg_col_input_delimiters, cfg_col_input_bg);
    gui_color[COLOR_WIN_INPUT_TEXT_NOT_FOUND] = gui_color_build (COLOR_WIN_INPUT_TEXT_NOT_FOUND, cfg_col_input_text_not_found, cfg_col_input_bg);
    gui_color[COLOR_WIN_NICK] = gui_color_build (COLOR_WIN_NICK, cfg_col_nick, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_AWAY] = gui_color_build (COLOR_WIN_NICK_AWAY, cfg_col_nick_away, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_CHANOWNER] = gui_color_build (COLOR_WIN_NICK_CHANOWNER, cfg_col_nick_chanowner, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_CHANADMIN] = gui_color_build (COLOR_WIN_NICK_CHANADMIN, cfg_col_nick_chanadmin, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_OP] = gui_color_build (COLOR_WIN_NICK_OP, cfg_col_nick_op, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_HALFOP] = gui_color_build (COLOR_WIN_NICK_HALFOP, cfg_col_nick_halfop, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_VOICE] = gui_color_build (COLOR_WIN_NICK_VOICE, cfg_col_nick_voice, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_MORE] = gui_color_build (COLOR_WIN_NICK_MORE, cfg_col_nick_more, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_SEP] = gui_color_build (COLOR_WIN_NICK_SEP, cfg_col_nick_sep, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_SELF] = gui_color_build (COLOR_WIN_NICK_SELF, cfg_col_nick_self, cfg_col_nick_bg);
    gui_color[COLOR_WIN_NICK_PRIVATE] = gui_color_build (COLOR_WIN_NICK_PRIVATE, cfg_col_nick_private, cfg_col_nick_bg);

    for (i = 0; i < COLOR_WIN_NICK_NUMBER; i++)
    {
        gui_color[COLOR_WIN_NICK_1 + i] = gui_color_build (COLOR_WIN_NICK_1 + i, cfg_col_nick_colors[i], cfg_col_chat_bg);
    }
        
    gui_color[COLOR_DCC_SELECTED] = gui_color_build (COLOR_DCC_SELECTED, cfg_col_dcc_selected, cfg_col_chat_bg);
    gui_color[COLOR_DCC_WAITING] = gui_color_build (COLOR_DCC_WAITING, cfg_col_dcc_waiting, cfg_col_chat_bg);
    gui_color[COLOR_DCC_CONNECTING] = gui_color_build (COLOR_DCC_CONNECTING, cfg_col_dcc_connecting, cfg_col_chat_bg);
    gui_color[COLOR_DCC_ACTIVE] = gui_color_build (COLOR_DCC_ACTIVE, cfg_col_dcc_active, cfg_col_chat_bg);
    gui_color[COLOR_DCC_DONE] = gui_color_build (COLOR_DCC_DONE, cfg_col_dcc_done, cfg_col_chat_bg);
    gui_color[COLOR_DCC_FAILED] = gui_color_build (COLOR_DCC_FAILED, cfg_col_dcc_failed, cfg_col_chat_bg);
    gui_color[COLOR_DCC_ABORTED] = gui_color_build (COLOR_DCC_ABORTED, cfg_col_dcc_aborted, cfg_col_chat_bg);
}

/*
 * gui_color_rebuild_weechat: rebuild WeeChat colors
 */

void
gui_color_rebuild_weechat ()
{
    int i;
    
    if (has_colors ())
    {
        for (i = 0; i < GUI_NUM_COLORS; i++)
        {
            if (gui_color[i])
            {
                if (gui_color[i]->string)
                    free (gui_color[i]->string);
                free (gui_color[i]);
                gui_color[i] = NULL;
            }
        }
        gui_color_init_weechat ();
    }
}

/*
 * gui_color_init: init GUI colors
 */

void
gui_color_init ()
{
    if (has_colors ())
    {
        start_color ();
        use_default_colors ();
    }
    gui_color_init_pairs ();
    gui_color_init_weechat ();
}
