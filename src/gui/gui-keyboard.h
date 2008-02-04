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


#ifndef __WEECHAT_GUI_KEYBOARD_H
#define __WEECHAT_GUI_KEYBOARD_H 1

#define GUI_KEYBOARD_BUFFER_BLOCK_SIZE 256

/* keyboard structures */

typedef void (t_gui_key_func)(char *args);

struct t_gui_key
{
    char *key;                      /* key combo (ex: a, ^W, ^W^C, meta-a)  */
    char *command;                  /* associated command (may be NULL)     */
    t_gui_key_func *function;       /* associated function (if cmd is NULL) */
    char *args;                     /* args for function (if cmd is NULL)   */
    struct t_gui_key *prev_key;     /* link to previous key                 */
    struct t_gui_key *next_key;     /* link to next key                     */
};

struct t_gui_key_function
{
    char *function_name;            /* name of function                     */
    t_gui_key_func *function;       /* associated function                  */
    char *description;              /* description of function              */
};

/* keyboard variables */

extern struct t_gui_key *gui_keys;
extern struct t_gui_key *last_gui_key;
extern struct t_gui_key_function gui_key_functions[];
extern char gui_key_combo_buffer[];
extern int gui_key_grab;
extern int gui_key_grab_count;
extern int *gui_keyboard_buffer;
extern int gui_keyboard_buffer_size;
extern int gui_keyboard_paste_pending;
extern time_t gui_keyboard_last_activity_time;

/* keyboard functions */

extern void gui_keyboard_init ();
extern void gui_keyboard_grab_init ();
extern void gui_keyboard_grab_end ();
extern char *gui_keyboard_get_internal_code (char *key);
extern char *gui_keyboard_get_expanded_name (char *key);
extern struct t_gui_key *gui_keyboard_search (char *key);
extern t_gui_key_func *gui_keyboard_function_search_by_name (char *name);
extern char *gui_keyboard_function_search_by_ptr (t_gui_key_func *function);
extern struct t_gui_key *gui_keyboard_bind (char *key, char *command);
extern int gui_keyboard_unbind (char *key);
extern int gui_keyboard_pressed (char *key_str);
extern void gui_keyboard_free (struct t_gui_key *key);
extern void gui_keyboard_free_all ();
extern void gui_keyboard_buffer_reset ();
extern void gui_keyboard_buffer_add (int key);
extern int gui_keyboard_get_paste_lines ();
extern void gui_keyboard_paste_accept ();
extern void gui_keyboard_paste_cancel ();

/* keyboard functions (GUI dependent) */

extern void gui_keyboard_default_bindings ();

#endif /* gui-keyboard.h */
