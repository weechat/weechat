/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* JSON parser and writer for relay (RFC 8259) */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "relay.h"
#include "relay-json.h"


struct t_relay_json_parser
{
    const char *ptr;                   /* current position in input         */
    int depth;                         /* current nesting of arrays/objects */
};

struct t_relay_json_buffer
{
    char *data;                        /* output string                     */
    size_t length;                     /* length of output (without NUL)    */
    size_t size_alloc;                 /* size allocated for data           */
};

struct t_relay_json *relay_json_parse_value (struct t_relay_json_parser *parser);
int relay_json_print_value (struct t_relay_json_buffer *buffer,
                            struct t_relay_json *json);


/*
 * Create a new JSON item with given type.
 *
 * Note: result must be freed after use with function relay_json_free.
 */

struct t_relay_json *
relay_json_new (enum t_relay_json_type type)
{
    struct t_relay_json *new_json;

    new_json = calloc (1, sizeof (*new_json));
    if (!new_json)
        return NULL;

    new_json->type = type;

    return new_json;
}

/*
 * Create a new JSON null.
 *
 * Note: result must be freed after use with function relay_json_free.
 */

struct t_relay_json *
relay_json_new_null (void)
{
    return relay_json_new (RELAY_JSON_NULL);
}

/*
 * Create a new JSON boolean.
 *
 * Note: result must be freed after use with function relay_json_free.
 */

struct t_relay_json *
relay_json_new_bool (int value)
{
    return relay_json_new ((value) ? RELAY_JSON_TRUE : RELAY_JSON_FALSE);
}

/*
 * Create a new JSON number.
 *
 * Note: result must be freed after use with function relay_json_free.
 */

struct t_relay_json *
relay_json_new_number (long long value)
{
    struct t_relay_json *new_json;

    new_json = relay_json_new (RELAY_JSON_NUMBER);
    if (!new_json)
        return NULL;

    new_json->integer = value;
    new_json->is_integer = 1;

    return new_json;
}

/*
 * Create a new JSON string.
 *
 * Return NULL if string is NULL.
 *
 * Note: result must be freed after use with function relay_json_free.
 */

struct t_relay_json *
relay_json_new_string (const char *string)
{
    struct t_relay_json *new_json;

    if (!string)
        return NULL;

    new_json = relay_json_new (RELAY_JSON_STRING);
    if (!new_json)
        return NULL;

    new_json->string = strdup (string);
    if (!new_json->string)
    {
        free (new_json);
        return NULL;
    }

    return new_json;
}

/*
 * Create a new JSON array (empty).
 *
 * Note: result must be freed after use with function relay_json_free.
 */

struct t_relay_json *
relay_json_new_array (void)
{
    return relay_json_new (RELAY_JSON_ARRAY);
}

/*
 * Create a new JSON object (empty).
 *
 * Note: result must be freed after use with function relay_json_free.
 */

struct t_relay_json *
relay_json_new_object (void)
{
    return relay_json_new (RELAY_JSON_OBJECT);
}

/*
 * Add an item at the end of children of an array or object.
 */

void
relay_json_append_child (struct t_relay_json *parent,
                         struct t_relay_json *item)
{
    item->prev = parent->last_child;
    item->next = NULL;
    if (parent->last_child)
        parent->last_child->next = item;
    else
        parent->child = item;
    parent->last_child = item;
    parent->size++;
}

/*
 * Add an item at the end of an array.
 *
 * The item is then owned by the array: it is freed with the array.
 *
 * Return:
 *   1: OK
 *   0: error (item is not added and not freed)
 */

int
relay_json_array_add (struct t_relay_json *array, struct t_relay_json *item)
{
    if (!array || !item || (array->type != RELAY_JSON_ARRAY) || (array == item))
        return 0;

    free (item->key);
    item->key = NULL;

    relay_json_append_child (array, item);

    return 1;
}

/*
 * Add an item at the end of an object, with the given key.
 *
 * The item is then owned by the object: it is freed with the object.
 *
 * Return:
 *   1: OK
 *   0: error (item is not added and not freed)
 */

int
relay_json_object_add (struct t_relay_json *object, const char *key,
                       struct t_relay_json *item)
{
    char *new_key;

    if (!object || !key || !item || (object->type != RELAY_JSON_OBJECT)
        || (object == item))
    {
        return 0;
    }

    new_key = strdup (key);
    if (!new_key)
        return 0;

    free (item->key);
    item->key = new_key;

    relay_json_append_child (object, item);

    return 1;
}

/*
 * Check if a JSON item is null.
 *
 * Return:
 *   1: item is null
 *   0: item is not null (or item is NULL pointer)
 */

int
relay_json_is_null (struct t_relay_json *json)
{
    return (json && (json->type == RELAY_JSON_NULL)) ? 1 : 0;
}

/*
 * Check if a JSON item is a boolean (true or false).
 *
 * Return:
 *   1: item is a boolean
 *   0: item is not a boolean
 */

int
relay_json_is_bool (struct t_relay_json *json)
{
    return (json
            && ((json->type == RELAY_JSON_TRUE)
                || (json->type == RELAY_JSON_FALSE))) ? 1 : 0;
}

/*
 * Check if a JSON item is the boolean "true".
 *
 * Return:
 *   1: item is true
 *   0: item is not true
 */

int
relay_json_is_true (struct t_relay_json *json)
{
    return (json && (json->type == RELAY_JSON_TRUE)) ? 1 : 0;
}

/*
 * Check if a JSON item is a number.
 *
 * Return:
 *   1: item is a number
 *   0: item is not a number
 */

int
relay_json_is_number (struct t_relay_json *json)
{
    return (json && (json->type == RELAY_JSON_NUMBER)) ? 1 : 0;
}

/*
 * Check if a JSON item is a string.
 *
 * Return:
 *   1: item is a string
 *   0: item is not a string
 */

int
relay_json_is_string (struct t_relay_json *json)
{
    return (json && (json->type == RELAY_JSON_STRING)) ? 1 : 0;
}

/*
 * Check if a JSON item is an array.
 *
 * Return:
 *   1: item is an array
 *   0: item is not an array
 */

int
relay_json_is_array (struct t_relay_json *json)
{
    return (json && (json->type == RELAY_JSON_ARRAY)) ? 1 : 0;
}

/*
 * Check if a JSON item is an object.
 *
 * Return:
 *   1: item is an object
 *   0: item is not an object
 */

int
relay_json_is_object (struct t_relay_json *json)
{
    return (json && (json->type == RELAY_JSON_OBJECT)) ? 1 : 0;
}

/*
 * Get value of a JSON number.
 *
 * Only integer numbers that fit in a long long are returned: a number with
 * a fraction or an exponent (like 1.5 or 1e3) is not returned.
 *
 * Return:
 *   1: OK, *value is set
 *   0: item is not an integer number, *value is not changed
 */

int
relay_json_get_number (struct t_relay_json *json, long long *value)
{
    if (!json || (json->type != RELAY_JSON_NUMBER) || !json->is_integer)
        return 0;

    if (value)
        *value = json->integer;

    return 1;
}

/*
 * Get value of a JSON string.
 *
 * Return NULL if item is not a string.
 */

const char *
relay_json_get_string (struct t_relay_json *json)
{
    return (json && (json->type == RELAY_JSON_STRING)) ? json->string : NULL;
}

/*
 * Get number of items in an array or object.
 *
 * Return 0 if item is not an array or object.
 */

int
relay_json_array_size (struct t_relay_json *json)
{
    if (!json
        || ((json->type != RELAY_JSON_ARRAY)
            && (json->type != RELAY_JSON_OBJECT)))
    {
        return 0;
    }

    return json->size;
}

/*
 * Get an item by index (first is 0) in an array or object.
 *
 * Return NULL if not found.
 */

struct t_relay_json *
relay_json_array_get (struct t_relay_json *json, int index)
{
    struct t_relay_json *ptr_item;

    if (!json || (index < 0)
        || ((json->type != RELAY_JSON_ARRAY)
            && (json->type != RELAY_JSON_OBJECT)))
    {
        return NULL;
    }

    for (ptr_item = json->child; ptr_item && (index > 0);
         ptr_item = ptr_item->next)
    {
        index--;
    }

    return ptr_item;
}

/*
 * Get an item by key in an object (key is case-sensitive).
 *
 * If the key is duplicated, the first item is returned.
 *
 * Return NULL if not found.
 */

struct t_relay_json *
relay_json_object_get (struct t_relay_json *json, const char *key)
{
    struct t_relay_json *ptr_item;

    if (!json || !key || (json->type != RELAY_JSON_OBJECT))
        return NULL;

    for (ptr_item = json->child; ptr_item; ptr_item = ptr_item->next)
    {
        if (ptr_item->key && (strcmp (ptr_item->key, key) == 0))
            return ptr_item;
    }

    return NULL;
}

/*
 * Detach an item by key from an object (key is case-sensitive).
 *
 * The item is removed from object but not freed.
 *
 * Return the detached item, NULL if not found.
 *
 * Note: result must be freed after use with function relay_json_free.
 */

struct t_relay_json *
relay_json_object_detach (struct t_relay_json *json, const char *key)
{
    struct t_relay_json *ptr_item;

    ptr_item = relay_json_object_get (json, key);
    if (!ptr_item)
        return NULL;

    if (ptr_item->prev)
        ptr_item->prev->next = ptr_item->next;
    else
        json->child = ptr_item->next;
    if (ptr_item->next)
        ptr_item->next->prev = ptr_item->prev;
    else
        json->last_child = ptr_item->prev;
    json->size--;

    ptr_item->prev = NULL;
    ptr_item->next = NULL;

    return ptr_item;
}

/*
 * Skip whitespace in parser input.
 */

void
relay_json_parse_skip_whitespace (struct t_relay_json_parser *parser)
{
    while ((parser->ptr[0] == ' ') || (parser->ptr[0] == '\t')
           || (parser->ptr[0] == '\n') || (parser->ptr[0] == '\r'))
    {
        parser->ptr++;
    }
}

/*
 * Parse 4 hexadecimal digits.
 *
 * Return the value (0x0000 to 0xFFFF), -1 if error.
 */

int
relay_json_parse_hex4 (const char *string)
{
    int i, value;

    value = 0;
    for (i = 0; i < 4; i++)
    {
        value <<= 4;
        if ((string[i] >= '0') && (string[i] <= '9'))
            value |= string[i] - '0';
        else if ((string[i] >= 'a') && (string[i] <= 'f'))
            value |= string[i] - 'a' + 10;
        else if ((string[i] >= 'A') && (string[i] <= 'F'))
            value |= string[i] - 'A' + 10;
        else
            return -1;
    }

    return value;
}

/*
 * Encode a Unicode code point (U+0001 to U+10FFFF) to UTF-8.
 *
 * Return number of bytes written in output (1 to 4).
 */

int
relay_json_encode_utf8 (unsigned int code_point, char *output)
{
    if (code_point <= 0x7F)
    {
        output[0] = code_point;
        return 1;
    }
    if (code_point <= 0x7FF)
    {
        output[0] = 0xC0 | (code_point >> 6);
        output[1] = 0x80 | (code_point & 0x3F);
        return 2;
    }
    if (code_point <= 0xFFFF)
    {
        output[0] = 0xE0 | (code_point >> 12);
        output[1] = 0x80 | ((code_point >> 6) & 0x3F);
        output[2] = 0x80 | (code_point & 0x3F);
        return 3;
    }
    output[0] = 0xF0 | (code_point >> 18);
    output[1] = 0x80 | ((code_point >> 12) & 0x3F);
    output[2] = 0x80 | ((code_point >> 6) & 0x3F);
    output[3] = 0x80 | (code_point & 0x3F);
    return 4;
}

/*
 * Parse a JSON string (parser must be on the opening double quote).
 *
 * Escapes are decoded, the result must be valid UTF-8 and must not contain
 * a NUL char ("\u0000").
 *
 * Return the decoded string, NULL if error.
 *
 * Note: result must be freed after use.
 */

char *
relay_json_parse_string (struct t_relay_json_parser *parser)
{
    const char *ptr_start, *ptr_end, *ptr_in;
    char *result, *ptr_out;
    int high, low, has_8bits;

    if (parser->ptr[0] != '"')
        return NULL;

    ptr_start = parser->ptr + 1;

    /*
     * Find end of string; the decoded string is never longer than the
     * raw string.
     */
    ptr_end = ptr_start;
    while (ptr_end[0] != '"')
    {
        if ((unsigned char)ptr_end[0] < 0x20)
            return NULL;
        if (ptr_end[0] == '\\')
        {
            ptr_end++;
            if (!ptr_end[0])
                return NULL;
        }
        ptr_end++;
    }

    result = malloc (ptr_end - ptr_start + 1);
    if (!result)
        return NULL;

    has_8bits = 0;
    ptr_out = result;
    ptr_in = ptr_start;
    while (ptr_in < ptr_end)
    {
        if (ptr_in[0] != '\\')
        {
            if ((unsigned char)ptr_in[0] >= 0x80)
                has_8bits = 1;
            *ptr_out++ = *ptr_in++;
            continue;
        }
        ptr_in++;
        switch (ptr_in[0])
        {
            case '"':
            case '\\':
            case '/':
                *ptr_out++ = ptr_in[0];
                ptr_in++;
                break;
            case 'b':
                *ptr_out++ = '\b';
                ptr_in++;
                break;
            case 'f':
                *ptr_out++ = '\f';
                ptr_in++;
                break;
            case 'n':
                *ptr_out++ = '\n';
                ptr_in++;
                break;
            case 'r':
                *ptr_out++ = '\r';
                ptr_in++;
                break;
            case 't':
                *ptr_out++ = '\t';
                ptr_in++;
                break;
            case 'u':
                if (ptr_end - ptr_in < 5)
                    goto error;
                high = relay_json_parse_hex4 (ptr_in + 1);
                if (high <= 0)
                    goto error;
                ptr_in += 5;
                if ((high >= 0xDC00) && (high <= 0xDFFF))
                    goto error;
                if ((high >= 0xD800) && (high <= 0xDBFF))
                {
                    /* high surrogate: a low surrogate must follow */
                    if ((ptr_end - ptr_in < 6)
                        || (ptr_in[0] != '\\') || (ptr_in[1] != 'u'))
                    {
                        goto error;
                    }
                    low = relay_json_parse_hex4 (ptr_in + 2);
                    if ((low < 0xDC00) || (low > 0xDFFF))
                        goto error;
                    ptr_in += 6;
                    high = 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
                }
                ptr_out += relay_json_encode_utf8 (high, ptr_out);
                break;
            default:
                goto error;
        }
    }
    ptr_out[0] = '\0';

    if (has_8bits && !weechat_utf8_is_valid (result, -1, NULL))
        goto error;

    parser->ptr = ptr_end + 1;

    return result;

error:
    free (result);
    return NULL;
}

/*
 * Parse a JSON number.
 *
 * The number must follow the JSON grammar:
 *   -? (0 | [1-9][0-9]*) (.[0-9]+)? ([eE][+-]?[0-9]+)?
 *
 * An integer that fits in a long long is stored as integer, any other
 * number (with fraction, exponent, or too big) is kept as raw string.
 *
 * Return the JSON number, NULL if error.
 */

struct t_relay_json *
relay_json_parse_number (struct t_relay_json_parser *parser)
{
    struct t_relay_json *new_json;
    const char *ptr_start, *ptr_end;
    char str_number[32];
    long long number;
    int is_integer;

    ptr_start = parser->ptr;
    ptr_end = ptr_start;
    is_integer = 1;

    if (ptr_end[0] == '-')
        ptr_end++;
    if (ptr_end[0] == '0')
    {
        ptr_end++;
    }
    else if ((ptr_end[0] >= '1') && (ptr_end[0] <= '9'))
    {
        while ((ptr_end[0] >= '0') && (ptr_end[0] <= '9'))
            ptr_end++;
    }
    else
    {
        return NULL;
    }
    if (ptr_end[0] == '.')
    {
        is_integer = 0;
        ptr_end++;
        if ((ptr_end[0] < '0') || (ptr_end[0] > '9'))
            return NULL;
        while ((ptr_end[0] >= '0') && (ptr_end[0] <= '9'))
            ptr_end++;
    }
    if ((ptr_end[0] == 'e') || (ptr_end[0] == 'E'))
    {
        is_integer = 0;
        ptr_end++;
        if ((ptr_end[0] == '+') || (ptr_end[0] == '-'))
            ptr_end++;
        if ((ptr_end[0] < '0') || (ptr_end[0] > '9'))
            return NULL;
        while ((ptr_end[0] >= '0') && (ptr_end[0] <= '9'))
            ptr_end++;
    }

    new_json = relay_json_new (RELAY_JSON_NUMBER);
    if (!new_json)
        return NULL;

    if (is_integer && ((size_t)(ptr_end - ptr_start) < sizeof (str_number)))
    {
        memcpy (str_number, ptr_start, ptr_end - ptr_start);
        str_number[ptr_end - ptr_start] = '\0';
        if (weechat_util_parse_longlong (str_number, 10, &number))
        {
            new_json->integer = number;
            new_json->is_integer = 1;
        }
    }

    if (!new_json->is_integer)
    {
        new_json->string = malloc (ptr_end - ptr_start + 1);
        if (!new_json->string)
        {
            free (new_json);
            return NULL;
        }
        memcpy (new_json->string, ptr_start, ptr_end - ptr_start);
        new_json->string[ptr_end - ptr_start] = '\0';
    }

    parser->ptr = ptr_end;

    return new_json;
}

/*
 * Parse a JSON array (parser must be on the opening bracket).
 *
 * Return the JSON array, NULL if error.
 */

struct t_relay_json *
relay_json_parse_array (struct t_relay_json_parser *parser)
{
    struct t_relay_json *new_json, *item;

    new_json = relay_json_new_array ();
    if (!new_json)
        return NULL;

    parser->ptr++;
    relay_json_parse_skip_whitespace (parser);

    if (parser->ptr[0] == ']')
    {
        parser->ptr++;
        return new_json;
    }

    while (1)
    {
        item = relay_json_parse_value (parser);
        if (!item)
            goto error;
        relay_json_append_child (new_json, item);
        relay_json_parse_skip_whitespace (parser);
        if (parser->ptr[0] == ']')
        {
            parser->ptr++;
            return new_json;
        }
        if (parser->ptr[0] != ',')
            goto error;
        parser->ptr++;
    }

error:
    relay_json_free (new_json);
    return NULL;
}

/*
 * Parse a JSON object (parser must be on the opening brace).
 *
 * Return the JSON object, NULL if error.
 */

struct t_relay_json *
relay_json_parse_object (struct t_relay_json_parser *parser)
{
    struct t_relay_json *new_json, *item;
    char *key;

    new_json = relay_json_new_object ();
    if (!new_json)
        return NULL;

    parser->ptr++;
    relay_json_parse_skip_whitespace (parser);

    if (parser->ptr[0] == '}')
    {
        parser->ptr++;
        return new_json;
    }

    while (1)
    {
        relay_json_parse_skip_whitespace (parser);
        key = relay_json_parse_string (parser);
        if (!key)
            goto error;
        relay_json_parse_skip_whitespace (parser);
        if (parser->ptr[0] != ':')
        {
            free (key);
            goto error;
        }
        parser->ptr++;
        item = relay_json_parse_value (parser);
        if (!item)
        {
            free (key);
            goto error;
        }
        item->key = key;
        relay_json_append_child (new_json, item);
        relay_json_parse_skip_whitespace (parser);
        if (parser->ptr[0] == '}')
        {
            parser->ptr++;
            return new_json;
        }
        if (parser->ptr[0] != ',')
            goto error;
        parser->ptr++;
    }

error:
    relay_json_free (new_json);
    return NULL;
}

/*
 * Parse a JSON value (leading whitespace is skipped).
 *
 * Return the JSON value, NULL if error.
 */

struct t_relay_json *
relay_json_parse_value (struct t_relay_json_parser *parser)
{
    struct t_relay_json *new_json;
    char *string;

    relay_json_parse_skip_whitespace (parser);

    switch (parser->ptr[0])
    {
        case '{':
        case '[':
            if (parser->depth >= RELAY_JSON_MAX_DEPTH)
                return NULL;
            parser->depth++;
            new_json = (parser->ptr[0] == '{') ?
                relay_json_parse_object (parser) :
                relay_json_parse_array (parser);
            parser->depth--;
            return new_json;
        case '"':
            string = relay_json_parse_string (parser);
            if (!string)
                return NULL;
            new_json = relay_json_new (RELAY_JSON_STRING);
            if (!new_json)
            {
                free (string);
                return NULL;
            }
            new_json->string = string;
            return new_json;
        case 't':
            if (strncmp (parser->ptr, "true", 4) != 0)
                return NULL;
            parser->ptr += 4;
            return relay_json_new (RELAY_JSON_TRUE);
        case 'f':
            if (strncmp (parser->ptr, "false", 5) != 0)
                return NULL;
            parser->ptr += 5;
            return relay_json_new (RELAY_JSON_FALSE);
        case 'n':
            if (strncmp (parser->ptr, "null", 4) != 0)
                return NULL;
            parser->ptr += 4;
            return relay_json_new (RELAY_JSON_NULL);
        default:
            return relay_json_parse_number (parser);
    }
}

/*
 * Parse a JSON string.
 *
 * The whole string must contain exactly one JSON value (with optional
 * whitespace around), optionally preceded by a UTF-8 BOM (RFC 8259, section
 * 8.1).
 *
 * Return the JSON value, NULL if error.
 *
 * Note: result must be freed after use with function relay_json_free.
 */

struct t_relay_json *
relay_json_parse (const char *string)
{
    struct t_relay_json_parser parser;
    struct t_relay_json *json;

    if (!string)
        return NULL;

    parser.ptr = (strncmp (string, "\xEF\xBB\xBF", 3) == 0) ?
        string + 3 : string;
    parser.depth = 0;

    json = relay_json_parse_value (&parser);
    if (!json)
        return NULL;

    relay_json_parse_skip_whitespace (&parser);
    if (parser.ptr[0])
    {
        relay_json_free (json);
        return NULL;
    }

    return json;
}

/*
 * Add bytes to an output buffer.
 *
 * Return:
 *   1: OK
 *   0: error (memory allocation)
 */

int
relay_json_buffer_add (struct t_relay_json_buffer *buffer,
                       const char *data, size_t length)
{
    char *new_data;
    size_t new_size_alloc;

    if (buffer->length + length + 1 > buffer->size_alloc)
    {
        new_size_alloc = buffer->size_alloc + (buffer->size_alloc / 2);
        if (new_size_alloc < buffer->length + length + 1)
            new_size_alloc = buffer->length + length + 1;
        new_data = realloc (buffer->data, new_size_alloc);
        if (!new_data)
            return 0;
        buffer->data = new_data;
        buffer->size_alloc = new_size_alloc;
    }

    memcpy (buffer->data + buffer->length, data, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';

    return 1;
}

/*
 * Add a JSON string (with double quotes and escaped chars) to an output
 * buffer.
 *
 * Any byte that is not part of a valid UTF-8 char is replaced by "?", so that
 * the output is always valid JSON.
 *
 * Return:
 *   1: OK
 *   0: error (memory allocation)
 */

int
relay_json_print_string (struct t_relay_json_buffer *buffer,
                         const char *string)
{
    const char *ptr_string, *ptr_start;
    char str_escape[8];
    int length_escape;

    if (!relay_json_buffer_add (buffer, "\"", 1))
        return 0;

    ptr_start = string;
    for (ptr_string = string; ptr_string && ptr_string[0]; ptr_string++)
    {
        switch (ptr_string[0])
        {
            case '"':
                memcpy (str_escape, "\\\"", 2);
                length_escape = 2;
                break;
            case '\\':
                memcpy (str_escape, "\\\\", 2);
                length_escape = 2;
                break;
            case '\b':
                memcpy (str_escape, "\\b", 2);
                length_escape = 2;
                break;
            case '\f':
                memcpy (str_escape, "\\f", 2);
                length_escape = 2;
                break;
            case '\n':
                memcpy (str_escape, "\\n", 2);
                length_escape = 2;
                break;
            case '\r':
                memcpy (str_escape, "\\r", 2);
                length_escape = 2;
                break;
            case '\t':
                memcpy (str_escape, "\\t", 2);
                length_escape = 2;
                break;
            default:
                if ((unsigned char)ptr_string[0] >= 0x80)
                {
                    /* Valid UTF-8 char: keep it as-is. */
                    if (weechat_utf8_is_valid (ptr_string, 1, NULL))
                    {
                        ptr_string += weechat_utf8_char_size (ptr_string) - 1;
                        continue;
                    }
                    /* Invalid UTF-8: replace the byte by "?". */
                    str_escape[0] = '?';
                    length_escape = 1;
                    break;
                }
                if ((unsigned char)ptr_string[0] >= 0x20)
                    continue;
                snprintf (str_escape, sizeof (str_escape),
                          "\\u%04x", (unsigned char)ptr_string[0]);
                length_escape = 6;
                break;
        }
        if (!relay_json_buffer_add (buffer, ptr_start, ptr_string - ptr_start)
            || !relay_json_buffer_add (buffer, str_escape, length_escape))
        {
            return 0;
        }
        ptr_start = ptr_string + 1;
    }

    return (relay_json_buffer_add (buffer, ptr_start, ptr_string - ptr_start)
            && relay_json_buffer_add (buffer, "\"", 1));
}

/*
 * Add children of an array or object to an output buffer, enclosed by
 * chars "open" and "close".
 *
 * Return:
 *   1: OK
 *   0: error (memory allocation)
 */

int
relay_json_print_children (struct t_relay_json_buffer *buffer,
                           struct t_relay_json *json,
                           const char *open, const char *close)
{
    struct t_relay_json *ptr_item;

    if (!relay_json_buffer_add (buffer, open, 1))
        return 0;

    for (ptr_item = json->child; ptr_item; ptr_item = ptr_item->next)
    {
        if ((ptr_item != json->child)
            && !relay_json_buffer_add (buffer, ",", 1))
        {
            return 0;
        }
        if (json->type == RELAY_JSON_OBJECT)
        {
            if (!relay_json_print_string (buffer,
                                          (ptr_item->key) ? ptr_item->key : "")
                || !relay_json_buffer_add (buffer, ":", 1))
            {
                return 0;
            }
        }
        if (!relay_json_print_value (buffer, ptr_item))
            return 0;
    }

    return relay_json_buffer_add (buffer, close, 1);
}

/*
 * Add a JSON value to an output buffer.
 *
 * Return:
 *   1: OK
 *   0: error (memory allocation)
 */

int
relay_json_print_value (struct t_relay_json_buffer *buffer,
                        struct t_relay_json *json)
{
    char str_number[32];
    int length;

    switch (json->type)
    {
        case RELAY_JSON_NULL:
            return relay_json_buffer_add (buffer, "null", 4);
        case RELAY_JSON_FALSE:
            return relay_json_buffer_add (buffer, "false", 5);
        case RELAY_JSON_TRUE:
            return relay_json_buffer_add (buffer, "true", 4);
        case RELAY_JSON_NUMBER:
            if (!json->is_integer)
            {
                return relay_json_buffer_add (buffer, json->string,
                                              strlen (json->string));
            }
            length = snprintf (str_number, sizeof (str_number),
                               "%lld", json->integer);
            return relay_json_buffer_add (buffer, str_number, length);
        case RELAY_JSON_STRING:
            return relay_json_print_string (buffer, json->string);
        case RELAY_JSON_ARRAY:
            return relay_json_print_children (buffer, json, "[", "]");
        case RELAY_JSON_OBJECT:
            return relay_json_print_children (buffer, json, "{", "}");
    }

    return 0;
}

/*
 * Convert a JSON value to a string (compact format, without whitespace).
 *
 * Return the string, NULL if error.
 *
 * Note: result must be freed after use.
 */

char *
relay_json_print (struct t_relay_json *json)
{
    struct t_relay_json_buffer buffer;

    if (!json)
        return NULL;

    buffer.size_alloc = 256;
    buffer.length = 0;
    buffer.data = malloc (buffer.size_alloc);
    if (!buffer.data)
        return NULL;
    buffer.data[0] = '\0';

    if (!relay_json_print_value (&buffer, json))
    {
        free (buffer.data);
        return NULL;
    }

    return buffer.data;
}

/*
 * Free a JSON value and all its children.
 *
 * The value must not be in an array/object (or it must be detached first).
 */

void
relay_json_free (struct t_relay_json *json)
{
    struct t_relay_json *ptr_item, *ptr_next_item;

    if (!json)
        return;

    ptr_item = json->child;
    while (ptr_item)
    {
        ptr_next_item = ptr_item->next;
        relay_json_free (ptr_item);
        ptr_item = ptr_next_item;
    }

    free (json->key);
    free (json->string);
    free (json);
}
