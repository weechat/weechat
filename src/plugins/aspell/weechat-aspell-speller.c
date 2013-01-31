/*
 * weechat-aspell-speller.c - speller management for aspell plugin
 *
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2013 Sebastien Helleu <flashcode@flashtux.org>
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
#include <string.h>

#include "../weechat-plugin.h"
#include "weechat-aspell.h"
#include "weechat-aspell-speller.h"
#include "weechat-aspell-config.h"


/*
 * spellers: one by dictionary (key is name of dictionary (eg: "fr"), value is
 * pointer on AspellSpeller)
 */
struct t_hashtable *weechat_aspell_spellers = NULL;

/*
 * spellers by buffer (key is buffer pointer, value is pointer on
 * struct t_aspell_speller_buffer)
 */
struct t_hashtable *weechat_aspell_speller_buffer = NULL;


/*
 * Checks if an aspell dictionary is supported (installed on system).
 *
 * Returns:
 *   1: aspell dict is supported
 *   0: aspell dict is NOT supported
 */

int
weechat_aspell_speller_dict_supported (const char *lang)
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
 * Checks if dictionaries are valid (called when user creates/changes
 * dictionaries for a buffer).
 *
 * An error is displayed for each invalid dictionary found.
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
                if (!weechat_aspell_speller_dict_supported (argv[i]))
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
 * Creates and adds a new speller instance in the hashtable.
 *
 * Returns pointer to new aspell speller, NULL if error.
 */

AspellSpeller *
weechat_aspell_speller_new (const char *lang)
{
    AspellConfig *config;
    AspellCanHaveError *ret;
    AspellSpeller *new_speller;
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

    new_speller = to_aspell_speller (ret);
    weechat_hashtable_set (weechat_aspell_spellers, lang, new_speller);

    /* free configuration */
    delete_aspell_config (config);

    return new_speller;
}

void
weechat_aspell_speller_add_dicts_to_hash (struct t_hashtable *hashtable,
                                          const char *dict)
{
    char **dicts;
    int num_dicts, i;

    if (!dict || !dict[0])
        return;

    dicts = weechat_string_split (dict, ",", 0, 0, &num_dicts);
    if (dicts)
    {
        for (i = 0; i < num_dicts; i++)
        {
            weechat_hashtable_set (hashtable, dicts[i], NULL);
        }
        weechat_string_free_split (dicts);
    }
}

/*
 * Removes a speller if it is NOT in hashtable "used_spellers".
 */

void
weechat_aspell_speller_remove_unused_cb (void *data,
                                         struct t_hashtable *hashtable,
                                         const void *key, const void *value)
{
    struct t_hashtable *used_spellers;

    /* make C compiler happy */
    (void) value;

    used_spellers = (struct t_hashtable *)data;

    /* if speller is not in "used_spellers", remove it (not used any more) */
    if (!weechat_hashtable_has_key (used_spellers, key))
        weechat_hashtable_remove (hashtable, key);
}

/*
 * Removes unused spellers from hashtable "weechat_aspell_spellers".
 */

void
weechat_aspell_speller_remove_unused ()
{
    struct t_hashtable *used_spellers;
    struct t_infolist *infolist;

    if (weechat_aspell_plugin->debug)
    {
        weechat_printf (NULL,
                        "%s: removing unused spellers",
                        ASPELL_PLUGIN_NAME);
    }

    /* create a hashtable that will contain all used spellers */
    used_spellers = weechat_hashtable_new (32,
                                           WEECHAT_HASHTABLE_STRING,
                                           WEECHAT_HASHTABLE_STRING,
                                           NULL,
                                           NULL);
    if (!used_spellers)
        return;

    /* collect used spellers and store them in hashtable "used_spellers" */
    weechat_aspell_speller_add_dicts_to_hash (used_spellers,
                                              weechat_config_string (weechat_aspell_config_check_default_dict));
    infolist = weechat_infolist_get ("option", NULL, "aspell.dict.*");
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            weechat_aspell_speller_add_dicts_to_hash (used_spellers,
                                                      weechat_infolist_string (infolist, "value"));
        }
        weechat_infolist_free (infolist);
    }

    /*
     * look at current spellers, and remove spellers that are not in hashtable
     * "used_spellers"
     */
    weechat_hashtable_map (weechat_aspell_spellers,
                           &weechat_aspell_speller_remove_unused_cb,
                           used_spellers);

    weechat_hashtable_free (used_spellers);
}

/*
 * Callback called when a key is removed in hashtable "weechat_aspell_spellers".
 */

void
weechat_aspell_speller_free_value_cb (struct t_hashtable *hashtable,
                                      const void *key, void *value)
{
    AspellSpeller *ptr_speller;

    /* make C compiler happy */
    (void) hashtable;

    if (weechat_aspell_plugin->debug)
    {
        weechat_printf (NULL,
                        "%s: removing speller for lang \"%s\"",
                        ASPELL_PLUGIN_NAME, (const char *)key);
    }

    /* free aspell data */
    ptr_speller = (AspellSpeller *)value;
    aspell_speller_save_all_word_lists (ptr_speller);
    delete_aspell_speller (ptr_speller);
}

/*
 * Creates a structure for buffer speller info in hashtable
 * "weechat_aspell_buffer_spellers".
 */

struct t_aspell_speller_buffer *
weechat_aspell_speller_buffer_new (struct t_gui_buffer *buffer)
{
    const char *buffer_dicts;
    char **dicts;
    int num_dicts, i;
    struct t_aspell_speller_buffer *new_speller_buffer;
    AspellSpeller *ptr_speller;

    if (!buffer)
        return NULL;

    weechat_hashtable_remove (weechat_aspell_speller_buffer, buffer);

    new_speller_buffer = malloc (sizeof (*new_speller_buffer));
    if (!new_speller_buffer)
        return NULL;

    new_speller_buffer->spellers = NULL;
    new_speller_buffer->modifier_string = NULL;
    new_speller_buffer->input_pos = -1;
    new_speller_buffer->modifier_result = NULL;

    buffer_dicts = weechat_aspell_get_dict (buffer);
    if (buffer_dicts)
    {
        dicts = weechat_string_split (buffer_dicts, ",", 0, 0, &num_dicts);
        if (dicts && (num_dicts > 0))
        {
            new_speller_buffer->spellers =
                malloc ((num_dicts + 1) * sizeof (AspellSpeller *));
            if (new_speller_buffer->spellers)
            {
                for (i = 0; i < num_dicts; i++)
                {
                    ptr_speller = weechat_hashtable_get (weechat_aspell_spellers,
                                                         dicts[i]);
                    if (!ptr_speller)
                        ptr_speller = weechat_aspell_speller_new (dicts[i]);
                    new_speller_buffer->spellers[i] = ptr_speller;
                }
                new_speller_buffer->spellers[num_dicts] = NULL;
            }
        }
        if (dicts)
            weechat_string_free_split (dicts);
    }

    weechat_hashtable_set (weechat_aspell_speller_buffer,
                           buffer,
                           new_speller_buffer);

    weechat_bar_item_update ("aspell_dict");

    return new_speller_buffer;
}

/*
 * Callback called when a key is removed in hashtable
 * "weechat_aspell_speller_buffer".
 */

void
weechat_aspell_speller_buffer_free_value_cb (struct t_hashtable *hashtable,
                                             const void *key, void *value)
{
    struct t_aspell_speller_buffer *ptr_speller_buffer;

    /* make C compiler happy */
    (void) hashtable;
    (void) key;

    ptr_speller_buffer = (struct t_aspell_speller_buffer *)value;

    if (ptr_speller_buffer->spellers)
        free (ptr_speller_buffer->spellers);
    if (ptr_speller_buffer->modifier_string)
        free (ptr_speller_buffer->modifier_string);
    if (ptr_speller_buffer->modifier_result)
        free (ptr_speller_buffer->modifier_result);

    free (ptr_speller_buffer);
}

/*
 * Initializes spellers (creates hashtables).
 *
 * Returns:
 *   1: OK (hashtables created)
 *   0: error (not enough memory)
 */

int
weechat_aspell_speller_init ()
{
    weechat_aspell_spellers = weechat_hashtable_new (32,
                                                     WEECHAT_HASHTABLE_STRING,
                                                     WEECHAT_HASHTABLE_POINTER,
                                                     NULL,
                                                     NULL);
    if (!weechat_aspell_spellers)
        return 0;
    weechat_hashtable_set_pointer (weechat_aspell_spellers,
                                   "callback_free_value",
                                   &weechat_aspell_speller_free_value_cb);

    weechat_aspell_speller_buffer = weechat_hashtable_new (32,
                                                           WEECHAT_HASHTABLE_POINTER,
                                                           WEECHAT_HASHTABLE_POINTER,
                                                           NULL,
                                                           NULL);
    if (!weechat_aspell_speller_buffer)
    {
        weechat_hashtable_free (weechat_aspell_spellers);
        return 0;
    }
    weechat_hashtable_set_pointer (weechat_aspell_speller_buffer,
                                   "callback_free_value",
                                   &weechat_aspell_speller_buffer_free_value_cb);

    return 1;
}

/*
 * Ends spellers (removes hashtables).
 */

void
weechat_aspell_speller_end ()
{
    weechat_hashtable_free (weechat_aspell_spellers);
    weechat_hashtable_free (weechat_aspell_speller_buffer);
}
