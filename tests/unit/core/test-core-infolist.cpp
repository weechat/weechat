/*
 * test-core-infolist.cpp - test infolist functions
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

#include "CppUTest/TestHarness.h"
#include "string.h"

extern "C"
{
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
}

struct t_hook *hook_test_infolist = NULL;


TEST_GROUP(CoreInfolist)
{
    /*
     * Callback for the infolist used in tests.
     */

    static struct t_infolist *
    test_infolist_cb (const void *pointer, void *data,
                      const char *infolist_name, void *obj_pointer,
                      const char *arguments)
    {
        struct t_infolist *ptr_infolist;
        struct t_infolist_item *ptr_item;
        static const char buffer[4] = { 'a', 'b', 'c', 0 };

        /* make C++ compiler happy */
        (void) pointer;
        (void) data;
        (void) infolist_name;
        (void) obj_pointer;

        ptr_infolist = infolist_new (NULL);
        if (!ptr_infolist)
            return NULL;

        ptr_item = infolist_new_item (ptr_infolist);
        if (!ptr_item)
            goto error;

        if (!infolist_new_var_integer (ptr_item, "integer", 123456))
            goto error;
        if (!infolist_new_var_string (ptr_item, "string", "test string"))
            goto error;
        if (!infolist_new_var_pointer (ptr_item, "pointer", (void *)0x123abc))
            goto error;
        if (!infolist_new_var_buffer (ptr_item, "buffer", (void *)buffer,
                                      sizeof (buffer)))
            goto error;
        if (!infolist_new_var_time (ptr_item, "time", 1234567890))
            goto error;

        if (arguments && (strcmp (arguments, "test2") == 0))
        {
            ptr_item = infolist_new_item (ptr_infolist);
            if (!ptr_item)
                goto error;

            if (!infolist_new_var_string (ptr_item, "string2", "test2"))
                goto error;
        }

        return ptr_infolist;

    error:
        infolist_free (ptr_infolist);
        return NULL;
    }

    void setup()
    {
        hook_test_infolist = hook_infolist (
            NULL, "infolist_test", "Test infolist", NULL, "test",
            &test_infolist_cb, NULL, NULL);
    }

    void teardown()
    {
        unhook (hook_test_infolist);
    }
};

/*
 * Tests functions:
 *   infolist_new
 *   infolist_new_item
 *   infolist_new_var_integer
 *   infolist_new_var_string
 *   infolist_new_var_pointer
 *   infolist_new_var_buffer
 *   infolist_new_var_time
 */

TEST(CoreInfolist, New)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    struct t_infolist_var *var_int, *var_str, *var_ptr, *var_buf, *var_time;
    const char buffer[3] = { 12, 34, 56 };

    /* create a new infolist */
    infolist = infolist_new(NULL);
    CHECK(infolist);

    /* check initial infolist values */
    POINTERS_EQUAL(NULL, infolist->plugin);
    POINTERS_EQUAL(NULL, infolist->items);
    POINTERS_EQUAL(NULL, infolist->last_item);
    POINTERS_EQUAL(NULL, infolist->ptr_item);

    /* check that the infolist is the last one in list */
    POINTERS_EQUAL(last_weechat_infolist, infolist);

    /* create a new item in infolist */
    item = infolist_new_item (infolist);
    CHECK(item);

    /* check initial item values */
    POINTERS_EQUAL(NULL, item->vars);
    POINTERS_EQUAL(NULL, item->last_var);
    STRCMP_EQUAL(NULL, item->fields);
    POINTERS_EQUAL(NULL, item->prev_item);
    POINTERS_EQUAL(NULL, item->next_item);

    /* check that item is in infolist */
    POINTERS_EQUAL(item, infolist->items);
    POINTERS_EQUAL(item, infolist->last_item);

    /* add an integer variable */
    var_int = infolist_new_var_integer (item, "test_integer", 123456);
    CHECK(var_int);

    /* check content of variable */
    STRCMP_EQUAL("test_integer", var_int->name);
    LONGS_EQUAL(INFOLIST_INTEGER, var_int->type);
    LONGS_EQUAL(123456, *((int *)var_int->value));
    LONGS_EQUAL(0, var_int->size);
    POINTERS_EQUAL(NULL, var_int->prev_var);
    POINTERS_EQUAL(NULL, var_int->next_var);

    /* check that variable is in item */
    POINTERS_EQUAL(var_int, item->vars);
    POINTERS_EQUAL(var_int, item->last_var);

    /* add a string variable */
    var_str = infolist_new_var_string (item, "test_string", "abc");
    CHECK(var_str);

    /* check content of variable */
    STRCMP_EQUAL("test_string", var_str->name);
    LONGS_EQUAL(INFOLIST_STRING, var_str->type);
    STRCMP_EQUAL("abc", (const char *)var_str->value);
    LONGS_EQUAL(0, var_str->size);
    POINTERS_EQUAL(var_int, var_str->prev_var);
    POINTERS_EQUAL(NULL, var_str->next_var);

    /* check that variable is in item */
    POINTERS_EQUAL(var_int, item->vars);
    POINTERS_EQUAL(var_str, item->last_var);

    /* add a pointer variable */
    var_ptr = infolist_new_var_pointer (item, "test_pointer",
                                        (void *)0x123abc);
    CHECK(var_ptr);

    /* check content of variable */
    STRCMP_EQUAL("test_pointer", var_ptr->name);
    LONGS_EQUAL(INFOLIST_POINTER, var_ptr->type);
    POINTERS_EQUAL(0x123abc, var_ptr->value);
    LONGS_EQUAL(0, var_ptr->size);
    POINTERS_EQUAL(var_str, var_ptr->prev_var);
    POINTERS_EQUAL(NULL, var_ptr->next_var);

    /* check that variable is in item */
    POINTERS_EQUAL(var_int, item->vars);
    POINTERS_EQUAL(var_ptr, item->last_var);

    /* add a buffer variable */
    var_buf = infolist_new_var_buffer (item, "test_buffer", (void *)buffer, 3);
    CHECK(var_buf);

    /* check content of variable */
    STRCMP_EQUAL("test_buffer", var_buf->name);
    LONGS_EQUAL(INFOLIST_BUFFER, var_buf->type);
    LONGS_EQUAL(12, ((char *)var_buf->value)[0]);
    LONGS_EQUAL(34, ((char *)var_buf->value)[1]);
    LONGS_EQUAL(56, ((char *)var_buf->value)[2]);
    LONGS_EQUAL(3, var_buf->size);
    POINTERS_EQUAL(var_ptr, var_buf->prev_var);
    POINTERS_EQUAL(NULL, var_buf->next_var);

    /* check that variable is in item */
    POINTERS_EQUAL(var_int, item->vars);
    POINTERS_EQUAL(var_buf, item->last_var);

    /* add a buffer variable */
    var_time = infolist_new_var_time (item, "test_time", 1234567890);
    CHECK(var_time);

    /* check content of variable */
    STRCMP_EQUAL("test_time", var_time->name);
    LONGS_EQUAL(INFOLIST_TIME, var_time->type);
    LONGS_EQUAL(1234567890, *((time_t *)var_time->value));
    LONGS_EQUAL(0, var_time->size);
    POINTERS_EQUAL(var_buf, var_time->prev_var);
    POINTERS_EQUAL(NULL, var_time->next_var);

    /* check that variable is in item */
    POINTERS_EQUAL(var_int, item->vars);
    POINTERS_EQUAL(var_time, item->last_var);

    infolist_free (infolist);
}

/*
 * Tests functions:
 *   infolist_valid
 *   infolist_free
 */

TEST(CoreInfolist, Valid)
{
    struct t_infolist *infolist;

    LONGS_EQUAL(0, infolist_valid (NULL));
    LONGS_EQUAL(0, infolist_valid ((struct t_infolist *)0x1));

    infolist = infolist_new (NULL);
    LONGS_EQUAL(1, infolist_valid (infolist));

    infolist_free (infolist);

    LONGS_EQUAL(0, infolist_valid (infolist));

    /* test free of NULL infolist */
    infolist_free (NULL);
}

/*
 * Tests functions:
 *   infolist_search_var
 */

TEST(CoreInfolist, Search)
{
    struct t_infolist *infolist;
    struct t_infolist_item *ptr_item;
    struct t_infolist_var *ptr_var;

    infolist = hook_infolist_get (NULL, "infolist_test", NULL, "test2");

    /* move to first item in infolist */
    ptr_item = infolist->items;
    POINTERS_EQUAL(ptr_item, infolist_next (infolist));

    /* search the first variable */
    ptr_var = infolist_search_var (infolist, "integer");
    POINTERS_EQUAL(ptr_item->vars, ptr_var);

    /* search the second variable */
    ptr_var = infolist_search_var (infolist, "string");
    POINTERS_EQUAL(ptr_item->vars->next_var, ptr_var);

    /* search an unknown variable */
    ptr_var = infolist_search_var (infolist, "string2");
    POINTERS_EQUAL(NULL, ptr_var);

    /* move to second item in infolist */
    ptr_item = ptr_item->next_item;
    POINTERS_EQUAL(ptr_item, infolist_next (infolist));

    /* search the first variable */
    ptr_var = infolist_search_var (infolist, "string2");
    POINTERS_EQUAL(ptr_item->vars, ptr_var);

    /* search an unknown variable */
    ptr_var = infolist_search_var (infolist, "string3");
    POINTERS_EQUAL(NULL, ptr_var);

    infolist_free (infolist);
}

/*
 * Tests functions:
 *   infolist_next
 *   infolist_prev
 *   infolist_reset_item_cursor
 */

TEST(CoreInfolist, Move)
{
    struct t_infolist *infolist;

    infolist = hook_infolist_get (NULL, "infolist_test", NULL, "test2");

    POINTERS_EQUAL(NULL, infolist->ptr_item);

    infolist_next (NULL);
    infolist_prev (NULL);
    infolist_reset_item_cursor (NULL);

    /* move to first item in infolist */
    POINTERS_EQUAL(infolist->items, infolist_next (infolist));
    POINTERS_EQUAL(infolist->items, infolist->ptr_item);

    /* reset item cursor */
    infolist_reset_item_cursor (infolist);
    POINTERS_EQUAL(NULL, infolist->ptr_item);

    infolist_next (infolist);

    /* move to second item in infolist */
    POINTERS_EQUAL(infolist->items->next_item, infolist_next (infolist));
    POINTERS_EQUAL(infolist->items->next_item, infolist->ptr_item);

    /* move back to first item in infolist */
    POINTERS_EQUAL(infolist->items, infolist_prev (infolist));
    POINTERS_EQUAL(infolist->items, infolist->ptr_item);

    /* move before first item in infolist */
    POINTERS_EQUAL(NULL, infolist_prev (infolist));
    POINTERS_EQUAL(NULL, infolist->ptr_item);

    /* move after second item in infolist */
    infolist_next (infolist);
    infolist_next (infolist);
    POINTERS_EQUAL(NULL, infolist_next (infolist));
    POINTERS_EQUAL(NULL, infolist->ptr_item);

    infolist_free (infolist);
}

/*
 * Tests functions:
 *   infolist_get
 */

TEST(CoreInfolist, Get)
{
    struct t_infolist *infolist;
    struct t_infolist_var *ptr_var;

    /* get an infolist with one item */
    infolist = hook_infolist_get (NULL, "infolist_test", NULL, NULL);
    CHECK(infolist);

    /* check that there is only one item */
    CHECK(infolist->items);
    POINTERS_EQUAL(NULL, infolist->items->next_item);

    infolist_free (infolist);

    /* get an infolist with two items */
    infolist = hook_infolist_get (NULL, "infolist_test", NULL, "test2");
    CHECK(infolist);

    /* check that there are exactly two items */
    CHECK(infolist->items);
    CHECK(infolist->items->next_item);
    POINTERS_EQUAL(NULL, infolist->items->next_item->next_item);

    /* check variables in first item */
    ptr_var = infolist->items->vars;
    STRCMP_EQUAL("integer", ptr_var->name);
    ptr_var = ptr_var->next_var;
    STRCMP_EQUAL("string", ptr_var->name);
    ptr_var = ptr_var->next_var;
    STRCMP_EQUAL("pointer", ptr_var->name);
    ptr_var = ptr_var->next_var;
    STRCMP_EQUAL("buffer", ptr_var->name);
    ptr_var = ptr_var->next_var;
    STRCMP_EQUAL("time", ptr_var->name);
    LONGS_EQUAL(NULL, ptr_var->next_var);

    /* check variables in second item */
    ptr_var = infolist->items->next_item->vars;
    STRCMP_EQUAL("string2", ptr_var->name);
    LONGS_EQUAL(NULL, ptr_var->next_var);

    infolist_free (infolist);
}

/*
 * Tests functions:
 *   infolist_integer
 *   infolist_string
 *   infolist_pointer
 *   infolist_buffer
 *   infolist_time
 */

TEST(CoreInfolist, GetValues)
{
    struct t_infolist *infolist;
    void *ptr_buffer;
    int size;

    infolist = hook_infolist_get (NULL, "infolist_test", NULL, "test2");
    CHECK(infolist);

    infolist_next (infolist);

    LONGS_EQUAL(123456, infolist_integer (infolist, "integer"));
    STRCMP_EQUAL("test string", infolist_string (infolist, "string"));
    LONGS_EQUAL(0x123abc, infolist_pointer (infolist, "pointer"));
    ptr_buffer = infolist_buffer (infolist, "buffer", &size);
    LONGS_EQUAL(4, size);
    LONGS_EQUAL('a', ((const char *)ptr_buffer)[0]);
    LONGS_EQUAL('b', ((const char *)ptr_buffer)[1]);
    LONGS_EQUAL('c', ((const char *)ptr_buffer)[2]);
    LONGS_EQUAL(0, ((const char *)ptr_buffer)[3]);
    LONGS_EQUAL(1234567890, infolist_time (infolist, "time"));

    infolist_free (infolist);
}

/*
 * Tests functions:
 *   infolist_fields
 */

TEST(CoreInfolist, Fields)
{
    struct t_infolist *infolist;
    const char *fields1 = "i:integer,s:string,p:pointer,b:buffer,t:time";
    const char *fields2 = "s:string2";

    infolist = hook_infolist_get (NULL, "infolist_test", NULL, "test2");
    CHECK(infolist);

    /* check fields in first item */
    infolist_next (infolist);
    LONGS_EQUAL(NULL, infolist->items->fields);
    STRCMP_EQUAL(fields1, infolist_fields (infolist));
    STRCMP_EQUAL(fields1, infolist->items->fields);

    /* check fields in second item */
    infolist_next (infolist);
    LONGS_EQUAL(NULL, infolist->items->next_item->fields);
    STRCMP_EQUAL(fields2, infolist_fields (infolist));
    STRCMP_EQUAL(fields2, infolist->items->next_item->fields);

    infolist_free (infolist);
}

/*
 * Tests functions:
 *   infolist_print_log
 */

TEST(CoreInfolist, PrintLog)
{
    /* TODO: write tests */
}
