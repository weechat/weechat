/*
 * weechat-aspell.c - aspell plugin for WeeChat: color for misspelled words
 *
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2013 Sebastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2012 Nils Görs <weechatter@arcor.de>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <wctype.h>

#include "../weechat-plugin.h"
#include "weechat-aspell.h"
#include "weechat-aspell-bar-item.h"
#include "weechat-aspell-command.h"
#include "weechat-aspell-completion.h"
#include "weechat-aspell-config.h"
#include "weechat-aspell-info.h"
#include "weechat-aspell-speller.h"


WEECHAT_PLUGIN_NAME(ASPELL_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Spell checker for input (with Aspell)"));
WEECHAT_PLUGIN_AUTHOR("Sebastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);

struct t_weechat_plugin *weechat_aspell_plugin = NULL;

int aspell_enabled = 0;

/*
 * aspell supported languages, updated on 2012-07-05
 * URL: ftp://ftp.gnu.org/gnu/aspell/dict/0index.html
 */

struct t_aspell_code aspell_langs[] =
{
    { "af",     "Afrikaans" },
    { "am",     "Amharic" },
    { "ar",     "Arabic" },
    { "ast",    "Asturian" },
    { "az",     "Azerbaijani" },
    { "be",     "Belarusian" },
    { "bg",     "Bulgarian" },
    { "bn",     "Bengali" },
    { "br",     "Breton" },
    { "ca",     "Catalan" },
    { "cs",     "Czech" },
    { "csb",    "Kashubian" },
    { "cy",     "Welsh" },
    { "da",     "Danish" },
    { "de",     "German" },
    { "de-alt", "German - Old Spelling" },
    { "el",     "Greek" },
    { "en",     "English" },
    { "eo",     "Esperanto" },
    { "es",     "Spanish" },
    { "et",     "Estonian" },
    { "fa",     "Persian" },
    { "fi",     "Finnish" },
    { "fo",     "Faroese" },
    { "fr",     "French" },
    { "fy",     "Frisian" },
    { "ga",     "Irish" },
    { "gd",     "Scottish Gaelic" },
    { "gl",     "Galician" },
    { "grc",    "Ancient Greek" },
    { "gu",     "Gujarati" },
    { "gv",     "Manx Gaelic" },
    { "he",     "Hebrew" },
    { "hi",     "Hindi" },
    { "hil",    "Hiligaynon" },
    { "hr",     "Croatian" },
    { "hsb",    "Upper Sorbian" },
    { "hu",     "Hungarian" },
    { "hus",    "Huastec" },
    { "hy",     "Armenian" },
    { "ia",     "Interlingua" },
    { "id",     "Indonesian" },
    { "is",     "Icelandic" },
    { "it",     "Italian" },
    { "kn",     "Kannada" },
    { "ku",     "Kurdi" },
    { "ky",     "Kirghiz" },
    { "la",     "Latin" },
    { "lt",     "Lithuanian" },
    { "lv",     "Latvian" },
    { "mg",     "Malagasy" },
    { "mi",     "Maori" },
    { "mk",     "Macedonian" },
    { "ml",     "Malayalam" },
    { "mn",     "Mongolian" },
    { "mr",     "Marathi" },
    { "ms",     "Malay" },
    { "mt",     "Maltese" },
    { "nb",     "Norwegian Bokmal" },
    { "nds",    "Low Saxon" },
    { "nl",     "Dutch" },
    { "nn",     "Norwegian Nynorsk" },
    { "ny",     "Chichewa" },
    { "or",     "Oriya" },
    { "pa",     "Punjabi" },
    { "pl",     "Polish" },
    { "pt_BR",  "Brazilian Portuguese" },
    { "pt_PT",  "Portuguese" },
    { "qu",     "Quechua" },
    { "ro",     "Romanian" },
    { "ru",     "Russian" },
    { "rw",     "Kinyarwanda" },
    { "sc",     "Sardinian" },
    { "sk",     "Slovak" },
    { "sl",     "Slovenian" },
    { "sr",     "Serbian" },
    { "sv",     "Swedish" },
    { "sw",     "Swahili" },
    { "ta",     "Tamil" },
    { "te",     "Telugu" },
    { "tet",    "Tetum" },
    { "tk",     "Turkmen" },
    { "tl",     "Tagalog" },
    { "tn",     "Setswana" },
    { "tr",     "Turkish" },
    { "uk",     "Ukrainian" },
    { "uz",     "Uzbek" },
    { "vi",     "Vietnamese" },
    { "wa",     "Walloon" },
    { "yi",     "Yiddish" },
    { "zu",     "Zulu" },
    { NULL,     NULL}
};

struct t_aspell_code aspell_countries[] =
{
    { "AT", "Austria" },
    { "BR", "Brazil" },
    { "CA", "Canada" },
    { "CH", "Switzerland" },
    { "DE", "Germany" },
    { "FR", "France" },
    { "GB", "Great Britain" },
    { "PT", "Portugal" },
    { "SK", "Slovakia" },
    { "US", "United States of America" },
    { NULL, NULL}
};

char *aspell_url_prefix[] =
{ "http:", "https:", "ftp:", "tftp:", "ftps:", "ssh:", "fish:", "dict:",
  "ldap:", "file:", "telnet:", "gopher:", "irc:", "ircs:", "irc6:", "irc6s:",
  "cvs:", "svn:", "svn+ssh:", "git:", NULL };


/*
 * Builds full name of buffer.
 *
 * Note: result must be freed after use.
 */

char *
weechat_aspell_build_option_name (struct t_gui_buffer *buffer)
{
    const char *plugin_name, *name;
    char *option_name;
    int length;

    if (!buffer)
        return NULL;

    plugin_name = weechat_buffer_get_string (buffer, "plugin");
    name = weechat_buffer_get_string (buffer, "name");

    length = strlen (plugin_name) + 1 + strlen (name) + 1;
    option_name = malloc (length);
    if (!option_name)
        return NULL;

    snprintf (option_name, length, "%s.%s", plugin_name, name);

    return option_name;
}

/*
 * Gets dictionary list for a name of buffer.
 *
 * First tries with all arguments, then removes one by one to find dict (from
 * specific to general dict).
 */

const char *
weechat_aspell_get_dict_with_buffer_name (const char *name)
{
    char *option_name, *ptr_end;
    struct t_config_option *ptr_option;

    if (!name)
        return NULL;

    option_name = strdup (name);
    if (option_name)
    {
        ptr_end = option_name + strlen (option_name);
        while (ptr_end >= option_name)
        {
            ptr_option = weechat_aspell_config_get_dict (option_name);
            if (ptr_option)
            {
                free (option_name);
                return weechat_config_string (ptr_option);
            }
            ptr_end--;
            while ((ptr_end >= option_name) && (ptr_end[0] != '.'))
            {
                ptr_end--;
            }
            if ((ptr_end >= option_name) && (ptr_end[0] == '.'))
                ptr_end[0] = '\0';
        }
        ptr_option = weechat_aspell_config_get_dict (option_name);

        free (option_name);

        if (ptr_option)
            return weechat_config_string (ptr_option);
    }

    /* nothing found => return default dictionary (if set) */
    if (weechat_config_string (weechat_aspell_config_check_default_dict)
        && weechat_config_string (weechat_aspell_config_check_default_dict)[0])
    {
        return weechat_config_string (weechat_aspell_config_check_default_dict);
    }

    /* no default dictionary set */
    return NULL;
}

/*
 * Gets dictionary list for a buffer.
 *
 * First tries with all arguments, then removes one by one to find dict (from
 * specific to general dict).
 */

const char *
weechat_aspell_get_dict (struct t_gui_buffer *buffer)
{
    char *name;
    const char *dict;

    name = weechat_aspell_build_option_name (buffer);
    if (!name)
        return NULL;

    dict = weechat_aspell_get_dict_with_buffer_name (name);

    free (name);

    return dict;
}

/*
 * Checks if command is authorized for spell checking.
 *
 * Returns:
 *   1: command authorized
 *   0: command not authorized
 */

int
weechat_aspell_command_authorized (const char *command)
{
    int length_command, i;

    if (!command)
        return 1;

    length_command = strlen (command);

    for (i = 0; i < weechat_aspell_count_commands_to_check; i++)
    {
        if ((weechat_aspell_length_commands_to_check[i] == length_command)
            && (weechat_strcasecmp (command,
                                    weechat_aspell_commands_to_check[i]) == 0))
        {
            /* command is authorized */
            return 1;
        }
    }

    /* command is not authorized */
    return 0;
}

/*
 * Checks if a word is an URL.
 *
 * Returns:
 *   1: word is an URL
 *   0: word is not an URL
 */

int
weechat_aspell_string_is_url (const char *word)
{
    int i;

    for (i = 0; aspell_url_prefix[i]; i++)
    {
        if (weechat_strncasecmp (word, aspell_url_prefix[i],
                                 strlen (aspell_url_prefix[i])) == 0)
            return 1;
    }

    /* word is not an URL */
    return 0;
}

/*
 * Checks if a word is made of digits and punctuation.
 *
 * Returns:
 *   1: word has only digits and punctuation
 *   0: word has some other chars (not digits neither punctuation)
 */

int
weechat_aspell_string_is_simili_number (const char *word)
{
    int utf8_char_int;

    if (!word || !word[0])
        return 0;

    while (word && word[0])
    {
        utf8_char_int = weechat_utf8_char_int (word);
        if (!iswpunct (utf8_char_int) && !iswdigit (utf8_char_int))
            return 0;
        word = weechat_utf8_next_char (word);
    }

    /* there are only digits or punctuation */
    return 1;
}

/*
 * Spell checks a word.
 *
 * Returns:
 *   1: word is OK
 *   0: word is misspelled
 */

int
weechat_aspell_check_word (struct t_gui_buffer *buffer,
                           struct t_aspell_speller_buffer *speller_buffer,
                           const char *word)
{
    const char *buffer_type, *buffer_nick, *buffer_channel;
    int i;

    /* word too small? then do not check word */
    if ((weechat_config_integer (weechat_aspell_config_check_word_min_length) > 0)
        && ((int)strlen (word) < weechat_config_integer (weechat_aspell_config_check_word_min_length)))
        return 1;

    /* word is a number? then do not check word */
    if (weechat_aspell_string_is_simili_number (word))
        return 1;

    /* word is a nick of nicklist on this buffer? then do not check word */
    if (weechat_nicklist_search_nick (buffer, NULL, word))
        return 1;

    /* for "private" buffers, ignore self and remote nicks */
    buffer_type = weechat_buffer_get_string (buffer, "localvar_type");
    if (buffer_type && (strcmp (buffer_type, "private") == 0))
    {
        /* check self nick */
        buffer_nick = weechat_buffer_get_string (buffer, "localvar_nick");
        if (buffer_nick && (weechat_strcasecmp (buffer_nick, word) == 0))
            return 1;
        /* check remote nick */
        buffer_channel = weechat_buffer_get_string (buffer, "localvar_channel");
        if (buffer_channel && (weechat_strcasecmp (buffer_channel, word) == 0))
            return 1;
    }

    /* check word with all spellers for this buffer (order is important) */
    if (speller_buffer->spellers)
    {
        for (i = 0; speller_buffer->spellers[i]; i++)
        {
            if (aspell_speller_check (speller_buffer->spellers[i], word, -1) == 1)
                return 1;
        }
    }

    /* misspelled word! */
    return 0;
}

/*
 * Gets suggestions for a word.
 *
 * Returns a string with format: "suggest1,suggest2,suggest3".
 *
 * Note: result (if not NULL) must be freed after use.
 */

char *
weechat_aspell_get_suggestions (struct t_aspell_speller_buffer *speller_buffer,
                                const char *word)
{
    int i, size, max_suggestions, num_suggestions;
    char *suggestions, *suggestions2;
    const char *ptr_word;
    const AspellWordList *list;
    AspellStringEnumeration *elements;

    max_suggestions = weechat_config_integer (weechat_aspell_config_check_suggestions);
    if (max_suggestions < 0)
        return NULL;

    size = 1;
    suggestions = malloc (size);
    if (!suggestions)
        return NULL;

    suggestions[0] = '\0';
    if (speller_buffer->spellers)
    {
        for (i = 0; speller_buffer->spellers[i]; i++)
        {
            list = aspell_speller_suggest (speller_buffer->spellers[i], word, -1);
            if (list)
            {
                elements = aspell_word_list_elements (list);
                num_suggestions = 0;
                while ((ptr_word = aspell_string_enumeration_next (elements)) != NULL)
                {
                    size += strlen (ptr_word) + ((suggestions[0]) ? 1 : 0);
                    suggestions2 = realloc (suggestions, size);
                    if (!suggestions2)
                    {
                        free (suggestions);
                        delete_aspell_string_enumeration (elements);
                        return NULL;
                    }
                    suggestions = suggestions2;
                    if (suggestions[0])
                        strcat (suggestions, (num_suggestions == 0) ? "/" : ",");
                    strcat (suggestions, ptr_word);
                    num_suggestions++;
                    if ((max_suggestions >= 0) && (num_suggestions == max_suggestions))
                        break;
                }
                delete_aspell_string_enumeration (elements);
            }
        }
    }

    /* no suggestions found */
    if (!suggestions[0])
    {
        free (suggestions);
        return NULL;
    }

    return suggestions;
}

/*
 * Updates input text by adding color for misspelled words.
 */

char *
weechat_aspell_modifier_cb (void *data, const char *modifier,
                            const char *modifier_data, const char *string)
{
    long unsigned int value;
    struct t_gui_buffer *buffer;
    struct t_aspell_speller_buffer *ptr_speller_buffer;
    char *result, *ptr_string, *pos_space, *ptr_end, save_end;
    char *word_for_suggestions, *old_suggestions, *suggestions;
    char *word_and_suggestions;
    const char *color_normal, *color_error, *ptr_suggestions;
    int utf8_char_int, char_size;
    int length, index_result, length_word, word_ok;
    int length_color_normal, length_color_error, rc;
    int input_pos, current_pos, word_start_pos, word_end_pos;

    /* make C compiler happy */
    (void) data;
    (void) modifier;

    if (!aspell_enabled)
        return NULL;

    if (!string)
        return NULL;

    rc = sscanf (modifier_data, "%lx", &value);
    if ((rc == EOF) || (rc == 0))
        return NULL;

    buffer = (struct t_gui_buffer *)value;

    /* check text during search only if option is enabled */
    if (weechat_buffer_get_integer (buffer, "text_search")
        && !weechat_config_boolean (weechat_aspell_config_check_during_search))
        return NULL;

    /* get structure with speller info for buffer */
    ptr_speller_buffer = weechat_hashtable_get (weechat_aspell_speller_buffer,
                                                buffer);
    if (!ptr_speller_buffer)
    {
        ptr_speller_buffer = weechat_aspell_speller_buffer_new (buffer);
        if (!ptr_speller_buffer)
            return NULL;
    }
    if (!ptr_speller_buffer->spellers)
        return NULL;

    /*
     * for performance: return last string built if input string is the
     * same (and cursor position is the same, if suggestions are enabled)
     */
    input_pos = weechat_buffer_get_integer (buffer, "input_pos");
    if (ptr_speller_buffer->modifier_string
        && (strcmp (string, ptr_speller_buffer->modifier_string) == 0)
        && ((weechat_config_integer (weechat_aspell_config_check_suggestions) < 0)
            || (input_pos == ptr_speller_buffer->input_pos)))
    {
        return (ptr_speller_buffer->modifier_result) ?
            strdup (ptr_speller_buffer->modifier_result) : NULL;
    }

    /* free last modifier string and result */
    if (ptr_speller_buffer->modifier_string)
    {
        free (ptr_speller_buffer->modifier_string);
        ptr_speller_buffer->modifier_string = NULL;
    }
    if (ptr_speller_buffer->modifier_result)
    {
        free (ptr_speller_buffer->modifier_result);
        ptr_speller_buffer->modifier_result = NULL;
    }

    word_for_suggestions = NULL;

    /* save last modifier string received */
    ptr_speller_buffer->modifier_string = strdup (string);
    ptr_speller_buffer->input_pos = input_pos;

    color_normal = weechat_color ("bar_fg");
    length_color_normal = strlen (color_normal);
    color_error = weechat_color (weechat_config_string (weechat_aspell_config_look_color));
    length_color_error = strlen (color_error);

    length = strlen (string);
    result = malloc (length + (length * length_color_error) + 1);

    if (result)
    {
        result[0] = '\0';

        ptr_string = ptr_speller_buffer->modifier_string;
        index_result = 0;

        /* check if string is a command */
        if (!weechat_string_input_for_buffer (ptr_string))
        {
            char_size = weechat_utf8_char_size (ptr_string);
            ptr_string += char_size;
            pos_space = ptr_string;
            while (pos_space && pos_space[0] && (pos_space[0] != ' '))
            {
                pos_space = weechat_utf8_next_char (pos_space);
            }
            if (!pos_space || !pos_space[0])
            {
                free (result);
                return NULL;
            }

            pos_space[0] = '\0';

            /* exit if command is not authorized for spell checking */
            if (!weechat_aspell_command_authorized (ptr_string))
            {
                free (result);
                return NULL;
            }
            memcpy (result + index_result,
                    ptr_speller_buffer->modifier_string,
                    char_size);
            index_result += char_size;
            strcpy (result + index_result, ptr_string);
            index_result += strlen (ptr_string);

            pos_space[0] = ' ';
            ptr_string = pos_space;
        }

        current_pos = 0;
        while (ptr_string[0])
        {
            /* find start of word */
            utf8_char_int = weechat_utf8_char_int (ptr_string);
            while ((!iswalnum (utf8_char_int) && (utf8_char_int != '\'')
                    && (utf8_char_int != '-'))
                   || iswspace (utf8_char_int))
            {
                char_size = weechat_utf8_char_size (ptr_string);
                memcpy (result + index_result, ptr_string, char_size);
                index_result += char_size;
                ptr_string += char_size;
                current_pos++;
                if (!ptr_string[0])
                    break;
                utf8_char_int = weechat_utf8_char_int (ptr_string);
            }
            if (!ptr_string[0])
                break;

            word_start_pos = current_pos;
            word_end_pos = current_pos;

            /* find end of word */
            ptr_end = weechat_utf8_next_char (ptr_string);
            utf8_char_int = weechat_utf8_char_int (ptr_end);
            while (iswalnum (utf8_char_int) || (utf8_char_int == '\'')
                   || (utf8_char_int == '-'))
            {
                ptr_end = weechat_utf8_next_char (ptr_end);
                word_end_pos++;
                if (!ptr_end[0])
                    break;
                utf8_char_int = weechat_utf8_char_int (ptr_end);
            }
            word_ok = 0;
            if (weechat_aspell_string_is_url (ptr_string))
            {
                /*
                 * word is an URL, then it is OK, and search for next space
                 * (will be end of word)
                 */
                word_ok = 1;
                if (ptr_end[0])
                {
                    utf8_char_int = weechat_utf8_char_int (ptr_end);
                    while (!iswspace (utf8_char_int))
                    {
                        ptr_end = weechat_utf8_next_char (ptr_end);
                        if (!ptr_end[0])
                            break;
                        utf8_char_int = weechat_utf8_char_int (ptr_end);
                    }
                }
            }
            save_end = ptr_end[0];
            ptr_end[0] = '\0';
            length_word = ptr_end - ptr_string;

            if (!word_ok)
            {
                if ((save_end != '\0')
                    || (weechat_config_integer (weechat_aspell_config_check_real_time)))
                {
                    word_ok = weechat_aspell_check_word (buffer,
                                                         ptr_speller_buffer,
                                                         ptr_string);
                    if (!word_ok && (input_pos >= word_start_pos))
                    {
                        /*
                         * if word is misspelled and that cursor is after
                         * the beginning of this word, save the word (we will
                         * look for suggestions after this loop)
                         */
                        if (word_for_suggestions)
                            free (word_for_suggestions);
                        word_for_suggestions = strdup (ptr_string);
                    }
                }
                else
                    word_ok = 1;
            }

            /* add error color */
            if (!word_ok)
            {
                strcpy (result + index_result, color_error);
                index_result += length_color_error;
            }

            /* add word */
            strcpy (result + index_result, ptr_string);
            index_result += length_word;

            /* add normal color (after misspelled word) */
            if (!word_ok)
            {
                strcpy (result + index_result, color_normal);
                index_result += length_color_normal;
            }

            if (save_end == '\0')
                break;

            ptr_end[0] = save_end;
            ptr_string = ptr_end;
            current_pos = word_end_pos + 1;
        }
        result[index_result] = '\0';
    }

    /* save old suggestions in buffer */
    ptr_suggestions = weechat_buffer_get_string (buffer,
                                                 "localvar_aspell_suggest");
    old_suggestions = (ptr_suggestions) ? strdup (ptr_suggestions) : NULL;

    /* if there is a misspelled word, get suggestions and set them in buffer */
    if (word_for_suggestions)
    {
        suggestions = weechat_aspell_get_suggestions (ptr_speller_buffer,
                                                      word_for_suggestions);
        if (suggestions)
        {
            length = strlen (word_for_suggestions) + 1 /* ":" */
                + strlen (suggestions) + 1;
            word_and_suggestions = malloc (length);
            if (word_and_suggestions)
            {
                snprintf (word_and_suggestions, length, "%s:%s",
                          word_for_suggestions, suggestions);
                weechat_buffer_set (buffer, "localvar_set_aspell_suggest",
                                    word_and_suggestions);
                free (word_and_suggestions);
            }
            else
            {
                weechat_buffer_set (buffer, "localvar_del_aspell_suggest", "");
            }
            free (suggestions);
        }
        else
        {
            weechat_buffer_set (buffer, "localvar_del_aspell_suggest", "");
        }
        free (word_for_suggestions);
    }
    else
    {
        weechat_buffer_set (buffer, "localvar_del_aspell_suggest", "");
    }

    /*
     * if suggestions have changed, update the bar item
     * and send signal "aspell_suggest"
     */
    ptr_suggestions = weechat_buffer_get_string (buffer,
                                                 "localvar_aspell_suggest");
    if ((old_suggestions && !ptr_suggestions)
        || (!old_suggestions && ptr_suggestions)
        || (old_suggestions && ptr_suggestions
            && (strcmp (old_suggestions, ptr_suggestions) != 0)))
    {
        weechat_bar_item_update ("aspell_suggest");
        weechat_hook_signal_send ("aspell_suggest",
                                  WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
    if (old_suggestions)
        free (old_suggestions);

    if (!result)
        return NULL;

    ptr_speller_buffer->modifier_result = strdup (result);

    return result;
}

/*
 * Refreshes bar items on signal "buffer_switch".
 */

int
weechat_aspell_buffer_switch_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    /* refresh bar items (for root bars) */
    weechat_bar_item_update ("aspell_dict");
    weechat_bar_item_update ("aspell_suggest");

    return WEECHAT_RC_OK;
}

/*
 * Refreshes bar items on signal "window_switch".
 */

int
weechat_aspell_window_switch_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    /* refresh bar items (for root bars) */
    weechat_bar_item_update ("aspell_dict");
    weechat_bar_item_update ("aspell_suggest");

    return WEECHAT_RC_OK;
}

/*
 * Removes struct for buffer in hashtable "weechat_aspell_speller_buffer" on
 * signal "buffer_closed".
 */

int
weechat_aspell_buffer_closed_cb (void *data, const char *signal,
                                 const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;

    weechat_hashtable_remove (weechat_aspell_speller_buffer, signal_data);

    return WEECHAT_RC_OK;
}

/*
 * Initializes aspell plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    if (!weechat_aspell_speller_init ())
        return WEECHAT_RC_ERROR;

    if (!weechat_aspell_config_init ())
        return WEECHAT_RC_ERROR;

    if (weechat_aspell_config_read () < 0)
        return WEECHAT_RC_ERROR;

    weechat_aspell_command_init ();

    weechat_aspell_completion_init ();

    /*
     * callback for spell checking input text
     * we use a low priority here, so that other modifiers "input_text_display"
     * (from other plugins) will be called before this one
     */
    weechat_hook_modifier ("500|input_text_display",
                           &weechat_aspell_modifier_cb, NULL);

    weechat_aspell_bar_item_init ();

    weechat_aspell_info_init ();

    weechat_hook_signal ("buffer_switch",
                         &weechat_aspell_buffer_switch_cb, NULL);
    weechat_hook_signal ("window_switch",
                         &weechat_aspell_window_switch_cb, NULL);
    weechat_hook_signal ("buffer_closed",
                         &weechat_aspell_buffer_closed_cb, NULL);

    return WEECHAT_RC_OK;
}

/*
 * Ends aspell plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    weechat_aspell_config_write ();
    weechat_aspell_config_free ();

    weechat_aspell_speller_end ();

    return WEECHAT_RC_OK;
}
