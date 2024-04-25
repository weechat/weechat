/*
 * gui-completion.c - word completion according to context (used by all GUI)
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <limits.h>
#include <unistd.h>

#include "../core/weechat.h"
#include "../core/core-arraylist.h"
#include "../core/core-completion.h"
#include "../core/core-config.h"
#include "../core/core-eval.h"
#include "../core/core-hashtable.h"
#include "../core/core-hdata.h"
#include "../core/core-hook.h"
#include "../core/core-list.h"
#include "../core/core-log.h"
#include "../core/core-string.h"
#include "../core/core-utf8.h"
#include "../plugins/plugin.h"
#include "gui-completion.h"
#include "gui-buffer.h"


struct t_gui_completion *weechat_completions = NULL;
struct t_gui_completion *last_weechat_completion = NULL;

int gui_completion_freeze = 0;         /* 1 to freeze completions (do not   */
                                       /* stop partial completion on key)   */


/*
 * Compares two words in completion list.
 */

int
gui_completion_word_compare_cb (void *data,
                                struct t_arraylist *arraylist,
                                void *pointer1, void *pointer2)
{
    struct t_gui_completion *completion;
    struct t_gui_completion_word *completion_word1, *completion_word2;

    /* make C compiler happy */
    (void) arraylist;

    completion = (struct t_gui_completion *)data;

    completion_word1 = (struct t_gui_completion_word *)pointer1;
    completion_word2 = (struct t_gui_completion_word *)pointer2;

    if (completion->case_sensitive)
        return string_strcmp (completion_word1->word, completion_word2->word);
    else
        return string_strcasecmp (completion_word1->word, completion_word2->word);
}

/*
 * Frees a word in completion list.
 */

void
gui_completion_word_free_cb (void *data,
                             struct t_arraylist *arraylist,
                             void *pointer)
{
    struct t_gui_completion_word *completion_word;

    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    completion_word = (struct t_gui_completion_word *)pointer;

    free (completion_word->word);

    free (completion_word);
}

/*
 * Initializes completion.
 */

void
gui_completion_init (struct t_gui_completion *completion,
                     struct t_weechat_plugin *plugin,
                     struct t_gui_buffer *buffer)
{
    completion->plugin = plugin;
    completion->buffer = buffer;
    completion->context = GUI_COMPLETION_NULL;
    completion->case_sensitive = CONFIG_BOOLEAN(config_completion_case_sensitive);
    completion->base_command = NULL;
    completion->base_command_arg_index = 0;
    completion->base_word = NULL;
    completion->base_word_pos = 0;
    completion->position = -1;
    completion->args = NULL;
    completion->direction = 0;
    completion->add_space = 1;
    completion->force_partial_completion = 0;
    completion->reverse_partial_completion = 0;

    completion->list = arraylist_new (
        32, 1, 0,
        &gui_completion_word_compare_cb, completion,
        &gui_completion_word_free_cb, NULL);

    completion->word_found = NULL;
    completion->word_found_is_nick = 0;
    completion->position_replace = 0;
    completion->diff_size = 0;
    completion->diff_length = 0;

    completion->partial_list = arraylist_new (
        0, 0, 0,
        &gui_completion_word_compare_cb, completion,
        &gui_completion_word_free_cb, NULL);
}

/*
 * Creates a new completion.
 *
 * Returns pointer to completion, NULL if error.
 */

struct t_gui_completion *
gui_completion_new (struct t_weechat_plugin *plugin,
                    struct t_gui_buffer *buffer)
{
    struct t_gui_completion *completion;

    if (!buffer)
        return NULL;

    completion = malloc (sizeof (*completion));
    if (!completion)
        return NULL;

    gui_completion_init (completion, plugin, buffer);

    /* add completion to the global list */
    completion->prev_completion = last_weechat_completion;
    completion->next_completion = NULL;
    if (last_weechat_completion)
        last_weechat_completion->next_completion = completion;
    else
        weechat_completions = completion;
    last_weechat_completion = completion;

    return completion;
}

/*
 * Adds an item to partial completion list.
 *
 * Returns pointer to new item, NULL if error.
 */

struct t_gui_completion_word *
gui_completion_partial_list_add (struct t_gui_completion *completion,
                                 const char *word, int count)
{
    struct t_gui_completion_word *new_completion_word;

    new_completion_word = malloc (sizeof (*new_completion_word));
    if (new_completion_word)
    {
        new_completion_word->word = strdup (word);
        new_completion_word->nick_completion = 0;
        new_completion_word->count = count;

        arraylist_add (completion->partial_list, new_completion_word);
    }

    return new_completion_word;
}

/*
 * Frees data in completion.
 */

void
gui_completion_free_data (struct t_gui_completion *completion)
{
    free (completion->base_command);
    completion->base_command = NULL;

    free (completion->base_word);
    completion->base_word = NULL;

    free (completion->args);
    completion->args = NULL;

    if (completion->list)
    {
        arraylist_free (completion->list);
        completion->list = NULL;
    }

    free (completion->word_found);
    completion->word_found = NULL;

    if (completion->partial_list)
    {
        arraylist_free (completion->partial_list);
        completion->partial_list = NULL;
    }
}

/*
 * Frees completion.
 */

void
gui_completion_free (struct t_gui_completion *completion)
{
    struct t_gui_completion *new_weechat_completions;

    if (!completion)
        return;

    /* remove completion from global list */
    if (last_weechat_completion == completion)
        last_weechat_completion = completion->prev_completion;
    if (completion->prev_completion)
    {
        (completion->prev_completion)->next_completion = completion->next_completion;
        new_weechat_completions = weechat_completions;
    }
    else
        new_weechat_completions = completion->next_completion;
    if (completion->next_completion)
        (completion->next_completion)->prev_completion = completion->prev_completion;
    weechat_completions = new_weechat_completions;

    /* free data */
    gui_completion_free_data (completion);

    free (completion);
}

/*
 * Frees all completions created by a plugin.
 */

void
gui_completion_free_all_plugin (struct t_weechat_plugin *plugin)
{
    struct t_gui_completion *ptr_completion, *next_completion;

    ptr_completion = weechat_completions;
    while (ptr_completion)
    {
        next_completion = ptr_completion->next_completion;
        if (ptr_completion->plugin == plugin)
            gui_completion_free (ptr_completion);
        ptr_completion = next_completion;
    }
}

/*
 * Stops completion (for example after 1 argument of command with 1 argument).
 */

void
gui_completion_stop (struct t_gui_completion *completion)
{
    if (!completion)
        return;

    completion->context = GUI_COMPLETION_NULL;
    completion->position = -1;

    if (completion->partial_list->size > 0)
    {
        arraylist_clear (completion->partial_list);
        (void) hook_signal_send ("partial_completion",
                                 WEECHAT_HOOK_SIGNAL_STRING, NULL);
    }
}

/*
 * Checks if nick has one or more ignored chars (for nick comparison).
 *
 * Returns:
 *   1: nick has one or more ignored chars
 *   0: nick has no ignored chars
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
 * Duplicates a nick and ignores some chars.
 *
 * Note: result must be freed after use.
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
 * Locale and case independent string comparison with max length for nicks
 * (alpha or digits only).
 *
 * Returns:
 *   < 0: base_word < nick
 *     0: base_word == nick
 *   > 0: base_word > nick
 */

int
gui_completion_nickncmp (const char *base_word, const char *nick, int max)
{
    char *base_word2, *nick2;
    int case_sensitive, return_cmp;

    case_sensitive = CONFIG_BOOLEAN(config_completion_nick_case_sensitive);

    if (!CONFIG_STRING(config_completion_nick_ignore_chars)
        || !CONFIG_STRING(config_completion_nick_ignore_chars)[0]
        || !base_word[0] || !nick[0]
        || gui_completion_nick_has_ignored_chars (base_word))
    {
        return (case_sensitive) ?
            strncmp (base_word, nick, max) :
            string_strncasecmp (base_word, nick, max);
    }

    base_word2 = gui_completion_nick_strdup_ignore_chars (base_word);
    nick2 = gui_completion_nick_strdup_ignore_chars (nick);

    return_cmp = (case_sensitive) ?
        strncmp (base_word2, nick2, utf8_strlen (base_word2)) :
        string_strncasecmp (base_word2, nick2, utf8_strlen (base_word2));

    free (base_word2);
    free (nick2);

    return return_cmp;
}

/*
 * Compares two strings (follows case sensitive flag in completion structure).
 *
 * Returns:
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
gui_completion_strcmp (struct t_gui_completion *completion,
                       const char *string1, const char *string2)
{
    if (!completion)
        return 0;

    return (completion->case_sensitive) ?
        string_strcmp (string1, string2) : string_strcasecmp (string1, string2);
}

/*
 * Compares two strings with max length (follows case sensitive flag in
 * completion structure).
 *
 * Returns:
 *   < 0: string1 < string2
 *     0: string1 == string2
 *   > 0: string1 > string2
 */

int
gui_completion_strncmp (struct t_gui_completion *completion,
                        const char *string1, const char *string2, int max)
{
    if (!completion)
        return 0;

    return (completion->case_sensitive) ?
        string_strncmp (string1, string2, max) :
        string_strncasecmp (string1, string2, max);
}

/*
 * Adds a word to completion list.
 */

void
gui_completion_list_add (struct t_gui_completion *completion, const char *word,
                         int nick_completion, const char *where)
{
    struct t_gui_completion_word *completion_word;
    char buffer[512];
    int index;

    if (!word || !word[0])
        return;

    if (!completion->base_word || !completion->base_word[0]
        || (nick_completion
            && (gui_completion_nickncmp (completion->base_word, word,
                                         utf8_strlen (completion->base_word)) == 0))
        || (!nick_completion
            && (gui_completion_strncmp (completion,
                                        completion->base_word, word,
                                        utf8_strlen (completion->base_word)) == 0)))
    {
        completion_word = malloc (sizeof (*completion_word));
        if (completion_word)
        {
            completion_word->nick_completion = nick_completion;
            completion_word->count = 0;

            index = -1;
            if (strcmp (where, WEECHAT_LIST_POS_BEGINNING) == 0)
            {
                completion->list->sorted = 0;
                index = 0;
            }
            else if (strcmp (where, WEECHAT_LIST_POS_END) == 0)
            {
                completion->list->sorted = 0;
                index = -1;
            }

            if (nick_completion && (completion->base_word_pos == 0))
            {
                snprintf (buffer, sizeof (buffer), "%s%s",
                          word,
                          CONFIG_STRING(config_completion_nick_completer));
                completion_word->word = strdup (buffer);
                arraylist_insert (completion->list, index, completion_word);
                completion->add_space = 0;
            }
            else
            {
                completion_word->word = strdup (word);
                arraylist_insert (completion->list, index, completion_word);
            }
        }
    }
}

/*
 * Custom completion by a plugin.
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
 * Builds data list according to a template.
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
                            gui_completion_stop (completion);
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
                                        if (hashtable_has_key (
                                                config_hashtable_completion_partial_templates,
                                                custom_completion))
                                        {
                                            completion->reverse_partial_completion = 1;
                                        }
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
 * Gets template matching arguments for command.
 */

int
gui_completion_get_matching_template (struct t_gui_completion *completion,
                                      struct t_hook *hook_command)
{
    int i, j, length, fallback, num_items;
    char **items;

    /* without at least one argument, we can't find matching template! */
    if (completion->base_command_arg_index <= 1)
        return -1;

    fallback = -1;

    for (i = 0; i < HOOK_COMMAND(hook_command, cplt_num_templates); i++)
    {
        items = string_split (HOOK_COMMAND(hook_command, cplt_templates_static)[i],
                              "|",
                              NULL,
                              WEECHAT_STRING_SPLIT_STRIP_LEFT
                              | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                              | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                              0,
                              &num_items);
        if (items)
        {
            for (j = 0; j < num_items; j++)
            {
                length = strlen (items[j]);
                if ((strncmp (items[j], completion->args, length) == 0)
                    && (completion->args[length] == ' '))
                {
                    string_free_split (items);
                    return i;
                }
            }
            string_free_split (items);
        }

        /*
         * try to find a fallback template if we don't find any matching
         * template, for example with these templates (command /set):
         *   %(config_options) %(config_option_values)
         *   diff %(config_options)|%*
         * if first argument is "diff", the match is OK (second template)
         * if first argument is not "diff", we will fallback on the first
         * template containing "%" (here first template)
         */
        if ((fallback < 0)
            && (strstr (HOOK_COMMAND(hook_command, cplt_templates_static)[i], "%")))
        {
            fallback = i;
        }
    }

    return fallback;
}

/*
 * Searches for a command hook.
 *
 * Returns pointer to hook found, NULL if not found.
 */

struct t_hook *
gui_completion_search_command (struct t_gui_completion *completion,
                               const char *command)
{
    struct t_hook *ptr_hook, *hook_for_other_plugin, *hook_incomplete_command;
    int length_command, allow_incomplete_commands, count_incomplete_commands;

    if (!completion || !command)
        return NULL;

    hook_for_other_plugin = NULL;
    hook_incomplete_command = NULL;
    length_command = utf8_strlen (command);
    count_incomplete_commands = 0;
    allow_incomplete_commands = CONFIG_BOOLEAN(config_look_command_incomplete);

    for (ptr_hook = weechat_hooks[HOOK_TYPE_COMMAND]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && HOOK_COMMAND(ptr_hook, command)
            && HOOK_COMMAND(ptr_hook, command)[0])
        {
            if (gui_completion_strcmp (completion,
                                       HOOK_COMMAND(ptr_hook, command), command) == 0)
            {
                if (ptr_hook->plugin == completion->buffer->plugin)
                    return ptr_hook;
                hook_for_other_plugin = ptr_hook;
            }
            else if (allow_incomplete_commands
                     && (gui_completion_strncmp (completion,
                                                 HOOK_COMMAND(ptr_hook, command),
                                                 command,
                                                 length_command) == 0))
            {
                hook_incomplete_command = ptr_hook;
                count_incomplete_commands++;
            }
        }
    }

    if (hook_for_other_plugin)
        return hook_for_other_plugin;

    return (count_incomplete_commands == 1) ?
        hook_incomplete_command : NULL;
}

/*
 * Gets template according to user arguments for command.
 */

char *
gui_completion_get_template_for_args (struct t_gui_completion *completion,
                                      struct t_hook *hook_command)
{
    int matching_template;

    /*
     * if template refers to another command, search this command and use its
     * template
     */
    if ((HOOK_COMMAND(hook_command, cplt_templates)[0][0] == '%')
        && (HOOK_COMMAND(hook_command, cplt_templates)[0][1] == '%')
        && (HOOK_COMMAND(hook_command, cplt_templates)[0][1]))
    {
        hook_command = gui_completion_search_command (completion,
                                                      HOOK_COMMAND(hook_command, cplt_templates)[0] + 2);
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
 * Builds data list according to command and argument index.
 */

void
gui_completion_build_list (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;
    char *template, *pos_template, *pos_space;
    int repeat_last, i, length;

    repeat_last = 0;

    ptr_hook = gui_completion_search_command (completion, completion->base_command);
    if (!ptr_hook || !HOOK_COMMAND(ptr_hook, completion))
    {
        completion->context = GUI_COMPLETION_AUTO;
        completion->base_command_arg_index = 0;
        free (completion->base_command);
        completion->base_command = NULL;
        return;
    }

    if (strcmp (HOOK_COMMAND(ptr_hook, completion), "-") == 0)
    {
        gui_completion_stop (completion);
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
        pos_space = strrchr (template, ' ');
        gui_completion_build_list_template (completion,
                                            (pos_space) ?
                                            pos_space + 1 : template,
                                            ptr_hook->plugin);
    }
    free (template);
}

/*
 * Finds context for completion.
 */

void
gui_completion_find_context (struct t_gui_completion *completion,
                             const char *data, int pos)
{
    int i, size, command_arg, pos_start, pos_end;
    const char *ptr_command, *ptr_data, *ptr_space, *ptr_newline, *prev_char;

    /* look for context */
    gui_completion_free_data (completion);
    gui_completion_init (completion, completion->plugin, completion->buffer);
    ptr_command = NULL;
    command_arg = 0;

    size = (data) ? strlen (data) : 0;

    /* check if data starts with a command */
    ptr_data = data;
    if (string_is_command_char (ptr_data))
    {
        ptr_data = utf8_next_char (ptr_data);
        if (ptr_data < data + pos)
        {
            if (string_is_command_char (ptr_data))
                ptr_data = utf8_next_char (ptr_data);
        }
        if (!string_is_command_char (ptr_data))
            ptr_command = ptr_data;
    }

    /*
     * search for the last command in data (only if there is no command at
     * beginning and if completion of inline commands is enabled)
     */
    if (!ptr_command && CONFIG_BOOLEAN(config_completion_command_inline))
    {
        ptr_data = data;
        while (ptr_data && (ptr_data < data + pos))
        {
            ptr_space = strchr (ptr_data, ' ');
            ptr_newline = strchr (ptr_data, '\n');
            ptr_data = (ptr_newline && (!ptr_space || (ptr_newline < ptr_space))) ?
                ptr_newline : ptr_space;
            if (!ptr_data)
                break;
            if (ptr_data < data + pos)
            {
                while ((ptr_data < data + pos) && ((ptr_data[0] == ' ') || (ptr_data[0] == '\n')))
                {
                    ptr_data++;
                }
            }
            if ((ptr_data < data + pos) && string_is_command_char (ptr_data))
            {
                ptr_data = utf8_next_char (ptr_data);
                if (!string_is_command_char (ptr_data))
                    ptr_command = ptr_data;
            }
        }
    }

    if (ptr_command)
    {
        /* search argument number and string with arguments */
        ptr_data = ptr_command;
        while (ptr_data < data + pos)
        {
            ptr_data = strchr (ptr_data, ' ');
            if (!ptr_data)
                break;
            command_arg++;
            while ((ptr_data < data + pos) && (ptr_data[0] == ' '))
            {
                ptr_data++;
            }
            if (!completion->args)
                completion->args = strdup (ptr_data);
        }

        /* set completion context */
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
            if ((i > 0) && (data[i - 1] != ' ') && (data[i - 1] != '\n'))
            {
                i--;
                while ((i >= 0) && (data[i] != ' ') && (data[i] != '\n'))
                {
                    i--;
                }
                pos_start = i + 1;
            }
        }
        else
        {
            while ((i >= 0) && (data[i] != ' ') && (data[i] != '\n'))
            {
                i--;
            }
            pos_start = i + 1;
        }
        if (CONFIG_BOOLEAN (config_completion_base_word_until_cursor))
        {
            /* base word stops at cursor */
            pos_end = pos - 1;
        }
        else
        {
            /* base word stops after first space found (on or after cursor) */
            i = pos;
            while ((i < size) && (data[i] != ' ') && (data[i] != '\n'))
            {
                i++;
            }
            pos_end = i - 1;
        }

        if (completion->context == GUI_COMPLETION_COMMAND)
        {
            pos_start++;
            if (string_is_command_char (data + pos_start))
                pos_start += utf8_char_size (data + pos_start);
        }

        completion->base_word_pos = pos_start;

        if (pos_start <= pos_end)
        {
            completion->position_replace = pos_start;
            completion->base_word = malloc (pos_end - pos_start + 2);
            for (i = pos_start; i <= pos_end; i++)
            {
                completion->base_word[i - pos_start] = data[i];
            }
            completion->base_word[pos_end - pos_start + 1] = '\0';
        }
    }

    if (!completion->base_word)
        completion->base_word = strdup ("");

    /* find command (for command argument completion only) */
    if (completion->context == GUI_COMPLETION_COMMAND_ARG)
    {
        pos_start = ptr_command - data;
        pos_end = pos_start;
        while ((pos_end < size) && (data[pos_end] != ' '))
        {
            pos_end += utf8_char_size (data + pos_end);
        }
        if (data[pos_end] == ' ')
        {
            prev_char = utf8_prev_char (data, data + pos_end);
            pos_end -= utf8_char_size (prev_char);
        }
        if (pos_end >= pos_start)
        {
            completion->base_command = malloc (pos_end - pos_start + 2);
            for (i = pos_start; i <= pos_end; i++)
            {
                completion->base_command[i - pos_start] = data[i];
            }
            completion->base_command[pos_end - pos_start + 1] = '\0';
        }
        else
            completion->base_command = strdup ("");
        gui_completion_build_list (completion);
    }

    /*
     * auto completion with nothing as base word is disabled,
     * in order to prevent completion when pasting messages with [tab] inside
     */
    if ((completion->context == GUI_COMPLETION_AUTO)
        && ((!completion->base_word) || (!completion->base_word[0])))
    {
        completion->context = GUI_COMPLETION_NULL;
        return;
    }
}

/*
 * Finds common prefix size in matching items (case is ignored).
 *
 * If utf_char is not null, only words beginning with this char are compared
 * (all other words are ignored).
 *
 * For example with items:
 *   FlashCode, flashy, flashouille
 *   => common prefix size is 5 ("flash")
 */

int
gui_completion_common_prefix_size (struct t_arraylist *list,
                                   const char *utf_char)
{
    const char *ptr_first_item, *ptr_char, *next_char;
    struct t_gui_completion_word *ptr_completion_word;
    int i;

    ptr_first_item = ((struct t_gui_completion_word *)(list->data[0]))->word;
    ptr_char = ptr_first_item;

    while (ptr_char && ptr_char[0])
    {
        next_char = utf8_next_char (ptr_char);

        for (i = 1; i < list->size; i++)
        {
            ptr_completion_word =
                (struct t_gui_completion_word *)(list->data[i]);
            if (!utf_char
                || (string_charcasecmp (utf_char,
                                        ptr_completion_word->word) == 0))
            {
                if ((ptr_completion_word->word[ptr_char - ptr_first_item] == '\0')
                    || (string_charcasecmp (
                            ptr_char,
                            ptr_completion_word->word + (ptr_char - ptr_first_item)) != 0))
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
 * Builds list with possible completions when a partial completion occurs.
 */

void
gui_completion_partial_build_list (struct t_gui_completion *completion,
                                   int common_prefix_size)
{
    int i, char_size, items_count, index;
    char utf_char[16], *word;
    struct t_gui_completion_word *ptr_completion_word, *new_completion_word;
    struct t_arraylist *list_temp;

    arraylist_clear (completion->partial_list);

    if (!completion->list || (completion->list->size == 0))
        return;

    list_temp = arraylist_new (completion->list->size, 1, 0,
                               &gui_completion_word_compare_cb, completion,
                               &gui_completion_word_free_cb, NULL);
    if (!list_temp)
        return;

    for (i = 0; i < completion->list->size; i++)
    {
        ptr_completion_word =
            (struct t_gui_completion_word *)completion->list->data[i];
        new_completion_word = malloc (sizeof (*new_completion_word));
        if (new_completion_word)
        {
            new_completion_word->word = strdup (
                ptr_completion_word->word + common_prefix_size);
            new_completion_word->nick_completion = 0;
            new_completion_word->count = 0;
            arraylist_add (list_temp, new_completion_word);
        }
    }

    while (list_temp->size > 0)
    {
        ptr_completion_word =
            (struct t_gui_completion_word *)list_temp->data[0];
        char_size = utf8_char_size (ptr_completion_word->word);
        memcpy (utf_char, ptr_completion_word->word, char_size);
        utf_char[char_size] = '\0';
        word = NULL;
        common_prefix_size = gui_completion_common_prefix_size (list_temp,
                                                                utf_char);
        if (common_prefix_size > 0)
        {
            word = string_strndup (ptr_completion_word->word,
                                   common_prefix_size);
        }
        items_count = 0;
        index = 0;
        while (index < list_temp->size)
        {
            ptr_completion_word =
                (struct t_gui_completion_word *)list_temp->data[index];
            if (string_charcasecmp (utf_char, ptr_completion_word->word) == 0)
            {
                arraylist_remove (list_temp, index);
                items_count++;
            }
            else
                index++;
        }
        if (word)
        {
            gui_completion_partial_list_add (
                completion,
                word,
                CONFIG_BOOLEAN(config_completion_partial_completion_count) ?
                items_count : -1);
            free (word);
        }
    }

    arraylist_free (list_temp);
}

/*
 * Completes word using matching items.
 */

void
gui_completion_complete (struct t_gui_completion *completion)
{
    int length, word_found_seen, other_completion, partial_completion;
    int common_prefix_size, index, index2;
    struct t_gui_completion_word *ptr_completion_word, *ptr_completion_word2;

    length = utf8_strlen (completion->base_word);
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

    if (completion->reverse_partial_completion)
        partial_completion ^= 1;

    common_prefix_size = 0;
    if (partial_completion
        && completion->list && (completion->list->size > 0))
    {
        common_prefix_size = gui_completion_common_prefix_size (completion->list,
                                                                NULL);
    }

    if (partial_completion
        && completion->word_found
        && (utf8_strlen (completion->word_found) >= common_prefix_size))
    {
        return;
    }

    index = -1;
    if (completion->list)
    {
        if (completion->direction < 0)
            index = completion->list->size - 1;
        else
            index = 0;
    }

    while ((index >= 0) && (index < completion->list->size))
    {
        ptr_completion_word =
            (struct t_gui_completion_word *)(completion->list->data[index]);
        if ((ptr_completion_word->nick_completion
             && (gui_completion_nickncmp (completion->base_word,
                                          ptr_completion_word->word,
                                          length) == 0))
            || (!ptr_completion_word->nick_completion
                && (gui_completion_strncmp (completion,
                                            completion->base_word,
                                            ptr_completion_word->word,
                                            length) == 0)))
        {
            if ((!completion->word_found) || word_found_seen)
            {
                free (completion->word_found);
                completion->word_found = strdup (ptr_completion_word->word);
                completion->word_found_is_nick =
                    ptr_completion_word->nick_completion;
                if (ptr_completion_word->nick_completion
                    && !CONFIG_BOOLEAN(config_completion_nick_add_space))
                {
                    completion->add_space = 0;
                }

                /* stop after first nick if user asked that */
                if (ptr_completion_word->nick_completion
                    && CONFIG_BOOLEAN(config_completion_nick_first_only))
                {
                    gui_completion_stop (completion);
                    return;
                }

                index2 = (completion->direction < 0) ? index - 1 : index + 1;
                while ((index2 >= 0) && (index2 < completion->list->size))
                {
                    ptr_completion_word2 =
                        (struct t_gui_completion_word *)(completion->list->data[index2]);
                    if ((ptr_completion_word->nick_completion
                         && (gui_completion_nickncmp (completion->base_word,
                                                      ptr_completion_word2->word,
                                                      length) == 0))
                        || (!ptr_completion_word->nick_completion
                            && (gui_completion_strncmp (completion,
                                                        completion->base_word,
                                                        ptr_completion_word2->word,
                                                        length) == 0)))
                    {
                        other_completion++;
                    }

                    index2 = (completion->direction < 0) ?
                        index2 - 1 : index2 + 1;
                }

                if (other_completion == 0)
                    completion->position = -1;
                else
                    if (completion->position < 0)
                        completion->position = 0;

                /* stop after common prefix, if asked by user */
                if (partial_completion
                    && ((utf8_strlen (completion->word_found) >= common_prefix_size))
                    && (other_completion > 0))
                {
                    completion->word_found[common_prefix_size] = '\0';
                    completion->word_found_is_nick = 0;
                    completion->add_space = 0;
                    completion->position = -1;

                    /* alert user of partial completion */
                    if (CONFIG_BOOLEAN(config_completion_partial_completion_alert))
                    {
                        fprintf (stderr, "\a");
                        fflush (stderr);
                    }

                    /*
                     * send "partial_completion" signal, to display possible
                     * completions in bar item
                     */
                    gui_completion_partial_build_list (completion,
                                                       common_prefix_size);
                    (void) hook_signal_send ("partial_completion",
                                             WEECHAT_HOOK_SIGNAL_STRING, NULL);
                    return;
                }

                arraylist_clear (completion->partial_list);
                (void) hook_signal_send ("partial_completion",
                                        WEECHAT_HOOK_SIGNAL_STRING, NULL);

                return;
            }
            other_completion++;
        }
        if (completion->word_found &&
            (gui_completion_strcmp (completion,
                                    ptr_completion_word->word,
                                    completion->word_found) == 0))
        {
            word_found_seen = 1;
        }

        index = (completion->direction < 0) ? index - 1 : index + 1;
    }

    /*
     * if we was on last completion in list, then complete again, starting from
     * first matching item
     */
    if (completion->word_found && (completion->position >= 0))
    {
        free (completion->word_found);
        completion->word_found = NULL;
        completion->word_found_is_nick = 0;
        gui_completion_complete (completion);
    }
}

/*
 * Completes a command.
 */

void
gui_completion_command (struct t_gui_completion *completion)
{
    struct t_hook *ptr_hook;

    if (completion->list->size == 0)
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
 * Gets default completion template: from buffer local variable
 * "completion_default_template" if defined, or the value of option
 * "weechat.completion.default_template".
 *
 * Note: result must be freed after use.
 */

char *
gui_completion_get_default_template (struct t_gui_completion *completion)
{
    const char *ptr_default_template;
    char *value;
    struct t_hashtable *pointers;

    /* search buffer local variable "completion_default_template" */
    ptr_default_template = hashtable_get (
        completion->buffer->local_variables,
        "completion_default_template");

    if (!ptr_default_template)
    {
        /* return the global default completion template */
        ptr_default_template = CONFIG_STRING(config_completion_default_template);
        return strdup ((ptr_default_template) ? ptr_default_template : "");
    }

    /* evaluate the buffer local variable */
    pointers = hashtable_new (32,
                              WEECHAT_HASHTABLE_STRING,
                              WEECHAT_HASHTABLE_POINTER,
                              NULL, NULL);
    hashtable_set (pointers, "buffer", completion->buffer);
    value = eval_expression (ptr_default_template, pointers, NULL, NULL);
    hashtable_free (pointers);

    return (value) ? value : strdup ("");
}

/*
 * Auto-completes: nick, filename or channel.
 */

void
gui_completion_auto (struct t_gui_completion *completion)
{
    char *default_completion;

    /* filename completion */
    if ((completion->base_word[0] == '/')
        || (completion->base_word[0] == '~'))
    {
        if (completion->list->size == 0)
            completion_list_add_filename_cb (NULL, NULL, NULL, NULL,
                                             completion);
        gui_completion_complete (completion);
        return;
    }

    /* use default template completion */
    if (completion->list->size == 0)
    {
        default_completion = gui_completion_get_default_template (completion);
        gui_completion_build_list_template (
            completion,
            (default_completion) ? default_completion : "",
            NULL);
        free (default_completion);
    }
    gui_completion_complete (completion);
}

/*
 * Completes word according to context.
 */

int
gui_completion_search (struct t_gui_completion *completion, const char *data,
                       int position, int direction)
{
    char *old_word_found;
    int real_position;

    if (!completion || !data || (position < 0))
        return 0;

    real_position = utf8_real_pos (data, position);

    completion->direction = direction;

    /* if new completion => look for base word */
    if (real_position != completion->position)
    {
        free (completion->word_found);
        completion->word_found = NULL;
        completion->word_found_is_nick = 0;
        gui_completion_find_context (completion, data, real_position);
        completion->force_partial_completion = (direction < 0);
    }

    /* completion */
    old_word_found = (completion->word_found) ?
        strdup (completion->word_found) : NULL;
    switch (completion->context)
    {
        case GUI_COMPLETION_NULL:
            free (old_word_found);
            return 0;
        case GUI_COMPLETION_COMMAND:
            gui_completion_command (completion);
            break;
        case GUI_COMPLETION_COMMAND_ARG:
            if (completion->list->size > 0)
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
    free (old_word_found);

    return 1;
}

/*
 * Gets a completion property as string.
 */

const char *
gui_completion_get_string (struct t_gui_completion *completion,
                           const char *property)
{
    if (!completion || !property)
        return NULL;

    if (strcmp (property, "base_command") == 0)
        return completion->base_command;
    else if (strcmp (property, "base_word") == 0)
        return completion->base_word;
    else if (strcmp (property, "args") == 0)
        return completion->args;

    return NULL;
}

/*
 * Returns hdata for completion.
 */

struct t_hdata *
gui_completion_hdata_completion_cb (const void *pointer, void *data,
                                    const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_completion", "next_completion",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_completion, plugin, POINTER, 0, NULL, "plugin");
        HDATA_VAR(struct t_gui_completion, buffer, POINTER, 0, NULL, "buffer");
        HDATA_VAR(struct t_gui_completion, context, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, case_sensitive, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, base_command, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, base_command_arg_index, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, base_word, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, base_word_pos, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, position, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, args, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, direction, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, add_space, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, force_partial_completion, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, reverse_partial_completion, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, list, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, word_found, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, word_found_is_nick, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, position_replace, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, diff_size, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, diff_length, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, partial_list, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion, prev_completion, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_completion, next_completion, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(weechat_completions, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_weechat_completion, 0);
    }
    return hdata;
}

/*
 * Returns hdata for completion word.
 */

struct t_hdata *
gui_completion_hdata_completion_word_cb (const void *pointer, void *data,
                                         const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, NULL, NULL, 0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_completion_word, word, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion_word, nick_completion, CHAR, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_completion_word, count, INTEGER, 0, NULL, NULL);
    }
    return hdata;
}

/*
 * Prints list of completion words in WeeChat log file (usually for crash dump).
 */

void
gui_completion_list_words_print_log (struct t_arraylist *list,
                                     const char *name)
{
    int i;
    struct t_gui_completion_word *ptr_completion_word;

    for (i = 0; i < list->size; i++)
    {
        ptr_completion_word = (struct t_gui_completion_word *)(list->data[i]);
        log_printf ("[%s (addr:%p)]", name, ptr_completion_word);
        log_printf ("  word. . . . . . . . . . : '%s'", ptr_completion_word->word);
        log_printf ("  nicklist_completion . . : %d", ptr_completion_word->nick_completion);
        log_printf ("  count . . . . . . . . . : %d", ptr_completion_word->count);
    }
}

/*
 * Prints completion list in WeeChat log file (usually for crash dump).
 */

void
gui_completion_print_log ()
{
    struct t_gui_completion *ptr_completion;

    for (ptr_completion = weechat_completions; ptr_completion;
         ptr_completion = ptr_completion->next_completion)
    {
        log_printf ("[completion (addr:%p)]", ptr_completion);
        log_printf ("  plugin. . . . . . . . . . : %p", ptr_completion->plugin);
        log_printf ("  buffer. . . . . . . . . . : %p ('%s')",
                    ptr_completion->buffer,
                    ptr_completion->buffer->full_name);
        log_printf ("  context . . . . . . . . . : %d", ptr_completion->context);
        log_printf ("  case_sensitive. . . . . . : %d", ptr_completion->case_sensitive);
        log_printf ("  base_command. . . . . . . : '%s'", ptr_completion->base_command);
        log_printf ("  base_command_arg_index. . : %d", ptr_completion->base_command_arg_index);
        log_printf ("  base_word . . . . . . . . : '%s'", ptr_completion->base_word);
        log_printf ("  base_word_pos . . . . . . : %d", ptr_completion->base_word_pos);
        log_printf ("  position. . . . . . . . . : %d", ptr_completion->position);
        log_printf ("  args. . . . . . . . . . . : '%s'", ptr_completion->args);
        log_printf ("  direction . . . . . . . . : %d", ptr_completion->direction);
        log_printf ("  add_space . . . . . . . . : %d", ptr_completion->add_space);
        log_printf ("  force_partial_completion. : %d", ptr_completion->force_partial_completion);
        log_printf ("  reverse_partial_completion: %d", ptr_completion->reverse_partial_completion);
        log_printf ("  list. . . . . . . . . . . : %p", ptr_completion->list);
        log_printf ("  word_found. . . . . . . . : '%s'", ptr_completion->word_found);
        log_printf ("  word_found_is_nick. . . . : %d", ptr_completion->word_found_is_nick);
        log_printf ("  position_replace. . . . . : %d", ptr_completion->position_replace);
        log_printf ("  diff_size . . . . . . . . : %d", ptr_completion->diff_size);
        log_printf ("  diff_length . . . . . . . : %d", ptr_completion->diff_length);
        log_printf ("  prev_completion . . . . . : %p", ptr_completion->prev_completion);
        log_printf ("  next_completion . . . . . : %p", ptr_completion->next_completion);
        if (ptr_completion->list && (ptr_completion->list->size > 0))
        {
            log_printf ("");
            gui_completion_list_words_print_log (ptr_completion->list,
                                                 "completion word");
        }
        if (ptr_completion->partial_list)
        {
            log_printf ("");
            arraylist_print_log (ptr_completion->partial_list,
                                 "partial completion word");
        }
    }
}
