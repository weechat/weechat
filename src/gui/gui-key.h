/*
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

#ifndef WEECHAT_GUI_KEY_H
#define WEECHAT_GUI_KEY_H

#include <time.h>

struct t_hashtable;

#define GUI_KEY_BUFFER_BLOCK_SIZE 256

#define GUI_KEY_BRACKETED_PASTE_START  "\x1B[200~"
#define GUI_KEY_BRACKETED_PASTE_END    "\x1B[201~"
#define GUI_KEY_BRACKETED_PASTE_LENGTH 6

enum t_gui_key_context
{
    GUI_KEY_CONTEXT_DEFAULT = 0,
    GUI_KEY_CONTEXT_SEARCH,
    GUI_KEY_CONTEXT_HISTSEARCH,
    GUI_KEY_CONTEXT_CURSOR,
    GUI_KEY_CONTEXT_MOUSE,
    /* number of key contexts */
    GUI_KEY_NUM_CONTEXTS,
};

enum t_gui_key_focus
{
    GUI_KEY_FOCUS_ANY = 0,
    GUI_KEY_FOCUS_CHAT,
    GUI_KEY_FOCUS_BAR,
    GUI_KEY_FOCUS_ITEM,
    /* number of key focus */
    GUI_KEY_NUM_FOCUS,
};

/* key structures */

struct t_gui_key
{
    char *key;                      /* key name (eg: ctrl-w, meta-left)     */
    char **chunks;                  /* key chunks (split on comma)          */
    int chunks_count;               /* number of chunks                     */
    int area_type[2];               /* type of areas (for cursor/mouse)     */
    char *area_name[2];             /* name of areas (for cursor/mouse)     */
    char *area_key;                 /* key after area (after ":")           */
    char *command;                  /* associated command                   */
    int score;                      /* score, for sorting keys              */
    struct t_gui_key *prev_key;     /* link to previous key                 */
    struct t_gui_key *next_key;     /* link to next key                     */
};

/* key variables */

extern struct t_gui_key *gui_keys[GUI_KEY_NUM_CONTEXTS];
extern struct t_gui_key *last_gui_key[GUI_KEY_NUM_CONTEXTS];
extern struct t_gui_key *gui_default_keys[GUI_KEY_NUM_CONTEXTS];
extern struct t_gui_key *last_gui_default_key[GUI_KEY_NUM_CONTEXTS];
extern int gui_keys_count[GUI_KEY_NUM_CONTEXTS];
extern int gui_default_keys_count[GUI_KEY_NUM_CONTEXTS];
extern char *gui_key_context_string[GUI_KEY_NUM_CONTEXTS];
extern int gui_key_debug;
extern int gui_key_verbose;
extern char gui_key_combo[];
extern int gui_key_grab;
extern int gui_key_grab_count;
extern int *gui_key_buffer;
extern int gui_key_buffer_size;
extern int gui_key_last_key_pressed_sent;
extern int gui_key_paste_pending;
extern int gui_key_paste_bracketed;
extern time_t gui_key_last_activity_time;

/* key functions */

extern void gui_key_init ();
extern int gui_key_search_context (const char *context);
extern void gui_key_grab_init (int grab_command, const char *delay);
extern int gui_key_expand (const char *key,
                           char **key_name, char **key_name_alias);
extern char *gui_key_legacy_to_alias (const char *key);
extern struct t_gui_key *gui_key_new (struct t_gui_buffer *buffer,
                                      int context,
                                      const char *key,
                                      const char *command,
                                      int create_option);
extern struct t_gui_key *gui_key_search (struct t_gui_key *keys,
                                         const char *key);
extern struct t_gui_key *gui_key_bind (struct t_gui_buffer *buffer,
                                       int context,
                                       const char *key,
                                       const char *command,
                                       int check_key);
extern int gui_key_bind_plugin (const char *context, struct t_hashtable *keys);
extern int gui_key_unbind (struct t_gui_buffer *buffer, int context,
                           const char *key);
extern int gui_key_unbind_plugin (const char *context, const char *key);
extern int gui_key_focus (const char *key, int context);
extern void gui_key_debug_print_key (const char *combo, const char *key_name,
                                     const char *key_name_alias,
                                     const char *command, int mouse);
extern int gui_key_pressed (const char *key_str);
extern void gui_key_free (int context,
                          struct t_gui_key **keys,
                          struct t_gui_key **last_key,
                          int *keys_count,
                          struct t_gui_key *key,
                          int delete_option);
extern void gui_key_free_all (int context,
                              struct t_gui_key **keys,
                              struct t_gui_key **last_key,
                              int *keys_count,
                              int delete_option);
extern void gui_key_buffer_reset ();
extern void gui_key_buffer_add (unsigned char key);
extern int gui_key_buffer_search (int start_index, int max_index,
                                  const char *string);
extern void gui_key_buffer_remove (int index, int number);
extern void gui_key_paste_remove_newline ();
extern void gui_key_paste_replace_tabs ();
extern void gui_key_paste_start ();
extern void gui_key_paste_finish ();
extern int gui_key_get_paste_lines ();
extern int gui_key_paste_check (int bracketed_paste);
extern void gui_key_paste_bracketed_timer_remove ();
extern void gui_key_paste_bracketed_timer_add ();
extern void gui_key_paste_bracketed_start ();
extern void gui_key_paste_bracketed_stop ();
extern void gui_key_paste_accept ();
extern void gui_key_paste_cancel ();
extern void gui_key_end ();
extern struct t_hdata *gui_key_hdata_key_cb (const void *pointer,
                                             void *data,
                                             const char *hdata_name);
extern int gui_key_add_to_infolist (struct t_infolist *infolist,
                                    struct t_gui_key *key);
extern void gui_key_print_log (struct t_gui_buffer *buffer);

/* key functions (GUI dependent) */

extern void gui_key_default_bindings (int context, int create_option);

#endif /* WEECHAT_GUI_KEY_H */
