/*
 * spell-command.c - spell checker commands
 *
 * Copyright (C) 2013-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "spell.h"
#include "spell-config.h"
#include "spell-speller.h"


/*
 * Converts an ISO lang code in its English full name.
 *
 * Note: result must be freed after use.
 */

char *
spell_command_iso_to_lang (const char *code)
{
    int i;

    for (i = 0; spell_langs[i].code; i++)
    {
        if (strcmp (spell_langs[i].code, code) == 0)
            return strdup (spell_langs[i].name);
    }

    /* lang code not found */
    return strdup ("Unknown");
}

/*
 * Converts an ISO country code in its English full name.
 *
 * Note: result must be freed after use.
 */

char *
spell_command_iso_to_country (const char *code)
{
    int i;

    for (i = 0; spell_countries[i].code; i++)
    {
        if (strcmp (spell_countries[i].code, code) == 0)
            return strdup (spell_countries[i].name);
    }

    /* country code not found */
    return strdup ("Unknown");
}

/*
 * Displays one dictionary when using enchant.
 */

#ifdef USE_ENCHANT
void
spell_enchant_dict_describe_cb (const char *lang_tag,
                                const char *provider_name,
                                const char *provider_desc,
                                const char *provider_file,
                                void *user_data)
{
    char *country, *lang, *pos, *iso;
    char str_dict[256];

    /* make C compiler happy */
    (void) provider_name;
    (void) provider_desc;
    (void) provider_file;
    (void) user_data;

    lang = NULL;
    country = NULL;

    pos = strchr (lang_tag, '_');

    if (pos)
    {
        iso = weechat_strndup (lang_tag, pos - lang_tag);
        if (iso)
        {
            lang = spell_command_iso_to_lang (iso);
            country = spell_command_iso_to_country (pos + 1);
            free (iso);
        }
    }
    else
        lang = spell_command_iso_to_lang ((char *)lang_tag);

    if (lang)
    {
        if (country)
        {
            snprintf (str_dict, sizeof (str_dict), "%-22s %s (%s)",
                      lang_tag, lang, country);
        }
        else
        {
            snprintf (str_dict, sizeof (str_dict), "%-22s %s",
                      lang_tag, lang);
        }
        weechat_printf (NULL, "  %s", str_dict);
    }

    if (lang)
        free (lang);
    if (country)
        free (country);
}
#endif /* USE_ENCHANT */

/*
 * Displays list of dictionaries installed on system.
 */

void
spell_command_speller_list_dicts ()
{
#ifndef USE_ENCHANT
    char *country, *lang, *pos, *iso;
    char str_dict[256], str_country[128];
    struct AspellConfig *config;
    AspellDictInfoList *list;
    AspellDictInfoEnumeration *elements;
    const AspellDictInfo *dict;
#endif /* USE_ENCHANT */

    weechat_printf (NULL, "");
    weechat_printf (NULL,
                    /* TRANSLATORS: "%s" is "spell" (name of plugin) */
                    _( "%s dictionaries list:"),
                    SPELL_PLUGIN_NAME);

#ifdef USE_ENCHANT
    enchant_broker_list_dicts (broker, spell_enchant_dict_describe_cb,
                               NULL);
#else
    config = new_aspell_config ();
    list = get_aspell_dict_info_list (config);
    elements = aspell_dict_info_list_elements (list);

    while ((dict = aspell_dict_info_enumeration_next (elements)) != NULL)
    {
        lang = NULL;
        country = NULL;
        pos = strchr (dict->code, '_');

        if (pos)
        {
            iso = weechat_strndup (dict->code, pos - dict->code);
            if (iso)
            {
                lang = spell_command_iso_to_lang (iso);
                country = spell_command_iso_to_country (pos + 1);
                free (iso);
            }
        }
        else
            lang = spell_command_iso_to_lang ((char*)dict->code);

        str_country[0] = '\0';
        if (country || dict->jargon[0])
        {
            snprintf (str_country, sizeof (str_country), " (%s%s%s)",
                      (country) ? country : dict->jargon,
                      (country && dict->jargon[0]) ? " - " : "",
                      (country && dict->jargon[0]) ? ((dict->jargon[0]) ? dict->jargon : country) : "");
        }

        snprintf (str_dict, sizeof (str_dict), "%-22s %s%s",
                  dict->name,
                  (lang) ? lang : "?",
                  str_country);

        weechat_printf (NULL, "  %s", str_dict);

        if (lang)
            free (lang);
        if (country)
            free (country);
    }

    delete_aspell_dict_info_enumeration (elements);
    delete_aspell_config (config);
#endif /* USE_ENCHANT */
}

/*
 * Sets a list of dictionaries for a buffer.
 */

void
spell_command_set_dict (struct t_gui_buffer *buffer, const char *value)
{
    char *name;
    int disabled;

    name = spell_build_option_name (buffer);
    if (!name)
        return;

    if (spell_config_set_dict (name, value) > 0)
    {
        if (value && value[0])
        {
            disabled = (strcmp (value, "-") == 0);
            weechat_printf (NULL, "%s: \"%s\" => %s%s%s%s",
                            SPELL_PLUGIN_NAME,
                            name,
                            value,
                            (disabled) ? " (" : "",
                            (disabled) ? _("spell checking disabled") : "",
                            (disabled) ? ")" : "");
        }
        else
            weechat_printf (NULL, _("%s: \"%s\" removed"),
                            SPELL_PLUGIN_NAME, name);
    }

    free (name);
}

/*
 * Adds a word in personal dictionary.
 */

void
spell_command_add_word (struct t_gui_buffer *buffer, const char *dict,
                                 const char *word)
{
    struct t_spell_speller_buffer *ptr_speller_buffer;
#ifdef USE_ENCHANT
    EnchantDict *new_speller, *ptr_speller;
#else
    AspellSpeller *new_speller, *ptr_speller;
#endif /* USE_ENCHANT */

    new_speller = NULL;

    if (dict)
    {
        ptr_speller = weechat_hashtable_get (spell_spellers, dict);
        if (!ptr_speller)
        {
            if (!spell_speller_dict_supported (dict))
            {
                weechat_printf (NULL,
                                _("%s: error: dictionary \"%s\" is not "
                                  "available on your system"),
                                SPELL_PLUGIN_NAME, dict);
                return;
            }
            new_speller = spell_speller_new (dict);
            if (!new_speller)
                return;
            ptr_speller = new_speller;
        }
    }
    else
    {
        ptr_speller_buffer = weechat_hashtable_get (spell_speller_buffer,
                                                    buffer);
        if (!ptr_speller_buffer)
            ptr_speller_buffer = spell_speller_buffer_new (buffer);
        if (!ptr_speller_buffer)
            goto error;
        if (!ptr_speller_buffer->spellers || !ptr_speller_buffer->spellers[0])
        {
            weechat_printf (NULL,
                            _("%s%s: no dictionary on this buffer for "
                              "adding word"),
                            weechat_prefix ("error"),
                            SPELL_PLUGIN_NAME);
            return;
        }
        else if (ptr_speller_buffer->spellers[1])
        {
            weechat_printf (NULL,
                            _("%s%s: many dictionaries are defined for "
                              "this buffer, please specify dictionary"),
                            weechat_prefix ("error"),
                            SPELL_PLUGIN_NAME);
            return;
        }
        ptr_speller = ptr_speller_buffer->spellers[0];
    }

#ifdef USE_ENCHANT
    enchant_dict_add (ptr_speller, word, strlen (word));
#else
    if (aspell_speller_add_to_personal (ptr_speller,
                                        word,
                                        strlen (word)) == 1)
    {
        weechat_printf (NULL,
                        _("%s: word \"%s\" added to personal dictionary"),
                        SPELL_PLUGIN_NAME, word);
    }
    else
        goto error;
#endif /* USE_ENCHANT */

    goto end;

error:
    weechat_printf (NULL,
                    _("%s%s: failed to add word to personal "
                      "dictionary"),
                    weechat_prefix ("error"), SPELL_PLUGIN_NAME);

end:
    if (new_speller)
        weechat_hashtable_remove (spell_spellers, dict);
}

/*
 * Callback for command "/spell".
 */

int
spell_command_cb (const void *pointer, void *data,
                  struct t_gui_buffer *buffer,
                  int argc, char **argv, char **argv_eol)
{
    char *dicts;
    const char *default_dict;
    struct t_infolist *infolist;
    int number;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc == 1)
    {
        /* display spell status */
        weechat_printf (NULL, "");
        weechat_printf (NULL,
                        /* TRANSLATORS: second "%s" is "aspell" or "enchant" */
                        _("%s (using %s)"),
                        (spell_enabled) ? _("Spell checking is enabled") : _("Spell checking is disabled"),
#ifdef USE_ENCHANT
                        "enchant"
#else
                        "aspell"
#endif /* USE_ENCHANT */
            );
        default_dict = weechat_config_string (spell_config_check_default_dict);
        weechat_printf (NULL,
                        _("Default dictionary: %s"),
                        (default_dict && default_dict[0]) ?
                        default_dict : _("(not set)"));
        number = 0;
        infolist = weechat_infolist_get ("option", NULL, "spell.dict.*");
        if (infolist)
        {
            while (weechat_infolist_next (infolist))
            {
                if (number == 0)
                    weechat_printf (NULL, _("Specific dictionaries on buffers:"));
                number++;
                weechat_printf (NULL, "  %s: %s",
                                weechat_infolist_string (infolist, "option_name"),
                                weechat_infolist_string (infolist, "value"));
            }
            weechat_infolist_free (infolist);
        }
        return WEECHAT_RC_OK;
    }

    /* enable spell */
    if (weechat_strcmp (argv[1], "enable") == 0)
    {
        weechat_config_option_set (spell_config_check_enabled, "1", 1);
        weechat_printf (NULL, _("Spell checker enabled"));
        return WEECHAT_RC_OK;
    }

    /* disable spell */
    if (weechat_strcmp (argv[1], "disable") == 0)
    {
        weechat_config_option_set (spell_config_check_enabled, "0", 1);
        weechat_printf (NULL, _("Spell checker disabled"));
        return WEECHAT_RC_OK;
    }

    /* toggle spell */
    if (weechat_strcmp (argv[1], "toggle") == 0)
    {
        if (spell_enabled)
        {
            weechat_config_option_set (spell_config_check_enabled, "0", 1);
            weechat_printf (NULL, _("Spell checker disabled"));
        }
        else
        {
            weechat_config_option_set (spell_config_check_enabled, "1", 1);
            weechat_printf (NULL, _("Spell checker enabled"));
        }
        return WEECHAT_RC_OK;
    }

    /* list of dictionaries */
    if (weechat_strcmp (argv[1], "listdict") == 0)
    {
        spell_command_speller_list_dicts ();
        return WEECHAT_RC_OK;
    }

    /* set dictionary for current buffer */
    if (weechat_strcmp (argv[1], "setdict") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "setdict");
        dicts = weechat_string_replace (argv_eol[2], " ", "");
        spell_command_set_dict (buffer,
                                (dicts) ? dicts : argv[2]);
        if (dicts)
            free (dicts);
        return WEECHAT_RC_OK;
    }

    /* delete dictionary used on current buffer */
    if (weechat_strcmp (argv[1], "deldict") == 0)
    {
        spell_command_set_dict (buffer, NULL);
        return WEECHAT_RC_OK;
    }

    /* add word to personal dictionary */
    if (weechat_strcmp (argv[1], "addword") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "addword");
        if (argc > 3)
        {
            /* use a given dict */
            spell_command_add_word (buffer, argv[2], argv_eol[3]);
        }
        else
        {
            /* use default dict */
            spell_command_add_word (buffer, NULL, argv_eol[2]);
        }
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Hooks spell command.
 */

void
spell_command_init ()
{
    weechat_hook_command (
        "spell",
        N_("spell plugin configuration"),
        N_("enable|disable|toggle"
           " || listdict"
           " || setdict -|<dict>[,<dict>...]"
           " || deldict"
           " || addword [<dict>] <word>"),
        N_("  enable: enable spell checker\n"
           " disable: disable spell checker\n"
           "  toggle: toggle spell checker\n"
           "listdict: show installed dictionaries\n"
           " setdict: set dictionary for current buffer (multiple dictionaries "
           "can be separated by a comma, the special value \"-\" disables "
           "spell checking on current buffer)\n"
           " deldict: delete dictionary used on current buffer\n"
           " addword: add a word in personal dictionary\n"
           "\n"
           "Input line beginning with a '/' is not checked, except for some "
           "commands (see /set spell.check.commands).\n"
           "\n"
           "To enable spell checker on all buffers, use option \"default_dict\", "
           "then enable spell checker, for example:\n"
           "  /set spell.check.default_dict \"en\"\n"
           "  /spell enable\n"
           "\n"
           "To display a list of suggestions in a bar, use item "
           "\"spell_suggest\".\n"
           "\n"
           "Default key to toggle spell checker is alt-s."),
        "enable"
        " || disable"
        " || toggle"
        " || listdict"
        " || setdict %(spell_dicts)"
        " || deldict"
        " || addword",
        &spell_command_cb, NULL, NULL);
}
