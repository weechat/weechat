/*
 * wee-hashtable.c - implementation of hashtable
 *
 * Copyright (C) 2010-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "weechat.h"
#include "wee-hashtable.h"
#include "wee-infolist.h"
#include "wee-list.h"
#include "wee-log.h"
#include "wee-string.h"
#include "wee-utf8.h"
#include "../plugins/plugin.h"


char *hashtable_type_string[HASHTABLE_NUM_TYPES] =
{ WEECHAT_HASHTABLE_INTEGER, WEECHAT_HASHTABLE_STRING,
  WEECHAT_HASHTABLE_POINTER, WEECHAT_HASHTABLE_BUFFER,
  WEECHAT_HASHTABLE_TIME };


/*
 * Searches for a hashtable type.
 *
 * Returns index of type in enum t_hashtable_type, -1 if type is not found.
 */

int
hashtable_get_type (const char *type)
{
    int i;

    if (!type)
        return -1;

    for (i = 0; i < HASHTABLE_NUM_TYPES; i++)
    {
        if (strcmp (hashtable_type_string[i], type) == 0)
            return i;
    }

    /* type not found */
    return -1;
}

/*
 * Hashes a string using a variant of djb2 hash.
 *
 * Returns the hash of the string.
 */

unsigned long long
hashtable_hash_key_djb2 (const char *string)
{
    uint64_t hash;
    const char *ptr_string;

    hash = 5381;
    for (ptr_string = string; ptr_string[0]; ptr_string++)
    {
        hash ^= (hash << 5) + (hash >> 2) + (int)(ptr_string[0]);
    }

    return hash;
}

/*
 * Hashes a key (default callback).
 *
 * Returns the hash of the key, depending on the type.
 */

unsigned long long
hashtable_hash_key_default_cb (struct t_hashtable *hashtable, const void *key)
{
    unsigned long long hash;

    hash = 0;

    switch (hashtable->type_keys)
    {
        case HASHTABLE_INTEGER:
            hash = (unsigned long long)(*((int *)key));
            break;
        case HASHTABLE_STRING:
            hash = hashtable_hash_key_djb2 ((const char *)key);
            break;
        case HASHTABLE_POINTER:
            hash = (unsigned long long)((unsigned long)((void *)key));
            break;
        case HASHTABLE_BUFFER:
            break;
        case HASHTABLE_TIME:
            hash = (unsigned long long)(*((time_t *)key));
            break;
        case HASHTABLE_NUM_TYPES:
            break;
    }

    return hash;
}

/*
 * Compares two keys (default callback).
 *
 * Returns:
 *   < 0: key1 < key2
 *     0: key1 == key2
 *   > 0: key1 > key2
 */

int
hashtable_keycmp_default_cb (struct t_hashtable *hashtable,
                             const void *key1, const void *key2)
{
    int rc;

    /* make C compiler happy */
    (void) hashtable;

    rc = 0;

    switch (hashtable->type_keys)
    {
        case HASHTABLE_INTEGER:
            if (*((int *)key1) < *((int *)key2))
                rc = -1;
            else if (*((int *)key1) > *((int *)key2))
                rc = 1;
            break;
        case HASHTABLE_STRING:
            rc = strcmp ((const char *)key1, (const char *)key2);
            break;
        case HASHTABLE_POINTER:
            if (key1 < key2)
                rc = -1;
            else if (key1 > key2)
                rc = 1;
            break;
        case HASHTABLE_BUFFER:
            break;
        case HASHTABLE_TIME:
            if (*((time_t *)key1) < *((time_t *)key2))
                rc = -1;
            else if (*((time_t *)key1) > *((time_t *)key2))
                rc = 1;
            break;
        case HASHTABLE_NUM_TYPES:
            break;
    }

    return rc;
}

/*
 * Creates a new hashtable.
 *
 * The size is NOT a limit for number of items in hashtable. It is the size of
 * internal array to store hashed keys: a high value uses more memory, but has
 * better performance because this reduces the collisions of hashed keys and
 * then reduces length of linked lists.
 *
 * Returns pointer to new hashtable, NULL if error.
 */

struct t_hashtable *
hashtable_new (int size,
               const char *type_keys, const char *type_values,
               t_hashtable_hash_key *callback_hash_key,
               t_hashtable_keycmp *callback_keycmp)
{
    struct t_hashtable *new_hashtable;
    int i, type_keys_int, type_values_int;

    if (size <= 0)
        return NULL;

    type_keys_int = hashtable_get_type (type_keys);
    if (type_keys_int < 0)
        return NULL;
    type_values_int = hashtable_get_type (type_values);
    if (type_values_int < 0)
        return NULL;

    /* the two callbacks are mandatory if type of keys is "buffer" */
    if ((type_keys_int == HASHTABLE_BUFFER) && (!callback_hash_key || !callback_keycmp))
        return NULL;

    new_hashtable = malloc (sizeof (*new_hashtable));
    if (new_hashtable)
    {
        new_hashtable->size = size;
        new_hashtable->type_keys = type_keys_int;
        new_hashtable->type_values = type_values_int;
        new_hashtable->htable = malloc (size * sizeof (*(new_hashtable->htable)));
        new_hashtable->keys_values = NULL;
        if (!new_hashtable->htable)
        {
            free (new_hashtable);
            return NULL;
        }
        for (i = 0; i < size; i++)
        {
            new_hashtable->htable[i] = NULL;
        }
        new_hashtable->items_count = 0;
        new_hashtable->oldest_item = NULL;
        new_hashtable->newest_item = NULL;

        new_hashtable->callback_hash_key = (callback_hash_key) ?
            callback_hash_key : &hashtable_hash_key_default_cb;
        new_hashtable->callback_keycmp = (callback_keycmp) ?
            callback_keycmp : &hashtable_keycmp_default_cb;

        new_hashtable->callback_free_key = NULL;
        new_hashtable->callback_free_value = NULL;
    }
    return new_hashtable;
}

/*
 * Allocates space for a key or value.
 */

void
hashtable_alloc_type (enum t_hashtable_type type,
                      const void *value, int size_value,
                      void **pointer, int *size)
{
    switch (type)
    {
        case HASHTABLE_INTEGER:
            if (value)
            {
                *pointer = malloc (sizeof (int));
                if (*pointer)
                    *((int *)(*pointer)) = *((int *)value);
            }
            else
                *pointer = NULL;
            *size = (*pointer) ? sizeof (int) : 0;
            break;
        case HASHTABLE_STRING:
            *pointer = (value) ? strdup ((const char *)value) : NULL;
            *size = (*pointer) ? strlen (*pointer) + 1 : 0;
            break;
        case HASHTABLE_POINTER:
            *pointer = (void *)value;
            *size = sizeof (void *);
            break;
        case HASHTABLE_BUFFER:
            if (value && (size_value > 0))
            {
                *pointer = malloc (size_value);
                if (*pointer)
                    memcpy (*pointer, value, size_value);
            }
            else
                *pointer = NULL;
            *size = (*pointer) ? size_value : 0;
            break;
        case HASHTABLE_TIME:
            if (value)
            {
                *pointer = malloc (sizeof (time_t));
                if (*pointer)
                    *((time_t *)(*pointer)) = *((time_t *)value);
            }
            else
                *pointer = NULL;
            *size = (*pointer) ? sizeof (time_t) : 0;
            break;
        case HASHTABLE_NUM_TYPES:
            break;
    }
}

/*
 * Frees space used by a key.
 */

void
hashtable_free_key (struct t_hashtable *hashtable,
                    struct t_hashtable_item *item)
{
    if (hashtable->callback_free_key)
    {
        (void) (hashtable->callback_free_key) (hashtable,
                                               item->key);
    }
    else
    {
        switch (hashtable->type_keys)
        {
            case HASHTABLE_INTEGER:
            case HASHTABLE_STRING:
            case HASHTABLE_BUFFER:
            case HASHTABLE_TIME:
                if (item->key)
                    free (item->key);
                break;
            case HASHTABLE_POINTER:
                break;
            case HASHTABLE_NUM_TYPES:
                break;
        }
    }
}

/*
 * Frees space used by a value.
 */

void
hashtable_free_value (struct t_hashtable *hashtable,
                      struct t_hashtable_item *item)
{
    if (hashtable->callback_free_value)
    {
        (void) (hashtable->callback_free_value) (hashtable,
                                                 item->key,
                                                 item->value);
    }
    else
    {
        switch (hashtable->type_values)
        {
            case HASHTABLE_INTEGER:
            case HASHTABLE_STRING:
            case HASHTABLE_BUFFER:
            case HASHTABLE_TIME:
                if (item->value)
                    free (item->value);
                break;
            case HASHTABLE_POINTER:
                break;
            case HASHTABLE_NUM_TYPES:
                break;
        }
    }
}

/*
 * Sets value for a key in hashtable.
 *
 * The size arguments are used only for type "buffer".
 *
 * Returns pointer to item created/updated, NULL if error.
 */

struct t_hashtable_item *
hashtable_set_with_size (struct t_hashtable *hashtable,
                         const void *key, int key_size,
                         const void *value, int value_size)
{
    unsigned long long hash;
    struct t_hashtable_item *ptr_item, *pos_item, *new_item;

    if (!hashtable || !key
        || ((hashtable->type_keys == HASHTABLE_BUFFER) && (key_size <= 0))
        || ((hashtable->type_values == HASHTABLE_BUFFER) && (value_size <= 0)))
    {
        return NULL;
    }

    /* search position for item in hashtable */
    hash = hashtable->callback_hash_key (hashtable, key) % hashtable->size;
    pos_item = NULL;
    for (ptr_item = hashtable->htable[hash];
         ptr_item
             && ((int)(hashtable->callback_keycmp) (hashtable, key, ptr_item->key) > 0);
         ptr_item = ptr_item->next_item)
    {
        pos_item = ptr_item;
    }

    /* replace value if item is already in hashtable */
    if (ptr_item && (hashtable->callback_keycmp (hashtable, key, ptr_item->key) == 0))
    {
        hashtable_free_value (hashtable, ptr_item);
        hashtable_alloc_type (hashtable->type_values,
                              value, value_size,
                              &ptr_item->value, &ptr_item->value_size);
        return ptr_item;
    }

    /* create new item */
    new_item = malloc (sizeof (*new_item));
    if (!new_item)
        return NULL;

    /* set key and value */
    hashtable_alloc_type (hashtable->type_keys,
                          key, key_size,
                          &new_item->key, &new_item->key_size);
    hashtable_alloc_type (hashtable->type_values,
                          value, value_size,
                          &new_item->value, &new_item->value_size);

    /* add item */
    if (pos_item)
    {
        /* insert item after position found */
        new_item->prev_item = pos_item;
        new_item->next_item = pos_item->next_item;
        if (pos_item->next_item)
            (pos_item->next_item)->prev_item = new_item;
        pos_item->next_item = new_item;
    }
    else
    {
        /* insert item at beginning of list */
        new_item->prev_item = NULL;
        new_item->next_item = hashtable->htable[hash];
        if (hashtable->htable[hash])
            (hashtable->htable[hash])->prev_item = new_item;
        hashtable->htable[hash] = new_item;
    }

    /* keep items ordered by date of creation */
    if (hashtable->newest_item)
        (hashtable->newest_item)->next_created_item = new_item;
    else
        hashtable->oldest_item = new_item;
    new_item->prev_created_item = hashtable->newest_item;
    new_item->next_created_item = NULL;
    hashtable->newest_item = new_item;

    hashtable->items_count++;

    return new_item;
}

/*
 * Sets value for a key in hashtable.
 *
 * Note: this function can be called *only* if key AND value are *not* of type
 * "buffer".
 *
 * Returns pointer to item created/updated, NULL if error.
 */

struct t_hashtable_item *
hashtable_set (struct t_hashtable *hashtable,
               const void *key, const void *value)
{
    return hashtable_set_with_size (hashtable, key, 0, value, 0);
}

/*
 * Searches for an item in hashtable.
 *
 * If hash is non NULL, then it is set with hash value of key (even if key is
 * not found).
 */

struct t_hashtable_item *
hashtable_get_item (struct t_hashtable *hashtable, const void *key,
                    unsigned long long *hash)
{
    unsigned long long key_hash;
    struct t_hashtable_item *ptr_item;

    if (!hashtable || !key)
        return NULL;

    key_hash = hashtable->callback_hash_key (hashtable, key) % hashtable->size;
    if (hash)
        *hash = key_hash;
    for (ptr_item = hashtable->htable[key_hash];
         ptr_item && hashtable->callback_keycmp (hashtable, key, ptr_item->key) > 0;
         ptr_item = ptr_item->next_item)
    {
    }

    if (ptr_item
        && (hashtable->callback_keycmp (hashtable, key, ptr_item->key) == 0))
    {
        return ptr_item;
    }

    return NULL;
}

/*
 * Gets value for a key in hashtable.
 *
 * Returns pointer to value for key, NULL if key is not found.
 */

void *
hashtable_get (struct t_hashtable *hashtable, const void *key)
{
    struct t_hashtable_item *ptr_item;

    ptr_item = hashtable_get_item (hashtable, key, NULL);

    return (ptr_item) ? ptr_item->value : NULL;
}

/*
 * Checks if a key exists in the hashtable.
 *
 * Returns:
 *   1: key exists
 *   0: key does not exist
 */

int
hashtable_has_key (struct t_hashtable *hashtable, const void *key)
{
    return (hashtable_get_item (hashtable, key, NULL) != NULL) ? 1 : 0;
}

/*
 * Converts a value (from any type) to a string.
 *
 * Returns pointer to a static buffer (for type string, returns pointer to
 * string itself), which must be used immediately, it is overwritten by
 * subsequent calls to this function.
 */

const char *
hashtable_to_string (enum t_hashtable_type type, const void *value)
{
    static char str_value[128];

    switch (type)
    {
        case HASHTABLE_INTEGER:
            snprintf (str_value, sizeof (str_value), "%d", *((int *)value));
            return str_value;
            break;
        case HASHTABLE_STRING:
            return (const char *)value;
            break;
        case HASHTABLE_POINTER:
        case HASHTABLE_BUFFER:
            snprintf (str_value, sizeof (str_value),
                      "0x%lx", (unsigned long)value);
            return str_value;
            break;
        case HASHTABLE_TIME:
            snprintf (str_value, sizeof (str_value),
                      "%lld", (long long)(*((time_t *)value)));
            return str_value;
            break;
        case HASHTABLE_NUM_TYPES:
            break;
    }

    return NULL;
}

/*
 * Calls a function on all hashtable entries.
 */

void
hashtable_map (struct t_hashtable *hashtable,
               t_hashtable_map *callback_map,
               void *callback_map_data)
{
    struct t_hashtable_item *ptr_item, *ptr_next_created_item;

    if (!hashtable)
        return;

    ptr_item = hashtable->oldest_item;
    while (ptr_item)
    {
        ptr_next_created_item = ptr_item->next_created_item;

        (void) (callback_map) (callback_map_data,
                               hashtable,
                               ptr_item->key,
                               ptr_item->value);

        ptr_item = ptr_next_created_item;
    }
}

/*
 * Calls a function on all hashtable entries (sends keys and values as strings).
 */

void
hashtable_map_string (struct t_hashtable *hashtable,
                      t_hashtable_map_string *callback_map,
                      void *callback_map_data)
{
    struct t_hashtable_item *ptr_item, *ptr_next_created_item;
    const char *str_key, *str_value;
    char *key, *value;

    if (!hashtable)
        return;

    ptr_item = hashtable->oldest_item;
    while (ptr_item)
    {
        ptr_next_created_item = ptr_item->next_created_item;

        str_key = hashtable_to_string (hashtable->type_keys,
                                       ptr_item->key);
        key = (str_key) ? strdup (str_key) : NULL;

        str_value = hashtable_to_string (hashtable->type_values,
                                         ptr_item->value);
        value = (str_value) ? strdup (str_value) : NULL;

        (void) (callback_map) (callback_map_data,
                               hashtable,
                               key,
                               value);

        if (key)
            free (key);
        if (value)
            free (value);

        ptr_item = ptr_next_created_item;
    }
}

/*
 * Duplicates key/value in another hashtable (callback called for each variable
 * in hashtable).
 */

void
hashtable_duplicate_map_cb (void *data,
                            struct t_hashtable *hashtable,
                            const void *key, const void *value)
{
    struct t_hashtable *hashtable2;

    /* make C compiler happy */
    (void) hashtable;

    hashtable2 = (struct t_hashtable *)data;
    if (hashtable2)
        hashtable_set (hashtable2, key, value);
}

/*
 * Duplicates a hashtable.
 *
 * Returns pointer to new hashtable, NULL if error.
 */

struct t_hashtable *
hashtable_dup (struct t_hashtable *hashtable)
{
    struct t_hashtable *new_hashtable;

    if (!hashtable)
        return NULL;

    new_hashtable = hashtable_new (hashtable->size,
                                   hashtable_type_string[hashtable->type_keys],
                                   hashtable_type_string[hashtable->type_values],
                                   hashtable->callback_hash_key,
                                   hashtable->callback_keycmp);
    if (new_hashtable)
    {
        new_hashtable->callback_free_key = hashtable->callback_free_key;
        new_hashtable->callback_free_value = hashtable->callback_free_value;
        hashtable_map (hashtable,
                       &hashtable_duplicate_map_cb,
                       new_hashtable);
    }

    return new_hashtable;
}

/*
 * Builds sorted list of keys (callback called for each variable in hashtable).
 */

void
hashtable_get_list_keys_map_cb (void *data,
                                struct t_hashtable *hashtable,
                                const void *key, const void *value)
{
    struct t_weelist *list;

    /* make C compiler happy */
    (void) hashtable;
    (void) value;

    list = (struct t_weelist *)data;

    weelist_add (list, hashtable_to_string (hashtable->type_keys, key),
                 WEECHAT_LIST_POS_SORT, NULL);
}

/*
 * Gets list with sorted keys of hashtable.
 *
 * Note: list must be freed after use.
 */

struct t_weelist *
hashtable_get_list_keys (struct t_hashtable *hashtable)
{
    struct t_weelist *weelist;

    if (!hashtable)
        return NULL;

    weelist = weelist_new ();
    if (weelist)
        hashtable_map (hashtable, &hashtable_get_list_keys_map_cb, weelist);
    return weelist;
}

/*
 * Gets a hashtable property as integer.
 */

int
hashtable_get_integer (struct t_hashtable *hashtable, const char *property)
{
    if (!hashtable || !property)
        return 0;

    if (strcmp (property, "size") == 0)
        return hashtable->size;
    else if (strcmp (property, "items_count") == 0)
        return hashtable->items_count;

    return 0;
}

/*
 * Computes length of all keys (callback called for each variable in hashtable).
 */

void
hashtable_compute_length_keys_cb (void *data,
                                  struct t_hashtable *hashtable,
                                  const void *key, const void *value)
{
    const char *str_key;
    int *length;

    /* make C compiler happy */
    (void) value;

    length = (int *)data;

    str_key = hashtable_to_string (hashtable->type_keys, key);
    if (str_key)
        *length += strlen (str_key) + 1;
}

/*
 * Computes length of all values (callback called for each variable in
 * hashtable).
 */

void
hashtable_compute_length_values_cb (void *data,
                                    struct t_hashtable *hashtable,
                                    const void *key, const void *value)
{
    const char *str_value;
    int *length;

    /* make C compiler happy */
    (void) key;

    length = (int *)data;

    if (value)
    {
        str_value = hashtable_to_string (hashtable->type_values, value);
        if (str_value)
            *length += strlen (str_value) + 1;
    }
    else
    {
        *length += strlen ("(null)") + 1;
    }
}

/*
 * Computes length of all keys + values (callback called for each variable in
 * hashtable).
 */

void
hashtable_compute_length_keys_values_cb (void *data,
                                         struct t_hashtable *hashtable,
                                         const void *key, const void *value)
{
    hashtable_compute_length_keys_cb (data, hashtable, key, value);
    hashtable_compute_length_values_cb (data, hashtable, key, value);
}

/*
 * Builds a string with all keys (callback called for each variable in
 * hashtable).
 */

void
hashtable_build_string_keys_cb (void *data,
                                struct t_hashtable *hashtable,
                                const void *key, const void *value)
{
    const char *str_key;
    char *str;

    /* make C compiler happy */
    (void) value;

    str = (char *)data;

    if (str[0])
        strcat (str, ",");

    str_key = hashtable_to_string (hashtable->type_keys, key);
    if (str_key)
        strcat (str, str_key);
}

/*
 * Builds a string with all values (callback called for each variable in
 * hashtable).
 */

void
hashtable_build_string_values_cb (void *data,
                                  struct t_hashtable *hashtable,
                                  const void *key, const void *value)
{
    const char *str_value;
    char *str;

    /* make C compiler happy */
    (void) key;

    str = (char *)data;

    if (str[0])
        strcat (str, ",");

    if (value)
    {
        str_value = hashtable_to_string (hashtable->type_values, value);
        if (str_value)
            strcat (str, str_value);
    }
    else
    {
        strcat (str, "(null)");
    }
}

/*
 * Builds a string with all keys + values (callback called for each variable in
 * hashtable).
 */

void
hashtable_build_string_keys_values_cb (void *data,
                                       struct t_hashtable *hashtable,
                                       const void *key, const void *value)
{
    const char *str_key, *str_value;
    char *str;

    str = (char *)data;

    if (str[0])
        strcat (str, ",");

    str_key = hashtable_to_string (hashtable->type_keys, key);
    if (str_key)
        strcat (str, str_key);

    strcat (str, ":");

    if (value)
    {
        str_value = hashtable_to_string (hashtable->type_values, value);
        if (str_value)
            strcat (str, str_value);
    }
    else
    {
        strcat (str, "(null)");
    }
}

/*
 * Gets keys and/or values of hashtable as string.
 *
 * Returns a string with one of these formats:
 *   if keys == 1 and values == 0: "key1,key2,key3"
 *   if keys == 0 and values == 1: "value1,value2,value3"
 *   if keys == 1 and values == 1: "key1:value1,key2:value2,key3:value3"
 */

const char *
hashtable_get_keys_values (struct t_hashtable *hashtable,
                           int keys, int sort_keys, int values)
{
    int length;
    struct t_weelist *list_keys;
    struct t_weelist_item *ptr_item;

    if (hashtable->keys_values)
    {
        free (hashtable->keys_values);
        hashtable->keys_values = NULL;
    }

    /* first compute length of string */
    length = 0;
    hashtable_map (hashtable,
                   (keys && values) ? &hashtable_compute_length_keys_values_cb :
                   ((keys) ? &hashtable_compute_length_keys_cb :
                    &hashtable_compute_length_values_cb),
                   &length);
    if (length == 0)
        return hashtable->keys_values;

    /* build string */
    hashtable->keys_values = malloc (length + 1);
    if (!hashtable->keys_values)
        return NULL;
    hashtable->keys_values[0] = '\0';
    if (keys && sort_keys)
    {
        list_keys = hashtable_get_list_keys (hashtable);
        if (list_keys)
        {
            for (ptr_item = list_keys->items; ptr_item;
                 ptr_item = ptr_item->next_item)
            {
                if (values)
                {
                    hashtable_build_string_keys_values_cb (hashtable->keys_values,
                                                           hashtable,
                                                           ptr_item->data,
                                                           hashtable_get (hashtable,
                                                                          ptr_item->data));
                }
                else
                {
                    hashtable_build_string_keys_cb (hashtable->keys_values,
                                                    hashtable,
                                                    ptr_item->data,
                                                    NULL);
                }
            }
            weelist_free (list_keys);
        }
    }
    else
    {
        hashtable_map (hashtable,
                       (keys && values) ? &hashtable_build_string_keys_values_cb :
                       ((keys) ? &hashtable_build_string_keys_cb :
                        &hashtable_build_string_values_cb),
                       hashtable->keys_values);
    }

    return hashtable->keys_values;
}

/*
 * Gets a hashtable property as string.
 */

const char *
hashtable_get_string (struct t_hashtable *hashtable, const char *property)
{
    if (!hashtable || !property)
        return NULL;

    if (strcmp (property, "type_keys") == 0)
        return hashtable_type_string[hashtable->type_keys];
    else if (strcmp (property, "type_values") == 0)
        return hashtable_type_string[hashtable->type_values];
    else if (strcmp (property, "keys") == 0)
        return hashtable_get_keys_values (hashtable, 1, 0, 0);
    else if (strcmp (property, "keys_sorted") == 0)
        return hashtable_get_keys_values (hashtable, 1, 1, 0);
    else if (strcmp (property, "values") == 0)
        return hashtable_get_keys_values (hashtable, 0, 0, 1);
    else if (strcmp (property, "keys_values") == 0)
        return hashtable_get_keys_values (hashtable, 1, 0, 1);
    else if (strcmp (property, "keys_values_sorted") == 0)
        return hashtable_get_keys_values (hashtable, 1, 1, 1);

    return NULL;
}

/*
 * Sets a hashtable property (pointer).
 */

void
hashtable_set_pointer (struct t_hashtable *hashtable, const char *property,
                       void *pointer)
{
    if (!hashtable || !property)
        return;

    if (strcmp (property, "callback_free_key") == 0)
        hashtable->callback_free_key = pointer;
    else if (strcmp (property, "callback_free_value") == 0)
        hashtable->callback_free_value = pointer;
}

/*
 * Adds hashtable keys and values in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hashtable_add_to_infolist (struct t_hashtable *hashtable,
                           struct t_infolist_item *infolist_item,
                           const char *prefix)
{
    int item_number;
    struct t_hashtable_item *ptr_item;
    char option_name[128];

    if (!hashtable || !infolist_item || !prefix)
        return 0;

    item_number = 0;
    ptr_item = hashtable->oldest_item;
    while (ptr_item)
    {
        snprintf (option_name, sizeof (option_name),
                  "%s_name_%05d", prefix, item_number);
        if (!infolist_new_var_string (infolist_item, option_name,
                                      hashtable_to_string (hashtable->type_keys,
                                                           ptr_item->key)))
            return 0;
        snprintf (option_name, sizeof (option_name),
                  "%s_value_%05d", prefix, item_number);
        switch (hashtable->type_values)
        {
            case HASHTABLE_INTEGER:
                if (!infolist_new_var_integer (infolist_item, option_name,
                                               *((int *)ptr_item->value)))
                    return 0;
                break;
            case HASHTABLE_STRING:
                if (!infolist_new_var_string (infolist_item, option_name,
                                              (const char *)ptr_item->value))
                    return 0;
                break;
            case HASHTABLE_POINTER:
                if (!infolist_new_var_pointer (infolist_item, option_name,
                                               ptr_item->value))
                    return 0;
                break;
            case HASHTABLE_BUFFER:
                if (!infolist_new_var_buffer (infolist_item, option_name,
                                              ptr_item->value,
                                              ptr_item->value_size))
                    return 0;
                break;
            case HASHTABLE_TIME:
                if (!infolist_new_var_time (infolist_item, option_name,
                                            *((time_t *)ptr_item->value)))
                    return 0;
                break;
            case HASHTABLE_NUM_TYPES:
                break;
        }
        item_number++;
        ptr_item = ptr_item->next_created_item;
    }
    return 1;
}

/*
 * Adds hashtable keys and values from an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hashtable_add_from_infolist (struct t_hashtable *hashtable,
                             struct t_infolist *infolist,
                             const char *prefix)
{
    struct t_infolist_var *ptr_name, *ptr_value;
    char prefix_name[1024], option_value[1024];
    int prefix_length, prefix_length_utf8;

    if (!hashtable || !infolist || !infolist->ptr_item || !prefix)
        return 0;

    /* TODO: implement other key types */
    if (hashtable->type_keys != HASHTABLE_STRING)
        return 0;

    snprintf (prefix_name, sizeof (prefix_name), "%s_name_", prefix);
    prefix_length = strlen (prefix_name);
    prefix_length_utf8 = utf8_strlen (prefix_name);

    for (ptr_name = infolist->ptr_item->vars; ptr_name;
         ptr_name = ptr_name->next_var)
    {
        if (string_strncasecmp (ptr_name->name, prefix_name,
                                prefix_length_utf8) == 0)
        {
            snprintf (option_value, sizeof (option_value),
                      "%s_value_%s", prefix, ptr_name->name + prefix_length);

            ptr_value = infolist_search_var (infolist, option_value);
            if (ptr_value)
            {
                switch (hashtable->type_values)
                {
                    case HASHTABLE_INTEGER:
                        if (ptr_value->type != INFOLIST_INTEGER)
                            return 0;
                        break;
                    case HASHTABLE_STRING:
                        if (ptr_value->type != INFOLIST_STRING)
                            return 0;
                        break;
                    case HASHTABLE_POINTER:
                        if (ptr_value->type != INFOLIST_POINTER)
                            return 0;
                        break;
                    case HASHTABLE_BUFFER:
                        if (ptr_value->type != INFOLIST_BUFFER)
                            return 0;
                        break;
                    case HASHTABLE_TIME:
                        if (ptr_value->type != INFOLIST_TIME)
                            return 0;
                        break;
                    case HASHTABLE_NUM_TYPES:
                        break;
                }
                if (hashtable->type_values == HASHTABLE_BUFFER)
                {
                    hashtable_set_with_size (hashtable,
                                             ptr_name->value,
                                             0,
                                             ptr_value->value,
                                             ptr_value->size);
                }
                else
                {
                    hashtable_set (hashtable,
                                   ptr_name->value,
                                   ptr_value->value);
                }
            }
        }
    }

    return 1;
}

/*
 * Removes an item from hashtable.
 */

void
hashtable_remove_item (struct t_hashtable *hashtable,
                       struct t_hashtable_item *item,
                       unsigned long long hash)
{
    if (!hashtable || !item)
        return;

    /* free key and value */
    hashtable_free_value (hashtable, item);
    hashtable_free_key (hashtable, item);

    /* remove item from ordered list (by date of creation) */
    if (item->prev_created_item)
        (item->prev_created_item)->next_created_item = item->next_created_item;
    if (item->next_created_item)
        (item->next_created_item)->prev_created_item = item->prev_created_item;
    if (hashtable->oldest_item == item)
        hashtable->oldest_item = item->next_created_item;
    if (hashtable->newest_item == item)
        hashtable->newest_item = item->prev_created_item;

    /* remove item from list */
    if (item->prev_item)
        (item->prev_item)->next_item = item->next_item;
    if (item->next_item)
        (item->next_item)->prev_item = item->prev_item;
    if (hashtable->htable[hash] == item)
        hashtable->htable[hash] = item->next_item;

    free (item);

    hashtable->items_count--;
}

/*
 * Removes an item from hashtable (searches it with key).
 */

void
hashtable_remove (struct t_hashtable *hashtable, const void *key)
{
    struct t_hashtable_item *ptr_item;
    unsigned long long hash;

    if (!hashtable || !key)
        return;

    ptr_item = hashtable_get_item (hashtable, key, &hash);
    if (ptr_item)
        hashtable_remove_item (hashtable, ptr_item, hash);
}

/*
 * Removes all items from hashtable.
 */

void
hashtable_remove_all (struct t_hashtable *hashtable)
{
    int i;

    if (!hashtable)
        return;

    for (i = 0; i < hashtable->size; i++)
    {
        while (hashtable->htable[i])
        {
            hashtable_remove_item (hashtable, hashtable->htable[i], i);
        }
    }
}

/*
 * Frees a hashtable: removes all items and frees hashtable.
 */

void
hashtable_free (struct t_hashtable *hashtable)
{
    if (!hashtable)
        return;

    hashtable_remove_all (hashtable);
    free (hashtable->htable);
    if (hashtable->keys_values)
        free (hashtable->keys_values);
    free (hashtable);
}

/*
 * Prints hashtable in WeeChat log file (usually for crash dump).
 */

void
hashtable_print_log (struct t_hashtable *hashtable, const char *name)
{
    struct t_hashtable_item *ptr_item;
    int i;

    log_printf ("");
    log_printf ("[hashtable %s (addr:0x%lx)]", name, hashtable);
    log_printf ("  size . . . . . . . . . : %d",    hashtable->size);
    log_printf ("  htable . . . . . . . . : 0x%lx", hashtable->htable);
    log_printf ("  items_count. . . . . . : %d",    hashtable->items_count);
    log_printf ("  oldest_item. . . . . . : 0x%lx", hashtable->oldest_item);
    log_printf ("  newest_item. . . . . . : 0x%lx", hashtable->newest_item);
    log_printf ("  type_keys. . . . . . . : %d (%s)",
                hashtable->type_keys,
                hashtable_type_string[hashtable->type_keys]);
    log_printf ("  type_values. . . . . . : %d (%s)",
                hashtable->type_values,
                hashtable_type_string[hashtable->type_values]);
    log_printf ("  callback_hash_key. . . : 0x%lx", hashtable->callback_hash_key);
    log_printf ("  callback_keycmp. . . . : 0x%lx", hashtable->callback_keycmp);
    log_printf ("  callback_free_key. . . : 0x%lx", hashtable->callback_free_key);
    log_printf ("  callback_free_value. . : 0x%lx", hashtable->callback_free_value);
    log_printf ("  keys_values. . . . . . : '%s'",  hashtable->keys_values);

    for (i = 0; i < hashtable->size; i++)
    {
        log_printf ("  htable[%06d] . . . . : 0x%lx", i, hashtable->htable[i]);
        for (ptr_item = hashtable->htable[i]; ptr_item;
             ptr_item = ptr_item->next_item)
        {
            log_printf ("    [item 0x%lx]", hashtable->htable);
            switch (hashtable->type_keys)
            {
                case HASHTABLE_INTEGER:
                    log_printf ("      key (integer). . . : %d", *((int *)ptr_item->key));
                    break;
                case HASHTABLE_STRING:
                    log_printf ("      key (string) . . . : '%s'", (char *)ptr_item->key);
                    break;
                case HASHTABLE_POINTER:
                    log_printf ("      key (pointer). . . : 0x%lx", ptr_item->key);
                    break;
                case HASHTABLE_BUFFER:
                    log_printf ("      key (buffer) . . . : 0x%lx", ptr_item->key);
                    break;
                case HASHTABLE_TIME:
                    log_printf ("      key (time) . . . . : %lld", (long long)(*((time_t *)ptr_item->key)));
                    break;
                case HASHTABLE_NUM_TYPES:
                    break;
            }
            log_printf ("      key_size . . . . . : %d", ptr_item->key_size);
            switch (hashtable->type_values)
            {
                case HASHTABLE_INTEGER:
                    log_printf ("      value (integer). . : %d", *((int *)ptr_item->value));
                    break;
                case HASHTABLE_STRING:
                    log_printf ("      value (string) . . : '%s'", (char *)ptr_item->value);
                    break;
                case HASHTABLE_POINTER:
                    log_printf ("      value (pointer). . : 0x%lx", ptr_item->value);
                    break;
                case HASHTABLE_BUFFER:
                    log_printf ("      value (buffer) . . : 0x%lx", ptr_item->value);
                    break;
                case HASHTABLE_TIME:
                    log_printf ("      value (time) . . . : %lld", (long long)(*((time_t *)ptr_item->value)));
                    break;
                case HASHTABLE_NUM_TYPES:
                    break;
            }
            log_printf ("      value_size . . . . : %d",    ptr_item->value_size);
            log_printf ("      prev_item. . . . . : 0x%lx", ptr_item->prev_item);
            log_printf ("      next_item. . . . . : 0x%lx", ptr_item->next_item);
            log_printf ("      prev_created_item. : 0x%lx", ptr_item->prev_created_item);
            log_printf ("      next_created_item. : 0x%lx", ptr_item->next_created_item);
        }
    }
}
