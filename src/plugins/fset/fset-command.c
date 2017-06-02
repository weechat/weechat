/*
 * fset-command.c - Fast Set command
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
    int num_options, line, append, value, i;
    struct t_fset_option *ptr_fset_option;
    struct t_config_option *ptr_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) argv_eol;

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
        fset_bar_item_update ();
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
            if (line < 0)
                WEECHAT_COMMAND_ERROR;
            fset_buffer_set_current_line (line);
            fset_buffer_check_line_outside_window ();
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
                    if (ptr_fset_option->marked)
                    {
                        ptr_option = weechat_config_get (ptr_fset_option->name);
                        if (ptr_option)
                            fset_option_toggle_value (ptr_fset_option, ptr_option);
                    }
                }
                if (weechat_config_boolean (fset_config_look_unmark_after_action))
                    fset_option_unmark_all ();
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
                    if (ptr_fset_option->marked)
                    {
                        ptr_option = weechat_config_get (ptr_fset_option->name);
                        if (ptr_option)
                            fset_option_add_value (ptr_fset_option, ptr_option, value);
                    }
                }
                if (weechat_config_boolean (fset_config_look_unmark_after_action))
                    fset_option_unmark_all ();
            }
            else
            {
                fset_command_get_option (&ptr_fset_option, &ptr_option);
                fset_option_add_value (ptr_fset_option, ptr_option, value);
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
                    if (ptr_fset_option->marked)
                    {
                        ptr_option = weechat_config_get (ptr_fset_option->name);
                        if (ptr_option)
                            fset_option_reset_value (ptr_fset_option, ptr_option);
                    }
                }
                if (weechat_config_boolean (fset_config_look_unmark_after_action))
                    fset_option_unmark_all ();
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
                    if (ptr_fset_option->marked)
                    {
                        ptr_option = weechat_config_get (ptr_fset_option->name);
                        if (ptr_option)
                            fset_option_unset_value (ptr_fset_option, ptr_option);
                    }
                }
                if (weechat_config_boolean (fset_config_look_unmark_after_action))
                    fset_option_unmark_all ();
            }
            else
            {
                fset_command_get_option (&ptr_fset_option, &ptr_option);
                fset_option_unset_value (ptr_fset_option, ptr_option);
            }
            return WEECHAT_RC_OK;
        }

        if ((weechat_strcasecmp (argv[1], "-set") == 0)
            || (weechat_strcasecmp (argv[1], "-append") == 0))
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            append = (weechat_strcasecmp (argv[1], "-append") == 0) ? 1 : 0;
            fset_option_set (ptr_fset_option, ptr_option, buffer, append);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcasecmp (argv[1], "-mark") == 0)
        {
            fset_command_get_option (&ptr_fset_option, &ptr_option);
            value = fset_command_get_int_arg (argc, argv, 2, 1);
            fset_option_toggle_mark (ptr_fset_option, ptr_option, value);
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
    struct t_hashtable *old_max_length_field, *eval_extra_vars, *eval_options;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    if (strncmp (command, "/set", 4) != 0)
        return WEECHAT_RC_OK;

    ptr_condition = weechat_config_string (fset_config_look_condition_catch_set);
    if (!ptr_condition || !ptr_condition[0])
        return WEECHAT_RC_OK;

    rc = WEECHAT_RC_OK;

    argv = weechat_string_split (command, " ", 0, 0, &argc);

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
    old_max_length_field = fset_option_max_length_field;
    fset_option_max_length_field = fset_option_get_hashtable_max_length_field ();
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
        if (old_max_length_field)
            weechat_hashtable_free (old_max_length_field);
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
        weechat_hashtable_free (fset_option_max_length_field);
        fset_option_max_length_field = old_max_length_field;
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
           " || -go <line>|end"
           " || -toggle"
           " || -add [<value>]"
           " || -reset"
           " || -unset"
           " || -set"
           " || -append"
           " || -mark [<number>]"
           " || filter"),
        N_("    -bar: add the fset bar\n"
           "-refresh: force the refresh of the \"fset\" bar item\n"
           "     -up: move the selected line up by \"number\" lines\n"
           "   -down: move the selected line down by \"number\" lines\n"
           "     -go: select a line by number, first line number is 0 "
           "(\"end\" to select the last line)\n"
           " -toggle: toggle the boolean value\n"
           "    -add: add \"value\", which can be a negative number "
           "(only for integers and colors)\n"
           "  -reset: reset the value of option\n"
           "  -unset: unset the option\n"
           "    -set: add the /set command in input to edit the value of "
           "option (move the cursor at the beginning of value)\n"
           " -append: add the /set command to append something in the value "
           "of option (move the cursor at the end of value)\n"
           "   -mark: toggle mark on the option and move \"number\" lines "
           "(up/down, default is 1: one line down)\n"
           "  filter: set a new filter to see only matching options (this "
           "filter can be used as input in fset buffer as well); allowed "
           "formats are:\n"
           "             *       show all options (no filter)\n"
           "             f:xxx   show only configuration file \"xxx\"\n"
           "             s:xxx   show only section \"xxx\"\n"
           "             d       show only changed options\n"
           "             d:xxx   show only changed options with \"xxx\" in name\n"
           "             d=xxx   show only changed options with \"xxx\" in value\n"
           "             d==xxx  show only changed options with exact value \"xxx\"\n"
           "             =xxx    show only options with \"xxx\" in value\n"
           "             ==xxx   show only options with exact value \"xxx\"\n"
           "\n"
           "The lines with options are displayed using string evaluation "
           "(see /help eval for the format), with these options:\n"
           "  - fset.format.option: format for an option which is not on "
           "current line\n"
           "  - fset.format.option_current: format for the current option\n"
           "\n"
           "The following variables can be used in these options:\n"
           "  - option data, with color and padded by spaces on the right:\n"
           "    - ${name}: option name\n"
           "    - ${parent_name}: parent option name\n"
           "    - ${type}: option type\n"
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
           "    - ${description}: option description\n"
           "    - ${string_values}: string values allowed for set of an "
           "integer option using strings\n"
           "    - ${marked}: \"1\" if option is marked, othersiwe \"0\"\n"
           "  - option data, with color but no spaces:\n"
           "    - same names prefixed by underscore, for example: ${_name}, "
           "${_type}, ...\n"
           "  - option data, raw format (no colors/spaces):\n"
           "    - same names prefixed by two underscores, for example: "
           "${__name}, ${__type}, ...\n"
           "\n"
           "Keys and input on fset buffer:\n"
           "  alt+space     t  toggle boolean value\n"
           "  alt+'-'       -  subtract 1 from value (integer/color)\n"
           "  alt+'+'       +  add 1 to value (integer/color)\n"
           "  alt+f, alt+r  r  reset value\n"
           "  alt+f, alt+u  u  unset value\n"
           "  alt+enter     s  set value\n"
           "  alt+f, alt+a  a  append to value\n"
           "  alt+','       ,  mark/unmark option and move one line down\n"
           "  shift+down       mark/unmark option and move one line down\n"
           "  shift+up         mark/unmark option and move one line up\n"
           "                $  refresh options, unmark all options\n"
           "                q  close fset buffer\n"
           "\n"
           "Note: spaces at beginning of input are ignored, so for example "
           "\"q\" closes the fset buffer while \" q\" searches all options "
           "with \"q\" inside name."),
        "-bar"
        " || -refresh"
        " || -up 1|2|3|4|5"
        " || -down 1|2|3|4|5"
        " || -go"
        " || -toggle"
        " || -add -1|1"
        " || -reset"
        " || -unset"
        " || -set"
        " || -append"
        " || -mark"
        " || *|f:|s:|d|d:|d=|d==|=|==",
        &fset_command_fset, NULL, NULL);
    weechat_hook_command_run ("/set", &fset_command_run_set_cb, NULL, NULL);
}
