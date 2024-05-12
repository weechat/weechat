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

#ifndef WEECHAT_GUI_BUFFER_H
#define WEECHAT_GUI_BUFFER_H

#include <limits.h>
#include <regex.h>

struct t_config_option;
struct t_gui_window;
struct t_hashtable;
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

enum t_gui_buffer_search
{
    GUI_BUFFER_SEARCH_DISABLED = 0,
    GUI_BUFFER_SEARCH_LINES,    /* search in buffer lines */
    GUI_BUFFER_SEARCH_HISTORY,  /* search in buffer command line history */
    /* number of search modes */
    GUI_BUFFER_NUM_SEARCH,
};

enum t_gui_buffer_search_dir
{
    GUI_BUFFER_SEARCH_DIR_BACKWARD = 0,
    GUI_BUFFER_SEARCH_DIR_FORWARD,
    /* number of search directions */
    GUI_BUFFER_NUM_SEARCH_DIR,
};

enum t_gui_buffer_search_history
{
    GUI_BUFFER_SEARCH_HISTORY_NONE = 0,
    GUI_BUFFER_SEARCH_HISTORY_LOCAL,
    GUI_BUFFER_SEARCH_HISTORY_GLOBAL,
    /* number of search history */
    GUI_BUFFER_NUM_SEARCH_HISTORY,
};

#define GUI_BUFFER_TYPE_DEFAULT GUI_BUFFER_TYPE_FORMATTED

#define GUI_BUFFER_MAIN "weechat"

#define GUI_BUFFERS_MAX 10000

#define GUI_BUFFER_NUMBER_MAX (INT_MAX - 10000)

/* search where in buffer lines? */
#define GUI_BUFFER_SEARCH_IN_MESSAGE        (1 << 0)
#define GUI_BUFFER_SEARCH_IN_PREFIX         (1 << 1)

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
    long long id;                      /* unique id for buffer              */
                                       /* (timestamp with microseconds)     */
    int opening;                       /* 1 if buffer is being opened       */
    struct t_weechat_plugin *plugin;   /* plugin which created this buffer  */
                                       /* (NULL for a WeeChat buffer)       */
    /*
     * when upgrading, plugins are not loaded, so we use next variable
     * to store plugin name, then restore plugin pointer when plugin is
     * loaded
     */
    char *plugin_name_for_upgrade;     /* plugin name when upgrading        */

    int number;                        /* buffer number (first is 1)        */
    int layout_number;                 /* number of buffer stored in layout */
    int layout_number_merge_order;     /* order in merge for layout         */
    char *name;                        /* buffer name                       */
    char *full_name;                   /* plugin name + '.' + buffer name   */
    char *old_full_name;               /* old full name (set before rename) */
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
    int hidden;                        /* 1 = buffer hidden                 */
    int zoomed;                        /* 1 if a merged buffer is zoomed    */
                                       /* (it can be another buffer)        */
    int print_hooks_enabled;           /* 1 if print hooks are enabled      */
    int day_change;                    /* 1 if "day change" displayed       */
    int clear;                         /* 1 if clear of buffer is allowed   */
                                       /* with command /buffer clear        */
    int filter;                        /* 1 if filters enabled for buffer   */

    /* close callback */
    int (*close_callback)(const void *pointer, /* called when buffer is     */
                          void *data,          /* closed                    */
                          struct t_gui_buffer *buffer);
    const void *close_callback_pointer; /* pointer for callback             */
    void *close_callback_data;         /* data for callback                 */
    int closing;                       /* 1 if the buffer is being closed   */

    /* buffer title */
    char *title;                       /* buffer title                      */

    /* buffer modes */
    char *modes;                       /* buffer modes                      */

    /* chat content */
    struct t_gui_lines *own_lines;     /* lines (for this buffer only)      */
    struct t_gui_lines *mixed_lines;   /* mixed lines (if buffers merged)   */
    struct t_gui_lines *lines;         /* pointer to "own_lines" or         */
                                       /* "mixed_lines"                     */
    int next_line_id;                  /* next line id                      */
                                       /* (used with formatted type only)   */
    int time_for_each_line;            /* time is displayed for each line?  */
    int chat_refresh_needed;           /* refresh for chat is needed ?      */
                                       /* (1=refresh, 2=erase+refresh)      */

    /* nicklist */
    int nicklist;                      /* = 1 if nicklist is enabled        */
    int nicklist_case_sensitive;       /* nicks are case sensitive ?        */
    struct t_gui_nick_group *nicklist_root; /* pointer to groups root       */
    int nicklist_max_length;           /* max length for a nick             */
    int nicklist_display_groups;       /* display groups ?                  */
    int nicklist_count;                /* number of nicks/groups            */
    int nicklist_visible_count;        /* number of nicks/groups displayed  */
    int nicklist_groups_count;         /* number of groups                  */
    int nicklist_groups_visible_count; /* number of groups displayed        */
    int nicklist_nicks_count;          /* number of nicks                   */
    int nicklist_nicks_visible_count;  /* number of nicks displayed         */
    long long nicklist_last_id_assigned; /* last id assigned for a grp/nick */
    int (*nickcmp_callback)(const void *pointer, /* called to compare nicks */
                            void *data,          /* (search in nicklist)    */
                            struct t_gui_buffer *buffer,
                            const char *nick1,
                            const char *nick2);
    const void *nickcmp_callback_pointer; /* pointer for callback           */
    void *nickcmp_callback_data;       /* data for callback                 */

    /* input */
    int input;                         /* = 1 if input is enabled           */
    int (*input_callback)(const void *pointer, /* called when user sends    */
                          void *data,          /* data                      */
                          struct t_gui_buffer *buffer,
                          const char *input_data);
    const void *input_callback_pointer; /* pointer for callback             */
    void *input_callback_data;         /* data for callback                 */
                                       /* to this buffer                    */
    int input_get_any_user_data;       /* get any user data, including cmds */
    int input_get_unknown_commands;    /* 1 if unknown commands are sent to */
                                       /* input_callback                    */
    int input_get_empty;               /* 1 if empty input is sent to       */
                                       /* input_callback                    */
    int input_multiline;               /* 1 if multiple lines are sent as   */
                                       /* one message to input_callback     */
    char *input_prompt;                /* input prompt                      */
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

    /* text search (in buffer lines or command line history) */
    enum t_gui_buffer_search text_search; /* text search type               */
    enum t_gui_buffer_search_dir text_search_direction;
                                          /* search dir.: backward/forward  */
    int text_search_exact;                /* case sensitive search?         */
    int text_search_regex;                /* search with a regex            */
    regex_t *text_search_regex_compiled;  /* regex used to search           */
    int text_search_where;                /* prefix and/or msg              */
    enum t_gui_buffer_search_history text_search_history; /* local/global   */
    int text_search_found;                /* 1 if text found, otherwise 0   */
    struct t_gui_history *text_search_ptr_history; /* ptr to history found  */
    char *text_search_input;              /* input saved before text search */

    /* highlight settings for buffer */
    char *highlight_words;             /* list of words to highlight        */
    char *highlight_regex;             /* regex for highlight               */
    regex_t *highlight_regex_compiled; /* compiled regex                    */
    char *highlight_disable_regex;     /* regex for disabling highlight     */
    regex_t *highlight_disable_regex_compiled; /* compiled regex            */
    char *highlight_tags_restrict;     /* restrict highlight to these tags  */
    int highlight_tags_restrict_count; /* number of restricted tags         */
    char ***highlight_tags_restrict_array; /* array with restricted tags    */
    char *highlight_tags;              /* force highlight on these tags     */
    int highlight_tags_count;          /* number of highlight tags          */
    char ***highlight_tags_array;      /* array with highlight tags         */

    /* hotlist */
    struct t_gui_hotlist *hotlist;     /* hotlist entry for buffer          */
    struct t_gui_hotlist *hotlist_removed; /* hotlist that can be restored  */
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
extern int gui_buffers_count;
extern struct t_gui_buffer_visited *gui_buffers_visited;
extern struct t_gui_buffer_visited *last_gui_buffer_visited;
extern int gui_buffers_visited_index;
extern int gui_buffers_visited_count;
extern int gui_buffers_visited_frozen;
extern struct t_gui_buffer *gui_buffer_last_displayed;
extern long long gui_buffer_last_id_assigned;
extern char *gui_buffer_reserved_names[];
extern char *gui_buffer_type_string[];
extern char *gui_buffer_notify_string[];
extern char *gui_buffer_properties_get_integer[];
extern char *gui_buffer_properties_get_string[];
extern char *gui_buffer_properties_get_pointer[];
extern char *gui_buffer_properties_set[];

/* buffer functions */

extern int gui_buffer_search_type (const char *type);
extern int gui_buffer_search_notify (const char *notify);
extern int gui_buffer_send_signal (struct t_gui_buffer *buffer,
                                   const char *signal,
                                   const char *type_data, void *signal_data);
extern const char *gui_buffer_get_plugin_name (struct t_gui_buffer *buffer);
extern const char *gui_buffer_get_short_name (struct t_gui_buffer *buffer);
extern void gui_buffer_build_full_name (struct t_gui_buffer *buffer);
extern void gui_buffer_local_var_add (struct t_gui_buffer *buffer,
                                      const char *name,
                                      const char *value);
extern void gui_buffer_local_var_remove (struct t_gui_buffer *buffer,
                                         const char *name);
extern void gui_buffer_notify_set_all ();
extern long long gui_buffer_generate_id ();
extern int gui_buffer_is_reserved_name (const char *name);
extern void gui_buffer_apply_config_option_property (struct t_gui_buffer *buffer,
                                                     struct t_config_option *option);
extern struct t_gui_buffer *gui_buffer_new_props_with_id (long long id,
                                                          struct t_weechat_plugin *plugin,
                                                          const char *name,
                                                          struct t_hashtable *properties,
                                                          int (*input_callback)(const void *pointer,
                                                                                void *data,
                                                                                struct t_gui_buffer *buffer,
                                                                                const char *input_data),
                                                          const void *input_callback_pointer,
                                                          void *input_callback_data,
                                                          int (*close_callback)(const void *pointer,
                                                                                void *data,
                                                                                struct t_gui_buffer *buffer),
                                                          const void *close_callback_pointer,
                                                          void *close_callback_data);
extern struct t_gui_buffer *gui_buffer_new_props (struct t_weechat_plugin *plugin,
                                                  const char *name,
                                                  struct t_hashtable *properties,
                                                  int (*input_callback)(const void *pointer,
                                                                        void *data,
                                                                        struct t_gui_buffer *buffer,
                                                                        const char *input_data),
                                                  const void *input_callback_pointer,
                                                  void *input_callback_data,
                                                  int (*close_callback)(const void *pointer,
                                                                        void *data,
                                                                        struct t_gui_buffer *buffer),
                                                  const void *close_callback_pointer,
                                                  void *close_callback_data);
extern struct t_gui_buffer *gui_buffer_new (struct t_weechat_plugin *plugin,
                                            const char *name,
                                            int (*input_callback)(const void *pointer,
                                                                  void *data,
                                                                  struct t_gui_buffer *buffer,
                                                                  const char *input_data),
                                            const void *input_callback_pointer,
                                            void *input_callback_data,
                                            int (*close_callback)(const void *pointer,
                                                                  void *data,
                                                                  struct t_gui_buffer *buffer),
                                            const void *close_callback_pointer,
                                            void *close_callback_data);
extern struct t_gui_buffer *gui_buffer_new_user (const char *name,
                                                 enum t_gui_buffer_type buffer_type);
extern void gui_buffer_user_set_callbacks ();
extern int gui_buffer_valid (struct t_gui_buffer *buffer);
extern char *gui_buffer_string_replace_local_var (struct t_gui_buffer *buffer,
                                                  const char *string);
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
extern void gui_buffer_set_modes (struct t_gui_buffer *buffer,
                                  const char *new_modes);
extern void gui_buffer_set_highlight_words (struct t_gui_buffer *buffer,
                                            const char *new_highlight_words);
extern void gui_buffer_set_highlight_disable_regex (struct t_gui_buffer *buffer,
                                                    const char *new_regex);
extern void gui_buffer_set_highlight_regex (struct t_gui_buffer *buffer,
                                            const char *new_regex);
extern void gui_buffer_set_highlight_tags_restrict (struct t_gui_buffer *buffer,
                                                    const char *new_tags);
extern void gui_buffer_set_highlight_tags (struct t_gui_buffer *buffer,
                                           const char *new_tags);
extern void gui_buffer_set_hotlist_max_level_nicks (struct t_gui_buffer *buffer,
                                                    const char *new_hotlist_max_level_nicks);
extern void gui_buffer_set_input_prompt (struct t_gui_buffer *buffer,
                                         const char *input_prompt);
extern void gui_buffer_set (struct t_gui_buffer *buffer, const char *property,
                            const char *value);
extern void gui_buffer_set_pointer (struct t_gui_buffer *buffer,
                                    const char *property, void *pointer);
extern void gui_buffer_compute_num_displayed ();
extern void gui_buffer_add_value_num_displayed (struct t_gui_buffer *buffer,
                                                int value);
extern int gui_buffer_is_main (const char *plugin_name, const char *name);
extern struct t_gui_buffer *gui_buffer_search_main ();
extern struct t_gui_buffer *gui_buffer_search_by_id (long long id);
extern struct t_gui_buffer *gui_buffer_search_by_full_name (const char *full_name);
extern struct t_gui_buffer *gui_buffer_search (const char *plugin, const char *name);
extern struct t_gui_buffer *gui_buffer_search_by_partial_name (const char *plugin,
                                                               const char *name);
extern struct t_gui_buffer *gui_buffer_search_by_number (int number);
extern struct t_gui_buffer *gui_buffer_search_by_number_or_name (const char *string);
extern int gui_buffer_count_merged_buffers (int number);
extern void gui_buffer_clear (struct t_gui_buffer *buffer);
extern void gui_buffer_clear_all ();
extern void gui_buffer_close (struct t_gui_buffer *buffer);
extern void gui_buffer_switch_by_number (struct t_gui_window *window,
                                         int number);
extern void gui_buffer_set_active_buffer (struct t_gui_buffer *buffer);
extern void gui_buffer_switch_active_buffer (struct t_gui_buffer *buffer);
extern void gui_buffer_switch_active_buffer_previous (struct t_gui_buffer *buffer);
extern void gui_buffer_zoom (struct t_gui_buffer *buffer);
extern void gui_buffer_renumber (int number1, int number2, int start_number);
extern void gui_buffer_move_to_number (struct t_gui_buffer *buffer, int number);
extern void gui_buffer_swap (int number1, int number2);
extern void gui_buffer_merge (struct t_gui_buffer *buffer,
                              struct t_gui_buffer *target_buffer);
extern void gui_buffer_unmerge (struct t_gui_buffer *buffer, int number);
extern void gui_buffer_unmerge_all ();
extern void gui_buffer_hide (struct t_gui_buffer *buffer);
extern void gui_buffer_hide_all ();
extern void gui_buffer_unhide (struct t_gui_buffer *buffer);
extern void gui_buffer_unhide_all ();
extern void gui_buffer_sort_by_layout_number ();
extern void gui_buffer_undo_snap (struct t_gui_buffer *buffer);
extern void gui_buffer_undo_snap_free (struct t_gui_buffer *buffer);
extern void gui_buffer_undo_add (struct t_gui_buffer *buffer);
extern void gui_buffer_undo_free (struct t_gui_buffer *buffer,
                                  struct t_gui_input_undo *undo);
extern void gui_buffer_undo_free_all (struct t_gui_buffer *buffer);
extern void gui_buffer_input_move_to_buffer (struct t_gui_buffer *from_buffer,
                                             struct t_gui_buffer *to_buffer);
extern struct t_gui_buffer_visited *gui_buffer_visited_add (struct t_gui_buffer *buffer);
extern void gui_buffer_jump_smart (struct t_gui_window *window);
extern void gui_buffer_jump_last_visible_number (struct t_gui_window *window);
extern void gui_buffer_jump_last_buffer_displayed (struct t_gui_window *window);
extern void gui_buffer_jump_previously_visited_buffer (struct t_gui_window *window);
extern void gui_buffer_jump_next_visited_buffer (struct t_gui_window *window);
extern struct t_hdata *gui_buffer_hdata_buffer_cb (const void *pointer,
                                                   void *data,
                                                   const char *hdata_name);
extern struct t_hdata *gui_buffer_hdata_input_undo_cb (const void *pointer,
                                                       void *data,
                                                       const char *hdata_name);
extern struct t_hdata *gui_buffer_hdata_buffer_visited_cb (const void *pointer,
                                                           void *data,
                                                           const char *hdata_name);
extern int gui_buffer_add_to_infolist (struct t_infolist *infolist,
                                       struct t_gui_buffer *buffer);
extern void gui_buffer_dump_hexa (struct t_gui_buffer *buffer);
extern void gui_buffer_print_log ();

#endif /* WEECHAT_GUI_BUFFER_H */
