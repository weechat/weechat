/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* IRC color decoding/encoding in messages */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-color.h"
#include "irc-config.h"


char *irc_color_to_weechat[IRC_NUM_COLORS] =
{ /*     0 */ "white",
  /*     1 */ "black",
  /*     2 */ "blue",
  /*     3 */ "green",
  /*     4 */ "lightred",
  /*     5 */ "red",
  /*     6 */ "magenta",
  /*     7 */ "brown",
  /*     8 */ "yellow",
  /*     9 */ "lightgreen",
  /*    10 */ "cyan",
  /*    11 */ "lightcyan",
  /*    12 */ "lightblue",
  /*    13 */ "lightmagenta",
  /*    14 */ "darkgray",
  /*    15 */ "gray",
  /* 16-23 */  "52",  "94", "100",  "58",  "22",  "29",  "23",  "24",
  /* 24-31 */  "17",  "54",  "53",  "89",  "88", "130", "142",  "64",
  /* 32-39 */  "28",  "35",  "30",  "25",  "18",  "91",  "90", "125",
  /* 40-47 */ "124", "166", "184", "106",  "34",  "49",  "37",  "33",
  /* 48-55 */  "19", "129", "127", "161", "196", "208", "226", "154",
  /* 56-63 */  "46",  "86",  "51",  "75",  "21", "171", "201", "198",
  /* 64-71 */ "203", "215", "227", "191",  "83", "122",  "87", "111",
  /* 72-79 */  "63", "177", "207", "205", "217", "223", "229", "193",
  /* 80-87 */ "157", "158", "159", "153", "147", "183", "219", "212",
  /* 88-95 */  "16", "233", "235", "237", "239", "241", "244", "247",
  /* 96-98 */ "250", "254", "231",
  /*    99 */ "default",
};
char irc_color_term2irc[IRC_COLOR_TERM2IRC_NUM_COLORS] =
{        /* term > IRC               */
    1,   /*   0     1 (black)        */
    5,   /*   1     5 (red)          */
    3,   /*   2     3 (green)        */
    7,   /*   3     7 (brown)        */
    2,   /*   4     2 (blue)         */
    6,   /*   5     6 (magenta)      */
    10,  /*   6    10 (cyan)         */
    15,  /*   7    15 (gray)         */
    14,  /*   8    14 (darkgray)     */
    4,   /*   9     4 (lightred)     */
    9,   /*  10     9 (lightgreen)   */
    8,   /*  11     8 (yellow)       */
    12,  /*  12    12 (lightblue)    */
    13,  /*  13    13 (lightmagenta) */
    11,  /*  14    11 (lightcyan)    */
    0,   /*  15     0 (white)        */
};
int irc_color_index_string_decoded = 0;
char *irc_color_string_decoded[32];


/*
 * Convert a RGB color to terminal color.
 *
 * Return a terminal color (between 0 and 255), -1 if error.
 */

int
irc_color_convert_rgb2term (long rgb)
{
    char str_color[64], *info_color;
    int number;

    if (rgb < 0)
        return -1;

    snprintf (str_color, sizeof (str_color), "%ld", rgb);

    info_color = weechat_info_get ("color_rgb2term", str_color);
    if (!info_color || !info_color[0])
    {
        free (info_color);
        return -1;
    }

    if (!weechat_util_parse_int (info_color, 10, &number))
    {
        free (info_color);
        return -1;
    }

    free (info_color);

    return number;
}

/*
 * Convert a RGB color to IRC color.
 *
 * Return a IRC color number (between 0 and 15), -1 if error.
 */

int
irc_color_convert_rgb2irc (long rgb)
{
    char str_color[64], *info_color;
    int number;

    if (rgb < 0)
        return -1;

    snprintf (str_color, sizeof (str_color),
              "%ld,%d",
              rgb,
              IRC_COLOR_TERM2IRC_NUM_COLORS);

    info_color = weechat_info_get ("color_rgb2term", str_color);
    if (!info_color || !info_color[0])
    {
        free (info_color);
        return -1;
    }

    if (!weechat_util_parse_int (info_color, 10, &number)
        || (number < 0) || (number >= IRC_COLOR_TERM2IRC_NUM_COLORS))
    {
        free (info_color);
        return -1;
    }

    free (info_color);

    return irc_color_term2irc[number];
}

/*
 * Convert a terminal color to IRC color.
 *
 * Return a IRC color number (between 0 and 15), -1 if error.
 */

int
irc_color_convert_term2irc (int color)
{
    char str_color[64], *info_color;
    long number;

    snprintf (str_color, sizeof (str_color), "%d", color);

    info_color = weechat_info_get ("color_term2rgb", str_color);
    if (!info_color || !info_color[0])
    {
        free (info_color);
        return -1;
    }

    if (!weechat_util_parse_long (info_color, 10, &number)
        || (number < 0) || (number > 0xFFFFFF))
    {
        free (info_color);
        return -1;
    }

    free (info_color);

    return irc_color_convert_rgb2irc (number);
}

/*
 * Check if a string starts with a RGB color of IRC hex colors: exactly 6
 * hexadecimal digits ("RRGGBB").
 *
 * Return:
 *   1: string starts with 6 hexadecimal digits
 *   0: string does not start with 6 hexadecimal digits
 */

int
irc_color_is_hex_color (const unsigned char *string)
{
    int i;

    for (i = 0; i < 6; i++)
    {
        if (!isxdigit (string[i]))
            return 0;
    }

    return 1;
}

/*
 * Get the value of a hex color: 6 hexadecimal digits at the beginning of a
 * string (checked with function irc_color_is_hex_color).
 *
 * Return the RGB value (0x000000 to 0xFFFFFF).
 */

int
irc_color_hex_color_value (const unsigned char *string)
{
    int i, value;

    value = 0;
    for (i = 0; i < 6; i++)
    {
        value = (value << 4)
            | (isdigit (string[i]) ?
               string[i] - '0' : tolower (string[i]) - 'a' + 10);
    }

    return value;
}

/*
 * Parse an IRC color code at the beginning of a string: "^C" followed by an
 * optional color ("NN" or "NN,MM", with 1 or 2 digits), or "^D" followed by
 * an optional hex color ("RRGGBB" or "RRGGBB,RRGGBB"); a background is read
 * only if a foreground is given (this function is used to decode, encode and
 * split messages, so that they all read the same codes).
 *
 * If not NULL, "fg" and "bg" are set to the colors of the code: IRC color
 * (0-99), RGB color with flag IRC_COLOR_RGB_FLAG (for "^D"), or
 * IRC_COLOR_FORMATTING_NONE if the code has no such color.
 *
 * Return the size of the code, 0 if the string does not start with "^C" or
 * "^D".
 */

int
irc_color_code_parse (const char *string, int *fg, int *bg)
{
    const unsigned char *ptr_string;
    int color_fg, color_bg;

    color_fg = IRC_COLOR_FORMATTING_NONE;
    color_bg = IRC_COLOR_FORMATTING_NONE;

    if (fg)
        *fg = color_fg;
    if (bg)
        *bg = color_bg;

    if (!string)
        return 0;

    ptr_string = (const unsigned char *)string;

    switch (ptr_string[0])
    {
        case IRC_COLOR_COLOR_CHAR:
            ptr_string++;
            if (isdigit (ptr_string[0]))
            {
                color_fg = ptr_string[0] - '0';
                ptr_string++;
                if (isdigit (ptr_string[0]))
                {
                    color_fg = (color_fg * 10) + ptr_string[0] - '0';
                    ptr_string++;
                }
                if ((ptr_string[0] == ',') && isdigit (ptr_string[1]))
                {
                    color_bg = ptr_string[1] - '0';
                    ptr_string += 2;
                    if (isdigit (ptr_string[0]))
                    {
                        color_bg = (color_bg * 10) + ptr_string[0] - '0';
                        ptr_string++;
                    }
                }
            }
            break;
        case IRC_COLOR_HEX_COLOR_CHAR:
            ptr_string++;
            if (irc_color_is_hex_color (ptr_string))
            {
                color_fg = irc_color_hex_color_value (ptr_string)
                    | IRC_COLOR_RGB_FLAG;
                ptr_string += 6;
                if ((ptr_string[0] == ',')
                    && irc_color_is_hex_color (ptr_string + 1))
                {
                    color_bg = irc_color_hex_color_value (ptr_string + 1)
                        | IRC_COLOR_RGB_FLAG;
                    ptr_string += 7;
                }
            }
            break;
        default:
            return 0;
    }

    if (fg)
        *fg = color_fg;
    if (bg)
        *bg = color_bg;

    return ptr_string - (const unsigned char *)string;
}

/*
 * Get the size of the char at the beginning of a string: the size of a valid
 * UTF-8 char, or 1 for a byte which is not valid UTF-8, so that it can not
 * include the beginning of an IRC code after it.
 *
 * Return the size of the char (1 at the end of string).
 */

int
irc_color_char_size (const char *string)
{
    int size;

    if (((unsigned char)string[0] < 0x80)
        || !weechat_utf8_is_valid (string, 1, NULL))
    {
        return 1;
    }

    size = weechat_utf8_char_size (string);

    return (size > 0) ? size : 1;
}

/*
 * Get the size of an IRC color code at the beginning of a string (see
 * function irc_color_code_parse).
 *
 * Return the size of the code, 0 if the string does not start with "^C" or
 * "^D".
 */

int
irc_color_code_size (const char *string)
{
    return irc_color_code_parse (string, NULL, NULL);
}

/*
 * Initialize formatting: no attribute, no color.
 */

void
irc_color_formatting_init (struct t_irc_color_formatting *formatting)
{
    if (!formatting)
        return;

    memset (formatting, 0, sizeof (*formatting));
    formatting->fg = IRC_COLOR_FORMATTING_NONE;
    formatting->bg = IRC_COLOR_FORMATTING_NONE;
}

/*
 * Set colors with the colors of an IRC color code (see function
 * irc_color_code_parse): a code without color sets both colors to "reset",
 * a text color alone keeps the background.
 */

void
irc_color_formatting_set_colors (int *fg, int *bg, int code_fg, int code_bg,
                                 int reset)
{
    if (code_fg == IRC_COLOR_FORMATTING_NONE)
    {
        *fg = reset;
        *bg = reset;
        return;
    }

    *fg = code_fg;
    if (code_bg != IRC_COLOR_FORMATTING_NONE)
        *bg = code_bg;
}

/*
 * Update formatting with the IRC formatting codes in a string (the first
 * "length" bytes of string): attributes are toggled, colors are set or
 * reset ("^C" or "^D" alone resets colors, a text color alone keeps the
 * background), "^O" resets everything.
 */

void
irc_color_formatting_update (struct t_irc_color_formatting *formatting,
                             const char *string, int length)
{
    const char *ptr_string, *ptr_end;
    int size, fg, bg;

    if (!formatting || !string)
        return;

    ptr_string = string;
    ptr_end = string + length;
    while ((ptr_string < ptr_end) && ptr_string[0])
    {
        switch (ptr_string[0])
        {
            case IRC_COLOR_BOLD_CHAR:
                formatting->bold ^= 1;
                break;
            case IRC_COLOR_REVERSE_CHAR:
                formatting->reverse ^= 1;
                break;
            case IRC_COLOR_ITALIC_CHAR:
                formatting->italic ^= 1;
                break;
            case IRC_COLOR_STRIKETHROUGH_CHAR:
                formatting->strikethrough ^= 1;
                break;
            case IRC_COLOR_UNDERLINE_CHAR:
                formatting->underline ^= 1;
                break;
            case IRC_COLOR_MONOSPACE_CHAR:
                formatting->monospace ^= 1;
                break;
            case IRC_COLOR_RESET_CHAR:
                irc_color_formatting_init (formatting);
                break;
            case IRC_COLOR_COLOR_CHAR:
            case IRC_COLOR_HEX_COLOR_CHAR:
                size = irc_color_code_parse (ptr_string, &fg, &bg);
                /* Code cut at the end of string: ignore it. */
                if (ptr_string + size > ptr_end)
                    return;
                irc_color_formatting_set_colors (&formatting->fg,
                                                 &formatting->bg, fg, bg,
                                                 IRC_COLOR_FORMATTING_NONE);
                ptr_string += size;
                continue;
        }
        ptr_string++;
    }
}

/*
 * Add a color to IRC codes: IRC color number on 2 digits (for "^C"), or RGB
 * color on 6 hexadecimal digits (for "^D", if color has flag
 * IRC_COLOR_RGB_FLAG).
 */

void
irc_color_formatting_add_color (char **output, int color)
{
    char str_color[16];

    if (color & IRC_COLOR_RGB_FLAG)
        snprintf (str_color, sizeof (str_color), "%06X", color & 0xFFFFFF);
    else
        snprintf (str_color, sizeof (str_color), "%02d", color);
    weechat_string_dyn_concat (output, str_color, -1);
}

/*
 * Add an IRC color code with a text color and an optional background: "^C"
 * for IRC colors, "^D" for RGB colors (both colors must be of the same kind).
 */

void
irc_color_formatting_add_code (char **output, int fg, int bg)
{
    weechat_string_dyn_concat (
        output,
        (fg & IRC_COLOR_RGB_FLAG) ? IRC_COLOR_HEX_COLOR_STR : IRC_COLOR_COLOR_STR,
        -1);
    irc_color_formatting_add_color (output, fg);
    if (bg != IRC_COLOR_FORMATTING_NONE)
    {
        weechat_string_dyn_concat (output, ",", -1);
        irc_color_formatting_add_color (output, bg);
    }
}

/*
 * Add the IRC codes that change the formatting "from" to the formatting "to":
 * codes for the attributes which differ (unless "colors_only" is 1), then a
 * color code if colors differ.
 *
 * Colors are only set (IRC_COLOR_FORMATTING_NONE in "to" means "no color
 * code"), and a missing text color in "to" is sent as the default color
 * (with a background).
 * A background is set only with a text color: with colors of different kinds,
 * the background is set with a text color of the same kind ("^C99,bg" or
 * "^DFFFFFF,bg"), then the text color is set (it keeps the background).
 * A background reset to the default color is sent as a color reset followed
 * by the text color ("^C^Cfg"), so that the colors remapped with the default
 * background (option irc.color.mirc_remap) apply.
 *
 * Return the kind of the last code added if it has only a text color (then
 * text starting with "," and a color would be read as a background, see
 * function irc_color_formatting_add_separator): 1 for "^Cfg", 2 for "^Dfg",
 * 0 otherwise.
 */

int
irc_color_formatting_transition (char **output,
                                 const struct t_irc_color_formatting *from,
                                 const struct t_irc_color_formatting *to,
                                 int colors_only)
{
    int fg, bg;

    if (!output || !from || !to)
        return 0;

    if (!colors_only)
    {
        if (from->bold != to->bold)
            weechat_string_dyn_concat (output, IRC_COLOR_BOLD_STR, -1);
        if (from->reverse != to->reverse)
            weechat_string_dyn_concat (output, IRC_COLOR_REVERSE_STR, -1);
        if (from->italic != to->italic)
            weechat_string_dyn_concat (output, IRC_COLOR_ITALIC_STR, -1);
        if (from->strikethrough != to->strikethrough)
            weechat_string_dyn_concat (output, IRC_COLOR_STRIKETHROUGH_STR, -1);
        if (from->underline != to->underline)
            weechat_string_dyn_concat (output, IRC_COLOR_UNDERLINE_STR, -1);
        if (from->monospace != to->monospace)
            weechat_string_dyn_concat (output, IRC_COLOR_MONOSPACE_STR, -1);
    }

    /* Text color sent: a missing color is the default color. */
    fg = (to->fg >= 0) ? to->fg : IRC_COLOR_DEFAULT;

    if ((to->bg != from->bg) && (to->bg != IRC_COLOR_FORMATTING_NONE))
    {
        bg = (to->bg >= 0) ? to->bg : IRC_COLOR_DEFAULT;
        if ((bg == IRC_COLOR_DEFAULT)
            && (from->bg != IRC_COLOR_FORMATTING_NONE)
            && (to->fg >= 0) && (to->fg != IRC_COLOR_DEFAULT))
        {
            weechat_string_dyn_concat (output, IRC_COLOR_COLOR_STR, -1);
            irc_color_formatting_add_code (output, fg,
                                           IRC_COLOR_FORMATTING_NONE);
            return (fg & IRC_COLOR_RGB_FLAG) ? 2 : 1;
        }
        if ((fg & IRC_COLOR_RGB_FLAG) == (bg & IRC_COLOR_RGB_FLAG))
        {
            irc_color_formatting_add_code (output, fg, bg);
            return 0;
        }
        irc_color_formatting_add_code (
            output,
            (bg & IRC_COLOR_RGB_FLAG) ? (int)(0xFFFFFF | IRC_COLOR_RGB_FLAG) : IRC_COLOR_DEFAULT,
            bg);
        irc_color_formatting_add_code (output, fg, IRC_COLOR_FORMATTING_NONE);
        return (fg & IRC_COLOR_RGB_FLAG) ? 2 : 1;
    }

    if ((to->fg != from->fg) && (to->fg != IRC_COLOR_FORMATTING_NONE))
    {
        irc_color_formatting_add_code (output, fg, IRC_COLOR_FORMATTING_NONE);
        return (fg & IRC_COLOR_RGB_FLAG) ? 2 : 1;
    }

    return 0;
}

/*
 * Add "^B^B" (empty bold) before text if the last code added has only a text
 * color ("fg_only" returned by function irc_color_formatting_transition) and
 * if the text starts with "," and a color, which would be read as a
 * background.
 *
 * The text after "," is a color if it starts with a digit for "^Cfg", or a
 * hexadecimal digit for "^Dfg" (some IRC clients read 1 to 6 hexadecimal
 * digits). If "length" is negative, "text" is the complete text after the
 * code; otherwise "text" is only the next "length" bytes, the rest may come
 * later: a "," at the end of the text is enough.
 */

void
irc_color_formatting_add_separator (char **output, int fg_only,
                                    const char *text, int length)
{
    int separator;

    if (!output || !fg_only || !text || (text[0] != ','))
        return;

    separator = (length == 1)
        || ((fg_only == 1) && isdigit ((unsigned char)text[1]))
        || ((fg_only == 2) && isxdigit ((unsigned char)text[1]));

    if (separator)
    {
        weechat_string_dyn_concat (output, IRC_COLOR_BOLD_STR, -1);
        weechat_string_dyn_concat (output, IRC_COLOR_BOLD_STR, -1);
    }
}

/*
 * Build the IRC codes that set the formatting from the default formatting,
 * for example at the beginning of the next part of a message split in
 * several messages; "next_text" is the text after the codes: if needed,
 * "^B^B" is added so that it can not be read as a background color.
 *
 * If "attributes_only" is 1, only the attributes are set (not the colors).
 *
 * Note: result must be freed after use.
 */

char *
irc_color_formatting_codes (struct t_irc_color_formatting *formatting,
                            const char *next_text, int attributes_only)
{
    struct t_irc_color_formatting formatting_default, formatting_to;
    char **codes;
    int fg_only;

    if (!formatting)
        return NULL;

    codes = weechat_string_dyn_alloc (32);
    if (!codes)
        return NULL;

    irc_color_formatting_init (&formatting_default);
    formatting_to = *formatting;
    if (attributes_only)
    {
        formatting_to.fg = IRC_COLOR_FORMATTING_NONE;
        formatting_to.bg = IRC_COLOR_FORMATTING_NONE;
    }
    fg_only = irc_color_formatting_transition (
        codes, &formatting_default, &formatting_to, 0);
    irc_color_formatting_add_separator (codes, fg_only, next_text, -1);

    return weechat_string_dyn_free (codes, 0);
}

/*
 * Replace IRC colors by WeeChat colors.
 *
 * If keep_colors == 0: removes any color/style in message otherwise keeps
 * colors.
 *
 * Note: result must be freed after use.
 */

char *
irc_color_decode (const char *string, int keep_colors)
{
    char **out;
    char str_fg[16], str_bg[16], str_color[128], str_key[128], str_to_add[128];
    const char *remapped_color;
    unsigned char *ptr_string;
    int length, fg, bg, fg_term, bg_term, current_bg_irc, current_bg_term;
    int bold, reverse, italic, strikethrough, underline, term_remap;

    if (!string)
        return NULL;

    length = strlen (string);
    out = weechat_string_dyn_alloc (length + (length / 2) + 1);
    if (!out)
        return NULL;

    /* Initialize attributes. */
    bold = 0;
    reverse = 0;
    italic = 0;
    strikethrough = 0;
    underline = 0;

    /*
     * Current background, used in key of remapped colors when a color code
     * has no background: -1 = default background, -2 = background set by
     * the other kind of color code (IRC color number or RGB color).
     */
    current_bg_irc = -1;
    current_bg_term = -1;

    ptr_string = (unsigned char *)string;
    while (ptr_string && ptr_string[0])
    {
        str_to_add[0] = '\0';
        switch (ptr_string[0])
        {
            case IRC_COLOR_BOLD_CHAR:
                if (keep_colors)
                {
                    snprintf (str_to_add, sizeof (str_to_add), "%s",
                              weechat_color ((bold) ? "-bold" : "bold"));
                }
                bold ^= 1;
                ptr_string++;
                break;
            case IRC_COLOR_RESET_CHAR:
                if (keep_colors)
                {
                    snprintf (str_to_add, sizeof (str_to_add), "%s",
                              weechat_color ("reset"));
                }
                bold = 0;
                reverse = 0;
                italic = 0;
                strikethrough = 0;
                underline = 0;
                current_bg_irc = -1;
                current_bg_term = -1;
                ptr_string++;
                break;
            case IRC_COLOR_REVERSE_CHAR:
                if (keep_colors)
                {
                    snprintf (str_to_add, sizeof (str_to_add), "%s",
                              weechat_color ((reverse) ? "-reverse" : "reverse"));
                }
                reverse ^= 1;
                ptr_string++;
                break;
            case IRC_COLOR_ITALIC_CHAR:
                if (keep_colors)
                {
                    snprintf (str_to_add, sizeof (str_to_add), "%s",
                              weechat_color ((italic) ? "-italic" : "italic"));
                }
                italic ^= 1;
                ptr_string++;
                break;
            case IRC_COLOR_STRIKETHROUGH_CHAR:
                if (keep_colors)
                {
                    snprintf (str_to_add, sizeof (str_to_add), "%s",
                              weechat_color ((strikethrough) ? "-dim" : "dim"));
                }
                strikethrough ^= 1;
                ptr_string++;
                break;
            case IRC_COLOR_UNDERLINE_CHAR:
                if (keep_colors)
                {
                    snprintf (str_to_add, sizeof (str_to_add), "%s",
                              weechat_color ((underline) ? "-underline" : "underline"));
                }
                underline ^= 1;
                ptr_string++;
                break;
            case IRC_COLOR_COLOR_CHAR:
                /*
                 * A background is read only if a foreground is given, so
                 * "^C,05" is a color reset followed by text ",05".
                 */
                ptr_string += irc_color_code_parse ((const char *)ptr_string,
                                                    &fg, &bg);
                if (keep_colors)
                {
                    if (fg != IRC_COLOR_FORMATTING_NONE)
                    {
                        if (bg >= 0)
                        {
                            /* IRC color 99 is the default background. */
                            current_bg_irc = (bg == 99) ? -1 : bg;
                            current_bg_term = (bg == 99) ? -1 : -2;
                        }
                        /*
                         * Search "fg,bg" in hashtable of remapped colors
                         * (with the current background if bg is not given).
                         */
                        snprintf (str_key, sizeof (str_key), "%d,%d",
                                  fg, (bg >= 0) ? bg : current_bg_irc);
                        remapped_color = weechat_hashtable_get (
                            irc_config_hashtable_color_mirc_remap,
                            str_key);
                        if (remapped_color)
                        {
                            snprintf (str_color, sizeof (str_color),
                                      "|%s", remapped_color);
                        }
                        else
                        {
                            snprintf (str_color, sizeof (str_color),
                                      "|%s%s%s",
                                      (fg >= 0) ? irc_color_to_weechat[fg] : "",
                                      (bg >= 0) ? "," : "",
                                      (bg >= 0) ? irc_color_to_weechat[bg] : "");
                        }
                        snprintf (str_to_add, sizeof (str_to_add), "%s",
                                  weechat_color (str_color));
                    }
                    else
                    {
                        current_bg_irc = -1;
                        current_bg_term = -1;
                        snprintf (str_to_add, sizeof (str_to_add), "%s",
                                  weechat_color ("resetcolor"));
                    }
                }
                break;
            case IRC_COLOR_HEX_COLOR_CHAR:
                /*
                 * Colors have exactly 6 hexadecimal digits ("RRGGBB"): "^D"
                 * followed by less digits is a color reset followed by text,
                 * and a background is read only if a foreground is given.
                 */
                ptr_string += irc_color_code_parse ((const char *)ptr_string,
                                                    &fg, &bg);
                if (keep_colors)
                {
                    if (fg != IRC_COLOR_FORMATTING_NONE)
                    {
                        str_bg[0] = '\0';
                        fg_term = -1;
                        bg_term = -1;
                        /*
                         * The RGB color is kept as-is (WeeChat displays it
                         * with the nearest terminal color if the terminal
                         * does not support direct colors); the terminal
                         * color is used only to search remapped colors
                         * (option irc.color.term_remap), so it is computed
                         * only if there are remapped colors.
                         */
                        term_remap = (weechat_hashtable_get_integer (
                                          irc_config_hashtable_color_term_remap,
                                          "items_count") > 0);
                        if (term_remap)
                            fg_term = irc_color_convert_rgb2term (fg & 0xFFFFFF);
                        snprintf (str_fg, sizeof (str_fg), "#%06x",
                                  fg & 0xFFFFFF);
                        if (bg != IRC_COLOR_FORMATTING_NONE)
                        {
                            if (term_remap)
                                bg_term = irc_color_convert_rgb2term (bg & 0xFFFFFF);
                            snprintf (str_bg, sizeof (str_bg), "#%06x",
                                      bg & 0xFFFFFF);
                            current_bg_irc = -2;
                            current_bg_term = bg_term;
                        }
                        /*
                         * Search "fg_term,bg_term" in hashtable of remapped
                         * colors (with the current background if bg_term is
                         * not given).
                         */
                        remapped_color = NULL;
                        if (term_remap)
                        {
                            snprintf (str_key, sizeof (str_key), "%d,%d",
                                      fg_term,
                                      (bg_term >= 0) ?
                                      bg_term : current_bg_term);
                            remapped_color = weechat_hashtable_get (
                                irc_config_hashtable_color_term_remap,
                                str_key);
                        }
                        if (remapped_color)
                        {
                            snprintf (str_color, sizeof (str_color),
                                      "|%s", remapped_color);
                        }
                        else
                        {
                            snprintf (str_color, sizeof (str_color),
                                      "|%s%s%s",
                                      str_fg,
                                      (str_bg[0]) ? "," : "",
                                      str_bg);
                        }
                        snprintf (str_to_add, sizeof (str_to_add), "%s",
                                  weechat_color (str_color));
                    }
                    else
                    {
                        current_bg_irc = -1;
                        current_bg_term = -1;
                        snprintf (str_to_add, sizeof (str_to_add), "%s",
                                  weechat_color ("resetcolor"));
                    }
                }
                break;
            default:
                /*
                 * We are not on an IRC color code, just copy the UTF-8 char
                 * into "str_to_add" (a byte which is not valid UTF-8 is
                 * copied alone, so that it can not include a code).
                 */
                length = irc_color_char_size ((const char *)ptr_string);
                memcpy (str_to_add, ptr_string, length);
                str_to_add[length] = '\0';
                ptr_string += length;
                break;
        }
        /* Add "str_to_add" (if not empty) to "out". */
        if (str_to_add[0])
            weechat_string_dyn_concat (out, str_to_add, -1);
    }

    return weechat_string_dyn_free (out, 0);
}

/*
 * Replace IRC colors by WeeChat colors and return a pointer to an allocated
 * string that doesn't need to be freed by the caller.
 *
 * If keep_colors == 0: removes any color/style in message otherwise keeps
 * colors.
 */

const char *
irc_color_decode_const (const char *string, int keep_colors)
{
    irc_color_index_string_decoded = (irc_color_index_string_decoded + 1) % 32;
    free (irc_color_string_decoded[irc_color_index_string_decoded]);
    irc_color_string_decoded[irc_color_index_string_decoded] = irc_color_decode (
        string, keep_colors);
    if (!irc_color_string_decoded[irc_color_index_string_decoded])
        irc_color_string_decoded[irc_color_index_string_decoded] = strdup ("");
    return (const char *)irc_color_string_decoded[irc_color_index_string_decoded];
}

/*
 * Replace color codes in command line by IRC color codes.
 *
 * If keep_colors == 0, remove any color/style in message, otherwise keeps
 * colors.
 *
 * Note: result must be freed after use.
 */

char *
irc_color_encode (const char *string, int keep_colors)
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
            case 0x02: /* ^B */
                if (keep_colors)
                    weechat_string_dyn_concat (out, IRC_COLOR_BOLD_STR, -1);
                ptr_string++;
                break;
            case 0x03: /* ^C */
            case 0x04: /* ^D */
                /* Color code with its colors: copied as-is (or removed). */
                length = irc_color_code_size ((const char *)ptr_string);
                if (keep_colors)
                {
                    weechat_string_dyn_concat (out, (const char *)ptr_string,
                                               length);
                }
                ptr_string += length;
                break;
            case 0x0F: /* ^O */
                if (keep_colors)
                    weechat_string_dyn_concat (out, IRC_COLOR_RESET_STR, -1);
                ptr_string++;
                break;
            case 0x16: /* ^V */
                if (keep_colors)
                    weechat_string_dyn_concat (out, IRC_COLOR_REVERSE_STR, -1);
                ptr_string++;
                break;
            case 0x1D: /* ^] */
                if (keep_colors)
                    weechat_string_dyn_concat (out, IRC_COLOR_ITALIC_STR, -1);
                ptr_string++;
                break;
            case 0x1E: /* ^^ */
                if (keep_colors)
                    weechat_string_dyn_concat (out, IRC_COLOR_STRIKETHROUGH_STR, -1);
                ptr_string++;
                break;
            case 0x1F: /* ^_ */
                if (keep_colors)
                    weechat_string_dyn_concat (out, IRC_COLOR_UNDERLINE_STR, -1);
                ptr_string++;
                break;
            default:
                /*
                 * A byte which is not valid UTF-8 is copied alone, so that it
                 * can not include a code.
                 */
                length = irc_color_char_size ((const char *)ptr_string);
                weechat_string_dyn_concat (out,
                                           (const char *)ptr_string,
                                           length);
                ptr_string += length;
        }
    }

    return weechat_string_dyn_free (out, 0);
}

/*
 * Read the next parameter (a number) of a normalized ANSI escape sequence
 * (see modifier "color_normalize_ansi"), and skip the separator ";" after it.
 *
 * Return 1 if a parameter was read, 0 if there is no more parameter or if
 * the parameter is invalid.
 */

int
irc_color_ansi_next_param (const char **ptr_param, const char *ptr_end,
                           int *value)
{
    const char *ptr_string;
    int number;

    ptr_string = *ptr_param;
    if ((ptr_string >= ptr_end) || !isdigit ((unsigned char)ptr_string[0]))
        return 0;

    number = 0;
    while ((ptr_string < ptr_end) && isdigit ((unsigned char)ptr_string[0]))
    {
        if (number < 1000000)
            number = (number * 10) + (ptr_string[0] - '0');
        ptr_string++;
    }
    if ((ptr_string < ptr_end) && (ptr_string[0] == ';'))
        ptr_string++;

    *ptr_param = ptr_string;
    *value = number;

    return 1;
}

/*
 * Read the color after parameter 38 or 48 of a normalized ANSI escape
 * sequence: "5;N" (terminal color) or "2;R;G;B" (RGB color).
 *
 * Return the color: terminal color (0-255), or RGB color with flag
 * IRC_COLOR_RGB_FLAG, -1 if the color is invalid.
 */

int
irc_color_ansi_next_color (const char **ptr_param, const char *ptr_end)
{
    int type, value, red, green, blue;

    if (!irc_color_ansi_next_param (ptr_param, ptr_end, &type))
        return -1;

    switch (type)
    {
        case 2:
            if (!irc_color_ansi_next_param (ptr_param, ptr_end, &red)
                || !irc_color_ansi_next_param (ptr_param, ptr_end, &green)
                || !irc_color_ansi_next_param (ptr_param, ptr_end, &blue)
                || (red > 255) || (green > 255) || (blue > 255))
            {
                return -1;
            }
            return IRC_COLOR_RGB_FLAG | (red << 16) | (green << 8) | blue;
        case 5:
            if (!irc_color_ansi_next_param (ptr_param, ptr_end, &value)
                || (value > 255))
            {
                return -1;
            }
            return value;
    }

    return -1;
}

/*
 * Reset state of ANSI colors: no attribute and default colors (like an IRC
 * client at the beginning of a message or after "^O").
 */

void
irc_color_decode_ansi_reset_state (struct t_irc_color_ansi_state *ansi_state)
{
    irc_color_formatting_init (&ansi_state->formatting);
    ansi_state->formatting.fg = IRC_COLOR_DEFAULT;
    ansi_state->formatting.bg = IRC_COLOR_DEFAULT;
    ansi_state->irc_fg = IRC_COLOR_DEFAULT;
    ansi_state->irc_bg = IRC_COLOR_DEFAULT;
    ansi_state->fg_only = 0;
}

/*
 * Set an attribute of ANSI state: if it changes, the IRC code of the
 * attribute (which toggles it) is added in output.
 */

void
irc_color_decode_ansi_set_attr (char **output, char *attribute, int value,
                                const char *code)
{
    if (*attribute == value)
        return;

    weechat_string_dyn_concat (output, code, -1);
    *attribute = value;
}

/*
 * Convert a normalized ANSI escape sequence (parameters between "\33[" and
 * "m", see modifier "color_normalize_ansi") to IRC colors, using and updating
 * the state of ANSI colors.
 */

void
irc_color_decode_ansi_sequence (char **output,
                                struct t_irc_color_ansi_state *ansi_state,
                                const char *params, int length)
{
    struct t_irc_color_formatting *formatting, formatting_start;
    const char *ptr_param, *ptr_end;
    char **codes;
    int value, color, fg_only;

    codes = weechat_string_dyn_alloc (32);
    if (!codes)
        return;

    formatting = &ansi_state->formatting;

    /*
     * Formatting before this sequence, with the colors displayed by the IRC
     * client (to send only colors which change).
     */
    formatting_start = *formatting;
    formatting_start.fg = ansi_state->irc_fg;
    formatting_start.bg = ansi_state->irc_bg;

    ptr_param = params;
    ptr_end = params + length;
    while (irc_color_ansi_next_param (&ptr_param, ptr_end, &value))
    {
        switch (value)
        {
            case 0: /* Reset */
                weechat_string_dyn_concat (codes, IRC_COLOR_RESET_STR, -1);
                irc_color_decode_ansi_reset_state (ansi_state);
                /* Colors are reset in IRC clients too. */
                formatting_start = *formatting;
                break;
            case 1: /* Bold */
                irc_color_decode_ansi_set_attr (codes, &formatting->bold, 1,
                                                IRC_COLOR_BOLD_STR);
                break;
            case 2: /* Dim (not in IRC): remove bold */
            case 21: /* Remove bold */
            case 22:
                irc_color_decode_ansi_set_attr (codes, &formatting->bold, 0,
                                                IRC_COLOR_BOLD_STR);
                break;
            case 3: /* Italic */
                irc_color_decode_ansi_set_attr (codes, &formatting->italic, 1,
                                                IRC_COLOR_ITALIC_STR);
                break;
            case 4: /* Underline */
                irc_color_decode_ansi_set_attr (codes, &formatting->underline,
                                                1, IRC_COLOR_UNDERLINE_STR);
                break;
            case 7: /* Reverse */
                irc_color_decode_ansi_set_attr (codes, &formatting->reverse,
                                                1, IRC_COLOR_REVERSE_STR);
                break;
            case 9: /* Strikethrough */
                irc_color_decode_ansi_set_attr (codes,
                                                &formatting->strikethrough, 1,
                                                IRC_COLOR_STRIKETHROUGH_STR);
                break;
            case 23: /* Remove italic */
                irc_color_decode_ansi_set_attr (codes, &formatting->italic, 0,
                                                IRC_COLOR_ITALIC_STR);
                break;
            case 24: /* Remove underline */
                irc_color_decode_ansi_set_attr (codes, &formatting->underline,
                                                0, IRC_COLOR_UNDERLINE_STR);
                break;
            case 27: /* Remove reverse */
                irc_color_decode_ansi_set_attr (codes, &formatting->reverse,
                                                0, IRC_COLOR_REVERSE_STR);
                break;
            case 29: /* Remove strikethrough */
                irc_color_decode_ansi_set_attr (codes,
                                                &formatting->strikethrough, 0,
                                                IRC_COLOR_STRIKETHROUGH_STR);
                break;
            case 30: /* Text color */
            case 31:
            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
                formatting->fg = irc_color_term2irc[value - 30];
                break;
            case 38: /* Text color */
            case 48: /* Background color */
                color = irc_color_ansi_next_color (&ptr_param, ptr_end);
                if (color < 0)
                    break;
                color = (color & IRC_COLOR_RGB_FLAG) ?
                    irc_color_convert_rgb2irc (color & 0xFFFFFF) :
                    irc_color_convert_term2irc (color);
                if (color >= 0)
                {
                    if (value == 38)
                        formatting->fg = color;
                    else
                        formatting->bg = color;
                }
                break;
            case 39: /* Default text color */
                formatting->fg = IRC_COLOR_DEFAULT;
                break;
            case 40: /* Background color */
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
                formatting->bg = irc_color_term2irc[value - 40];
                break;
            case 49: /* Default background color */
                formatting->bg = IRC_COLOR_DEFAULT;
                break;
            case 90: /* Text color (bright) */
            case 91:
            case 92:
            case 93:
            case 94:
            case 95:
            case 96:
            case 97:
                formatting->fg = irc_color_term2irc[value - 90 + 8];
                break;
            case 100: /* Background color (bright) */
            case 101:
            case 102:
            case 103:
            case 104:
            case 105:
            case 106:
            case 107:
                formatting->bg = irc_color_term2irc[value - 100 + 8];
                break;
        }
    }

    /*
     * Send colors only if they have changed, and the background only if it
     * has changed: "^Cfg" keeps the background (and the remapped colors with
     * the default background in option irc.color.mirc_remap still apply);
     * the foreground is always sent with a background, because "^C,bg" is a
     * color reset followed by text ",bg".
     */
    fg_only = irc_color_formatting_transition (
        codes, &formatting_start, formatting,
        1);
    ansi_state->irc_fg = formatting->fg;
    ansi_state->irc_bg = formatting->bg;

    /*
     * Text after a foreground only ("^Cfg") may need a separator; the state
     * is not changed if this sequence adds nothing.
     */
    if ((*codes)[0])
    {
        weechat_string_dyn_concat (output, *codes, -1);
        ansi_state->fg_only = fg_only;
    }
    weechat_string_dyn_free (codes, 1);
}

/*
 * Add text (not an ANSI escape sequence) in output of irc_color_decode_ansi.
 *
 * If the last color code added has only a foreground ("^Cfg"), "^B^B" is
 * added before the text if needed, otherwise the text would be read as a
 * background of the color code (see function
 * irc_color_formatting_add_separator).
 *
 * IRC codes in text change the state of the IRC client: a reset ("^O") or a
 * new line (sent in another message) resets attributes and colors, and a
 * color code ("^C" or "^D") changes the colors displayed by the IRC client
 * (but not the ANSI colors), so that the next ANSI colors are sent according
 * to them.
 */

void
irc_color_decode_ansi_add_text (char **output, const char *text, int length,
                                struct t_irc_color_ansi_state *ansi_state)
{
    int i, size, fg, bg, end_code, fg_only;

    if (length <= 0)
        return;

    irc_color_formatting_add_separator (output, ansi_state->fg_only, text,
                                        length);
    weechat_string_dyn_concat (output, text, length);

    end_code = -1;
    fg_only = 0;
    for (i = 0; i < length; i++)
    {
        switch (text[i])
        {
            case '\n':
            case IRC_COLOR_RESET_CHAR:
                irc_color_decode_ansi_reset_state (ansi_state);
                break;
            case IRC_COLOR_COLOR_CHAR:
            case IRC_COLOR_HEX_COLOR_CHAR:
                /*
                 * Colors of the code (a code without color resets the colors,
                 * a text color alone keeps the background); the code ends
                 * before the next escape sequence.
                 */
                size = irc_color_code_parse (text + i, &fg, &bg);
                irc_color_formatting_set_colors (&ansi_state->irc_fg,
                                                 &ansi_state->irc_bg, fg, bg,
                                                 IRC_COLOR_DEFAULT);
                /*
                 * A text color alone ("^Cfg" or "^Dfg"): text starting with
                 * "," and a color after it would be read as a background.
                 */
                fg_only = ((fg != IRC_COLOR_FORMATTING_NONE)
                           && (bg == IRC_COLOR_FORMATTING_NONE)) ?
                    ((text[i] == IRC_COLOR_COLOR_CHAR) ? 1 : 2) : 0;
                end_code = i + ((size > 0) ? size : 1);
                if (size > 1)
                    i += size - 1;
                break;
        }
    }

    /*
     * If the text ends with a text color alone, the next text (after an
     * escape sequence which adds nothing) may need a separator.
     */
    ansi_state->fg_only = (end_code == length) ? fg_only : 0;
}

/*
 * Get the size of a normalized ANSI escape sequence at the beginning of a
 * string: "\33[" + parameters (digits and ";") + "m".
 *
 * Return the size of the sequence, 0 if the string does not start with a
 * normalized ANSI escape sequence.
 */

int
irc_color_ansi_sequence_size (const char *string)
{
    int length;

    if ((string[0] != '\33') || (string[1] != '['))
        return 0;

    length = strspn (string + 2, "0123456789;");
    if ((length == 0) || (string[2 + length] != 'm'))
        return 0;

    return length + 3;
}

/*
 * Replace ANSI colors by IRC colors.
 *
 * The ANSI escape sequences are first normalized by the modifier
 * "color_normalize_ansi" of WeeChat core.
 *
 * If keep_colors == 0: removes any color/style in message otherwise keeps
 * colors.
 *
 * Note: result must be freed after use.
 */

char *
irc_color_decode_ansi (const char *string, int keep_colors)
{
    struct t_irc_color_ansi_state ansi_state;
    char *normalized, **output;
    const char *ptr_string, *ptr_text;
    int size;

    if (!string)
        return NULL;

    /* Without ESC, there is nothing to decode. */
    if (!strchr (string, '\33'))
        return strdup (string);

    normalized = weechat_hook_modifier_exec ("color_normalize_ansi",
                                             (keep_colors) ? "1" : "0",
                                             string);
    if (!normalized || !keep_colors)
        return normalized;

    output = weechat_string_dyn_alloc (strlen (normalized) + 1);
    if (!output)
    {
        free (normalized);
        return NULL;
    }

    irc_color_decode_ansi_reset_state (&ansi_state);

    ptr_text = normalized;
    ptr_string = strchr (normalized, '\33');
    while (ptr_string)
    {
        size = irc_color_ansi_sequence_size (ptr_string);
        if (size > 0)
        {
            irc_color_decode_ansi_add_text (output, ptr_text,
                                            ptr_string - ptr_text,
                                            &ansi_state);
            irc_color_decode_ansi_sequence (output, &ansi_state,
                                            ptr_string + 2, size - 3);
            ptr_text = ptr_string + size;
        }
        ptr_string = strchr (ptr_string + ((size > 0) ? size : 1), '\33');
    }
    irc_color_decode_ansi_add_text (output, ptr_text, strlen (ptr_text),
                                    &ansi_state);

    free (normalized);

    return weechat_string_dyn_free (output, 0);
}

/*
 * Callback for modifiers "irc_color_decode", "irc_color_encode" and
 * "irc_color_decode_ansi".
 *
 * These modifiers can be used by other plugins to decode/encode IRC/ANSI
 * colors in messages.
 */

char *
irc_color_modifier_cb (const void *pointer, void *data,
                       const char *modifier, const char *modifier_data,
                       const char *string)
{
    int keep_colors;

    /* Make C compiler happy. */
    (void) pointer;
    (void) data;

    keep_colors = (modifier_data && (strcmp (modifier_data, "1") == 0)) ? 1 : 0;

    if (strcmp (modifier, "irc_color_decode") == 0)
        return irc_color_decode (string, keep_colors);

    if (strcmp (modifier, "irc_color_encode") == 0)
        return irc_color_encode (string, keep_colors);

    if (strcmp (modifier, "irc_color_decode_ansi") == 0)
        return irc_color_decode_ansi (string, keep_colors);

    /* Unknown modifier */
    return NULL;
}

/*
 * Return color name for tags (replace "," by ":").
 *
 * Note: result must be freed after use.
 */

char *
irc_color_for_tags (const char *color)
{
    if (!color)
        return NULL;

    return weechat_string_replace (color, ",", ":");
}

/*
 * Add mapping between IRC color codes and WeeChat color names in an infolist.
 *
 * Return:
 *   1: OK
 *   0: error
 */

int
irc_color_weechat_add_to_infolist (struct t_infolist *infolist)
{
    struct t_infolist_item *ptr_item;
    char str_color_irc[32];
    int i;

    if (!infolist)
        return 0;

    for (i = 0; i < IRC_NUM_COLORS; i++)
    {
        ptr_item = weechat_infolist_new_item (infolist);
        if (!ptr_item)
            return 0;

        snprintf (str_color_irc, sizeof (str_color_irc), "%02d", i);
        if (!weechat_infolist_new_var_string (ptr_item, "color_irc", str_color_irc))
            return 0;
        if (!weechat_infolist_new_var_string (ptr_item, "color_weechat", irc_color_to_weechat[i]))
            return 0;
    }

    return 1;
}

/*
 * Initialize IRC colors.
 */

void
irc_color_init (void)
{
    irc_color_index_string_decoded = 0;
    memset (irc_color_string_decoded, 0, sizeof (irc_color_string_decoded));
}

/*
 * End IRC colors.
 */

void
irc_color_end (void)
{
    int i;

    for (i = 0; i < 32; i++)
    {
        free (irc_color_string_decoded[i]);
    }
}
