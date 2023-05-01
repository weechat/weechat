/*
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_GUI_COLOR_H
#define WEECHAT_GUI_COLOR_H

#include <regex.h>

struct t_config_option;

/*
 * Color from configuration options.
 *
 * When changing some colors below:
 * - always add to the end
 * - never remove a color (mark it as obsolete if needed)
 * - do not re-use an obsolete color
 * - add build of color in file src/gui/curses/gui-curses-color.c,
 *   function gui_color_init_weechat ()
 * - update the Developer's guide
 */

enum t_gui_color_enum
{
    GUI_COLOR_SEPARATOR = 0,

    GUI_COLOR_CHAT,
    GUI_COLOR_CHAT_TIME,
    GUI_COLOR_CHAT_TIME_DELIMITERS,
    GUI_COLOR_CHAT_PREFIX_ERROR,
    GUI_COLOR_CHAT_PREFIX_NETWORK,
    GUI_COLOR_CHAT_PREFIX_ACTION,
    GUI_COLOR_CHAT_PREFIX_JOIN,
    GUI_COLOR_CHAT_PREFIX_QUIT,
    GUI_COLOR_CHAT_PREFIX_MORE,
    GUI_COLOR_CHAT_PREFIX_SUFFIX,
    GUI_COLOR_CHAT_BUFFER,
    GUI_COLOR_CHAT_SERVER,
    GUI_COLOR_CHAT_CHANNEL,
    GUI_COLOR_CHAT_NICK,
    GUI_COLOR_CHAT_NICK_SELF,
    GUI_COLOR_CHAT_NICK_OTHER,
    /*
     * following obsolete colors are kept here for compatibility
     * with WeeChat <= 0.3.3
     */
    GUI_COLOR_CHAT_NICK1_OBSOLETE,  /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK2_OBSOLETE,  /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK3_OBSOLETE,  /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK4_OBSOLETE,  /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK5_OBSOLETE,  /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK6_OBSOLETE,  /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK7_OBSOLETE,  /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK8_OBSOLETE,  /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK9_OBSOLETE,  /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK10_OBSOLETE, /* not used any more since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_HOST,
    GUI_COLOR_CHAT_DELIMITERS,
    GUI_COLOR_CHAT_HIGHLIGHT,
    GUI_COLOR_CHAT_READ_MARKER,
    GUI_COLOR_CHAT_TEXT_FOUND,
    GUI_COLOR_CHAT_VALUE,
    GUI_COLOR_CHAT_PREFIX_BUFFER,
    GUI_COLOR_CHAT_TAGS,
    GUI_COLOR_CHAT_INACTIVE_WINDOW,
    GUI_COLOR_CHAT_INACTIVE_BUFFER,
    GUI_COLOR_CHAT_PREFIX_BUFFER_INACTIVE_BUFFER,
    GUI_COLOR_CHAT_NICK_OFFLINE,
    GUI_COLOR_CHAT_NICK_OFFLINE_HIGHLIGHT,
    GUI_COLOR_CHAT_NICK_PREFIX,
    GUI_COLOR_CHAT_NICK_SUFFIX,
    GUI_COLOR_EMPHASIS,
    GUI_COLOR_CHAT_DAY_CHANGE,
    GUI_COLOR_CHAT_VALUE_NULL,
    GUI_COLOR_CHAT_STATUS_DISABLED,
    GUI_COLOR_CHAT_STATUS_ENABLED,

    /* number of colors */
    GUI_COLOR_NUM_COLORS,
};

/* WeeChat internal color attributes (should never be in protocol messages) */

#define GUI_COLOR_COLOR_CHAR           '\x19'
#define GUI_COLOR_SET_ATTR_CHAR        '\x1A'
#define GUI_COLOR_REMOVE_ATTR_CHAR     '\x1B'
#define GUI_COLOR_RESET_CHAR           '\x1C'

#define GUI_COLOR_ATTR_BOLD_CHAR       '\x01'
#define GUI_COLOR_ATTR_REVERSE_CHAR    '\x02'
#define GUI_COLOR_ATTR_ITALIC_CHAR     '\x03'
#define GUI_COLOR_ATTR_UNDERLINE_CHAR  '\x04'
#define GUI_COLOR_ATTR_BLINK_CHAR      '\x05'
#define GUI_COLOR_ATTR_DIM_CHAR        '\x06'

#define GUI_COLOR(color) ((gui_color[color]) ? gui_color[color]->string : "")
#define GUI_NO_COLOR     "\x1C"

#define GUI_COLOR_CUSTOM_BAR_FG    (gui_color_get_custom ("bar_fg"))
#define GUI_COLOR_CUSTOM_BAR_DELIM (gui_color_get_custom ("bar_delim"))
#define GUI_COLOR_CUSTOM_BAR_BG    (gui_color_get_custom ("bar_bg"))

/* color codes for chat and bars */
#define GUI_COLOR_FG_CHAR                     'F'
#define GUI_COLOR_BG_CHAR                     'B'
#define GUI_COLOR_FG_BG_CHAR                  '*'
#define GUI_COLOR_EXTENDED_CHAR               '@'
#define GUI_COLOR_EXTENDED_BLINK_CHAR         '%'
#define GUI_COLOR_EXTENDED_DIM_CHAR           '.'
#define GUI_COLOR_EXTENDED_BOLD_CHAR          '*'
#define GUI_COLOR_EXTENDED_REVERSE_CHAR       '!'
#define GUI_COLOR_EXTENDED_ITALIC_CHAR        '/'
#define GUI_COLOR_EXTENDED_UNDERLINE_CHAR     '_'
#define GUI_COLOR_EXTENDED_KEEPATTR_CHAR      '|'
#define GUI_COLOR_EMPHASIS_CHAR               'E'

/* color codes specific to bars */
#define GUI_COLOR_BAR_CHAR                    'b'
#define GUI_COLOR_BAR_FG_CHAR                 'F'
#define GUI_COLOR_BAR_DELIM_CHAR              'D'
#define GUI_COLOR_BAR_BG_CHAR                 'B'
#define GUI_COLOR_BAR_START_INPUT_CHAR        '_'
#define GUI_COLOR_BAR_START_INPUT_HIDDEN_CHAR '-'
#define GUI_COLOR_BAR_MOVE_CURSOR_CHAR        '#'
#define GUI_COLOR_BAR_START_ITEM              'i'
#define GUI_COLOR_BAR_START_LINE_ITEM         'l'
#define GUI_COLOR_BAR_SPACER                  's'

#define GUI_COLOR_EXTENDED_FLAG               0x0100000
#define GUI_COLOR_EXTENDED_BOLD_FLAG          0x0200000
#define GUI_COLOR_EXTENDED_REVERSE_FLAG       0x0400000
#define GUI_COLOR_EXTENDED_ITALIC_FLAG        0x0800000
#define GUI_COLOR_EXTENDED_UNDERLINE_FLAG     0x1000000
#define GUI_COLOR_EXTENDED_KEEPATTR_FLAG      0x2000000
#define GUI_COLOR_EXTENDED_BLINK_FLAG         0x4000000
#define GUI_COLOR_EXTENDED_DIM_FLAG           0x8000000
#define GUI_COLOR_EXTENDED_MASK               0x00FFFFF
#define GUI_COLOR_EXTENDED_MAX                99999

#define GUI_COLOR_REGEX_ANSI_DECODE             \
    "\33("                                      \
    "([()].)|"                                  \
    "([<>])|"                                   \
    "(\\[[0-9;?]*[A-Za-z]))"

#define GUI_COLOR_BUFFER_NAME "color"

/* color structure */

struct t_gui_color
{
    int foreground;                /* foreground color                      */
    int background;                /* background color                      */
    int attributes;                /* attributes (bold, ..)                 */
    char *string;                  /* WeeChat color: "\x19??", ?? is #color */
};

/* custom color in palette */

struct t_gui_color_palette
{
    char *alias;                   /* alias name for this color pair        */
    int r, g, b;                   /* red/green/blue values for color       */
};

/* color variables */

extern struct t_gui_color *gui_color[];
extern struct t_hashtable *gui_color_hash_palette_color;
extern struct t_hashtable *gui_color_hash_palette_alias;
extern struct t_weelist *gui_color_list_with_alias;
extern int gui_color_term256[];

/* color functions */

extern const char *gui_color_from_option (struct t_config_option *option);
extern const char *gui_color_search_config (const char *color_name);
extern int gui_color_attr_get_flag (char c);
extern void gui_color_attr_build_string (int color, char *str_attr);
extern const char *gui_color_get_custom (const char *color_name);
extern int gui_color_convert_term_to_rgb (int color);
extern int gui_color_convert_rgb_to_term (int rgb, int limit);
extern int gui_color_code_size (const char *string);
extern char *gui_color_decode (const char *string, const char *replacement);
extern char *gui_color_decode_ansi (const char *string, int keep_colors);
extern char *gui_color_encode_ansi (const char *string);
extern char *gui_color_emphasize (const char *string, const char *search,
                                  int case_sensitive, regex_t *regex);
extern void gui_color_free (struct t_gui_color *color);
extern void gui_color_palette_alloc_structs ();
extern int gui_color_palette_get_alias (const char *alias);
extern struct t_gui_color_palette *gui_color_palette_get (int number);
extern void gui_color_palette_add (int number, const char *value);
extern void gui_color_palette_remove (int number);
extern void gui_color_init ();
extern void gui_color_end ();

/* color functions (GUI dependent) */

extern int gui_color_search (const char *color_name);
extern const char *gui_color_search_index (int index);
extern int gui_color_get_extended_flags (int attrs);
extern int gui_color_assign (int *color, char const *color_name);
extern int gui_color_assign_by_diff (int *color, const char *color_name,
                                     int diff);
extern int gui_color_get_weechat_colors_number ();
extern int gui_color_get_term_colors ();
extern int gui_color_get_term_color_pairs ();
extern const char *gui_color_get_name (int num_color);
extern void gui_color_free_vars ();
extern void gui_color_init_weechat ();
extern void gui_color_display_terminal_colors ();
extern void gui_color_info_term_colors (char *buffer, int size);
extern void gui_color_buffer_display ();
extern void gui_color_switch_colors ();
extern void gui_color_reset_pairs ();
extern void gui_color_buffer_assign ();
extern void gui_color_buffer_open ();
extern void gui_color_palette_build_aliases ();
extern struct t_gui_color_palette *gui_color_palette_new (int number,
                                                          const char *value);
extern void gui_color_palette_free (struct t_gui_color_palette *color_palette);
extern void gui_color_dump ();

#endif /* WEECHAT_GUI_COLOR_H */
