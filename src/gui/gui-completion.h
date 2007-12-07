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


#ifndef __WEECHAT_GUI_COMPLETION_H
#define __WEECHAT_GUI_COMPLETION_H 1

#define GUI_COMPLETION_NULL         0
#define GUI_COMPLETION_NICK         1
#define GUI_COMPLETION_COMMAND      2
#define GUI_COMPLETION_COMMAND_ARG  3
#define GUI_COMPLETION_AUTO         4

struct t_gui_completion
{
    /* completion context */
    struct t_gui_buffer *buffer;/* buffer where completion was asked         */
    int context;                /* context: null, nick, command, cmd arg     */
    char *base_command;         /* command with arg to complete (can be NULL)*/
    int base_command_arg;       /* # arg to complete (if context is cmd arg) */
    int arg_is_nick;            /* argument is nick                          */
    char *base_word;            /* word to complete (when Tab was pressed)   */
    int base_word_pos;          /* beggining of base word                    */
    int position;               /* position where Tab was pressed            */
    char *args;                 /* command line args (including base word)   */
    int direction;              /* +1 = search next word, -1 = previous word */
    int add_space;              /* add space after completion?               */
    
    /* for command argument completion */
    struct t_weelist *completion_list; /* data list for completion           */
    
    /* completion found */
    char *word_found;           /* word found (to replace base word)         */
    int position_replace;       /* position where word has to be replaced    */
    int diff_size;              /* size difference (< 0 = char(s) deleted)   */
    int diff_length;            /* length difference (<= diff_size)          */
};

/* completion functions */

extern void gui_completion_init (struct t_gui_completion *, struct t_gui_buffer *);
extern void gui_completion_free (struct t_gui_completion *);
extern void gui_completion_search (struct t_gui_completion *, int, char *, int, int);
extern void gui_completion_print_log (struct t_gui_completion *);

#endif /* gui-completion.h */
