/*
 * Copyright (c) 2003-2010 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* alias-config.c: alias configuration options */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "alias.h"


struct t_config_file *alias_config_file = NULL;
struct t_config_section *alias_config_section_cmd = NULL;
struct t_config_section *alias_config_section_completion = NULL;


/*
 * alias_config_cmd_change_cb: callback called when alias option is modified
 *                             in section "cmd" (command)
 */

void
alias_config_cmd_change_cb (void *data, struct t_config_option *option)
{
    struct t_config_option *ptr_option_completion;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_option_completion = weechat_config_search_option (alias_config_file,
                                                          alias_config_section_completion,
                                                          weechat_config_option_get_pointer (option, "name"));
    
    alias_new (weechat_config_option_get_pointer (option, "name"),
               weechat_config_option_get_pointer (option, "value"),
               (ptr_option_completion) ?
               weechat_config_option_get_pointer (ptr_option_completion, "value") : NULL);
}

/*
 * alias_config_cmd_delete_cb: callback called when alias option is deleted
 *                             in section "cmd" (command)
 */

void
alias_config_cmd_delete_cb (void *data, struct t_config_option *option)
{
    struct t_config_option *ptr_option_completion;
    struct t_alias *ptr_alias;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_option_completion = weechat_config_search_option (alias_config_file,
                                                          alias_config_section_completion,
                                                          weechat_config_option_get_pointer (option, "name"));
    
    ptr_alias = alias_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_alias)
        alias_free (ptr_alias);
    
    if (ptr_option_completion)
        weechat_config_option_free (ptr_option_completion);
}

/*
 * alias_config_completion_change_cb: callback called when alias completion is
 *                                    modified in section "completion"
 */

void
alias_config_completion_change_cb (void *data, struct t_config_option *option)
{
    struct t_alias *ptr_alias;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_alias = alias_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_alias && ptr_alias->hook)
    {
        alias_update_completion (ptr_alias,
                                 weechat_config_option_get_pointer (option, "value"));
    }
}

/*
 * alias_config_completion_delete_cb: callback called when alias completion is
 *                                    deleted in section "completion"
 */

void
alias_config_completion_delete_cb (void *data, struct t_config_option *option)
{
    struct t_alias *ptr_alias;
    
    /* make C compiler happy */
    (void) data;
    
    ptr_alias = alias_search (weechat_config_option_get_pointer (option, "name"));
    if (ptr_alias && ptr_alias->hook && ptr_alias->completion)
    {
        alias_update_completion (ptr_alias, NULL);
    }
}

/*
 * alias_config_reload: reload alias configuration file
 */

int
alias_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_config_section_free_options (alias_config_section_cmd);
    weechat_config_section_free_options (alias_config_section_completion);
    alias_free_all ();
    
    return weechat_config_reload (config_file);
}

/*
 * alias_config_cmd_write_default_cb: write default aliases in configuration
 *                                    file in section "cmd" (command)
 */

void
alias_config_cmd_write_default_cb (void *data,
                                   struct t_config_file *config_file,
                                   const char *section_name)
{
    /* make C compiler happy */
    (void) data;
    
    weechat_config_write_line (config_file, section_name, NULL);
    
    weechat_config_write_line (config_file, "AAWAY", "%s", "\"allserv /away\"");
    weechat_config_write_line (config_file, "AME", "%s", "\"allchan /me\"");
    weechat_config_write_line (config_file, "AMSG", "%s", "\"allchan /msg *\"");
    weechat_config_write_line (config_file, "ANICK", "%s", "\"allserv /nick\"");
    weechat_config_write_line (config_file, "BYE", "%s", "\"quit\"");
    weechat_config_write_line (config_file, "C", "%s", "\"buffer clear\"");
    weechat_config_write_line (config_file, "CL", "%s", "\"buffer clear\"");
    weechat_config_write_line (config_file, "CLOSE", "%s", "\"buffer close\"");
    weechat_config_write_line (config_file, "CHAT", "%s", "\"dcc chat\"");
    weechat_config_write_line (config_file, "EXIT", "%s", "\"quit\"");
    weechat_config_write_line (config_file, "IG", "%s", "\"ignore\"");
    weechat_config_write_line (config_file, "J", "%s", "\"join\"");
    weechat_config_write_line (config_file, "K", "%s", "\"kick\"");
    weechat_config_write_line (config_file, "KB", "%s", "\"kickban\"");
    weechat_config_write_line (config_file, "LEAVE", "%s", "\"part\"");
    weechat_config_write_line (config_file, "M", "%s", "\"msg\"");
    weechat_config_write_line (config_file, "MUB", "%s", "\"unban *\"");
    weechat_config_write_line (config_file, "N", "%s", "\"names\"");
    weechat_config_write_line (config_file, "Q", "%s", "\"query\"");
    weechat_config_write_line (config_file, "REDRAW", "%s", "\"window refresh\"");
    weechat_config_write_line (config_file, "SAY", "%s", "\"msg *\"");
    weechat_config_write_line (config_file, "SIGNOFF", "%s", "\"quit\"");
    weechat_config_write_line (config_file, "T", "%s", "\"topic\"");
    weechat_config_write_line (config_file, "UB", "%s", "\"unban\"");
    weechat_config_write_line (config_file, "V", "%s", "\"command core version\"");
    weechat_config_write_line (config_file, "W", "%s", "\"who\"");
    weechat_config_write_line (config_file, "WC", "%s", "\"window merge\"");
    weechat_config_write_line (config_file, "WI", "%s", "\"whois\"");
    weechat_config_write_line (config_file, "WII", "%s", "\"whois $1 $1\"");
    weechat_config_write_line (config_file, "WW", "%s", "\"whowas\"");
}

/*
 * alias_config_cmd_new_option: create new option in section "cmd" (command)
 */

void
alias_config_cmd_new_option (const char *name, const char *command)
{
    weechat_config_new_option (alias_config_file, alias_config_section_cmd,
                               name, "string", NULL,
                               NULL, 0, 0, "", command, 0,
                               NULL, NULL,
                               &alias_config_cmd_change_cb, NULL,
                               &alias_config_cmd_delete_cb, NULL);
}

/*
 * alias_config_cmd_create_option_cb: create an alias in section "cmd"
 *                                    (command)
 */

int
alias_config_cmd_create_option_cb (void *data,
                                   struct t_config_file *config_file,
                                   struct t_config_section *section,
                                   const char *option_name, const char *value)
{
    struct t_alias *ptr_alias;
    int rc;
    
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;
    
    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    
    /* create config option */
    alias_config_cmd_new_option (option_name, value);
    
    /* create alias */
    ptr_alias = alias_search (option_name);
    if (ptr_alias)
        alias_free (ptr_alias);
    if (value && value[0])
        rc = (alias_new (option_name, value, NULL)) ?
            WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
    else
        rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
    
    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating alias \"%s\" => \"%s\""),
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        option_name, value);
    }
    
    return rc;
}

/*
 * alias_config_completion_new_option: create new option in section "completion"
 */

void
alias_config_completion_new_option (const char *name, const char *completion)
{
    weechat_config_new_option (alias_config_file,
                               alias_config_section_completion,
                               name, "string", NULL,
                               NULL, 0, 0, "", completion, 0,
                               NULL, NULL,
                               &alias_config_completion_change_cb, NULL,
                               &alias_config_completion_delete_cb, NULL);
}

/*
 * alias_config_completion_create_option_cb: create a completion for an alias
 *                                           in section "completion"
 */

int
alias_config_completion_create_option_cb (void *data,
                                          struct t_config_file *config_file,
                                          struct t_config_section *section,
                                          const char *option_name,
                                          const char *value)
{
    struct t_alias *ptr_alias;
    
    /* make C compiler happy */
    (void) data;
    (void) config_file;
    (void) section;
    
    ptr_alias = alias_search (option_name);
    if (!ptr_alias)
    {
        weechat_printf (NULL,
                        _("%s%s: error creating completion for alias \"%s\": "
                          "alias not found"),
                        weechat_prefix ("error"), ALIAS_PLUGIN_NAME,
                        option_name);
        return WEECHAT_CONFIG_OPTION_SET_ERROR;
    }
    
    /* create config option */
    alias_config_completion_new_option (option_name, value);
    
    /* create/update completion in alias */
    alias_update_completion (ptr_alias, value);
    
    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * alias_config_init: init alias configuration file
 *                    return: 1 if ok, 0 if error
 */

int
alias_config_init ()
{
    struct t_config_section *ptr_section;
    
    alias_config_file = weechat_config_new (ALIAS_CONFIG_NAME,
                                            &alias_config_reload, NULL);
    if (!alias_config_file)
        return 0;
    
    ptr_section = weechat_config_new_section (alias_config_file, "cmd",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &alias_config_cmd_write_default_cb, NULL,
                                              &alias_config_cmd_create_option_cb, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (alias_config_file);
        return 0;
    }
    alias_config_section_cmd = ptr_section;
    
    ptr_section = weechat_config_new_section (alias_config_file, "completion",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &alias_config_completion_create_option_cb, NULL,
                                              NULL, NULL);
    if (!ptr_section)
    {
        weechat_config_free (alias_config_file);
        return 0;
    }
    alias_config_section_completion = ptr_section;
    
    return 1;
}

/*
 * alias_config_read: read alias configuration file
 */

int
alias_config_read ()
{
    return weechat_config_read (alias_config_file);
}

/*
 * alias_config_write: write alias configuration file
 */

int
alias_config_write ()
{
    return weechat_config_write (alias_config_file);
}
