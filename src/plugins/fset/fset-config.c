/*
 * fset-config.c - Fast Set configuration options (file fset.conf)
 *
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-config.h"
#include "fset-bar-item.h"
#include "fset-buffer.h"
#include "fset-option.h"


struct t_config_file *fset_config_file = NULL;

/* fset config, look section */

struct t_config_option *fset_config_look_auto_refresh;
struct t_config_option *fset_config_look_auto_unmark;
struct t_config_option *fset_config_look_condition_catch_set;
struct t_config_option *fset_config_look_export_help_default;
struct t_config_option *fset_config_look_format_number;
struct t_config_option *fset_config_look_marked_string;
struct t_config_option *fset_config_look_scroll_horizontal;
struct t_config_option *fset_config_look_show_plugins_desc;
struct t_config_option *fset_config_look_sort;
struct t_config_option *fset_config_look_unmarked_string;
struct t_config_option *fset_config_look_use_color_value;
struct t_config_option *fset_config_look_use_keys;
struct t_config_option *fset_config_look_use_mute;

/* fset config, format section */

struct t_config_option *fset_config_format_export_help;
struct t_config_option *fset_config_format_export_option;
struct t_config_option *fset_config_format_export_option_null;
struct t_config_option *fset_config_format_option[2];

/* fset config, color section */

struct t_config_option *fset_config_color_default_value[2];
struct t_config_option *fset_config_color_description[2];
struct t_config_option *fset_config_color_file[2];
struct t_config_option *fset_config_color_file_changed[2];
struct t_config_option *fset_config_color_help_default_value;
struct t_config_option *fset_config_color_help_description;
struct t_config_option *fset_config_color_help_name;
struct t_config_option *fset_config_color_help_quotes;
struct t_config_option *fset_config_color_help_values;
struct t_config_option *fset_config_color_index[2];
struct t_config_option *fset_config_color_line_marked_bg[2];
struct t_config_option *fset_config_color_line_selected_bg[2];
struct t_config_option *fset_config_color_marked[2];
struct t_config_option *fset_config_color_max[2];
struct t_config_option *fset_config_color_min[2];
struct t_config_option *fset_config_color_name[2];
struct t_config_option *fset_config_color_name_changed[2];
struct t_config_option *fset_config_color_option[2];
struct t_config_option *fset_config_color_option_changed[2];
struct t_config_option *fset_config_color_parent_name[2];
struct t_config_option *fset_config_color_parent_value[2];
struct t_config_option *fset_config_color_quotes[2];
struct t_config_option *fset_config_color_quotes_changed[2];
struct t_config_option *fset_config_color_section[2];
struct t_config_option *fset_config_color_section_changed[2];
struct t_config_option *fset_config_color_string_values[2];
struct t_config_option *fset_config_color_title_count_options;
struct t_config_option *fset_config_color_title_current_option;
struct t_config_option *fset_config_color_title_filter;
struct t_config_option *fset_config_color_title_marked_options;
struct t_config_option *fset_config_color_title_sort;
struct t_config_option *fset_config_color_type[2];
struct t_config_option *fset_config_color_unmarked[2];
struct t_config_option *fset_config_color_value[2];
struct t_config_option *fset_config_color_value_changed[2];
struct t_config_option *fset_config_color_value_undef[2];

char **fset_config_auto_refresh = NULL;
char **fset_config_sort_fields = NULL;
int fset_config_sort_fields_count = 0;
int fset_config_format_option_num_lines[2] = { 1, 1 };


/*
 * Callback for changes on option "fset.look.auto_refresh".
 */

void
fset_config_change_auto_refresh_cb (const void *pointer, void *data,
                                    struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (fset_config_auto_refresh)
        weechat_string_free_split (fset_config_auto_refresh);

    fset_config_auto_refresh = weechat_string_split (
        weechat_config_string (fset_config_look_auto_refresh),
        ",",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        NULL);
}

/*
 * Callback for changes on option "fset.look.format_number".
 */

void
fset_config_change_format_number_cb (const void *pointer, void *data,
                                     struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    fset_buffer_refresh (1);
    fset_buffer_check_line_outside_window ();
}

/*
 * Callback for changes on option "fset.look.show_plugins_desc".
 */

void
fset_config_change_show_plugins_desc_cb (const void *pointer, void *data,
                                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (fset_buffer)
    {
        fset_option_get_options ();
        fset_buffer_refresh (1);
    }
}

/*
 * Callback for changes on option "fset.look.sort".
 */

void
fset_config_change_sort_cb (const void *pointer, void *data,
                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (fset_config_sort_fields)
        weechat_string_free_split (fset_config_sort_fields);

    fset_config_sort_fields = weechat_string_split (
        weechat_config_string (fset_config_look_sort),
        ",",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        &fset_config_sort_fields_count);

    if (fset_buffer)
    {
        fset_option_get_options ();
        fset_buffer_refresh (0);
    }
}

/*
 * Callback for changes on option "fset.look.use_color_value".
 */

void
fset_config_change_use_color_value_cb (const void *pointer, void *data,
                                       struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    fset_buffer_refresh (0);
}

/*
 * Callback for changes on option "fset.look.use_keys".
 */

void
fset_config_change_use_keys_cb (const void *pointer, void *data,
                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (fset_buffer)
        fset_buffer_set_keys ();
}

/*
 * Counts the number of "substring" in "string".
 *
 * Returns the number of times substring is in string.
 */

int
fset_config_count_substring (const char *string,
                             const char *substring)
{
    int count, length;
    const char *pos;

    count = 0;
    length = strlen (substring);
    pos = string;
    while (pos && pos[0])
    {
        pos = strstr (pos, substring);
        if (!pos)
            break;
        count++;
        pos += length;
    }

    return count;
}

/*
 * Callback for changes on format options.
 */

void
fset_config_change_format_cb (const void *pointer, void *data,
                              struct t_config_option *option)
{
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    for (i = 0; i < 2; i++)
    {
        fset_config_format_option_num_lines[i] = fset_config_count_substring (
            weechat_config_string (fset_config_format_option[i]), "${newline}") + 1;
    }

    fset_buffer_refresh (1);
    fset_buffer_check_line_outside_window ();
}

/*
 * Callback for changes on help color options.
 */

void
fset_config_change_help_color_cb (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    fset_bar_item_update ();
}

/*
 * Callback for changes on color options.
 */

void
fset_config_change_color_cb (const void *pointer, void *data,
                             struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    fset_buffer_refresh (0);
}

/*
 * Callback for changes on title color options.
 */

void
fset_config_change_title_color_cb (const void *pointer, void *data,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    fset_buffer_set_title ();
}

/*
 * Initializes fset configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
fset_config_init ()
{
    struct t_config_section *ptr_section;

    fset_config_file = weechat_config_new (FSET_CONFIG_NAME,
                                           NULL, NULL, NULL);
    if (!fset_config_file)
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (fset_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (fset_config_file);
        fset_config_file = NULL;
        return 0;
    }

    fset_config_look_auto_refresh = weechat_config_new_option (
        fset_config_file, ptr_section,
        "auto_refresh", "string",
        N_("comma separated list of options to automatically refresh on the "
           "fset buffer (if opened); \"*\" means all options (recommended), "
           "a name beginning with \"!\" is a negative value to prevent an "
           "option to be refreshed, wildcard \"*\" is allowed in names "
           "(example: \"*,!plugin.section.*\")"),
        NULL, 0, 0, "*", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_auto_refresh_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_auto_unmark = weechat_config_new_option (
        fset_config_file, ptr_section,
        "auto_unmark", "boolean",
        N_("automatically unmark all options after an action on marked "
           "options or after a refresh"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_condition_catch_set = weechat_config_new_option (
        fset_config_file, ptr_section,
        "condition_catch_set", "string",
        N_("condition to catch /set command and display results in the fset "
           "buffer; following variables can be used: ${name} (name of option "
           "given for the /set command), ${count} (number of options found "
           "with the /set argument); an empty string disables the catch of "
           "/set command; with value \"1\", the fset buffer is always used "
           "with /set command"),
        NULL, 0, 0, "${count} >= 1", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_export_help_default = weechat_config_new_option (
        fset_config_file, ptr_section,
        "export_help_default", "boolean",
        N_("write help for each option exported by default (this can be "
           "overridden with arguments \"-help\" and \"-nohelp\" for command "
           "/fset -export)"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_format_number = weechat_config_new_option (
        fset_config_file, ptr_section,
        "format_number", "integer",
        N_("number of format used to display options; this is dynamically "
           "changed by the key ctrl-X on the fset buffer"),
        NULL, 1, 2, "1", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_format_number_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_marked_string = weechat_config_new_option (
        fset_config_file, ptr_section,
        "marked_string", "string",
        N_("string displayed when an option is marked (to do an action on "
           "multiple options)"),
        NULL, 0, 0, "*", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_scroll_horizontal = weechat_config_new_option (
        fset_config_file, ptr_section,
        "scroll_horizontal", "integer",
        N_("left/right scroll in fset buffer (percent of width)"),
        NULL, 1, 100, "10", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_show_plugins_desc = weechat_config_new_option (
        fset_config_file, ptr_section,
        "show_plugins_desc", "boolean",
        N_("show the plugin description options (plugins.desc.*)"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_show_plugins_desc_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_sort = weechat_config_new_option (
        fset_config_file, ptr_section,
        "sort", "string",
        N_("comma-separated list of fields to sort options (see /help fset "
           "for a list of fields); char \"-\" can be used before field to "
           "reverse order, char \"~\" can be used to do a case insensitive "
           "comparison; example: \"-~name\" for case insensitive and reverse "
           "sort on option name"),
        NULL, 0, 0, "~name", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_sort_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_unmarked_string = weechat_config_new_option (
        fset_config_file, ptr_section,
        "unmarked_string", "string",
        N_("string displayed when an option is not marked"),
        NULL, 0, 0, " ", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_use_color_value = weechat_config_new_option (
        fset_config_file, ptr_section,
        "use_color_value", "boolean",
        N_("use the color to display value of color options"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_use_color_value_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_use_keys = weechat_config_new_option (
        fset_config_file, ptr_section,
        "use_keys", "boolean",
        N_("use keys alt+X in fset buffer to do actions on options; "
           "if disabled, only the input is allowed"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_use_keys_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_use_mute = weechat_config_new_option (
        fset_config_file, ptr_section,
        "use_mute", "boolean",
        N_("use /mute command to set options"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_use_keys_cb, NULL, NULL,
        NULL, NULL, NULL);

    /* format */
    ptr_section = weechat_config_new_section (fset_config_file, "format",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (fset_config_file);
        fset_config_file = NULL;
        return 0;
    }

    fset_config_format_export_help = weechat_config_new_option (
        fset_config_file, ptr_section,
        "export_help", "string",
        N_("format of help line written before each option exported in a file "
           "(note: content is evaluated, see /help fset)"),
        NULL, 0, 0,
        "# ${description2}",
        NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_format_export_option = weechat_config_new_option (
        fset_config_file, ptr_section,
        "export_option", "string",
        N_("format of each option exported in a file "
           "(note: content is evaluated, see /help fset)"),
        NULL, 0, 0,
        "/set ${name} ${quoted_value}",
        NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_format_export_option_null = weechat_config_new_option (
        fset_config_file, ptr_section,
        "export_option_null", "string",
        N_("format of each option with \"null\" value exported in a file "
           "(note: content is evaluated, see /help fset)"),
        NULL, 0, 0,
        "/unset ${name}",
        NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_format_option[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option1", "string",
        N_("first format of each line, used when option "
           "fset.look.format_number is set to 1 "
           "(note: content is evaluated, see /help fset); "
           "an empty string uses the default format "
           "(\"${marked} ${name}  ${type}  ${value2}\"), which is without "
           "evaluation of string and then much faster; "
           "formats can be switched with key ctrl+X"),
        NULL, 0, 0,
        "",
        NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_format_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_format_option[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option2", "string",
        N_("second format of each line, used when option "
           "fset.look.format_number is set to 2 "
           "(note: content is evaluated, see /help fset); "
           "an empty string uses the default format "
           "(\"${marked} ${name}  ${type}  ${value2}\"), which is without "
           "evaluation of string and then much faster; "
           "formats can be switched with key ctrl+X"),
        NULL, 0, 0,
        "${marked} ${name}  ${type}  ${value2}${newline}"
        "  ${empty_name}  ${_default_value}${color:darkgray} -- "
        "${min}..${max}${newline}"
        "  ${empty_name}  ${description}",
        NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_format_cb, NULL, NULL,
        NULL, NULL, NULL);

    /* color */
    ptr_section = weechat_config_new_section (fset_config_file, "color",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (fset_config_file);
        fset_config_file = NULL;
        return 0;
    }

    fset_config_color_default_value[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "default_value", "color",
        N_("color for default value"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_default_value[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "default_value_selected", "color",
        N_("color for default value on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_description[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "description", "color",
        N_("color for description"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_description[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "description_selected", "color",
        N_("color for description on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_file[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "file", "color",
        N_("color for file"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_file_changed[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "file_changed", "color",
        N_("color for file if value is changed"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_file_changed[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "file_changed_selected", "color",
        N_("color for file if value is changed on the selected line"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_file[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "file_selected", "color",
        N_("color for file on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_help_default_value = weechat_config_new_option (
        fset_config_file, ptr_section,
        "help_default_value", "color",
        N_("color for default value in help bar"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_help_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_help_description = weechat_config_new_option (
        fset_config_file, ptr_section,
        "help_description", "color",
        N_("color for description in help bar"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_help_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_help_name = weechat_config_new_option (
        fset_config_file, ptr_section,
        "help_name", "color",
        N_("color for name in help bar"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_help_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_help_quotes = weechat_config_new_option (
        fset_config_file, ptr_section,
        "help_quotes", "color",
        N_("color for quotes around string values"),
        NULL, 0, 0, "darkgray", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_help_values = weechat_config_new_option (
        fset_config_file, ptr_section,
        "help_values", "color",
        N_("color for allowed values"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_index[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "index", "color",
        N_("color for index of option"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_index[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "index_selected", "color",
        N_("color for index of option on the selected line"),
        NULL, 0, 0, "lightcyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_line_marked_bg[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "line_marked_bg1", "color",
        N_("background color for a marked line "
           "(used with the first format, see option fset.format.option1)"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_line_marked_bg[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "line_marked_bg2", "color",
        N_("background color for a marked line "
           "(used with the second format, see option fset.format.option2)"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_line_selected_bg[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "line_selected_bg1", "color",
        N_("background color for the selected line "
           "(used with the first format, see option fset.format.option1)"),
        NULL, 0, 0, "blue", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_line_selected_bg[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "line_selected_bg2", "color",
        N_("background color for the selected line "
           "(used with the second format, see option fset.format.option2)"),
        NULL, 0, 0, "red", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_marked[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "marked", "color",
        N_("color for mark indicator"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_marked[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "marked_selected", "color",
        N_("color for mark indicator on the selected line"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_max[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "max", "color",
        N_("color for max value"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_max[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "max_selected", "color",
        N_("color for max value on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_min[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "min", "color",
        N_("color for min value"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_min[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "min_selected", "color",
        N_("color for min value on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_name[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "name", "color",
        N_("color for name"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_name_changed[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "name_changed", "color",
        N_("color for name if value is changed"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_name_changed[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "name_changed_selected", "color",
        N_("color for name if value is changed on the selected line"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_name[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "name_selected", "color",
        N_("color for name on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_option[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option", "color",
        N_("color for option"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_option_changed[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option_changed", "color",
        N_("color for option if value is changed"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_option_changed[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option_changed_selected", "color",
        N_("color for option if value is changed on the selected line"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_option[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option_selected", "color",
        N_("color for option on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_parent_name[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_name", "color",
        N_("color for name of parent option"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_parent_name[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_name_selected", "color",
        N_("color for name of parent option on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_parent_value[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_value", "color",
        N_("color for value of parent option"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_parent_value[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_value_selected", "color",
        N_("color for value of parent option on the selected line"),
        NULL, 0, 0, "lightcyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_quotes[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "quotes", "color",
        N_("color for quotes around string values"),
        NULL, 0, 0, "darkgray", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_quotes_changed[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "quotes_changed", "color",
        N_("color for quotes around string values which are changed"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_quotes_changed[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "quotes_changed_selected", "color",
        N_("color for quotes around string values which are changed "
           "on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_quotes[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "quotes_selected", "color",
        N_("color for quotes around string values on the selected line"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_section[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "section", "color",
        N_("color for section"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_section_changed[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "section_changed", "color",
        N_("color for section if value is changed"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_section_changed[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "section_changed_selected", "color",
        N_("color for section if value is changed on the selected line"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_section[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "section_selected", "color",
        N_("color for section on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_string_values[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "string_values", "color",
        N_("color for string values"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_string_values[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "string_values_selected", "color",
        N_("color for string values on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_title_count_options = weechat_config_new_option (
        fset_config_file, ptr_section,
        "title_count_options", "color",
        N_("color for the count of options found with the current filter "
           "in title of buffer"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_title_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_title_current_option = weechat_config_new_option (
        fset_config_file, ptr_section,
        "title_current_option", "color",
        N_("color for current option number in title of buffer"),
        NULL, 0, 0, "lightcyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_title_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_title_filter = weechat_config_new_option (
        fset_config_file, ptr_section,
        "title_filter", "color",
        N_("color for filter in title of buffer"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_title_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_title_marked_options = weechat_config_new_option (
        fset_config_file, ptr_section,
        "title_marked_options", "color",
        N_("color for number of marked options in title of buffer"),
        NULL, 0, 0, "lightgreen", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_title_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_title_sort = weechat_config_new_option (
        fset_config_file, ptr_section,
        "title_sort", "color",
        N_("color for sort in title of buffer"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_title_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_type[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "type", "color",
        N_("color for type"),
        NULL, 0, 0, "green", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_type[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "type_selected", "color",
        N_("color for type on the selected line"),
        NULL, 0, 0, "lightgreen", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_unmarked[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "unmarked", "color",
        N_("color for mark indicator when the option is not marked"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_unmarked[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "unmarked_selected", "color",
        N_("color for mark indicator when the option is not marked "
           "on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value", "color",
        N_("color for value"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value_changed[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_changed", "color",
        N_("color for value changed (different from default)"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value_changed[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_changed_selected", "color",
        N_("color for value changed (different from default) on the selected "
           "line"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_selected", "color",
        N_("color for value on the selected line"),
        NULL, 0, 0, "lightcyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value_undef[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_undef", "color",
        N_("color for undefined value"),
        NULL, 0, 0, "magenta", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value_undef[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_undef_selected", "color",
        N_("color for undefined value on the selected line"),
        NULL, 0, 0, "lightmagenta", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);

    return 1;
}

/*
 * Reads fset configuration file.
 */

int
fset_config_read ()
{
    int rc;

    rc = weechat_config_read (fset_config_file);

    if (rc == WEECHAT_CONFIG_READ_OK)
    {
        fset_config_change_auto_refresh_cb (NULL, NULL, NULL);
        fset_config_change_sort_cb (NULL, NULL, NULL);
        fset_config_change_format_cb (NULL, NULL, NULL);
    }

    return rc;
}

/*
 * Writes fset configuration file.
 */

int
fset_config_write ()
{
    return weechat_config_write (fset_config_file);
}

/*
 * Frees fset configuration.
 */

void
fset_config_free ()
{
    weechat_config_free (fset_config_file);

    if (fset_config_auto_refresh)
    {
        weechat_string_free_split (fset_config_auto_refresh);
        fset_config_auto_refresh = NULL;
    }
    if (fset_config_sort_fields)
    {
        weechat_string_free_split (fset_config_sort_fields);
        fset_config_sort_fields = NULL;
        fset_config_sort_fields_count = 0;
    }
}
