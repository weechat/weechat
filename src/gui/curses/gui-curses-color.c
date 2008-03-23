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

/* gui-curses-color.c: color functions for Curses GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-string.h"
#include "../../core/wee-utf8.h"
#include "../gui-color.h"
#include "../gui-chat.h"
#include "gui-curses.h"


struct t_gui_color gui_weechat_colors[] =
{ { -1,            0, 0,        "default"      },
  { COLOR_BLACK,   0, 0,        "black"        },
  { COLOR_RED,     0, 0,        "red"          },
  { COLOR_RED,     0, A_BOLD,   "lightred"     },
  { COLOR_GREEN,   0, 0,        "green"        },
  { COLOR_GREEN,   0, A_BOLD,   "lightgreen"   },
  { COLOR_YELLOW,  0, 0,        "brown"        },
  { COLOR_YELLOW,  0, A_BOLD,   "yellow"       },
  { COLOR_BLUE,    0, 0,        "blue"         },
  { COLOR_BLUE,    0, A_BOLD,   "lightblue"    },
  { COLOR_MAGENTA, 0, 0,        "magenta"      },
  { COLOR_MAGENTA, 0, A_BOLD,   "lightmagenta" },
  { COLOR_CYAN,    0, 0,        "cyan"         },
  { COLOR_CYAN,    0, A_BOLD,   "lightcyan"    },
  { COLOR_WHITE,   0, A_BOLD,   "white"        },
  { 0,             0, 0,        NULL           }
};

struct t_gui_color *gui_color[GUI_NUM_COLORS];


/*
 * gui_color_search: search a color by name
 *                   Return: number of color in WeeChat colors table
 */

int
gui_color_search (char *color_name)
{
    int i;

    for (i = 0; gui_weechat_colors[i].string; i++)
    {
        if (string_strcasecmp (gui_weechat_colors[i].string, color_name) == 0)
            return i;
    }

    /* color not found */
    return -1;
}

/*
 * gui_color_get_fg_bg: get foreground and background from a string with format:
 *                      foreground,background
 */

/*void
gui_color_get_fg_bg (char *string, char **fg, char **bg)
{
    char *pos, *pos_end_fg;
    
    pos = strchr (string, ',');
    if (pos)
    {
        if (pos > string)
        {
            pos_end_fg = pos - 1;
            while ((pos_end_fg > string) && (pos_end_fg == ' '))
            {
                pos_end_fg--;
            }
            *fg = string_strndup (string, pos_end_fg - string + 1);
        }
        else
            *fg = strudp ("default");
        if (pos[1])
        {
            pos++;
            while (pos[0] && (pos[0] == ' '))
            {
                pos++;
            }
            *bg = strdup (pos);
        }
        else
            *bg = strdup ("default");
    }
    else
    {
        *fg = strdup (string);
        *bg = strdup ("default");
    }
}*/

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
        if (string_strcasecmp (gui_weechat_colors[i].string, color_name) == 0)
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
 * gui_color_assign: assign a WeeChat color (read from config)
 */

/*void
gui_color_assign (t_gui_color **color, char *fg_and_bg)
{
    char *color_fg, *color_bg, *color_fg2, *color_bg2;
    int value_fg, value_bg;
    t_config_option *ptr_option;
    
    if (!(*color))
    {
        *color = malloc (sizeof (**color));
        if (!(*color))
            return;
        *color->foreground = 0;
        *color->background = 0;
        *color->attributes = 0;
        *color->string = NULL;
    }

    gui_color_get_fg_bg (fg_and_bg, &color_fg, &color_bg);

    if (color_fg && color_bg)
    {
        // look for curses colors in table 
        value_fg = gui_color_search (color_fg);
        value_bg = gui_color_search (color_bg);
        
        if (value_fg < 0)
        {
            // it's not a known value for foreground, maybe it's reference to
            // another config option ?
            value_fg = 0;
            ptr_option = config_option_section_option_search (weechat_config_sections,
                                                              weechat_config_options,
                                                              color_fg);
            if (ptr_option && *(ptr_option->ptr_color)
                && *(ptr_option->ptr_string))
            {
                gui_color_get_fg_bg (*(ptr_option->ptr_string),
                                     &color_fg2, &color_bg2);
                if (color_fg2)
                    value_fg = gui_color_search (color_fg2);

                if (color_fg2)
                    free (color_fg2);
                if (color_bg2)
                    free (color_bg2);
            }
        }
        
        if (value_bg < 0)
        {
            // it's not a known value for background, maybe it's reference to
            // another config option ?
            value_bg = 0;
            ptr_option = config_option_section_option_search (weechat_config_sections,
                                                              weechat_config_options,
                                                              color_bg);
            if (ptr_option && *(ptr_option->ptr_color)
                && *(ptr_option->ptr_string))
            {
                gui_color_get_fg_bg (*(ptr_option->ptr_string),
                                     &color_fg2, &color_bg2);
                if (color_bg2)
                    value_bg = gui_color_search (color_bg2);
                
                if (color_fg2)
                    free (color_fg2);
                if (color_bg2)
                    free (color_bg2);
            }
        }
        
        *color->foreground = gui_weechat_colors[value_fg].foreground;
        *color->background = gui_weechat_colors[value_bg].background;
        *color->attributes = gui_weechat_colors[value_fg].attributes;
        
        if (*color->string)
            free (*color->string);
        *color->string = malloc (4);
        if (*color->string)
            snprintf (*color->string, 4,
                      "%s%02d",
                      GUI_COLOR_COLOR_STR, number);
    }
    else
    {
        *color->foreground = 0;
        *color->background = 0;
        *color->attributes = 0;
        if (*color->string)
            free (*color->string);
        *color->string = NULL;
    }
    
    if (color_fg)
        free (color_fg);
    if (color_bg)
        free (color_bg);
}*/

/*
 * gui_color_get_name: get color name
 */

char *
gui_color_get_name (int num_color)
{
    return gui_weechat_colors[num_color].string;
}

/*
 * gui_color_build: build a WeeChat color with foreground,
 *                  background and attributes (attributes are
 *                  given with foreground color, with a OR)
 */

struct t_gui_color *
gui_color_build (int number, int foreground, int background)
{
    struct t_gui_color *new_color;
    
    new_color = malloc (sizeof (*new_color));
    if (!new_color)
        return NULL;
    
    new_color->foreground = gui_weechat_colors[foreground].foreground;
    new_color->background = gui_weechat_colors[background].foreground;
    new_color->attributes = gui_weechat_colors[foreground].attributes;
    new_color->string = malloc (4);
    if (new_color->string)
        snprintf (new_color->string, 4,
                  "%s%02d",
                  GUI_COLOR_COLOR_STR, number);
    
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
        return COLOR_WHITE;
    
    fg = gui_color[num_color]->foreground;
    bg = gui_color[num_color]->background;
    
    if (((fg == -1) || (fg == 99))
        && ((bg == -1) || (bg == 99)))
        return 63;
    if ((fg == -1) || (fg == 99))
        fg = COLOR_WHITE;
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
    
    if (has_colors ())
    {
        for (i = 1; i < 64; i++)
            init_pair (i, i % 8, (i < 8) ? -1 : i / 8);
        
        /* disable white on white, replaced by black on white */
        init_pair (63, -1, -1);
        
        /* white on default bg is default (-1) */
        if (!CONFIG_BOOLEAN(config_look_color_real_white))
            init_pair (COLOR_WHITE, -1, -1);
    }
}

/*
 * gui_color_init_weechat: init WeeChat colors
 */

void
gui_color_init_weechat ()
{
    int i;
    
    gui_color[GUI_COLOR_SEPARATOR] = gui_color_build (GUI_COLOR_SEPARATOR, CONFIG_COLOR(config_color_separator), CONFIG_COLOR(config_color_chat_bg));
    
    gui_color[GUI_COLOR_TITLE] = gui_color_build (GUI_COLOR_TITLE, CONFIG_COLOR(config_color_title), CONFIG_COLOR(config_color_title_bg));
    gui_color[GUI_COLOR_TITLE_MORE] = gui_color_build (GUI_COLOR_TITLE_MORE, CONFIG_COLOR(config_color_title_more), CONFIG_COLOR(config_color_title_bg));
    
    gui_color[GUI_COLOR_CHAT] = gui_color_build (GUI_COLOR_CHAT, CONFIG_COLOR(config_color_chat), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_TIME] = gui_color_build (GUI_COLOR_CHAT_TIME, CONFIG_COLOR(config_color_chat_time), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_TIME_DELIMITERS] = gui_color_build (GUI_COLOR_CHAT_TIME_DELIMITERS, CONFIG_COLOR(config_color_chat_time_delimiters), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_PREFIX_INFO] = gui_color_build (GUI_COLOR_CHAT_PREFIX_INFO, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_INFO]), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_PREFIX_ERROR] = gui_color_build (GUI_COLOR_CHAT_PREFIX_ERROR, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_ERROR]), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_PREFIX_NETWORK] = gui_color_build (GUI_COLOR_CHAT_PREFIX_NETWORK, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_NETWORK]), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_PREFIX_ACTION] = gui_color_build (GUI_COLOR_CHAT_PREFIX_ACTION, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_ACTION]), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_PREFIX_JOIN] = gui_color_build (GUI_COLOR_CHAT_PREFIX_JOIN, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_JOIN]), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_PREFIX_QUIT] = gui_color_build (GUI_COLOR_CHAT_PREFIX_QUIT, CONFIG_COLOR(config_color_chat_prefix[GUI_CHAT_PREFIX_QUIT]), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_PREFIX_MORE] = gui_color_build (GUI_COLOR_CHAT_PREFIX_MORE, CONFIG_COLOR(config_color_chat_prefix_more), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_PREFIX_SUFFIX] = gui_color_build (GUI_COLOR_CHAT_PREFIX_SUFFIX, CONFIG_COLOR(config_color_chat_prefix_suffix), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_BUFFER] = gui_color_build (GUI_COLOR_CHAT_BUFFER, CONFIG_COLOR(config_color_chat_buffer), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_SERVER] = gui_color_build (GUI_COLOR_CHAT_SERVER, CONFIG_COLOR(config_color_chat_server), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_CHANNEL] = gui_color_build (GUI_COLOR_CHAT_CHANNEL, CONFIG_COLOR(config_color_chat_channel), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_NICK] = gui_color_build (GUI_COLOR_CHAT_NICK, CONFIG_COLOR(config_color_chat_nick), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_NICK_SELF] = gui_color_build (GUI_COLOR_CHAT_NICK_SELF, CONFIG_COLOR(config_color_chat_nick_self), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_NICK_OTHER] = gui_color_build (GUI_COLOR_CHAT_NICK_OTHER, CONFIG_COLOR(config_color_chat_nick_other), CONFIG_COLOR(config_color_chat_bg));
    for (i = 0; i < GUI_COLOR_NICK_NUMBER; i++)
    {
        gui_color[GUI_COLOR_CHAT_NICK1 + i] = gui_color_build (GUI_COLOR_CHAT_NICK1 + i, CONFIG_COLOR(config_color_chat_nick_colors[i]), CONFIG_COLOR(config_color_chat_bg));
    }
    gui_color[GUI_COLOR_CHAT_HOST] = gui_color_build (GUI_COLOR_CHAT_HOST, CONFIG_COLOR(config_color_chat_host), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_DELIMITERS] = gui_color_build (GUI_COLOR_CHAT_DELIMITERS, CONFIG_COLOR(config_color_chat_delimiters), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_HIGHLIGHT] = gui_color_build (GUI_COLOR_CHAT_HIGHLIGHT, CONFIG_COLOR(config_color_chat_highlight), CONFIG_COLOR(config_color_chat_bg));
    gui_color[GUI_COLOR_CHAT_READ_MARKER] = gui_color_build (GUI_COLOR_CHAT_READ_MARKER, CONFIG_COLOR(config_color_chat_read_marker), CONFIG_COLOR(config_color_chat_read_marker_bg));
    
    gui_color[GUI_COLOR_STATUS] = gui_color_build (GUI_COLOR_STATUS, CONFIG_COLOR(config_color_status), CONFIG_COLOR(config_color_status_bg));
    gui_color[GUI_COLOR_STATUS_DELIMITERS] = gui_color_build (GUI_COLOR_STATUS_DELIMITERS, CONFIG_COLOR(config_color_status_delimiters), CONFIG_COLOR(config_color_status_bg));
    gui_color[GUI_COLOR_STATUS_NUMBER] = gui_color_build (GUI_COLOR_STATUS_NUMBER, CONFIG_COLOR(config_color_status_number), CONFIG_COLOR(config_color_status_bg));
    gui_color[GUI_COLOR_STATUS_CATEGORY] = gui_color_build (GUI_COLOR_STATUS_CATEGORY, CONFIG_COLOR(config_color_status_category), CONFIG_COLOR(config_color_status_bg));
    gui_color[GUI_COLOR_STATUS_NAME] = gui_color_build (GUI_COLOR_STATUS_NAME, CONFIG_COLOR(config_color_status_name), CONFIG_COLOR(config_color_status_bg));
    gui_color[GUI_COLOR_STATUS_DATA_MSG] = gui_color_build (GUI_COLOR_STATUS_DATA_MSG, CONFIG_COLOR(config_color_status_data_msg), CONFIG_COLOR(config_color_status_bg));
    gui_color[GUI_COLOR_STATUS_DATA_PRIVATE] = gui_color_build (GUI_COLOR_STATUS_DATA_PRIVATE, CONFIG_COLOR(config_color_status_data_private), CONFIG_COLOR(config_color_status_bg));
    gui_color[GUI_COLOR_STATUS_DATA_HIGHLIGHT] = gui_color_build (GUI_COLOR_STATUS_DATA_HIGHLIGHT, CONFIG_COLOR(config_color_status_data_highlight), CONFIG_COLOR(config_color_status_bg));
    gui_color[GUI_COLOR_STATUS_DATA_OTHER] = gui_color_build (GUI_COLOR_STATUS_DATA_OTHER, CONFIG_COLOR(config_color_status_data_other), CONFIG_COLOR(config_color_status_bg));
    gui_color[GUI_COLOR_STATUS_MORE] = gui_color_build (GUI_COLOR_STATUS_MORE, CONFIG_COLOR(config_color_status_more), CONFIG_COLOR(config_color_status_bg));
    
    gui_color[GUI_COLOR_INFOBAR] = gui_color_build (GUI_COLOR_INFOBAR, CONFIG_COLOR(config_color_infobar), CONFIG_COLOR(config_color_infobar_bg));
    gui_color[GUI_COLOR_INFOBAR_DELIMITERS] = gui_color_build (GUI_COLOR_INFOBAR_DELIMITERS, CONFIG_COLOR(config_color_infobar_delimiters), CONFIG_COLOR(config_color_infobar_bg));
    gui_color[GUI_COLOR_INFOBAR_HIGHLIGHT] = gui_color_build (GUI_COLOR_INFOBAR_HIGHLIGHT, CONFIG_COLOR(config_color_infobar_highlight), CONFIG_COLOR(config_color_infobar_bg));
    
    gui_color[GUI_COLOR_INPUT] = gui_color_build (GUI_COLOR_INPUT, CONFIG_COLOR(config_color_input), CONFIG_COLOR(config_color_input_bg));
    gui_color[GUI_COLOR_INPUT_SERVER] = gui_color_build (GUI_COLOR_INPUT_SERVER, CONFIG_COLOR(config_color_input_server), CONFIG_COLOR(config_color_input_bg));
    gui_color[GUI_COLOR_INPUT_CHANNEL] = gui_color_build (GUI_COLOR_INPUT_CHANNEL, CONFIG_COLOR(config_color_input_channel), CONFIG_COLOR(config_color_input_bg));
    gui_color[GUI_COLOR_INPUT_NICK] = gui_color_build (GUI_COLOR_INPUT_NICK, CONFIG_COLOR(config_color_input_nick), CONFIG_COLOR(config_color_input_bg));
    gui_color[GUI_COLOR_INPUT_DELIMITERS] = gui_color_build (GUI_COLOR_INPUT_DELIMITERS, CONFIG_COLOR(config_color_input_delimiters), CONFIG_COLOR(config_color_input_bg));
    gui_color[GUI_COLOR_INPUT_TEXT_NOT_FOUND] = gui_color_build (GUI_COLOR_INPUT_TEXT_NOT_FOUND, CONFIG_COLOR(config_color_input_text_not_found), CONFIG_COLOR(config_color_input_bg));
    gui_color[GUI_COLOR_INPUT_ACTIONS] = gui_color_build (GUI_COLOR_INPUT_ACTIONS, CONFIG_COLOR(config_color_input_actions), CONFIG_COLOR(config_color_input_bg));
    
    gui_color[GUI_COLOR_NICKLIST] = gui_color_build (GUI_COLOR_NICKLIST, CONFIG_COLOR(config_color_nicklist), CONFIG_COLOR(config_color_nicklist_bg));
    gui_color[GUI_COLOR_NICKLIST_GROUP] = gui_color_build (GUI_COLOR_NICKLIST_GROUP, CONFIG_COLOR(config_color_nicklist_group), CONFIG_COLOR(config_color_nicklist_bg));
    gui_color[GUI_COLOR_NICKLIST_AWAY] = gui_color_build (GUI_COLOR_NICKLIST_AWAY, CONFIG_COLOR(config_color_nicklist_away), CONFIG_COLOR(config_color_nicklist_bg));
    gui_color[GUI_COLOR_NICKLIST_PREFIX1] = gui_color_build (GUI_COLOR_NICKLIST_PREFIX1, CONFIG_COLOR(config_color_nicklist_prefix1), CONFIG_COLOR(config_color_nicklist_bg));
    gui_color[GUI_COLOR_NICKLIST_PREFIX2] = gui_color_build (GUI_COLOR_NICKLIST_PREFIX2, CONFIG_COLOR(config_color_nicklist_prefix2), CONFIG_COLOR(config_color_nicklist_bg));
    gui_color[GUI_COLOR_NICKLIST_PREFIX3] = gui_color_build (GUI_COLOR_NICKLIST_PREFIX3, CONFIG_COLOR(config_color_nicklist_prefix3), CONFIG_COLOR(config_color_nicklist_bg));
    gui_color[GUI_COLOR_NICKLIST_PREFIX4] = gui_color_build (GUI_COLOR_NICKLIST_PREFIX4, CONFIG_COLOR(config_color_nicklist_prefix4), CONFIG_COLOR(config_color_nicklist_bg));
    gui_color[GUI_COLOR_NICKLIST_PREFIX5] = gui_color_build (GUI_COLOR_NICKLIST_PREFIX5, CONFIG_COLOR(config_color_nicklist_prefix5), CONFIG_COLOR(config_color_nicklist_bg));
    gui_color[GUI_COLOR_NICKLIST_MORE] = gui_color_build (GUI_COLOR_NICKLIST_MORE, CONFIG_COLOR(config_color_nicklist_more), CONFIG_COLOR(config_color_nicklist_bg));
    gui_color[GUI_COLOR_NICKLIST_SEPARATOR] = gui_color_build (GUI_COLOR_NICKLIST_SEPARATOR, CONFIG_COLOR(config_color_nicklist_separator), CONFIG_COLOR(config_color_nicklist_bg));
    
    gui_color[GUI_COLOR_INFO] = gui_color_build (GUI_COLOR_INFO, CONFIG_COLOR(config_color_info), CONFIG_COLOR(config_color_info_bg));
    gui_color[GUI_COLOR_INFO_WAITING] = gui_color_build (GUI_COLOR_INFO_WAITING, CONFIG_COLOR(config_color_info_waiting), CONFIG_COLOR(config_color_info_bg));
    gui_color[GUI_COLOR_INFO_CONNECTING] = gui_color_build (GUI_COLOR_INFO_CONNECTING, CONFIG_COLOR(config_color_info_connecting), CONFIG_COLOR(config_color_info_bg));
    gui_color[GUI_COLOR_INFO_ACTIVE] = gui_color_build (GUI_COLOR_INFO_ACTIVE, CONFIG_COLOR(config_color_info_active), CONFIG_COLOR(config_color_info_bg));
    gui_color[GUI_COLOR_INFO_DONE] = gui_color_build (GUI_COLOR_INFO_DONE, CONFIG_COLOR(config_color_info_done), CONFIG_COLOR(config_color_info_bg));
    gui_color[GUI_COLOR_INFO_FAILED] = gui_color_build (GUI_COLOR_INFO_FAILED, CONFIG_COLOR(config_color_info_failed), CONFIG_COLOR(config_color_info_bg));
    gui_color[GUI_COLOR_INFO_ABORTED] = gui_color_build (GUI_COLOR_INFO_ABORTED, CONFIG_COLOR(config_color_info_aborted), CONFIG_COLOR(config_color_info_bg));
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

/*
 * gui_color_end: end GUI colors
 */

void
gui_color_end ()
{
    int i;

    for (i = 0; i < GUI_NUM_COLORS; i++)
    {
        gui_color_free (gui_color[i]);
    }
}
