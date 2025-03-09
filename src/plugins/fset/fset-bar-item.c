/*
 * fset-bar-item.c - bar item for Fast Set plugin
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include "fset.h"
#include "fset-bar-item.h"
#include "fset-buffer.h"
#include "fset-config.h"
#include "fset-option.h"


struct t_gui_bar_item *fset_bar_item_fset = NULL;


/*
 * Updates fset bar item if fset is enabled.
 */

void
fset_bar_item_update (void)
{
    weechat_bar_item_update (FSET_BAR_ITEM_NAME);
}

/*
 * Returns content of bar item "fset": help on currently selected option.
 */

char *
fset_bar_item_fset_cb (const void *pointer, void *data,
                       struct t_gui_bar_item *item,
                       struct t_gui_window *window,
                       struct t_gui_buffer *buffer,
                       struct t_hashtable *extra_info)
{
    struct t_fset_option *ptr_fset_option;
    struct t_config_option *ptr_option;
    char str_help[8192], **default_and_values;
    const char **ptr_string_values;
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) item;
    (void) window;
    (void) buffer;
    (void) extra_info;

    if (!fset_buffer)
        return NULL;

    ptr_fset_option = weechat_arraylist_get (fset_options,
                                             fset_buffer_selected_line);
    if (!ptr_fset_option)
        return NULL;

    default_and_values = weechat_string_dyn_alloc (256);
    if (!default_and_values)
        return NULL;

    weechat_string_dyn_concat (default_and_values, weechat_color ("bar_fg"), -1);
    weechat_string_dyn_concat (default_and_values, _("default: "), -1);
    if (ptr_fset_option->default_value)
    {
        if (ptr_fset_option->type == FSET_OPTION_TYPE_STRING)
        {
            weechat_string_dyn_concat (default_and_values,
                                       weechat_color (
                                           weechat_config_string (
                                               fset_config_color_help_quotes)),
                                       -1);
            weechat_string_dyn_concat (default_and_values, "\"", -1);
        }
        weechat_string_dyn_concat (
            default_and_values,
            weechat_color (weechat_config_string (fset_config_color_help_default_value)),
            -1);
        weechat_string_dyn_concat (default_and_values,
                                   ptr_fset_option->default_value,
                                   -1);
        if (ptr_fset_option->type == FSET_OPTION_TYPE_STRING)
        {
            weechat_string_dyn_concat (default_and_values,
                                       weechat_color (
                                           weechat_config_string (
                                               fset_config_color_help_quotes)),
                                       -1);
            weechat_string_dyn_concat (default_and_values, "\"", -1);
        }
    }
    else
    {
        weechat_string_dyn_concat (
            default_and_values,
            weechat_color (weechat_config_string (fset_config_color_help_default_value)),
            -1);
        weechat_string_dyn_concat (
            default_and_values,
            FSET_OPTION_VALUE_NULL,
            -1);
    }

    if (ptr_fset_option->type == FSET_OPTION_TYPE_INTEGER)
    {
        ptr_option = weechat_config_get (ptr_fset_option->name);
        if (ptr_option)
        {
            weechat_string_dyn_concat (default_and_values,
                                       weechat_color ("bar_fg"),
                                       -1);
            weechat_string_dyn_concat (default_and_values, ", ", -1);
            weechat_string_dyn_concat (default_and_values, _("values:"), -1);
            weechat_string_dyn_concat (default_and_values, " ", -1);
            weechat_string_dyn_concat (
                default_and_values,
                weechat_color (
                    weechat_config_string (
                        fset_config_color_help_values)),
                -1);
            weechat_string_dyn_concat (default_and_values,
                                       ptr_fset_option->min,
                                       -1);
            weechat_string_dyn_concat (default_and_values,
                                       weechat_color ("bar_fg"),
                                       -1);
            weechat_string_dyn_concat (default_and_values, " ... ", -1);
            weechat_string_dyn_concat (
                default_and_values,
                weechat_color (
                    weechat_config_string (
                        fset_config_color_help_values)),
                -1);
            weechat_string_dyn_concat (default_and_values,
                                       ptr_fset_option->max,
                                       -1);
        }
    }
    else if (ptr_fset_option->type == FSET_OPTION_TYPE_ENUM)
    {
        ptr_option = weechat_config_get (ptr_fset_option->name);
        if (ptr_option)
        {
            ptr_string_values = NULL;
            if (ptr_fset_option->string_values && ptr_fset_option->string_values[0])
            {
                ptr_string_values = weechat_config_option_get_pointer (
                    ptr_option, "string_values");
            }
            if (ptr_string_values)
            {
                weechat_string_dyn_concat (default_and_values,
                                           weechat_color ("bar_fg"),
                                           -1);
                weechat_string_dyn_concat (default_and_values, ", ", -1);
                weechat_string_dyn_concat (default_and_values, _("values:"), -1);
                weechat_string_dyn_concat (default_and_values, " ", -1);
                for (i = 0; ptr_string_values[i]; i++)
                {
                    if (i > 0)
                    {
                        weechat_string_dyn_concat (default_and_values,
                                                   weechat_color ("bar_fg"),
                                                   -1);
                        weechat_string_dyn_concat (default_and_values,
                                                   ", ",
                                                   -1);
                    }
                    weechat_string_dyn_concat (
                        default_and_values,
                        weechat_color (
                            weechat_config_string (
                                fset_config_color_help_values)),
                        -1);
                    weechat_string_dyn_concat (default_and_values,
                                               ptr_string_values[i],
                                               -1);
                }
            }
        }
    }

    snprintf (str_help, sizeof (str_help),
              /* TRANSLATORS: "%s%s%s:" at beginning of string it the name of option */
              _("%s%s%s: %s%s%s %s[%s%s]%s"),
              weechat_color (weechat_config_string (fset_config_color_help_name)),
              ptr_fset_option->name,
              weechat_color ("bar_fg"),
              weechat_color (weechat_config_string (fset_config_color_help_description)),
              (ptr_fset_option->description && ptr_fset_option->description[0]) ?
              _(ptr_fset_option->description) : _("(no description)"),
              weechat_color ("bar_fg"),
              weechat_color ("bar_delim"),
              *default_and_values,
              weechat_color ("bar_delim"),
              weechat_color ("bar_fg"));

    weechat_string_dyn_free (default_and_values, 1);

    return strdup (str_help);
}

/*
 * Initializes fset bar items.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
fset_bar_item_init (void)
{
    fset_bar_item_fset = weechat_bar_item_new (
        FSET_BAR_ITEM_NAME,
        &fset_bar_item_fset_cb, NULL, NULL);

    return 1;
}

/*
 * Ends fset bar items.
 */

void
fset_bar_item_end (void)
{
    weechat_bar_item_remove (fset_bar_item_fset);
    fset_bar_item_fset = NULL;
}
