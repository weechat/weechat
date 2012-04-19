/*
 * Copyright (C) 2010-2012 Sebastien Helleu <flashcode@flashtux.org>
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

/*
 * rmodifier-config.c: rmodifier configuration options (file rmodifier.conf)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "../weechat-plugin.h"
#include "rmodifier.h"
#include "rmodifier-config.h"


struct t_config_file *rmodifier_config_file = NULL;
struct t_config_section *rmodifier_config_section_modifier = NULL;

struct t_config_option *rmodifier_config_look_hide_char;

char *rmodifier_config_default_list[][4] =
{
    { "nickserv", "history_add,input_text_display",
      "^(/(msg|quote) +nickserv +(id|identify|ghost \\S+|release \\S+) +)(.*)", "1,4*"
    },
    { "server", "history_add,input_text_display",
      "^(/(server|connect) .*-(sasl_)?password=)(\\S+)(.*)", "1,4*,5"
    },
    { "oper", "history_add,input_text_display",
      "^(/oper +\\S+ +)(.*)", "1,2*"
    },
    { "quote_pass", "history_add,input_text_display",
      "^(/quote pass +)(.*)", "1,2*"
    },
    { "set_pass", "history_add",
      "^(/set +\\S*password\\S* +)(.*)", "1,2*"
    },
    { NULL, NULL, NULL, NULL },
};


/*
 * rmodifier_config_reload: reload rmodifier configuration file
 */

int
rmodifier_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;

    rmodifier_free_all ();
    weechat_config_section_free_options (rmodifier_config_section_modifier);

    return weechat_config_reload (config_file);
}

/*
 * rmodifier_config_modifier_change_cb: callback called when a rmodifier is
 *                                      modified in section "modifier"
 */

void
rmodifier_config_modifier_change_cb (void *data, struct t_config_option *option)
{
    /* make C compiler happy */
    (void) data;

    rmodifier_new_with_string (weechat_config_option_get_pointer (option, "name"),
                               weechat_config_option_get_pointer (option, "value"));
}

/*
 * rmodifier_config_modifier_delete_cb: callback called when rmodifier option
 *                           is deleted in section "modifier"
 */

void
rmodifier_config_modifier_delete_cb (void *data, struct t_config_option *option)
{
    struct t_rmodifier *ptr_rmodifier;

    /* make C compiler happy */
    (void) data;

    ptr_rmodifier = rmodifier_search (weechat_config_option_get_pointer (option,
                                                                         "name"));
    if (ptr_rmodifier)
        rmodifier_free (ptr_rmodifier);
}

/*
 * rmodifier_config_modifier_write_default_cb: write default rmodifiers in
 *                                             configuration file in section
 *                                             "modifier"
 */

int
rmodifier_config_modifier_write_default_cb (void *data,
                                            struct t_config_file *config_file,
                                            const char *section_name)
{
    int i;

    /* make C compiler happy */
    (void) data;

    if (!weechat_config_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (i = 0; rmodifier_config_default_list[i][0]; i++)
    {
        if (!weechat_config_write_line (config_file,
                                        rmodifier_config_default_list[i][0],
                                        "\"%s;%s;%s\"",
                                        rmodifier_config_default_list[i][1],
                                        rmodifier_config_default_list[i][2],
                                        rmodifier_config_default_list[i][3]))
            return WEECHAT_CONFIG_WRITE_ERROR;
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * rmodifier_config_modifier_new_option: create new option in section "modifier"
 */

void
rmodifier_config_modifier_new_option (const char *name, const char *modifiers,
                                      const char *regex, const char *groups)
{
    int length;
    char *value;

    length = strlen (modifiers) + 1 + strlen (regex) + 1 +
        ((groups) ? strlen (groups) : 0) + 1;
    value = malloc (length);
    if (value)
    {
        snprintf (value, length, "%s;%s;%s",
                  modifiers, regex,
                  (groups) ? groups : "");
        weechat_config_new_option (rmodifier_config_file,
                                   rmodifier_config_section_modifier,
                                   name, "string", NULL,
                                   NULL, 0, 0, "", value, 0,
                                   NULL, NULL,
                                   &rmodifier_config_modifier_change_cb, NULL,
                                   &rmodifier_config_modifier_delete_cb, NULL);
        free (value);
    }
}

/*
 * rmodifier_config_modifier_create_option_cb: callback to create option in
 *                                             "modifier" section
 */

int
rmodifier_config_modifier_create_option_cb (void *data,
                                            struct t_config_file *config_file,
                                            struct t_config_section *section,
                                            const char *option_name,
                                            const char *value)
{
    struct t_rmodifier *ptr_rmodifier;
    int rc;

    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;

    /* create rmodifier */
    ptr_rmodifier = rmodifier_search (option_name);
    if (ptr_rmodifier)
        rmodifier_free (ptr_rmodifier);
    if (value && value[0])
    {
        ptr_rmodifier = rmodifier_new_with_string (option_name, value);
        if (ptr_rmodifier)
        {
            rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            /* create option */
            rmodifier_config_modifier_new_option (ptr_rmodifier->name,
                                                  ptr_rmodifier->modifiers,
                                                  ptr_rmodifier->str_regex,
                                                  ptr_rmodifier->groups);
        }
        else
            rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    }
    else
        rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating rmodifier "
                          "\"%s\" => \"%s\""),
                        weechat_prefix ("error"), RMODIFIER_PLUGIN_NAME,
                        option_name, value);
    }

    return rc;
}

/*
 * rmodifier_config_init: init rmodifier configuration file
 *                        return: 1 if ok, 0 if error
 */

int
rmodifier_config_init ()
{
    struct t_config_section *ptr_section;

    rmodifier_config_file = weechat_config_new (RMODIFIER_CONFIG_NAME,
                                                &rmodifier_config_reload, NULL);
    if (!rmodifier_config_file)
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (rmodifier_config_file, "look",
                                              0, 0,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL, NULL, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (rmodifier_config_file);
        return 0;
    }

    rmodifier_config_look_hide_char = weechat_config_new_option (
        rmodifier_config_file, ptr_section,
        "hide_char", "string",
        N_("char used to hide part of a string"),
        NULL, 0, 0, "*", NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);

    /* modifier */
    ptr_section = weechat_config_new_section (rmodifier_config_file, "modifier",
                                              0, 0,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &rmodifier_config_modifier_write_default_cb, NULL,
                                              &rmodifier_config_modifier_create_option_cb, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (rmodifier_config_file);
        return 0;
    }
    rmodifier_config_section_modifier = ptr_section;

    return 1;
}

/*
 * rmodifier_config_read: read rmodifier configuration file
 */

int
rmodifier_config_read ()
{
    return weechat_config_read (rmodifier_config_file);
}

/*
 * rmodifier_config_write: write rmodifier configuration file
 */

int
rmodifier_config_write ()
{
    return weechat_config_write (rmodifier_config_file);
}
