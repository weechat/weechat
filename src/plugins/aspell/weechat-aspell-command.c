/*
 * weechat-aspell-command.c - aspell commands
 *
 * Copyright (C) 2013 Sebastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "weechat-aspell.h"
#include "weechat-aspell-config.h"
#include "weechat-aspell-speller.h"


/*
 * Converts an aspell ISO lang code in its English full name.
 *
 * Note: result must be freed after use.
 */

char *
weechat_aspell_command_iso_to_lang (const char *code)
{
    int i;

    for (i = 0; aspell_langs[i].code; i++)
    {
        if (strcmp (aspell_langs[i].code, code) == 0)
            return strdup (aspell_langs[i].name);
    }

    /* lang code not found */
    return strdup ("Unknown");
}

/*
 * Converts an aspell ISO country code in its English full name.
 *
 * Note: result must be freed after use.
 */

char *
weechat_aspell_command_iso_to_country (const char *code)
{
    int i;

    for (i = 0; aspell_countries[i].code; i++)
    {
        if (strcmp (aspell_countries[i].code, code) == 0)
            return strdup (aspell_countries[i].name);
    }

    /* country code not found */
    return strdup ("Unknown");
}

/*
 * Displays list of aspell dictionaries installed on system.
 */

void
weechat_aspell_command_speller_list_dicts ()
{
    char *country, *lang, *pos;
    char str_dict[256], str_country[128];
    struct AspellConfig *config;
    AspellDictInfoList *list;
    AspellDictInfoEnumeration *elements;
    const AspellDictInfo *dict;

    config = new_aspell_config();
    list = get_aspell_dict_info_list (config);
    elements = aspell_dict_info_list_elements (list);

    weechat_printf (NULL, "");
    weechat_printf (NULL,
                    /* TRANSLATORS: "%s" is "aspell" */
                    _( "%s dictionaries list:"),
                    ASPELL_PLUGIN_NAME);

    while ((dict = aspell_dict_info_enumeration_next (elements)) != NULL)
    {
        country = NULL;
        pos = strchr (dict->code, '_');

        if (pos)
        {
            pos[0] = '\0';
            lang = weechat_aspell_command_iso_to_lang ((char*)dict->code);
            pos[0] = '_';
            country = weechat_aspell_command_iso_to_country (pos + 1);
        }
        else
            lang = weechat_aspell_command_iso_to_lang ((char*)dict->code);

        str_country[0] = '\0';
        if (country || dict->jargon[0])
        {
            snprintf (str_country, sizeof (str_country), " (%s%s%s)",
                      (country) ? country : dict->jargon,
                      (country && dict->jargon[0]) ? " - " : "",
                      (country && dict->jargon[0]) ? ((dict->jargon[0]) ? dict->jargon : country) : "");
        }

        snprintf (str_dict, sizeof (str_dict), "%-22s %s%s",
                  dict->name, lang, str_country);

        weechat_printf (NULL, "  %s", str_dict);

        if (lang)
            free (lang);
        if (country)
            free (country);
    }

    delete_aspell_dict_info_enumeration (elements);
    delete_aspell_config (config);
}

/*
 * Sets a list of dictionaries for a buffer.
 */

void
weechat_aspell_command_set_dict (struct t_gui_buffer *buffer, const char *value)
{
    char *name;

    name = weechat_aspell_build_option_name (buffer);
    if (!name)
        return;

    if (weechat_aspell_config_set_dict (name, value) > 0)
    {
        if (value && value[0])
            weechat_printf (NULL, "%s: \"%s\" => %s",
                            ASPELL_PLUGIN_NAME, name, value);
        else
            weechat_printf (NULL, _("%s: \"%s\" removed"),
                            ASPELL_PLUGIN_NAME, name);
    }

    free (name);
}

/*
 * Adds a word in personal dictionary.
 */

void
weechat_aspell_command_add_word (struct t_gui_buffer *buffer, const char *dict,
                                 const char *word)
{
    struct t_aspell_speller_buffer *ptr_speller_buffer;
    AspellSpeller *new_speller, *ptr_speller;

    new_speller = NULL;

    if (dict)
    {
        ptr_speller = weechat_hashtable_get (weechat_aspell_spellers, dict);
        if (!ptr_speller)
        {
            if (!weechat_aspell_speller_dict_supported (dict))
            {
                weechat_printf (NULL,
                                _("%s: error: dictionary \"%s\" is not "
                                  "available on your system"),
                                ASPELL_PLUGIN_NAME, dict);
                return;
            }
            new_speller = weechat_aspell_speller_new (dict);
            if (!new_speller)
                return;
            ptr_speller = new_speller;
        }
    }
    else
    {
        ptr_speller_buffer = weechat_hashtable_get (weechat_aspell_speller_buffer,
                                                    buffer);
        if (!ptr_speller_buffer)
            ptr_speller_buffer = weechat_aspell_speller_buffer_new (buffer);
        if (!ptr_speller_buffer)
            goto error;
        if (!ptr_speller_buffer->spellers || !ptr_speller_buffer->spellers[0])
        {
            weechat_printf (NULL,
                            _("%s%s: no dictionary on this buffer for "
                              "adding word"),
                            weechat_prefix ("error"),
                            ASPELL_PLUGIN_NAME);
            return;
        }
        else if (ptr_speller_buffer->spellers[1])
        {
            weechat_printf (NULL,
                            _("%s%s: many dictionaries are defined for "
                              "this buffer, please specify dictionary"),
                            weechat_prefix ("error"),
                            ASPELL_PLUGIN_NAME);
            return;
        }
        ptr_speller = ptr_speller_buffer->spellers[0];
    }

    if (aspell_speller_add_to_personal (ptr_speller,
                                        word,
                                        strlen (word)) == 1)
    {
        weechat_printf (NULL,
                        _("%s: word \"%s\" added to personal dictionary"),
                        ASPELL_PLUGIN_NAME, word);
    }
    else
        goto error;

    goto end;

error:
    weechat_printf (NULL,
                    _("%s%s: failed to add word to personal "
                      "dictionary"),
                    weechat_prefix ("error"), ASPELL_PLUGIN_NAME);

end:
    if (new_speller)
        weechat_hashtable_remove (weechat_aspell_spellers, dict);
}

/*
 * Callback for command "/aspell".
 */

int
weechat_aspell_command_cb (void *data, struct t_gui_buffer *buffer,
                           int argc, char **argv, char **argv_eol)
{
    char *dicts;
    const char *default_dict;
    struct t_infolist *infolist;
    int number;

    /* make C compiler happy */
    (void) data;

    if (argc == 1)
    {
        /* display aspell status */
        weechat_printf (NULL, "");
        weechat_printf (NULL, "%s",
                        (aspell_enabled) ?
                        _("Aspell is enabled") : _("Aspell is disabled"));
        default_dict = weechat_config_string (weechat_aspell_config_check_default_dict);
        weechat_printf (NULL,
                        _("Default dictionary: %s"),
                        (default_dict && default_dict[0]) ?
                        default_dict : _("(not set)"));
        number = 0;
        infolist = weechat_infolist_get ("option", NULL, "aspell.dict.*");
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

    /* enable aspell */
    if (weechat_strcasecmp (argv[1], "enable") == 0)
    {
        weechat_config_option_set (weechat_aspell_config_check_enabled, "1", 1);
        weechat_printf (NULL, _("Aspell enabled"));
        return WEECHAT_RC_OK;
    }

    /* disable aspell */
    if (weechat_strcasecmp (argv[1], "disable") == 0)
    {
        weechat_config_option_set (weechat_aspell_config_check_enabled, "0", 1);
        weechat_printf (NULL, _("Aspell disabled"));
        return WEECHAT_RC_OK;
    }

    /* toggle aspell */
    if (weechat_strcasecmp (argv[1], "toggle") == 0)
    {
        if (aspell_enabled)
        {
            weechat_config_option_set (weechat_aspell_config_check_enabled, "0", 1);
            weechat_printf (NULL, _("Aspell disabled"));
        }
        else
        {
            weechat_config_option_set (weechat_aspell_config_check_enabled, "1", 1);
            weechat_printf (NULL, _("Aspell enabled"));
        }
        return WEECHAT_RC_OK;
    }

    /* list of dictionaries */
    if (weechat_strcasecmp (argv[1], "listdict") == 0)
    {
        weechat_aspell_command_speller_list_dicts ();
        return WEECHAT_RC_OK;
    }

    /* set dictionary for current buffer */
    if (weechat_strcasecmp (argv[1], "setdict") == 0)
    {
        if (argc > 2)
        {
            dicts = weechat_string_replace (argv_eol[2], " ", "");
            weechat_aspell_command_set_dict (buffer,
                                             (dicts) ? dicts : argv[2]);
            if (dicts)
                free (dicts);
        }
        return WEECHAT_RC_OK;
    }

    /* delete dictionary used on current buffer */
    if (weechat_strcasecmp (argv[1], "deldict") == 0)
    {
        weechat_aspell_command_set_dict (buffer, NULL);
        return WEECHAT_RC_OK;
    }

    /* add word to personal dictionary */
    if (weechat_strcasecmp (argv[1], "addword") == 0)
    {
        if (argc > 3)
            weechat_aspell_command_add_word (buffer, argv[2], argv_eol[3]);
        else
            weechat_aspell_command_add_word (buffer, NULL, argv_eol[2]);
        return WEECHAT_RC_OK;
    }

    return WEECHAT_RC_ERROR;
}

/*
 * Hooks aspell command.
 */

void
weechat_aspell_command_init ()
{
    weechat_hook_command ("aspell",
                          N_("aspell plugin configuration"),
                          N_("enable|disable|toggle"
                             " || listdict"
                             " || setdict <dict>[,<dict>...]"
                             " || deldict"
                             " || addword [<dict>] <word>"),
                          N_("  enable: enable aspell\n"
                             " disable: disable aspell\n"
                             "  toggle: toggle aspell\n"
                             "listdict: show installed dictionaries\n"
                             " setdict: set dictionary for current buffer "
                             "(multiple dictionaries can be separated by a "
                             "comma)\n"
                             " deldict: delete dictionary used on current "
                             "buffer\n"
                             " addword: add a word in personal aspell "
                             "dictionary\n"
                             "\n"
                             "Input line beginning with a '/' is not checked, "
                             "except for some commands (see /set "
                             "aspell.check.commands).\n\n"
                             "To enable aspell on all buffers, use option "
                             "\"default_dict\", then enable aspell, for "
                             "example:\n"
                             "  /set aspell.check.default_dict \"en\"\n"
                             "  /aspell enable\n\n"
                             "Default key to toggle aspell is alt-s."),
                          "enable"
                          " || disable"
                          " || toggle"
                          " || listdict"
                          " || setdict %(aspell_dicts)"
                          " || deldict"
                          " || addword",
                          &weechat_aspell_command_cb, NULL);
}
