/*
 * fset-command.c - Fast Set command
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
#include "fset-command.h"
#include "fset-bar-item.h"
#include "fset-buffer.h"
#include "fset-config.h"
#include "fset-option.h"


/*
 * Gets the currently selected fset_option pointer and the associated
 * config_option pointer.
 */

void
fset_command_get_option (struct t_fset_option **fset_option,
                         struct t_config_option **config_option)
{

    *config_option = NULL;

    *fset_option = weechat_arraylist_get (fset_options,
                                          fset_buffer_selected_line);
    if (*fset_option)
        *config_option = weechat_config_get ((*fset_option)->name);
}

/*
 * Gets an integer argument given to the /fset command.
 */

int
fset_command_get_int_arg (int argc, char **argv, int arg_number,
                          int default_value)
{
    long value;
    char *error;

    value = default_value;
    if (argc > arg_number)
    {
        error = NULL;
        value = strtol (argv[arg_number], &error, 10);
        if (!error || error[0])
            value = default_value;
    }
    return (int)value;
}

/*
 * Callback for command "/fset".
 */

int
fset_command_fset (const void *pointer, void *data,
                   struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    int num_options, line, value, i, with_help, min, max, format_number, count;
    char str_command[512], str_number[64];
    const char *ptr_filename;
    struct t_fset_option *ptr_fset_option;
    struct t_config_option *ptr_option;
    struct t_gui_window *ptr_window;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    if (argc == 1)
    {
        if (weechat_arraylist_size (fset_options) == 0)
        {
            fset_option_get_options ();
        }
        if (!fset_buffer)
        {
            fset_buffer_open ();
            fset_buffer_refresh (1);
        }
        weechat_buffer_set (fset_buffer, "display", "1");
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-bar") == 0)
    {
        fset_add_bar ();
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-refresh") == 0)
    {
        fset_option_get_options ();
        fset_buffer_refresh (0);
        weechat_command (NULL, "/window refresh");
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-up") == 0)
    {
        if (fset_buffer)
        {
            value = fset_command_get_int_arg (argc, argv, 2, 1);
            num_options = weechat_arraylist_size (fset_options);
            if (num_options > 0)
            {
                line = fset_buffer_selected_line - value;
                if (line < 0)
                    line = 0;
                if (line != fset_buffer_selected_line)
                {
                    fset_buffer_set_current_line (line);
                    fset_buffer_check_line_outside_window ();
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-down") == 0)
    {
        if (fset_buffer)
        {
            value = fset_command_get_int_arg (argc, argv, 2, 1);
            num_options = weechat_arraylist_size (fset_options);
            if (num_options > 0)
            {
                line = fset_buffer_selected_line + value;
                if (line >= num_options)
                    line = num_options - 1;
                if (line != fset_buffer_selected_line)
                {
                    fset_buffer_set_current_line (line);
                    fset_buffer_check_line_outside_window ();
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-left") == 0)
    {
        if (fset_buffer)
        {
            ptr_window = weechat_window_search_with_buffer (fset_buffer);
            if (ptr_window)
            {
                value = fset_command_get_int_arg (
                    argc, argv, 2,
                    weechat_config_integer (fset_config_look_scroll_horizontal));
                if (value < 1)
                    value = 1;
                else if (value > 100)
                    value = 100;
                snprintf (str_command, sizeof (str_command),
                          "/window scroll_horiz -window %d -%d%%",
                          weechat_window_get_integer (ptr_window, "number"),
                          value);
                weechat_command (fset_buffer, str_command);
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-right") == 0)
    {
        if (fset_buffer)
        {
            ptr_window = weechat_window_search_with_buffer (fset_buffer);
            if (ptr_window)
            {
                value = fset_command_get_int_arg (
                    argc, argv, 2,
                    weechat_config_integer (fset_config_look_scroll_horizontal));
                if (value < 1)
                    value = 1;
                else if (value > 100)
                    value = 100;
                snprintf (str_command, sizeof (str_command),
                          "/window scroll_horiz -window %d %d%%",
                          weechat_window_get_integer (ptr_window, "number"),
                          value);
                weechat_command (fset_buffer, str_command);
            }
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "-go") == 0)
    {
        if (fset_buffer)
        {
            if (argc < 3)
                WEECHAT_COMMAND_ERROR;
            if (weechat_strcmp (argv[2], "end") == 0)
                line = weechat_arraylist_size (fset_options) - 1;
            else
                line = fset_command_get_int_arg (argc, argv, 2, -1);
            if (line >= 0)
            {
                fset_buffer_set_current_line (line);
                fset_buffer_check_line_outside_window ();
            }
        }
        return WEECHAT_RC_OK;
    }

    if (argv[1][0] == '-')
    {
        if (weechat_strcmp (argv[1], "-toggle") == 0)
        {
            if (fset_option_count_marked > 0)
            {
                num_options = weechat_arraylist_size (fset_options);
                for (i = 0; i < num_options; i++)
                {
                    ptr_fset_option = weechat_arraylist_get (fset_options, i);
                    if (ptr_fset_option && ptr_fset_option->marked)
                    {
                        ptr_option = weechat_config_get (ptr_fset_option->name);
                        if (ptr_option)
                            fset_option_toggle_value (ptr_fset_option, ptr_option);
                    }
                }
            }
            else
            {
                fset_command_get_option (&ptr_fset_option, &ptr_option);
                fset_option_toggle_value (ptr_fset_option, ptr_option);
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-add") == 0)
        {
            value = fset_command_get_int_arg (argc, argv, 2, 0);
            if (value == 0)
                WEECHAT_COMMAND_ERROR;

            if (fset_option_count_marked > 0)
            {
                num_options = weechat_arraylist_size (fset_options);
                for (i = 0; i < num_options; i++)
                {
                    ptr_fset_option = weechat_arraylist_get (fset_options, i);
                    if (ptr_fset_option && ptr_fset_option->marked)
                    {
                        ptr_option = weechat_config_get (ptr_fset_option->name);
                        if (ptr_option)
                            fset_option_add_value (ptr_fset_option, ptr_option, value);
                    }
                }
            }
            else
            {
                fset_command_get_option (&ptr_fset_option, &ptr_option);
                if (ptr_fset_option &&
                    ((ptr_fset_option->type == FSET_OPTION_TYPE_INTEGER)
                     || (ptr_fset_option->type == FSET_OPTION_TYPE_COLOR)
                     || (ptr_fset_option->type == FSET_OPTION_TYPE_ENUM)))
                {
                    fset_option_add_value (ptr_fset_option, ptr_option, value);
                }
                else
                {
                    fset_option_set (ptr_fset_option, ptr_option, buffer,
                                     (value > 0) ? 1 : 0);
                }
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-reset") == 0)
        {
            if (fset_option_count_marked > 0)
            {
                num_options = weechat_arraylist_size (fset_options);
                for (i = 0; i < num_options; i++)
                {
                    ptr_fset_option = weechat_arraylist_get (fset_options, i);
                    if (ptr_fset_option && ptr_fset_option->marked)
                    {
                        ptr_option = weechat_config_get (ptr_fset_option->name);
                        if (ptr_option)
                            fset_option_reset_value (ptr_fset_option, ptr_option);
                    }
                }
            }
            else
            {
                fset_command_get_option (&ptr_fset_option, &ptr_option);
                fset_option_reset_value (ptr_fset_option, ptr_option);
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-unset") == 0)
        {
            if (fset_option_count_marked > 0)
            {
                num_options = weechat_arraylist_size (fset_options);
                for (i = 0; i < num_options; i++)
                {
                    ptr_fset_option = weechat_arraylist_get (fset_options, i);
                    if (ptr_fset_option && ptr_fset_option->marked)
                    {
                        ptr_option = weechat_config_get (ptr_fset_option->name);
                        if (ptr_option)
                            fset_option_unset_value (ptr_fset_option, ptr_option);
                    }
                }
            }
            else
            {
                fset_command_get_option (&ptr_fset_option, &ptr_option);
                fset_option_unset_value (ptr_fset_option, ptr_option);
            }
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-set") == 0)
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            fset_option_set (ptr_fset_option, ptr_option, buffer, 0);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-setnew") == 0)
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            fset_option_set (ptr_fset_option, ptr_option, buffer, -1);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-append") == 0)
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            fset_option_set (ptr_fset_option, ptr_option, buffer, 1);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-mark") == 0)
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            fset_option_toggle_mark (ptr_fset_option, ptr_option);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-format") == 0)
        {
            min = weechat_hdata_integer (fset_hdata_config_option,
                                         fset_config_look_format_number,
                                         "min");
            max = weechat_hdata_integer (fset_hdata_config_option,
                                         fset_config_look_format_number,
                                         "max");
            format_number = weechat_config_integer (fset_config_look_format_number) + 1;
            if (format_number > max)
                format_number = min;
            snprintf (str_number, sizeof (str_number), "%d", format_number);
            weechat_config_option_set (fset_config_look_format_number,
                                       str_number, 1);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-export") == 0)
        {
            if (argc < 3)
                WEECHAT_COMMAND_ERROR;
            with_help = weechat_config_boolean (fset_config_look_export_help_default);
            ptr_filename = argv_eol[2];
            if (weechat_strcmp (argv[2], "-help") == 0)
            {
                with_help = 1;
                if (argc < 4)
                    WEECHAT_COMMAND_ERROR;
                ptr_filename = argv_eol[3];
            }
            else if (weechat_strcmp (argv[2], "-nohelp") == 0)
            {
                with_help = 0;
                if (argc < 4)
                    WEECHAT_COMMAND_ERROR;
                ptr_filename = argv_eol[3];
            }
            num_options = weechat_arraylist_size (fset_options);
            if (num_options == 0)
            {
                weechat_printf (NULL,
                                _("%s%s: there are no options displayed, "
                                  "unable to export."),
                                weechat_prefix ("error"), FSET_PLUGIN_NAME);
                return WEECHAT_RC_ERROR;
            }
            if (!fset_option_export (ptr_filename, with_help))
                WEECHAT_COMMAND_ERROR;
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[1], "-import") == 0)
        {
            if (argc < 3)
                WEECHAT_COMMAND_ERROR;
            count = fset_option_import (argv_eol[2]);
            switch (count)
            {
                case -2:
                    weechat_printf (NULL,
                        _("%s%s: not enough memory"),
                        weechat_prefix ("error"), FSET_PLUGIN_NAME);
                    return WEECHAT_RC_ERROR;
                case -1:
                    weechat_printf (NULL,
                                    _("%s%s: file \"%s\" not found"),
                                    weechat_prefix ("error"), FSET_PLUGIN_NAME,
                                    argv_eol[2]);
                    return WEECHAT_RC_ERROR;
                default:
                    weechat_printf (NULL,
                                    NG_("%d command executed in file \"%s\"",
                                        "%d commands executed in file \"%s\"",
                                        count),
                                    count, argv_eol[2]);
                    break;
            }
            return WEECHAT_RC_OK;
        }

        WEECHAT_COMMAND_ERROR;
    }
    else
    {
        /* set new filter */
        if (!fset_buffer)
            fset_buffer_open ();
        weechat_buffer_set (fset_buffer, "display", "1");
        fset_option_filter_options (argv_eol[1]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks execution of command "/set".
 */

int
fset_command_run_set_cb (const void *pointer, void *data,
                         struct t_gui_buffer *buffer, const char *command)
{
    char **argv, *old_filter, *result, str_number[64];
    const char *ptr_condition;
    int rc, argc, old_count_marked, old_buffer_selected_line, condition_ok;
    struct t_arraylist *old_options;
    struct t_fset_option_max_length *old_max_length;
    struct t_hashtable *eval_extra_vars, *eval_options;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* ignore /set command if issued on fset buffer */
    if (fset_buffer && (buffer == fset_buffer))
        return WEECHAT_RC_OK;

    if (strncmp (command, "/set", 4) != 0)
        return WEECHAT_RC_OK;

    ptr_condition = weechat_config_string (fset_config_look_condition_catch_set);
    if (!ptr_condition || !ptr_condition[0])
        return WEECHAT_RC_OK;

    rc = WEECHAT_RC_OK;

    argv = weechat_string_split (command, " ", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);

    if (argc > 2)
        goto end;

    /*
     * ignore "diff" and "env" arguments for /set
     * (we must not catch that in fset!)
     */
    if ((argc > 1)
        && ((weechat_strcmp (argv[1], "diff") == 0)
            || (weechat_strcmp (argv[1], "env") == 0)))
    {
        goto end;
    }

    /* backup current options/max length field/selected line/filter */
    old_options = fset_options;
    fset_options = fset_option_get_arraylist_options ();
    old_count_marked = fset_option_count_marked;
    old_max_length = fset_option_max_length;
    fset_option_max_length = fset_option_get_max_length ();
    old_filter = (fset_option_filter) ? strdup (fset_option_filter) : NULL;
    fset_option_set_filter ((argc > 1) ? argv[1] : NULL);
    old_buffer_selected_line = fset_buffer_selected_line;
    fset_buffer_selected_line = 0;

    fset_option_get_options ();

    /* evaluate condition to catch /set command */
    condition_ok = 0;
    eval_extra_vars = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL,
        NULL);
    eval_options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (eval_extra_vars && eval_options)
    {
        snprintf (str_number, sizeof (str_number),
                  "%d", weechat_arraylist_size (fset_options));
        weechat_hashtable_set (eval_extra_vars, "count", str_number);
        weechat_hashtable_set (eval_extra_vars, "name",
                               (argc > 1) ? argv[1] : "");
        weechat_hashtable_set (eval_options, "type", "condition");
        result = weechat_string_eval_expression (ptr_condition,
                                                 NULL,
                                                 eval_extra_vars,
                                                 eval_options);
        condition_ok = (result && (strcmp (result, "1") == 0));
        free (result);
    }
    weechat_hashtable_free (eval_extra_vars);
    weechat_hashtable_free (eval_options);

    /* check condition to trigger the fset buffer */
    if (condition_ok)
    {
        weechat_arraylist_free (old_options);
        free (old_max_length);
        free (old_filter);

        if (!fset_buffer)
            fset_buffer_open ();

        fset_buffer_set_localvar_filter ();
        fset_buffer_refresh (1);
        weechat_buffer_set (fset_buffer, "display", "1");

        rc = WEECHAT_RC_OK_EAT;
    }
    else
    {
        weechat_arraylist_free (fset_options);
        fset_options = old_options;
        fset_option_count_marked = old_count_marked;
        free (fset_option_max_length);
        fset_option_max_length = old_max_length;
        fset_option_set_filter (old_filter);
        free (old_filter);
        fset_buffer_selected_line = old_buffer_selected_line;
    }

end:
    weechat_string_free_split (argv);
    return rc;
}

/*
 * Hooks execution of command "/key".
 */

int
fset_command_run_key_cb (const void *pointer, void *data,
                         struct t_gui_buffer *buffer, const char *command)
{
    const char *ptr_args;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    if (strncmp (command, "/key", 4) != 0)
        return WEECHAT_RC_OK;

    ptr_args = strchr (command,  ' ');
    while (ptr_args && (ptr_args[0] == ' '))
    {
        ptr_args++;
    }

    if (!ptr_args || !ptr_args[0])
    {
        fset_option_filter_options ("weechat.key*");
        if (!fset_buffer)
            fset_buffer_open ();
        fset_buffer_set_localvar_filter ();
        fset_buffer_refresh (1);
        weechat_buffer_set (fset_buffer, "display", "1");
        return WEECHAT_RC_OK_EAT;
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks fset commands.
 */

void
fset_command_init (void)
{
    weechat_hook_command (
        "fset",
        N_("fast set WeeChat and plugins options"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") may be translated */
        N_("-bar"
           " || -refresh"
           " || -up|-down [<number>]"
           " || -left|-right [<percent>]"
           " || -go <line>|end"
           " || -toggle"
           " || -add [<value>]"
           " || -reset"
           " || -unset"
           " || -set"
           " || -setnew"
           " || -append"
           " || -mark"
           " || -format"
           " || -export [-help|-nohelp] <filename>"
           " || -import <filename>"
           " || <filter>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[-bar]: add the help bar"),
            N_("raw[-refresh]: refresh list of options, then whole screen "
               "(command: /window refresh)"),
            N_("raw[-up]: move the selected line up by \"number\" lines"),
            N_("raw[-down]: move the selected line down by \"number\" lines"),
            N_("raw[-left]: scroll the buffer by \"percent\" of width on the left"),
            N_("raw[-right]: scroll the buffer by \"percent\" of width on the right"),
            N_("raw[-go]: select a line by number, first line number is 0 "
               "(\"end\" to select the last line)"),
            N_("raw[-toggle]: toggle the boolean value"),
            N_("raw[-add]: add \"value\" (which can be a negative number) "
               "for integers, colors and enums, set/append to value for other types "
               "(set for a negative value, append for a positive value)"),
            N_("raw[-reset]: reset the value of option"),
            N_("raw[-unset]: unset the option"),
            N_("raw[-set]: add the /set command in input to edit the value of "
               "option (move the cursor at the beginning of value)"),
            N_("raw[-setnew]: add the /set command in input to edit a new value "
               "for the option"),
            N_("raw[-append]: add the /set command to append something in the value "
               "of option (move the cursor at the end of value)"),
            N_("raw[-mark]: toggle mark"),
            N_("raw[-format]: switch to the next available format"),
            N_("raw[-export]: export the options and values displayed to a file "
               "(each line has format: \"/set name value\" or \"/unset name\")"),
            N_("raw[-import]: import the options from a file "
               "(all lines containing commands are executed)"),
            N_("raw[-help]: force writing of help on options in exported file "
               "(see /help fset.look.export_help_default)"),
            N_("raw[-nohelp]: do not write help on options in exported file "
               "(see /help fset.look.export_help_default)"),
            N_("filter: set a new filter to see only matching options (this "
               "filter can be used as input in fset buffer as well); allowed "
               "formats are:"),
            N_("> `*`: show all options (no filter)"),
            N_("> `xxx`: show only options with \"xxx\" in name"),
            N_("> `f:xxx`: show only configuration file \"xxx\""),
            N_("> `t:xxx`: show only type \"xxx\" (bool/int/str/col/enum "
               "or boolean/integer/string/color/enum)"),
            N_("> `d`: show only changed options"),
            N_("> `d:xxx`: show only changed options with \"xxx\" in "
               "name"),
            N_("> `d=xxx`: show only changed options with \"xxx\" in "
               "value"),
            N_("> `d==xxx`: show only changed options with exact value "
               "\"xxx\""),
            N_("> `h=xxx`: show only options with \"xxx\" in "
               "description (translated)"),
            N_("> `he=xxx`: show only options with \"xxx\" in "
               "description (in English)"),
            N_("> `=xxx`: show only options with \"xxx\" in value"),
            N_("> `==xxx`: show only options with exact value \"xxx\""),
            N_("> `c:xxx`: show only options matching the evaluated "
               "condition \"xxx\", using following variables: file, section, "
               "option, name, parent_name, type, type_en, type_short "
               "(bool/int/str/col/enum), type_tiny (b/i/s/c/e), default_value, "
               "default_value_undef, value, quoted_value, value_undef, "
               "value_changed, parent_value, min, max, description, description2, "
               "description_en, description_en2, string_values, allowed_values"),
            "",
            N_("The lines with options are displayed using string evaluation "
               "(see /help eval for the format), with these options:"),
            N_("  - fset.format.option1: first format for an option"),
            N_("  - fset.format.option2: second format for an option"),
            "",
            N_("The following variables can be used in these options:"),
            N_("  - option data, with color and padded by spaces on the right:"),
            N_("    - ${file}: configuration file (for example \"weechat\" or "
               "\"irc\")"),
            N_("    - ${section}: section"),
            N_("    - ${option}: option name"),
            N_("    - ${name}: full option name (file.section.option)"),
            N_("    - ${parent_name}: parent option name"),
            N_("    - ${type}: option type (translated)"),
            N_("    - ${type_en}: option type (in English)"),
            N_("    - ${type_short}: short option type (bool/int/str/col/enum)"),
            N_("    - ${type_tiny}: tiny option type (b/i/s/c/e)"),
            N_("    - ${default_value}: option default value"),
            N_("    - ${default_value_undef}: \"1\" if default value is null, "
               "otherwise \"0\""),
            N_("    - ${value}: option value"),
            N_("    - ${value_undef}: \"1\" if value is null, otherwise \"0\""),
            N_("    - ${value_changed}: \"1\" if value is different from default "
               "value, otherwise \"0\""),
            N_("    - ${value2}: option value, with inherited value if null"),
            N_("    - ${parent_value}: parent option value"),
            N_("    - ${min}: min value"),
            N_("    - ${max}: max value"),
            N_("    - ${description}: option description (translated)"),
            N_("    - ${description2}: option description (translated), "
               "\"(no description)\" (translated) if there's no description"),
            N_("    - ${description_en}: option description (in English)"),
            N_("    - ${description_en2}: option description (in English), "
               "\"(no description)\" if there's no description"),
            N_("    - ${string_values}: string values allowed for set of an enum "
               "option"),
            N_("    - ${allowed_values}: allowed values"),
            N_("    - ${marked}: \"1\" if option is marked, otherwise \"0\""),
            N_("    - ${index}: index of option in list"),
            N_("  - option data, with color but no spaces:"),
            N_("    - same names prefixed by underscore, for example: ${_name}, "
               "${_type}, ..."),
            N_("  - option data, raw format (no colors/spaces):"),
            N_("    - same names prefixed by two underscores, for example: "
               "${__name}, ${__type}, ..."),
            N_("  - option data, only spaces:"),
            N_("    - same names prefixed with \"empty_\", for example: "
               "${empty_name}, ${empty_type}"),
            N_("  - other data:"),
            N_("    - ${selected_line}: \"1\" if the line is selected, "
               "otherwise \"0\""),
            N_("    - ${newline}: insert a new line at point, so the option is "
               "displayed on multiple lines"),
            "",
            N_("For keys, input and mouse actions on the buffer, "
               "see key bindings in User's guide."),
            "",
            N_("Note: if input has one or more leading spaces, the following text "
               "is interpreted as a filter, without the spaces. For example "
               "\" q\" searches all options with \"q\" inside name while \"q\" "
               "closes the fset buffer."),
            "",
            N_("Examples:"),
            AI("  /fset d:irc.*"),
            AI("  /fset nicklist"),
            AI("  /fset =red"),
            AI("  /fset ==red"),
            AI("  /fset c:${file} == irc && ${type_en} == integer")),
        "-bar"
        " || -refresh"
        " || -up 1|2|3|4|5"
        " || -down 1|2|3|4|5"
        " || -left 10|20|30|40|50|60|70|80|90|100"
        " || -right 10|20|30|40|50|60|70|80|90|100"
        " || -go 0|end"
        " || -toggle"
        " || -add -1|1"
        " || -reset"
        " || -unset"
        " || -set"
        " || -setnew"
        " || -append"
        " || -mark"
        " || -format"
        " || -export -help|-nohelp|%(filename) %(filename)"
        " || -import %(filename)"
        " || *|c:|f:|s:|d|d:|d=|d==|=|==|%(fset_options)",
        &fset_command_fset, NULL, NULL);
    weechat_hook_command_run ("/set", &fset_command_run_set_cb, NULL, NULL);
    weechat_hook_command_run ("/key", &fset_command_run_key_cb, NULL, NULL);
}
