/*
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_GUI_COMPLETION_H
#define __WEECHAT_GUI_COMPLETION_H 1

#define GUI_COMPLETION_NULL         0
#define GUI_COMPLETION_COMMAND      1
#define GUI_COMPLETION_COMMAND_ARG  2
#define GUI_COMPLETION_AUTO         3

struct t_gui_completion_partial
{
    char *word;                   /* (partial) word matching completion      */
    int count;                    /* number of matching items with this word */
    struct t_gui_completion_partial *prev_item;
    struct t_gui_completion_partial *next_item;
};

struct t_gui_completion
{
    /* completion context */
    struct t_gui_buffer *buffer;  /* buffer where completion was asked       */
    int context;                  /* context: null, nick, command, cmd arg   */
    char *base_command;           /* cmd with arg to complete (can be NULL)  */
    int base_command_arg_index;   /* # arg to complete (if context=cmd arg)  */
    char *base_word;              /* word to complete (when Tab was pressed) */
    int base_word_pos;            /* beginning of base word                  */
    int position;                 /* position where Tab was pressed          */
    char *args;                   /* command line args (including base word) */
    int direction;                /* +1=search next word, -1=previous word   */
    int add_space;                /* add space after completion?             */
    int force_partial_completion; /* force partial completion?               */

    /* for command argument completion */
    struct t_weelist *completion_list; /* data list for completion           */

    /* completion found */
    char *word_found;             /* word found (to replace base word)       */
    int word_found_is_nick;       /* word found is nick?                     */
    int position_replace;         /* position where word has to be replaced  */
    int diff_size;                /* size difference (< 0 = char(s) deleted) */
    int diff_length;              /* length difference (<= diff_size)        */

    /* partial completion */
    struct t_gui_completion_partial *partial_completion_list;
    struct t_gui_completion_partial *last_partial_completion;
};

/* completion functions */

extern void gui_completion_buffer_init (struct t_gui_completion *completion,
                                        struct t_gui_buffer *buffer);
extern void gui_completion_free (struct t_gui_completion *completion);
extern void gui_completion_stop (struct t_gui_completion *completion);
extern void gui_completion_list_add (struct t_gui_completion *completion,
                                     const char *word,
                                     int nick_completion, const char *where);
extern void gui_completion_search (struct t_gui_completion *completion,
                                   int direction, const char *data, int size,
                                   int pos);
extern const char *gui_completion_get_string (struct t_gui_completion *completion,
                                              const char *property);
extern struct t_hdata *gui_completion_hdata_completion_cb (void *data,
                                                           const char *hdata_name);
extern struct t_hdata *gui_completion_hdata_completion_partial_cb (void *data,
                                                                   const char *hdata_name);
extern void gui_completion_print_log (struct t_gui_completion *completion);

#endif /* __WEECHAT_GUI_COMPLETION_H */
