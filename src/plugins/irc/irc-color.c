/*
 * irc-color.c - IRC color decoding/encoding in messages
 *
 * Copyright (C) 2003-2018 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_PCRE
#include <pcreposix.h>
#else
#include <regex.h>
#endif

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
regex_t *irc_color_regex_ansi = NULL;


/*
 * Replaces IRC colors by WeeChat colors.
 *
 * If keep_colors == 0: removes any color/style in message otherwise keeps
 * colors.
 *
 * Note: result must be freed after use.
 */

char *
irc_color_decode (const char *string, int keep_colors)
{
    unsigned char *out, *out2, *ptr_string;
    int out_length, length, out_pos, length_to_add;
    char str_fg[3], str_bg[3], str_color[128], str_key[128], str_to_add[128];
    const char *remapped_color;
    int fg, bg, bold, reverse, italic, underline, rc;

    if (!string)
        return NULL;

    /*
     * create output string with size of length*2 (with min 128 bytes),
     * this string will be realloc() later with a larger size if needed
     */
    out_length = (strlen (string) * 2) + 1;
    if (out_length < 128)
        out_length = 128;
    out = malloc (out_length);
    if (!out)
        return NULL;

    /* initialize attributes */
    bold = 0;
    reverse = 0;
    italic = 0;
    underline = 0;

    ptr_string = (unsigned char *)string;
    out[0] = '\0';
    out_pos = 0;
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
                underline = 0;
                ptr_string++;
                break;
            case IRC_COLOR_FIXED_CHAR:
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
                ptr_string++;
                str_fg[0] = '\0';
                str_bg[0] = '\0';
                if (isdigit (ptr_string[0]))
                {
                    str_fg[0] = ptr_string[0];
                    str_fg[1] = '\0';
                    ptr_string++;
                    if (isdigit (ptr_string[0]))
                    {
                        str_fg[1] = ptr_string[0];
                        str_fg[2] = '\0';
                        ptr_string++;
                    }
                }
                if ((ptr_string[0] == ',') && (isdigit (ptr_string[1])))
                {
                    ptr_string++;
                    str_bg[0] = ptr_string[0];
                    str_bg[1] = '\0';
                    ptr_string++;
                    if (isdigit (ptr_string[0]))
                    {
                        str_bg[1] = ptr_string[0];
                        str_bg[2] = '\0';
                        ptr_string++;
                    }
                }
                if (keep_colors)
                {
                    if (str_fg[0] || str_bg[0])
                    {
                        fg = -1;
                        bg = -1;
                        if (str_fg[0])
                        {
                            rc = sscanf (str_fg, "%d", &fg);
                            if ((rc != EOF) && (rc >= 1))
                            {
                                fg %= IRC_NUM_COLORS;
                            }
                        }
                        if (str_bg[0])
                        {
                            rc = sscanf (str_bg, "%d", &bg);
                            if ((rc != EOF) && (rc >= 1))
                            {
                                bg %= IRC_NUM_COLORS;
                            }
                        }
                        /* search "fg,bg" in hashtable of remapped colors */
                        snprintf (str_key, sizeof (str_key), "%d,%d", fg, bg);
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
                        snprintf (str_to_add, sizeof (str_to_add), "%s",
                                  weechat_color ("resetcolor"));
                    }
                }
                break;
            default:
                /*
                 * we are not on an IRC color code, just copy the UTF-8 char
                 * into "str_to_add"
                 */
                length = weechat_utf8_char_size ((char *)ptr_string);
                if (length == 0)
                    length = 1;
                memcpy (str_to_add, ptr_string, length);
                str_to_add[length] = '\0';
                ptr_string += length;
                break;
        }
        /* add "str_to_add" (if not empty) to "out" */
        if (str_to_add[0])
        {
            /* if "out" is too small for adding "str_to_add", do a realloc() */
            length_to_add = strlen (str_to_add);
            if (out_pos + length_to_add + 1 > out_length)
            {
                /* try to double the size of "out" */
                out_length *= 2;
                out2 = realloc (out, out_length);
                if (!out2)
                    return (char *)out;
                out = out2;
            }
            /* add "str_to_add" to "out" */
            memcpy (out + out_pos, str_to_add, length_to_add + 1);
            out_pos += length_to_add;
        }
    }

    return (char *)out;
}

/*
 * Replaces color codes in command line by IRC color codes.
 *
 * If keep_colors == 0, remove any color/style in message, otherwise keeps
 * colors.
 *
 * Note: result must be freed after use.
 */

char *
irc_color_encode (const char *string, int keep_colors)
{
    unsigned char *out, *ptr_string;
    int out_length, out_pos, length;

    if (!string)
        return NULL;

    out_length = (strlen (string) * 2) + 1;
    out = malloc (out_length);
    if (!out)
        return NULL;

    ptr_string = (unsigned char *)string;
    out_pos = 0;
    while (ptr_string && ptr_string[0] && (out_pos < out_length - 1))
    {
        switch (ptr_string[0])
        {
            case 0x02: /* ^B */
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_BOLD_CHAR;
                ptr_string++;
                break;
            case 0x03: /* ^C */
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_COLOR_CHAR;
                ptr_string++;
                if (isdigit (ptr_string[0]))
                {
                    if (keep_colors)
                        out[out_pos++] = ptr_string[0];
                    ptr_string++;
                    if (isdigit (ptr_string[0]))
                    {
                        if (keep_colors)
                            out[out_pos++] = ptr_string[0];
                        ptr_string++;
                    }
                }
                if (ptr_string[0] == ',')
                {
                    if (keep_colors)
                        out[out_pos++] = ',';
                    ptr_string++;
                    if (isdigit (ptr_string[0]))
                    {
                        if (keep_colors)
                            out[out_pos++] = ptr_string[0];
                        ptr_string++;
                        if (isdigit (ptr_string[0]))
                        {
                            if (keep_colors)
                                out[out_pos++] = ptr_string[0];
                            ptr_string++;
                        }
                    }
                }
                break;
            case 0x0F: /* ^O */
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_RESET_CHAR;
                ptr_string++;
                break;
            case 0x16: /* ^V */
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_REVERSE_CHAR;
                ptr_string++;
                break;
            case 0x1F: /* ^_ */
                if (keep_colors)
                    out[out_pos++] = IRC_COLOR_UNDERLINE_CHAR;
                ptr_string++;
                break;
            default:
                length = weechat_utf8_char_size ((char *)ptr_string);
                if (length == 0)
                    length = 1;
                memcpy (out + out_pos, ptr_string, length);
                out_pos += length;
                ptr_string += length;
        }
    }

    out[out_pos] = '\0';

    return (char *)out;
}

/*
 * Converts a RGB color to IRC color.
 *
 * Returns a IRC color number (between 0 and 15), -1 if error.
 */

int
irc_color_convert_rgb2irc (int rgb)
{
    char str_color[64], *error;
    const char *info_color;
    long number;

    snprintf (str_color, sizeof (str_color),
              "%d,%d",
              rgb,
              IRC_COLOR_TERM2IRC_NUM_COLORS);

    info_color = weechat_info_get ("color_rgb2term", str_color);
    if (!info_color || !info_color[0])
        return -1;

    error = NULL;
    number = strtol (info_color, &error, 10);
    if (!error || error[0]
        || (number < 0) || (number >= IRC_COLOR_TERM2IRC_NUM_COLORS))
    {
        return -1;
    }

    return irc_color_term2irc[number];
}

/*
 * Converts a terminal color to IRC color.
 *
 * Returns a IRC color number (between 0 and 15), -1 if error.
 */

int
irc_color_convert_term2irc (int color)
{
    char str_color[64], *error;
    const char *info_color;
    long number;

    snprintf (str_color, sizeof (str_color), "%d", color);

    info_color = weechat_info_get ("color_term2rgb", str_color);
    if (!info_color || !info_color[0])
        return -1;

    error = NULL;
    number = strtol (info_color, &error, 10);
    if (!error || error[0] || (number < 0) || (number > 0xFFFFFF))
        return -1;

    return irc_color_convert_rgb2irc (number);
}

/*
 * Replaces ANSI colors by IRC colors (or removes them).
 *
 * This callback is called by irc_color_decode_ansi, it must not be called
 * directly.
 */

char *
irc_color_decode_ansi_cb (void *data, const char *text)
{
    struct t_irc_color_ansi_state *ansi_state;
    char *text2, **items, *output, str_color[128];
    int i, length, num_items, value, color;

    ansi_state = (struct t_irc_color_ansi_state *)data;

    /* if we don't keep colors of if text is empty, just return empty string */
    if (!ansi_state->keep_colors || !text || !text[0])
        return strdup ("");

    /* only sequences ending with 'm' are used, the others are discarded */
    length = strlen (text);
    if (text[length - 1] != 'm')
        return strdup ("");

    /* sequence "\33[m" resets color */
    if (length < 4)
        return strdup (weechat_color ("reset"));

    text2 = NULL;
    items = NULL;
    output = NULL;

    /* extract text between "\33[" and "m" */
    text2 = weechat_strndup (text + 2, length - 3);
    if (!text2)
        goto end;

    items = weechat_string_split (text2, ";", 0, 0, &num_items);
    if (!items)
        goto end;

    output = malloc ((32 * num_items) + 1);
    if (!output)
        goto end;
    output[0] = '\0';

    for (i = 0; i < num_items; i++)
    {
        value = atoi (items[i]);
        switch (value)
        {
            case 0: /* reset */
                strcat (output, IRC_COLOR_RESET_STR);
                ansi_state->bold = 0;
                ansi_state->underline = 0;
                ansi_state->italic = 0;
                break;
            case 1: /* bold */
                if (!ansi_state->bold)
                {
                    strcat (output, IRC_COLOR_BOLD_STR);
                    ansi_state->bold = 1;
                }
                break;
            case 2: /* remove bold */
            case 21:
            case 22:
                if (ansi_state->bold)
                {
                    strcat (output, IRC_COLOR_BOLD_STR);
                    ansi_state->bold = 0;
                }
                break;
            case 3: /* italic */
                if (!ansi_state->italic)
                {
                    strcat (output, IRC_COLOR_ITALIC_STR);
                    ansi_state->italic = 1;
                }
                break;
            case 4: /* underline */
                if (!ansi_state->underline)
                {
                    strcat (output, IRC_COLOR_UNDERLINE_STR);
                    ansi_state->underline = 1;
                }
                break;
            case 23: /* remove italic */
                if (ansi_state->italic)
                {
                    strcat (output, IRC_COLOR_ITALIC_STR);
                    ansi_state->italic = 0;
                }
                break;
            case 24: /* remove underline */
                if (ansi_state->underline)
                {
                    strcat (output, IRC_COLOR_UNDERLINE_STR);
                    ansi_state->underline = 0;
                }
                break;
            case 30: /* text color */
            case 31:
            case 32:
            case 33:
            case 34:
            case 35:
            case 36:
            case 37:
                snprintf (str_color, sizeof (str_color),
                          "%c%02d",
                          IRC_COLOR_COLOR_CHAR,
                          irc_color_term2irc[value - 30]);
                strcat (output, str_color);
                break;
            case 38: /* text color */
                if (i + 1 < num_items)
                {
                    switch (atoi (items[i + 1]))
                    {
                        case 2: /* RGB color */
                            if (i + 4 < num_items)
                            {
                                color = irc_color_convert_rgb2irc (
                                    (atoi (items[i + 2]) << 16) |
                                    (atoi (items[i + 3]) << 8) |
                                    atoi (items[i + 4]));
                                if (color >= 0)
                                {
                                    snprintf (str_color, sizeof (str_color),
                                              "%c%02d",
                                              IRC_COLOR_COLOR_CHAR,
                                              color);
                                    strcat (output, str_color);
                                }
                                i += 4;
                            }
                            break;
                        case 5: /* terminal color (0-255) */
                            if (i + 2 < num_items)
                            {
                                color = irc_color_convert_term2irc (atoi (items[i + 2]));
                                if (color >= 0)
                                {
                                    snprintf (str_color, sizeof (str_color),
                                              "%c%02d",
                                              IRC_COLOR_COLOR_CHAR,
                                              color);
                                    strcat (output, str_color);
                                }
                                i += 2;
                            }
                            break;
                    }
                }
                break;
            case 39: /* default text color */
                snprintf (str_color, sizeof (str_color),
                          "%c15",
                          IRC_COLOR_COLOR_CHAR);
                strcat (output, str_color);
                break;
            case 40: /* background color */
            case 41:
            case 42:
            case 43:
            case 44:
            case 45:
            case 46:
            case 47:
                snprintf (str_color, sizeof (str_color),
                          "%c,%02d",
                          IRC_COLOR_COLOR_CHAR,
                          irc_color_term2irc[value - 40]);
                strcat (output, str_color);
                break;
            case 48: /* background color */
                if (i + 1 < num_items)
                {
                    switch (atoi (items[i + 1]))
                    {
                        case 2: /* RGB color */
                            if (i + 4 < num_items)
                            {
                                color = irc_color_convert_rgb2irc (
                                    (atoi (items[i + 2]) << 16) |
                                    (atoi (items[i + 3]) << 8) |
                                    atoi (items[i + 4]));
                                if (color >= 0)
                                {
                                    snprintf (str_color, sizeof (str_color),
                                              "%c,%02d",
                                              IRC_COLOR_COLOR_CHAR,
                                              color);
                                    strcat (output, str_color);
                                }
                                i += 4;
                            }
                            break;
                        case 5: /* terminal color (0-255) */
                            if (i + 2 < num_items)
                            {
                                color = irc_color_convert_term2irc (atoi (items[i + 2]));
                                if (color >= 0)
                                {
                                    snprintf (str_color, sizeof (str_color),
                                              "%c,%02d",
                                              IRC_COLOR_COLOR_CHAR,
                                              color);
                                    strcat (output, str_color);
                                }
                                i += 2;
                            }
                            break;
                    }
                }
                break;
            case 49: /* default background color */
                snprintf (str_color, sizeof (str_color),
                          "%c,01",
                          IRC_COLOR_COLOR_CHAR);
                strcat (output, str_color);
                break;
            case 90: /* text color (bright) */
            case 91:
            case 92:
            case 93:
            case 94:
            case 95:
            case 96:
            case 97:
                snprintf (str_color, sizeof (str_color),
                          "%c%02d",
                          IRC_COLOR_COLOR_CHAR,
                          irc_color_term2irc[value - 90 + 8]);
                strcat (output, str_color);
                break;
            case 100: /* background color (bright) */
            case 101:
            case 102:
            case 103:
            case 104:
            case 105:
            case 106:
            case 107:
                snprintf (str_color, sizeof (str_color),
                          "%c,%02d",
                          IRC_COLOR_COLOR_CHAR,
                          irc_color_term2irc[value - 100 + 8]);
                strcat (output, str_color);
                break;
        }
    }

end:
    if (items)
        weechat_string_free_split (items);
    if (text2)
        free (text2);

    return (output) ? output : strdup ("");
}

/*
 * Replaces ANSI colors by IRC colors.
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

    /* allocate/compile regex if needed (first call) */
    if (!irc_color_regex_ansi)
    {
        irc_color_regex_ansi = malloc (sizeof (*irc_color_regex_ansi));
        if (!irc_color_regex_ansi)
            return NULL;
        if (weechat_string_regcomp (irc_color_regex_ansi,
                                    weechat_info_get ("color_ansi_regex", NULL),
                                    REG_EXTENDED) != 0)
        {
            free (irc_color_regex_ansi);
            irc_color_regex_ansi = NULL;
            return NULL;
        }
    }

    ansi_state.keep_colors = keep_colors;
    ansi_state.bold = 0;
    ansi_state.underline = 0;
    ansi_state.italic = 0;

    return weechat_string_replace_regex (string, irc_color_regex_ansi,
                                         "$0", '$',
                                         &irc_color_decode_ansi_cb,
                                         &ansi_state);
}

/*
 * Callback for modifiers "irc_color_decode" and "irc_color_encode".
 *
 * This modifier can be used by other plugins to decode/encode IRC colors in
 * messages.
 */

char *
irc_color_modifier_cb (const void *pointer, void *data,
                       const char *modifier, const char *modifier_data,
                       const char *string)
{
    int keep_colors;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    keep_colors = (modifier_data && (strcmp (modifier_data, "1") == 0)) ? 1 : 0;

    if (strcmp (modifier, "irc_color_decode") == 0)
        return irc_color_decode (string, keep_colors);

    if (strcmp (modifier, "irc_color_encode") == 0)
        return irc_color_encode (string, keep_colors);

    if (strcmp (modifier, "irc_color_decode_ansi") == 0)
        return irc_color_decode_ansi (string, keep_colors);

    /* unknown modifier */
    return NULL;
}

/*
 * Returns color name for tags (replace "," by ":").
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
 * Adds mapping between IRC color codes and WeeChat color names in an infolist.
 *
 * Returns:
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
 * Ends IRC colors.
 */

void
irc_color_end ()
{
    if (irc_color_regex_ansi)
    {
        regfree (irc_color_regex_ansi);
        free (irc_color_regex_ansi);
        irc_color_regex_ansi = NULL;
    }
}
