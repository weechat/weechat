/*
 * fset-command.c - Fast Set command
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
    int num_options, line, value, i, with_help, min, max, format_number;
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

    if (weechat_strcasecmp (argv[1], "-bar") == 0)
    {
        fset_add_bar ();
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "-refresh") == 0)
    {
        fset_option_get_options ();
        fset_buffer_refresh (0);
        weechat_command (NULL, "/window refresh");
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "-up") == 0)
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

    if (weechat_strcasecmp (argv[1], "-down") == 0)
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

    if (weechat_strcasecmp (argv[1], "-left") == 0)
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

    if (weechat_strcasecmp (argv[1], "-right") == 0)
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

    if (weechat_strcasecmp (argv[1], "-go") == 0)
    {
        if (fset_buffer)
        {
            if (argc < 3)
                WEECHAT_COMMAND_ERROR;
            if (weechat_strcasecmp (argv[2], "end") == 0)
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
        if (weechat_strcasecmp (argv[1], "-toggle") == 0)
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

        if (weechat_strcasecmp (argv[1], "-add") == 0)
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
                     || (ptr_fset_option->type == FSET_OPTION_TYPE_COLOR)))
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

        if (weechat_strcasecmp (argv[1], "-reset") == 0)
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

        if (weechat_strcasecmp (argv[1], "-unset") == 0)
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

        if (weechat_strcasecmp (argv[1], "-set") == 0)
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            fset_option_set (ptr_fset_option, ptr_option, buffer, 0);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcasecmp (argv[1], "-setnew") == 0)
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            fset_option_set (ptr_fset_option, ptr_option, buffer, -1);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcasecmp (argv[1], "-append") == 0)
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            fset_option_set (ptr_fset_option, ptr_option, buffer, 1);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcasecmp (argv[1], "-mark") == 0)
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            fset_option_toggle_mark (ptr_fset_option, ptr_option);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcasecmp (argv[1], "-format") == 0)
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

        if (weechat_strcasecmp (argv[1], "-export") == 0)
        {
            if (argc < 3)
                WEECHAT_COMMAND_ERROR;
            with_help = weechat_config_boolean (fset_config_look_export_help_default);
            ptr_filename = argv_eol[2];
            if (weechat_strcasecmp (argv[2], "-help") == 0)
            {
                with_help = 1;
                if (argc < 4)
                    WEECHAT_COMMAND_ERROR;
                ptr_filename = argv_eol[3];
            }
            else if (weechat_strcasecmp (argv[2], "-nohelp") == 0)
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
                return WEECHAT_RC_OK;
            }
            if (!fset_option_export (ptr_filename, with_help))
                WEECHAT_COMMAND_ERROR;
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
    (void) buffer;

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
        && ((weechat_strcasecmp (argv[1], "diff") == 0)
            || (weechat_strcasecmp (argv[1], "env") == 0)))
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
        if (result)
            free (result);
    }
    if (eval_extra_vars)
        weechat_hashtable_free (eval_extra_vars);
    if (eval_options)
        weechat_hashtable_free (eval_options);

    /* check condition to trigger the fset buffer */
    if (condition_ok)
    {
        if (old_options)
            weechat_arraylist_free (old_options);
        if (old_max_length)
            free (old_max_length);
        if (old_filter)
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
        if (old_filter)
            free (old_filter);
        fset_buffer_selected_line = old_buffer_selected_line;
    }

end:
    if (argv)
        weechat_string_free_split (argv);

    return rc;
}

/*
 * Hooks fset commands.
 */

void
fset_command_init ()
{
    weechat_hook_command (
        "fset",
        N_("fast set WeeChat and plugins options"),
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
           " || <filter>"),
        N_("       -bar: add the help bar\n"
           "   -refresh: refresh list of options, then whole screen "
           "(command: /window refresh)\n"
           "        -up: move the selected line up by \"number\" lines\n"
           "      -down: move the selected line down by \"number\" lines\n"
           "      -left: scroll the fset buffer by \"percent\" of width "
           "on the left\n"
           "     -right: scroll the fset buffer by \"percent\" of width "
           "on the right\n"
           "        -go: select a line by number, first line number is 0 "
           "(\"end\" to select the last line)\n"
           "    -toggle: toggle the boolean value\n"
           "       -add: add \"value\" (which can be a negative number) "
           "for integers and colors, set/append to value for other types "
           "(set for a negative value, append for a positive value)\n"
           "     -reset: reset the value of option\n"
           "     -unset: unset the option\n"
           "       -set: add the /set command in input to edit the value of "
           "option (move the cursor at the beginning of value)\n"
           "    -setnew: add the /set command in input to edit a new value "
           "for the option\n"
           "    -append: add the /set command to append something in the value "
           "of option (move the cursor at the end of value)\n"
           "      -mark: toggle mark\n"
           "    -format: switch to the next available format\n"
           "    -export: export the options and values displayed in a file "
           "(each line has format: \"/set name value\" or \"/unset name\")\n"
           "      -help: force writing of help on options in exported file "
           "(see /help fset.look.export_help_default)\n"
           "    -nohelp: do not write help on options in exported file "
           "(see /help fset.look.export_help_default)\n"
           "     filter: set a new filter to see only matching options (this "
           "filter can be used as input in fset buffer as well); allowed "
           "formats are:\n"
           "               *       show all options (no filter)\n"
           "               xxx     show only options with \"xxx\" in name\n"
           "               f:xxx   show only configuration file \"xxx\"\n"
           "               t:xxx   show only type \"xxx\" (bool/int/str/col)\n"
           "               d       show only changed options\n"
           "               d:xxx   show only changed options with \"xxx\" in "
           "name\n"
           "               d=xxx   show only changed options with \"xxx\" in "
           "value\n"
           "               d==xxx  show only changed options with exact value "
           "\"xxx\"\n"
           "               h=xxx   show only options with \"xxx\" in "
           "description (translated)\n"
           "               he=xxx  show only options with \"xxx\" in "
           "description (in English)\n"
           "               =xxx    show only options with \"xxx\" in value\n"
           "               ==xxx   show only options with exact value \"xxx\"\n"
           "               c:xxx   show only options matching the evaluated "
           "condition \"xxx\", using following variables: file, section, "
           "option, name, parent_name, type, type_en, type_short "
           "(bool/int/str/col), type_tiny (b/i/s/c), default_value, "
           "default_value_undef, value, quoted_value, value_undef, "
           "value_changed, parent_value, min, max, description, description2, "
           "description_en, description_en2, string_values\n"
           "\n"
           "The lines with options are displayed using string evaluation "
           "(see /help eval for the format), with these options:\n"
           "  - fset.format.option1: first format for an option\n"
           "  - fset.format.option2: second format for an option\n"
           "\n"
           "The following variables can be used in these options:\n"
           "  - option data, with color and padded by spaces on the right:\n"
           "    - ${file}: configuration file (for example \"weechat\" or "
           "\"irc\")\n"
           "    - ${section}: section\n"
           "    - ${option}: option name\n"
           "    - ${name}: full option name (file.section.option)\n"
           "    - ${parent_name}: parent option name\n"
           "    - ${type}: option type (translated)\n"
           "    - ${type_en}: option type (in English)\n"
           "    - ${type_short}: short option type (bool/int/str/col)\n"
           "    - ${type_tiny}: tiny option type (b/i/s/c)\n"
           "    - ${default_value}: option default value\n"
           "    - ${default_value_undef}: \"1\" if default value is null, "
           "otherwise \"0\"\n"
           "    - ${value}: option value\n"
           "    - ${value_undef}: \"1\" if value is null, otherwise \"0\"\n"
           "    - ${value_changed}: \"1\" if value is different from default "
           "value, otherwise \"0\"\n"
           "    - ${value2}: option value, with inherited value if null\n"
           "    - ${parent_value}: parent option value\n"
           "    - ${min}: min value\n"
           "    - ${max}: max value\n"
           "    - ${description}: option description (translated)\n"
           "    - ${description2}: option description (translated), "
           "\"(no description)\" (translated) if there's no description\n"
           "    - ${description_en}: option description (in English)\n"
           "    - ${description_en2}: option description (in English), "
           "\"(no description)\" if there's no description\n"
           "    - ${string_values}: string values allowed for set of an "
           "integer option using strings\n"
           "    - ${marked}: \"1\" if option is marked, otherwise \"0\"\n"
           "    - ${index}: index of option in list\n"
           "  - option data, with color but no spaces:\n"
           "    - same names prefixed by underscore, for example: ${_name}, "
           "${_type}, ...\n"
           "  - option data, raw format (no colors/spaces):\n"
           "    - same names prefixed by two underscores, for example: "
           "${__name}, ${__type}, ...\n"
           "  - option data, only spaces:\n"
           "    - same names prefixed with \"empty_\", for example: "
           "${empty_name}, ${empty_type}\n"
           "  - other data:\n"
           "    - ${selected_line}: \"1\" if the line is selected, "
           "otherwise \"0\"\n"
           "    - ${newline}: insert a new line at point, so the option is "
           "displayed on multiple lines\n"
           "\n"
           "Keys and input to move in on fset buffer:\n"
           "  up                        move one line up\n"
           "  down                      move one line down\n"
           "  pgup                      move one page up\n"
           "  pgdn                      move one page down\n"
           "  alt-home          <<      move to first line\n"
           "  alt-end           >>      move to last line\n"
           "  F11               <       scroll horizontally on the left\n"
           "  F12               >       scroll horizontally on the right\n"
           "\n"
           "Keys and input to set options on fset buffer:\n"
           "  alt+space         t       toggle boolean value\n"
           "  alt+'-'           -       subtract 1 from value for integer/color, "
           "set value for other types\n"
           "  alt+'+'           +       add 1 to value for integer/color, append "
           "to value for other types\n"
           "  alt+f, alt+r      r       reset value\n"
           "  alt+f, alt+u      u       unset value\n"
           "  alt+enter         s       set value\n"
           "  alt+f, alt+n      n       set new value\n"
           "  alt+f, alt+a      a       append to value\n"
           "  alt+','           ,       mark/unmark option\n"
           "  shift+up                  move one line up and mark/unmark option\n"
           "  shift+down                mark/unmark option and move one line down\n"
           "                    m:xxx   mark options displayed that are "
           "matching filter \"xxx\" (any filter on option or value is allowed, "
           "see filters above)\n"
           "                    u:xxx   unmark options displayed that are "
           "matching filter \"xxx\" (any filter on option or value is allowed, "
           "see filters above)\n"
           "\n"
           "Other keys and input on fset buffer:\n"
           "  ctrl+L                    refresh options and whole screen "
           "(command: /fset -refresh)\n"
           "                    $       refresh options (keep marked options)\n"
           "                    $$      refresh options (unmark all options)\n"
           "  alt+p             p       toggle plugin description options "
           "(plugins.desc.*)\n"
           "  alt+v             v       toggle help bar\n"
           "                    s:x,y   sort options by fields x,y "
           "(see /help fset.look.sort)\n"
           "                    s:      reset sort to its default value "
           "(see /help fset.look.sort)\n"
           "                    w:xxx   export options in file \"xxx\"\n"
           "                    w-:xxx  export options in file \"xxx\" without help\n"
           "                    w+:xxx  export options in file \"xxx\" with help\n"
           "  ctrl+X            x       switch the format used to display options\n"
           "                    q       close fset buffer\n"
           "\n"
           "Mouse actions on fset buffer:\n"
           "  wheel up/down                   move line up/down\n"
           "  left button                     move line here\n"
           "  right button                    toggle boolean (on/off) or "
           "edit the option value\n"
           "  right button + drag left/right  increase/decrease value "
           "for integer/color, set/append to value for other types\n"
           "  right button + drag up/down     mark/unmark multiple options\n"
           "\n"
           "Note: if input has one or more leading spaces, the following text "
           "is interpreted as a filter, without the spaces. For example "
           "\" q\" searches all options with \"q\" inside name while \"q\" "
           "closes the fset buffer.\n"
           "\n"
           "Examples:\n"
           "  show IRC options changed:\n"
           "    /fset d:irc.*\n"
           "  show all options with \"nicklist\" in name:\n"
           "    /fset nicklist\n"
           "  show all values which contain \"red\":\n"
           "    /fset =red\n"
           "  show all values which are exactly \"red\":\n"
           "    /fset ==red\n"
           "  show all integer options in irc plugin:\n"
           "    /fset c:${file} == irc && ${type_en} == integer"),
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
        " || *|c:|f:|s:|d|d:|d=|d==|=|==|%(fset_options)",
        &fset_command_fset, NULL, NULL);
    weechat_hook_command_run ("/set", &fset_command_run_set_cb, NULL, NULL);
}
