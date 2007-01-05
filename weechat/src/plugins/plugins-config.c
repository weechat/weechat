/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* plugins-config.c: plugins configuration */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../common/weechat.h"
#include "weechat-plugin.h"
#include "plugins-config.h"
#include "../common/util.h"
#include "../irc/irc.h"


t_plugin_option *plugin_options = NULL;
t_plugin_option *last_plugin_option = NULL;


/*
 * plugin_config_search_internal: search a plugin option (internal function)
 *                                This function should not be called directly.
 */

t_plugin_option *
plugin_config_search_internal (char *option)
{
    t_plugin_option *ptr_plugin_option;
    
    for (ptr_plugin_option = plugin_options; ptr_plugin_option;
         ptr_plugin_option = ptr_plugin_option->next_option)
    {
        if (ascii_strcasecmp (ptr_plugin_option->name, option) == 0)
        {
            return ptr_plugin_option;
        }
    }
    
    /* plugin option not found */
    return NULL;
}

/*
 * plugin_config_search: search a plugin option
 */

t_plugin_option *
plugin_config_search (t_weechat_plugin *plugin, char *option)
{
    char *internal_option;
    t_plugin_option *ptr_plugin_option;
    
    internal_option = (char *)malloc (strlen (plugin->name) +
                                      strlen (option) + 2);
    if (!internal_option)
        return NULL;
    
    strcpy (internal_option, plugin->name);
    strcat (internal_option, ".");
    strcat (internal_option, option);
    
    ptr_plugin_option = plugin_config_search_internal (internal_option);
    
    free (internal_option);
    
    return ptr_plugin_option;
}

/*
 * plugin_config_find_pos: find position for a plugin option (for sorting options)
 */

t_plugin_option *
plugin_config_find_pos (char *name)
{
    t_plugin_option *ptr_option;
    
    for (ptr_option = plugin_options; ptr_option;
         ptr_option = ptr_option->next_option)
    {
        if (ascii_strcasecmp (name, ptr_option->name) < 0)
            return ptr_option;
    }
    return NULL;
}

/*
 * plugin_config_set_internal: set value for a plugin option (internal function)
 *                             This function should not be called directly.
 */

int
plugin_config_set_internal (char *option, char *value)
{
    t_plugin_option *ptr_plugin_option, *pos_option;
    
    ptr_plugin_option = plugin_config_search_internal (option);
    if (ptr_plugin_option)
    {
        if (!value || !value[0])
        {
            /* remove option from list */
            if (ptr_plugin_option->prev_option)
                (ptr_plugin_option->prev_option)->next_option =
                    ptr_plugin_option->next_option;
            else
                plugin_options = ptr_plugin_option->next_option;
            if (ptr_plugin_option->next_option)
                (ptr_plugin_option->next_option)->prev_option =
                    ptr_plugin_option->prev_option;
            return 1;
        }
        else
        {
            /* replace old value by new one */
            if (ptr_plugin_option->value)
                free (ptr_plugin_option->value);
            ptr_plugin_option->value = strdup (value);
            return 1;
        }
    }
    else
    {
        if (value && value[0])
        {
            ptr_plugin_option = (t_plugin_option *)malloc (sizeof (t_plugin_option));
            if (ptr_plugin_option)
            {
                /* create new option */
                ptr_plugin_option->name = strdup (option);
                ascii_tolower (ptr_plugin_option->name);
                ptr_plugin_option->value = strdup (value);
                
                if (plugin_options)
                {
                    pos_option = plugin_config_find_pos (ptr_plugin_option->name);
                    if (pos_option)
                    {
                        /* insert option into the list (before option found) */
                        ptr_plugin_option->prev_option = pos_option->prev_option;
                        ptr_plugin_option->next_option = pos_option;
                        if (pos_option->prev_option)
                            pos_option->prev_option->next_option = ptr_plugin_option;
                        else
                            plugin_options = ptr_plugin_option;
                        pos_option->prev_option = ptr_plugin_option;
                    }
                    else
                    {
                        /* add option to the end */
                        ptr_plugin_option->prev_option = last_plugin_option;
                        ptr_plugin_option->next_option = NULL;
                        last_plugin_option->next_option = ptr_plugin_option;
                        last_plugin_option = ptr_plugin_option;
                    }
                }
                else
                {
                    ptr_plugin_option->prev_option = NULL;
                    ptr_plugin_option->next_option = NULL;
                    plugin_options = ptr_plugin_option;
                    last_plugin_option = ptr_plugin_option;
                }
                return 1;
            }
        }
        else
            return 1;
    }
    
    /* failed to set plugin option */
    return 0;
}

/*
 * plugin_config_set: set value for a plugin option (create it if not found)
 */

int
plugin_config_set (t_weechat_plugin *plugin, char *option, char *value)
{
    char *internal_option;
    int return_code;
    
    internal_option = (char *)malloc (strlen (plugin->name) +
                                      strlen (option) + 2);
    if (!internal_option)
        return 0;
    
    strcpy (internal_option, plugin->name);
    strcat (internal_option, ".");
    strcat (internal_option, option);
    
    return_code = plugin_config_set_internal (internal_option, value);
    free (internal_option);
    
    return return_code;
}

/*
 * plugin_config_read: read WeeChat plugins configuration
 */

void
plugin_config_read ()
{
    int filename_length;
    char *filename;
    FILE *file;
    int line_number;
    char line[1024], *ptr_line, *ptr_line2, *pos, *pos2;

    filename_length = strlen (weechat_home) +
        strlen (WEECHAT_PLUGINS_CONFIG_NAME) + 2;
    filename =
        (char *) malloc (filename_length * sizeof (char));
    if (!filename)
        return;
    snprintf (filename, filename_length, "%s%s" WEECHAT_PLUGINS_CONFIG_NAME,
              weechat_home, DIR_SEPARATOR);
    if ((file = fopen (filename, "r")) == NULL)
        return;
    
    line_number = 0;
    while (!feof (file))
    {
        ptr_line = fgets (line, sizeof (line) - 1, file);
        line_number++;
        if (ptr_line)
        {
            /* encode line to internal charset */
            ptr_line2 = weechat_iconv_to_internal (NULL, ptr_line);
            if (ptr_line2)
            {
                snprintf (line, sizeof (line) - 1, "%s", ptr_line2);
                free (ptr_line2);
            }
            
            /* skip spaces */
            while (ptr_line[0] == ' ')
                ptr_line++;
            /* not a comment and not an empty line */
            if ((ptr_line[0] != '#') && (ptr_line[0] != '\r')
                && (ptr_line[0] != '\n'))
            {
                pos = strchr (line, '=');
                if (pos == NULL)
                {
                    irc_display_prefix (NULL, NULL, PREFIX_ERROR);
                    gui_printf (NULL,
                                _("%s %s, line %d: invalid syntax, missing \"=\"\n"),
                                WEECHAT_WARNING, filename, line_number);
                }
                else
                {
                    pos[0] = '\0';
                    pos++;
                    
                    /* remove spaces before '=' */
                    pos2 = pos - 2;
                    while ((pos2 > line) && (pos2[0] == ' '))
                    {
                        pos2[0] = '\0';
                        pos2--;
                    }
                    
                    /* skip spaces after '=' */
                    while (pos[0] && (pos[0] == ' '))
                    {
                        pos++;
                    }
                    
                    /* remove CR/LF */
                    pos2 = strchr (pos, '\r');
                    if (pos2 != NULL)
                        pos2[0] = '\0';
                    pos2 = strchr (pos, '\n');
                    if (pos2 != NULL)
                        pos2[0] = '\0';
                    
                    /* remove simple or double quotes 
                       and spaces at the end */
                    if (strlen(pos) > 1)
                    {
                        pos2 = pos + strlen (pos) - 1;
                        while ((pos2 > pos) && (pos2[0] == ' '))
                        {
                            pos2[0] = '\0';
                            pos2--;
                        }
                        pos2 = pos + strlen (pos) - 1;
                        if (((pos[0] == '\'') &&
                             (pos2[0] == '\'')) ||
                            ((pos[0] == '"') &&
                             (pos2[0] == '"')))
                        {
                            pos2[0] = '\0';
                            pos++;
                        }
                    }
                    
                    plugin_config_set_internal (ptr_line, pos);
                }
            }
        }
    }
    
    fclose (file);
    free (filename);
}

/*
 * plugin_config_write: write WeeChat configurtion
 *                      return:  0 if ok
 *                             < 0 if error
 */

int
plugin_config_write ()
{
    int filename_length;
    char *filename;
    FILE *file;
    time_t current_time;
    t_plugin_option *ptr_plugin_option;
    
    filename_length = strlen (weechat_home) +
        strlen (WEECHAT_PLUGINS_CONFIG_NAME) + 2;
    filename =
        (char *) malloc (filename_length * sizeof (char));
    if (!filename)
        return -2;
    snprintf (filename, filename_length, "%s%s" WEECHAT_PLUGINS_CONFIG_NAME,
              weechat_home, DIR_SEPARATOR);
    
    if ((file = fopen (filename, "w")) == NULL)
    {
        irc_display_prefix (NULL, NULL, PREFIX_ERROR);
        gui_printf (NULL, _("%s cannot create file \"%s\"\n"),
                    WEECHAT_ERROR, filename);
        free (filename);
        return -1;
    }
    
    current_time = time (NULL);
    weechat_iconv_fprintf (file, _("#\n# %s plugins configuration file, created by "
                                   "%s v%s on %s"),
                           PACKAGE_NAME, PACKAGE_NAME, PACKAGE_VERSION,
                           ctime (&current_time));
    weechat_iconv_fprintf (file, _("# WARNING! Be careful when editing this file, "
                                   "WeeChat writes this file when options are updated.\n#\n"));
    
    for (ptr_plugin_option = plugin_options; ptr_plugin_option;
         ptr_plugin_option = ptr_plugin_option->next_option)
    {
        weechat_iconv_fprintf (file, "%s = \"%s\"\n",
                               ptr_plugin_option->name,
                               ptr_plugin_option->value);
    }
    
    fclose (file);
    chmod (filename, 0600);
    free (filename);
    return 0;
}
