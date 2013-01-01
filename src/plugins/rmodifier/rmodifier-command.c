/*
 * rmodifier-command.c - rmodifier command
 *
 * Copyright (C) 2010-2013 Sebastien Helleu <flashcode@flashtux.org>
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

#include "../weechat-plugin.h"
#include "rmodifier.h"
#include "rmodifier-command.h"
#include "rmodifier-config.h"


/*
 * Displays a rmodifier.
 */

void
rmodifier_command_print (const char *name, const char *modifiers,
                         const char *str_regex, const char *groups)
{
    weechat_printf (NULL,
                    "  %s[%s%s%s]%s %s%s: \"%s%s%s\" [%s%s%s]",
                    weechat_color ("chat_delimiters"),
                    weechat_color ("chat"),
                    name,
                    weechat_color ("chat_delimiters"),
                    weechat_color ("chat"),
                    modifiers,
                    weechat_color ("chat_delimiters"),
                    weechat_color ("chat_host"),
                    str_regex,
                    weechat_color ("chat_delimiters"),
                    weechat_color ("chat"),
                    groups,
                    weechat_color ("chat_delimiters"));
}

/*
 * Displays list of rmodifiers.
 */

void
rmodifier_command_list (const char *message)
{
    struct t_rmodifier *ptr_rmodifier;

    if (rmodifier_list)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, message);
        for (ptr_rmodifier = rmodifier_list; ptr_rmodifier;
             ptr_rmodifier = ptr_rmodifier->next_rmodifier)
        {
            rmodifier_command_print (ptr_rmodifier->name,
                                     ptr_rmodifier->modifiers,
                                     ptr_rmodifier->str_regex,
                                     ptr_rmodifier->groups);
        }
    }
    else
        weechat_printf (NULL, _("No rmodifier defined"));
}

/*
 * Callback for command "/rmodifier": manages rmodifiers.
 */

int
rmodifier_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                      char **argv, char **argv_eol)
{
    struct t_rmodifier *ptr_rmodifier;
    struct t_config_option *ptr_option;
    int i, count;

    /* make C compiler happy */
    (void) data;
    (void) buffer;

    if ((argc == 1)
        || ((argc == 2) && (weechat_strcasecmp (argv[1], "list") == 0)))
    {
        /* list all rmodifiers */
        rmodifier_command_list (_("List of rmodifiers:"));
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "listdefault") == 0)
    {
        /* list default rmodifiers */
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("Default rmodifiers:"));
        for (i = 0; rmodifier_config_default_list[i][0]; i++)
        {
            rmodifier_command_print (rmodifier_config_default_list[i][0],
                                     rmodifier_config_default_list[i][1],
                                     rmodifier_config_default_list[i][2],
                                     rmodifier_config_default_list[i][3]);
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "add") == 0)
    {
        /* add a rmodifier */
        if (argc < 6)
        {
            weechat_printf (NULL,
                            _("%sError: missing arguments for \"%s\" "
                              "command"),
                            weechat_prefix ("error"), "rmodifier");
            return WEECHAT_RC_OK;
        }
        ptr_rmodifier = rmodifier_new (argv[2], argv[3], argv_eol[5], argv[4]);
        if (!ptr_rmodifier)
        {
            weechat_printf (NULL,
                            _("%s%s: error creating rmodifier \"%s\""),
                            weechat_prefix ("error"), RMODIFIER_PLUGIN_NAME,
                            argv[2]);
            return WEECHAT_RC_OK;
        }
        /* create configuration option */
        ptr_option = weechat_config_search_option (rmodifier_config_file,
                                                   rmodifier_config_section_modifier,
                                                   argv[2]);
        if (ptr_option)
            weechat_config_option_free (ptr_option);
        rmodifier_config_modifier_new_option (ptr_rmodifier->name,
                                              ptr_rmodifier->modifiers,
                                              ptr_rmodifier->str_regex,
                                              ptr_rmodifier->groups);

        /* display message */
        weechat_printf (NULL, _("Rmodifier \"%s\" created"),
                        ptr_rmodifier->name);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcasecmp (argv[1], "del") == 0)
    {
        /* add a rmodifier */
        if (argc < 3)
        {
            weechat_printf (NULL,
                            _("%sError: missing arguments for \"%s\" "
                              "command"),
                            weechat_prefix ("error"), "rmodifier");
            return WEECHAT_RC_OK;
        }
        if (weechat_strcasecmp (argv[2], "-all") == 0)
        {
            count = rmodifier_count;
            rmodifier_free_all ();
            weechat_config_section_free_options (rmodifier_config_section_modifier);
            if (count > 0)
                weechat_printf (NULL, _("%d rmodifiers removed"), count);
        }
        else
        {
            for (i = 2; i < argc; i++)
            {
                ptr_rmodifier = rmodifier_search (argv[i]);
                if (ptr_rmodifier)
                {
                    ptr_option = weechat_config_search_option (rmodifier_config_file,
                                                               rmodifier_config_section_modifier,
                                                               argv[i]);
                    if (ptr_option)
                        weechat_config_option_free (ptr_option);
                    rmodifier_free (ptr_rmodifier);
                    weechat_printf (NULL, _("Rmodifier \"%s\" removed"),
                                    argv[i]);
                }
                else
                {
                    weechat_printf (NULL, _("%sRmodifier \"%s\" not found"),
                                    weechat_prefix ("error"), argv[i]);
                }
            }
        }
        return WEECHAT_RC_OK;
    }

    /* restore default rmodifiers (only with "-yes", for security reason) */
    if (weechat_strcasecmp (argv[1], "default") == 0)
    {
        if ((argc >= 3) && (weechat_strcasecmp (argv[2], "-yes") == 0))
        {
            rmodifier_free_all ();
            weechat_config_section_free_options (rmodifier_config_section_modifier);
            rmodifier_create_default ();
            rmodifier_command_list (_("Default rmodifiers restored:"));
        }
        else
        {
            weechat_printf (NULL,
                            _("%sError: \"-yes\" argument is required for "
                              "restoring default rmodifiers (security reason)"),
                            weechat_prefix ("error"));
            return WEECHAT_RC_OK;
        }
        return WEECHAT_RC_OK;
    }

    return WEECHAT_RC_OK;
}

/*
 * Hooks command.
 */

void
rmodifier_command_init ()
{
    weechat_hook_command ("rmodifier",
                          N_("alter modifier strings with regular expressions"),
                          N_("list|listdefault"
                             " || add <name> <modifiers> <groups> <regex>"
                             " || del <name>|-all [<name>...]"
                             " || default -yes"),
                          N_("       list: list all rmodifiers\n"
                             "listdefault: list default rmodifiers\n"
                             "        add: add a rmodifier\n"
                             "       name: name of rmodifier\n"
                             "  modifiers: comma separated list of modifiers\n"
                             "     groups: action on groups found: comma separated "
                             "list of groups (from 1 to 9) with optional \"*\" "
                             "after number to hide group\n"
                             "      regex: regular expression (case insensitive, "
                             "can start by \"(?-i)\" to become case sensitive)\n"
                             "        del: delete a rmodifier\n"
                             "       -all: delete all rmodifiers\n"
                             "    default: restore default rmodifiers\n\n"
                             "Examples:\n"
                             "  hide everything typed after a command /password:\n"
                             "    /rmodifier add password input_text_display 1,2* ^(/password +)(.*)\n"
                             "  delete rmodifier \"password\":\n"
                             "    /rmodifier del password\n"
                             "  delete all rmodifiers:\n"
                             "    /rmodifier del -all"),
                          "list"
                          " || listdefault"
                          " || add %(rmodifier)"
                          " || del %(rmodifier)|-all %(rmodifier)|%*"
                          " || default",
                          &rmodifier_command_cb, NULL);
}
