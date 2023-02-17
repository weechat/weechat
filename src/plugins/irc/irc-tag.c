/*
 * irc-tag.c - functions for IRC message tags
 *
 * Copyright (C) 2021-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-tag.h"


/*
 * Escapes a tag value, the following sequences are replaced:
 *
 *    character     | escaped value
 *   ---------------+-------------------------
 *    ; (semicolon) | \: (backslash and colon)
 *    SPACE         | \s
 *    \             | \\
 *    CR            | \r
 *    LF            | \n
 *    all others    | the character itself
 *
 * See: https://ircv3.net/specs/extensions/message-tags#escaping-values
 *
 * Note: result must be freed after use.
 */

char *
irc_tag_escape_value (const char *string)
{
    char **out;
    unsigned char *ptr_string;
    int length;

    if (!string)
        return NULL;

    length = strlen (string);
    out = weechat_string_dyn_alloc (length + (length / 2) + 1);
    if (!out)
        return NULL;

    ptr_string = (unsigned char *)string;
    while (ptr_string && ptr_string[0])
    {
        switch (ptr_string[0])
        {
            case ';':
                weechat_string_dyn_concat (out, "\\:", -1);
                ptr_string++;
                break;
            case ' ':
                weechat_string_dyn_concat (out, "\\s", -1);
                ptr_string++;
                break;
            case '\\':
                weechat_string_dyn_concat (out, "\\\\", -1);
                ptr_string++;
                break;
            case '\r':
                weechat_string_dyn_concat (out, "\\r", -1);
                ptr_string++;
                break;
            case '\n':
                weechat_string_dyn_concat (out, "\\n", -1);
                ptr_string++;
                break;
            default:
                length = weechat_utf8_char_size ((char *)ptr_string);
                if (length == 0)
                    length = 1;
                weechat_string_dyn_concat (out,
                                           (const char *)ptr_string,
                                           length);
                ptr_string += length;
                break;
        }
    }

    return weechat_string_dyn_free (out, 0);
}

/*
 * Unescapes a tag value.
 *
 * See: https://ircv3.net/specs/extensions/message-tags#escaping-values
 *
 * Note: result must be freed after use.
 */

char *
irc_tag_unescape_value (const char *string)
{
    char **out;
    unsigned char *ptr_string;
    int length;

    if (!string)
        return NULL;

    length = strlen (string);
    out = weechat_string_dyn_alloc (length + (length / 2) + 1);
    if (!out)
        return NULL;

    ptr_string = (unsigned char *)string;
    while (ptr_string && ptr_string[0])
    {
        switch (ptr_string[0])
        {
            case '\\':
                ptr_string++;
                switch (ptr_string[0])
                {
                    case ':':
                        weechat_string_dyn_concat (out, ";", -1);
                        ptr_string++;
                        break;
                    case 's':
                        weechat_string_dyn_concat (out, " ", -1);
                        ptr_string++;
                        break;
                    case '\\':
                        weechat_string_dyn_concat (out, "\\", -1);
                        ptr_string++;
                        break;
                    case 'r':
                        weechat_string_dyn_concat (out, "\r", -1);
                        ptr_string++;
                        break;
                    case 'n':
                        weechat_string_dyn_concat (out, "\n", -1);
                        ptr_string++;
                        break;
                    default:
                        if (ptr_string[0])
                        {
                            length = weechat_utf8_char_size ((char *)ptr_string);
                            if (length == 0)
                                length = 1;
                            weechat_string_dyn_concat (out,
                                                       (const char *)ptr_string,
                                                       length);
                            ptr_string += length;
                        }
                        break;
                }
                break;
            default:
                length = weechat_utf8_char_size ((char *)ptr_string);
                if (length == 0)
                    length = 1;
                weechat_string_dyn_concat (out,
                                           (const char *)ptr_string,
                                           length);
                ptr_string += length;
                break;
        }
    }

    return weechat_string_dyn_free (out, 0);
}

/*
 * Callback for modifiers "irc_tag_escape_value" and "irc_tag_unescape_value".
 *
 * These modifiers can be used by other plugins to escape/unescape IRC message
 * tags.
 */

char *
irc_tag_modifier_cb (const void *pointer, void *data,
                     const char *modifier, const char *modifier_data,
                     const char *string)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier_data;

    if (strcmp (modifier, "irc_tag_escape_value") == 0)
        return irc_tag_escape_value (string);

    if (strcmp (modifier, "irc_tag_unescape_value") == 0)
        return irc_tag_unescape_value (string);

    /* unknown modifier */
    return NULL;
}

/*
 * Parses tags received in an IRC message and returns the number of tags
 * set in the hashtable "hashtable" (values are unescaped tag values).
 *
 * If prefix_key is not NULL, it is used as prefix before the name of keys.
 *
 * Example:
 *
 *   input:
 *     tags == "aaa=bbb;ccc;example.com/ddd=value\swith\sspaces"
 *     prefix_key == "tag_"
 *   output:
 *     hashtable is completed with the following keys/values:
 *       "tag_aaa" => "bbb"
 *       "tag_ccc" => NULL
 *       "tag_example.com/ddd" => "value with spaces"
 */

int
irc_tag_parse (const char *tags,
               struct t_hashtable *hashtable, const char *prefix_key)
{
    char **items, *pos, *key, str_key[4096], *unescaped;
    int num_items, i, num_tags;

    if (!tags || !tags[0] || !hashtable)
        return 0;

    num_tags = 0;

    items = weechat_string_split (tags, ";", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            pos = strchr (items[i], '=');
            if (pos)
            {
                /* format: "tag=value" */
                key = weechat_strndup (items[i], pos - items[i]);
                if (key)
                {
                    snprintf (str_key, sizeof (str_key),
                              "%s%s",
                              (prefix_key) ? prefix_key : "",
                              key);
                    unescaped = irc_tag_unescape_value (pos + 1);
                    weechat_hashtable_set (hashtable, str_key, unescaped);
                    if (unescaped)
                        free (unescaped);
                    free (key);
                    num_tags++;
                }
            }
            else
            {
                /* format: "tag" */
                snprintf (str_key, sizeof (str_key),
                          "%s%s",
                          (prefix_key) ? prefix_key : "",
                          items[i]);
                weechat_hashtable_set (hashtable, str_key, NULL);
                num_tags++;
            }
        }
        weechat_string_free_split (items);
    }

    return num_tags;
}
