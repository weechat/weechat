/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* notify.c: Notify plugin for WeeChat: set/save buffer notify levels */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

#include "../weechat-plugin.h"


#define NOTIFY_PLUGIN_NAME "notify"

WEECHAT_PLUGIN_NAME(NOTIFY_PLUGIN_NAME);
WEECHAT_PLUGIN_DESCRIPTION("Notify plugin for WeeChat (set/save buffer notify levels)");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_WEECHAT_VERSION(WEECHAT_VERSION);
WEECHAT_PLUGIN_LICENSE("GPL3");

#define NOTIFY_CONFIG_NAME "notify"

struct t_weechat_plugin *weechat_notify_plugin = NULL;
#define weechat_plugin weechat_notify_plugin

struct t_config_file *notify_config_file = NULL;
struct t_config_section *notify_config_section_buffer = NULL;

#define NOTIFY_NUM_LEVELS 4

char *notify_string[NOTIFY_NUM_LEVELS] =
{ "none", "highlight", "message", "all" };

int notify_debug = 0;


/*
 * notify_search: search a notify level by name
 */

int
notify_search (const char *notify_name)
{
    int i;

    for (i = 0; i < NOTIFY_NUM_LEVELS; i++)
    {
        if (weechat_strcasecmp (notify_name, notify_string[i]) == 0)
            return i;
    }
    
    /* notify level not found */
    return -1;
}

/*
 * notify_build_option_name: build option name for a buffer
 */

char *
notify_build_option_name (struct t_gui_buffer *buffer)
{
    char *option_name, *plugin_name, *name;
    int length;
    
    plugin_name = weechat_buffer_get_string (buffer, "plugin");
    name = weechat_buffer_get_string (buffer, "name");
    
    length = ((plugin_name) ? strlen (plugin_name) : strlen ("core")) + 1 +
        strlen (name) + 1;
    option_name = malloc (length);
    if (!option_name)
        return NULL;
    
    snprintf (option_name, length, "%s.%s",
              (plugin_name) ? plugin_name : "core",
              name);
    
    return option_name;
}

/*
 * notify_debug_cb: callback for "debug" signal
 */

int
notify_debug_cb (void *data, const char *signal, const char *type_data,
                 void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;

    if (strcmp (type_data, WEECHAT_HOOK_SIGNAL_STRING) == 0)
    {
        if (weechat_strcasecmp ((char *)signal_data, NOTIFY_PLUGIN_NAME) == 0)
        {
            notify_debug ^= 1;
            if (notify_debug)
                weechat_printf (NULL, _("%s: debug enabled"), NOTIFY_PLUGIN_NAME);
            else
                weechat_printf (NULL, _("%s: debug disabled"), NOTIFY_PLUGIN_NAME);
        }
    }
    
    return WEECHAT_RC_OK;
}

/*
 * notify_get: read a notify level in config file
 *             we first try with all arguments, then remove one by one
 *             to find notify level (from specific to general notify)
 */

int
notify_get (const char *name)
{
    char *option_name, *ptr_end;
    struct t_config_option *ptr_option;
    
    option_name = strdup (name);
    if (option_name)
    {
        ptr_end = option_name + strlen (option_name);
        while (ptr_end >= option_name)
        {
            ptr_option = weechat_config_search_option (notify_config_file,
                                                       notify_config_section_buffer,
                                                       option_name);
            if (ptr_option)
            {
                free (option_name);
                return weechat_config_integer (ptr_option);
            }
            ptr_end--;
            while ((ptr_end >= option_name) && (ptr_end[0] != '.'))
            {
                ptr_end--;
            }
            if ((ptr_end >= option_name) && (ptr_end[0] == '.'))
                ptr_end[0] = '\0';
        }
        ptr_option = weechat_config_search_option (notify_config_file,
                                                   notify_config_section_buffer,
                                                   option_name);
        
        free (option_name);
        
        if (ptr_option)
            return weechat_config_integer (ptr_option);
    }

    /* notify level not found */
    return -1;
}

/*
 * notify_set_buffer: set notify for a buffer
 */

void
notify_set_buffer (struct t_gui_buffer *buffer)
{
    char *option_name, notify_str[16];
    int notify;

    option_name = notify_build_option_name (buffer);
    if (option_name)
    {
        notify = notify_get (option_name);
        if (notify_debug)
        {
            weechat_printf (NULL,
                            _("notify: debug: set notify for buffer %s to "
                              "%d (%s)"),
                            option_name, notify,
                            (notify < 0) ? "reset" : notify_string[notify]);
        }
        /* set notify for buffer */
        snprintf (notify_str, sizeof (notify_str), "%d", notify);
        weechat_buffer_set (buffer, "notify", notify_str);
        free (option_name);
    }
}

/*
 * notify_set_buffer_all: set notify for all open buffers
 */

void
notify_set_buffer_all ()
{
    struct t_infolist *ptr_infolist;
    
    ptr_infolist = weechat_infolist_get ("buffer", NULL, NULL);
    if (ptr_infolist)
    {
        while (weechat_infolist_next (ptr_infolist))
        {
            notify_set_buffer (weechat_infolist_pointer (ptr_infolist,
                                                         "pointer"));
        }
        weechat_infolist_free (ptr_infolist);
    }
}

/*
 * notify_config_cb: callback for config hook
 */

int
notify_config_cb (void *data, const char *option, const char *value)
{
    /* make C compiler happy */
    (void) data;
    (void) option;
    (void) value;
    
    notify_set_buffer_all ();
    
    return WEECHAT_RC_OK;
}

/*
 * notify_config_reaload: reload notify configuration file
 */

int
notify_config_reload (void *data, struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) data;
    
    /* free all notify levels */
    weechat_config_section_free_options (notify_config_section_buffer);
    
    return weechat_config_reload (config_file);
}

/* 
 * notify_config_create_option: set a notify level
 */

int
notify_config_create_option (void *data, struct t_config_file *config_file,
                             struct t_config_section *section,
                             const char *option_name, const char *value)
{
    struct t_config_option *ptr_option;
    int rc;
    
    /* make C compiler happy */
    (void) data;
    
    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
    
    if (option_name)
    {
        ptr_option = weechat_config_search_option (config_file, section,
                                                   option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = weechat_config_option_set (ptr_option, value, 1);
            else
            {
                weechat_config_option_free (ptr_option);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = weechat_config_new_option (
                    config_file, section,
                    option_name, "integer", NULL,
                    "none|highlight|message|all",
                    0, 0, value, NULL, NULL, NULL, NULL, NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }
    
    if (rc == WEECHAT_CONFIG_OPTION_SET_ERROR)
    {
        weechat_printf (NULL,
                        _("%s%s: unable to set notify level \"%s\" => \"%s\""),
                        weechat_prefix ("error"), NOTIFY_PLUGIN_NAME,
                        option_name, value);
    }
    
    return rc;
}

/*
 * notify_config_init: init notify configuration file
 *                     return: 1 if ok, 0 if error
 */

int
notify_config_init ()
{
    struct t_config_section *ptr_section;
    
    notify_config_file = weechat_config_new (NOTIFY_CONFIG_NAME,
                                             &notify_config_reload, NULL);
    if (!notify_config_file)
        return 0;
    
    ptr_section = weechat_config_new_section (notify_config_file, "buffer",
                                              1, 1,
                                              NULL, NULL,
                                              NULL, NULL,
                                              NULL, NULL,
                                              &notify_config_create_option, NULL);
    if (!ptr_section)
    {
        weechat_config_free (notify_config_file);
        return 0;
    }
    
    notify_config_section_buffer = ptr_section;
    
    return 1;
}

/*
 * notify_config_read: read notify configuration file
 */

int
notify_config_read ()
{
    return weechat_config_read (notify_config_file);
}

/*
 * notify_config_write: write notify configuration file
 */

int
notify_config_write ()
{
    return weechat_config_write (notify_config_file);
}

/*
 * notify_buffer_open_signal_cb: callback for "buffer_open" signal
 */

int
notify_buffer_open_signal_cb (void *data, const char *signal,
                              const char *type_data, void *signal_data)
{
    /* make C compiler happy */
    (void) data;
    (void) signal;
    (void) type_data;
    
    notify_set_buffer (signal_data);
    
    return WEECHAT_RC_OK;
}

/*
 * notify_set: set a notify level for a buffer
 */

void
notify_set (struct t_gui_buffer *buffer, const char *name, int value)
{
    char notify_str[16];

    /* create/update option */
    if (notify_config_create_option (NULL,
                                     notify_config_file,
                                     notify_config_section_buffer,
                                     name,
                                     (value < 0) ?
                                     NULL : notify_string[value]) > 0)
    {
        /* set notify for buffer */
        snprintf (notify_str, sizeof (notify_str), "%d", value);
        weechat_buffer_set (buffer, "notify", notify_str);

        /* display message */
        if (value >= 0)
            weechat_printf (NULL, _("Notify level: %s => %s"),
                            name, notify_string[value]);
        else
            weechat_printf (NULL, _("Notify level: %s: removed"), name);
    }
}

/*
 * notify_command_cb: callback for /notify command
 */

int
notify_command_cb (void *data, struct t_gui_buffer *buffer, int argc,
                   char **argv, char **argv_eol)
{
    int notify_level;
    char *option_name;
    
    /* make C compiler happy */
    (void) data;
    
    /* check arguments */
    if (argc < 2)
    {
        weechat_printf (NULL,
                        _("%s%s: missing parameters"),
                        weechat_prefix ("error"), NOTIFY_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }

    /* check if notify level exists */
    if (weechat_strcasecmp (argv[1], "reset") == 0)
        notify_level = -1;
    else
    {
        notify_level = notify_search (argv_eol[1]);
        if (notify_level < 0)
        {
            weechat_printf (NULL,
                            _("%s%s: unknown notify level \"%s\""),
                            weechat_prefix ("error"), NOTIFY_PLUGIN_NAME,
                            argv_eol[1]);
            return WEECHAT_RC_ERROR;
        }
    }
    
    /* set buffer notify level */
    option_name = notify_build_option_name (buffer);
    
    if (option_name)
    {
        notify_set (buffer, option_name, notify_level);
        free (option_name);
    }
    else
        return WEECHAT_RC_ERROR;
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_init: init notify plugin
 */

int
weechat_plugin_init (struct t_weechat_plugin *plugin, int argc, char *argv[])
{
    /* make C compiler happy */
    (void) argc;
    (void) argv;
    
    weechat_plugin = plugin;
    
    notify_debug = weechat_config_boolean (weechat_config_get ("weechat.plugin.debug"));
    
    if (!notify_config_init ())
    {
        weechat_printf (NULL,
                        _("%s%s: error creating configuration file"),
                        weechat_prefix("error"), NOTIFY_PLUGIN_NAME);
        return WEECHAT_RC_ERROR;
    }
    notify_config_read ();
    
    /* /notify command */
    weechat_hook_command ("notify",
                          N_("change notify level for current buffer"),
                          N_("reset | none | highlight | message | all"),
                          N_("    reset: reset notify level to default value\n"
                             "     none: buffer will never be in hotlist\n"
                             "highlight: buffer will be in hotlist for "
                             "highlights only\n"
                             "  message: buffer will be in hotlist for "
                             "highlights and user messages only\n"
                             "      all: buffer will be in hotlist for "
                             "any text printed"),
                          "reset|none|highlight|message|all",
                          &notify_command_cb, NULL);

    /* callback when a buffer is open */
    weechat_hook_signal ("buffer_open", &notify_buffer_open_signal_cb, NULL);

    /* callback when a config option is changed */
    weechat_hook_config ("notify.buffer.*", &notify_config_cb, NULL);
    
    /* callback for debug */
    weechat_hook_signal ("debug", &notify_debug_cb, NULL);
    
    /* set notify for open buffers */
    notify_set_buffer_all ();
    
    return WEECHAT_RC_OK;
}

/*
 * weechat_plugin_end: end notify plugin
 */

int
weechat_plugin_end (struct t_weechat_plugin *plugin)
{
    /* make C compiler happy */
    (void) plugin;
    
    notify_config_write ();
    
    weechat_config_free (notify_config_file);
    
    return WEECHAT_RC_OK;
}
