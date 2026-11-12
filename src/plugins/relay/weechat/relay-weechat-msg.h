/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_RELAY_WEECHAT_MSG_H
#define WEECHAT_PLUGIN_RELAY_WEECHAT_MSG_H

#include <time.h>

struct t_relay_client;
struct t_relay_weechat_nicklist;

#define RELAY_WEECHAT_MSG_INITIAL_ALLOC 4096

/* Object ids in binary messages */
#define RELAY_WEECHAT_MSG_OBJ_CHAR      "chr"
#define RELAY_WEECHAT_MSG_OBJ_INT       "int"
#define RELAY_WEECHAT_MSG_OBJ_LONG      "lon"
#define RELAY_WEECHAT_MSG_OBJ_STRING    "str"
#define RELAY_WEECHAT_MSG_OBJ_BUFFER    "buf"
#define RELAY_WEECHAT_MSG_OBJ_POINTER   "ptr"
#define RELAY_WEECHAT_MSG_OBJ_TIME      "tim"
#define RELAY_WEECHAT_MSG_OBJ_HASHTABLE "htb"
#define RELAY_WEECHAT_MSG_OBJ_HDATA     "hda"
#define RELAY_WEECHAT_MSG_OBJ_INFO      "inf"
#define RELAY_WEECHAT_MSG_OBJ_INFOLIST  "inl"
#define RELAY_WEECHAT_MSG_OBJ_ARRAY     "arr"

/* Source of a string converted without RGB colors */
enum t_relay_weechat_msg_var_source
{
    RELAY_WEECHAT_MSG_VAR_HDATA = 0,      /* variable of hdata              */
    RELAY_WEECHAT_MSG_VAR_INFOLIST,       /* variable of infolist           */
    RELAY_WEECHAT_MSG_VAR_INFO,           /* info (no variable name)        */
};

/* Type of string of hdata/infolist/info converted without RGB colors */
enum t_relay_weechat_msg_var_type
{
    RELAY_WEECHAT_MSG_VAR_COLOR_NAME = 1, /* color name ("#ff8800")         */
    RELAY_WEECHAT_MSG_VAR_TAGS,           /* tags of a line ("prefix_nick_") */
};

struct t_relay_weechat_msg_color_var
{
    enum t_relay_weechat_msg_var_source source; /* hdata, infolist or info  */
    const char *object;                /* name of hdata, infolist or info   */
    const char *var;                   /* mask of variable ("*" for info)   */
    enum t_relay_weechat_msg_var_type type; /* type of variable             */
};

struct t_relay_weechat_msg
{
    char *id;                          /* message id                        */
    char *data;                        /* binary buffer                     */
    int data_alloc;                    /* currently allocated size          */
    int data_size;                     /* current size of buffer            */
    int rgb_colors;                    /* 1 to keep RGB colors in strings,  */
                                       /* 0 to use nearest terminal color   */
};

extern struct t_relay_weechat_msg *relay_weechat_msg_new (struct t_relay_client *client,
                                                          const char *id);
extern void relay_weechat_msg_add_bytes (struct t_relay_weechat_msg *msg,
                                         const void *buffer, int size);
extern void relay_weechat_msg_set_bytes (struct t_relay_weechat_msg *msg,
                                         int position, const void *buffer,
                                         int size);
extern void relay_weechat_msg_add_type (struct t_relay_weechat_msg *msg,
                                        const char *string);
extern void relay_weechat_msg_add_char (struct t_relay_weechat_msg *msg,
                                        char c);
extern void relay_weechat_msg_add_int (struct t_relay_weechat_msg *msg,
                                       int value);
extern void relay_weechat_msg_add_long (struct t_relay_weechat_msg *msg,
                                        long value);
extern void relay_weechat_msg_add_longlong (struct t_relay_weechat_msg *msg,
                                            long long value);
extern void relay_weechat_msg_add_string (struct t_relay_weechat_msg *msg,
                                          const char *string);
extern void relay_weechat_msg_add_color_name (struct t_relay_weechat_msg *msg,
                                              const char *name);
extern void relay_weechat_msg_add_var_string (struct t_relay_weechat_msg *msg,
                                              enum t_relay_weechat_msg_var_source source,
                                              const char *object,
                                              const char *var,
                                              const char *value);
extern void relay_weechat_msg_add_buffer (struct t_relay_weechat_msg *msg,
                                          void *data, int length);
extern void relay_weechat_msg_add_pointer (struct t_relay_weechat_msg *msg,
                                           void *pointer);
extern void relay_weechat_msg_add_time (struct t_relay_weechat_msg *msg,
                                        time_t time);
extern void relay_weechat_msg_add_hashtable (struct t_relay_weechat_msg *msg,
                                             struct t_hashtable *hashtable);
extern int relay_weechat_msg_add_hdata (struct t_relay_weechat_msg *msg,
                                        const char *path, const char *keys);
extern void relay_weechat_msg_add_infolist (struct t_relay_weechat_msg *msg,
                                            const char *name,
                                            void *pointer,
                                            const char *arguments);
extern void relay_weechat_msg_add_nicklist (struct t_relay_weechat_msg *msg,
                                            struct t_gui_buffer *buffer,
                                            struct t_relay_weechat_nicklist *nicklist);
extern void relay_weechat_msg_send (struct t_relay_client *client,
                                    struct t_relay_weechat_msg *msg);
extern void relay_weechat_msg_free (struct t_relay_weechat_msg *msg);

#endif /* WEECHAT_PLUGIN_RELAY_WEECHAT_MSG_H */
