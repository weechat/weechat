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

/* gui-completion.c: completes words according to context */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hook.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../core/wee-list.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-config.h"
#include "gui-completion.h"
#include "gui-bar.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-keyboard.h"
#include "gui-nicklist.h"


/*
 * gui_completion_init: init completion
 */

void
gui_completion_init (struct t_gui_completion *completion,
                     struct t_gui_buffer *buffer)
{
    completion->buffer = buffer;
    completion->context = GUI_COMPLETION_NULL;
    completion->base_command = NULL;
    completion->base_command_arg = 0;
    completion->arg_is_nick = 0;
    completion->base_word = NULL;
    completion->base_word_pos = 0;
    completion->position = -1; 
    completion->args = NULL;
    completion->direction = 0;
    completion->add_space = 1;
    
    completion->completion_list = weelist_new ();
    
    completion->word_found = NULL;
    completion->position_replace = 0;
    completion->diff_size = 0;
    completion->diff_length = 0;
}

/*
 * gui_completion_free_data: free data in completion
 */

void
gui_completion_free_data (struct t_gui_completion *completion)
{
    if (completion->base_command)
        free (completion->base_command);
    completion->base_command = NULL;
    
    if (completion->base_word)
        free (completion->base_word);
    completion->base_word = NULL;
    
    if (completion->args)
        free (completion->args);
    completion->args = NULL;

    if (completion->completion_list)
    {
        weelist_free (completion->completion_list);
        completion->completion_list = NULL;
    }
    
    if (completion->word_found)
        free (completion->word_found);
    completion->word_found = NULL;
}

/*
 * gui_completion_free: free completion
 */

void
gui_completion_free (struct t_gui_completion *completion)
{
    gui_completion_free_data (completion);
    free (completion);
}

/*
 * gui_completion_stop: stop completion (for example after 1 arg of command
 *                      with 1 arg)
 */

void
gui_completion_stop (struct t_gui_completion *completion)
{
    completion->context = GUI_COMPLETION_NULL;
    completion->position = -1;
}

/*
 * gui_completion_search_command: search command hook
 */

struct t_hook *
gui_completion_search_command (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0]
            && (HOOK_COMMAND(ptr_hook, level) == 0)
            && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command),
                                   completion->base_command) == 0))
            return ptr_hook;
    }
    
    /* command not found */
    return NULL;
}

/*
 * gui_completion_is_only_alphanum: return 1 if there is only alpha/num chars
 *                                  in a string
 */

int
gui_completion_is_only_alphanum (char *string)
{
    while (string[0])
    {
        if (strchr (CONFIG_STRING(config_look_nick_completion_ignore),
                    string[0]))
            return 0;
        string++;
    }
    return 1;
}

/*
 * gui_completion_strdup_alphanum: duplicate alpha/num chars in a string
 */

char *
gui_completion_strdup_alphanum (char *string)
{
    char *result, *pos;
    
    result = malloc (strlen (string) + 1);
    pos = result;
    while (string[0])
    {
        if (!strchr (CONFIG_STRING(config_look_nick_completion_ignore),
                     string[0]))
        {
            pos[0] = string[0];
            pos++;
        }
        string++;
    }
    pos[0] = '\0';
    return result;
}

/*
 * gui_completion_nickncmp: locale and case independent string comparison
 *                          with max length for nicks (alpha or digits only)
 */

int
gui_completion_nickncmp (char *base_word, char *nick, int max)
{
    char *base_word2, *nick2;
    int return_cmp;
    
    if (!CONFIG_STRING(config_look_nick_completion_ignore)
        || !CONFIG_STRING(config_look_nick_completion_ignore)[0]
        || !base_word || !nick || !base_word[0] || !nick[0]
        || (!gui_completion_is_only_alphanum (base_word)))
        return string_strncasecmp (base_word, nick, max);
    
    base_word2 = gui_completion_strdup_alphanum (base_word);
    nick2 = gui_completion_strdup_alphanum (nick);
    
    return_cmp = string_strncasecmp (base_word2, nick2, strlen (base_word2));
    
    free (base_word2);
    free (nick2);
    
    return return_cmp;
}

/*
 * gui_completion_list_add: add a word to completion word list
 */

void
gui_completion_list_add (struct t_gui_completion *completion, char *word,
                         int nick_completion, char *where)
{
    if (!word || !word[0])
        return;
    
    if (!completion->base_word || !completion->base_word[0]
        || (nick_completion && (gui_completion_nickncmp (completion->base_word, word,
                                                         strlen (completion->base_word)) == 0))
        || (!nick_completion && (string_strncasecmp (completion->base_word, word,
                                                     strlen (completion->base_word)) == 0)))
    {
        weelist_add (completion->completion_list, word, where);
    }
}

/*
 * gui_completion_list_add_bars_names: add buffers names to completion list
 */

void
gui_completion_list_add_bars_names (struct t_gui_completion *completion)
{
    struct t_gui_bar *ptr_bar;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        gui_completion_list_add (completion, ptr_bar->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_buffers_names: add buffers names to completion list
 */

void
gui_completion_list_add_buffers_names (struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_completion_list_add (completion, ptr_buffer->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_buffers_categories_names: add buffers categories
 *                                                   and names to completion
 *                                                   list
 */

void
gui_completion_list_add_buffers_categories_names (struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;
    char name[256];
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        snprintf (name, sizeof (name), "%s.%s",
                  ptr_buffer->category, ptr_buffer->name);
        gui_completion_list_add (completion, name, 0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_buffers_categories: add buffers categories to
 *                                             completion list
 */

void
gui_completion_list_add_buffers_categories (struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_completion_list_add (completion, ptr_buffer->category,
                                 0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_config_files: add config files to completion list
 */

void
gui_completion_list_add_config_files (struct t_gui_completion *completion)
{
    struct t_config_file *ptr_config_file;

    for (ptr_config_file = config_files; ptr_config_file;
         ptr_config_file = ptr_config_file->next_config)
    {
        gui_completion_list_add (completion, ptr_config_file->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_filename: add filename to completion list
 */

void
gui_completion_list_add_filename (struct t_gui_completion *completion)
{
    char *path_d, *path_b, *p, *d_name;
    char *real_prefix, *prefix;
    char *buffer;
    int buffer_len;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char home[3] = { '~', DIR_SEPARATOR_CHAR, '\0' };
    
    buffer_len = PATH_MAX;
    buffer = malloc (buffer_len);
    if (!buffer)
	return;
    
    completion->add_space = 0;
    
    if ((strncmp (completion->base_word, home, 2) == 0) && getenv("HOME"))
    {
	real_prefix = strdup (getenv("HOME"));
	prefix = strdup (home);
    }
    else if ((strncmp (completion->base_word, DIR_SEPARATOR, 1) != 0)
	     || (strcmp (completion->base_word, "") == 0))
    {
	real_prefix = strdup (weechat_home);
	prefix = strdup ("");
    }
    else
    {
	real_prefix = strdup (DIR_SEPARATOR);
	prefix = strdup (DIR_SEPARATOR);
    }
	
    snprintf (buffer, buffer_len, "%s", completion->base_word + strlen (prefix));
    p = strrchr (buffer, DIR_SEPARATOR_CHAR);
    if (p)
    {
	*p = '\0';
	path_d = strdup (buffer);
	p++;
	path_b = strdup (p);
    }
    else {
	path_d = strdup ("");
	path_b = strdup (buffer);
    }
    
    sprintf (buffer, "%s%s%s", real_prefix, DIR_SEPARATOR, path_d);
    d_name = strdup (buffer);
    dp = opendir(d_name);
    if (dp != NULL)
    {
	while((entry = readdir(dp)) != NULL) 
	{	
	    if (strncmp (entry->d_name, path_b, strlen(path_b)) == 0) {
		
		if (strcmp (entry->d_name, ".") == 0 || strcmp (entry->d_name, "..") == 0)
		    continue;
		
		snprintf(buffer, buffer_len, "%s%s%s", 
			 d_name, DIR_SEPARATOR, entry->d_name);
		if (stat(buffer, &statbuf) == -1)
		    continue;
		
		snprintf(buffer, buffer_len, "%s%s%s%s%s%s",
			 prefix, 
			 ((strcmp(prefix, "") == 0)
			  || strchr(prefix, DIR_SEPARATOR_CHAR)) ? "" : DIR_SEPARATOR, 
			 path_d, 
			 strcmp(path_d, "") == 0 ? "" : DIR_SEPARATOR, 
			 entry->d_name, 
			 S_ISDIR(statbuf.st_mode) ? DIR_SEPARATOR : "");
		
		gui_completion_list_add (completion, buffer,
                                         0, WEECHAT_LIST_POS_SORT);
	    }
	}
    }
    
    free (d_name);
    free (prefix);
    free (real_prefix);
    free (path_d);
    free (path_b);
    free (buffer);
}

/*
 * gui_completion_list_add_command_hooks: add command hooks to completion list
 */

void
gui_completion_list_add_command_hooks (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (HOOK_COMMAND(ptr_hook, command))
            && (HOOK_COMMAND(ptr_hook, command)[0]))
            gui_completion_list_add (completion,
                                     HOOK_COMMAND(ptr_hook, command),
                                     0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_key_cmd: add key commands/functions to completion
 *                                  list
 */

void
gui_completion_list_add_key_cmd (struct t_gui_completion *completion)
{
    int i;
    
    for (i = 0; gui_key_functions[i].function_name; i++)
    {
        gui_completion_list_add (completion, gui_key_functions[i].function_name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_self_nick: add self nick on server to completion list
 */

void
gui_completion_list_add_self_nick (struct t_gui_completion *completion)
{
    if (completion->buffer->input_nick)
        gui_completion_list_add (completion,
                                 completion->buffer->input_nick,
                                 0, WEECHAT_LIST_POS_SORT);
}

/*
 * gui_completion_list_add_nicks: add nicks to completion list
 */

void
gui_completion_list_add_nicks (struct t_gui_completion *completion)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    int count_before;
    
    count_before = weelist_size (completion->completion_list);
    hook_completion_exec (completion->buffer->plugin,
                          "nick",
                          completion->buffer,
                          completion->completion_list);
    if (weelist_size (completion->completion_list) == count_before)
    {
        /* no plugin overrides nick completion, then we use default nick */
        /* completion, wich nicks of nicklist, in order of nicklist */
        ptr_group = NULL;
        ptr_nick = NULL;
        gui_nicklist_get_next_item (completion->buffer,
                                    &ptr_group, &ptr_nick);
        while (ptr_group || ptr_nick)
        {
            if (ptr_nick && ptr_nick->visible)
            {
                gui_completion_list_add (completion,
                                         ptr_nick->name,
                                         1, WEECHAT_LIST_POS_END);
            }
            gui_nicklist_get_next_item (completion->buffer,
                                        &ptr_group, &ptr_nick);
        }
    }
    
    completion->arg_is_nick = 1;
}

/*
 * gui_completion_list_add_option: add config option to completion list
 */

void
gui_completion_list_add_option (struct t_gui_completion *completion)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    int length;
    char *option_full_name;
    
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        for (ptr_section = ptr_config->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                length = strlen (ptr_config->name) + 1
                    + strlen (ptr_section->name) + 1
                    + strlen (ptr_option->name) + 1;
                option_full_name = malloc (length);
                if (option_full_name)
                {
                    snprintf (option_full_name, length, "%s.%s.%s",
                              ptr_config->name, ptr_section->name,
                              ptr_option->name);
                    gui_completion_list_add (completion,
                                             option_full_name,
                                             0, WEECHAT_LIST_POS_SORT);
                    free (option_full_name);
                }
            }
        }
    }
}

/*
 * gui_completion_list_add_plugin: add plugin name to completion list
 */

void
gui_completion_list_add_plugin (struct t_gui_completion *completion)
{
    struct t_weechat_plugin *ptr_plugin;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        gui_completion_list_add (completion, ptr_plugin->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
}

/*
 * gui_completion_list_add_plugin_commands: add plugin commands to completion
 *                                          list (plugin name is previous
 *                                          argument)
 */

void
gui_completion_list_add_plugin_commands (struct t_gui_completion *completion)
{
    char *pos_space, *plugin_name;
    struct t_weechat_plugin *ptr_plugin;
    struct t_hook *ptr_hook;
    
    if (completion->args)
    {
        pos_space = strchr (completion->args, ' ');
        if (pos_space)
            plugin_name = string_strndup (completion->args,
                                          pos_space - completion->args);
        else
            plugin_name = strdup (completion->args);
        
        if (plugin_name)
        {
            ptr_plugin = NULL;
            if (string_strcasecmp (plugin_name, "weechat") != 0)
            {
                /* plugin name is different from "weechat", then search it in
                   plugin list */
                ptr_plugin = plugin_search (plugin_name);
                if (!ptr_plugin)
                    return;
            }
            for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
                 ptr_hook = ptr_hook->next_hook)
            {
                if (!ptr_hook->deleted
                    && (ptr_hook->plugin == ptr_plugin)
                    && HOOK_COMMAND(ptr_hook, command)
                    && HOOK_COMMAND(ptr_hook, command)[0])
                {
                    gui_completion_list_add (completion,
                                             HOOK_COMMAND(ptr_hook, command),
                                             0, WEECHAT_LIST_POS_SORT);
                }
            }
            free (plugin_name);
        }
    }
}

/*
 * gui_completion_list_add_option_value: add option value to completion list
 */

void
gui_completion_list_add_option_value (struct t_gui_completion *completion)
{
    char *pos_space, *option_full_name, *color_name, *pos_section, *pos_option;
    char *file, *section, *value_string;
    int length, i, num_colors;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section, *section_found;
    struct t_config_option *option_found;
    
    if (completion->args)
    {
        pos_space = strchr (completion->args, ' ');
        if (pos_space)
            option_full_name = string_strndup (completion->args,
                                               pos_space - completion->args);
        else
            option_full_name = strdup (completion->args);
        
        if (option_full_name)
        {
            file = NULL;
            section = NULL;
            pos_option = NULL;
            
            pos_section = strchr (option_full_name, '.');
            pos_option = (pos_section) ? strchr (pos_section + 1, '.') : NULL;
            
            if (pos_section && pos_option)
            {
                file = string_strndup (option_full_name,
                                       pos_section - option_full_name);
                section = string_strndup (pos_section + 1,
                                          pos_option - pos_section - 1);
                pos_option++;
            }
            if (file && section && pos_option)
            {
                ptr_config = config_file_search (file);
                if (ptr_config)
                {
                    ptr_section = config_file_search_section (ptr_config,
                                                              section);
                    if (ptr_section)
                    {
                        config_file_search_section_option (ptr_config,
                                                           ptr_section,
                                                           pos_option,
                                                           &section_found,
                                                           &option_found);
                        if (option_found)
                        {
                            switch (option_found->type)
                            {
                                case CONFIG_OPTION_TYPE_BOOLEAN:
                                    gui_completion_list_add (completion, "on",
                                                             0, WEECHAT_LIST_POS_SORT);
                                    gui_completion_list_add (completion, "off",
                                                             0, WEECHAT_LIST_POS_SORT);
                                    gui_completion_list_add (completion, "toggle",
                                                             0, WEECHAT_LIST_POS_END);
                                    if (CONFIG_BOOLEAN(option_found) == CONFIG_BOOLEAN_TRUE)
                                        gui_completion_list_add (completion, "on",
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                    else
                                        gui_completion_list_add (completion, "off",
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                    break;
                                case CONFIG_OPTION_TYPE_INTEGER:
                                    length = 64;
                                    value_string = malloc (length);
                                    if (value_string)
                                    {
                                        if (option_found->string_values)
                                        {
                                            for (i = 0; option_found->string_values[i]; i++)
                                            {
                                                gui_completion_list_add (completion,
                                                                         option_found->string_values[i],
                                                                         0, WEECHAT_LIST_POS_SORT);
                                            }
                                            gui_completion_list_add (completion, "++1",
                                                                     0, WEECHAT_LIST_POS_END);
                                            gui_completion_list_add (completion, "--1",
                                                                     0, WEECHAT_LIST_POS_END);
                                            snprintf (value_string, length,
                                                      "%s",
                                                      option_found->string_values[CONFIG_INTEGER(option_found)]);
                                        }
                                        else
                                        {
                                            if (CONFIG_INTEGER(option_found) > option_found->min)
                                                gui_completion_list_add (completion, "--1",
                                                                         0, WEECHAT_LIST_POS_BEGINNING);
                                            if (CONFIG_INTEGER(option_found) < option_found->max)
                                                gui_completion_list_add (completion, "++1",
                                                                         0, WEECHAT_LIST_POS_BEGINNING);
                                            snprintf (value_string, length,
                                                      "%d", CONFIG_INTEGER(option_found));
                                        }
                                        gui_completion_list_add (completion,
                                                                 value_string,
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                        free (value_string);
                                    }
                                    break;
                                case CONFIG_OPTION_TYPE_STRING:
                                    gui_completion_list_add (completion,
                                                             "\"\"",
                                                             0, WEECHAT_LIST_POS_BEGINNING);
                                    length = strlen (CONFIG_STRING(option_found)) + 2 + 1;
                                    value_string = malloc (length);
                                    if (value_string)
                                    {
                                        snprintf (value_string, length,
                                                  "\"%s\"",
                                                  CONFIG_STRING(option_found));
                                        gui_completion_list_add (completion,
                                                                 value_string,
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                        free (value_string);
                                    }
                                    break;
                                case CONFIG_OPTION_TYPE_COLOR:
                                    num_colors = gui_color_get_number ();
                                    for (i = 0; i < num_colors; i++)
                                    {
                                        color_name = gui_color_get_name (i);
                                        if (color_name)
                                            gui_completion_list_add (completion,
                                                                     color_name,
                                                                     0, WEECHAT_LIST_POS_SORT);
                                    }
                                    gui_completion_list_add (completion, "++1",
                                                             0, WEECHAT_LIST_POS_END);
                                    gui_completion_list_add (completion, "--1",
                                                             0, WEECHAT_LIST_POS_END);
                                    color_name = gui_color_get_name (CONFIG_INTEGER(option_found));
                                    if (color_name)
                                    {
                                        gui_completion_list_add (completion,
                                                                 color_name,
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                    }
                                    break;
                                case CONFIG_NUM_OPTION_TYPES:
                                    break;
                            }
                        }
                    }
                }
            }
            if (file)
                free (file);
            if (section)
                free (section);
        }
    }
}

/*
 * gui_completion_list_add_weechat_cmd: add WeeChat commands to completion list
 */

void
gui_completion_list_add_weechat_cmd (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && !ptr_hook->plugin
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0])
        {
            gui_completion_list_add (completion,
                                     HOOK_COMMAND(ptr_hook, command),
                                     0, WEECHAT_LIST_POS_SORT);
        }
    }
}

/*
 * gui_completion_custom: custom completion by a plugin
 */

void
gui_completion_custom (struct t_gui_completion *completion,
                       char *custom_completion,
                       struct t_weechat_plugin *plugin)
{
    hook_completion_exec (plugin,
                          custom_completion,
                          completion->buffer,
                          completion->completion_list);
}

/*
 * gui_completion_build_list_template: build data list according to a template
 */

void
gui_completion_build_list_template (struct t_gui_completion *completion,
                                    char *template,
                                    struct t_weechat_plugin *plugin)
{
    char *word, *pos, *pos_end, *custom_completion;
    int word_offset;
    
    word = strdup (template);
    word_offset = 0;
    pos = template;
    while (pos)
    {
        switch (pos[0])
        {
            case '\0':
            case ' ':
            case '|':
                if (word_offset > 0)
                {
                    word[word_offset] = '\0';
                    weelist_add (completion->completion_list,
                                 word, WEECHAT_LIST_POS_SORT);
                }
                word_offset = 0;
                break;
            case '%':
                pos++;
                if (pos && pos[0])
                {
                    switch (pos[0])
                    {
                        case '-': /* stop completion */
                            gui_completion_stop (completion);
                            free (word);
                            return;
                            break;
                        case '*': /* repeat last completion (do nothing there) */
                            break;
                        case 'b': /* buffers names */
                            gui_completion_list_add_buffers_names (completion);
                            break;
                        case 'B': /* buffers categories + names */
                            gui_completion_list_add_buffers_categories_names (completion);
                            break;
                        case 'c': /* buffers categories */
                            gui_completion_list_add_buffers_categories (completion);
                            break;
                        case 'C': /* config files */
                            gui_completion_list_add_config_files (completion);
                            break;
                        case 'f': /* filename */
                            gui_completion_list_add_filename (completion);
                            break;
                        case 'h': /* command hooks */
                            gui_completion_list_add_command_hooks (completion);
                            break;
                        case 'k': /* key cmd/funtcions*/
                            gui_completion_list_add_key_cmd (completion);
                            break;
                        case 'm': /* self nickname */
                            gui_completion_list_add_self_nick (completion);
                            break;
                        case 'n': /* nick */
                            gui_completion_list_add_nicks (completion);
                            break;
                        case 'o': /* config option */
                            gui_completion_list_add_option (completion);
                            break;
                        case 'p': /* plugin name */
                            gui_completion_list_add_plugin (completion);
                            break;
                        case 'P': /* plugin commands */
                            gui_completion_list_add_plugin_commands (completion);
                            break;
                        case 'r': /* bar names */
                            gui_completion_list_add_bars_names (completion);
                            break;
                        case 'v': /* value of config option */
                            gui_completion_list_add_option_value (completion);
                            break;
                        case 'w': /* WeeChat commands */
                            gui_completion_list_add_weechat_cmd (completion);
                            break;
                        case '(': /* custom completion by a plugin */
                            pos++;
                            pos_end = strchr (pos, ')');
                            if (pos_end)
                            {
                                if (pos_end > pos)
                                {
                                    custom_completion = string_strndup (pos,
                                                                        pos_end - pos);
                                    if (custom_completion)
                                    {
                                        gui_completion_custom (completion,
                                                               custom_completion,
                                                               plugin);
                                        free (custom_completion);
                                    }
                                }
                                pos = pos_end + 1;
                            }
                    }
                }
                break;
            default:
                word[word_offset++] = pos[0];
        }
        /* end of argument in template? */
        if (!pos[0] || (pos[0] == ' '))
            pos = NULL;
        else
            pos++;
    }
    free (word);
}

/*
 * gui_completion_build_list: build data list according to command and argument #
 */

void
gui_completion_build_list (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    char *pos_template, *pos_space;
    int repeat_last, i, length;

    repeat_last = 0;
    
    ptr_hook = gui_completion_search_command (completion);
    if (!ptr_hook || !HOOK_COMMAND(ptr_hook, completion)
        || (strcmp (HOOK_COMMAND(ptr_hook, completion), "-") == 0))
    {
        gui_completion_stop (completion);
        return;
    }
    
    length = strlen (HOOK_COMMAND(ptr_hook, completion));
    if (length >= 2)
    {
        if (strcmp (HOOK_COMMAND(ptr_hook, completion) + length - 2,
                    "%*") == 0)
            repeat_last = 1;
    }
    
    i = 1;
    pos_template = HOOK_COMMAND(ptr_hook, completion);
    while (pos_template && pos_template[0])
    {
        pos_space = strchr (pos_template, ' ');
        if (i == completion->base_command_arg)
        {
            gui_completion_build_list_template (completion, pos_template,
                                                ptr_hook->plugin);
            return;
        }
        if (pos_space)
        {
            pos_template = pos_space;
            while (pos_template[0] == ' ')
                pos_template++;
        }
        else
            pos_template = NULL;
        i++;
    }
    if (repeat_last)
    {
        pos_space = rindex (HOOK_COMMAND(ptr_hook, completion), ' ');
        gui_completion_build_list_template (completion,
                                            (pos_space) ?
                                            pos_space + 1 : HOOK_COMMAND(ptr_hook,
                                                                         completion),
                                            ptr_hook->plugin);
    }
}

/*
 * gui_completion_find_context: find context for completion
 */

void
gui_completion_find_context (struct t_gui_completion *completion, char *data,
                             int size, int pos)
{
    int i, command, command_arg, pos_start, pos_end;
    
    /* look for context */
    gui_completion_free_data (completion);
    gui_completion_init (completion, completion->buffer);
    command = ((data[0] == '/') && (data[1] != '/')) ? 1 : 0;
    command_arg = 0;
    i = 0;
    while (i < pos)
    {
        if (data[i] == ' ')
        {
            command_arg++;
            i++;
            while ((i < pos) && (data[i] == ' ')) i++;
            if (!completion->args)
                completion->args = strdup (data + i);
        }
        else
            i++;
    }
    if (command)
    {
        if (command_arg > 0)
        {
            completion->context = GUI_COMPLETION_COMMAND_ARG;
            completion->base_command_arg = command_arg;
        }
        else
        {
            completion->context = GUI_COMPLETION_COMMAND;
            completion->base_command_arg = 0;
        }
    }
    else
        completion->context = GUI_COMPLETION_AUTO;
    
    /* look for word to complete (base word) */
    completion->base_word_pos = 0;
    completion->position_replace = pos;
    
    if (size > 0)
    {
        i = pos;
        pos_start = i;
        if (data[i] == ' ')
        {
            if ((i > 0) && (data[i-1] != ' '))
            {
                i--;
                while ((i >= 0) && (data[i] != ' '))
                    i--;
                pos_start = i + 1;
            }
        }
        else
        {
            while ((i >= 0) && (data[i] != ' '))
                i--;
            pos_start = i + 1;
        }
        i = pos;
        while ((i < size) && (data[i] != ' '))
            i++;
        pos_end = i - 1;

        if (completion->context == GUI_COMPLETION_COMMAND)
            pos_start++;
        
        completion->base_word_pos = pos_start;
        
        if (pos_start <= pos_end)
        {
            completion->position_replace = pos_start;
            completion->base_word = malloc (pos_end - pos_start + 2);
            for (i = pos_start; i <= pos_end; i++)
                completion->base_word[i - pos_start] = data[i];
            completion->base_word[pos_end - pos_start + 1] = '\0';
        }
    }
    
    if (!completion->base_word)
        completion->base_word = strdup ("");
    
    /* find command (for command argument completion only) */
    if (completion->context == GUI_COMPLETION_COMMAND_ARG)
    {
        pos_start = 0;
        while ((pos_start < size) && (data[pos_start] != '/'))
            pos_start++;
        if (data[pos_start] == '/')
        {
            pos_start++;
            pos_end = pos_start;
            while ((pos_end < size) && (data[pos_end] != ' '))
                pos_end++;
            if (data[pos_end] == ' ')
                pos_end--;
            
            completion->base_command = malloc (pos_end - pos_start + 2);
            for (i = pos_start; i <= pos_end; i++)
                completion->base_command[i - pos_start] = data[i];
            completion->base_command[pos_end - pos_start + 1] = '\0';
            gui_completion_build_list (completion);
        }
    }
    
    /* auto completion with nothing as base word is disabled,
       in order to prevent completion when pasting messages with [tab] inside */
    if ((completion->context == GUI_COMPLETION_AUTO)
        && ((!completion->base_word) || (!completion->base_word[0])))
    {
        completion->context = GUI_COMPLETION_NULL;
        return;
    }
}

/*
 * gui_completion_command: complete a command
 */

void
gui_completion_command (struct t_gui_completion *completion)
{
    int length, word_found_seen, other_completion;
    struct t_hook *ptr_hook;
    struct t_weelist_item *ptr_item, *ptr_item2;
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    if (!completion->completion_list->items)
    {
        for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted
                && HOOK_COMMAND(ptr_hook, command)
                && HOOK_COMMAND(ptr_hook, command)[0]
                && (HOOK_COMMAND(ptr_hook, level) == 0))
            {
                gui_completion_list_add (completion,
                                         HOOK_COMMAND(ptr_hook, command),
                                         0, WEECHAT_LIST_POS_SORT);
            }
        }
    }
    if (completion->direction < 0)
        ptr_item = completion->completion_list->last_item;
    else
        ptr_item = completion->completion_list->items;
    
    while (ptr_item)
    {
        if (string_strncasecmp (ptr_item->data, completion->base_word, length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_item->data);
                
                if (completion->direction < 0)
                    ptr_item2 = ptr_item->prev_item;
                else
                    ptr_item2 = ptr_item->next_item;
                
                while (ptr_item2)
                {
                    if (string_strncasecmp (ptr_item2->data,
                                            completion->base_word, length) == 0)
                        other_completion++;
                    
                    if (completion->direction < 0)
                        ptr_item2 = ptr_item2->prev_item;
                    else
                        ptr_item2 = ptr_item2->next_item;
                }
                
                if (other_completion == 0)
                    completion->position = -1;
                else
                    if (completion->position < 0)
                        completion->position = 0;
                return;
            }
            other_completion++;
        }
        if (completion->word_found &&
            (string_strcasecmp (ptr_item->data, completion->word_found) == 0))
            word_found_seen = 1;
        
        if (completion->direction < 0)
            ptr_item = ptr_item->prev_item;
        else
            ptr_item = ptr_item->next_item;
    }
    if (completion->word_found)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        gui_completion_command (completion);
    }
}

/*
 * gui_completion_command_arg: complete a command argument
 */

void
gui_completion_command_arg (struct t_gui_completion *completion,
                            int nick_completion)
{
    int length, word_found_seen, other_completion;
    struct t_weelist_item *ptr_item, *ptr_item2;
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    if (completion->direction < 0)
        ptr_item = completion->completion_list->last_item;
    else
        ptr_item = completion->completion_list->items;
    
    while (ptr_item)
    {
        if ((nick_completion
             && (gui_completion_nickncmp (completion->base_word, ptr_item->data,
                                          length) == 0))
            || ((!nick_completion)
                && (string_strncasecmp (completion->base_word, ptr_item->data,
                                        length) == 0)))
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_item->data);
                
                if (completion->direction < 0)
                    ptr_item2 = ptr_item->prev_item;
                else
                    ptr_item2 = ptr_item->next_item;
                
                while (ptr_item2)
                {
                    if ((nick_completion
                         && (gui_completion_nickncmp (completion->base_word,
                                                      ptr_item2->data,
                                                      length) == 0))
                        || ((!nick_completion)
                            && (string_strncasecmp (completion->base_word,
                                                    ptr_item2->data,
                                                    length) == 0)))
                        other_completion++;
                    
                    if (completion->direction < 0)
                        ptr_item2 = ptr_item2->prev_item;
                    else
                        ptr_item2 = ptr_item2->next_item;
                }
                
                if (other_completion == 0)
                    completion->position = -1;
                else
                    if (completion->position < 0)
                        completion->position = 0;
                return;
            }
            other_completion++;
        }
        if (completion->word_found &&
            (string_strcasecmp (ptr_item->data, completion->word_found) == 0))
            word_found_seen = 1;

        if (completion->direction < 0)
            ptr_item = ptr_item->prev_item;
        else
            ptr_item = ptr_item->next_item;
    }
    if (completion->word_found)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        gui_completion_command_arg (completion, nick_completion);
    }
}

/*
 * gui_completion_nick: complete a nick
 */

void
gui_completion_nick (struct t_gui_completion *completion)
{
    int length, word_found_seen, other_completion;
    struct t_weelist_item *ptr_item, *ptr_item2;
    
    if (!completion->completion_list->items)
        gui_completion_list_add_nicks (completion);
    
    completion->context = GUI_COMPLETION_NICK;
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    
    if (completion->direction < 0)
        ptr_item = completion->completion_list->last_item;
    else
        ptr_item = completion->completion_list->items;
    
    while (ptr_item)
    {
        if (gui_completion_nickncmp (completion->base_word, ptr_item->data,
                                     length) == 0)
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_item->data);
                if (CONFIG_BOOLEAN(config_look_nick_complete_first))
                {
                    completion->position = -1;
                    return;
                }
                
                if (completion->direction < 0)
                    ptr_item2 = ptr_item->prev_item;
                else
                    ptr_item2 = ptr_item->next_item;
                
                while (ptr_item2)
                {
                    if (gui_completion_nickncmp (completion->base_word,
                                                 ptr_item2->data,
                                                 length) == 0)
                        other_completion++;
                    
                    if (completion->direction < 0)
                        ptr_item2 = ptr_item2->prev_item;
                    else
                        ptr_item2 = ptr_item2->next_item;
                }
                
                if (other_completion == 0)
                    completion->position = -1;
                else
                {
                    if (completion->position < 0)
                        completion->position = 0;
                }
                return;
            }
            other_completion++;
        }
        if (completion->word_found &&
            (string_strcasecmp (ptr_item->data, completion->word_found) == 0))
            word_found_seen = 1;
        
        if (completion->direction < 0)
            ptr_item = ptr_item->prev_item;
        else
            ptr_item = ptr_item->next_item;
    }
    if (completion->word_found)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        gui_completion_nick (completion);
    }
}

/*
 * gui_completion_auto: auto complete: nick, filename or channel
 */

void
gui_completion_auto (struct t_gui_completion *completion)
{
    /* filename completion */
    if ((completion->base_word[0] == '/')
        || (completion->base_word[0] == '~'))
    {
        if (!completion->completion_list->items)
            gui_completion_list_add_filename (completion);
        gui_completion_command_arg (completion, 0);
        return;
    }
    
    /* default: nick completion (if there's a nicklist) */
    if (completion->buffer->nicklist_root)
        gui_completion_nick (completion);
    else
        completion->context = GUI_COMPLETION_NULL;
}

/*
 * gui_completion_search: complete word according to context
 */

void
gui_completion_search (struct t_gui_completion *completion, int direction,
                       char *data, int size, int pos)
{
    char *old_word_found;
    
    completion->direction = direction;
    
    /* if new completion => look for base word */
    if (pos != completion->position)
    {
        if (completion->word_found)
            free (completion->word_found);
        completion->word_found = NULL;
        gui_completion_find_context (completion, data, size, pos);
    }
    
    /* completion */
    old_word_found = (completion->word_found) ?
        strdup (completion->word_found) : NULL;
    switch (completion->context)
    {
        case GUI_COMPLETION_NULL:
            /* should never be executed */
            return;
        case GUI_COMPLETION_NICK:
            gui_completion_nick (completion);
            break;
        case GUI_COMPLETION_COMMAND:
            gui_completion_command (completion);
            break;
        case GUI_COMPLETION_COMMAND_ARG:
            if (completion->completion_list->items)
                gui_completion_command_arg (completion, completion->arg_is_nick);
            else
            {
                completion->context = GUI_COMPLETION_AUTO;
                gui_completion_auto (completion);
            }
            break;
        case GUI_COMPLETION_AUTO:
            gui_completion_auto (completion);
            break;
    }
    if (completion->word_found)
    {
        if (old_word_found)
        {
            completion->diff_size = strlen (completion->word_found) -
                strlen (old_word_found);
            completion->diff_length = utf8_strlen (completion->word_found) -
                utf8_strlen (old_word_found);
        }
        else
        {
            completion->diff_size = strlen (completion->word_found) -
                strlen (completion->base_word);
            completion->diff_length = utf8_strlen (completion->word_found) -
                utf8_strlen (completion->base_word);
        }
    }
    if (old_word_found)
        free (old_word_found);
}

/*
 * gui_completion_print_log: print completion list in log (usually for crash dump)
 */

void
gui_completion_print_log (struct t_gui_completion *completion)
{
    log_printf ("[completion (addr:0x%x)]", completion);
    log_printf ("  buffer . . . . . . . . : 0x%x", completion->buffer);
    log_printf ("  context. . . . . . . . : %d",   completion->context);
    log_printf ("  base_command . . . . . : '%s'", completion->base_command);
    log_printf ("  base_command_arg . . . : %d",   completion->base_command_arg);
    log_printf ("  arg_is_nick. . . . . . : %d",   completion->arg_is_nick);
    log_printf ("  base_word. . . . . . . : '%s'", completion->base_word);
    log_printf ("  base_word_pos. . . . . : %d",   completion->base_word_pos);
    log_printf ("  position . . . . . . . : %d",   completion->position);
    log_printf ("  args . . . . . . . . . : '%s'", completion->args);
    log_printf ("  direction. . . . . . . : %d",   completion->direction);
    log_printf ("  add_space. . . . . . . : %d",   completion->add_space);
    log_printf ("  completion_list. . . . : 0x%x", completion->completion_list);
    log_printf ("  word_found . . . . . . : '%s'", completion->word_found);
    log_printf ("  position_replace . . . : %d",   completion->position_replace);
    log_printf ("  diff_size. . . . . . . : %d",   completion->diff_size);
    log_printf ("  diff_length. . . . . . : %d",   completion->diff_length);
    if (completion->completion_list)
    {
        log_printf ("");
        weelist_print_log (completion->completion_list,
                           "completion list element");
    }
}
