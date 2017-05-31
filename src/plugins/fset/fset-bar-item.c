/*
 * fset-bar-item.c - bar item for Fast Set plugin
 *
 * Copyright (C) 2003-2017 Sébastien Helleu <flashcode@flashtux.org>
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
fset_bar_item_update ()
{
    if (weechat_config_boolean (fset_config_look_help_bar))
        weechat_bar_item_update (FSET_BAR_ITEM_NAME);
}

/*
 * Returns content of bar item "buffer_plugin": bar item with buffer plugin.
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
    char str_help[8192], **string_values;
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

    string_values = weechat_string_dyn_alloc (256);
    if (!string_values)
        return NULL;

    if (ptr_fset_option->string_values && ptr_fset_option->string_values[0])
    {
        ptr_option = weechat_config_get (ptr_fset_option->name);
        if (ptr_option)
        {
            ptr_string_values = weechat_config_option_get_pointer (
                ptr_option, "string_values");
            if (ptr_string_values)
            {
                weechat_string_dyn_concat (string_values,
                                           weechat_color ("bar_fg"));
                weechat_string_dyn_concat (string_values, ", ");
                weechat_string_dyn_concat (string_values, _("values:"));
                weechat_string_dyn_concat (string_values, " ");
                for (i = 0; ptr_string_values[i]; i++)
                {
                    if (i > 0)
                    {
                        weechat_string_dyn_concat (string_values,
                                                   weechat_color ("bar_fg"));
                        weechat_string_dyn_concat (string_values, ", ");
                    }
                    weechat_string_dyn_concat (
                        string_values,
                        weechat_color (
                            weechat_config_string (
                                fset_config_color_help_quotes)));
                    weechat_string_dyn_concat (string_values, "\"");
                    weechat_string_dyn_concat (
                        string_values,
                        weechat_color (
                            weechat_config_string (
                                fset_config_color_help_string_values)));
                    weechat_string_dyn_concat (string_values,
                                               ptr_string_values[i]);
                    weechat_string_dyn_concat (
                        string_values,
                        weechat_color (
                            weechat_config_string (
                                fset_config_color_help_quotes)));
                    weechat_string_dyn_concat (string_values, "\"");
                }
            }
        }
    }

    snprintf (str_help, sizeof (str_help),
              _("%s%s%s: %s %s[%sdefault: %s%s%s%s]%s"),
              weechat_color (weechat_config_string (fset_config_color_help_name)),
              ptr_fset_option->name,
              weechat_color ("bar_fg"),
              ptr_fset_option->description,
              weechat_color ("bar_delim"),
              weechat_color ("bar_fg"),
              weechat_color (weechat_config_string (fset_config_color_help_default_value)),
              ptr_fset_option->default_value,
              *string_values,
              weechat_color ("bar_delim"),
              weechat_color ("bar_fg"));

    weechat_string_dyn_free (string_values, 1);

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
fset_bar_item_init ()
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
fset_bar_item_end ()
{
    weechat_bar_item_remove (fset_bar_item_fset);
}
