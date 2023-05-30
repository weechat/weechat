/*
 * fset-buffer.c - buffer for Fast Set plugin
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
    char str_marked[32], str_title[8192];

    if (!fset_buffer)
        return;

    str_marked[0] = '\0';
    if (fset_option_count_marked > 0)
    {
        snprintf (str_marked, sizeof (str_marked),
                  " (%s: %s%d%s)",
                  weechat_config_string (fset_config_look_marked_string),
                  weechat_color (weechat_config_string (fset_config_color_title_marked_options)),
                  fset_option_count_marked,
                  weechat_color ("bar_fg"));
    }

    num_options = weechat_arraylist_size (fset_options);

    snprintf (str_title, sizeof (str_title),
              _("%s%d%s/%s%d%s%s | Filter: %s%s%s | Sort: %s%s%s | "
                "Key(input): "
                "alt+space=toggle boolean, "
                "alt+'-'(-)=subtract 1 or set, "
                "alt+'+'(+)=add 1 or append, "
                "alt+f,alt+r(r)=reset, "
                "alt+f,alt+u(u)=unset, "
                "alt+enter(s)=set, "
                "alt+f,alt+n(n)=set new value, "
                "alt+f,alt+a(a)=append, "
                "alt+','=mark/unmark, "
                "shift+down=mark and move down, "
                "shift+up=move up and mark, "
                "($)=refresh, "
                "($$)=unmark/refresh, "
                "(m)=mark matching options, "
                "(u)=unmark matching options, "
                "alt+p(p)=toggle plugins desc, "
                "alt+v(v)=toggle help bar, "
                "ctrl+x(x)=switch format, "
                "(q)=close buffer"),
              weechat_color (weechat_config_string (fset_config_color_title_current_option)),
              (num_options > 0) ? fset_buffer_selected_line + 1 : 0,
              weechat_color ("bar_fg"),
              weechat_color (weechat_config_string (fset_config_color_title_count_options)),
              num_options,
              weechat_color ("bar_fg"),
              str_marked,
              weechat_color (weechat_config_string (fset_config_color_title_filter)),
              (fset_option_filter) ? fset_option_filter : "*",
              weechat_color ("bar_fg"),
              weechat_color (weechat_config_string (fset_config_color_title_sort)),
              weechat_config_string (fset_config_look_sort),
              weechat_color ("bar_fg"));

    weechat_buffer_set (fset_buffer, "title", str_title);
}

/*
 * Fills a field with spaces (according to max length for this field).
 *
 * If fill_right == 1, fills with spaces on the right. Otherwise
 * fills with spaces on the left before the value.
 *
 * If skip_colors == 1, the field value may contain color codes, so
 * weechat_strlen_screen() is used instead of weechat_utf8_strlen_screen()
 * and then the functions is slower.
 */

void
fset_buffer_fills_field (char *field, char *field_spaces,
                         int size, int max_length,
                         int fill_right, int skip_colors)
{
    int length, length_screen, num_spaces;

    length = strlen (field);
    length_screen = (skip_colors) ?
        weechat_strlen_screen (field) : weechat_utf8_strlen_screen (field);

    if (max_length > size - 1)
        max_length = size - 1;

    num_spaces = max_length - length_screen;
    if (num_spaces > 0)
    {
        if (length + num_spaces >= size)
            num_spaces = size - length - 1;

        if (fill_right)
        {
            /* add spaces after the value */
            memset (field + length, ' ', num_spaces);
        }
        else
        {
            /* insert spaces before the value */
            memmove (field + num_spaces, field, length);
            memset (field, ' ', num_spaces);
        }
        field[length + num_spaces] = '\0';
    }

    /* field with spaces */
    if (field_spaces)
    {
        memset (field_spaces, ' ', max_length);
        field_spaces[max_length] = '\0';
    }
}

/*
 * Displays a line with an fset option using an evaluated format.
 *
 * Returns the index of last line displayed in buffer (this depends on the
 * format number used), -1 if no option was displayed.
 */

int
fset_buffer_display_option_eval (struct t_fset_option *fset_option)
{
    char *line, str_color_line[128], **lines;
    char *str_field, *str_field2;
    char str_color_value[128], str_color_quotes[128], str_color_name[512];
    char str_number[64];
    int length, length_field, selected_line, y, y_max, i, num_lines;
    int default_value_undef, value_undef, value_changed;
    int add_quotes, add_quotes_parent, format_number;

    if (!fset_option)
        return -1;

    y_max = -1;

    length_field = 4096;
    length = (fset_option->value) ?
        strlen (fset_option->value) + 256 : 0;
    if (length > length_field)
        length_field = length;
    length = (fset_option->default_value) ?
        strlen (fset_option->default_value) + 256 : 0;
    if (length > length_field)
        length_field = length;

    str_field = malloc (length_field);
    if (!str_field)
        return -1;
    str_field2 = malloc (length_field);
    if (!str_field2)
    {
        free (str_field);
        return -1;
    }

    selected_line = (fset_option->index == fset_buffer_selected_line) ? 1 : 0;

    default_value_undef = (fset_option->default_value == NULL) ? 1 : 0;
    value_undef = (fset_option->value == NULL) ? 1 : 0;
    value_changed = (fset_option_value_is_changed (fset_option)) ? 1 : 0;

    /* set pointers */
    weechat_hashtable_set (fset_buffer_hashtable_pointers,
                           "fset_option", fset_option);

    /* file */
    snprintf (str_field, length_field, "%s", fset_option->file);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__file", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (
                  weechat_config_string (
                      (value_changed) ?
                      fset_config_color_name_changed[selected_line] :
                      fset_config_color_name[selected_line])),
              (fset_option->file) ? fset_option->file : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_file", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->file, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "file", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_file", str_field2);

    /* section */
    snprintf (str_field, length_field, "%s", fset_option->section);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__section", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (
                  weechat_config_string (
                      (value_changed) ?
                      fset_config_color_name_changed[selected_line] :
                      fset_config_color_name[selected_line])),
              (fset_option->section) ? fset_option->section : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_section", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->section, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "section", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_section", str_field2);

    /* option */
    snprintf (str_field, length_field, "%s", fset_option->option);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__option", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (
                  weechat_config_string (
                      (value_changed) ?
                      fset_config_color_name_changed[selected_line] :
                      fset_config_color_name[selected_line])),
              (fset_option->option) ? fset_option->option : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_option", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->option, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "option", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_option", str_field2);

    /* name */
    snprintf (str_field, length_field, "%s", fset_option->name);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__name", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (
                  weechat_config_string (
                      (value_changed) ?
                      fset_config_color_name_changed[selected_line] :
                      fset_config_color_name[selected_line])),
              (fset_option->name) ? fset_option->name : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_name", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->name, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "name", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_name", str_field2);

    /* parent_name */
    snprintf (str_field, length_field, "%s", fset_option->parent_name);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__parent_name", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_parent_name[selected_line])),
              (fset_option->parent_name) ? fset_option->parent_name : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_parent_name", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->parent_name, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "parent_name", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_parent_name", str_field2);

    /* type */
    snprintf (str_field, length_field,
              "%s", _(fset_option_type_string[fset_option->type]));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__type", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_type[selected_line])),
              _(fset_option_type_string[fset_option->type]));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_type", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->type, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "type", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_type", str_field2);

    /* type_en */
    snprintf (str_field, length_field,
              "%s", fset_option_type_string[fset_option->type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__type_en", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_type[selected_line])),
              fset_option_type_string[fset_option->type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_type_en", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->type_en, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "type_en", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_type_en", str_field2);

    /* type_short */
    snprintf (str_field, length_field,
              "%s", fset_option_type_string_short[fset_option->type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__type_short", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_type[selected_line])),
              fset_option_type_string_short[fset_option->type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_type_short", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->type_short, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "type_short", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_type_short", str_field2);

    /* type_tiny */
    snprintf (str_field, length_field,
              "%s", fset_option_type_string_tiny[fset_option->type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__type_tiny", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_type[selected_line])),
              fset_option_type_string_tiny[fset_option->type]);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_type_tiny", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->type_tiny, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "type_tiny", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_type_tiny", str_field2);

    /* default_value */
    add_quotes = (fset_option->default_value && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
    snprintf (str_color_value, sizeof (str_color_value),
              "%s",
              weechat_color (
                  weechat_config_string (
                      (default_value_undef) ?
                      fset_config_color_value_undef[selected_line] :
                      fset_config_color_default_value[selected_line])));
    str_color_name[0] = '\0';
    if ((fset_option->type == FSET_OPTION_TYPE_COLOR)
        && weechat_config_boolean (fset_config_look_use_color_value)
        && fset_option->default_value)
    {
        snprintf (str_color_name, sizeof (str_color_name),
                  "%s (%s%s%s)",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_quotes[selected_line])),
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_color_name[selected_line])),
                  fset_option->default_value,
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_quotes[selected_line])));
    }
    snprintf (str_field, length_field,
              "%s", (fset_option->default_value) ? fset_option->default_value : FSET_OPTION_VALUE_NULL);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__default_value", str_field);
    snprintf (str_field, length_field,
              "%s%s%s%s%s%s%s",
              (add_quotes) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
              (add_quotes) ? "\"" : "",
              str_color_value,
              (fset_option->default_value) ? fset_option->default_value : FSET_OPTION_VALUE_NULL,
              (add_quotes) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
              (add_quotes) ? "\"" : "",
              str_color_name);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_default_value", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->default_value, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "default_value", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_default_value", str_field2);

    /* value */
    add_quotes = (fset_option->value && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
    if ((fset_option->type == FSET_OPTION_TYPE_COLOR)
        && weechat_config_boolean (fset_config_look_use_color_value))
    {
        snprintf (str_color_value, sizeof (str_color_value),
                  "%s",
                  weechat_color (fset_option->value));
        snprintf (str_color_quotes, sizeof (str_color_quotes),
                  "%s",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_quotes[selected_line])));
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
    str_color_name[0] = '\0';
    if ((fset_option->type == FSET_OPTION_TYPE_COLOR)
        && weechat_config_boolean (fset_config_look_use_color_value))
    {
        snprintf (str_color_name, sizeof (str_color_name),
                  "%s (%s%s%s)",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_quotes[selected_line])),
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_color_name[selected_line])),
                  fset_option->value,
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_quotes[selected_line])));
    }
    snprintf (str_field, length_field,
              "%s", (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__value", str_field);
    snprintf (str_field, length_field,
              "%s%s%s%s%s%s%s",
              (add_quotes) ? str_color_quotes : "",
              (add_quotes) ? "\"" : "",
              str_color_value,
              (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
              (add_quotes) ? str_color_quotes : "",
              (add_quotes) ? "\"" : "",
              str_color_name);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_value", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->value, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "value", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_value", str_field2);

    /* value2 (value with parent value in case of inherited value) */
    if (value_undef && fset_option->parent_value)
    {
        add_quotes_parent = (fset_option->parent_value && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
        snprintf (str_field, length_field,
                  "%s -> %s",
                  (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
                  (fset_option->parent_value) ? fset_option->parent_value : FSET_OPTION_VALUE_NULL);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "__value2", str_field);
        snprintf (str_field, length_field,
                  "%s%s%s%s%s%s%s -> %s%s%s%s%s%s%s",
                  (add_quotes) ? str_color_quotes : "",
                  (add_quotes) ? "\"" : "",
                  str_color_value,
                  (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
                  (add_quotes) ? str_color_quotes : "",
                  (add_quotes) ? "\"" : "",
                  weechat_color ("default"),
                  (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                  (add_quotes_parent) ? "\"" : "",
                  weechat_color (weechat_config_string (fset_config_color_parent_value[selected_line])),
                  (fset_option->parent_value) ? fset_option->parent_value : FSET_OPTION_VALUE_NULL,
                  (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                  (add_quotes_parent) ? "\"" : "",
                  str_color_name);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "_value2", str_field);
        fset_buffer_fills_field (str_field, str_field2, length_field,
                                 fset_option_max_length->value2, 1, 1);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "value2", str_field);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "empty_value2", str_field2);
    }
    else
    {
        snprintf (str_field, length_field,
                  "%s",
                  (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "__value2", str_field);
        snprintf (str_field, length_field,
                  "%s%s%s%s%s%s%s",
                  (add_quotes) ? str_color_quotes : "",
                  (add_quotes) ? "\"" : "",
                  str_color_value,
                  (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
                  (add_quotes) ? str_color_quotes : "",
                  (add_quotes) ? "\"" : "",
                  str_color_name);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "_value2", str_field);
        fset_buffer_fills_field (str_field, str_field2, length_field,
                                 fset_option_max_length->value2, 1, 1);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "value2", str_field);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "empty_value2", str_field2);
    }

    /* parent_value (set only if value is NULL and inherited from parent) */
    if (value_undef && fset_option->parent_value)
    {
        add_quotes_parent = (fset_option->parent_value && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
        snprintf (str_field, length_field,
                  "%s",
                  (fset_option->parent_value) ? fset_option->parent_value : FSET_OPTION_VALUE_NULL);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "__parent_value", str_field);
        snprintf (str_field, length_field,
                  "%s%s%s%s%s%s",
                  (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                  (add_quotes_parent) ? "\"" : "",
                  weechat_color (weechat_config_string (fset_config_color_parent_value[selected_line])),
                  (fset_option->parent_value) ? fset_option->parent_value : FSET_OPTION_VALUE_NULL,
                  (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                  (add_quotes_parent) ? "\"" : "");
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "_parent_value", str_field);
        fset_buffer_fills_field (str_field, str_field2, length_field,
                                 fset_option_max_length->parent_value, 1, 1);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "parent_value", str_field);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "empty_parent_value", str_field2);
    }
    else
    {
        str_field[0] = '\0';
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "__parent_value", str_field);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "_parent_value", str_field);
        fset_buffer_fills_field (str_field, str_field2, length_field,
                                 fset_option_max_length->parent_value, 1, 1);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "parent_value", str_field);
        weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                               "empty_parent_value", str_field2);
    }

    /* min */
    snprintf (str_field, length_field, "%s", fset_option->min);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__min", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_min[selected_line])),
              (fset_option->min) ? fset_option->min : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_min", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->min, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "min", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_min", str_field2);

    /* max */
    snprintf (str_field, length_field, "%s", fset_option->max);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__max", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_max[selected_line])),
              (fset_option->max) ? fset_option->max : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_max", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->max, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "max", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_max", str_field);

    /* description */
    snprintf (str_field, length_field,
              "%s", (fset_option->description && fset_option->description[0]) ? _(fset_option->description) : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__description", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_description[selected_line])),
              (fset_option->description && fset_option->description[0]) ? _(fset_option->description) : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_description", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->description, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "description", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_description", str_field2);

    /* description2 */
    snprintf (str_field, length_field,
              "%s", (fset_option->description && fset_option->description[0]) ? _(fset_option->description) : _("(no description)"));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__description2", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_description[selected_line])),
              (fset_option->description && fset_option->description[0]) ? _(fset_option->description) : _("(no description)"));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_description2", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->description2, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "description2", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_description2", str_field2);

    /* description_en */
    snprintf (str_field, length_field, "%s", fset_option->description);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__description_en", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_description[selected_line])),
              (fset_option->description) ? fset_option->description : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_description_en", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->description_en, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "description_en", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_description_en", str_field2);

    /* description_en2 */
    snprintf (str_field, length_field,
              "%s", (fset_option->description && fset_option->description[0]) ? fset_option->description : "(no description)");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__description_en2", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_description[selected_line])),
              (fset_option->description && fset_option->description[0]) ? fset_option->description : "(no description)");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_description_en2", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->description_en2, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "description_en2", str_field);

    /* string_values */
    snprintf (str_field, length_field, "%s", fset_option->string_values);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__string_values", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              weechat_color (weechat_config_string (fset_config_color_string_values[selected_line])),
              (fset_option->string_values) ? fset_option->string_values : "");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_string_values", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->string_values, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "string_values", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_string_values", str_field2);

    /* marked */
    snprintf (str_field, length_field,
              "%s",
              (fset_option->marked) ?
              weechat_config_string (fset_config_look_marked_string) :
              weechat_config_string (fset_config_look_unmarked_string));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__marked", str_field);
    snprintf (str_field, length_field,
              "%s%s",
              (fset_option->marked) ?
              weechat_color (weechat_config_string (fset_config_color_marked[selected_line])) :
              weechat_color (weechat_config_string (fset_config_color_unmarked[selected_line])),
              (fset_option->marked) ?
              weechat_config_string (fset_config_look_marked_string) :
              weechat_config_string (fset_config_look_unmarked_string));
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_marked", str_field);
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             fset_option_max_length->marked, 1, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "marked", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_marked", str_field2);

    /* index */
    snprintf (str_field, length_field, "%d", fset_option->index + 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "__index", str_field);
    snprintf (str_field, length_field,
              "%s%d",
              weechat_color (weechat_config_string (fset_config_color_index[selected_line])),
              fset_option->index + 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "_index", str_field);
    snprintf (str_number, sizeof (str_number),
              "%d", weechat_arraylist_size (fset_options));
    fset_buffer_fills_field (str_field, str_field2, length_field,
                             strlen (str_number), 0, 1);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "index", str_field);
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "empty_index", str_field2);

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

    /* set other variables */
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "selected_line",
                           (selected_line) ? "1" : "0");
    weechat_hashtable_set (fset_buffer_hashtable_extra_vars,
                           "newline", "\r\n");

    /* build string for line */
    str_color_line[0] = '\0';
    format_number = weechat_config_integer (fset_config_look_format_number);
    if (selected_line)
    {
        snprintf (str_color_line, sizeof (str_color_line),
                  ",%s",
                  weechat_config_string (
                      fset_config_color_line_selected_bg[format_number - 1]));
    }
    else if (fset_option->marked)
    {
        snprintf (str_color_line, sizeof (str_color_line),
                  ",%s",
                  weechat_config_string (
                      fset_config_color_line_marked_bg[format_number - 1]));
    }

    /* evaluate line */
    line = weechat_string_eval_expression (
        weechat_config_string (fset_config_format_option[format_number - 1]),
        fset_buffer_hashtable_pointers,
        fset_buffer_hashtable_extra_vars,
        NULL);
    if (line)
    {
        lines = weechat_string_split (line, "\r\n", NULL,
                                      WEECHAT_STRING_SPLIT_STRIP_LEFT
                                      | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                      | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                      0, &num_lines);
        if (lines)
        {
            y = fset_option->index * fset_config_format_option_num_lines[format_number - 1];
            for (i = 0; i < num_lines; i++)
            {
                weechat_printf_y (
                    fset_buffer, y,
                    "%s%s",
                    (str_color_line[0]) ? weechat_color (str_color_line) : "",
                    lines[i]);
                y_max = y;
                y++;
            }
            weechat_string_free_split (lines);
        }
        free (line);
    }

    free (str_field);
    free (str_field2);

    return y_max;
}

/*
 * Displays a line with an fset option using a predefined format
 * (much faster because there is no eval).
 *
 * Returns the index of last line displayed in buffer (this depends on the
 * format number used), -1 if no option was displayed.
 */

int
fset_buffer_display_option_predefined_format (struct t_fset_option *fset_option)
{
    int selected_line, value_undef, value_changed, format_number;
    int add_quotes, add_quotes_parent, length_value;
    char str_marked[128], str_name[4096], str_type[128], *str_value;
    char str_color_line[128], str_color_value[128], str_color_quotes[128];
    char str_color_name[512];

    if (!fset_option)
        return -1;

    selected_line = (fset_option->index == fset_buffer_selected_line) ? 1 : 0;
    value_undef = (fset_option->value == NULL) ? 1 : 0;
    value_changed = (fset_option_value_is_changed (fset_option)) ? 1 : 0;
    format_number = weechat_config_integer (fset_config_look_format_number);

    str_color_line[0] = '\0';
    if (selected_line)
    {
        snprintf (str_color_line, sizeof (str_color_line),
                  ",%s",
                  weechat_config_string (
                      fset_config_color_line_selected_bg[format_number - 1]));
    }
    else if (fset_option->marked)
    {
        snprintf (str_color_line, sizeof (str_color_line),
                  ",%s",
                  weechat_config_string (
                      fset_config_color_line_marked_bg[format_number - 1]));
    }

    snprintf (str_marked, sizeof (str_marked),
              "%s",
              (fset_option->marked) ?
              weechat_config_string (fset_config_look_marked_string) :
              weechat_config_string (fset_config_look_unmarked_string));
    fset_buffer_fills_field (str_marked, NULL, sizeof (str_marked),
                             fset_option_max_length->marked, 1, 0);

    snprintf (str_name, sizeof (str_name),
              "%s",
              (fset_option->name) ? fset_option->name : "");
    fset_buffer_fills_field (str_name, NULL, sizeof (str_name),
                             fset_option_max_length->name, 1, 0);

    snprintf (str_type, sizeof (str_type),
              "%s",
              _(fset_option_type_string[fset_option->type]));
    fset_buffer_fills_field (str_type, NULL, sizeof (str_type),
                             fset_option_max_length->type, 1, 0);

    add_quotes = (fset_option->value && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
    if ((fset_option->type == FSET_OPTION_TYPE_COLOR)
        && weechat_config_boolean (fset_config_look_use_color_value))
    {
        snprintf (str_color_value, sizeof (str_color_value),
                  "%s",
                  weechat_color (fset_option->value));
        snprintf (str_color_quotes, sizeof (str_color_quotes),
                  "%s",
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_quotes[selected_line])));
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
    str_color_name[0] = '\0';
    if ((fset_option->type == FSET_OPTION_TYPE_COLOR)
        && weechat_config_boolean (fset_config_look_use_color_value)
        && fset_option->value)
    {
        snprintf (str_color_name, sizeof (str_color_name),
                  "%s (%s%s%s)",
                  str_color_quotes,
                  weechat_color (
                      weechat_config_string (
                          fset_config_color_color_name[selected_line])),
                  fset_option->value,
                  str_color_quotes);
    }
    length_value = (fset_option->value) ?
        strlen (fset_option->value) + 256 : 4096;
    str_value = malloc (length_value);
    if (str_value)
    {
        if (value_undef && fset_option->parent_value)
        {
            add_quotes_parent = (fset_option->parent_value && (fset_option->type == FSET_OPTION_TYPE_STRING)) ? 1 : 0;
            snprintf (str_value, length_value,
                      "%s%s%s%s%s%s%s -> %s%s%s%s%s%s%s",
                      (add_quotes) ? str_color_quotes : "",
                      (add_quotes) ? "\"" : "",
                      str_color_value,
                      (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
                      (add_quotes) ? str_color_quotes : "",
                      (add_quotes) ? "\"" : "",
                      weechat_color ("default"),
                      (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                      (add_quotes_parent) ? "\"" : "",
                      weechat_color (weechat_config_string (fset_config_color_parent_value[selected_line])),
                      (fset_option->parent_value) ? fset_option->parent_value : FSET_OPTION_VALUE_NULL,
                      (add_quotes_parent) ? weechat_color (weechat_config_string (fset_config_color_quotes[selected_line])) : "",
                      (add_quotes_parent) ? "\"" : "",
                      str_color_name);
        }
        else
        {
            snprintf (str_value, length_value,
                      "%s%s%s%s%s%s%s",
                      (add_quotes) ? str_color_quotes : "",
                      (add_quotes) ? "\"" : "",
                      str_color_value,
                      (fset_option->value) ? fset_option->value : FSET_OPTION_VALUE_NULL,
                      (add_quotes) ? str_color_quotes : "",
                      (add_quotes) ? "\"" : "",
                      str_color_name);
        }
    }

    weechat_printf_y (
        fset_buffer, fset_option->index,
        "%s%s%s %s%s  %s%s  %s",
        (str_color_line[0]) ? weechat_color (str_color_line) : "",
        (fset_option->marked) ?
        weechat_color (
            weechat_config_string (fset_config_color_marked[selected_line])) :
        weechat_color (
            weechat_config_string (fset_config_color_unmarked[selected_line])),
        str_marked,
        weechat_color (
            weechat_config_string (
                (value_changed) ?
                fset_config_color_name_changed[selected_line] :
                fset_config_color_name[selected_line])),
        str_name,
        weechat_color (
            weechat_config_string (fset_config_color_type[selected_line])),
        str_type,
        (str_value) ? str_value : "");

    if (str_value)
        free (str_value);

    return fset_option->index;
}

/*
 * Displays a line with an fset option.
 *
 * Returns the index of last line displayed in buffer (this depends on the
 * format number used), -1 if no option was displayed.
 */

int
fset_buffer_display_option (struct t_fset_option *fset_option)
{
    int format_number;
    const char *ptr_format;

    format_number = weechat_config_integer (fset_config_look_format_number);
    ptr_format = weechat_config_string (fset_config_format_option[format_number - 1]);

    if (ptr_format && ptr_format[0])
        return fset_buffer_display_option_eval (fset_option);
    else
        return fset_buffer_display_option_predefined_format (fset_option);
}

/*
 * Returns the last line index (y) for a buffer with free content,
 * -1 if buffer is empty.
 */

int
fset_buffer_get_last_y (struct t_gui_buffer *buffer)
{
    struct t_hdata *hdata_buffer, *hdata_lines, *hdata_line, *hdata_line_data;
    void *own_lines, *last_line, *line_data;

    hdata_buffer = weechat_hdata_get ("buffer");
    own_lines = weechat_hdata_pointer (hdata_buffer, buffer, "own_lines");
    if (!own_lines)
        return -1;

    hdata_lines = weechat_hdata_get ("lines");
    last_line = weechat_hdata_pointer (hdata_lines, own_lines, "last_line");
    if (!last_line)
        return -1;

    hdata_line = weechat_hdata_get ("line");
    line_data = weechat_hdata_pointer (hdata_line, last_line, "data");
    if (!line_data)
        return -1;

    hdata_line_data = weechat_hdata_get ("line_data");
    return weechat_hdata_integer (hdata_line_data, line_data, "y");
}

/*
 * Updates list of options in fset buffer.
 */

void
fset_buffer_refresh (int clear)
{
    int num_options, i, y, y_max_displayed, last_y;
    struct t_fset_option *ptr_fset_option;

    if (!fset_buffer)
        return;

    num_options = weechat_arraylist_size (fset_options);

    if (clear)
    {
        weechat_buffer_clear (fset_buffer);
        fset_buffer_selected_line = 0;
    }

    y_max_displayed = -1;

    for (i = 0; i < num_options; i++)
    {
        ptr_fset_option = weechat_arraylist_get (fset_options, i);
        if (ptr_fset_option)
        {
            y = fset_buffer_display_option (ptr_fset_option);
            if (y > y_max_displayed)
                y_max_displayed = y;
        }
    }

    /* remove lines displayed after the last one just displayed */
    last_y = fset_buffer_get_last_y (fset_buffer);
    for (y = last_y; y > y_max_displayed; y--)
    {
        weechat_printf_y (fset_buffer, y, "");
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
            (void) fset_buffer_display_option (
                weechat_arraylist_get (fset_options, old_line));
        }
        (void) fset_buffer_display_option (
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
    int start_line_y, chat_height, format_number, lines_per_option;
    int selected_y, selected_y2;
    char str_command[256];

    window = weechat_window_search_with_buffer (fset_buffer);
    if (!window)
        return;

    fset_buffer_get_window_info (window, &start_line_y, &chat_height);

    format_number = weechat_config_integer (fset_config_look_format_number);
    lines_per_option = fset_config_format_option_num_lines[format_number - 1];
    if (lines_per_option > chat_height)
        return;
    selected_y = fset_buffer_selected_line * lines_per_option;
    selected_y2 = selected_y + lines_per_option - 1;

    if ((start_line_y > selected_y)
        || (start_line_y < selected_y2 - chat_height + 1))
    {
        snprintf (str_command, sizeof (str_command),
                  "/window scroll -window %d %s%d",
                  weechat_window_get_integer (window, "number"),
                  (start_line_y > selected_y) ? "-" : "+",
                  (start_line_y > selected_y) ?
                  start_line_y - selected_y :
                  selected_y2 - start_line_y - chat_height + 1);
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
    int format_number, lines_per_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    /* scrolled another window/buffer? then just ignore */
    if (weechat_window_get_pointer (signal_data, "buffer") != fset_buffer)
        return WEECHAT_RC_OK;

    fset_buffer_get_window_info (signal_data, &start_line_y, &chat_height);

    format_number = weechat_config_integer (fset_config_look_format_number);
    lines_per_option = fset_config_format_option_num_lines[format_number - 1];
    line = fset_buffer_selected_line;
    while (line * lines_per_option < start_line_y)
    {
        line += chat_height / lines_per_option;
    }
    while (line * lines_per_option >= start_line_y + chat_height)
    {
        line -= chat_height / lines_per_option;
    }
    if (line * lines_per_option < start_line_y)
        line = (start_line_y / lines_per_option) + 1;

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
        { "<<", "/fset -go 0"                                   },
        { ">>", "/fset -go end"                                 },
        { "<",  "/fset -left"                                   },
        { ">",  "/fset -right"                                  },
        { "t",  "/fset -toggle"                                 },
        { "-",  "/fset -add -1"                                 },
        { "+",  "/fset -add 1"                                  },
        { "r",  "/fset -reset"                                  },
        { "u",  "/fset -unset"                                  },
        { "s",  "/fset -set"                                    },
        { "n",  "/fset -setnew"                                 },
        { "a",  "/fset -append"                                 },
        { ",",  "/fset -mark 1"                                 },
        { "p",  "/mute /set fset.look.show_plugins_desc toggle" },
        { "v",  "/bar toggle " FSET_BAR_NAME                    },
        { "x",  "/fset -format"                                 },
        { NULL, NULL                                            },
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
        { "up",            "/fset -up"                                     },
        { "down",          "/fset -down"                                   },
        { "meta-home",     "/fset -go 0"                                   },
        { "meta-end",      "/fset -go end"                                 },
        { "f11",           "/fset -left"                                   },
        { "f12",           "/fset -right"                                  },
        { "meta-space",    "/fset -toggle"                                 },
        { "meta--",        "/fset -add -1"                                 },
        { "meta-+",        "/fset -add 1"                                  },
        { "meta-f,meta-r", "/fset -reset"                                  },
        { "meta-f,meta-u", "/fset -unset"                                  },
        { "meta-return",   "/fset -set"                                    },
        { "meta-f,meta-n", "/fset -setnew"                                 },
        { "meta-f,meta-a", "/fset -append"                                 },
        { "meta-comma",    "/fset -mark"                                   },
        { "shift-up",      "/fset -up; /fset -mark"                        },
        { "shift-down",    "/fset -mark; /fset -down"                      },
        { "ctrl-l",        "/fset -refresh"                                },
        { "meta-p",        "/mute /set fset.look.show_plugins_desc toggle" },
        { "meta-v",        "/bar toggle " FSET_BAR_NAME                    },
        { "ctrl-x",        "/fset -format"                                 },
        { NULL, NULL },
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
    struct t_hashtable *buffer_props;

    if (fset_buffer)
        return;

    buffer_props = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_POINTER,
        NULL,
        NULL);
    if (buffer_props)
    {
        weechat_hashtable_set (buffer_props, "type", "free");
        weechat_hashtable_set (buffer_props, "localvar_set_type", "option");
    }

    fset_buffer = weechat_buffer_new_props (
        FSET_BUFFER_NAME,
        buffer_props,
        &fset_buffer_input_cb, NULL, NULL,
        &fset_buffer_close_cb, NULL, NULL);

    if (buffer_props)
        weechat_hashtable_free (buffer_props);

    if (!fset_buffer)
        return;

    fset_buffer_set_keys ();
    fset_buffer_set_localvar_filter ();

    fset_buffer_selected_line = 0;
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
