/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_GUI_FILTER_H
#define WEECHAT_GUI_FILTER_H

#include <regex.h>

#define GUI_FILTER_TAG_NO_FILTER "no_filter"

/* Filter structures */

struct t_gui_line_data;

struct t_gui_filter
{
    int enabled;                       /* 1 if filter enabled, otherwise 0  */
    char *name;                        /* filter name                       */
    char *buffer_name;                 /* name of buffer(s)                 */
    int num_buffers;                   /* number of buffers in list         */
    char **buffers;                    /* list of buffer names              */
    char *tags;                        /* tags                              */
    int tags_count;                    /* number of tags                    */
    char ***tags_array;                /* array of tags                     */
    char *regex;                       /* regex                             */
    regex_t *regex_prefix;             /* regex for line prefix             */
    regex_t *regex_message;            /* regex for line message            */
    struct t_gui_filter *prev_filter;  /* link to previous filter           */
    struct t_gui_filter *next_filter;  /* link to next filter               */
};

/* Filter variables */

extern struct t_gui_filter *gui_filters;
extern struct t_gui_filter *last_gui_filter;
extern int gui_filters_enabled;

/* Filter functions */

extern int gui_filter_check_line (struct t_gui_line_data *line_data);
extern void gui_filter_buffer (struct t_gui_buffer *buffer,
                               struct t_gui_line_data *line_data);
extern void gui_filter_all_buffers (struct t_gui_filter *filter);
extern void gui_filter_global_enable (void);
extern void gui_filter_global_disable (void);
extern struct t_gui_filter *gui_filter_search_by_name (const char *name);
extern struct t_gui_filter *gui_filter_new (int enabled,
                                            const char *name,
                                            const char *buffer_name,
                                            const char *tags,
                                            const char *regex);
extern int gui_filter_rename (struct t_gui_filter *filter,
                              const char *new_name);
extern void gui_filter_free (struct t_gui_filter *filter);
extern void gui_filter_free_all (void);
extern struct t_hdata *gui_filter_hdata_filter_cb (const void *pointer,
                                                   void *data,
                                                   const char *hdata_name);
extern int gui_filter_add_to_infolist (struct t_infolist *infolist,
                                       struct t_gui_filter *filter);
extern void gui_filter_print_log (void);

#endif /* WEECHAT_GUI_FILTER_H */
