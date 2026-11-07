/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_CURSES_COLOR_H
#define WEECHAT_GUI_CURSES_COLOR_H

#define GUI_CURSES_NUM_WEECHAT_COLORS 17

/* Number of RGB colors displayed on each line of color buffer */
#define GUI_COLOR_BUFFER_RGB_COUNT 64

#ifndef A_ITALIC /* A_ITALIC is defined in ncurses >= 5.9 patch 20130831 */
#define A_ITALIC 0
#endif /* A_ITALIC */

#define A_ALL_ATTR A_BLINK | A_DIM | A_BOLD | A_UNDERLINE | A_REVERSE | A_ITALIC

extern struct t_gui_color *gui_weechat_colors;
extern int gui_color_term_colors;
extern int gui_color_term_direct_colors;
extern int gui_color_term_direct_indexed;
extern int gui_color_use_term_colors;
extern int gui_color_pairs_used;
extern struct t_hashtable *gui_color_pairs_direct;
extern int gui_color_num_pairs;
extern int gui_color_pairs_auto_reset;
extern int gui_color_pairs_auto_reset_pending;
extern time_t gui_color_pairs_auto_reset_last;
extern int gui_color_buffer_refresh_needed;
extern int gui_color_buffer_sample_rgb;

extern int gui_color_get_gui_attrs (int color);
extern int gui_color_get_term_direct_indexed (void);
extern void gui_color_init_pair (int number, int fg, int bg);
extern int gui_color_get_valid_term_color (int color);
extern int gui_color_alloc_pair (void);
extern int gui_color_get_direct_color (int color);
extern int gui_color_get_pair_direct (int fg, int bg);
extern void gui_color_pair_direct_key_get_colors (const void *key, int *fg,
                                                  int *bg);
extern void gui_color_init_pair_direct_map_cb (void *data,
                                               struct t_hashtable *hashtable,
                                               const void *key,
                                               const void *value);
extern void gui_color_direct_color_string (int color, char *buffer, int size);
extern void gui_color_dump_pair_direct_map_cb (void *data,
                                               struct t_hashtable *hashtable,
                                               const void *key,
                                               const void *value);
extern int gui_color_get_pair (int fg, int bg);
extern int gui_color_weechat_get_pair (int weechat_color);
extern char *gui_color_buffer_rgb_line (const int *colors, int count);
extern int gui_color_buffer_display_rgb (int y);
extern char *gui_color_buffer_palette_line (const char *label,
                                            int label_width, int rgb);
extern int gui_color_buffer_display_palette_rgb (int y);
extern int gui_color_buffer_display_sample (int y, int rgb);
extern void gui_color_alloc (void);

#endif /* WEECHAT_GUI_CURSES_COLOR_H */
