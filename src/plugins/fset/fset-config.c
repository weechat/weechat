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
#include "fset-buffer.h"


struct t_config_file *fset_config_file = NULL;

/* fset config, look section */

struct t_config_option *fset_config_look_enabled;
struct t_config_option *fset_config_look_use_keys;
struct t_config_option *fset_config_look_use_mute;

/* fset config, format section */

struct t_config_option *fset_config_format_option;
struct t_config_option *fset_config_format_option_current;

/* fset config, color section */

struct t_config_option *fset_config_color_default_value[2];
struct t_config_option *fset_config_color_description[2];
struct t_config_option *fset_config_color_max[2];
struct t_config_option *fset_config_color_min[2];
struct t_config_option *fset_config_color_name[2];
struct t_config_option *fset_config_color_parent_name[2];
struct t_config_option *fset_config_color_quotes[2];
struct t_config_option *fset_config_color_type[2];
struct t_config_option *fset_config_color_value[2];
struct t_config_option *fset_config_color_value_diff[2];
struct t_config_option *fset_config_color_value_undef[2];

char *fset_config_eval_format_option_current = NULL;


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
fset_config_change_format (const void *pointer, void *data,
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
 * Callback for changes on color options.
 */

void
fset_config_change_color (const void *pointer, void *data,
                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    fset_buffer_refresh (0);
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

    fset_config_look_enabled = weechat_config_new_option (
        fset_config_file, ptr_section,
        "enabled", "boolean",
        N_("enable fset"),
        NULL, 0, 0, "on", NULL, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
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
        " ${name} ${type} ${value2}",
        NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_format, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_format_option_current = weechat_config_new_option (
        fset_config_file, ptr_section,
        "option_current", "string",
        N_("format for the line with current option "
           "(note: content is evaluated, see /help fset)"),
        NULL, 0, 0, "${color:,blue}${format_option}", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_format, NULL, NULL,
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
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_default_value[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "default_value_selected", "color",
        N_("color for default value on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_description[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "description", "color",
        N_("color for description"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_description[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "description_selected", "color",
        N_("color for description on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_max[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "max", "color",
        N_("color for max value"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_max[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "max_selected", "color",
        N_("color for max value on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_min[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "min", "color",
        N_("color for min value"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_min[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "min_selected", "color",
        N_("color for min value on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_name[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "name", "color",
        N_("color for name"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_name[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "name_selected", "color",
        N_("color for name on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_parent_name[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_name", "color",
        N_("color for parent name"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_parent_name[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "parent_name_selected", "color",
        N_("color for parent name on the selected line"),
        NULL, 0, 0, "white", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_quotes[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "quote", "color",
        N_("color for quotes around string values"),
        NULL, 0, 0, "darkgray", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_quotes[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "quotes_selected", "color",
        N_("color for quotes around string values on the selected line"),
        NULL, 0, 0, "default", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_type[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "type", "color",
        N_("color for type"),
        NULL, 0, 0, "brown", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_type[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "type_selected", "color",
        N_("color for type on the selected line"),
        NULL, 0, 0, "yellow", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value", "color",
        N_("color for value"),
        NULL, 0, 0, "cyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_selected", "color",
        N_("color for value on the selected line"),
        NULL, 0, 0, "lightcyan", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value_diff[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_diff", "color",
        N_("color for value different from default"),
        NULL, 0, 0, "green", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value_diff[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_diff_selected", "color",
        N_("color for value different from default on the selected line"),
        NULL, 0, 0, "lightgreen", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value_undef[0] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_undef", "color",
        N_("color for undefined value"),
        NULL, 0, 0, "magenta", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
        NULL, NULL, NULL);
    fset_config_color_value_undef[1] = weechat_config_new_option (
        fset_config_file, ptr_section,
        "value_undef_selected", "color",
        N_("color for undefined value on the selected line"),
        NULL, 0, 0, "lightmagenta", NULL, 0,
        NULL, NULL, NULL,
        &fset_config_change_color, NULL, NULL,
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
        fset_config_change_format (NULL, NULL, NULL);
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

    if (fset_config_eval_format_option_current)
    {
        free (fset_config_eval_format_option_current);
        fset_config_eval_format_option_current = NULL;
    }
}
