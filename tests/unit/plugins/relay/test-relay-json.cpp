/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test JSON functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/relay-json.h"
}

/* parse JSON and check that printed JSON is "__result" */
#define WEE_CHECK_PARSE(__result, __json)                               \
    json = relay_json_parse (__json);                                   \
    CHECK(json);                                                        \
    WEE_TEST_STR(__result, relay_json_print (json));                    \
    relay_json_free (json);

/* check that parse of JSON fails */
#define WEE_CHECK_PARSE_ERROR(__json)                                   \
    POINTERS_EQUAL(NULL, relay_json_parse (__json));

TEST_GROUP(RelayJson)
{
};

/*
 * Test functions:
 *   relay_json_new_null
 *   relay_json_new_bool
 *   relay_json_new_number
 *   relay_json_new_string
 *   relay_json_new_array
 *   relay_json_new_object
 *   relay_json_is_null
 *   relay_json_is_bool
 *   relay_json_is_true
 *   relay_json_is_number
 *   relay_json_is_string
 *   relay_json_is_array
 *   relay_json_is_object
 *   relay_json_free
 */

TEST(RelayJson, NewIsFree)
{
    struct t_relay_json *json;
    char *str;

    LONGS_EQUAL(0, relay_json_is_null (NULL));
    LONGS_EQUAL(0, relay_json_is_bool (NULL));
    LONGS_EQUAL(0, relay_json_is_true (NULL));
    LONGS_EQUAL(0, relay_json_is_number (NULL));
    LONGS_EQUAL(0, relay_json_is_string (NULL));
    LONGS_EQUAL(0, relay_json_is_array (NULL));
    LONGS_EQUAL(0, relay_json_is_object (NULL));

    relay_json_free (NULL);

    json = relay_json_new_null ();
    CHECK(json);
    LONGS_EQUAL(1, relay_json_is_null (json));
    LONGS_EQUAL(0, relay_json_is_bool (json));
    WEE_TEST_STR("null", relay_json_print (json));
    relay_json_free (json);

    json = relay_json_new_bool (0);
    CHECK(json);
    LONGS_EQUAL(1, relay_json_is_bool (json));
    LONGS_EQUAL(0, relay_json_is_true (json));
    WEE_TEST_STR("false", relay_json_print (json));
    relay_json_free (json);

    json = relay_json_new_bool (42);
    CHECK(json);
    LONGS_EQUAL(1, relay_json_is_bool (json));
    LONGS_EQUAL(1, relay_json_is_true (json));
    WEE_TEST_STR("true", relay_json_print (json));
    relay_json_free (json);

    json = relay_json_new_number (-123);
    CHECK(json);
    LONGS_EQUAL(1, relay_json_is_number (json));
    LONGS_EQUAL(0, relay_json_is_string (json));
    WEE_TEST_STR("-123", relay_json_print (json));
    relay_json_free (json);

    /* integers must never be printed with exponent (like 1.7e+15) */
    json = relay_json_new_number (1709932823238640LL);
    WEE_TEST_STR("1709932823238640", relay_json_print (json));
    relay_json_free (json);

    json = relay_json_new_number (1000000000000000000LL);
    WEE_TEST_STR("1000000000000000000", relay_json_print (json));
    relay_json_free (json);

    json = relay_json_new_number (-100000000000000000LL);
    WEE_TEST_STR("-100000000000000000", relay_json_print (json));
    relay_json_free (json);

    json = relay_json_new_number (LLONG_MAX);
    WEE_TEST_STR("9223372036854775807", relay_json_print (json));
    relay_json_free (json);

    json = relay_json_new_number (LLONG_MIN);
    WEE_TEST_STR("-9223372036854775808", relay_json_print (json));
    relay_json_free (json);

    POINTERS_EQUAL(NULL, relay_json_new_string (NULL));

    json = relay_json_new_string ("");
    CHECK(json);
    LONGS_EQUAL(1, relay_json_is_string (json));
    WEE_TEST_STR("\"\"", relay_json_print (json));
    relay_json_free (json);

    json = relay_json_new_string ("a\"b\\c/d\b\f\n\r\t\x01\x1f\x7f é");
    WEE_TEST_STR("\"a\\\"b\\\\c/d\\b\\f\\n\\r\\t\\u0001\\u001f\x7f é\"",
                 relay_json_print (json));
    relay_json_free (json);

    /* invalid UTF-8 is replaced by "?" */
    json = relay_json_new_string ("a\xff" "b\xc3");
    WEE_TEST_STR("\"a?b?\"", relay_json_print (json));
    relay_json_free (json);
    json = relay_json_new_string ("\xc0\xaf|\xed\xa0\x80|\xf0\x9f\x98|é😀");
    WEE_TEST_STR("\"??|???|???|é😀\"", relay_json_print (json));
    relay_json_free (json);
    json = relay_json_new_object ();
    relay_json_object_add (json, "k\xe9y", relay_json_new_string ("\x80"));
    str = relay_json_print (json);
    STRCMP_EQUAL("{\"k?y\":\"?\"}", str);
    relay_json_free (json);
    json = relay_json_parse (str);
    CHECK(json);
    STRCMP_EQUAL("?", relay_json_get_string (relay_json_object_get (json, "k?y")));
    relay_json_free (json);
    free (str);

    json = relay_json_new_array ();
    CHECK(json);
    LONGS_EQUAL(1, relay_json_is_array (json));
    LONGS_EQUAL(0, relay_json_is_object (json));
    WEE_TEST_STR("[]", relay_json_print (json));
    relay_json_free (json);

    json = relay_json_new_object ();
    CHECK(json);
    LONGS_EQUAL(1, relay_json_is_object (json));
    LONGS_EQUAL(0, relay_json_is_array (json));
    WEE_TEST_STR("{}", relay_json_print (json));
    relay_json_free (json);
}

/*
 * Test functions:
 *   relay_json_array_add
 *   relay_json_object_add
 *   relay_json_array_size
 *   relay_json_array_get
 *   relay_json_object_get
 *   relay_json_object_detach
 */

TEST(RelayJson, AddGetDetach)
{
    struct t_relay_json *json, *array, *item, *item2;
    char *str;

    json = relay_json_new_object ();
    array = relay_json_new_array ();

    /* invalid arguments */
    LONGS_EQUAL(0, relay_json_array_add (NULL, NULL));
    LONGS_EQUAL(0, relay_json_array_add (array, NULL));
    LONGS_EQUAL(0, relay_json_array_add (array, array));
    LONGS_EQUAL(0, relay_json_object_add (NULL, "a", NULL));
    LONGS_EQUAL(0, relay_json_object_add (json, "a", NULL));
    LONGS_EQUAL(0, relay_json_object_add (json, "a", json));
    item = relay_json_new_null ();
    LONGS_EQUAL(0, relay_json_object_add (json, NULL, item));
    LONGS_EQUAL(0, relay_json_object_add (array, "a", item));
    LONGS_EQUAL(0, relay_json_array_add (json, item));
    relay_json_free (item);

    LONGS_EQUAL(1, relay_json_array_add (array, relay_json_new_number (1)));
    LONGS_EQUAL(1, relay_json_array_add (array, relay_json_new_string ("two")));
    LONGS_EQUAL(1, relay_json_array_add (array, relay_json_new_bool (1)));

    LONGS_EQUAL(1, relay_json_object_add (json, "id", relay_json_new_number (1)));
    LONGS_EQUAL(1, relay_json_object_add (json, "ID", relay_json_new_number (2)));
    LONGS_EQUAL(1, relay_json_object_add (json, "id", relay_json_new_number (3)));
    LONGS_EQUAL(1, relay_json_object_add (json, "list", array));
    LONGS_EQUAL(1, relay_json_object_add (json, "empty", relay_json_new_object ()));

    WEE_TEST_STR("{\"id\":1,\"ID\":2,\"id\":3,\"list\":[1,\"two\",true],\"empty\":{}}",
                 relay_json_print (json));

    /* size */
    LONGS_EQUAL(0, relay_json_array_size (NULL));
    LONGS_EQUAL(0, relay_json_array_size (relay_json_object_get (json, "id")));
    LONGS_EQUAL(5, relay_json_array_size (json));
    LONGS_EQUAL(3, relay_json_array_size (array));

    /* get by index */
    POINTERS_EQUAL(NULL, relay_json_array_get (NULL, 0));
    POINTERS_EQUAL(NULL, relay_json_array_get (array, -1));
    POINTERS_EQUAL(NULL, relay_json_array_get (array, 3));
    POINTERS_EQUAL(NULL, relay_json_array_get (relay_json_array_get (array, 0), 0));
    STRCMP_EQUAL("two", relay_json_get_string (relay_json_array_get (array, 1)));
    LONGS_EQUAL(1, relay_json_is_true (relay_json_array_get (array, 2)));
    POINTERS_EQUAL(array, relay_json_array_get (json, 3));

    /* get by key (case-sensitive, first wins) */
    POINTERS_EQUAL(NULL, relay_json_object_get (NULL, "id"));
    POINTERS_EQUAL(NULL, relay_json_object_get (json, NULL));
    POINTERS_EQUAL(NULL, relay_json_object_get (json, "xxx"));
    POINTERS_EQUAL(NULL, relay_json_object_get (json, "Id"));
    POINTERS_EQUAL(NULL, relay_json_object_get (array, "id"));
    POINTERS_EQUAL(relay_json_array_get (json, 0),
                   relay_json_object_get (json, "id"));
    POINTERS_EQUAL(relay_json_array_get (json, 1),
                   relay_json_object_get (json, "ID"));
    POINTERS_EQUAL(array, relay_json_object_get (json, "list"));

    /* detach */
    POINTERS_EQUAL(NULL, relay_json_object_detach (NULL, "id"));
    POINTERS_EQUAL(NULL, relay_json_object_detach (json, "xxx"));
    item = relay_json_object_detach (json, "list");
    POINTERS_EQUAL(array, item);
    POINTERS_EQUAL(NULL, item->prev);
    POINTERS_EQUAL(NULL, item->next);
    LONGS_EQUAL(4, relay_json_array_size (json));
    WEE_TEST_STR("{\"id\":1,\"ID\":2,\"id\":3,\"empty\":{}}",
                 relay_json_print (json));
    item2 = relay_json_object_detach (json, "empty");
    WEE_TEST_STR("{\"id\":1,\"ID\":2,\"id\":3}", relay_json_print (json));
    relay_json_free (item2);
    item2 = relay_json_object_detach (json, "id");
    WEE_TEST_STR("{\"ID\":2,\"id\":3}", relay_json_print (json));
    relay_json_free (item2);
    relay_json_free (relay_json_object_detach (json, "id"));
    relay_json_free (relay_json_object_detach (json, "ID"));
    LONGS_EQUAL(0, relay_json_array_size (json));
    WEE_TEST_STR("{}", relay_json_print (json));

    /* re-add detached item with a new key */
    LONGS_EQUAL(1, relay_json_object_add (json, "new", item));
    WEE_TEST_STR("{\"new\":[1,\"two\",true]}", relay_json_print (json));

    relay_json_free (json);
}

/*
 * Test functions:
 *   relay_json_get_number
 *   relay_json_get_string
 */

TEST(RelayJson, GetValue)
{
    struct t_relay_json *json;
    long long number;

    number = 42;
    LONGS_EQUAL(0, relay_json_get_number (NULL, &number));
    POINTERS_EQUAL(NULL, relay_json_get_string (NULL));

    json = relay_json_new_string ("123");
    LONGS_EQUAL(0, relay_json_get_number (json, &number));
    LONGS_EQUAL(42, number);
    STRCMP_EQUAL("123", relay_json_get_string (json));
    relay_json_free (json);

    json = relay_json_new_number (1709932823238637LL);
    POINTERS_EQUAL(NULL, relay_json_get_string (json));
    LONGS_EQUAL(1, relay_json_get_number (json, NULL));
    LONGS_EQUAL(1, relay_json_get_number (json, &number));
    CHECK(number == 1709932823238637LL);
    relay_json_free (json);

    /* non-integer number: kept as raw string, not returned */
    number = 42;
    json = relay_json_parse ("1.5");
    LONGS_EQUAL(1, relay_json_is_number (json));
    LONGS_EQUAL(0, relay_json_get_number (json, &number));
    LONGS_EQUAL(42, number);
    POINTERS_EQUAL(NULL, relay_json_get_string (json));
    relay_json_free (json);
}

/*
 * Test functions:
 *   relay_json_parse (literals, numbers)
 */

TEST(RelayJson, ParseScalar)
{
    struct t_relay_json *json;
    long long number;
    char *str;

    WEE_CHECK_PARSE_ERROR(NULL);
    WEE_CHECK_PARSE_ERROR("");
    WEE_CHECK_PARSE_ERROR("   ");
    WEE_CHECK_PARSE_ERROR("tru");
    WEE_CHECK_PARSE_ERROR("True");
    WEE_CHECK_PARSE_ERROR("nul");
    WEE_CHECK_PARSE_ERROR("NULL");
    WEE_CHECK_PARSE_ERROR("fals");
    WEE_CHECK_PARSE_ERROR("truex");
    WEE_CHECK_PARSE_ERROR("null null");
    WEE_CHECK_PARSE_ERROR("undefined");

    WEE_CHECK_PARSE("null", "null");

    /* UTF-8 BOM (allowed only at beginning) */
    WEE_CHECK_PARSE_ERROR("\xEF\xBB\xBF");
    WEE_CHECK_PARSE_ERROR("\xEF\xBB");
    WEE_CHECK_PARSE_ERROR(" \xEF\xBB\xBFnull");
    WEE_CHECK_PARSE_ERROR("[\xEF\xBB\xBF" "1]");
    WEE_CHECK_PARSE("null", "\xEF\xBB\xBFnull");
    WEE_CHECK_PARSE("{\"a\":1}", "\xEF\xBB\xBF {\"a\":1} ");
    WEE_CHECK_PARSE("true", " \t\r\ntrue \t\r\n");
    WEE_CHECK_PARSE("false", "false");

    /* invalid numbers */
    WEE_CHECK_PARSE_ERROR("-");
    WEE_CHECK_PARSE_ERROR("+1");
    WEE_CHECK_PARSE_ERROR("01");
    WEE_CHECK_PARSE_ERROR("-01");
    WEE_CHECK_PARSE_ERROR("1.");
    WEE_CHECK_PARSE_ERROR(".5");
    WEE_CHECK_PARSE_ERROR("1e");
    WEE_CHECK_PARSE_ERROR("1e+");
    WEE_CHECK_PARSE_ERROR("1.e3");
    WEE_CHECK_PARSE_ERROR("0x10");
    WEE_CHECK_PARSE_ERROR("1 2");
    WEE_CHECK_PARSE_ERROR("- 1");
    WEE_CHECK_PARSE_ERROR("NaN");
    WEE_CHECK_PARSE_ERROR("Infinity");
    WEE_CHECK_PARSE_ERROR("-Infinity");

    /* integer numbers */
    WEE_CHECK_PARSE("0", "0");
    WEE_CHECK_PARSE("0", "-0");
    WEE_CHECK_PARSE("123", " 123 ");
    WEE_CHECK_PARSE("-123", "-123");
    WEE_CHECK_PARSE("1709932823238640", "1709932823238640");
    WEE_CHECK_PARSE("1000000000000000000", "1000000000000000000");
    WEE_CHECK_PARSE("9223372036854775807", "9223372036854775807");
    WEE_CHECK_PARSE("-9223372036854775808", "-9223372036854775808");

    json = relay_json_parse ("1709932823238637");
    LONGS_EQUAL(1, relay_json_get_number (json, &number));
    CHECK(number == 1709932823238637LL);
    relay_json_free (json);

    json = relay_json_parse ("9223372036854775807");
    LONGS_EQUAL(1, relay_json_get_number (json, &number));
    CHECK(number == LLONG_MAX);
    relay_json_free (json);

    json = relay_json_parse ("-9223372036854775808");
    LONGS_EQUAL(1, relay_json_get_number (json, &number));
    CHECK(number == LLONG_MIN);
    relay_json_free (json);

    /* numbers kept as raw string (overflow, fraction, exponent) */
    WEE_CHECK_PARSE("9223372036854775808", "9223372036854775808");
    WEE_CHECK_PARSE("-9223372036854775809", "-9223372036854775809");
    WEE_CHECK_PARSE("123456789012345678901234567890",
                    "123456789012345678901234567890");
    WEE_CHECK_PARSE("1.5", "1.5");
    WEE_CHECK_PARSE("-0.0", "-0.0");
    WEE_CHECK_PARSE("1e3", "1e3");
    WEE_CHECK_PARSE("1E+3", "1E+3");
    WEE_CHECK_PARSE("-1.25e-10", "-1.25e-10");

    json = relay_json_parse ("9223372036854775808");
    LONGS_EQUAL(1, relay_json_is_number (json));
    LONGS_EQUAL(0, relay_json_get_number (json, &number));
    relay_json_free (json);

    json = relay_json_parse ("1e3");
    LONGS_EQUAL(1, relay_json_is_number (json));
    LONGS_EQUAL(0, relay_json_get_number (json, &number));
    relay_json_free (json);
}

/*
 * Test functions:
 *   relay_json_parse (strings)
 */

TEST(RelayJson, ParseString)
{
    struct t_relay_json *json;
    char *str;

    WEE_CHECK_PARSE_ERROR("\"");
    WEE_CHECK_PARSE_ERROR("\"abc");
    WEE_CHECK_PARSE_ERROR("\"abc\\\"");
    WEE_CHECK_PARSE_ERROR("\"\\");
    WEE_CHECK_PARSE_ERROR("'abc'");
    WEE_CHECK_PARSE_ERROR("\"a\nb\"");
    WEE_CHECK_PARSE_ERROR("\"a\tb\"");
    WEE_CHECK_PARSE_ERROR("\"a\x01" "b\"");
    WEE_CHECK_PARSE_ERROR("\"\\x\"");
    WEE_CHECK_PARSE_ERROR("\"\\a\"");
    WEE_CHECK_PARSE_ERROR("\"\\u\"");
    WEE_CHECK_PARSE_ERROR("\"\\u12\"");
    WEE_CHECK_PARSE_ERROR("\"\\u123\"");
    WEE_CHECK_PARSE_ERROR("\"\\u12g4\"");
    WEE_CHECK_PARSE_ERROR("\"\\u0000\"");
    WEE_CHECK_PARSE_ERROR("\"abc\"x");

    /* lone or invalid surrogates */
    WEE_CHECK_PARSE_ERROR("\"\\ud83d\"");
    WEE_CHECK_PARSE_ERROR("\"\\ud83dx\"");
    WEE_CHECK_PARSE_ERROR("\"\\ud83d\\n\"");
    WEE_CHECK_PARSE_ERROR("\"\\ud83d\\u0041\"");
    WEE_CHECK_PARSE_ERROR("\"\\ud83d\\ud83d\"");
    WEE_CHECK_PARSE_ERROR("\"\\ude00\"");

    /* invalid UTF-8 */
    WEE_CHECK_PARSE_ERROR("\"\xe9\"");
    WEE_CHECK_PARSE_ERROR("\"a\xc3\"");
    WEE_CHECK_PARSE_ERROR("\"\xc0\xaf\"");
    WEE_CHECK_PARSE_ERROR("\"\xed\xa0\x80\"");
    WEE_CHECK_PARSE_ERROR("\"\xff\"");

    WEE_CHECK_PARSE("\"\"", "\"\"");
    WEE_CHECK_PARSE("\"abc\"", "  \"abc\"  ");
    WEE_CHECK_PARSE("\"a\\\"b\\\\c/d\\b\\f\\n\\r\\t\"",
                    "\"a\\\"b\\\\c\\/d\\b\\f\\n\\r\\t\"");
    WEE_CHECK_PARSE("\"\\u0001\\u001f\"", "\"\\u0001\\u001F\"");
    WEE_CHECK_PARSE("\"A\"", "\"\\u0041\"");
    WEE_CHECK_PARSE("\"é\"", "\"\\u00e9\"");
    WEE_CHECK_PARSE("\"é\"", "\"é\"");
    WEE_CHECK_PARSE("\"€\"", "\"\\u20AC\"");
    WEE_CHECK_PARSE("\"\xef\xbf\xbf\"", "\"\\uffff\"");
    WEE_CHECK_PARSE("\"😀\"", "\"\\ud83d\\ude00\"");
    WEE_CHECK_PARSE("\"😀\"", "\"😀\"");
    WEE_CHECK_PARSE("\"\xf4\x8f\xbf\xbf\"", "\"\\udbff\\udfff\"");
    WEE_CHECK_PARSE("\"a😀b€c\"", "\"a\\uD83D\\uDE00b\\u20acc\"");

    json = relay_json_parse ("\"\\ud83d\\ude00\"");
    STRCMP_EQUAL("\xf0\x9f\x98\x80", relay_json_get_string (json));
    relay_json_free (json);
}

/*
 * Test functions:
 *   relay_json_parse (arrays, objects)
 */

TEST(RelayJson, ParseArrayObject)
{
    struct t_relay_json *json, *ptr_item;
    char str_json[4096], *str;
    int i, count;

    /* invalid arrays */
    WEE_CHECK_PARSE_ERROR("[");
    WEE_CHECK_PARSE_ERROR("]");
    WEE_CHECK_PARSE_ERROR("[1");
    WEE_CHECK_PARSE_ERROR("[1,");
    WEE_CHECK_PARSE_ERROR("[1,]");
    WEE_CHECK_PARSE_ERROR("[,1]");
    WEE_CHECK_PARSE_ERROR("[,]");
    WEE_CHECK_PARSE_ERROR("[1 2]");
    WEE_CHECK_PARSE_ERROR("[1:2]");
    WEE_CHECK_PARSE_ERROR("[1]]");
    WEE_CHECK_PARSE_ERROR("[[1]");

    /* invalid objects */
    WEE_CHECK_PARSE_ERROR("{");
    WEE_CHECK_PARSE_ERROR("}");
    WEE_CHECK_PARSE_ERROR("{\"a\"}");
    WEE_CHECK_PARSE_ERROR("{\"a\":}");
    WEE_CHECK_PARSE_ERROR("{\"a\" 1}");
    WEE_CHECK_PARSE_ERROR("{\"a\":1,}");
    WEE_CHECK_PARSE_ERROR("{,\"a\":1}");
    WEE_CHECK_PARSE_ERROR("{\"a\":1 \"b\":2}");
    WEE_CHECK_PARSE_ERROR("{a:1}");
    WEE_CHECK_PARSE_ERROR("{'a':1}");
    WEE_CHECK_PARSE_ERROR("{1:1}");
    WEE_CHECK_PARSE_ERROR("{\"a\":1}}");
    WEE_CHECK_PARSE_ERROR("{\"\\u0000\":1}");
    WEE_CHECK_PARSE_ERROR("{\"a\":1} // comment");
    WEE_CHECK_PARSE_ERROR("/* comment */ {\"a\":1}");

    WEE_CHECK_PARSE("[]", "[]");
    WEE_CHECK_PARSE("[]", " [ \n ] ");
    WEE_CHECK_PARSE("{}", "{}");
    WEE_CHECK_PARSE("{}", " { \t } ");
    WEE_CHECK_PARSE("[1,\"a\",true,false,null,[],{},1.5]",
                    " [ 1 , \"a\" , true , false , null , [ ] , { } , 1.5 ] ");
    WEE_CHECK_PARSE("{\"a\":1,\"b\":[1,{\"c\":null}],\"\":\"\",\"a\":2}",
                    "{ \"a\" : 1 , \"b\" : [ 1 , { \"c\" : null } ] , "
                    "\"\" : \"\" , \"a\" : 2 }");
    WEE_CHECK_PARSE("{\"\\\"key\\n\":\"é\"}", "{\"\\\"key\\n\":\"\\u00e9\"}");

    /* real message from api relay */
    WEE_CHECK_PARSE(
        "[{\"request\":\"POST /api/input\","
        "\"body\":{\"buffer_name\":\"irc.libera.#weechat\","
        "\"command\":\"hello\"}},"
        "{\"request\":\"GET /api/buffers\",\"request_id\":null}]",
        "[\n"
        "  {\n"
        "    \"request\": \"POST /api/input\",\n"
        "    \"body\": {\n"
        "      \"buffer_name\": \"irc.libera.#weechat\",\n"
        "      \"command\": \"hello\"\n"
        "    }\n"
        "  },\n"
        "  {\n"
        "    \"request\": \"GET /api/buffers\",\n"
        "    \"request_id\": null\n"
        "  }\n"
        "]\n");

    /* iterate on children */
    json = relay_json_parse ("{\"a\":1,\"b\":2,\"c\":3}");
    count = 0;
    RELAY_JSON_FOREACH(ptr_item, json)
    {
        LONGS_EQUAL(1, relay_json_is_number (ptr_item));
        CHECK(ptr_item->key);
        count++;
    }
    LONGS_EQUAL(3, count);
    STRCMP_EQUAL("c", json->last_child->key);
    relay_json_free (json);

    count = 0;
    RELAY_JSON_FOREACH(ptr_item, (struct t_relay_json *)NULL)
    {
        count++;
    }
    LONGS_EQUAL(0, count);

    /* max depth */
    for (i = 0; i < RELAY_JSON_MAX_DEPTH; i++)
    {
        str_json[i] = '[';
        str_json[RELAY_JSON_MAX_DEPTH + i] = ']';
    }
    str_json[RELAY_JSON_MAX_DEPTH * 2] = '\0';
    json = relay_json_parse (str_json);
    CHECK(json);
    str = relay_json_print (json);
    STRCMP_EQUAL(str_json, str);
    free (str);
    relay_json_free (json);

    /* max depth + 1 */
    for (i = 0; i < RELAY_JSON_MAX_DEPTH + 1; i++)
    {
        str_json[i] = '[';
        str_json[RELAY_JSON_MAX_DEPTH + 1 + i] = ']';
    }
    str_json[(RELAY_JSON_MAX_DEPTH + 1) * 2] = '\0';
    WEE_CHECK_PARSE_ERROR(str_json);

    /* same with objects */
    str_json[0] = '\0';
    for (i = 0; i < RELAY_JSON_MAX_DEPTH + 1; i++)
        strcat (str_json, "{\"a\":");
    strcat (str_json, "1");
    for (i = 0; i < RELAY_JSON_MAX_DEPTH + 1; i++)
        strcat (str_json, "}");
    WEE_CHECK_PARSE_ERROR(str_json);
}

/*
 * Test functions:
 *   relay_json_print
 */

TEST(RelayJson, Print)
{
    struct t_relay_json *json;
    char *str, *str2;
    int i;

    POINTERS_EQUAL(NULL, relay_json_print (NULL));

    /* long string, with many escapes (buffer reallocation) */
    json = relay_json_new_array ();
    for (i = 0; i < 1000; i++)
    {
        relay_json_array_add (json,
                              relay_json_new_string ("line \"quoted\"\n"));
    }
    str = relay_json_print (json);
    CHECK(str);
    relay_json_free (json);
    json = relay_json_parse (str);
    CHECK(json);
    LONGS_EQUAL(1000, relay_json_array_size (json));
    STRCMP_EQUAL("line \"quoted\"\n",
                 relay_json_get_string (relay_json_array_get (json, 999)));
    str2 = relay_json_print (json);
    STRCMP_EQUAL(str, str2);
    free (str);
    free (str2);
    relay_json_free (json);
}
