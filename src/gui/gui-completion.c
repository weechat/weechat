/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
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

/* gui-completion.c: word completion according to context */


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
#include "../core/wee-list.h"
#include "../core/wee-log.h"
#include "../core/wee-proxy.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "../plugins/plugin-config.h"
#include "gui-completion.h"
#include "gui-bar.h"
#include "gui-buffer.h"
#include "gui-color.h"
#include "gui-filter.h"
#include "gui-keyboard.h"
#include "gui-nicklist.h"


/*
 * gui_completion_buffer_init: init completion for a buffer
 */

void
gui_completion_buffer_init (struct t_gui_completion *completion,
                            struct t_gui_buffer *buffer)
{
    completion->buffer = buffer;
    completion->context = GUI_COMPLETION_NULL;
    completion->base_command = NULL;
    completion->base_command_arg_index = 0;
    completion->base_word = NULL;
    completion->base_word_pos = 0;
    completion->position = -1; 
    completion->args = NULL;
    completion->direction = 0;
    completion->add_space = 1;
    completion->force_partial_completion = 0;
    
    completion->completion_list = weelist_new ();
    
    completion->word_found = NULL;
    completion->word_found_is_nick = 0;
    completion->position_replace = 0;
    completion->diff_size = 0;
    completion->diff_length = 0;
    
    completion->partial_completion_list = NULL;
    completion->last_partial_completion = NULL;
}

/*
 * gui_completion_partial_list_add: add an item to partial completions list
 */

struct t_gui_completion_partial *
gui_completion_partial_list_add (struct t_gui_completion *completion,
                                 const char *word, int count)
{
    struct t_gui_completion_partial *new_item;
    
    new_item = malloc (sizeof (*new_item));
    if (new_item)
    {
        new_item->word = strdup (word);
        new_item->count = count;
        
        new_item->prev_item = completion->last_partial_completion;
        if (completion->partial_completion_list)
            (completion->last_partial_completion)->next_item = new_item;
        else
            completion->partial_completion_list = new_item;
        completion->last_partial_completion = new_item;
        new_item->next_item = NULL;
    }
    return new_item;
}

/*
 * gui_completion_partial_free: remove an item from partial completions list
 */

void
gui_completion_partial_list_free (struct t_gui_completion *completion,
                                  struct t_gui_completion_partial *item)
{
    /* remove partial completion item from list */
    if (item->prev_item)
        (item->prev_item)->next_item = item->next_item;
    if (item->next_item)
        (item->next_item)->prev_item = item->prev_item;
    if (completion->partial_completion_list == item)
        completion->partial_completion_list = item->next_item;
    if (completion->last_partial_completion == item)
        completion->last_partial_completion = item->prev_item;
    
    /* free data */
    if (item->word)
        free (item->word);
    
    free (item);
}

/*
 * gui_completion_partial_free: remove partial completions list
 */

void
gui_completion_partial_list_free_all (struct t_gui_completion *completion)
{
    while (completion->partial_completion_list)
    {
        gui_completion_partial_list_free (completion,
                                          completion->partial_completion_list);
    }
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

    gui_completion_partial_list_free_all (completion);
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
gui_completion_stop (struct t_gui_completion *completion,
                     int remove_partial_completion_list)
{
    completion->context = GUI_COMPLETION_NULL;
    completion->position = -1;
    if (remove_partial_completion_list)
    {
        gui_completion_partial_list_free_all (completion);
        hook_signal_send ("partial_completion",
                          WEECHAT_HOOK_SIGNAL_STRING, NULL);
    }
}

/*
 * gui_completion_search_command: search command hook
 */

struct t_hook *
gui_completion_search_command (const char *command)
{
    struct t_hook *ptr_hook;
    
    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0]
            && (string_strcasecmp (HOOK_COMMAND(ptr_hook, command),
                                   command) == 0))
            return ptr_hook;
    }
    
    /* command not found */
    return NULL;
}

/*
 * gui_completion_nick_has_ignored_chars: return 1 if nick has one or more
 *                                        ignored chars for nick comparison
 */

int
gui_completion_nick_has_ignored_chars (const char *string)
{
    int char_size;
    char utf_char[16];
    
    while (string[0])
    {
        char_size = utf8_char_size (string);
        memcpy (utf_char, string, char_size);
        utf_char[char_size] = '\0';
        
        if (strstr (CONFIG_STRING(config_completion_nick_ignore_chars),
                    utf_char))
            return 1;
        
        string += char_size;
    }
    return 0;
}

/*
 * gui_completion_nick_strdup_ignore_chars: duplicate a nick and ignore some
 *                                          chars
 */

char *
gui_completion_nick_strdup_ignore_chars (const char *string)
{
    int char_size;
    char *result, *pos, utf_char[16];
    
    result = malloc (strlen (string) + 1);
    pos = result;
    while (string[0])
    {
        char_size = utf8_char_size (string);
        memcpy (utf_char, string, char_size);
        utf_char[char_size] = '\0';
        
        if (!strstr (CONFIG_STRING(config_completion_nick_ignore_chars),
                     utf_char))
        {
            memcpy (pos, utf_char, char_size);
            pos += char_size;
        }
        
        string += char_size;
    }
    pos[0] = '\0';
    return result;
}

/*
 * gui_completion_nickncmp: locale and case independent string comparison
 *                          with max length for nicks (alpha or digits only)
 */

int
gui_completion_nickncmp (const char *base_word, const char *nick, int max)
{
    char *base_word2, *nick2;
    int return_cmp;
    
    if (!CONFIG_STRING(config_completion_nick_ignore_chars)
        || !CONFIG_STRING(config_completion_nick_ignore_chars)[0]
        || !base_word || !nick || !base_word[0] || !nick[0]
        || gui_completion_nick_has_ignored_chars (base_word))
        return string_strncasecmp (base_word, nick, max);
    
    base_word2 = gui_completion_nick_strdup_ignore_chars (base_word);
    nick2 = gui_completion_nick_strdup_ignore_chars (nick);
    
    return_cmp = string_strncasecmp (base_word2, nick2,
                                     utf8_strlen (base_word2));
    
    free (base_word2);
    free (nick2);
    
    return return_cmp;
}

/*
 * gui_completion_list_add: add a word to completion word list
 */

void
gui_completion_list_add (struct t_gui_completion *completion, const char *word,
                         int nick_completion, const char *where)
{
    char buffer[512];
    
    if (!word || !word[0])
        return;
    
    if (!completion->base_word || !completion->base_word[0]
        || (nick_completion && (gui_completion_nickncmp (completion->base_word, word,
                                                         strlen (completion->base_word)) == 0))
        || (!nick_completion && (string_strncasecmp (completion->base_word, word,
                                                     strlen (completion->base_word)) == 0)))
    {
        if (nick_completion && (completion->base_word_pos == 0))
        {
            snprintf (buffer, sizeof (buffer), "%s%s",
                      word, CONFIG_STRING(config_completion_nick_completer));
            weelist_add (completion->completion_list, buffer, where,
                         (nick_completion) ? (void *)1 : (void *)0);
        }
        else
        {
            weelist_add (completion->completion_list, word, where,
                         (nick_completion) ? (void *)1 : (void *)0);
        }
    }
}

/*
 * gui_completion_list_add_bars_names_cb: add bars names to completion list
 */

int
gui_completion_list_add_bars_names_cb (void *data,
                                       const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    struct t_gui_bar *ptr_bar;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        gui_completion_list_add (completion, ptr_bar->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_bars_options: add bars options to completion list
 */

int
gui_completion_list_add_bars_options_cb (void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion)
{
    int i;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        gui_completion_list_add (completion, gui_bar_option_string[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_buffers_names_cb: add buffers names to completion
 *                                           list
 */

int
gui_completion_list_add_buffers_names_cb (void *data,
                                          const char *completion_item,
                                          struct t_gui_buffer *buffer,
                                          struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        gui_completion_list_add (completion, ptr_buffer->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }

    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_buffers_numbers_cb: add buffers numbers to
 *                                             completion list
 */

int
gui_completion_list_add_buffers_numbers_cb (void *data,
                                            const char *completion_item,
                                            struct t_gui_buffer *buffer,
                                            struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;
    char str_number[32];
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        snprintf (str_number, sizeof (str_number), "%d", ptr_buffer->number);
        gui_completion_list_add (completion, str_number,
                                 0, WEECHAT_LIST_POS_END);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_buffers_plugins_names_cb: add plugins + buffers names
 *                                                   to completion list
 */

int
gui_completion_list_add_buffers_plugins_names_cb (void *data,
                                                  const char *completion_item,
                                                  struct t_gui_buffer *buffer,
                                                  struct t_gui_completion *completion)
{
    struct t_gui_buffer *ptr_buffer;
    char name[512];
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        snprintf (name, sizeof (name), "%s.%s",
                  plugin_get_name (ptr_buffer->plugin),
                  ptr_buffer->name);
        gui_completion_list_add (completion, name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_config_files_cb: add config files to completion list
 */

int
gui_completion_list_add_config_files_cb (void *data,
                                         const char *completion_item,
                                         struct t_gui_buffer *buffer,
                                         struct t_gui_completion *completion)
{
    struct t_config_file *ptr_config_file;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_config_file = config_files; ptr_config_file;
         ptr_config_file = ptr_config_file->next_config)
    {
        gui_completion_list_add (completion, ptr_config_file->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_filename: add filename to completion list
 */

int
gui_completion_list_add_filename_cb (void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    char *path_d, *path_b, *p, *d_name;
    char *real_prefix, *prefix;
    char *buf;
    int buf_len;
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;
    char home[3] = { '~', DIR_SEPARATOR_CHAR, '\0' };
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    buf_len = PATH_MAX;
    buf = malloc (buf_len);
    if (!buf)
	return WEECHAT_RC_OK;
    
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
	
    snprintf (buf, buf_len, "%s", completion->base_word + strlen (prefix));
    p = strrchr (buf, DIR_SEPARATOR_CHAR);
    if (p)
    {
	*p = '\0';
	path_d = strdup (buf);
	p++;
	path_b = strdup (p);
    }
    else {
	path_d = strdup ("");
	path_b = strdup (buf);
    }
    
    sprintf (buf, "%s%s%s", real_prefix, DIR_SEPARATOR, path_d);
    d_name = strdup (buf);
    dp = opendir (d_name);
    if (dp != NULL)
    {
	while ((entry = readdir (dp)) != NULL) 
	{	
	    if (strncmp (entry->d_name, path_b, strlen (path_b)) == 0) {
		
		if (strcmp (entry->d_name, ".") == 0 || strcmp (entry->d_name, "..") == 0)
		    continue;
		
		snprintf(buf, buf_len, "%s%s%s", 
			 d_name, DIR_SEPARATOR, entry->d_name);
		if (stat(buf, &statbuf) == -1)
		    continue;
		
		snprintf(buf, buf_len, "%s%s%s%s%s%s",
			 prefix, 
			 ((strcmp(prefix, "") == 0)
			  || strchr(prefix, DIR_SEPARATOR_CHAR)) ? "" : DIR_SEPARATOR, 
			 path_d, 
			 strcmp(path_d, "") == 0 ? "" : DIR_SEPARATOR, 
			 entry->d_name, 
			 S_ISDIR(statbuf.st_mode) ? DIR_SEPARATOR : "");
		
		gui_completion_list_add (completion, buf,
                                         0, WEECHAT_LIST_POS_SORT);
	    }
	}
        closedir (dp);
    }
    
    free (d_name);
    free (prefix);
    free (real_prefix);
    free (path_d);
    free (path_b);
    free (buf);
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_filters_cb: add filters to completion list
 */

int
gui_completion_list_add_filters_cb (void *data,
                                    const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    struct t_gui_filter *ptr_filter;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        gui_completion_list_add (completion, ptr_filter->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_commands_cb: add command hooks to completion list
 */

int
gui_completion_list_add_commands_cb (void *data,
                                     const char *completion_item,
                                     struct t_gui_buffer *buffer,
                                     struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
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
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_infos_cb: add info hooks to completion list
 */

int
gui_completion_list_add_infos_cb (void *data,
                                  const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_hook = weechat_hooks[HOOK_TYPE_INFO]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (HOOK_INFO(ptr_hook, info_name))
            && (HOOK_INFO(ptr_hook, info_name)[0]))
            gui_completion_list_add (completion,
                                     HOOK_INFO(ptr_hook, info_name),
                                     0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_infolists_cb: add infolist hooks to completion list
 */

int
gui_completion_list_add_infolists_cb (void *data,
                                      const char *completion_item,
                                      struct t_gui_buffer *buffer,
                                      struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_hook = weechat_hooks[HOOK_TYPE_INFOLIST]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (HOOK_INFOLIST(ptr_hook, infolist_name))
            && (HOOK_INFOLIST(ptr_hook, infolist_name)[0]))
            gui_completion_list_add (completion,
                                     HOOK_INFOLIST(ptr_hook, infolist_name),
                                     0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_nicks_cb: add nicks to completion list
 */

int
gui_completion_list_add_nicks_cb (void *data,
                                  const char *completion_item,
                                  struct t_gui_buffer *buffer,
                                  struct t_gui_completion *completion)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    int count_before;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    count_before = weelist_size (completion->completion_list);
    hook_completion_exec (completion->buffer->plugin,
                          "nick",
                          completion->buffer,
                          completion);
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
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_config_options_cb: add config option to completion
 *                                            list
 */

int
gui_completion_list_add_config_options_cb (void *data,
                                           const char *completion_item,
                                           struct t_gui_buffer *buffer,
                                           struct t_gui_completion *completion)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    int length;
    char *option_full_name;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
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
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_plugins_cb: add plugin name to completion list
 */

int
gui_completion_list_add_plugins_cb (void *data,
                                    const char *completion_item,
                                    struct t_gui_buffer *buffer,
                                    struct t_gui_completion *completion)
{
    struct t_weechat_plugin *ptr_plugin;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        gui_completion_list_add (completion, ptr_plugin->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_plugins_commands_cb: add plugin commands to completion
 *                                              list (plugin name is previous
 *                                              argument)
 */

int
gui_completion_list_add_plugins_commands_cb (void *data,
                                             const char *completion_item,
                                             struct t_gui_buffer *buffer,
                                             struct t_gui_completion *completion)
{
    char *pos_space, *plugin_name;
    struct t_weechat_plugin *ptr_plugin;
    struct t_hook *ptr_hook;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
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
            if (string_strcasecmp (plugin_name, PLUGIN_CORE) != 0)
            {
                /* plugin name is different from "core", then search it in
                   plugin list */
                ptr_plugin = plugin_search (plugin_name);
                if (!ptr_plugin)
                    return WEECHAT_RC_OK;
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
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_config_option_values_cb: add option value to
 *                                                  completion list
 */

int
gui_completion_list_add_config_option_values_cb (void *data,
                                                 const char *completion_item,
                                                 struct t_gui_buffer *buffer,
                                                 struct t_gui_completion *completion)
{
    char *pos_space, *option_full_name, *pos_section, *pos_option;
    char *file, *section, *value_string;
    const char *color_name;
    int length, i, num_colors;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section, *section_found;
    struct t_config_option *option_found;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
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
                                    if (option_found->value)
                                    {
                                        if (CONFIG_BOOLEAN(option_found) == CONFIG_BOOLEAN_TRUE)
                                            gui_completion_list_add (completion, "on",
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        else
                                            gui_completion_list_add (completion, "off",
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                    }
                                    else
                                    {
                                        gui_completion_list_add (completion,
                                                                 WEECHAT_CONFIG_OPTION_NULL,
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                    }
                                    break;
                                case CONFIG_OPTION_TYPE_INTEGER:
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
                                        if (option_found->value)
                                        {
                                            gui_completion_list_add (completion,
                                                                     option_found->string_values[CONFIG_INTEGER(option_found)],
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        }
                                        else
                                        {
                                            gui_completion_list_add (completion,
                                                                     WEECHAT_CONFIG_OPTION_NULL,
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        }
                                    }
                                    else
                                    {
                                        if (option_found->value && CONFIG_INTEGER(option_found) > option_found->min)
                                            gui_completion_list_add (completion, "--1",
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        if (option_found->value && CONFIG_INTEGER(option_found) < option_found->max)
                                            gui_completion_list_add (completion, "++1",
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        if (option_found->value)
                                        {
                                            length = 64;
                                            value_string = malloc (length);
                                            if (value_string)
                                            {
                                                snprintf (value_string, length,
                                                          "%d", CONFIG_INTEGER(option_found));
                                                gui_completion_list_add (completion,
                                                                         value_string,
                                                                         0, WEECHAT_LIST_POS_BEGINNING);
                                                free (value_string);
                                            }
                                        }
                                        else
                                        {
                                            gui_completion_list_add (completion,
                                                                     WEECHAT_CONFIG_OPTION_NULL,
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        }
                                    }
                                    break;
                                case CONFIG_OPTION_TYPE_STRING:
                                    gui_completion_list_add (completion,
                                                             "\"\"",
                                                             0, WEECHAT_LIST_POS_BEGINNING);
                                    if (option_found->value)
                                    {
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
                                    }
                                    else
                                    {
                                        gui_completion_list_add (completion,
                                                                 WEECHAT_CONFIG_OPTION_NULL,
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
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
                                    if (option_found->value)
                                    {
                                        color_name = gui_color_get_name (CONFIG_INTEGER(option_found));
                                        if (color_name)
                                        {
                                            gui_completion_list_add (completion,
                                                                     color_name,
                                                                     0, WEECHAT_LIST_POS_BEGINNING);
                                        }
                                    }
                                    else
                                    {
                                        gui_completion_list_add (completion,
                                                                 WEECHAT_CONFIG_OPTION_NULL,
                                                                 0, WEECHAT_LIST_POS_BEGINNING);
                                    }
                                    break;
                                case CONFIG_NUM_OPTION_TYPES:
                                    break;
                            }
                            if (option_found->value
                                && option_found->null_value_allowed)
                            {
                                gui_completion_list_add (completion,
                                                         WEECHAT_CONFIG_OPTION_NULL,
                                                         0,
                                                         WEECHAT_LIST_POS_END);
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
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_weechat_commands_cb: add WeeChat commands to
 *                                              completion list
 */

int
gui_completion_list_add_weechat_commands_cb (void *data,
                                             const char *completion_item,
                                             struct t_gui_buffer *buffer,
                                             struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
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
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_proxies_names_cb: add proxies names to completion
 *                                           list
 */

int
gui_completion_list_add_proxies_names_cb (void *data,
                                          const char *completion_item,
                                          struct t_gui_buffer *buffer,
                                          struct t_gui_completion *completion)
{
    struct t_proxy *ptr_proxy;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_proxy = weechat_proxies; ptr_proxy;
         ptr_proxy = ptr_proxy->next_proxy)
    {
        gui_completion_list_add (completion, ptr_proxy->name,
                                 0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_proxies_options: add proxies options to completion
 *                                          list
 */

int
gui_completion_list_add_proxies_options_cb (void *data,
                                            const char *completion_item,
                                            struct t_gui_buffer *buffer,
                                            struct t_gui_completion *completion)
{
    int i;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (i = 0; i < PROXY_NUM_OPTIONS; i++)
    {
        gui_completion_list_add (completion, proxy_option_string[i],
                                 0, WEECHAT_LIST_POS_SORT);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_list_add_keys_codes_cb: add keys to completion list
 */

int
gui_completion_list_add_keys_codes_cb (void *data,
                                       const char *completion_item,
                                       struct t_gui_buffer *buffer,
                                       struct t_gui_completion *completion)
{
    struct t_gui_key *ptr_key;
    char *expanded_name;
    
    /* make C compiler happy */
    (void) data;
    (void) completion_item;
    (void) buffer;
    
    for (ptr_key = gui_keys; ptr_key; ptr_key = ptr_key->next_key)
    {
        expanded_name = gui_keyboard_get_expanded_name (ptr_key->key);
        gui_completion_list_add (completion,
                                 (expanded_name) ? expanded_name : ptr_key->key,
                                 0, WEECHAT_LIST_POS_SORT);
        if (expanded_name)
            free (expanded_name);
    }
    
    return WEECHAT_RC_OK;
}

/*
 * gui_completion_custom: custom completion by a plugin
 */

void
gui_completion_custom (struct t_gui_completion *completion,
                       const char *custom_completion,
                       struct t_weechat_plugin *plugin)
{
    hook_completion_exec (plugin,
                          custom_completion,
                          completion->buffer,
                          completion);
}

/*
 * gui_completion_build_list_template: build data list according to a template
 */

void
gui_completion_build_list_template (struct t_gui_completion *completion,
                                    const char *template,
                                    struct t_weechat_plugin *plugin)
{
    char *word, *custom_completion;
    const char *pos, *pos_end;
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
                    gui_completion_list_add (completion, word,
                                             0, WEECHAT_LIST_POS_SORT);
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
                            gui_completion_stop (completion, 1);
                            free (word);
                            return;
                            break;
                        case '*': /* repeat last completion (do nothing there) */
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
 * gui_completion_get_matching_template: get template matching arguments for
 *                                       command
 */

int
gui_completion_get_matching_template (struct t_gui_completion *completion,
                                      struct t_hook *hook_command)
{
    int i, length;
    
    /* without at least one argument, we can't find matching template! */
    if (completion->base_command_arg_index <= 1)
        return -1;
    
    for (i = 0; i < HOOK_COMMAND(hook_command, cplt_num_templates); i++)
    {
        length = strlen (HOOK_COMMAND(hook_command, cplt_templates_static)[i]);
        if ((strncmp (HOOK_COMMAND(hook_command, cplt_templates_static)[i],
                      completion->args, length) == 0)
            && (completion->args[length] == ' '))
        {
            return i;
        }
    }
    
    return -1;
}

/*
 * gui_completion_get_template_for_args: get template according to user
 *                                       arguments for command
 */

char *
gui_completion_get_template_for_args (struct t_gui_completion *completion,
                                      struct t_hook *hook_command)
{
    int matching_template;
    
    /* if template refers to another command, search this command and use its
       template */
    if ((HOOK_COMMAND(hook_command, cplt_templates)[0][0] == '%')
        && (HOOK_COMMAND(hook_command, cplt_templates)[0][1] == '%')
        && (HOOK_COMMAND(hook_command, cplt_templates)[0][1]))
    {
        hook_command = gui_completion_search_command (HOOK_COMMAND(hook_command, cplt_templates)[0] + 2);
        if (!hook_command
            || ((HOOK_COMMAND(hook_command, cplt_templates)[0][0] == '%')
                && (HOOK_COMMAND(hook_command, cplt_templates)[0][1] == '%')))
        {
            return strdup ("");
        }
    }
    
    /* if only one template available, then use it */
    if (HOOK_COMMAND(hook_command, cplt_num_templates) == 1)
        return strdup (HOOK_COMMAND(hook_command, cplt_templates)[0]);
    
    /* search which template is matching arguments from user */
    matching_template = gui_completion_get_matching_template (completion,
                                                              hook_command);
    if (matching_template >= 0)
    {
        return strdup (HOOK_COMMAND(hook_command, cplt_templates[matching_template]));
    }
    else
    {
        if (HOOK_COMMAND(hook_command, cplt_template_num_args_concat) >= completion->base_command_arg_index)
            return strdup (HOOK_COMMAND(hook_command, cplt_template_args_concat[completion->base_command_arg_index - 1]));
        else
            return strdup ("");
    }
}

/*
 * gui_completion_build_list: build data list according to command and
 *                            argument index
 */

void
gui_completion_build_list (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    char *template, *pos_template, *pos_space;
    int repeat_last, i, length;
    
    repeat_last = 0;
    
    ptr_hook = gui_completion_search_command (completion->base_command);
    if (!ptr_hook || !HOOK_COMMAND(ptr_hook, completion)
        || (strcmp (HOOK_COMMAND(ptr_hook, completion), "-") == 0))
    {
        gui_completion_stop (completion, 1);
        return;
    }
    
    template = gui_completion_get_template_for_args (completion, ptr_hook);
    if (!template)
        return;
    
    length = strlen (template);
    if (length >= 2)
    {
        if (strcmp (template + length - 2, "%*") == 0)
            repeat_last = 1;
    }
    
    i = 1;
    pos_template = template;
    while (pos_template && pos_template[0])
    {
        pos_space = strchr (pos_template, ' ');
        if (i == completion->base_command_arg_index)
        {
            gui_completion_build_list_template (completion, pos_template,
                                                ptr_hook->plugin);
            free (template);
            return;
        }
        if (pos_space)
        {
            pos_template = pos_space;
            while (pos_template[0] == ' ')
            {
                pos_template++;
            }
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
    free (template);
}

/*
 * gui_completion_find_context: find context for completion
 */

void
gui_completion_find_context (struct t_gui_completion *completion,
                             const char *data, int size, int pos)
{
    int i, command, command_arg, pos_start, pos_end;
    
    /* look for context */
    gui_completion_free_data (completion);
    gui_completion_buffer_init (completion, completion->buffer);
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
            completion->base_command_arg_index = command_arg;
        }
        else
        {
            completion->context = GUI_COMPLETION_COMMAND;
            completion->base_command_arg_index = 0;
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
 * gui_completion_common_prefix_size: find common prefix size in matching items
 *                                    (case is ignored)
 *                                    if utf_char is not null, only words
 *                                    beginning with this char are compared
 *                                    (all other words are ignored)
 *                                    for example with items:
 *                                        FlashCode, flashy, flashouille
 *                                    common prefix size is 5 ("flash")
 */

int
gui_completion_common_prefix_size (struct t_weelist *list,
                                   const char *utf_char)
{
    struct t_weelist_item *ptr_item;
    char *ptr_first_item, *ptr_char, *next_char;
    
    ptr_first_item = list->items->data;
    ptr_char = ptr_first_item;
    
    while (ptr_char && ptr_char[0])
    {
        next_char = utf8_next_char (ptr_char);
        
        for (ptr_item = list->items->next_item; ptr_item;
             ptr_item = ptr_item->next_item)
        {
            if (!utf_char
                || (utf8_charcasecmp (utf_char, ptr_item->data) == 0))
            {
                if ((ptr_item->data[ptr_char - ptr_first_item] == '\0')
                    || (utf8_charcasecmp (ptr_char,
                                          ptr_item->data + (ptr_char - ptr_first_item)) != 0))
                {
                    return ptr_char - ptr_first_item;
                }
            }
        }
        
        ptr_char = next_char;
    }
    return ptr_char - ptr_first_item;
}

/*
 * gui_completion_partial_build_list: build list with possible completions
 *                                    when a partial completion occurs
 */

void
gui_completion_partial_build_list (struct t_gui_completion *completion,
                                   int common_prefix_size)
{
    int char_size, items_count;
    char utf_char[16], *word;
    struct t_weelist *weelist_temp, *weelist_words;
    struct t_weelist_item *ptr_item, *next_item;
    
    gui_completion_partial_list_free_all (completion);
    
    if (!completion->completion_list || !completion->completion_list->items)
        return;
    
    weelist_temp = weelist_new ();
    if (!weelist_temp)
        return;
    
    weelist_words = weelist_new ();
    if (!weelist_words)
    {
        weelist_free (weelist_temp);
        return;
    }
    
    for (ptr_item = completion->completion_list->items; ptr_item;
         ptr_item = ptr_item->next_item)
    {
        weelist_add (weelist_temp, ptr_item->data + common_prefix_size,
                     WEECHAT_LIST_POS_END, NULL);
    }
    
    while (weelist_temp->items)
    {
        char_size = utf8_char_size (weelist_temp->items->data);
        memcpy (utf_char, weelist_temp->items->data, char_size);
        utf_char[char_size] = '\0';
        word = NULL;
        common_prefix_size = gui_completion_common_prefix_size (weelist_temp,
                                                                utf_char);
        if (common_prefix_size > 0)
        {
            word = string_strndup (weelist_temp->items->data,
                                   common_prefix_size);
        }
        items_count = 0;
        ptr_item = weelist_temp->items;
        while (ptr_item)
        {
            next_item = ptr_item->next_item;
            
            if (utf8_charcasecmp (utf_char, ptr_item->data) == 0)
            {
                weelist_remove (weelist_temp, ptr_item);
                items_count++;
            }
            
            ptr_item = next_item;
        }
        if (word)
        {
            gui_completion_partial_list_add (completion,
                                             word,
                                             CONFIG_BOOLEAN(config_completion_partial_completion_count) ?
                                             items_count : -1);
            free (word);
        }
    }
    
    weelist_free (weelist_temp);
}

/*
 * gui_completion_complete: complete word using matching items
 */

void
gui_completion_complete (struct t_gui_completion *completion)
{
    int length, word_found_seen, other_completion, partial_completion;
    int common_prefix_size, item_is_nick;
    struct t_weelist_item *ptr_item, *ptr_item2;
    
    length = strlen (completion->base_word);
    word_found_seen = 0;
    other_completion = 0;
    
    partial_completion = completion->force_partial_completion;
    
    if (!partial_completion)
    {
        if (completion->context == GUI_COMPLETION_COMMAND)
        {
            partial_completion = CONFIG_BOOLEAN(config_completion_partial_completion_command);
        }
        else if (completion->context == GUI_COMPLETION_COMMAND_ARG)
        {
            partial_completion = CONFIG_BOOLEAN(config_completion_partial_completion_command_arg);
        }
        else
            partial_completion = CONFIG_BOOLEAN(config_completion_partial_completion_other);
    }
    
    common_prefix_size = 0;
    if (partial_completion
        && completion->completion_list && completion->completion_list->items)
    {
        common_prefix_size = gui_completion_common_prefix_size (completion->completion_list,
                                                                NULL);
    }
    
    if (completion->direction < 0)
        ptr_item = completion->completion_list->last_item;
    else
        ptr_item = completion->completion_list->items;
    
    if (partial_completion
        && completion->word_found
        && ((int)strlen (completion->word_found) >= common_prefix_size))
    {
        return;
    }
    
    while (ptr_item)
    {
        item_is_nick = ((int)(ptr_item->user_data) == 1);
        if ((item_is_nick
             && (gui_completion_nickncmp (completion->base_word, ptr_item->data,
                                          length) == 0))
            || ((!item_is_nick)
                && (string_strncasecmp (completion->base_word, ptr_item->data,
                                        length) == 0)))
        {
            if ((!completion->word_found) || word_found_seen)
            {
                if (completion->word_found)
                    free (completion->word_found);
                completion->word_found = strdup (ptr_item->data);
                completion->word_found_is_nick = item_is_nick;
                if (item_is_nick && (completion->base_word_pos > 0)
                    && !CONFIG_BOOLEAN(config_completion_nick_add_space))
                {
                    completion->add_space = 0;
                }
                
                /* stop after first nick if user asked that */
                if (item_is_nick
                    && CONFIG_BOOLEAN(config_completion_nick_first_only))
                {
                    gui_completion_stop (completion, 1);
                    return;
                }
                
                if (completion->direction < 0)
                    ptr_item2 = ptr_item->prev_item;
                else
                    ptr_item2 = ptr_item->next_item;
                
                while (ptr_item2)
                {
                    if ((item_is_nick
                         && (gui_completion_nickncmp (completion->base_word,
                                                      ptr_item2->data,
                                                      length) == 0))
                        || ((!item_is_nick)
                            && (string_strncasecmp (completion->base_word,
                                                    ptr_item2->data,
                                                    length) == 0)))
                    {
                        other_completion++;
                    }
                    
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
                
                /* stop after common prefix, if asked by user */
                if (partial_completion
                    && (((int)strlen (completion->word_found) >= common_prefix_size))
                    && (other_completion > 0))
                {
                    completion->word_found[common_prefix_size] = '\0';
                    completion->word_found_is_nick = 0;
                    completion->add_space = 0;
                    completion->position = -1;
                    string_tolower (completion->word_found);
                    
                    /* alert user of partial completion */
                    if (CONFIG_BOOLEAN(config_completion_partial_completion_alert))
                        printf ("\a");
                    
                    /* send "partial_completion" signal, to display possible
                       completions in bar item */
                    gui_completion_partial_build_list (completion,
                                                       common_prefix_size);
                    hook_signal_send ("partial_completion",
                                      WEECHAT_HOOK_SIGNAL_STRING, NULL);
                    return;
                }
                
                gui_completion_partial_list_free_all (completion);
                
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
    
    /* if we was on last completion in list, then recomplete, starting from
       first matching item */
    if (completion->word_found && (completion->position >= 0))
    {
        free (completion->word_found);
        completion->word_found = NULL;
        completion->word_found_is_nick = 0;
        gui_completion_complete (completion);
    }
}

/*
 * gui_completion_command: complete a command
 */

void
gui_completion_command (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    
    if (!completion->completion_list->items)
    {
        for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted
                && HOOK_COMMAND(ptr_hook, command)
                && HOOK_COMMAND(ptr_hook, command)[0])
            {
                gui_completion_list_add (completion,
                                         HOOK_COMMAND(ptr_hook, command),
                                         0, WEECHAT_LIST_POS_SORT);
            }
        }
    }
    
    gui_completion_complete (completion);
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
            gui_completion_list_add_filename_cb (NULL, NULL, NULL, completion);
        gui_completion_complete (completion);
        return;
    }
    
    /* use default template completion */
    if (!completion->completion_list->items)
    {
        gui_completion_build_list_template (completion,
                                            CONFIG_STRING(config_completion_default_template),
                                            NULL);
    }
    gui_completion_complete (completion);
}

/*
 * gui_completion_search: complete word according to context
 */

void
gui_completion_search (struct t_gui_completion *completion, int direction,
                       const char *data, int size, int pos)
{
    char *old_word_found;
    
    completion->direction = direction;
    
    /* if new completion => look for base word */
    if (pos != completion->position)
    {
        if (completion->word_found)
            free (completion->word_found);
        completion->word_found = NULL;
        completion->word_found_is_nick = 0;
        gui_completion_find_context (completion, data, size, pos);
        completion->force_partial_completion = (direction < 0);
    }
    
    /* completion */
    old_word_found = (completion->word_found) ?
        strdup (completion->word_found) : NULL;
    switch (completion->context)
    {
        case GUI_COMPLETION_NULL:
            /* should never be executed */
            return;
        case GUI_COMPLETION_COMMAND:
            gui_completion_command (completion);
            break;
        case GUI_COMPLETION_COMMAND_ARG:
            if (completion->completion_list->items)
                gui_completion_complete (completion);
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
    struct t_gui_completion_partial *ptr_item;
    
    log_printf ("[completion (addr:0x%lx)]", completion);
    log_printf ("  buffer. . . . . . . . . : 0x%lx", completion->buffer);
    log_printf ("  context . . . . . . . . : %d",    completion->context);
    log_printf ("  base_command. . . . . . : '%s'",  completion->base_command);
    log_printf ("  base_command_arg_index. : %d",    completion->base_command_arg_index);
    log_printf ("  base_word . . . . . . . : '%s'",  completion->base_word);
    log_printf ("  base_word_pos . . . . . : %d",    completion->base_word_pos);
    log_printf ("  position. . . . . . . . : %d",    completion->position);
    log_printf ("  args. . . . . . . . . . : '%s'",  completion->args);
    log_printf ("  direction . . . . . . . : %d",    completion->direction);
    log_printf ("  add_space . . . . . . . : %d",    completion->add_space);
    log_printf ("  force_partial_completion: %d",    completion->force_partial_completion);
    log_printf ("  completion_list . . . . : 0x%lx", completion->completion_list);
    log_printf ("  word_found. . . . . . . : '%s'",  completion->word_found);
    log_printf ("  word_found_is_nick. . . : %d",    completion->word_found_is_nick);
    log_printf ("  position_replace. . . . : %d",    completion->position_replace);
    log_printf ("  diff_size . . . . . . . : %d",    completion->diff_size);
    log_printf ("  diff_length . . . . . . : %d",    completion->diff_length);
    if (completion->completion_list)
    {
        log_printf ("");
        weelist_print_log (completion->completion_list,
                           "completion list element");
    }
    if (completion->partial_completion_list)
    {
        log_printf ("");
        for (ptr_item = completion->partial_completion_list;
             ptr_item; ptr_item = ptr_item->next_item)
        {
            log_printf ("[partial completion item (addr:0x%lx)]", ptr_item);
            log_printf ("  word. . . . . . . . . . : '%s'",  ptr_item->word);
            log_printf ("  count . . . . . . . . . : %d",    ptr_item->count);
            log_printf ("  prev_item . . . . . . . : 0x%lx", ptr_item->prev_item);
            log_printf ("  next_item . . . . . . . : 0x%lx", ptr_item->next_item);
        }
    }
}

/*
 * gui_completion_init: add hooks for completions done by WeeChat core
 */

void
gui_completion_init ()
{
    hook_completion (NULL, "buffers_names", /* it was %b */
                     N_("names of buffers"),
                     &gui_completion_list_add_buffers_names_cb, NULL);
    hook_completion (NULL, "buffers_numbers",
                     N_("numbers of buffers"),
                     &gui_completion_list_add_buffers_numbers_cb, NULL);
    hook_completion (NULL, "buffers_plugins_names", /* it was %B */
                     N_("names of buffers (including plugins names)"),
                     &gui_completion_list_add_buffers_plugins_names_cb, NULL);
    hook_completion (NULL, "config_files", /* it was %c */
                     N_("configuration files"),
                     &gui_completion_list_add_config_files_cb, NULL);
    hook_completion (NULL, "filename", /* it was %f */
                     N_("filename"),
                     &gui_completion_list_add_filename_cb, NULL);
    hook_completion (NULL, "filters_names", /* it was %F */
                     N_("names of filters"),
                     &gui_completion_list_add_filters_cb, NULL);
    hook_completion (NULL, "commands", /* it was %h */
                     N_("commands (weechat and plugins)"),
                     &gui_completion_list_add_commands_cb, NULL);
    hook_completion (NULL, "infos", /* it was %i */
                     N_("names of infos hooked"),
                     &gui_completion_list_add_infos_cb, NULL);
    hook_completion (NULL, "infolists", /* it was %I */
                     N_("names of infolists hooked"),
                     &gui_completion_list_add_infolists_cb, NULL);
    hook_completion (NULL, "nicks", /* it was %n */
                     N_("nicks in nicklist of current buffer"),
                     &gui_completion_list_add_nicks_cb, NULL);
    hook_completion (NULL, "config_options", /* it was %o */
                     N_("configuration options"),
                     &gui_completion_list_add_config_options_cb, NULL);
    hook_completion (NULL, "plugins_names", /* it was %p */
                     N_("names of plugins"),
                     &gui_completion_list_add_plugins_cb, NULL);
    hook_completion (NULL, "plugins_commands", /* it was %P */
                     N_("commands defined by plugins"),
                     &gui_completion_list_add_plugins_commands_cb, NULL);
    hook_completion (NULL, "bars_names", /* it was %r */
                     N_("names of bars"),
                     &gui_completion_list_add_bars_names_cb, NULL);
    hook_completion (NULL, "config_option_values", /* it was %v */
                     N_("values for a configuration option"),
                     &gui_completion_list_add_config_option_values_cb, NULL);
    hook_completion (NULL, "weechat_commands", /* it was %w */
                     N_("weechat commands"),
                     &gui_completion_list_add_weechat_commands_cb, NULL);
    hook_completion (NULL, "proxies_names", /* it was %y */
                     N_("names of proxies"),
                     &gui_completion_list_add_proxies_names_cb, NULL);
    hook_completion (NULL, "proxies_options",
                     N_("options for proxies"),
                     &gui_completion_list_add_proxies_options_cb, NULL);
    hook_completion (NULL, "bars_options",
                     N_("options for bars"),
                     &gui_completion_list_add_bars_options_cb, NULL);
    hook_completion (NULL, "keys_codes",
                     N_("key codes"),
                     &gui_completion_list_add_keys_codes_cb, NULL);
}
