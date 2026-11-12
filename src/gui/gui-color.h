/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
     * Following obsolete colors are kept here for compatibility
     * with WeeChat <= 0.3.3.
     */
    GUI_COLOR_CHAT_NICK1_OBSOLETE,  /* not used anymore since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK2_OBSOLETE,  /* not used anymore since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK3_OBSOLETE,  /* not used anymore since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK4_OBSOLETE,  /* not used anymore since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK5_OBSOLETE,  /* not used anymore since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK6_OBSOLETE,  /* not used anymore since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK7_OBSOLETE,  /* not used anymore since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK8_OBSOLETE,  /* not used anymore since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK9_OBSOLETE,  /* not used anymore since WeeChat 0.3.4 */
    GUI_COLOR_CHAT_NICK10_OBSOLETE, /* not used anymore since WeeChat 0.3.4 */
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

/* Color codes for chat and bars */
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

/* Color codes specific to bars */
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

/*
 * RGB colors ("#RRGGBB") are interned: each distinct RGB value gets a number
 * above GUI_COLOR_EXTENDED_MAX, used like any other extended color number;
 * in strings, the RGB value is always used: "@#RRGGBB" (instead of "@00123").
 */
#define GUI_COLOR_EXTENDED_RGB_CHAR           '#'
#define GUI_COLOR_RGB_FIRST                   (GUI_COLOR_EXTENDED_MAX + 1)
/* RGB colors used in options: numbers never change */
#define GUI_COLOR_RGB_OPTIONS_FIRST           GUI_COLOR_RGB_FIRST
#define GUI_COLOR_RGB_OPTIONS_MAX             100000
/* RGB colors used in strings: numbers are reset with color pairs */
#define GUI_COLOR_RGB_DISPLAY_FIRST                                     \
    (GUI_COLOR_RGB_OPTIONS_FIRST + GUI_COLOR_RGB_OPTIONS_MAX)
#define GUI_COLOR_RGB_DISPLAY_MAX             32768
#define GUI_COLOR_RGB_LAST                                              \
    (GUI_COLOR_RGB_DISPLAY_FIRST + GUI_COLOR_RGB_DISPLAY_MAX - 1)

/* Size of cache for downgrade of RGB colors (power of 2) */
#define GUI_COLOR_DOWNGRADE_CACHE_SIZE        64

#define GUI_COLOR_REGEX_ANSI_DECODE             \
    "\33("                                      \
    "([()].)|"                                  \
    "([<>])|"                                   \
    "(\\[[0-9;?]*[A-Za-z]))"

/* RGB color in the result of ANSI color parsing: flag + 0xRRGGBB */
#define GUI_COLOR_ANSI_RGB_FLAG               0x1000000

/* Parameter given to the callback after the last parameter of a sequence */
#define GUI_COLOR_ANSI_PARAM_END              -1

#define GUI_COLOR_ANSI_DEFAULT_FG 39
#define GUI_COLOR_ANSI_DEFAULT_BG 49

#define GUI_COLOR_BUFFER_NAME "color"

/* Callback for each parameter of an ANSI escape sequence */
typedef void (t_gui_color_ansi_param_cb)(char **output, int value,
                                         int color);

/* Data for function gui_color_ansi_replace */
struct t_gui_color_ansi_replace
{
    int keep_colors;                     /* 1 to keep colors               */
    t_gui_color_ansi_param_cb *callback; /* callback for each parameter    */
};

/* Color structure */

struct t_gui_color
{
    int foreground;                /* foreground color                      */
    int background;                /* background color                      */
    int attributes;                /* attributes (bold, ..)                 */
    char *string;                  /* WeeChat color: "\x19??", ?? is #color */
};

/* Table of RGB colors (each RGB value has a color number) */

struct t_gui_color_rgb_table
{
    int first;                     /* color number of first RGB color       */
    int max;                       /* max number of RGB colors in table     */
    int *values;                   /* RGB value of each color               */
    int *term;                     /* nearest terminal color (cache)        */
    int count;                     /* number of RGB colors in table         */
    int size;                      /* allocated size of arrays              */
    int term_limit;                /* limit used for the cache "term"       */
    struct t_hashtable *index;     /* RGB value -> index in table           */
};

/* Custom color in palette */

struct t_gui_color_palette
{
    char *alias;                   /* alias name for this color pair        */
    int r, g, b;                   /* red/green/blue values for color       */
};

/* Color variables */

extern struct t_gui_color *gui_color[];
extern struct t_hashtable *gui_color_hash_palette_color;
extern struct t_hashtable *gui_color_hash_palette_alias;
extern struct t_weelist *gui_color_list_with_alias;
extern int gui_color_term256[];
extern struct t_gui_color_rgb_table gui_color_rgb_options;
extern struct t_gui_color_rgb_table gui_color_rgb_display;
extern int gui_color_rgb_display_reset_needed;
extern time_t gui_color_rgb_display_reset_last;

/* Color functions */

extern const char *gui_color_from_option (struct t_config_option *option);
extern const char *gui_color_search_config (const char *color_name);
extern int gui_color_attr_char_is_valid (char c);
extern int gui_color_attr_get_flag (char c);
extern void gui_color_attr_build_string (int color, char *str_attr);
extern const char *gui_color_get_custom (const char *color_name);
extern int gui_color_convert_term_to_rgb (int color);
extern int gui_color_palette_rgb (int color, int limit);
extern int gui_color_convert_rgb_to_term (int rgb, int limit);
extern int gui_color_convert_rgb_to_term_display (int rgb, int limit);
extern int gui_color_convert_rgb_to_term_range (int rgb, int first,
                                                int limit);
extern int gui_color_ansi_is_indexed (const char *sequence);
extern int gui_color_convert_term_to_direct (int color, int num_indexed);
extern int gui_color_rgb_to_direct (int rgb);
extern int gui_color_hue_to_rgb (int hue);
extern const char *gui_color_suggest_term_direct (const char *term,
                                                  int tmux);
extern int gui_color_rgb_parse (const char *string);
extern int gui_color_rgb_parse_exact (const char *string);
extern unsigned long long gui_color_hash_key_cb (struct t_hashtable *hashtable,
                                                const void *key);
extern int gui_color_rgb_table_add (struct t_gui_color_rgb_table *table,
                                    int rgb);
extern struct t_gui_color_rgb_table *gui_color_rgb_table_search (int number,
                                                                int *index);
extern void gui_color_rgb_table_free (struct t_gui_color_rgb_table *table);
extern int gui_color_rgb_to_number (int rgb);
extern int gui_color_rgb_to_number_option (int rgb);
extern void gui_color_rgb_display_reset (void);
extern int gui_color_rgb_from_number (int number);
extern int gui_color_rgb_to_term_cached (int number, int limit);
extern int gui_color_extended_size (const char *string);
extern int gui_color_extended_value (const char *string, int *rgb,
                                     int *color);
extern int gui_color_extended_parse (const char *string, int *color);
extern const char *gui_color_extended_string (int number);
extern int gui_color_code_size (const char *string);
extern char *gui_color_decode (const char *string, const char *replacement);
extern int gui_color_downgrade_rgb_term (int rgb);
extern char *gui_color_downgrade_rgb (const char *string);
extern char *gui_color_downgrade_rgb_name (const char *name);
extern int gui_color_ansi_rgb_component (const char *item);
extern char *gui_color_ansi_parse_sequence (const char *sequence,
                                            t_gui_color_ansi_param_cb *callback);
extern char *gui_color_ansi_replace (const char *string, int keep_colors,
                                     t_gui_color_ansi_param_cb *callback);
extern char *gui_color_decode_ansi (const char *string, int keep_colors);
extern int gui_color_encode_ansi_extended (char **output, const char *string,
                                           int background, int rgb_colors);
extern void gui_color_encode_ansi_color (char *buffer, int size, int color,
                                         int background, int rgb_colors);
extern char *gui_color_encode_ansi_rgb (const char *string, int rgb_colors);
extern char *gui_color_encode_ansi (const char *string);
extern char *gui_color_emphasize (const char *string, const char *search,
                                  int case_sensitive, regex_t *regex);
extern void gui_color_free (struct t_gui_color *color);
extern void gui_color_palette_alloc_structs (void);
extern int gui_color_palette_get_alias (const char *alias);
extern struct t_gui_color_palette *gui_color_palette_get (int number);
extern void gui_color_palette_add (int number, const char *value);
extern void gui_color_palette_remove (int number);
extern void gui_color_init (void);
extern void gui_color_end (void);

/* Color functions (GUI dependent) */

extern int gui_color_search (const char *color_name);
extern const char *gui_color_search_index (int index);
extern int gui_color_get_extended_flags (int attrs);
extern int gui_color_assign (int *color, char const *color_name);
extern int gui_color_assign_by_diff (int *color, const char *color_name,
                                     int diff);
extern int gui_color_get_weechat_colors_number (void);
extern int gui_color_get_term_colors (void);
extern int gui_color_get_term_color_pairs (void);
extern int gui_color_get_pair_colors (int pair, int *fg, int *bg);
extern const char *gui_color_get_name (int num_color);
extern void gui_color_free_vars (void);
extern void gui_color_init_weechat (void);
extern void gui_color_display_terminal_colors (void);
extern void gui_color_info_term_colors (char *buffer, int size);
extern void gui_color_info_rgb (char *buffer, int size);
extern void gui_color_buffer_display (void);
extern void gui_color_switch_colors (void);
extern void gui_color_reset_pairs (void);
extern void gui_color_buffer_assign (void);
extern void gui_color_buffer_open (void);
extern void gui_color_palette_build_aliases (void);
extern struct t_gui_color_palette *gui_color_palette_new (int number,
                                                          const char *value);
extern void gui_color_palette_free (struct t_gui_color_palette *color_palette);
extern void gui_color_dump (void);

#endif /* WEECHAT_GUI_COLOR_H */
