/*
 * fset-config.c - Fast Set configuration options (file fset.conf)
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
#include <string.h>

#include "../weechat-plugin.h"
#include "fset.h"
#include "fset-config.h"
#include "fset-bar-item.h"
#include "fset-buffer.h"
#include "fset-option.h"


struct t_config_file *fset_config_file = NULL;

/* fset config, look section */

struct t_config_option *fset_config_look_auto_unmark;
struct t_config_option *fset_config_look_condition_catch_set;
struct t_config_option *fset_config_look_marked_string;
struct t_config_option *fset_config_look_show_help_bar;
struct t_config_option *fset_config_look_show_plugin_description;
struct t_config_option *fset_config_look_sort;
struct t_config_option *fset_config_look_unmarked_string;
struct t_config_option *fset_config_look_use_color_value;
struct t_config_option *fset_config_look_use_keys;
struct t_config_option *fset_config_look_use_mute;

/* fset config, format section */

struct t_config_option *fset_config_format_option;
struct t_config_option *fset_config_format_option_current;

/* fset config, color section */

struct t_config_option *fset_config_color_default_value[2];
struct t_config_option *fset_config_color_description[2];
struct t_config_option *fset_config_color_file[2];
struct t_config_option *fset_config_color_file_changed[2];
struct t_config_option *fset_config_color_help_default_value;
struct t_config_option *fset_config_color_help_description;
struct t_config_option *fset_config_color_help_name;
struct t_config_option *fset_config_color_help_quotes;
struct t_config_option *fset_config_color_help_string_values;
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

char **fset_config_sort_fields = NULL;
int fset_config_sort_fields_count = 0;
char *fset_config_eval_format_option_current = NULL;


/*
 * Callback for changes on option "fset.look.show_help_bar".
 */

void
fset_config_change_show_help_bar_cb (const void *pointer, void *data,
                                     struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    weechat_command (NULL, "/window refresh");
}

/*
 * Callback for changes on option "fset.look.show_plugin_description".
 */

void
fset_config_change_show_plugin_description_cb (const void *pointer, void *data,
                                               struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (fset_buffer)
    {
        fset_buffer_selected_line = 0;
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
        ",", 0, 0, &fset_config_sort_fields_count);

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
 * Callback for changes on format options.
 */

void
fset_config_change_format_cb (const void *pointer, void *data,
                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (fset_config_eval_format_option_current)
        free (fset_config_eval_format_option_current);

    fset_config_eval_format_option_current = weechat_string_replace (
        weechat_config_string (fset_config_format_option_current),
        "${format_option}",
        weechat_config_string (fset_config_format_option));

    fset_buffer_refresh (0);
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
        return 0;
    }

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
    fset_config_look_marked_string = weechat_config_new_option (
        fset_config_file, ptr_section,
        "marked_string", "string",
        N_("string displayed when an option is marked (to do an action on "
           "multiple options)"),
        NULL, 0, 0, "*", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_show_help_bar = weechat_config_new_option (
        fset_config_file, ptr_section,
        "show_help_bar", "boolean",
        N_("display help bar in fset buffer (description of option, "
           "allowed values and default value)"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_show_help_bar_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_look_show_plugin_description = weechat_config_new_option (
        fset_config_file, ptr_section,
        "show_plugin_description", "boolean",
        N_("show the plugin description options (plugins.desc.*)"),
        NULL, 0, 0, "off", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_show_plugin_description_cb, NULL, NULL,
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
        return 0;
    }

    fset_config_format_option = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option", "string",
        N_("format of each line with an option "
           "(note: content is evaluated, see /help fset)"),
        NULL, 0, 0,
        "${marked} ${name}  ${type}  ${value2}",
        NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_format_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_format_option_current = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option_current", "string",
        N_("format for the line with current option "
           "(note: content is evaluated, see /help fset)"),
        NULL, 0, 0, "${color:,blue}${format_option}", NULL, 0,
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
    fset_config_color_file[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "file_selected", "color",
        N_("color for file on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
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
    fset_config_color_help_string_values = weechat_config_new_option (
        fset_config_file, ptr_section,
        "help_string_values", "color",
        N_("color for string values"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_marked[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "marked", "color",
        N_("color for marked string"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_marked[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "marked_selected", "color",
        N_("color for marked string on the selected line"),
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
    fset_config_color_name[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "name_selected", "color",
        N_("color for name on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
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
    fset_config_color_option[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option", "color",
        N_("color for option"),
        NULL, 0, 0, "default", NULL, 0,
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
   fset_config_color_parent_name[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_name", "color",
        N_("color for parent name"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_parent_name[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_name_selected", "color",
        N_("color for parent name on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_parent_value[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_value", "color",
        N_("color for parent value"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_parent_value[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_value_selected", "color",
        N_("color for parent value on the selected line"),
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
    fset_config_color_quotes[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "quotes_selected", "color",
        N_("color for quotes around string values on the selected line"),
        NULL, 0, 0, "default", NULL, 0,
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
    fset_config_color_section[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "section", "color",
        N_("color for section"),
        NULL, 0, 0, "default", NULL, 0,
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
        N_("color for unmarked string"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color_cb, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_unmarked[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "unmarked_selected", "color",
        N_("color for unmarked string on the selected line"),
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
    fset_config_color_value[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_selected", "color",
        N_("color for value on the selected line"),
        NULL, 0, 0, "lightcyan", NULL, 0,
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

    if (fset_config_sort_fields)
    {
        weechat_string_free_split (fset_config_sort_fields);
        fset_config_sort_fields = NULL;
        fset_config_sort_fields_count = 0;
    }

    if (fset_config_eval_format_option_current)
    {
        free (fset_config_eval_format_option_current);
        fset_config_eval_format_option_current = NULL;
    }
}
