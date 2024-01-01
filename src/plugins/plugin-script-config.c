/*
 * plugin-script-config.c - configuration options, used by script plugins
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#include "weechat-plugin.h"
#include "plugin-script.h"


/*
 * Initializes script configuration.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
plugin_script_config_init (struct t_weechat_plugin *weechat_plugin,
                           struct t_plugin_script_data *plugin_data)
{
    struct t_config_section *ptr_section;
    char str_prio_name[PATH_MAX];

    snprintf (str_prio_name, sizeof (str_prio_name),
              "%d|%s",
              weechat_plugin->priority,
              weechat_plugin->name);

    *(plugin_data->config_file) = weechat_config_new (str_prio_name,
                                                      NULL, NULL, NULL);
    if (!(*plugin_data->config_file))
        return 0;

    /* look */
    ptr_section = weechat_config_new_section (*(plugin_data->config_file),
                                              "look",
                                              0, 0,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL,
                                              NULL, NULL, NULL);
    if (ptr_section)
    {
        *(plugin_data->config_look_check_license) = weechat_config_new_option (
            *(plugin_data->config_file), ptr_section,
            "check_license", "boolean",
            N_("check the license of scripts when they are loaded: if the "
               "license is different from the plugin license, a warning is "
               "displayed"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        *(plugin_data->config_look_eval_keep_context) = weechat_config_new_option (
            *(plugin_data->config_file), ptr_section,
            "eval_keep_context", "boolean",
            N_("keep context between two calls to the source code evaluation "
               "(option \"eval\" of script command or info \"%s_eval\"); "
               "a hidden script is used to eval script code; "
               "if this option is disabled, this hidden script is unloaded "
               "after each eval: this uses less memory, but is slower"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    return 1;
}
