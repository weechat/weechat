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

#ifndef __WEECHAT_GUI_BUFFER_H
#define __WEECHAT_GUI_BUFFER_H 1

#include <regex.h>

struct t_hashtable;
struct t_gui_window;
struct t_infolist;

enum t_gui_buffer_type
{
    GUI_BUFFER_TYPE_FORMATTED = 0,
    GUI_BUFFER_TYPE_FREE,
    /* number of buffer types */
    GUI_BUFFER_NUM_TYPES,
};

enum t_gui_buffer_notify
{
    GUI_BUFFER_NOTIFY_NONE = 0,
    GUI_BUFFER_NOTIFY_HIGHLIGHT,
    GUI_BUFFER_NOTIFY_MESSAGE,
    GUI_BUFFER_NOTIFY_ALL,
    /* number of buffer notify */
    GUI_BUFFER_NUM_NOTIFY,
};

#define GUI_BUFFER_MAIN "weechat"

#define GUI_TEXT_SEARCH_DISABLED 0
#define GUI_TEXT_SEARCH_BACKWARD 1
#define GUI_TEXT_SEARCH_FORWARD  2

#define GUI_BUFFER_INPUT_BLOCK_SIZE 256

/* buffer structures */

struct t_gui_input_undo
{
    char *data;                        /* content of input buffer           */
    int pos;                           /* position of cursor in buffer      */
    struct t_gui_input_undo *prev_undo;/* link to previous undo             */
    struct t_gui_input_undo *next_undo;/* link to next undo                 */
};

struct t_gui_buffer
{
    struct t_weechat_plugin *plugin;   /* plugin which created this buffer  */
                                       /* (NULL for a WeeChat buffer)       */
    /*
     * when upgrading, plugins are not loaded, so we use next variable
     * to store plugin name, then restore plugin pointer when plugin is
     * loaded
     */
    char *plugin_name_for_upgrade;     /* plugin name when upgrading        */

    int number;                        /* buffer number (first is 1)        */
    int layout_number;                 /* number of buffer saved in layout  */
    int layout_number_merge_order;     /* order in merge for layout         */
    char *name;                        /* buffer name                       */
    char *full_name;                   /* plugin name + '.' + buffer name   */
    char *short_name;                  /* short buffer name                 */
    enum t_gui_buffer_type type;       /* buffer type (formatted, free, ..) */
    int notify;                        /* 0 = never                         */
                                       /* 1 = highlight only                */
                                       /* 2 = highlight + msg               */
                                       /* 3 = highlight + msg + join/part   */
    int num_displayed;                 /* number of windows displaying buf. */
    int active;                        /* 0 = buffer merged and not active  */
                                       /* 1 = active (merged or not)        */
                                       /* 2 = the only active (merged)      */
    int print_hooks_enabled;           /* 1 if print hooks are enabled      */

    /* close callback */
    int (*close_callback)(void *data,  /* called when buffer is closed      */
                          struct t_gui_buffer *buffer);
    void *close_callback_data;         /* data for callback                 */

    /* buffer title */
    char *title;                       /* buffer title                      */

    /* chat content */
    struct t_gui_lines *own_lines;     /* lines (for this buffer only)      */
    struct t_gui_lines *mixed_lines;   /* mixed lines (if buffers merged)   */
    struct t_gui_lines *lines;         /* pointer to "own_lines" or         */
                                       /* "mixed_lines"                     */
    int time_for_each_line;            /* time is displayed for each line?  */
    int chat_refresh_needed;           /* refresh for chat is needed ?      */
                                       /* (1=refresh, 2=erase+refresh)      */

    /* nicklist */
    int nicklist;                      /* = 1 if nicklist is enabled        */
    int nicklist_case_sensitive;       /* nicks are case sensitive ?        */
    struct t_gui_nick_group *nicklist_root; /* pointer to groups root       */
    int nicklist_max_length;           /* max length for a nick             */
    int nicklist_display_groups;       /* display groups ?                  */
    int nicklist_visible_count;        /* number of nicks/groups to display */
    int (*nickcmp_callback)(void *data, /* called to compare nicks (search  */
                            struct t_gui_buffer *buffer,  /* in nicklist)   */
                            const char *nick1,
                            const char *nick2);
    void *nickcmp_callback_data;       /* data for callback                 */

    /* input */
    int input;                         /* = 1 if input is enabled           */
    int (*input_callback)(void *data,  /* called when user send data        */
                          struct t_gui_buffer *buffer,
                          const char *input_data);
    void *input_callback_data;         /* data for callback                 */
                                       /* to this buffer                    */
    int input_get_unknown_commands;    /* 1 if unknown commands are sent to */
                                       /* input_callback                    */
    char *input_buffer;                /* input buffer                      */
    int input_buffer_alloc;            /* input buffer: allocated size      */
    int input_buffer_size;             /* buffer size in bytes              */
    int input_buffer_length;           /* number of chars in buffer         */
    int input_buffer_pos;              /* position into buffer              */
    int input_buffer_1st_display;      /* first char displayed on screen    */

    /* undo/redo for input */
    struct t_gui_input_undo *input_undo_snap; /* snapshot of input buffer   */
    struct t_gui_input_undo *input_undo;      /* undo for input             */
    struct t_gui_input_undo *last_input_undo; /* last undo for input        */
    struct t_gui_input_undo *ptr_input_undo;  /* pointer to current undo    */
    int input_undo_count;              /* number of undos                   */

    /* completion */
    struct t_gui_completion *completion; /* completion                      */

    /* history */
    struct t_gui_history *history;     /* commands history                  */
    struct t_gui_history *last_history;/* last command in history           */
    struct t_gui_history *ptr_history; /* current command in history        */
    int num_history;                   /* number of commands in history     */

    /* text search */
    int text_search;                   /* text search type                  */
    int text_search_exact;             /* exact search (case sensitive) ?   */
    int text_search_found;             /* 1 if text found, otherwise 0      */
    char *text_search_input;           /* input saved before text search    */

    /* highlight settings for buffer */
    char *highlight_words;             /* list of words to highlight        */
    char *highlight_regex;             /* regex for highlight               */
    regex_t *highlight_regex_compiled; /* compiled regex                    */
    char *highlight_tags;              /* tags to highlight                 */
    int highlight_tags_count;          /* number of tags to highlight       */
                                       /* (if 0, any tag is highlighted)    */
    char **highlight_tags_array;       /* tags to highlight                 */

    /* hotlist settings for buffer */
    struct t_hashtable *hotlist_max_level_nicks; /* max hotlist level for   */
                                                 /* some nicks              */

    /* keys associated to buffer */
    struct t_gui_key *keys;            /* keys specific to buffer           */
    struct t_gui_key *last_key;        /* last key for buffer               */
    int keys_count;                    /* number of keys in buffer          */

    /* local variables */
    struct t_hashtable *local_variables; /* local variables                 */

    /* link to previous/next buffer */
    struct t_gui_buffer *prev_buffer;  /* link to previous buffer           */
    struct t_gui_buffer *next_buffer;  /* link to next buffer               */
};

struct t_gui_buffer_visited
{
    struct t_gui_buffer *buffer;              /* pointer to buffer          */
    struct t_gui_buffer_visited *prev_buffer; /* link to previous variable  */
    struct t_gui_buffer_visited *next_buffer; /* link to next variable      */
};

/* buffer variables */

extern struct t_gui_buffer *gui_buffers;
extern struct t_gui_buffer *last_gui_buffer;
extern struct t_gui_buffer_visited *gui_buffers_visited;
extern struct t_gui_buffer_visited *last_gui_buffer_visited;
extern int gui_buffers_visited_index;
extern int gui_buffers_visited_count;
extern int gui_buffers_visited_frozen;
extern struct t_gui_buffer *gui_buffer_last_displayed;
extern char *gui_buffer_notify_string[];
extern char *gui_buffer_properties_get_integer[];
extern char *gui_buffer_properties_get_string[];
extern char *gui_buffer_properties_get_pointer[];
extern char *gui_buffer_properties_set[];

/* buffer functions */

extern const char *gui_buffer_get_plugin_name (struct t_gui_buffer *buffer);
extern const char *gui_buffer_get_short_name (struct t_gui_buffer *buffer);
extern void gui_buffer_build_full_name (struct t_gui_buffer *buffer);
extern void gui_buffer_notify_set_all ();
extern void gui_buffer_input_buffer_init (struct t_gui_buffer *buffer);
extern struct t_gui_buffer *gui_buffer_new (struct t_weechat_plugin *plugin,
                                            const char *name,
                                            int (*input_callback)(void *data,
                                                                  struct t_gui_buffer *buffer,
                                                                  const char *input_data),
                                            void *input_callback_data,
                                            int (*close_callback)(void *data,
                                                                  struct t_gui_buffer *buffer),
                                            void *close_callback_data);
extern int gui_buffer_valid (struct t_gui_buffer *buffer);
extern char *gui_buffer_string_replace_local_var (struct t_gui_buffer *buffer,
                                                  const char *string);
extern int gui_buffer_match_list_split (struct t_gui_buffer *buffer,
                                        int num_buffers, char **buffers);
extern int gui_buffer_match_list (struct t_gui_buffer *buffer,
                                  const char *string);
extern void gui_buffer_set_plugin_for_upgrade (char *name,
                                               struct t_weechat_plugin *plugin);
extern int gui_buffer_property_in_list (char *properties[], char *property);
extern int gui_buffer_get_integer (struct t_gui_buffer *buffer,
                                   const char *property);
extern const char *gui_buffer_get_string (struct t_gui_buffer *buffer,
                                          const char *property);
extern void *gui_buffer_get_pointer (struct t_gui_buffer *buffer,
                                     const char *property);
extern void gui_buffer_ask_chat_refresh (struct t_gui_buffer *buffer,
                                         int refresh);
extern void gui_buffer_set_title (struct t_gui_buffer *buffer,
                                  const char *new_title);
extern void gui_buffer_set_highlight_words (struct t_gui_buffer *buffer,
                                            const char *new_highlight_words);
extern void gui_buffer_set_highlight_regex (struct t_gui_buffer *buffer,
                                            const char *new_highlight_regex);
extern void gui_buffer_set_highlight_tags (struct t_gui_buffer *buffer,
                                           const char *new_highlight_tags);
extern void gui_buffer_set_hotlist_max_level_nicks (struct t_gui_buffer *buffer,
                                                    const char *new_hotlist_max_level_nicks);
extern void gui_buffer_set_unread (struct t_gui_buffer *buffer);
extern void gui_buffer_set (struct t_gui_buffer *buffer, const char *property,
                            const char *value);
extern void gui_buffer_set_pointer (struct t_gui_buffer *buffer,
                                    const char *property, void *pointer);
extern void gui_buffer_add_value_num_displayed (struct t_gui_buffer *buffer,
                                                int value);
extern int gui_buffer_is_main (const char *plugin_name, const char *name);
extern struct t_gui_buffer *gui_buffer_search_main ();
extern struct t_gui_buffer *gui_buffer_search_by_name (const char *plugin,
                                                       const char *name);
extern struct t_gui_buffer *gui_buffer_search_by_full_name (const char *full_name);
extern struct t_gui_buffer *gui_buffer_search_by_partial_name (const char *plugin,
                                                               const char *name);
extern struct t_gui_buffer *gui_buffer_search_by_number (int number);
extern struct t_gui_buffer *gui_buffer_search_by_layout_number (int layout_number,
                                                                int layout_number_merge_order);
extern int gui_buffer_count_merged_buffers (int number);
extern int gui_buffer_is_scrolled (struct t_gui_buffer *buffer);
extern void gui_buffer_clear (struct t_gui_buffer *buffer);
extern void gui_buffer_clear_all ();
extern void gui_buffer_close (struct t_gui_buffer *buffer);
extern void gui_buffer_switch_by_number (struct t_gui_window *window,
                                         int number);
extern void gui_buffer_set_active_buffer (struct t_gui_buffer *buffer);
extern struct t_gui_buffer *gui_buffer_get_next_active_buffer (struct t_gui_buffer *buffer);
extern struct t_gui_buffer *gui_buffer_get_previous_active_buffer (struct t_gui_buffer *buffer);
extern void gui_buffer_move_to_number (struct t_gui_buffer *buffer, int number);
extern void gui_buffer_swap (struct t_gui_buffer *buffer1,
                             struct t_gui_buffer *buffer2);
extern void gui_buffer_merge (struct t_gui_buffer *buffer,
                              struct t_gui_buffer *target_buffer);
extern void gui_buffer_unmerge (struct t_gui_buffer *buffer, int number);
extern void gui_buffer_unmerge_all ();
extern void gui_buffer_sort_by_layout_number ();
extern void gui_buffer_undo_snap (struct t_gui_buffer *buffer);
extern void gui_buffer_undo_snap_free (struct t_gui_buffer *buffer);
extern void gui_buffer_undo_add (struct t_gui_buffer *buffer);
extern void gui_buffer_undo_free (struct t_gui_buffer *buffer,
                                  struct t_gui_input_undo *undo);
extern void gui_buffer_undo_free_all (struct t_gui_buffer *buffer);
extern struct t_gui_buffer_visited *gui_buffer_visited_search_by_number (int number);
extern void gui_buffer_visited_remove (struct t_gui_buffer_visited *buffer_visited);
extern void gui_buffer_visited_remove_by_buffer (struct t_gui_buffer *buffer);
extern struct t_gui_buffer_visited *gui_buffer_visited_add (struct t_gui_buffer *buffer);
extern int gui_buffer_visited_get_index_previous ();
extern int gui_buffer_visited_get_index_next ();
extern struct t_hdata *gui_buffer_hdata_buffer_cb (void *data,
                                                   const char *hdata_name);
extern struct t_hdata *gui_buffer_hdata_input_undo_cb (void *data,
                                                       const char *hdata_name);
extern int gui_buffer_add_to_infolist (struct t_infolist *infolist,
                                       struct t_gui_buffer *buffer);
extern void gui_buffer_dump_hexa (struct t_gui_buffer *buffer);
extern void gui_buffer_print_log ();

#endif /* __WEECHAT_GUI_BUFFER_H */
