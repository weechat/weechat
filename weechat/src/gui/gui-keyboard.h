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


#ifndef __WEECHAT_GUI_KEY_H
#define __WEECHAT_GUI_KEY_H 1

#define KEY_SHOW_MODE_DISPLAY   1
#define KEY_SHOW_MODE_BIND      2

/* key structures */

typedef void (t_gui_key_func)(t_gui_window *, char *);

typedef struct t_gui_key t_gui_key;

struct t_gui_key
{
    char *key;                      /* key combo (ex: a, ^W, ^W^C, meta-a)  */
    char *command;                  /* associated command (may be NULL)     */
    t_gui_key_func *function;       /* associated function (if cmd is NULL) */
    char *args;                     /* args for function (if cmd is NULL)   */
    t_gui_key *prev_key;            /* link to previous key                 */
    t_gui_key *next_key;            /* link to next key                     */
};

typedef struct t_gui_key_function t_gui_key_function;

struct t_gui_key_function
{
    char *function_name;            /* name of function                     */
    t_gui_key_func *function;       /* associated function                  */
    char *description;              /* description of function              */
};

/* key variables */

extern t_gui_key *gui_keys;
extern t_gui_key *last_gui_key;
extern t_gui_key_function gui_key_functions[];
extern char gui_key_buffer[128];
extern int gui_key_grab;
extern int gui_key_grab_count;

#endif /* gui-key.h */
