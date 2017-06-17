/*
 * fset-buffer.c - buffer for Fast Set plugin
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
#include "fset-buffer.h"
#include "fset-bar-item.h"
#include "fset-config.h"
#include "fset-option.h"


struct t_gui_buffer *fset_buffer = NULL;
int fset_buffer_selected_line = 0;
struct t_hashtable *fset_buffer_hashtable_pointers = NULL;
struct t_hashtable *fset_buffer_hashtable_extra_vars = NULL;


/*
 * Sets title of fset buffer.
 */

void
fset_buffer_set_title ()
{
    int num_options;
    char str_marked[32], str_title[1024];

    if (!fset_buffer)
        return;

    str_marked[0] = '\0';
    if (fset_option_count_marked > 0)
    {
        snprintf (str_marked, sizeof (str_marked),
                  " (*: %s%d%s)",
                  weechat_color (weechat_config_string (fset_config_color_title_marked_options)),
                  fset_option_count_marked,
                  weechat_color ("bar_fg"));
    }

    num_options = weechat_arraylist_size (fset_options);

    snprintf (str_title, sizeof (str_title),
              _("Filter: %s%s%s | %s%d%s/%s%d%s%s | "
                "Sort: %s%s%s | "
                "Key(input): "
                "alt+space=toggle boolean, "
                "alt+'-'(-)=subtract 1 or set, "
                "alt+'+'(+)=add 1 or append, "
                "alt+f,alt+r(r)=reset, "
                "alf+f,alt+u(u)=unset, "
                "alt+enter(s)=set, "
                "alt+f,alt+a(a)=append, "
                "alt+','=mark/unmark, "
                "shift+down=mark and move down, "
                "shift+up=mark and move up, "
                "($)=refresh, "
                "($$)=unmark/refresh, "
                "(m)=mark matching options, "
                "(u)=unmark matching options, "
                "alt+p(p)=toggle plugins desc, "
                "alt+v(v)=toggle help bar, "
                "ctrl+X(x)=switch format, "
                "(q)=close buffer"),
              weechat_color (weechat_config_string (fset_config_color_title_filter)),
              (fset_option_filter) ? fset_option_filter : "*",
              weechat_color ("bar_fg"),
              weechat_color (weechat_config_string (fset_config_color_title_current_option)),
              (num_options > 0) ? fset_buffer_selected_line + 1 : 0,
              weechat_color ("bar_fg"),
              weechat_color (weechat_config_string (fset_config_color_title_count_options)),
              num_options,
              weechat_color ("bar_fg"),
              str_marked,
              weechat_color (weechat_config_string (fset_config_color_title_sort)),
              weechat_config_string (fset_config_look_sort),
              weechat_color ("bar_fg"));

    weechat_buffer_set (fset_buffer, "title", str_title);
}

/*
 * Fills a field with spaces (according to max length in hashtable
 * "fset_option_max_length_field" for this field.
 */

void
fset_buffer_fills_field (char *field, int size,
                         const char *field_name, int default_max_length)
{
    int length, length_screen, *ptr_length, num_spaces;

    length = strlen (field);
    length_screen = weechat_strlen_screen (field);

    ptr_length = (int *)weechat_hashtable_get (fset_option_max_length_field,
                                               field_name);
    if (!ptr_length)
        ptr_length = &default_max_length;

    num_spaces = *ptr_length - length_screen;
    if (num_spaces <= 0)
        return;
    if (length + num_spaces >= size)
        num_spaces = size - length - 1;

    memset (field + length, ' ', num_spaces);
    field[length + num_spaces] = '\0';
}

/*
 * Displays a line with an fset option.
 */

void
fset_buffer_display_line (int y, struct t_fset_option *fset_option)
{
    char *line, str_field[4096], str_color_value[128], str_color_quotes[128];
    const char *ptr_field, *ptr_parent_value;
    int selected_line;
    int default_value_undef, value_undef, value_changed;
    int type, marked, add_quotes, add_quotes_parent, format_number;

    if (!fset_option)
        return;

    selected_line = (y == fset_buffer_selected_line) ? 1 : 0;

    default_value_undef = (fset_option->default_value == NULL) ? 1 : 0;
    value_undef = (fset_option->value == NULL) ? 1 : 0;
    value_changed = (fset_option_value_is_changed (fset_option)) ? 1 : 0;

    /* set pointers */
    weechat_hashtable_set (fset_buffer_hashtable_pointers,
                           "fset_option", fset_option);

    /* file */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "file");
    snprintf (str_field, sizeof (str_field), "%s", ptr_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__file", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (
                  weechat_config_string (
                      (value_changed) ?
                      fset_config_color_name_changed[selected_line] :
                      fset_config_color_name[selected_line])),
              (ptr_field) ? ptr_field : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_file", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "file", 16);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "file", str_field);

    /* section */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "section");
    snprintf (str_field, sizeof (str_field), "%s", ptr_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__section", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (
                  weechat_config_string (
                      (value_changed) ?
                      fset_config_color_name_changed[selected_line] :
                      fset_config_color_name[selected_line])),
              (ptr_field) ? ptr_field : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_section", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "section", 16);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "section", str_field);

    /* option */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "option");
    snprintf (str_field, sizeof (str_field), "%s", ptr_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__option", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (
                  weechat_config_string (
                      (value_changed) ?
                      fset_config_color_name_changed[selected_line] :
                      fset_config_color_name[selected_line])),
              (ptr_field) ? ptr_field : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_option", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "option", 16);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "option", str_field);

    /* name */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "name");
    snprintf (str_field, sizeof (str_field), "%s", ptr_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__name", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (
                  weechat_config_string (
                      (value_changed) ?
                      fset_config_color_name_changed[selected_line] :
                      fset_config_color_name[selected_line])),
              (ptr_field) ? ptr_field : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_name", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "name", 64);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "name", str_field);

    /* parent_name */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "parent_name");
    snprintf (str_field, sizeof (str_field), "%s", ptr_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__parent_name", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_parent_name[selected_line])),
              (ptr_field) ? ptr_field : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_parent_name", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "parent_name", 64);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "parent_name", str_field);

    /* type */
    type = weechat_hdata_integer (fset_hdata_fset_option, fset_option, "type");
    snprintf (str_field, sizeof (str_field),
              "%s", _(fset_option_type_string[type]));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__type", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_type[selected_line])),
              _(fset_option_type_string[type]));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_type", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "type", 8);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "type", str_field);

    /* type_en */
    snprintf (str_field, sizeof (str_field),
              "%s", fset_option_type_string[type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__type_en", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_type[selected_line])),
              fset_option_type_string[type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_type_en", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "type_en", 8);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "type_en", str_field);

    /* type_short */
    snprintf (str_field, sizeof (str_field),
              "%s", fset_option_type_string_short[type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__type_short", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_type[selected_line])),
              fset_option_type_string_short[type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_type_short", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "type_short", 4);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "type_short", str_field);

    /* type_tiny */
    snprintf (str_field, sizeof (str_field),
              "%s", fset_option_type_string_tiny[type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__type_tiny", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_type[selected_line])),
              fset_option_type_string_tiny[type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_type_tiny", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "type_tiny", 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "type_tiny", str_field);

    /* default_value */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "default_value");
    add_quotes = (ptr_field && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
    snprintf (str_color_value, sizeof (str_color_value),
              "%s",
              weechat_color (
                  weechat_config_string (
                      (default_value_undef) ?
                      fset_config_color_value_undef[selected_line] :
                      fset_config_color_default_value[selected_line])));
    snprintf (str_field, sizeof (str_field),
              "%s", (ptr_field) ? ptr_field : FSET_OPTION_VALUE_NULL);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__default_value", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s%s%s%s%s",
              (add_quotes) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
              (add_quotes) ? "\"" : "",
              str_color_value,
              (ptr_field) ? ptr_field : FSET_OPTION_VALUE_NULL,
              (add_quotes) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
              (add_quotes) ? "\"" : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_default_value", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "default_value", 16);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "default_value", str_field);

    /* value */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "value");
    add_quotes = (ptr_field && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
    if ((fset_option->type == FSET_OPTION_TYPE_COLOR)
        && weechat_config_boolean (fset_config_look_use_color_value))
    {
        snprintf (str_color_value, sizeof (str_color_value),
                  "%s",
                  weechat_color (fset_option->value));
    }
    else if (value_undef)
    {
        snprintf (str_color_value, sizeof (str_color_value),
                  "%s",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_value_undef[selected_line])));
        snprintf (str_color_quotes, sizeof (str_color_quotes),
                  "%s",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_quotes[selected_line])));
    }
    else if (value_changed)
    {
        snprintf (str_color_value, sizeof (str_color_value),
                  "%s",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_value_changed[selected_line])));
        snprintf (str_color_quotes, sizeof (str_color_quotes),
                  "%s",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_quotes_changed[selected_line])));
    }
    else
    {
        snprintf (str_color_value, sizeof (str_color_value),
                  "%s",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_value[selected_line])));
        snprintf (str_color_quotes, sizeof (str_color_quotes),
                  "%s",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_quotes[selected_line])));
    }
    snprintf (str_field, sizeof (str_field),
              "%s", (ptr_field) ? ptr_field : FSET_OPTION_VALUE_NULL);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__value", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s%s%s%s%s",
              (add_quotes) ? str_color_quotes : "",
              (add_quotes) ? "\"" : "",
              str_color_value,
              (ptr_field) ? ptr_field : FSET_OPTION_VALUE_NULL,
              (add_quotes) ? str_color_quotes : "",
              (add_quotes) ? "\"" : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_value", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "value", 16);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "value", str_field);

    /* value2 (value with parent value in case of inherited value) */
    ptr_parent_value = weechat_hdata_string (fset_hdata_fset_option,
                                             fset_option, "parent_value");
    if (value_undef && ptr_parent_value)
    {
        add_quotes_parent = (ptr_parent_value && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
        snprintf (str_field, sizeof (str_field),
                  "%s -> %s",
                  (ptr_field) ? ptr_field : FSET_OPTION_VALUE_NULL,
                  (ptr_parent_value) ? ptr_parent_value : FSET_OPTION_VALUE_NULL);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "__value2", str_field);
        snprintf (str_field, sizeof (str_field),
                  "%s%s%s%s%s%s%s -> %s%s%s%s%s%s",
                  (add_quotes) ? str_color_quotes : "",
                  (add_quotes) ? "\"" : "",
                  str_color_value,
                  (ptr_field) ? ptr_field : FSET_OPTION_VALUE_NULL,
                  (add_quotes) ? str_color_quotes : "",
                  (add_quotes) ? "\"" : "",
                  weechat_color ("default"),
                  (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                  (add_quotes_parent) ? "\"" : "",
                  weechat_color (weechat_config_string (fset_config_color_parent_value[selected_line])),
                  (ptr_parent_value) ? ptr_parent_value : FSET_OPTION_VALUE_NULL,
                  (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                  (add_quotes_parent) ? "\"" : "");
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "_value2", str_field);
        fset_buffer_fills_field (str_field, sizeof (str_field), "value2", 32);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "value2", str_field);
    }
    else
    {
        snprintf (str_field, sizeof (str_field),
                  "%s",
                  (ptr_field) ? ptr_field : FSET_OPTION_VALUE_NULL);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "__value2", str_field);
        snprintf (str_field, sizeof (str_field),
              "%s%s%s%s%s%s",
              (add_quotes) ? str_color_quotes : "",
              (add_quotes) ? "\"" : "",
              str_color_value,
              (ptr_field) ? ptr_field : FSET_OPTION_VALUE_NULL,
              (add_quotes) ? str_color_quotes : "",
              (add_quotes) ? "\"" : "");
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "_value2", str_field);
        fset_buffer_fills_field (str_field, sizeof (str_field), "value2", 32);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "value2", str_field);
    }

    /* parent_value (set only if value is NULL and inherited from parent) */
    if (value_undef && ptr_parent_value)
    {
        add_quotes_parent = (ptr_parent_value && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
        snprintf (str_field, sizeof (str_field),
                  "%s",
                  (ptr_parent_value) ? ptr_parent_value : FSET_OPTION_VALUE_NULL);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "__parent_value", str_field);
        snprintf (str_field, sizeof (str_field),
                  "%s%s%s%s%s%s",
                  (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                  (add_quotes_parent) ? "\"" : "",
                  weechat_color (weechat_config_string (fset_config_color_parent_value[selected_line])),
                  (ptr_parent_value) ? ptr_parent_value : FSET_OPTION_VALUE_NULL,
                  (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                  (add_quotes_parent) ? "\"" : "");
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "_parent_value", str_field);
        fset_buffer_fills_field (str_field, sizeof (str_field), "parent_value", 16);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "parent_value", str_field);
    }
    else
    {
        str_field[0] = '\0';
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "__parent_value", str_field);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "_parent_value", str_field);
        fset_buffer_fills_field (str_field, sizeof (str_field), "parent_value", 16);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "parent_value", str_field);
    }

    /* min */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "min");
    snprintf (str_field, sizeof (str_field), "%s", ptr_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__min", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_min[selected_line])),
              (ptr_field) ? ptr_field : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_min", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "min", 8);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "min", str_field);

    /* max */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "max");
    snprintf (str_field, sizeof (str_field), "%s", ptr_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__max", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_max[selected_line])),
              (ptr_field) ? ptr_field : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_max", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "max", 8);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "max", str_field);

    /* description */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "description");
    snprintf (str_field, sizeof (str_field),
              "%s", (ptr_field && ptr_field[0]) ? _(ptr_field) : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__description", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_description[selected_line])),
              (ptr_field && ptr_field[0]) ? _(ptr_field) : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_description", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "description", 64);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "description", str_field);

    /* description2 */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "description");
    snprintf (str_field, sizeof (str_field),
              "%s", (ptr_field && ptr_field[0]) ? _(ptr_field) : _("(no description)"));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__description2", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_description[selected_line])),
              (ptr_field && ptr_field[0]) ? _(ptr_field) : _("(no description)"));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_description2", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "description2", 64);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "description2", str_field);

    /* description_en */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "description");
    snprintf (str_field, sizeof (str_field), "%s", ptr_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__description_en", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_description[selected_line])),
              (ptr_field) ? ptr_field : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_description_en", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "description_en", 64);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "description_en", str_field);

    /* description_en2 */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "description");
    snprintf (str_field, sizeof (str_field),
              "%s", (ptr_field && ptr_field[0]) ? ptr_field : "(no description)");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__description_en2", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_description[selected_line])),
              (ptr_field && ptr_field[0]) ? ptr_field : "(no description)");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_description_en2", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "description_en2", 64);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "description_en2", str_field);

    /* string_values */
    ptr_field = weechat_hdata_string (fset_hdata_fset_option,
                                      fset_option, "string_values");
    snprintf (str_field, sizeof (str_field), "%s", ptr_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__string_values", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_string_values[selected_line])),
              (ptr_field) ? ptr_field : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_string_values", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "string_values", 32);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "string_values", str_field);

    /* marked */
    marked = weechat_hdata_integer (fset_hdata_fset_option,
                                    fset_option, "marked");
    snprintf (str_field, sizeof (str_field),
              "%s",
              (marked) ?
              weechat_config_string (fset_config_look_marked_string) :
              weechat_config_string (fset_config_look_unmarked_string));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__marked", str_field);
    snprintf (str_field, sizeof (str_field),
              "%s%s",
              (marked) ?
              weechat_color (weechat_config_string (fset_config_color_marked[selected_line])) :
              weechat_color (weechat_config_string (fset_config_color_unmarked[selected_line])),
              (marked) ?
              weechat_config_string (fset_config_look_marked_string) :
              weechat_config_string (fset_config_look_unmarked_string));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_marked", str_field);
    fset_buffer_fills_field (str_field, sizeof (str_field), "marked", 2);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "marked", str_field);

    /* set other variables depending on the value */
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "default_value_undef",
                           (default_value_undef) ? "1" : "0");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "value_undef",
                           (value_undef) ? "1" : "0");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "value_changed",
                           (value_changed) ? "1" : "0");

    /* build string for line */
    format_number = weechat_config_integer (fset_config_look_format_number);
    line = weechat_string_eval_expression (
        (selected_line) ?
        fset_config_eval_format_option_current[format_number - 1] :
        weechat_config_string (fset_config_format_option[format_number - 1]),
        fset_buffer_hashtable_pointers,
        fset_buffer_hashtable_extra_vars,
        NULL);

    if (line)
    {
        weechat_printf_y (fset_buffer, y, "%s", line);
        free (line);
    }
}

/*
 * Updates list of options in fset buffer.
 */

void
fset_buffer_refresh (int clear)
{
    int num_options, i;
    struct t_fset_option *ptr_fset_option;

    if (!fset_buffer)
        return;

    num_options = weechat_arraylist_size (fset_options);

    if (clear)
        weechat_buffer_clear (fset_buffer);

    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (ptr_fset_option)
            fset_buffer_display_line (i, ptr_fset_option);
    }

    fset_buffer_set_title ();
    fset_bar_item_update ();
}

/*
 * Sets current selected line.
 */

void
fset_buffer_set_current_line (int line)
{
    int old_line;

    if ((line >= 0) && (line < weechat_arraylist_size (fset_options)))
    {
        old_line = fset_buffer_selected_line;
        fset_buffer_selected_line = line;

        if (old_line != fset_buffer_selected_line)
        {
            fset_buffer_display_line (
                old_line,
                weechat_arraylist_get (fset_options, old_line));
        }
        fset_buffer_display_line (
            fset_buffer_selected_line,
            weechat_arraylist_get (fset_options, fset_buffer_selected_line));

        fset_buffer_set_title ();
        fset_bar_item_update ();
    }
}

/*
 * Gets info about a window.
 */

void
fset_buffer_get_window_info (struct t_gui_window *window,
                             int *start_line_y, int *chat_height)
{
    struct t_hdata *hdata_window, *hdata_window_scroll, *hdata_line;
    struct t_hdata *hdata_line_data;
    void *window_scroll, *start_line, *line_data;

    hdata_window = weechat_hdata_get ("window");
    hdata_window_scroll = weechat_hdata_get ("window_scroll");
    hdata_line = weechat_hdata_get ("line");
    hdata_line_data = weechat_hdata_get ("line_data");
    *start_line_y = 0;
    window_scroll = weechat_hdata_pointer (hdata_window, window, "scroll");
    if (window_scroll)
    {
        start_line = weechat_hdata_pointer (hdata_window_scroll, window_scroll,
                                            "start_line");
        if (start_line)
        {
            line_data = weechat_hdata_pointer (hdata_line, start_line, "data");
            if (line_data)
            {
                *start_line_y = weechat_hdata_integer (hdata_line_data,
                                                       line_data, "y");
            }
        }
    }
    *chat_height = weechat_hdata_integer (hdata_window, window,
                                          "win_chat_height");
}

/*
 * Checks if current line is outside window.
 *
 * Returns:
 *   1: line is outside window
 *   0: line is inside window
 */

void
fset_buffer_check_line_outside_window ()
{
    struct t_gui_window *window;
    int start_line_y, chat_height;
    char str_command[256];

    window = weechat_window_search_with_buffer (fset_buffer);
    if (!window)
        return;

    fset_buffer_get_window_info (window, &start_line_y, &chat_height);
    if ((start_line_y > fset_buffer_selected_line)
        || (start_line_y <= fset_buffer_selected_line - chat_height))
    {
        snprintf (str_command, sizeof (str_command),
                  "/window scroll -window %d %s%d",
                  weechat_window_get_integer (window, "number"),
                  (start_line_y > fset_buffer_selected_line) ? "-" : "+",
                  (start_line_y > fset_buffer_selected_line) ?
                  start_line_y - fset_buffer_selected_line :
                  fset_buffer_selected_line - start_line_y - chat_height + 1);
        weechat_command (fset_buffer, str_command);
    }
}

/*
 * Callback for signal "window_scrolled".
 */

int
fset_buffer_window_scrolled_cb (const void *pointer, void *data,
                                const char *signal, const char *type_data,
                                void *signal_data)
{
    int start_line_y, chat_height, line, num_options;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    /* scrolled another window/buffer? then just ignore */
    if (weechat_window_get_pointer (signal_data, "buffer") != fset_buffer)
        return WEECHAT_RC_OK;

    fset_buffer_get_window_info (signal_data, &start_line_y, &chat_height);

    line = fset_buffer_selected_line;
    while (line < start_line_y)
    {
        line += chat_height;
    }
    while (line >= start_line_y + chat_height)
    {
        line -= chat_height;
    }
    if (line < start_line_y)
        line = start_line_y;
    num_options = weechat_arraylist_size (fset_options);
    if (line >= num_options)
        line = num_options - 1;
    fset_buffer_set_current_line (line);

    return WEECHAT_RC_OK;
}

/*
 * Callback for user data in fset buffer.
 */

int
fset_buffer_input_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer,
                      const char *input_data)
{
    char *actions[][2] = {
        { "<<", "/fset -go 0"   },
        { ">>", "/fset -go end" },
        { "<",  "/fset -left"   },
        { ">",  "/fset -right"  },
        { "t",  "/fset -toggle" },
        { "-",  "/fset -add -1" },
        { "+",  "/fset -add 1"  },
        { "r",  "/fset -reset"  },
        { "u",  "/fset -unset"  },
        { "s",  "/fset -set"    },
        { "a",  "/fset -append" },
        { ",",  "/fset -mark 1" },
        { "p",  "/mute /set fset.look.show_plugins_desc toggle", },
        { "v",  "/mute /set fset.look.show_help_bar toggle"      },
        { "x",  "/fset -format" },
        { NULL, NULL            },
    };
    const char *ptr_input;
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* close buffer */
    if (strcmp (input_data, "q") == 0)
    {
        weechat_buffer_close (buffer);
        return WEECHAT_RC_OK;
    }

    /* refresh buffer */
    if (strcmp (input_data, "$") == 0)
    {
        fset_option_get_options ();
        fset_buffer_refresh (0);
        return WEECHAT_RC_OK;
    }

    /* unmark all options and refresh buffer */
    if (strcmp (input_data, "$$") == 0)
    {
        fset_option_unmark_all ();
        fset_option_get_options ();
        fset_buffer_refresh (0);
        return WEECHAT_RC_OK;
    }

    /* mark options matching filter */
    if (strncmp (input_data, "m:", 2) == 0)
    {
        fset_option_mark_options_matching_filter (input_data + 2, 1);
        return WEECHAT_RC_OK;
    }

    /* unmark options matching filter */
    if (strncmp (input_data, "u:", 2) == 0)
    {
        fset_option_mark_options_matching_filter (input_data + 2, 0);
        return WEECHAT_RC_OK;
    }

    /* change sort of options */
    if (strncmp (input_data, "s:", 2) == 0)
    {
        if (input_data[2])
            weechat_config_option_set (fset_config_look_sort, input_data + 2, 1);
        else
            weechat_config_option_reset (fset_config_look_sort, 1);
        return WEECHAT_RC_OK;
    }

    /* export options in a file */
    if (strncmp (input_data, "w:", 2) == 0)
    {
        if (input_data[2])
        {
            fset_option_export (
                input_data + 2,
                weechat_config_boolean (fset_config_look_export_help_default));
        }
        return WEECHAT_RC_OK;
    }

    /* export options in a file (without help) */
    if (strncmp (input_data, "w-:", 3) == 0)
    {
        if (input_data[3])
            fset_option_export (input_data + 3, 0);
        return WEECHAT_RC_OK;
    }

    /* export options in a file (with help) */
    if (strncmp (input_data, "w+:", 3) == 0)
    {
        if (input_data[3])
            fset_option_export (input_data + 3, 1);
        return WEECHAT_RC_OK;
    }

    /* execute action on an option */
    for (i = 0; actions[i][0]; i++)
    {
        if (strcmp (input_data, actions[i][0]) == 0)
        {
            weechat_command (buffer, actions[i][1]);
            return WEECHAT_RC_OK;
        }
    }

    /* filter options with given text */
    ptr_input = input_data;
    while (ptr_input[0] == ' ')
    {
        ptr_input++;
    }
    if (ptr_input[0])
        fset_option_filter_options (ptr_input);

    return WEECHAT_RC_OK;
}

/*
 * Callback called when fset buffer is closed.
 */

int
fset_buffer_close_cb (const void *pointer, void *data,
                      struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    fset_buffer = NULL;
    fset_buffer_selected_line = 0;
    weechat_arraylist_clear (fset_options);
    fset_option_count_marked = 0;

    return WEECHAT_RC_OK;
}

/*
 * Restore buffer callbacks (input and close) for buffer created by fset
 * plugin.
 */

void
fset_buffer_set_callbacks ()
{
    struct t_gui_buffer *ptr_buffer;

    ptr_buffer = weechat_buffer_search (FSET_PLUGIN_NAME, FSET_BUFFER_NAME);
    if (ptr_buffer)
    {
        fset_buffer = ptr_buffer;
        weechat_buffer_set_pointer (fset_buffer, "close_callback", &fset_buffer_close_cb);
        weechat_buffer_set_pointer (fset_buffer, "input_callback", &fset_buffer_input_cb);
    }
}

/*
 * Sets keys on fset buffer.
 */

void
fset_buffer_set_keys ()
{
    char *keys[][2] = {
        { "meta2-A",       "/fset -up"      },
        { "meta2-B",       "/fset -down"    },
        { "meta-meta2-1~", "/fset -go 0"    },
        { "meta-meta2-4~", "/fset -go end"  },
        { "meta2-23~",     "/fset -left"    },
        { "meta2-24~",     "/fset -right"   },
        { "meta- ",        "/fset -toggle"  },
        { "meta--",        "/fset -add -1"  },
        { "meta-+",        "/fset -add 1"   },
        { "meta-fmeta-r",  "/fset -reset"   },
        { "meta-fmeta-u",  "/fset -unset"   },
        { "meta-ctrl-J",   "/fset -set"     },
        { "meta-ctrl-M",   "/fset -set"     },
        { "meta-fmeta-a",  "/fset -append"  },
        { "meta-,",        "/fset -mark 0"  },
        { "meta2-a",       "/fset -mark -1" },
        { "meta2-b",       "/fset -mark 1"  },
        { "ctrl-L",        "/fset -refresh" },
        { "meta-p",        "/mute /set fset.look.show_plugins_desc toggle", },
        { "meta-v",        "/mute /set fset.look.show_help_bar toggle"      },
        { "ctrl-X",        "/fset -format"  },
        { NULL,            NULL             },
    };
    char str_key[64];
    int i;

    for (i = 0; keys[i][0]; i++)
    {
        if (weechat_config_boolean (fset_config_look_use_keys))
        {
            snprintf (str_key, sizeof (str_key), "key_bind_%s", keys[i][0]);
            weechat_buffer_set (fset_buffer, str_key, keys[i][1]);
        }
        else
        {
            snprintf (str_key, sizeof (str_key), "key_unbind_%s", keys[i][0]);
            weechat_buffer_set (fset_buffer, str_key, "");
        }
    }
}

/*
 * Sets the local variable "filter" in the fset buffer.
 */

void
fset_buffer_set_localvar_filter ()
{
    if (!fset_buffer)
        return;

    weechat_buffer_set (fset_buffer, "localvar_set_filter",
                        (fset_option_filter) ? fset_option_filter : "*");
}

/*
 * Opens fset buffer.
 */

void
fset_buffer_open ()
{
    if (!fset_buffer)
    {
        fset_buffer = weechat_buffer_new (
            FSET_BUFFER_NAME,
            &fset_buffer_input_cb, NULL, NULL,
            &fset_buffer_close_cb, NULL, NULL);

        /* failed to create buffer ? then exit */
        if (!fset_buffer)
            return;

        weechat_buffer_set (fset_buffer, "type", "free");
        fset_buffer_set_keys ();
        weechat_buffer_set (fset_buffer, "localvar_set_type", "option");
        weechat_buffer_set (fset_buffer, "localvar_set_filter", "*");

        fset_buffer_selected_line = 0;
    }
}

/*
 * Initializes fset buffer.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
fset_buffer_init ()
{
    fset_buffer_set_callbacks ();

    /* create hashtables used by the bar item callback */
    fset_buffer_hashtable_pointers = weechat_hashtable_new (
        8,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL,
        NULL);
    if (!fset_buffer_hashtable_pointers)
        return 0;

    fset_buffer_hashtable_extra_vars = weechat_hashtable_new (
        128,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    if (!fset_buffer_hashtable_extra_vars)
    {
        weechat_hashtable_free (fset_buffer_hashtable_pointers);
        return 0;
    }

    return 1;
}

/*
 * Ends fset buffer.
 */

void
fset_buffer_end ()
{
    weechat_hashtable_free (fset_buffer_hashtable_pointers);
    fset_buffer_hashtable_pointers = NULL;

    weechat_hashtable_free (fset_buffer_hashtable_extra_vars);
    fset_buffer_hashtable_extra_vars = NULL;
}
