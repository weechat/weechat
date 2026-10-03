/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_RELAY_JSON_H
#define WEECHAT_PLUGIN_RELAY_JSON_H

/* max nesting of arrays/objects accepted by the parser */
#define RELAY_JSON_MAX_DEPTH 256

#define RELAY_JSON_FOREACH(__item, __parent)                            \
    for (__item = (__parent) ? (__parent)->child : NULL; __item;        \
         __item = __item->next)

enum t_relay_json_type
{
    RELAY_JSON_NULL = 0,
    RELAY_JSON_FALSE,
    RELAY_JSON_TRUE,
    RELAY_JSON_NUMBER,
    RELAY_JSON_STRING,
    RELAY_JSON_ARRAY,
    RELAY_JSON_OBJECT,
};

struct t_relay_json
{
    enum t_relay_json_type type;       /* type of value                     */
    char *key;                         /* member name (if in an object)     */
    char *string;                      /* string value, or raw number if    */
                                       /* number is not a long long         */
    long long integer;                 /* number value (if is_integer)      */
    int is_integer;                    /* 1 if number is a long long        */
    struct t_relay_json *child;        /* first child (array/object)        */
    struct t_relay_json *last_child;   /* last child (array/object)         */
    int size;                          /* number of children                */
    struct t_relay_json *prev;         /* previous item in array/object     */
    struct t_relay_json *next;         /* next item in array/object         */
};

extern struct t_relay_json *relay_json_new_null (void);
extern struct t_relay_json *relay_json_new_bool (int value);
extern struct t_relay_json *relay_json_new_number (long long value);
extern struct t_relay_json *relay_json_new_string (const char *string);
extern struct t_relay_json *relay_json_new_array (void);
extern struct t_relay_json *relay_json_new_object (void);
extern int relay_json_array_add (struct t_relay_json *array,
                                 struct t_relay_json *item);
extern int relay_json_object_add (struct t_relay_json *object,
                                  const char *key,
                                  struct t_relay_json *item);
extern int relay_json_is_null (struct t_relay_json *json);
extern int relay_json_is_bool (struct t_relay_json *json);
extern int relay_json_is_true (struct t_relay_json *json);
extern int relay_json_is_number (struct t_relay_json *json);
extern int relay_json_is_string (struct t_relay_json *json);
extern int relay_json_is_array (struct t_relay_json *json);
extern int relay_json_is_object (struct t_relay_json *json);
extern int relay_json_get_number (struct t_relay_json *json,
                                  long long *value);
extern const char *relay_json_get_string (struct t_relay_json *json);
extern int relay_json_array_size (struct t_relay_json *json);
extern struct t_relay_json *relay_json_array_get (struct t_relay_json *json,
                                                  int index);
extern struct t_relay_json *relay_json_object_get (struct t_relay_json *json,
                                                   const char *key);
extern struct t_relay_json *relay_json_object_detach (struct t_relay_json *json,
                                                      const char *key);
extern struct t_relay_json *relay_json_parse (const char *string);
extern char *relay_json_print (struct t_relay_json *json);
extern void relay_json_free (struct t_relay_json *json);

#endif /* WEECHAT_PLUGIN_RELAY_JSON_H */
