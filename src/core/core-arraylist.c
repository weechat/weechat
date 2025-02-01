/*
 * core-arraylist.c - array lists management
 *
 * Copyright (C) 2014-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>

#include "weechat.h"
#include "core-arraylist.h"
#include "core-log.h"
#include "core-string.h"


/*
 * Compares two arraylist entries (default comparator).
 * It just compares pointers.
 *
 * Returns:
 *   -1: pointer1 < pointer2
 *    0: pointer1 == pointer2
 *    1: pointer1 > pointer2
 */

int
arraylist_cmp_default_cb (void *data, struct t_arraylist *arraylist,
                          void *pointer1, void *pointer2)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    if (pointer1 < pointer2)
        return -1;
    if (pointer1 > pointer2)
        return 1;
    return 0;
}

/*
 * Creates a new arraylist.
 *
 * Returns pointer to arraylist, NULL if error.
 */

struct t_arraylist *
arraylist_new (int initial_size,
               int sorted,
               int allow_duplicates,
               t_arraylist_cmp *callback_cmp, void *callback_cmp_data,
               t_arraylist_free *callback_free, void *callback_free_data)
{
    struct t_arraylist *new_arraylist;

    /* check arguments */
    if (initial_size < 0)
        return NULL;

    new_arraylist = malloc (sizeof (*new_arraylist));
    if (!new_arraylist)
        return NULL;

    new_arraylist->size = 0;
    if (initial_size > 0)
    {
        new_arraylist->size_alloc = initial_size;
        new_arraylist->size_alloc_min = initial_size;
        new_arraylist->data = calloc (initial_size,
                                      sizeof (*new_arraylist->data));
        if (!new_arraylist->data)
        {
            free (new_arraylist);
            return NULL;
        }
    }
    else
    {
        new_arraylist->size_alloc = 0;
        new_arraylist->size_alloc_min = 0;
        new_arraylist->data = NULL;
    }
    new_arraylist->sorted = sorted;
    new_arraylist->allow_duplicates = allow_duplicates;
    new_arraylist->callback_cmp = (callback_cmp) ?
        callback_cmp : &arraylist_cmp_default_cb;
    new_arraylist->callback_cmp_data = (callback_cmp) ?
        callback_cmp_data : NULL;
    new_arraylist->callback_free = callback_free;
    new_arraylist->callback_free_data = callback_free_data;

    return new_arraylist;
}

/*
 * Returns the size of an arraylist (number of elements).
 */

int
arraylist_size (struct t_arraylist *arraylist)
{
    if (!arraylist)
        return 0;

    return arraylist->size;
}

/*
 * Returns the pointer to an arraylist element, by index.
 */

void *
arraylist_get (struct t_arraylist *arraylist, int index)
{
    if (!arraylist || (index < 0) || (index >= arraylist->size))
        return NULL;

    return arraylist->data[index];
}

/*
 * Adjusts the allocated size of arraylist to add one element (if needed),
 * so that the list has enough allocated data to store (current_size + 1)
 * elements.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
arraylist_grow (struct t_arraylist *arraylist)
{
    int new_size_alloc;
    void **data;

    if (!arraylist)
        return 0;

    /* if we have enough space allocated, do nothing */
    if (arraylist->size + 1 <= arraylist->size_alloc)
        return 1;

    new_size_alloc = (arraylist->size_alloc < 2) ?
        2 : arraylist->size_alloc + (arraylist->size_alloc / 2);

    data = realloc (arraylist->data,
                    new_size_alloc * sizeof (*arraylist->data));
    if (!data)
        return 0;
    arraylist->data = data;
    memset (&arraylist->data[arraylist->size_alloc],
            0,
            (new_size_alloc - arraylist->size_alloc) *
            sizeof (*arraylist->data));
    arraylist->size_alloc = new_size_alloc;

    return 1;
}

/*
 * Adjusts the allocated size of arraylist to remove one element (if needed),
 * so that the list has enough allocated data to store (current size - 1)
 * elements.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
arraylist_shrink (struct t_arraylist *arraylist)
{
    int new_size_alloc;
    void **data;

    if (!arraylist)
        return 0;

    /* we don't shrink if we are below the min allocated size */
    if ((arraylist->size_alloc == 0)
        || (arraylist->size_alloc <= arraylist->size_alloc_min))
    {
        return 1;
    }

    /* clear the arraylist if current allocated size is 1 */
    if (arraylist->size_alloc == 1)
    {
        free (arraylist->data);
        arraylist->data = NULL;
        arraylist->size_alloc = 0;
        return 1;
    }

    new_size_alloc = arraylist->size_alloc - (arraylist->size_alloc / 2);

    if (arraylist->size - 1 >= new_size_alloc)
        return 1;

    data = realloc (arraylist->data,
                    new_size_alloc * sizeof (*arraylist->data));
    if (!data)
        return 0;
    arraylist->data = data;
    arraylist->size_alloc = new_size_alloc;

    return 1;
}

/*
 * Performs a binary search in the arraylist to find an element
 * (this function must be called only if the arraylist is sorted).
 *
 * If "index" is not NULL, it is set with the index of element found (or -1 if
 * element was not found).
 *
 * If "index_insert" is not NULL, it is set with the index that must be used to
 * insert the element in the arraylist (to keep arraylist sorted).
 *
 * Returns pointer to element found, NULL if not found.
 */

void *
arraylist_binary_search (struct t_arraylist *arraylist, void *pointer,
                         int *index, int *index_insert)
{
    int ret_index, ret_index_insert, start, end, middle, rc;
    void *ret_pointer;

    ret_index = -1;
    ret_index_insert = -1;
    ret_pointer = NULL;

    if (!arraylist)
        goto end;

    start = 0;
    end = arraylist->size - 1;

    /*
     * statistically we often add at the end, or before first element, so
     * first check these cases (for performance), before doing the binary
     * search
     */
    rc = (arraylist->callback_cmp) (arraylist->callback_cmp_data,
                                    arraylist,
                                    pointer,
                                    arraylist->data[end]);
    if (rc == 0)
    {
        ret_index = end;
        /* by convention, add an element with same value after the last one */
        ret_index_insert = end + 1;
        ret_pointer = arraylist->data[end];
        goto end;
    }
    if (rc > 0)
    {
        ret_index = -1;
        ret_index_insert = -1;
        ret_pointer = NULL;
        goto end;
    }
    if (arraylist->size == 1)
    {
        ret_index = -1;
        ret_index_insert = 0;
        ret_pointer = NULL;
        goto end;
    }

    rc = (arraylist->callback_cmp) (arraylist->callback_cmp_data,
                                    arraylist,
                                    pointer,
                                    arraylist->data[start]);
    if (rc == 0)
    {
        ret_index = start;
        ret_index_insert = start + 1;
        ret_pointer = arraylist->data[start];
        goto end;
    }
    if (rc < 0)
    {
        ret_index = -1;
        ret_index_insert = start;
        ret_pointer = NULL;
        goto end;
    }
    if (arraylist->size == 2)
    {
        ret_index = -1;
        ret_index_insert = end;
        ret_pointer = NULL;
        goto end;
    }

    start++;
    end--;

    /* perform a binary search to find the index */
    while (start <= end)
    {
        middle = (start + end) / 2;

        rc = (arraylist->callback_cmp) (arraylist->callback_cmp_data,
                                        arraylist,
                                        pointer,
                                        arraylist->data[middle]);
        if (rc == 0)
        {
            ret_index = middle;
            ret_index_insert = middle + 1;
            ret_pointer = arraylist->data[middle];
            goto end;
        }

        if (rc < 0)
            end = middle - 1;
        else
            start = middle + 1;

        if (start > end)
        {
            ret_index = -1;
            ret_index_insert = (rc < 0) ? middle : middle + 1;
            ret_pointer = NULL;
        }
    }

end:
    if ((ret_index >= 0) && arraylist->allow_duplicates)
    {
        /*
         * in case of duplicates in table, the index of element found
         * is the first element with the value, and the index for
         * insert is the last element with the value + 1
         */
        start = ret_index - 1;
        while (start >= 0)
        {
            rc = (arraylist->callback_cmp) (
                arraylist->callback_cmp_data,
                arraylist,
                pointer,
                arraylist->data[start]);
            if (rc != 0)
                break;
            start--;
        }
        start++;
        end = ret_index + 1;
        while (end < arraylist->size)
        {
            rc = (arraylist->callback_cmp) (
                arraylist->callback_cmp_data,
                arraylist,
                pointer,
                arraylist->data[end]);
            if (rc != 0)
                break;
            end++;
        }
        end--;
        ret_index = start;
        ret_index_insert = end + 1;
        ret_pointer = arraylist->data[start];
    }

    if (index)
        *index = ret_index;
    if (index_insert)
        *index_insert = ret_index_insert;

    return ret_pointer;
}

/*
 * Performs a standard search in the arraylist to find an element
 * (this function must be called only if the arraylist is NOT sorted).
 *
 * If "index" is not NULL, it is set with the index of element found (or -1 if
 * element was not found).
 *
 * If "index_insert" is not NULL, it is set to -1 (elements are always added
 * at the end of list when it is not sorted).
 *
 * Returns pointer to element found, NULL if not found.
 */

void *
arraylist_standard_search (struct t_arraylist *arraylist, void *pointer,
                           int *index, int *index_insert)
{
    int i;

    if (!arraylist)
        goto end;

    for (i = 0; i < arraylist->size; i++)
    {
        if ((arraylist->callback_cmp) (arraylist->callback_cmp_data,
                                       arraylist, arraylist->data[i],
                                       pointer) == 0)
        {
            if (index)
                *index = i;
            if (index_insert)
                *index_insert = -1;
            return arraylist->data[i];
        }
    }

end:
    if (index)
        *index = -1;
    if (index_insert)
        *index_insert = -1;
    return NULL;
}

/*
 * Searches an element in the arraylist.
 *
 * If "index" is not NULL, it is set with the index of element found (or -1 if
 * element was not found).
 *
 * If "index_insert" is not NULL, it is set with the index that must be used to
 * insert the element in the arraylist (to keep arraylist sorted).
 *
 * Returns pointer to element found, NULL if not found.
 */

void *
arraylist_search (struct t_arraylist *arraylist, void *pointer,
                  int *index, int *index_insert)
{
    if (index)
        *index = -1;
    if (index_insert)
        *index_insert = -1;

    if (!arraylist || (arraylist->size == 0))
        return NULL;

    if (arraylist->sorted)
    {
        return arraylist_binary_search (arraylist, pointer,
                                        index, index_insert);
    }
    else
    {
        return arraylist_standard_search (arraylist, pointer,
                                          index, index_insert);
    }
}

/*
 * Inserts an element at a given index (and shifts next elements by one
 * position), or at automatic index if the arraylist is sorted.
 *
 * If the index is negative and that the arraylist is not sorted, the element
 * is added at the end of arraylist.
 *
 * If the arraylist is sorted, the argument "index" is ignored (the element
 * will be inserted at appropriate position, to keep arraylist sorted).
 *
 * Returns the index of the new element (>= 0) or -1 if error.
 */

int
arraylist_insert (struct t_arraylist *arraylist, int index, void *pointer)
{
    int index_insert, i;

    if (!arraylist)
        return -1;

    if (arraylist->sorted)
    {
        (void) arraylist_search (arraylist, pointer, &index, &index_insert);
        if ((index >= 0) && !arraylist->allow_duplicates)
        {
            while ((index < arraylist->size)
                   && (((arraylist->callback_cmp) (
                            arraylist->callback_cmp_data,
                            arraylist, arraylist->data[index],
                            pointer)) == 0))
            {
                arraylist_remove (arraylist, index);
            }
        }
        else
            index = index_insert;
    }
    else if (!arraylist->allow_duplicates)
    {
        /*
         * arraylist is not sorted and does not allow duplicates, then we
         * remove any element with the same value
         */
        i = 0;
        while (i < arraylist->size)
        {
            if ((arraylist->callback_cmp) (arraylist->callback_cmp_data,
                                           arraylist, arraylist->data[i],
                                           pointer) == 0)
            {
                arraylist_remove (arraylist, i);
            }
            else
                i++;
        }
    }

    /* if index is negative or too big, add at the end */
    if ((index < 0) || (index > arraylist->size))
        index = arraylist->size;

    if (!arraylist_grow (arraylist))
        return -1;

    /* shift next elements by one position */
    if (index < arraylist->size)
    {
        memmove (&arraylist->data[index + 1],
                 &arraylist->data[index],
                 (arraylist->size - index) * sizeof (*arraylist->data));
    }

    /* set element */
    arraylist->data[index] = pointer;

    (arraylist->size)++;

    return index;
}

/*
 * Adds an element at the end of arraylist (or in the middle if the arraylist
 * is sorted).
 *
 * Returns the index of the new element (>= 0) or -1 if error.
 */

int
arraylist_add (struct t_arraylist *arraylist, void *pointer)
{
    if (!arraylist)
        return -1;

    return arraylist_insert (arraylist, -1, pointer);
}

/*
 * Removes one element from the arraylist.
 *
 * Returns the index removed or -1 if error.
 */

int
arraylist_remove (struct t_arraylist *arraylist, int index)
{
    if (!arraylist || (index < 0) || (index >= arraylist->size))
        return -1;

    if (arraylist->callback_free)
    {
        (arraylist->callback_free) (arraylist->callback_free_data,
                                    arraylist,
                                    arraylist->data[index]);
    }

    if (index < arraylist->size - 1)
    {
        memmove (&arraylist->data[index],
                 &arraylist->data[index + 1],
                 (arraylist->size - index - 1) * sizeof (*arraylist->data));
        memset (&arraylist->data[arraylist->size - 1], 0,
                sizeof (*arraylist->data));
    }
    else
    {
        memset (&arraylist->data[index], 0, sizeof (*arraylist->data));
    }

    arraylist_shrink (arraylist);

    (arraylist->size)--;

    return index;
}

/*
 * Removes all elements in the arraylist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
arraylist_clear (struct t_arraylist *arraylist)
{
    int i;

    if (!arraylist)
        return 0;

    if (arraylist->callback_free)
    {
        for (i = 0; i < arraylist->size; i++)
        {
            (arraylist->callback_free) (arraylist->callback_free_data,
                                        arraylist,
                                        arraylist->data[i]);
        }
    }

    if (arraylist->data)
    {
        if (arraylist->size_alloc != arraylist->size_alloc_min)
        {
            free (arraylist->data);
            arraylist->data = NULL;
            arraylist->size_alloc = 0;
            if (arraylist->size_alloc_min > 0)
            {
                arraylist->data = calloc (arraylist->size_alloc_min,
                                          sizeof (*arraylist->data));
                if (!arraylist->data)
                    return 0;
                arraylist->size_alloc = arraylist->size_alloc_min;
            }
        }
        else if (arraylist->size_alloc > 0)
        {
            memset (arraylist->data,
                    0,
                    arraylist->size_alloc * sizeof (*arraylist->data));
        }
    }

    arraylist->size = 0;

    return 1;
}

/*
 * Frees an arraylist.
 */

void
arraylist_free (struct t_arraylist *arraylist)
{
    int i;

    if (!arraylist)
        return;

    if (arraylist->callback_free)
    {
        for (i = 0; i < arraylist->size; i++)
        {
            (arraylist->callback_free) (arraylist->callback_free_data,
                                        arraylist,
                                        arraylist->data[i]);
        }
    }
    free (arraylist->data);
    free (arraylist);
}

/*
 * Prints an arraylist in WeeChat log file (usually for crash dump).
 */

void
arraylist_print_log (struct t_arraylist *arraylist, const char *name)
{
    int i;

    log_printf ("[arraylist %s (addr:%p)]", name, arraylist);
    log_printf ("  size . . . . . . . . . : %d", arraylist->size);
    log_printf ("  size_alloc . . . . . . : %d", arraylist->size_alloc);
    log_printf ("  size_alloc_min . . . . : %d", arraylist->size_alloc_min);
    log_printf ("  sorted . . . . . . . . : %d", arraylist->sorted);
    log_printf ("  allow_duplicates . . . : %d", arraylist->allow_duplicates);
    log_printf ("  data . . . . . . . . . : %p", arraylist->data);
    if (arraylist->data)
    {
        for (i = 0; i < arraylist->size_alloc; i++)
        {
            log_printf ("    data[%08d] . . . : %p", i, arraylist->data[i]);
        }
    }
    log_printf ("  callback_cmp . . . . . : %p", arraylist->callback_cmp);
    log_printf ("  callback_cmp_data. . . : %p",
                arraylist->callback_cmp_data);
    log_printf ("  callback_free. . . . . : %p", arraylist->callback_free);
    log_printf ("  callback_free_data . . : %p",
                arraylist->callback_free_data);
}
