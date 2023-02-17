/*
 * fset-completion.c - completion for Fast Set commands
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "fset.h"


/*
 * Adds current server to completion list.
 */

int
fset_completion_option_cb (const void *pointer, void *data,
                           const char *completion_item,
                           struct t_gui_buffer *buffer,
                           struct t_gui_completion *completion)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char **words;
    int config_section_added, num_words, i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) completion_item;
    (void) buffer;

    ptr_config = weechat_hdata_get_list (fset_hdata_config_file,
                                         "config_files");
    while (ptr_config)
    {
        ptr_section = weechat_hdata_pointer (fset_hdata_config_file,
                                             ptr_config, "sections");
        while (ptr_section)
        {
            config_section_added = 0;
            ptr_option = weechat_hdata_pointer (fset_hdata_config_section,
                                                ptr_section, "options");
            while (ptr_option)
            {
                if (!config_section_added)
                {
                    weechat_completion_list_add (
                        completion,
                        weechat_config_option_get_string (ptr_option,
                                                          "config_name"),
                        0, WEECHAT_LIST_POS_SORT);
                    weechat_completion_list_add (
                        completion,
                        weechat_config_option_get_string (ptr_option,
                                                          "section_name"),
                        0, WEECHAT_LIST_POS_SORT);
                    config_section_added = 1;
                }
                weechat_completion_list_add (
                    completion,
                    weechat_config_option_get_string (ptr_option, "name"),
                    0,
                    WEECHAT_LIST_POS_SORT);
                words = weechat_string_split (
                    weechat_config_option_get_string (ptr_option, "name"),
                    "_",
                    NULL,
                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                    0,
                    &num_words);
                if (words && (num_words > 1))
                {
                    for (i = 0; i < num_words; i++)
                    {
                        weechat_completion_list_add (
                            completion, words[i], 0, WEECHAT_LIST_POS_SORT);
                    }
                }
                if (words)
                    weechat_string_free_split (words);
                ptr_option = weechat_hdata_move (fset_hdata_config_option,
                                                 ptr_option, 1);
            }
            ptr_section = weechat_hdata_move (fset_hdata_config_section,
                                              ptr_section, 1);
        }
        ptr_config = weechat_hdata_move (fset_hdata_config_file,
                                         ptr_config, 1);
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks completions.
 */

void
fset_completion_init ()
{
    weechat_hook_completion ("fset_options",
                             N_("configuration files, sections, options and "
                                "words of options"),
                             &fset_completion_option_cb, NULL, NULL);
}
