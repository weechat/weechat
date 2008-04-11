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

/* script.c: script interface for WeeChat plugins */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-callback.h"


#define SCRIPT_OPTION_CHECK_LICENSE "check_license"

int script_option_check_license = 0;


/*
 * script_config_read: read script configuration
 */

void
script_config_read (struct t_weechat_plugin *weechat_plugin)
{
    char *string;
    
    string = weechat_config_get_plugin (SCRIPT_OPTION_CHECK_LICENSE);
    if (!string)
    {
        weechat_config_set_plugin (SCRIPT_OPTION_CHECK_LICENSE, "on");
        string = weechat_config_get_plugin (SCRIPT_OPTION_CHECK_LICENSE);
    }
    if (string && (weechat_config_string_to_boolean (string) > 0))
        script_option_check_license = 1;
    else
        script_option_check_license = 0;
}

/*
 * script_config_cb: callback called when config option is changed
 */

int
script_config_cb (void *data, char *option, char *value)
{
    /* make C compiler happy */
    (void) option;
    (void) value;
    
    script_config_read (data);
    
    return WEECHAT_RC_OK;
}

/*
 * script_init: initialize script
 */

void
script_init (struct t_weechat_plugin *weechat_plugin,
             int (*callback_command)(void *data,
                                     struct t_gui_buffer *buffer,
                                     int argc, char **argv,
                                     char **argv_eol),
             int (*callback_completion)(void *data, char *completion,
                                        struct t_gui_buffer *buffer,
                                        struct t_weelist *list),
             int (*callback_signal_debug_dump)(void *data, char *signal,
                                               char *type_data,
                                               void *signal_data),
             int (*callback_load_file)(void *data, char *filename))
{
    char *string, *completion = "list|listfull|load|autoload|reload|unload %f";
    int length;
    
    /* read script configuration */
    script_config_read (weechat_plugin);
    
    /* add hook for config option */
    length = strlen (weechat_plugin->name) + 64;
    string = malloc (length);
    if (string)
    {
        snprintf (string, length, "plugins.var.%s.%s",
                  weechat_plugin->name, SCRIPT_OPTION_CHECK_LICENSE);
        weechat_hook_config (string, &script_config_cb, weechat_plugin);
        free (string);
    }
    
    /* create directories in WeeChat home */
    weechat_mkdir_home (weechat_plugin->name, 0755);
    length = strlen (weechat_plugin->name) + strlen ("/autoload") + 1;
    string = malloc (length);
    if (string)
    {
        snprintf (string, length, "%s/autoload", weechat_plugin->name);
        weechat_mkdir_home (string, 0755);
        free (string);
    }
    
    /* add command */
    length = strlen (completion) + strlen (weechat_plugin->name) + 16;
    string = malloc (length);
    if (string)
    {
        snprintf (string, length, "%s|%%(%s_script)",
                  completion, weechat_plugin->name);
    }
    weechat_hook_command (weechat_plugin->name,
                          _("list/load/unload scripts"),
                          _("[list [name]] | [listfull [name]] "
                            "[load filename] | [autoload] | "
                            "[reload] | [unload [name]]"),
                          _("filename: script (file) to load\n"
                            "name: a script name\n\n"
                            "Without argument, this command "
                            "lists all loaded scripts."),
                          (string) ? string : completion,
                          callback_command, NULL);
    if (string)
        free (string);

    /* add completion */
    length = strlen (weechat_plugin->name) + 16;
    string = malloc (length);
    if (string)
    {
        snprintf (string, length, "%s_script", weechat_plugin->name);
        weechat_hook_completion (string, callback_completion, NULL);
        free (string);
    }
    
    /* add signal for "debug_dump" */
    weechat_hook_signal ("debug_dump", callback_signal_debug_dump, NULL);
    
    /* autoload scripts */
    script_auto_load (weechat_plugin, callback_load_file);
}

/*
 * script_ptr2str: convert pointer to string for usage in a script
 *                 (any language)
 *                 WARNING: result has to be free() after use
 */

char *
script_ptr2str (void *pointer)
{
    char pointer_str[128];

    if (!pointer)
        return strdup ("");
    
    snprintf (pointer_str, sizeof (pointer_str) - 1,
              "0x%x", (unsigned int)pointer);
    
    return strdup (pointer_str);
}

/*
 * script_str2ptr: convert stirng to pointer for usage outside script
 */

void *
script_str2ptr (char *pointer_str)
{
    unsigned int value;
    
    if (!pointer_str || (pointer_str[0] != '0') || (pointer_str[1] != 'x'))
        return NULL;
    
    sscanf (pointer_str + 2, "%x", &value);
    
    return (void *)value;
}

/*
 * script_auto_load: auto-load all scripts in a directory
 */

void
script_auto_load (struct t_weechat_plugin *weechat_plugin,
                  int (*callback)(void *data, char *filename))
{
    char *dir_home, *dir_name;
    int dir_length;
    
    /* build directory, adding WeeChat home */
    dir_home = weechat_info_get ("weechat_dir");
    if (!dir_home)
        return;
    dir_length = strlen (dir_home) + strlen (weechat_plugin->name) + 16;
    dir_name = malloc (dir_length);
    if (!dir_name)
        return;
    
    snprintf (dir_name, dir_length,
              "%s/%s/autoload", dir_home, weechat_plugin->name);
    weechat_exec_on_files (dir_name, NULL, callback);
    
    free (dir_name);
}

/*
 * script_search: search a script in list
 */

struct t_plugin_script *
script_search (struct t_weechat_plugin *weechat_plugin,
               struct t_plugin_script *scripts, char *name)
{
    struct t_plugin_script *ptr_script;
    
    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (weechat_strcasecmp (ptr_script->name, name) == 0)
            return ptr_script;
    }
    
    /* script not found */
    return NULL;
}

/*
 * script_search_full_name: search the full path name of a script
 */

char *
script_search_full_name (struct t_weechat_plugin *weechat_plugin,
                         char *filename)
{
    char *final_name, *dir_home, *dir_system;
    int length;
    struct stat st;
    
    if (filename[0] == '~')
    {
        dir_home = getenv ("HOME");
        if (!dir_home)
            return NULL;
        length = strlen (dir_home) + strlen (filename + 1) + 1;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length, "%s%s", dir_home, filename + 1);
            return final_name;
        }
        return NULL;
    }
    
    dir_home = weechat_info_get ("weechat_dir");
    if (dir_home)
    {
        /* try WeeChat user's autoload dir */
        length = strlen (dir_home) + strlen (weechat_plugin->name) + 8 +
            strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s/autoload/%s",
                      dir_home, weechat_plugin->name, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }

        /* try WeeChat language user's dir */
        length = strlen (dir_home) + strlen (weechat_plugin->name) +
            strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s/%s", dir_home, weechat_plugin->name, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }
        
        /* try WeeChat user's dir */
        length = strlen (dir_home) + strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name, length,
                      "%s/%s", dir_home, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }
    }
    
    /* try WeeChat system dir */
    dir_system = weechat_info_get ("weechat_sharedir");
    if (dir_system)
    {
        length = strlen (dir_system) + strlen (weechat_plugin->name) +
            strlen (filename) + 16;
        final_name = malloc (length);
        if (final_name)
        {
            snprintf (final_name,length,
                      "%s/%s/%s", dir_system, weechat_plugin->name, filename);
            if ((stat (final_name, &st) == 0) && (st.st_size > 0))
                return final_name;
            free (final_name);
        }
    }
    
    return strdup (filename);
}

/*
 * script_add: add a script to list of scripts
 */

struct t_plugin_script *
script_add (struct t_weechat_plugin *weechat_plugin,
            struct t_plugin_script **scripts,
            char *filename, char *name, char *author, char *version,
            char *license, char *description, char *shutdown_func,
            char *charset)
{
    struct t_plugin_script *new_script;
    
    if (strchr (name, ' '))
    {
        weechat_printf (NULL,
                        _("%s: error loading script \"%s\" (bad name, spaces "
                          "are forbidden)"),
                        weechat_plugin->name, name);
        return NULL;
    }
    
    if (script_option_check_license
        && (weechat_strcmp_ignore_chars (weechat_plugin->license, license,
                                         "0123456789-.,/\\()[]{}", 0) != 0))
    {
        weechat_printf (NULL,
                        _("%s%s: warning, license \"%s\" for script \"%s\" "
                          "differs from plugin license (\"%s\")"),
                        weechat_prefix ("error"), weechat_plugin->name,
                        license, name, weechat_plugin->license);
    }
    
    new_script = malloc (sizeof (*new_script));
    if (new_script)
    {
        new_script->filename = strdup (filename);
        new_script->interpreter = NULL;
        new_script->name = strdup (name);
        new_script->author = strdup (author);
        new_script->version = strdup (version);
        new_script->license = strdup (license);
        new_script->description = strdup (description);
        new_script->shutdown_func = (shutdown_func) ?
            strdup (shutdown_func) : NULL;
        new_script->charset = (charset) ? strdup (charset) : NULL;
        
        new_script->callbacks = NULL;
        
        /* add new script to list */
        if (*scripts)
            (*scripts)->prev_script = new_script;
        new_script->prev_script = NULL;
        new_script->next_script = *scripts;
        *scripts = new_script;
        
        return new_script;
    }
    
    weechat_printf (NULL,
                    _("%s: error loading script \"%s\" (not enough memory)"),
                    weechat_plugin->name, name);
    
    return NULL;
}

/*
 * script_remove: remove a script from list of scripts
 */

void
script_remove (struct t_weechat_plugin *weechat_plugin,
               struct t_plugin_script **scripts,
               struct t_plugin_script *script)
{
    struct t_script_callback *ptr_script_callback;
    
    for (ptr_script_callback = script->callbacks; ptr_script_callback;
         ptr_script_callback = ptr_script_callback->next_callback)
    {
        /* unhook */
        if (ptr_script_callback->hook)
        {
            weechat_unhook (ptr_script_callback->hook);
        }
        
        /* free config file */
        if (ptr_script_callback->config_file
            && !ptr_script_callback->config_section
            && !ptr_script_callback->config_option)
        {
            if (weechat_config_boolean (weechat_config_get ("weechat.plugin.save_config_on_unload")))
                weechat_config_write (ptr_script_callback->config_file);
            weechat_config_free (ptr_script_callback->config_file);
        }
        
        /* remove bar item */
        if (ptr_script_callback->bar_item)
            weechat_bar_item_remove (ptr_script_callback->bar_item);
    }
    
    /* remove all callbacks created by this script */
    script_callback_remove_all (script);
    
    /* free data */
    if (script->filename)
        free (script->filename);
    if (script->name)
        free (script->name);
    if (script->description)
        free (script->description);
    if (script->version)
        free (script->version);
    if (script->shutdown_func)
        free (script->shutdown_func);
    if (script->charset)
        free (script->charset);
    
    /* remove script from list */
    if (script->prev_script)
        (script->prev_script)->next_script = script->next_script;
    else
        *scripts = script->next_script;
    if (script->next_script)
        (script->next_script)->prev_script = script->prev_script;
    
    /* free script */
    free (script);
}

/*
 * script_completion: complete with list of scripts
 */

void
script_completion (struct t_weechat_plugin *weechat_plugin,
                   struct t_weelist *list,
                   struct t_plugin_script *scripts)
{
    struct t_plugin_script *ptr_script;

    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        weechat_list_add (list, ptr_script->name, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * script_display_list: print list of scripts
 */

void
script_display_list (struct t_weechat_plugin *weechat_plugin,
                     struct t_plugin_script *scripts,
                     char *name, int full)
{
    struct t_plugin_script *ptr_script;
    
    weechat_printf (NULL, "");
    weechat_printf (NULL,
                    /* TRANSLATORS: %s is language (for example "perl") */
                    _("%s scripts loaded:"),
                    weechat_plugin->name);
    if (scripts)
    {
        for (ptr_script = scripts; ptr_script;
             ptr_script = ptr_script->next_script)
        {
            if (!name || (weechat_strcasestr (ptr_script->name, name)))
            {
                weechat_printf (NULL,
                                "  %s%s%s v%s - %s",
                                weechat_color ("chat_buffer"),
                                ptr_script->name,
                                weechat_color ("chat"),
                                ptr_script->version,
                                ptr_script->description);
                if (full)
                {
                    weechat_printf (NULL,
                                    _("    file: %s"),
                                    ptr_script->filename);
                    weechat_printf (NULL,
                                    _("    written by \"%s\", license: %s"),
                                    ptr_script->author,
                                    ptr_script->license);
                }
            }
        }
    }
    else
        weechat_printf (NULL, _("  (none)"));
}

/*
 * script_print_log: print script infos in log (usually for crash dump)
 */

void
script_print_log (struct t_weechat_plugin *weechat_plugin,
                  struct t_plugin_script *scripts)
{
    struct t_plugin_script *ptr_script;
    struct t_script_callback *ptr_script_callback;
    
    weechat_log_printf ("");
    weechat_log_printf ("***** \"%s\" plugin dump *****",
                        weechat_plugin->name);
    
    for (ptr_script = scripts; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[script %s (addr:0x%x)]",      ptr_script->name, ptr_script);
        weechat_log_printf ("  filename. . . . . . : '%s'", ptr_script->filename);
        weechat_log_printf ("  interpreter . . . . : 0x%x", ptr_script->interpreter);
        weechat_log_printf ("  name. . . . . . . . : '%s'", ptr_script->name);
        weechat_log_printf ("  author. . . . . . . : '%s'", ptr_script->author);
        weechat_log_printf ("  version . . . . . . : '%s'", ptr_script->version);
        weechat_log_printf ("  license . . . . . . : '%s'", ptr_script->license);
        weechat_log_printf ("  description . . . . : '%s'", ptr_script->description);
        weechat_log_printf ("  shutdown_func . . . : '%s'", ptr_script->shutdown_func);
        weechat_log_printf ("  charset . . . . . . : '%s'", ptr_script->charset);
        weechat_log_printf ("  callbacks . . . . . : 0x%x", ptr_script->callbacks);
        weechat_log_printf ("  prev_script . . . . : 0x%x", ptr_script->prev_script);
        weechat_log_printf ("  next_script . . . . : 0x%x", ptr_script->next_script);

        for (ptr_script_callback = ptr_script->callbacks; ptr_script_callback;
             ptr_script_callback = ptr_script_callback->next_callback)
        {
            script_callback_print_log (weechat_plugin, ptr_script_callback);
        }
    }
    
    weechat_log_printf ("");
    weechat_log_printf ("***** End of \"%s\" plugin dump *****",
                        weechat_plugin->name);
}
