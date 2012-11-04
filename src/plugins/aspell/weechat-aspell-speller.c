/*
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2012 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * weechat-aspell-speller.c: speller management for aspell plugin
 */

#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "weechat-aspell.h"
#include "weechat-aspell-speller.h"


struct t_aspell_speller *weechat_aspell_spellers = NULL;
struct t_aspell_speller *last_weechat_aspell_speller = NULL;


/*
 * weechat_aspell_speller_exists: return 1 if an aspell dict exists for a lang,
 *                                0 otherwise
 */

int
weechat_aspell_speller_exists (const char *lang)
{
    struct AspellConfig *config;
    AspellDictInfoList *list;
    AspellDictInfoEnumeration *elements;
    const AspellDictInfo *dict;
    int rc;

    rc = 0;

    config = new_aspell_config ();
    list = get_aspell_dict_info_list (config);
    elements = aspell_dict_info_list_elements (list);

    while ((dict = aspell_dict_info_enumeration_next (elements)) != NULL)
    {
        if (strcmp (dict->name, lang) == 0)
        {
            rc = 1;
            break;
        }
    }

    delete_aspell_dict_info_enumeration (elements);
    delete_aspell_config (config);

    return rc;
}

/*
 * weechat_aspell_speller_check_dictionaries: check dictionaries (called when
 *                                            user creates/changes dictionaries
 *                                            for a buffer)
 */

void
weechat_aspell_speller_check_dictionaries (const char *dict_list)
{
    char **argv;
    int argc, i;

    if (dict_list)
    {
        argv = weechat_string_split (dict_list, ",", 0, 0, &argc);
        if (argv)
        {
            for (i = 0; i < argc; i++)
            {
                if (!weechat_aspell_speller_exists (argv[i]))
                {
                    weechat_printf (NULL,
                                    _("%s: warning: dictionary \"%s\" is not "
                                      "available on your system"),
                                    ASPELL_PLUGIN_NAME, argv[i]);
                }
            }
            weechat_string_free_split (argv);
        }
    }
}

/*
 * weechat_aspell_speller_search: search a speller
 */

struct t_aspell_speller *
weechat_aspell_speller_search (const char *lang)
{
    struct t_aspell_speller *ptr_speller;

    for (ptr_speller = weechat_aspell_spellers; ptr_speller;
         ptr_speller = ptr_speller->next_speller)
    {
        if (strcmp (ptr_speller->lang, lang) == 0)
            return ptr_speller;
    }

    /* no speller found */
    return NULL;
}

/*
 * weechat_aspell_speller_new: create and add a new speller instance
 */

struct t_aspell_speller *
weechat_aspell_speller_new (const char *lang)
{
    struct t_aspell_speller *new_speller;
    AspellConfig *config;
    AspellCanHaveError *ret;
    struct t_infolist *infolist;

    if (!lang)
        return NULL;

    if (weechat_aspell_plugin->debug)
    {
        weechat_printf (NULL,
                        "%s: creating new speller for lang \"%s\"",
                        ASPELL_PLUGIN_NAME, lang);
    }

    /* create a speller instance for the newly created cell */
    config = new_aspell_config();
    aspell_config_replace (config, "lang", lang);

    /* apply all options on speller */
    infolist = weechat_infolist_get ("option", NULL, "aspell.option.*");
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            aspell_config_replace (config,
                                   weechat_infolist_string (infolist, "option_name"),
                                   weechat_infolist_string (infolist, "value"));
        }
        weechat_infolist_free (infolist);
    }

    ret = new_aspell_speller (config);

    if (aspell_error (ret) != 0)
    {
        weechat_printf (NULL,
                        "%s%s: error: %s",
                        weechat_prefix ("error"), ASPELL_PLUGIN_NAME,
                        aspell_error_message (ret));
        delete_aspell_config (config);
        delete_aspell_can_have_error (ret);
        return NULL;
    }

    /* create and add a new speller cell */
    new_speller = malloc (sizeof (*new_speller));
    if (!new_speller)
    {
        weechat_printf (NULL,
                        _("%s%s: not enough memory to create new speller"),
                        weechat_prefix ("error"), ASPELL_PLUGIN_NAME);
        return NULL;
    }

    new_speller->speller = to_aspell_speller (ret);
    new_speller->lang = strdup (lang);

    /* add speller to list */
    new_speller->prev_speller = last_weechat_aspell_speller;
    new_speller->next_speller = NULL;
    if (weechat_aspell_spellers)
        last_weechat_aspell_speller->next_speller = new_speller;
    else
        weechat_aspell_spellers = new_speller;
    last_weechat_aspell_speller = new_speller;

    /* free config */
    delete_aspell_config (config);

    return new_speller;
}

/*
 * weechat_aspell_speller_free: remove a speller instance
 */

void
weechat_aspell_speller_free (struct t_aspell_speller *speller)
{
    if (!speller)
        return;

    if (weechat_aspell_plugin->debug)
    {
        weechat_printf (NULL,
                        "%s: removing speller for lang \"%s\"",
                        ASPELL_PLUGIN_NAME, speller->lang);
    }

    /* free data */
    if (speller->speller)
    {
        aspell_speller_save_all_word_lists (speller->speller);
        delete_aspell_speller (speller->speller);
    }
    if (speller->lang)
        free (speller->lang);

    /* remove speller from list */
    if (speller->prev_speller)
        (speller->prev_speller)->next_speller = speller->next_speller;
    if (speller->next_speller)
        (speller->next_speller)->prev_speller = speller->prev_speller;
    if (weechat_aspell_spellers == speller)
        weechat_aspell_spellers = speller->next_speller;
    if (last_weechat_aspell_speller == speller)
        last_weechat_aspell_speller = speller->prev_speller;

    free (speller);
}

/*
 * weechat_aspell_speller_free_all: free all spellers
 */

void
weechat_aspell_speller_free_all ()
{
    while (weechat_aspell_spellers)
    {
        weechat_aspell_speller_free (weechat_aspell_spellers);
    }
}
