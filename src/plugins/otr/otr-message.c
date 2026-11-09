/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Conversion of OTR message text */

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-message.h"
#include "otr-util.h"


/*
 * Check if a message is a CTCP (enclosed in chars "\x01").
 *
 * Return 1 if the message is a CTCP, 0 otherwise.
 */

int
otr_message_is_ctcp (const char *message)
{
    size_t length;

    length = strlen (message);

    return ((length >= 2) && (message[0] == '\x01')
            && (message[length - 1] == '\x01')) ? 1 : 0;
}

/*
 * Check if a message is a CTCP ACTION ("/me").
 *
 * Return 1 if the message is a CTCP ACTION, 0 otherwise.
 */

int
otr_message_is_action (const char *message)
{
    return (otr_message_is_ctcp (message)
            && (strncmp (message, "\x01" "ACTION ", 8) == 0)) ? 1 : 0;
}

/*
 * Convert an IRC message to the text sent with OTR: a CTCP ACTION ("\x01ACTION
 * text\x01") becomes "/me text", any other message is copied as-is.
 *
 * Note: result must be freed after use.
 */

char *
otr_message_plain_from_irc (const char *message)
{
    char *result;
    size_t length;

    if (!otr_message_is_action (message))
        return otr_util_dupstr (message);

    length = strlen (message);
    weechat_asprintf (&result, OTR_ACTION_PREFIX "%.*s",
                      (int)(length - 9), message + 8);

    return result;
}

/*
 * Escape HTML special chars ("&", "<" and ">") in a string.
 *
 * Note: result must be freed after use.
 */

char *
otr_message_html_escape (const char *string)
{
    char **result;
    const char *ptr_string;

    result = weechat_string_dyn_alloc (256);
    if (!result)
        return NULL;

    for (ptr_string = string; ptr_string[0]; ptr_string++)
    {
        if (ptr_string[0] == '&')
            weechat_string_dyn_concat (result, "&amp;", -1);
        else if (ptr_string[0] == '<')
            weechat_string_dyn_concat (result, "&lt;", -1);
        else if (ptr_string[0] == '>')
            weechat_string_dyn_concat (result, "&gt;", -1);
        else
            weechat_string_dyn_concat (result, ptr_string, 1);
    }

    return weechat_string_dyn_free (result, 0);
}

/*
 * Add a Unicode code point encoded in UTF-8 to a dynamic string (code point 0
 * and invalid code points are ignored).
 */

void
otr_message_utf8_append (char **string, unsigned long code_point)
{
    char utf_char[5];

    if (code_point == 0)
        return;

    if (code_point < 0x80)
    {
        utf_char[0] = (char)code_point;
        utf_char[1] = '\0';
    }
    else if (code_point < 0x800)
    {
        utf_char[0] = (char)(0xC0 | (code_point >> 6));
        utf_char[1] = (char)(0x80 | (code_point & 0x3F));
        utf_char[2] = '\0';
    }
    else if (code_point < 0x10000)
    {
        utf_char[0] = (char)(0xE0 | (code_point >> 12));
        utf_char[1] = (char)(0x80 | ((code_point >> 6) & 0x3F));
        utf_char[2] = (char)(0x80 | (code_point & 0x3F));
        utf_char[3] = '\0';
    }
    else if (code_point <= 0x10FFFF)
    {
        utf_char[0] = (char)(0xF0 | (code_point >> 18));
        utf_char[1] = (char)(0x80 | ((code_point >> 12) & 0x3F));
        utf_char[2] = (char)(0x80 | ((code_point >> 6) & 0x3F));
        utf_char[3] = (char)(0x80 | (code_point & 0x3F));
        utf_char[4] = '\0';
    }
    else
    {
        return;
    }

    weechat_string_dyn_concat (string, utf_char, -1);
}

/*
 * Reduce HTML to IRC text: keep text, convert "<br>" to a new line and links to
 * "[text](url)" (or "[url]" if the text is the URL), decode a few entities and
 * drop all other tags.
 *
 * Note: result must be freed after use.
 */

char *
otr_message_html_filter (const char *html)
{
    char **result, link[1024], tag_name[16], entity[16], *tag, *ptr_end;
    char quote;
    const char *ptr_html, *ptr_tag_end, *ptr_semicolon, *ptr_href;
    const char *ptr_tag_name, *ptr_tag;
    size_t link_start, link_length, entity_length;
    unsigned long code_point;
    int closing, i;

    result = weechat_string_dyn_alloc (256);
    if (!result)
        return NULL;

    link[0] = '\0';
    link_start = 0;
    ptr_html = html;

    while (ptr_html[0])
    {
        ptr_tag_end = NULL;
        if ((ptr_html[0] == '<')
            && (isalpha ((unsigned char)ptr_html[1]) || (ptr_html[1] == '/')
                || (ptr_html[1] == '!')))
        {
            ptr_tag_end = strchr (ptr_html, '>');
        }
        ptr_semicolon = NULL;
        if (!ptr_tag_end && (ptr_html[0] == '&'))
        {
            ptr_semicolon = strchr (ptr_html, ';');
            if (ptr_semicolon
                && (((ptr_semicolon - ptr_html) > 10)
                    || ((ptr_semicolon - ptr_html) <= 1)))
            {
                ptr_semicolon = NULL;
            }
        }

        if (ptr_tag_end)
        {
            /* HTML tag. */
            ptr_tag_name = ptr_html + 1;
            closing = 0;
            if (ptr_tag_name[0] == '/')
            {
                closing = 1;
                ptr_tag_name++;
            }
            for (i = 0; (i < (int)sizeof (tag_name) - 1)
                     && isalpha ((unsigned char)ptr_tag_name[i]); i++)
            {
                tag_name[i] = (char)tolower ((unsigned char)ptr_tag_name[i]);
            }
            tag_name[i] = '\0';

            if (!closing && (strcmp (tag_name, "br") == 0))
            {
                weechat_string_dyn_concat (result, "\n", -1);
            }
            else if (!closing && (strcmp (tag_name, "a") == 0))
            {
                /* Look for href="..." inside the tag. */
                tag = weechat_strndup (ptr_html, ptr_tag_end - ptr_html);
                if (tag)
                {
                    ptr_href = NULL;
                    for (ptr_tag = tag; ptr_tag[0]; ptr_tag++)
                    {
                        if (weechat_strncasecmp (ptr_tag, "href=", 5) == 0)
                        {
                            ptr_href = ptr_tag + 5;
                            break;
                        }
                    }
                    if (ptr_href)
                    {
                        quote = 0;
                        if ((ptr_href[0] == '"') || (ptr_href[0] == '\''))
                            quote = ptr_href[0];
                        if (quote)
                            ptr_href++;
                        link_length = 0;
                        while (ptr_href[link_length]
                               && (link_length < sizeof (link) - 1))
                        {
                            if (quote && (ptr_href[link_length] == quote))
                                break;
                            if (!quote
                                && isspace (
                                    (unsigned char)ptr_href[link_length]))
                            {
                                break;
                            }
                            link_length++;
                        }
                        memcpy (link, ptr_href, link_length);
                        link[link_length] = '\0';
                        if (link_length > 0)
                        {
                            weechat_string_dyn_concat (result, "[", -1);
                            link_start = strlen (*result);
                        }
                    }
                    free (tag);
                }
            }
            else if (closing && (strcmp (tag_name, "a") == 0) && link[0])
            {
                link_length = strlen (link);
                if ((strlen (*result) - link_start == link_length)
                    && (memcmp (*result + link_start, link, link_length) == 0))
                {
                    weechat_string_dyn_concat (result, "]", -1);
                }
                else
                {
                    weechat_string_dyn_concat (result, "](", -1);
                    weechat_string_dyn_concat (result, link, -1);
                    weechat_string_dyn_concat (result, ")", -1);
                }
                link[0] = '\0';
            }
            ptr_html = ptr_tag_end + 1;
        }
        else if (ptr_semicolon)
        {
            /* HTML entity. */
            entity_length = ptr_semicolon - ptr_html - 1;
            memcpy (entity, ptr_html + 1, entity_length);
            entity[entity_length] = '\0';
            if (entity[0] == '#')
            {
                code_point = ((entity[1] == 'x') || (entity[1] == 'X')) ?
                    strtoul (entity + 2, &ptr_end, 16) :
                    strtoul (entity + 1, &ptr_end, 10);
                if (!ptr_end[0] && (entity_length > 1) && (code_point > 0))
                {
                    otr_message_utf8_append (result, code_point);
                }
                else
                {
                    weechat_string_dyn_concat (
                        result, ptr_html, ptr_semicolon - ptr_html + 1);
                }
            }
            else if (strcmp (entity, "amp") == 0)
            {
                weechat_string_dyn_concat (result, "&", -1);
            }
            else if (strcmp (entity, "lt") == 0)
            {
                weechat_string_dyn_concat (result, "<", -1);
            }
            else if (strcmp (entity, "gt") == 0)
            {
                weechat_string_dyn_concat (result, ">", -1);
            }
            else if (strcmp (entity, "quot") == 0)
            {
                weechat_string_dyn_concat (result, "\"", -1);
            }
            else if (strcmp (entity, "apos") == 0)
            {
                weechat_string_dyn_concat (result, "'", -1);
            }
            else if (strcmp (entity, "nbsp") == 0)
            {
                weechat_string_dyn_concat (result, " ", -1);
            }
            else
            {
                weechat_string_dyn_concat (
                    result, ptr_html, ptr_semicolon - ptr_html + 1);
            }
            ptr_html = ptr_semicolon + 1;
        }
        else
        {
            weechat_string_dyn_concat (result, ptr_html, 1);
            ptr_html++;
        }
    }

    return weechat_string_dyn_free (result, 0);
}

/*
 * Add one decrypted line as an IRC PRIVMSG to a dynamic string.
 *
 * CTCP markers that a peer could smuggle in are removed; only a well-formed
 * "/me" (or CTCP ACTION) becomes a CTCP ACTION.
 */

void
otr_message_add_decrypted_line (char **messages, const char *tags,
                                const char *from, const char *target,
                                const char *line, size_t length)
{
    const char *ptr_line, *ptr_end;
    int action;

    ptr_line = line;
    ptr_end = line + length;
    action = 0;

    if ((length >= 4) && (strncmp (line, OTR_ACTION_PREFIX, 4) == 0))
    {
        action = 1;
        ptr_line += 4;
    }
    else if ((length >= 9) && (strncmp (line, "\x01" "ACTION ", 8) == 0)
             && (line[length - 1] == '\x01'))
    {
        action = 1;
        ptr_line += 8;
        ptr_end--;
    }

    if ((*messages)[0])
        weechat_string_dyn_concat (messages, "\r\n", -1);
    weechat_string_dyn_concat (messages, tags, -1);
    weechat_string_dyn_concat (messages, ":", -1);
    weechat_string_dyn_concat (messages, from, -1);
    weechat_string_dyn_concat (messages, " PRIVMSG ", -1);
    weechat_string_dyn_concat (messages, target, -1);
    weechat_string_dyn_concat (messages, " :", -1);
    if (action)
        weechat_string_dyn_concat (messages, "\x01" "ACTION ", -1);
    for (; ptr_line < ptr_end; ptr_line++)
    {
        if ((ptr_line[0] != '\x01') && (ptr_line[0] != '\r')
            && (ptr_line[0] != '\0'))
        {
            weechat_string_dyn_concat (messages, ptr_line, 1);
        }
    }
    if (action)
        weechat_string_dyn_concat (messages, "\x01", -1);
}

/*
 * Build incoming IRC PRIVMSG messages (separated by "\r\n") with the lines of a
 * message: each line becomes its own PRIVMSG.
 *
 * If decrypted is 1, lines are added with function
 * otr_message_add_decrypted_line, otherwise they are added as-is after
 * line_prefix.
 *
 * Note: result must be freed after use.
 */

char *
otr_message_build_in_privmsgs (const char *tags, const char *from,
                               const char *target, const char *message,
                               const char *line_prefix, int decrypted)
{
    char **result;
    const char *ptr_message, *ptr_newline, *ptr_char;
    size_t length;

    result = weechat_string_dyn_alloc (256);
    if (!result)
        return NULL;

    ptr_message = message;
    while (ptr_message[0])
    {
        ptr_newline = strchr (ptr_message, '\n');
        length = (ptr_newline) ?
            (size_t)(ptr_newline - ptr_message) : strlen (ptr_message);
        if ((length > 0) && (ptr_message[length - 1] == '\r'))
            length--;
        if (decrypted)
        {
            otr_message_add_decrypted_line (result, tags, from, target,
                                            ptr_message, length);
        }
        else
        {
            if ((*result)[0])
                weechat_string_dyn_concat (result, "\r\n", -1);
            weechat_string_dyn_concat (result, tags, -1);
            weechat_string_dyn_concat (result, ":", -1);
            weechat_string_dyn_concat (result, from, -1);
            weechat_string_dyn_concat (result, " PRIVMSG ", -1);
            weechat_string_dyn_concat (result, target, -1);
            weechat_string_dyn_concat (result, " :", -1);
            for (ptr_char = line_prefix; ptr_char[0]; ptr_char++)
            {
                if (ptr_char[0] != '\r')
                    weechat_string_dyn_concat (result, ptr_char, 1);
            }
            for (ptr_char = ptr_message; ptr_char < ptr_message + length;
                 ptr_char++)
            {
                if ((ptr_char[0] != '\r') && (ptr_char[0] != '\0'))
                    weechat_string_dyn_concat (result, ptr_char, 1);
            }
        }
        ptr_message = (ptr_newline) ?
            ptr_newline + 1 : ptr_message + strlen (ptr_message);
    }

    return weechat_string_dyn_free (result, 0);
}
