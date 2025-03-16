/*
 * spell.c - spell checker plugin for WeeChat
 *
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2025 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <wctype.h>
#include <ctype.h>

#include "../weechat-plugin.h"
#include "spell.h"
#include "spell-bar-item.h"
#include "spell-command.h"
#include "spell-completion.h"
#include "spell-config.h"
#include "spell-info.h"
#include "spell-speller.h"


WEECHAT_PLUGIN_NAME(SPELL_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION(N_("Spell checker for input"));
WEECHAT_PLUGIN_AUTHOR("Sébastien Helleu <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE(WEECHAT_LICENSE);
WEECHAT_PLUGIN_PRIORITY(SPELL_PLUGIN_PRIORITY);

struct t_weechat_plugin *weechat_spell_plugin = NULL;

int spell_enabled = 0;

char *spell_nick_completer = NULL;
int spell_len_nick_completer = 0;

#ifdef USE_ENCHANT
EnchantBroker *spell_enchant_broker = NULL;
#endif /* USE_ENCHANT */

/*
 * aspell supported languages, updated on 2012-07-05
 * URL: ftp://ftp.gnu.org/gnu/aspell/dict/0index.html
 */

struct t_spell_code spell_langs[] =
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

struct t_spell_code spell_countries[] =
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

char *spell_url_prefix[] =
{ "http:", "https:", "ftp:", "tftp:", "ftps:", "ssh:", "fish:", "dict:",
  "ldap:", "file:", "telnet:", "gopher:", "irc:", "ircs:", "irc6:", "irc6s:",
  "cvs:", "svn:", "svn+ssh:", "git:", NULL };


/*
 * Displays a warning if the file aspell.conf is still present in WeeChat
 * home directory and spell.conf not yet created (upgrade from a version ≤ 2.4
 * to a version ≥ 2.5).
 */

void
spell_warning_aspell_config (void)
{
    char *aspell_filename, *spell_filename;

    aspell_filename = weechat_string_eval_path_home (
        "${weechat_config_dir}/aspell.conf", NULL, NULL, NULL);
    spell_filename = weechat_string_eval_path_home (
        "${weechat_config_dir}/" SPELL_CONFIG_NAME ".conf", NULL, NULL, NULL);

    /* if aspell.conf is there and not spell.conf, display a warning */
    if (aspell_filename && spell_filename
        && (access (aspell_filename, F_OK) == 0)
        && (access (spell_filename, F_OK) != 0))
    {
        weechat_printf (NULL,
                        _("%s%s: warning: the plugin \"aspell\" has been "
                          "renamed to \"spell\" and the file %s still exists "
                          "(but not %s); if you upgraded from an older "
                          "version, you should check instructions in release "
                          "notes (version 2.5) to recover your settings"),
                        weechat_prefix ("error"),
                        SPELL_PLUGIN_NAME,
                        aspell_filename,
                        spell_filename);
    }

    free (aspell_filename);
    free (spell_filename);
}

/*
 * Builds full name of buffer.
 *
 * Note: result must be freed after use.
 */

char *
spell_build_option_name (struct t_gui_buffer *buffer)
{
    const char *plugin_name, *name;
    char *option_name;

    if (!buffer)
        return NULL;

    plugin_name = weechat_buffer_get_string (buffer, "plugin");
    name = weechat_buffer_get_string (buffer, "name");

    weechat_asprintf (&option_name, "%s.%s", plugin_name, name);

    return option_name;
}

/*
 * Gets dictionary list for a name of buffer.
 *
 * First tries with all arguments, then removes one by one to find dict (from
 * specific to general dict).
 */

const char *
spell_get_dict_with_buffer_name (const char *name)
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
            ptr_option = spell_config_get_dict (option_name);
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
        ptr_option = spell_config_get_dict (option_name);

        free (option_name);

        if (ptr_option)
            return weechat_config_string (ptr_option);
    }

    /* nothing found => return default dictionary (if set) */
    if (weechat_config_string (spell_config_check_default_dict)
        && weechat_config_string (spell_config_check_default_dict)[0])
    {
        return weechat_config_string (spell_config_check_default_dict);
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
spell_get_dict (struct t_gui_buffer *buffer)
{
    char *name;
    const char *dict;

    name = spell_build_option_name (buffer);
    if (!name)
        return NULL;

    dict = spell_get_dict_with_buffer_name (name);

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
spell_command_authorized (const char *command)
{
    int length_command, i;

    if (!command)
        return 1;

    length_command = strlen (command);

    for (i = 0; i < spell_count_commands_to_check; i++)
    {
        if ((spell_length_commands_to_check[i] == length_command)
            && (strcmp (command, spell_commands_to_check[i]) == 0))
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
spell_string_is_url (const char *word)
{
    int i;

    for (i = 0; spell_url_prefix[i]; i++)
    {
        if (weechat_strncasecmp (word, spell_url_prefix[i],
                                 weechat_utf8_strlen (spell_url_prefix[i])) == 0)
        {
            return 1;
        }
    }

    /* word is not an URL */
    return 0;
}

/*
 * Checks if a word is a nick of nicklist.
 *
 * Returns:
 *   1: word is a nick of nicklist
 *   0: word is not a nick of nicklist
 */

int
spell_string_is_nick (struct t_gui_buffer *buffer, const char *word)
{
    char *pos, *pos_nick_completer, *pos_space, saved_char;
    const char *buffer_type, *buffer_nick, *buffer_channel;
    int rc;

    pos_nick_completer = (spell_nick_completer) ?
        strstr (word, spell_nick_completer) : NULL;
    pos_space = strchr (word, ' ');

    pos = NULL;
    if (pos_nick_completer && pos_space)
    {
        if ((pos_nick_completer < pos_space)
            && (pos_nick_completer + spell_len_nick_completer == pos_space))
        {
            pos = pos_nick_completer;
        }
        else
            pos = pos_space;
    }
    else
    {
        pos = (pos_nick_completer
               && !pos_nick_completer[spell_len_nick_completer]) ?
            pos_nick_completer : pos_space;
    }

    if (pos)
    {
        saved_char = pos[0];
        pos[0] = '\0';
    }

    rc = (weechat_nicklist_search_nick (buffer, NULL, word)) ? 1 : 0;

    if (!rc)
    {
        /* for "private" buffers, check if word is self or remote nick */
        buffer_type = weechat_buffer_get_string (buffer, "localvar_type");
        if (buffer_type && (strcmp (buffer_type, "private") == 0))
        {
            /* check self nick */
            buffer_nick = weechat_buffer_get_string (buffer, "localvar_nick");
            if (buffer_nick && (weechat_strcasecmp (buffer_nick, word) == 0))
            {
                rc = 1;
            }
            else
            {
                /* check remote nick */
                buffer_channel = weechat_buffer_get_string (buffer,
                                                            "localvar_channel");
                if (buffer_channel
                    && (weechat_strcasecmp (buffer_channel, word) == 0))
                {
                    rc = 1;
                }
            }
        }
    }

    if (pos)
        pos[0] = saved_char;

    return rc;
}

/*
 * Checks if a word is made of digits and punctuation.
 *
 * Returns:
 *   1: word has only digits and punctuation
 *   0: word has some other chars (not digits neither punctuation)
 */

int
spell_string_is_simili_number (const char *word)
{
    int code_point;

    if (!word || !word[0])
        return 0;

    while (word && word[0])
    {
        code_point = weechat_utf8_char_int (word);
        if (!iswpunct (code_point) && !iswdigit (code_point))
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
spell_check_word (struct t_spell_speller_buffer *speller_buffer,
                  const char *word)
{
    int i;

    /* word too small? then do not check word */
    if ((weechat_config_integer (spell_config_check_word_min_length) > 0)
        && ((int)strlen (word) < weechat_config_integer (spell_config_check_word_min_length)))
        return 1;

    /* word is a number? then do not check word */
    if (spell_string_is_simili_number (word))
        return 1;

    /* check word with all spellers (order is important) */
    if (speller_buffer->spellers)
    {
        for (i = 0; speller_buffer->spellers[i]; i++)
        {
#ifdef USE_ENCHANT
            if (enchant_dict_check (speller_buffer->spellers[i], word, strlen (word)) == 0)
#else
            if (aspell_speller_check (speller_buffer->spellers[i], word, -1) == 1)
#endif /* USE_ENCHANT */
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
 * Note: result must be freed after use (if not NULL).
 */

char *
spell_get_suggestions (struct t_spell_speller_buffer *speller_buffer,
                       const char *word)
{
    int i, size, max_suggestions, num_suggestions;
    char *suggestions, *suggestions2;
    const char *ptr_word;
#ifdef USE_ENCHANT
    char **elements;
    size_t num_elements;
#else
    const AspellWordList *list;
    AspellStringEnumeration *elements;
#endif /* USE_ENCHANT */

    max_suggestions = weechat_config_integer (spell_config_check_suggestions);
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
#ifdef USE_ENCHANT
            elements = enchant_dict_suggest (speller_buffer->spellers[i], word,
                                             -1, &num_elements);
            if (elements)
            {
                if (num_elements > 0)
                {
                    num_suggestions = 0;
                    while ((ptr_word = elements[num_suggestions]) != NULL)
                    {
                        size += strlen (ptr_word) + ((suggestions[0]) ? 1 : 0);
                        suggestions2 = realloc (suggestions, size);
                        if (!suggestions2)
                        {
                            free (suggestions);
                            enchant_dict_free_string_list (speller_buffer->spellers[i],
                                                           elements);
                            return NULL;
                        }
                        suggestions = suggestions2;
                        if (suggestions[0])
                            strcat (suggestions, (num_suggestions == 0) ? "/" : ",");
                        strcat (suggestions, ptr_word);
                        num_suggestions++;
                        if (num_suggestions == max_suggestions)
                            break;
                    }
                }
                enchant_dict_free_string_list (speller_buffer->spellers[i], elements);
            }
#else
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
                    if (num_suggestions == max_suggestions)
                        break;
                }
                delete_aspell_string_enumeration (elements);
            }
#endif /* USE_ENCHANT */
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
 * Skips WeeChat and IRC color codes in *string and adds them to "result".
 */

void
spell_skip_color_codes (char **string, char **result)
{
    int color_code_size;

    while ((*string)[0])
    {
        color_code_size = weechat_string_color_code_size (*string);
        if (color_code_size > 0)
        {
            /* WeeChat color code */
            weechat_string_dyn_concat (result, *string, color_code_size);
            (*string) += color_code_size;
        }
        else if ((*string)[0] == '\x02' || (*string)[0] == '\x0F'
                 || (*string)[0] == '\x11' || (*string)[0] == '\x16'
                 || (*string)[0] == '\x1D' || (*string)[0] == '\x1F')
        {
            /*
             * IRC attribute:
             *   0x02: bold
             *   0x0F: reset
             *   0x11: monospaced font
             *   0x16: reverse video
             *   0x1D: italic
             *   0x1F: underlined text
             */
            weechat_string_dyn_concat (result, *string, 1);
            (*string)++;
        }
        else if ((*string)[0] == '\x03')
        {
            /* IRC color code */
            weechat_string_dyn_concat (result, *string, 1);
            (*string)++;
            if (isdigit ((unsigned char)((*string)[0])))
            {
                /* foreground */
                weechat_string_dyn_concat (result, *string, 1);
                (*string)++;
                if (isdigit ((unsigned char)((*string)[0])))
                {
                    weechat_string_dyn_concat (result, *string, 1);
                    (*string)++;
                }
            }
            if (((*string)[0] == ',')
                && (isdigit ((unsigned char)((*string)[1]))))
            {
                /* background */
                weechat_string_dyn_concat (result, *string, 1);
                (*string)++;
                if (isdigit ((unsigned char)((*string)[0])))
                {
                    weechat_string_dyn_concat (result, *string, 1);
                    (*string)++;
                }
            }
        }
        else
        {
            /* not a color code */
            break;
        }
    }
}

/*
 * Updates input text by adding color for misspelled words.
 */

char *
spell_modifier_cb (const void *pointer, void *data,
                   const char *modifier,
                   const char *modifier_data, const char *string)
{
    struct t_gui_buffer *buffer;
    struct t_spell_speller_buffer *ptr_speller_buffer;
    char **result, *ptr_string, *ptr_string_orig, *pos_space;
    char *ptr_end, *ptr_end_valid, save_end;
    char *misspelled_word, *old_misspelled_word, *old_suggestions, *suggestions;
    char *word_and_suggestions;
    const char *color_normal, *color_error, *ptr_suggestions, *pos_colon;
    int code_point, char_size;
    int length, word_ok, rc;
    int input_pos, current_pos, word_start_pos, word_end_pos, word_end_pos_valid;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier;

    if (!spell_enabled)
        return NULL;

    if (!string)
        return NULL;

    rc = sscanf (modifier_data, "%p", &buffer);
    if ((rc == EOF) || (rc == 0))
        return NULL;

    /* check text during search only if option is enabled */
    if (weechat_buffer_get_integer (buffer, "text_search")
        && !weechat_config_boolean (spell_config_check_during_search))
        return NULL;

    /* get structure with speller info for buffer */
    ptr_speller_buffer = weechat_hashtable_get (spell_speller_buffer,
                                                buffer);
    if (!ptr_speller_buffer)
    {
        ptr_speller_buffer = spell_speller_buffer_new (buffer);
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
        && ((weechat_config_integer (spell_config_check_suggestions) < 0)
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

    misspelled_word = NULL;

    /* save last modifier string received */
    ptr_speller_buffer->modifier_string = strdup (string);
    ptr_speller_buffer->input_pos = input_pos;

    color_normal = weechat_color ("bar_fg");
    color_error = weechat_color (weechat_config_string (spell_config_color_misspelled));

    length = strlen (string);
    result = weechat_string_dyn_alloc ((length * 2) + 1);
    if (!result)
        return NULL;

    ptr_string = ptr_speller_buffer->modifier_string;

    /* check if string is a command */
    if (!weechat_string_input_for_buffer (ptr_string))
    {
        char_size = weechat_utf8_char_size (ptr_string);
        ptr_string += char_size;
        pos_space = ptr_string;
        while (pos_space && pos_space[0] && (pos_space[0] != ' '))
        {
            pos_space = (char *)weechat_utf8_next_char (pos_space);
        }
        if (!pos_space || !pos_space[0])
        {
            weechat_string_dyn_free (result, 1);
            return NULL;
        }

        pos_space[0] = '\0';

        /* exit if command is not authorized for spell checking */
        if (!spell_command_authorized (ptr_string))
        {
            weechat_string_dyn_free (result, 1);
            return NULL;
        }
        weechat_string_dyn_concat (result,
                                   ptr_speller_buffer->modifier_string,
                                   char_size);
        weechat_string_dyn_concat (result, ptr_string, -1);

        pos_space[0] = ' ';
        ptr_string = pos_space;
    }

    current_pos = 0;
    while (ptr_string[0])
    {
        ptr_string_orig = NULL;

        spell_skip_color_codes (&ptr_string, result);
        if (!ptr_string[0])
            break;

        /* find start of word: it must start with an alphanumeric char */
        code_point = weechat_utf8_char_int (ptr_string);
        while ((!iswalnum (code_point)) || iswspace (code_point))
        {
            spell_skip_color_codes (&ptr_string, result);
            if (!ptr_string[0])
                break;

            if (!ptr_string_orig && !iswspace (code_point))
                ptr_string_orig = ptr_string;

            char_size = weechat_utf8_char_size (ptr_string);
            weechat_string_dyn_concat (result, ptr_string, char_size);
            ptr_string += char_size;
            current_pos++;
            if (!ptr_string[0])
                break;
            code_point = weechat_utf8_char_int (ptr_string);
        }
        if (!ptr_string[0])
            break;
        if (!ptr_string_orig)
            ptr_string_orig = ptr_string;

        word_start_pos = current_pos;
        word_end_pos = current_pos;
        word_end_pos_valid = current_pos;

        /* find end of word: ' and - allowed in word, but not at the end */
        ptr_end_valid = ptr_string;
        ptr_end = (char *)weechat_utf8_next_char (ptr_string);
        code_point = weechat_utf8_char_int (ptr_end);
        while (iswalnum (code_point) || (code_point == '\'')
               || (code_point == '-'))
        {
            word_end_pos++;
            if (iswalnum (code_point))
            {
                /* pointer to last alphanumeric char in the word */
                ptr_end_valid = ptr_end;
                word_end_pos_valid = word_end_pos;
            }
            ptr_end = (char *)weechat_utf8_next_char (ptr_end);
            if (!ptr_end[0])
                break;
            code_point = weechat_utf8_char_int (ptr_end);
        }
        ptr_end = (char *)weechat_utf8_next_char (ptr_end_valid);
        word_end_pos = word_end_pos_valid;
        word_ok = 0;
        if (spell_string_is_url (ptr_string)
            || spell_string_is_nick (buffer, ptr_string_orig))
        {
            /*
             * word is an URL or a nick, then it is OK: search for next
             * space (will be end of word)
             */
            word_ok = 1;
            if (ptr_end[0])
            {
                code_point = weechat_utf8_char_int (ptr_end);
                while (!iswspace (code_point))
                {
                    ptr_end = (char *)weechat_utf8_next_char (ptr_end);
                    if (!ptr_end[0])
                        break;
                    code_point = weechat_utf8_char_int (ptr_end);
                }
            }
        }
        save_end = ptr_end[0];
        ptr_end[0] = '\0';

        if (!word_ok)
        {
            if ((save_end != '\0')
                || (weechat_config_integer (spell_config_check_real_time)))
            {
                word_ok = spell_check_word (ptr_speller_buffer,
                                            ptr_string);
                if (!word_ok && (input_pos >= word_start_pos))
                {
                    /*
                     * if word is misspelled and that cursor is after
                     * the beginning of this word, save the word (we will
                     * look for suggestions after this loop)
                     */
                    free (misspelled_word);
                    misspelled_word = strdup (ptr_string);
                }
            }
            else
                word_ok = 1;
        }

        /* add error color */
        if (!word_ok)
            weechat_string_dyn_concat (result, color_error, -1);

        /* add word */
        weechat_string_dyn_concat (result, ptr_string, -1);

        /* add normal color (after misspelled word) */
        if (!word_ok)
            weechat_string_dyn_concat (result, color_normal, -1);

        if (save_end == '\0')
            break;

        ptr_end[0] = save_end;
        ptr_string = ptr_end;
        current_pos = word_end_pos + 1;
    }

    /* save old suggestions in buffer */
    ptr_suggestions = weechat_buffer_get_string (buffer,
                                                 "localvar_spell_suggest");
    old_suggestions = (ptr_suggestions) ? strdup (ptr_suggestions) : NULL;

    /* if there is a misspelled word, get suggestions and set them in buffer */
    if (misspelled_word)
    {
        /*
         * get the old misspelled word; we'll get suggestions or clear
         * local variable "spell_suggest" only if the current misspelled
         * word is different
         */
        old_misspelled_word = NULL;
        if (old_suggestions)
        {
            pos_colon = strchr (old_suggestions, ':');
            old_misspelled_word = (pos_colon) ?
                weechat_strndup (old_suggestions, pos_colon - old_suggestions) :
                strdup (old_suggestions);
        }

        if (!old_misspelled_word
            || (strcmp (old_misspelled_word, misspelled_word) != 0))
        {
            suggestions = spell_get_suggestions (ptr_speller_buffer,
                                                 misspelled_word);
            if (suggestions)
            {
                if (weechat_asprintf (&word_and_suggestions,
                                      "%s:%s",
                                      misspelled_word,
                                      suggestions) >= 0)
                {
                    weechat_buffer_set (buffer,
                                        "localvar_set_spell_suggest",
                                        word_and_suggestions);
                    free (word_and_suggestions);
                }
                else
                {
                    weechat_buffer_set (buffer,
                                        "localvar_del_spell_suggest", "");
                }
                free (suggestions);
            }
            else
            {
                /* set a misspelled word in buffer, also without suggestions */
                weechat_buffer_set (buffer, "localvar_set_spell_suggest",
                                    misspelled_word);
            }
        }

        free (old_misspelled_word);
        free (misspelled_word);
    }
    else
    {
        weechat_buffer_set (buffer, "localvar_del_spell_suggest", "");
    }

    /*
     * if suggestions have changed, update the bar item
     * and send signal "spell_suggest"
     */
    ptr_suggestions = weechat_buffer_get_string (buffer,
                                                 "localvar_spell_suggest");
    if ((old_suggestions && !ptr_suggestions)
        || (!old_suggestions && ptr_suggestions)
        || (old_suggestions && ptr_suggestions
            && (strcmp (old_suggestions, ptr_suggestions) != 0)))
    {
        weechat_bar_item_update ("spell_suggest");
        (void) weechat_hook_signal_send ("spell_suggest",
                                         WEECHAT_HOOK_SIGNAL_POINTER, buffer);
    }
    free (old_suggestions);

    ptr_speller_buffer->modifier_result = strdup (*result);

    return weechat_string_dyn_free (result, 0);
}

/*
 * Refreshes bar items on signal "buffer_switch".
 */

int
spell_buffer_switch_cb (const void *pointer, void *data, const char *signal,
                        const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    /* refresh bar items (for root bars) */
    weechat_bar_item_update ("spell_dict");
    weechat_bar_item_update ("spell_suggest");

    return WEECHAT_RC_OK;
}

/*
 * Refreshes bar items on signal "window_switch".
 */

int
spell_window_switch_cb (const void *pointer, void *data, const char *signal,
                        const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

    /* refresh bar items (for root bars) */
    weechat_bar_item_update ("spell_dict");
    weechat_bar_item_update ("spell_suggest");

    return WEECHAT_RC_OK;
}

/*
 * Removes struct for buffer in hashtable "spell_speller_buffer" on
 * signal "buffer_closed".
 */

int
spell_buffer_closed_cb (const void *pointer, void *data, const char *signal,
                        const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    weechat_hashtable_remove (spell_speller_buffer, signal_data);

    return WEECHAT_RC_OK;
}

/*
 * Display infos about external libraries used.
 */

int
spell_debug_libs_cb (const void *pointer, void *data, const char *signal,
                     const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;
    (void) signal_data;

#ifdef USE_ENCHANT
#ifdef HAVE_ENCHANT_GET_VERSION
    weechat_printf (NULL, "  %s: enchant %s",
                    SPELL_PLUGIN_NAME, enchant_get_version ());
#else
    weechat_printf (NULL, "  %s: enchant (?)", SPELL_PLUGIN_NAME);
#endif /* HAVE_ENCHANT_GET_VERSION */
#else
#ifdef HAVE_ASPELL_VERSION_STRING
    weechat_printf (NULL, "  %s: aspell %s",
                    SPELL_PLUGIN_NAME, aspell_version_string ());
#else
    weechat_printf (NULL, "  %s: aspell (?)", SPELL_PLUGIN_NAME);
#endif /* HAVE_ASPELL_VERSION_STRING */
#endif /* USE_ENCHANT */

    return WEECHAT_RC_OK;
}

/*
 * Callback for changes to option "weechat.completion.nick_completer".
 */

int
spell_config_change_nick_completer_cb (const void *pointer, void *data,
                                       const char *option, const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    free (spell_nick_completer);

    spell_nick_completer = weechat_string_strip (value, 0, 1, " ");
    spell_len_nick_completer =
        (spell_nick_completer) ? strlen (spell_nick_completer) : 0;

    return WEECHAT_RC_OK;
}

/*
 * Initializes spell plugin.
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;

    weechat_plugin = plugin;

    spell_enabled = 0;

    spell_warning_aspell_config ();

#ifdef USE_ENCHANT
    /* acquire enchant broker */
    spell_enchant_broker = enchant_broker_init ();
    if (!spell_enchant_broker)
        return WEECHAT_RC_ERROR;
#ifdef ENCHANT_MYSPELL_DICT_DIR
    enchant_broker_set_param(spell_enchant_broker,
                             "enchant.myspell.dictionary.path",
                             ENCHANT_MYSPELL_DICT_DIR);
#endif
#endif /* USE_ENCHANT */

    if (!spell_speller_init ())
        return WEECHAT_RC_ERROR;

    if (!spell_config_init ())
        return WEECHAT_RC_ERROR;

    spell_config_read ();

    spell_command_init ();

    spell_completion_init ();

    /*
     * callback for spell checking input text
     * we use a low priority here, so that other modifiers "input_text_display"
     * (from other plugins) will be called before this one
     */
    weechat_hook_modifier ("500|input_text_display",
                           &spell_modifier_cb, NULL, NULL);

    spell_bar_item_init ();

    spell_info_init ();

    weechat_hook_signal ("buffer_switch",
                         &spell_buffer_switch_cb, NULL, NULL);
    weechat_hook_signal ("window_switch",
                         &spell_window_switch_cb, NULL, NULL);
    weechat_hook_signal ("buffer_closed",
                         &spell_buffer_closed_cb, NULL, NULL);
    weechat_hook_signal ("debug_libs",
                         &spell_debug_libs_cb, NULL, NULL);

    weechat_hook_config ("weechat.completion.nick_completer",
                         &spell_config_change_nick_completer_cb,
                         NULL, NULL);
    /* manually call callback to initialize */
    spell_config_change_nick_completer_cb (
        NULL, NULL, "weechat.completion.nick_completer",
        weechat_config_string (
            weechat_config_get ("weechat.completion.nick_completer")));

    return WEECHAT_RC_OK;
}

/*
 * Ends spell plugin.
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;

    spell_config_write ();
    spell_config_free ();

    spell_speller_end ();

#ifdef USE_ENCHANT
    /* release enchant broker */
    enchant_broker_free (spell_enchant_broker);
    spell_enchant_broker = NULL;
#endif /* USE_ENCHANT */

    if (spell_nick_completer)
    {
        free (spell_nick_completer);
        spell_nick_completer = NULL;
    }
    spell_len_nick_completer = 0;

    return WEECHAT_RC_OK;
}
