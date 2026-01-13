/*
 * SPDX-FileCopyrightText: 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
 * SPDX-FileCopyrightText: 2006 Emmanuel Bouthenot <kolter@openics.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* UTF-8 string functions */

/* for wcwidth in wchar.h */
#define _XOPEN_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "weechat.h"
#include "core-utf8.h"
#include "core-config.h"
#include "core-string.h"


int local_utf8 = 0;


/*
 * Unicode code points for grapheme cluster detection (UAX #29).
 */

#define UNICODE_ZWJ                 0x200D   /* Zero Width Joiner */
#define UNICODE_VS16                0xFE0F   /* Variation Selector 16 (emoji) */
#define UNICODE_VS15                0xFE0E   /* Variation Selector 15 (text) */
#define UNICODE_VS1_START           0xFE00   /* Variation Selector 1 */
#define UNICODE_VS16_END            0xFE0F   /* Variation Selector 16 */
#define UNICODE_REGIONAL_START      0x1F1E6  /* Regional Indicator A */
#define UNICODE_REGIONAL_END        0x1F1FF  /* Regional Indicator Z */
#define UNICODE_SKIN_TONE_START     0x1F3FB  /* Skin Tone Light */
#define UNICODE_SKIN_TONE_END       0x1F3FF  /* Skin Tone Dark */
#define UNICODE_COMBINING_START     0x0300   /* Combining Diacritical Marks start */
#define UNICODE_COMBINING_END       0x036F   /* Combining Diacritical Marks end */
#define UNICODE_KEYCAP              0x20E3   /* Combining Enclosing Keycap */
#define UNICODE_TAG_START           0xE0020  /* Tag Space */
#define UNICODE_TAG_END             0xE007F  /* Cancel Tag */

/*
 * Checks if a Unicode code point is a Zero Width Joiner.
 *
 * Returns:
 *   1: code point is ZWJ
 *   0: code point is not ZWJ
 */

int
utf8_is_zwj (int codepoint)
{
    return (codepoint == UNICODE_ZWJ);
}

/*
 * Checks if a Unicode code point is a Variation Selector (VS1-VS16).
 *
 * Returns:
 *   1: code point is a variation selector
 *   0: code point is not a variation selector
 */

int
utf8_is_variation_selector (int codepoint)
{
    return (codepoint >= UNICODE_VS1_START && codepoint <= UNICODE_VS16_END);
}

/*
 * Checks if a Unicode code point is a Regional Indicator.
 *
 * Returns:
 *   1: code point is a regional indicator
 *   0: code point is not a regional indicator
 */

int
utf8_is_regional_indicator (int codepoint)
{
    return (codepoint >= UNICODE_REGIONAL_START && codepoint <= UNICODE_REGIONAL_END);
}

/*
 * Checks if a Unicode code point is a Skin Tone Modifier (Fitzpatrick).
 *
 * Returns:
 *   1: code point is a skin tone modifier
 *   0: code point is not a skin tone modifier
 */

int
utf8_is_skin_tone_modifier (int codepoint)
{
    return (codepoint >= UNICODE_SKIN_TONE_START && codepoint <= UNICODE_SKIN_TONE_END);
}

/*
 * Checks if a Unicode code point is a Combining Mark.
 *
 * Returns:
 *   1: code point is a combining mark
 *   0: code point is not a combining mark
 */

int
utf8_is_combining_mark (int codepoint)
{
    /* Basic Combining Diacritical Marks */
    if (codepoint >= UNICODE_COMBINING_START && codepoint <= UNICODE_COMBINING_END)
        return 1;

    /* Combining Diacritical Marks Extended */
    if (codepoint >= 0x1AB0 && codepoint <= 0x1AFF)
        return 1;

    /* Combining Diacritical Marks Supplement */
    if (codepoint >= 0x1DC0 && codepoint <= 0x1DFF)
        return 1;

    /* Combining Diacritical Marks for Symbols */
    if (codepoint >= 0x20D0 && codepoint <= 0x20FF)
        return 1;

    /* Combining Half Marks */
    if (codepoint >= 0xFE20 && codepoint <= 0xFE2F)
        return 1;

    return 0;
}

/*
 * Checks if a Unicode code point is a Tag character (for flag sequences).
 *
 * Returns:
 *   1: code point is a tag character
 *   0: code point is not a tag character
 */

int
utf8_is_tag_character (int codepoint)
{
    return (codepoint >= UNICODE_TAG_START && codepoint <= UNICODE_TAG_END);
}

/*
 * Checks if a Unicode code point extends a grapheme cluster.
 *
 * A grapheme extender is a code point that should be combined with
 * the previous character to form a single grapheme cluster.
 *
 * Returns:
 *   1: code point extends grapheme cluster
 *   0: code point does not extend grapheme cluster
 */

int
utf8_is_grapheme_extender (int codepoint)
{
    /* Zero Width Joiner - joins characters */
    if (utf8_is_zwj (codepoint))
        return 1;

    /* Variation Selectors - modify previous character appearance */
    if (utf8_is_variation_selector (codepoint))
        return 1;

    /* Skin Tone Modifiers - modify emoji skin color */
    if (utf8_is_skin_tone_modifier (codepoint))
        return 1;

    /* Combining Marks - modify previous character */
    if (utf8_is_combining_mark (codepoint))
        return 1;

    /* Keycap - for keycap sequences like 1️⃣ */
    if (codepoint == UNICODE_KEYCAP)
        return 1;

    /* Tag characters - for subdivision flags like 🏴󠁧󠁢󠁥󠁮󠁧󠁿 */
    if (utf8_is_tag_character (codepoint))
        return 1;

    return 0;
}

/*
 * Gets pointer to next grapheme cluster in a string.
 *
 * A grapheme cluster is a user-perceived character, which may consist
 * of multiple Unicode code points (e.g., emoji with skin tone, flag
 * sequences, characters with combining marks).
 *
 * Returns pointer to next grapheme cluster, NULL if string was NULL or empty.
 */

const char *
utf8_grapheme_next (const char *string)
{
    const char *ptr_next;
    int codepoint, next_codepoint, in_regional_pair;

    if (!string || !string[0])
        return NULL;

    /* Move past first code point */
    ptr_next = utf8_next_char (string);
    if (!ptr_next || !ptr_next[0])
        return ptr_next;

    /* Get first code point to check if it's a regional indicator */
    codepoint = utf8_char_int (string);
    in_regional_pair = utf8_is_regional_indicator (codepoint);

    /* Keep consuming code points that extend the grapheme cluster */
    while (ptr_next && ptr_next[0])
    {
        next_codepoint = utf8_char_int (ptr_next);

        /* Regional indicators come in pairs (flags) */
        if (in_regional_pair && utf8_is_regional_indicator (next_codepoint))
        {
            /* Consume the second regional indicator */
            ptr_next = utf8_next_char (ptr_next);
            in_regional_pair = 0;
            continue;
        }

        /* Check if next code point extends the grapheme cluster */
        if (utf8_is_grapheme_extender (next_codepoint))
        {
            /* After ZWJ, consume the next character too */
            if (utf8_is_zwj (next_codepoint))
            {
                ptr_next = utf8_next_char (ptr_next);
                if (ptr_next && ptr_next[0])
                {
                    /* Continue to potentially consume more extenders */
                    ptr_next = utf8_next_char (ptr_next);
                }
            }
            else
            {
                ptr_next = utf8_next_char (ptr_next);
            }
            continue;
        }

        /* No more extenders, we've reached the end of the grapheme cluster */
        break;
    }

    return ptr_next;
}

/*
 * Gets number of chars needed on screen to display a grapheme cluster.
 *
 * A grapheme cluster is displayed as a single unit, so complex emoji
 * sequences like ❤️‍🔥 should have width 2, not 4.
 *
 * Returns the number of chars (>= 0), or -1 for non-printable grapheme.
 */

int
utf8_grapheme_size_screen (const char *string)
{
    const char *ptr_next;
    int first_codepoint, codepoint;
    int width, has_vs16;
    wchar_t wc;

    if (!string || !string[0])
        return 0;

    /* Get the first code point */
    first_codepoint = utf8_char_int (string);

    /* Handle special cases */
    if (string[0] == '\t')
        return CONFIG_INTEGER(config_look_tab_width);

    if (((unsigned char)string[0]) < 32)
        return 1;

    /* Special non-printable chars */
    if ((first_codepoint == 0x00AD) || (first_codepoint == 0x200B))
        return -1;

    /* Find the end of the grapheme cluster */
    ptr_next = utf8_grapheme_next (string);
    if (!ptr_next)
        ptr_next = string + strlen (string);

    /*
     * For grapheme clusters, we need to determine the display width.
     * The rules are:
     * 1. If the cluster contains VS16 (emoji presentation), width is 2
     * 2. Regional indicator pairs (flags) have width 2
     * 3. Otherwise, use wcwidth of the base character
     */

    /* Check for VS16 and regional indicators in the cluster */
    has_vs16 = 0;

    /* Check if first char is regional indicator */
    if (utf8_is_regional_indicator (first_codepoint))
    {
        /* Regional indicator pair (flag) always has width 2 */
        return 2;
    }

    /* Scan through the cluster for VS16 */
    {
        const char *ptr_scan = utf8_next_char (string);
        while (ptr_scan && ptr_scan < ptr_next)
        {
            codepoint = utf8_char_int (ptr_scan);
            if (codepoint == UNICODE_VS16)
            {
                has_vs16 = 1;
                break;
            }
            ptr_scan = utf8_next_char (ptr_scan);
        }
    }

    /* If VS16 is present, emoji presentation selector forces width 2 */
    if (has_vs16)
        return 2;

    /* For other grapheme clusters, use wcwidth of the base character */
    wc = (wchar_t)first_codepoint;
    width = wcwidth (wc);

    /* If base character is an emoji (wide char), return 2 */
    if (width == 2)
        return 2;

    /* Single-width character with possible combining marks */
    if (width >= 0)
        return width;

    /* Fallback for unknown width */
    return 1;
}

/*
 * Gets the size in bytes of a grapheme cluster.
 *
 * Returns an integer >= 0.
 */

int
utf8_grapheme_size (const char *string)
{
    const char *ptr_next;

    if (!string || !string[0])
        return 0;

    ptr_next = utf8_grapheme_next (string);
    if (!ptr_next)
        return strlen (string);

    return ptr_next - string;
}

/*
 * Gets length of an UTF-8 string in number of grapheme clusters (not bytes).
 *
 * Returns length of string (>= 0).
 */

int
utf8_grapheme_strlen (const char *string)
{
    int length;

    if (!string)
        return 0;

    length = 0;
    while (string && string[0])
    {
        string = utf8_grapheme_next (string);
        length++;
    }
    return length;
}

/*
 * Gets number of chars needed on screen to display the UTF-8 string,
 * counting grapheme clusters properly.
 *
 * Returns the number of chars (>= 0).
 */

int
utf8_grapheme_strlen_screen (const char *string)
{
    int size_screen, size_screen_char;
    const char *ptr_string;

    if (!string)
        return 0;

    if (!local_utf8)
        return utf8_strlen (string);

    size_screen = 0;
    ptr_string = string;
    while (ptr_string && ptr_string[0])
    {
        size_screen_char = utf8_grapheme_size_screen (ptr_string);
        /* count only chars that use at least one column */
        if (size_screen_char > 0)
            size_screen += size_screen_char;
        ptr_string = utf8_grapheme_next (ptr_string);
    }

    return size_screen;
}


/*
 * Initializes UTF-8 in WeeChat.
 */

void
utf8_init (void)
{
    local_utf8 = (string_strcasecmp (weechat_local_charset, "utf-8") == 0);
}

/*
 * Checks if a string has some 8-bit chars.
 *
 * Returns:
 *   1: string has 8-bit chars
 *   0: string has only 7-bit chars
 */

int
utf8_has_8bits (const char *string)
{
    while (string && string[0])
    {
        if (string[0] & 0x80)
            return 1;
        string++;
    }
    return 0;
}

/*
 * Checks if a string is UTF-8 valid.
 *
 * If length is <= 0, checks whole string.
 * If length is > 0, checks only this number of chars (not bytes).
 *
 * Returns:
 *   1: string is UTF-8 valid
 *   0: string it not UTF-8 valid, and then if error is not NULL, it is set
 *      with first non valid UTF-8 char in string
 */

int
utf8_is_valid (const char *string, int length, char **error)
{
    int code_point, current_char;

    current_char = 0;

    while (string && string[0]
           && ((length <= 0) || (current_char < length)))
    {
        /*
         * UTF-8, 2 bytes, should be: 110vvvvv 10vvvvvv
         * and in range: U+0080 - U+07FF
         */
        if (((unsigned char)(string[0]) & 0xE0) == 0xC0)
        {
            if (!string[1] || (((unsigned char)(string[1]) & 0xC0) != 0x80))
                goto invalid;
            code_point = utf8_char_int (string);
            if ((code_point < 0x0080) || (code_point > 0x07FF))
                goto invalid;
            string += 2;
        }
        /*
         * UTF-8, 3 bytes, should be: 1110vvvv 10vvvvvv 10vvvvvv
         * and in range: U+0800 - U+FFFF
         * (note: high and low surrogate halves used by UTF-16 (U+D800 through
         * U+DFFF) are not legal Unicode values)
         */
        else if (((unsigned char)(string[0]) & 0xF0) == 0xE0)
        {
            if (!string[1] || !string[2]
                || (((unsigned char)(string[1]) & 0xC0) != 0x80)
                || (((unsigned char)(string[2]) & 0xC0) != 0x80))
            {
                goto invalid;
            }
            code_point = utf8_char_int (string);
            if ((code_point < 0x0800)
                || (code_point > 0xFFFF)
                || ((code_point >= 0xD800) && (code_point <= 0xDFFF)))
            {
                goto invalid;
            }
            string += 3;
        }
        /*
         * UTF-8, 4 bytes, should be: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv
         * and in range: U+10000 - U+1FFFFF
         */
        else if (((unsigned char)(string[0]) & 0xF8) == 0xF0)
        {
            if (!string[1] || !string[2] || !string[3]
                || (((unsigned char)(string[1]) & 0xC0) != 0x80)
                || (((unsigned char)(string[2]) & 0xC0) != 0x80)
                || (((unsigned char)(string[3]) & 0xC0) != 0x80))
            {
                goto invalid;
            }
            code_point = utf8_char_int (string);
            if ((code_point < 0x10000) || (code_point > 0x1FFFFF))
                goto invalid;
            string += 4;
        }
        /* UTF-8, 1 byte, should be: 0vvvvvvv */
        else if ((unsigned char)(string[0]) >= 0x80)
            goto invalid;
        else
            string++;
        current_char++;
    }
    if (error)
        *error = NULL;
    return 1;

invalid:
    if (error)
        *error = (char *)string;
    return 0;
}

/*
 * Normalizes an string: removes non UTF-8 chars and replaces them by a
 * "replacement" char.
 */

void
utf8_normalize (char *string, char replacement)
{
    char *error;

    while (string && string[0])
    {
        if (utf8_is_valid (string, -1, &error))
            return;
        error[0] = replacement;
        string = error + 1;
    }
}

/*
 * Gets pointer to previous UTF-8 char in a string.
 *
 * Returns pointer to previous UTF-8 char, NULL if not found (for example
 * "string_start" was reached).
 */

const char *
utf8_prev_char (const char *string_start, const char *string)
{
    if (!string || (string <= string_start))
        return NULL;

    string--;

    if (((unsigned char)(string[0]) & 0xC0) == 0x80)
    {
        /* UTF-8, at least 2 bytes */
        string--;
        if (string < string_start)
            return string + 1;
        if (((unsigned char)(string[0]) & 0xC0) == 0x80)
        {
            /* UTF-8, at least 3 bytes */
            string--;
            if (string < string_start)
                return string + 1;
            if (((unsigned char)(string[0]) & 0xC0) == 0x80)
            {
                /* UTF-8, 4 bytes */
                string--;
                if (string < string_start)
                    return string + 1;
                return string;
            }
            else
            {
                return string;
            }
        }
        else
        {
            return string;
        }
    }
    return string;
}

/*
 * Gets pointer to next UTF-8 char in a string.
 *
 * Returns pointer to next UTF-8 char, NULL if string was NULL.
 */

const char *
utf8_next_char (const char *string)
{
    if (!string || !string[0])
        return NULL;

    /* UTF-8, 2 bytes: 110vvvvv 10vvvvvv */
    if (((unsigned char)(string[0]) & 0xE0) == 0xC0)
    {
        if (!string[1])
            return string + 1;
        return string + 2;
    }
    /* UTF-8, 3 bytes: 1110vvvv 10vvvvvv 10vvvvvv */
    else if (((unsigned char)(string[0]) & 0xF0) == 0xE0)
    {
        if (!string[1])
            return string + 1;
        if (!string[2])
            return string + 2;
        return string + 3;
    }
    /* UTF-8, 4 bytes: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
    else if (((unsigned char)(string[0]) & 0xF8) == 0xF0)
    {
        if (!string[1])
            return string + 1;
        if (!string[2])
            return string + 2;
        if (!string[3])
            return string + 3;
        return string + 4;
    }
    /* UTF-8, 1 byte: 0vvvvvvv */
    return string + 1;
}

/*
 * Gets pointer to the beginning of the UTF-8 line in a string.
 *
 * Returns pointer to the beginning of the UTF-8 line, NULL if string was NULL.
 */

const char *
utf8_beginning_of_line (const char *string_start, const char *string)
{
    if (string && (string[0] == '\n'))
        string = utf8_prev_char (string_start, string);

    while (string && (string[0] != '\n'))
    {
        string = utf8_prev_char (string_start, string);
    }

    if (string)
        return utf8_next_char (string);

    return string_start;
}

/*
 * Gets pointer to the end of the UTF-8 line in a string.
 *
 * Returns pointer to the end of the UTF-8 line, NULL if string was NULL.
 */

const char *
utf8_end_of_line (const char *string)
{
    if (!string)
        return NULL;

    while (string && string[0] && (string[0] != '\n'))
    {
        string = utf8_next_char (string);
    }

    return string;
}

/*
 * Gets UTF-8 char as an integer.
 *
 * Returns the UTF-8 char as integer number.
 */

int
utf8_char_int (const char *string)
{
    const unsigned char *ptr_string;

    if (!string)
        return 0;

    ptr_string = (unsigned char *)string;

    /* UTF-8, 2 bytes: 110vvvvv 10vvvvvv */
    if ((ptr_string[0] & 0xE0) == 0xC0)
    {
        if (!ptr_string[1])
            return (int)(ptr_string[0] & 0x1F);
        return ((int)(ptr_string[0] & 0x1F) << 6) +
            ((int)(ptr_string[1] & 0x3F));
    }
    /* UTF-8, 3 bytes: 1110vvvv 10vvvvvv 10vvvvvv */
    else if ((ptr_string[0] & 0xF0) == 0xE0)
    {
        if (!ptr_string[1])
            return (int)(ptr_string[0] & 0x0F);
        if (!ptr_string[2])
            return (((int)(ptr_string[0] & 0x0F)) << 6) +
                ((int)(ptr_string[1] & 0x3F));
        return (((int)(ptr_string[0] & 0x0F)) << 12) +
            (((int)(ptr_string[1] & 0x3F)) << 6) +
            ((int)(ptr_string[2] & 0x3F));
    }
    /* UTF-8, 4 bytes: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
    else if ((ptr_string[0] & 0xF8) == 0xF0)
    {
        if (!ptr_string[1])
            return (int)(ptr_string[0] & 0x07);
        if (!ptr_string[2])
            return (((int)(ptr_string[0] & 0x07)) << 6) +
                ((int)(ptr_string[1] & 0x3F));
        if (!ptr_string[3])
            return (((int)(ptr_string[0] & 0x07)) << 12) +
                (((int)(ptr_string[1] & 0x3F)) << 6) +
                ((int)(ptr_string[2] & 0x3F));
        return (((int)(ptr_string[0] & 0x07)) << 18) +
            (((int)(ptr_string[1] & 0x3F)) << 12) +
            (((int)(ptr_string[2] & 0x3F)) << 6) +
            ((int)(ptr_string[3] & 0x3F));
    }
    /* UTF-8, 1 byte: 0vvvvvvv */
    return (int)ptr_string[0];
}

/*
 * Converts a unicode char (as unsigned integer) to a string.
 *
 * The string must have a size >= 5
 * (4 bytes for the UTF-8 char + the final '\0').
 *
 * In case of error (if unicode value is > 0x1FFFFF), the string is set to an
 * empty string (string[0] == '\0').
 *
 * Returns the number of bytes in the UTF-8 char (not counting the final '\0').
 */

int
utf8_int_string (unsigned int unicode_value, char *string)
{
    int num_bytes;

    num_bytes = 0;

    if (!string)
        return num_bytes;

    string[0] = '\0';

    if (unicode_value == 0)
    {
        /* NUL char */
    }
    else if (unicode_value <= 0x007F)
    {
        /* UTF-8, 1 byte: 0vvvvvvv */
        string[0] = unicode_value;
        string[1] = '\0';
        num_bytes = 1;
    }
    else if (unicode_value <= 0x07FF)
    {
        /* UTF-8, 2 bytes: 110vvvvv 10vvvvvv */
        string[0] = 0xC0 | ((unicode_value >> 6) & 0x1F);
        string[1] = 0x80 | (unicode_value & 0x3F);
        string[2] = '\0';
        num_bytes = 2;
    }
    else if (unicode_value <= 0xFFFF)
    {
        /* UTF-8, 3 bytes: 1110vvvv 10vvvvvv 10vvvvvv */
        string[0] = 0xE0 | ((unicode_value >> 12) & 0x0F);
        string[1] = 0x80 | ((unicode_value >> 6) & 0x3F);
        string[2] = 0x80 | (unicode_value & 0x3F);
        string[3] = '\0';
        num_bytes = 3;
    }
    else if (unicode_value <= 0x1FFFFF)
    {
        /* UTF-8, 4 bytes: 11110vvv 10vvvvvv 10vvvvvv 10vvvvvv */
        string[0] = 0xF0 | ((unicode_value >> 18) & 0x07);
        string[1] = 0x80 | ((unicode_value >> 12) & 0x3F);
        string[2] = 0x80 | ((unicode_value >> 6) & 0x3F);
        string[3] = 0x80 | (unicode_value & 0x3F);
        string[4] = '\0';
        num_bytes = 4;
    }

    return num_bytes;
}

/*
 * Gets size of UTF-8 char (in bytes).
 *
 * Returns an integer between 0 and 4.
 */

int
utf8_char_size (const char *string)
{
    const char *ptr_next;

    if (!string || !string[0])
        return 0;

    ptr_next = utf8_next_char (string);
    if (!ptr_next)
        return 0;

    return ptr_next - string;
}

/*
 * Gets length of an UTF-8 string in number of chars (not bytes).
 * Result is <= strlen (string).
 *
 * Returns length of string (>= 0).
 */

int
utf8_strlen (const char *string)
{
    int length;

    if (!string)
        return 0;

    length = 0;
    while (string && string[0])
    {
        string = utf8_next_char (string);
        length++;
    }
    return length;
}

/*
 * Gets length of an UTF-8 string for N bytes max in string.
 *
 * Returns length of string (>= 0).
 */

int
utf8_strnlen (const char *string, int bytes)
{
    char *start;
    int length;

    if (!string)
        return 0;

    start = (char *)string;
    length = 0;
    while (string && string[0] && (string - start < bytes))
    {
        string = utf8_next_char (string);
        length++;
    }
    return length;
}

/*
 * Gets number of chars needed on screen to display the UTF-8 char.
 *
 * Returns the number of chars (>= 0).
 */

int
utf8_char_size_screen (const char *string)
{
    wchar_t codepoint;

    if (!string || !string[0])
        return 0;

    if (string[0] == '\t')
        return CONFIG_INTEGER(config_look_tab_width);

    /*
     * chars < 32 are displayed with a letter/symbol and reverse video,
     * so exactly one column
     */
    if (((unsigned char)string[0]) < 32)
        return 1;

    codepoint = (wchar_t)utf8_char_int (string);

    /*
     * special chars not displayed (because not handled by WeeChat):
     *   U+00AD: soft hyphen      (wcwidth == 1)
     *   U+200B: zero width space (wcwidth == 0)
     */
    if ((codepoint == 0x00AD) || (codepoint == 0x200B))
    {
        return -1;
    }

    return wcwidth (codepoint);
}

/*
 * Gets number of chars needed on screen to display the UTF-8 string.
 *
 * Returns the number of chars (>= 0).
 */

int
utf8_strlen_screen (const char *string)
{
    int size_screen, size_screen_char;
    const char *ptr_string;

    if (!string)
        return 0;

    if (!local_utf8)
        return utf8_strlen (string);

    size_screen = 0;
    ptr_string = string;
    while (ptr_string && ptr_string[0])
    {
        size_screen_char = utf8_char_size_screen (ptr_string);
        /* count only chars that use at least one column */
        if (size_screen_char > 0)
            size_screen += size_screen_char;
        ptr_string = utf8_next_char (ptr_string);
    }

    return size_screen;
}

/*
 * Moves forward N chars in an UTF-8 string.
 *
 * Returns pointer to the new position in string.
 */

const char *
utf8_add_offset (const char *string, int offset)
{
    if (!string)
        return NULL;

    while (string && string[0] && (offset > 0))
    {
        string = utf8_next_char (string);
        offset--;
    }
    return string;
}

/*
 * Gets real position in UTF-8 string, in bytes.
 *
 * Argument "pos" is a number of chars (not bytes).
 *
 * Example: ("déca", 2) returns 3.
 *
 * Returns the real position (>= 0).
 */

int
utf8_real_pos (const char *string, int pos)
{
    int count, real_pos;
    const char *next_char;

    if (!string)
        return pos;

    count = 0;
    real_pos = 0;
    while (string && string[0] && (count < pos))
    {
        next_char = utf8_next_char (string);
        real_pos += (next_char - string);
        string = next_char;
        count++;
    }
    return real_pos;
}

/*
 * Gets position in UTF-8 string, in chars.
 *
 * Argument "real_pos" is a number of bytes (not chars).
 *
 * Example: ("déca", 3) returns 2.
 *
 * Returns the position in string.
 */

int
utf8_pos (const char *string, int real_pos)
{
    int count;
    char *limit;

    if (!string || !weechat_local_charset)
        return real_pos;

    count = 0;
    limit = (char *)string + real_pos;
    while (string && string[0] && (string < limit))
    {
        string = utf8_next_char (string);
        count++;
    }
    return count;
}

/*
 * Duplicates an UTF-8 string, with max N chars.
 *
 * Note: result must be freed after use.
 */

char *
utf8_strndup (const char *string, int length)
{
    const char *end;

    if (!string || (length < 0))
        return NULL;

    if (length == 0)
        return strdup ("");

    end = utf8_add_offset (string, length);
    if (!end || (end == string))
        return strdup (string);

    return string_strndup (string, end - string);
}

/*
 * Copies max N chars from a string to another and adds null byte at the end.
 *
 * Note: the target string "dest" must be long enough.
 */

void
utf8_strncpy (char *dest, const char *string, int length)
{
    const char *end;

    if (!dest)
        return;

    dest[0] = '\0';

    if (!string || (length <= 0))
        return;

    end = utf8_add_offset (string, length);
    if (!end || (end == string))
        return;

    memcpy (dest, string, end - string);
    dest[end - string] = '\0';
}
