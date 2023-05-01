/*
 * plugin-api.c - extra functions for plugin API
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include <gcrypt.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-crypto.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-input.h"
#include "../core/wee-proxy.h"
#include "../core/wee-string.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-bar-window.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-completion.h"
#include "../gui/gui-color.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-history.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-line.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "plugin.h"
#include "plugin-api.h"
#include "plugin-api-info.h"
#include "plugin-config.h"


/*
 * Sets plugin charset.
 */

void
plugin_api_charset_set (struct t_weechat_plugin *plugin, const char *charset)
{
    if (!plugin || !charset)
        return;

    if (plugin->charset)
        free (plugin->charset);

    plugin->charset = (charset) ? strdup (charset) : NULL;
}

/*
 * Translates a string using gettext.
 */

const char *
plugin_api_gettext (const char *string)
{
    return _(string);
}

/*
 * Translates a string using gettext (with plural form).
 */

const char *
plugin_api_ngettext (const char *single, const char *plural, int count)
{
    /* make C compiler happy */
    (void) single;
    (void) count;

    return NG_(single, plural, count);
}

/*
 * Computes hash of data using the given algorithm.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
plugin_api_crypto_hash (const void *data, int data_size, const char *hash_algo,
                        void *hash, int *hash_size)
{
    int algo;

    if (!hash)
        return 0;

    if (hash_size)
        *hash_size = 0;

    if (!data || (data_size < 1) || !hash_algo)
        return 0;

    algo = weecrypto_get_hash_algo (hash_algo);
    if (algo == GCRY_MD_NONE)
        return 0;

    return weecrypto_hash (data, data_size, algo, hash, hash_size);
}

/*
 * Computes hash of a file using the given algorithm.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
plugin_api_crypto_hash_file (const char *filename, const char *hash_algo,
                             void *hash, int *hash_size)
{
    int algo;

    if (!hash)
        return 0;

    if (hash_size)
        *hash_size = 0;

    if (!filename || !filename[0] || !hash_algo)
        return 0;

    algo = weecrypto_get_hash_algo (hash_algo);
    if (algo == GCRY_MD_NONE)
        return 0;

    return weecrypto_hash_file (filename, algo, hash, hash_size);
}

/*
 * Computes PKCS#5 Passphrase Based Key Derivation Function number 2 (PBKDF2)
 * hash of data.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
plugin_api_crypto_hash_pbkdf2 (const void *data, int data_size,
                               const char *hash_algo,
                               const void *salt, int salt_size,
                               int iterations,
                               void *hash, int *hash_size)
{
    int algo;

    if (!hash)
        return 0;

    if (hash_size)
        *hash_size = 0;

    if (!data || (data_size < 1) || !hash_algo || !salt || (salt_size < 1))
        return 0;

    algo = weecrypto_get_hash_algo (hash_algo);
    if (algo == GCRY_MD_NONE)
        return 0;

    return weecrypto_hash_pbkdf2 (data, data_size, algo, salt, salt_size,
                                  iterations, hash, hash_size);
}

/*
 * Computes HMAC of key + message using the given algorithm.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
plugin_api_crypto_hmac (const void *key, int key_size,
                        const void *message, int message_size,
                        const char *hash_algo,
                        void *hash, int *hash_size)
{
    int algo;

    if (!hash)
        return 0;

    if (hash_size)
        *hash_size = 0;

    if (!key || (key_size < 1) || !message || (message_size < 1) || !hash_algo)
        return 0;

    algo = weecrypto_get_hash_algo (hash_algo);
    if (algo == GCRY_MD_NONE)
        return 0;

    return weecrypto_hmac (key, key_size, message, message_size,
                           algo, hash, hash_size);
}

/*
 * Frees an option.
 */

void
plugin_api_config_file_option_free (struct t_config_option *option)
{
    config_file_option_free (option, 1);
}

/*
 * Gets pointer on an option.
 */

struct t_config_option *
plugin_api_config_get (const char *option_name)
{
    struct t_config_option *ptr_option;

    config_file_search_with_string (option_name, NULL, NULL, &ptr_option, NULL);

    return ptr_option;
}

/*
 * Gets value of a plugin option.
 */

const char *
plugin_api_config_get_plugin (struct t_weechat_plugin *plugin,
                              const char *option_name)
{
    struct t_config_option *ptr_option;

    if (!plugin || !option_name)
        return NULL;

    ptr_option = plugin_config_search (plugin->name, option_name);
    if (ptr_option)
        return ptr_option->value;

    /* option not found */
    return NULL;
}

/*
 * Checks if a plugin option is set.
 *
 * Returns:
 *   1: plugin option is set
 *   0: plugin option does not exist
 */

int
plugin_api_config_is_set_plugin (struct t_weechat_plugin *plugin,
                                 const char *option_name)
{
    struct t_config_option *ptr_option;

    if (!plugin || !option_name)
        return 0;

    ptr_option = plugin_config_search (plugin->name, option_name);
    if (ptr_option)
        return 1;

    return 0;
}

/*
 * Sets value of a plugin option.
 */

int
plugin_api_config_set_plugin (struct t_weechat_plugin *plugin,
                              const char *option_name, const char *value)
{
    if (!plugin || !option_name)
        return WEECHAT_CONFIG_OPTION_SET_OPTION_NOT_FOUND;

    return plugin_config_set (plugin->name, option_name, value);
}

/*
 * Sets description of a plugin option.
 */

void
plugin_api_config_set_desc_plugin (struct t_weechat_plugin *plugin,
                                   const char *option_name,
                                   const char *description)
{
    if (plugin && option_name)
        plugin_config_set_desc (plugin->name, option_name, description);
}

/*
 * Unsets a plugin option.
 */

int
plugin_api_config_unset_plugin (struct t_weechat_plugin *plugin,
                                const char *option_name)
{
    struct t_config_option *ptr_option;

    if (!plugin || !option_name)
        return WEECHAT_CONFIG_OPTION_UNSET_ERROR;

    ptr_option = plugin_config_search (plugin->name, option_name);
    if (!ptr_option)
        return WEECHAT_CONFIG_OPTION_UNSET_ERROR;

    return config_file_option_unset (ptr_option);
}

/*
 * Returns a prefix for display with printf.
 */

const char *
plugin_api_prefix (const char *prefix)
{
    if (!prefix)
        return gui_chat_prefix_empty;

    if (strcmp (prefix, "error") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_ERROR];
    if (strcmp (prefix, "network") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_NETWORK];
    if (strcmp (prefix, "action") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_ACTION];
    if (strcmp (prefix, "join") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_JOIN];
    if (strcmp (prefix, "quit") == 0)
        return gui_chat_prefix[GUI_CHAT_PREFIX_QUIT];

    return gui_chat_prefix_empty;
}

/*
 * Returns a WeeChat color for display with printf.
 */

const char *
plugin_api_color (const char *color_name)
{
    const char *str_color;

    if (!color_name)
        return GUI_NO_COLOR;

    /* name is a weechat color option ? => then return this color */
    str_color = gui_color_search_config (color_name);
    if (str_color)
        return str_color;

    return gui_color_get_custom (color_name);
}

/*
 * Executes a command on a buffer (simulates user entry) with options.
 */

int
plugin_api_command_options (struct t_weechat_plugin *plugin,
                            struct t_gui_buffer *buffer, const char *command,
                            struct t_hashtable *options)
{
    char *command2, *error;
    const char *ptr_commands_allowed, *ptr_delay, *ptr_split_newline;
    long delay;
    int rc, split_newline;

    if (!plugin || !command)
        return WEECHAT_RC_ERROR;

    ptr_commands_allowed = NULL;
    delay = 0;
    split_newline = 0;

    if (options)
    {
        ptr_commands_allowed = hashtable_get (options, "commands");
        ptr_delay = hashtable_get (options, "delay");
        if (ptr_delay)
        {
            error = NULL;
            delay = strtol (ptr_delay, &error, 10);
            if (!error || error[0])
                delay = 0;
        }
        ptr_split_newline = hashtable_get (options, "split_newline");
        if (ptr_split_newline)
        {
            split_newline = (string_strcmp (ptr_split_newline, "1") == 0) ?
                1 : 0;
        }
    }

    command2 = string_iconv_to_internal (plugin->charset, command);

    rc = input_data_delayed ((buffer) ? buffer : gui_current_window->buffer,
                             (command2) ? command2 : command,
                             ptr_commands_allowed,
                             split_newline,
                             delay);

    if (command2)
        free (command2);

    return rc;
}

/*
 * Executes a command on a buffer (simulates user entry).
 */

int
plugin_api_command (struct t_weechat_plugin *plugin,
                    struct t_gui_buffer *buffer, const char *command)
{
    return plugin_api_command_options (plugin, buffer, command, NULL);
}

/*
 * Modifier callback: decodes ANSI colors: converts ANSI color codes to WeeChat
 * colors (or removes them).
 */

char *
plugin_api_modifier_color_decode_ansi_cb (const void *pointer, void *data,
                                          const char *modifier,
                                          const char *modifier_data,
                                          const char *string)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier;

    return gui_color_decode_ansi (
        string,
        (modifier_data && (strcmp (modifier_data, "1") == 0)) ?
        1: 0);
}

/*
 * Modifier callback: encodes ANSI colors: converts WeeChat colors to ANSI
 * color codes.
 */

char *
plugin_api_modifier_color_encode_ansi_cb (const void *pointer, void *data,
                                          const char *modifier,
                                          const char *modifier_data,
                                          const char *string)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier;
    (void) modifier_data;

    return gui_color_encode_ansi (string);
}

/*
 * Modifier callback: evaluates a home path.
 */

char *
plugin_api_modifier_eval_path_home_cb (const void *pointer, void *data,
                                       const char *modifier,
                                       const char *modifier_data,
                                       const char *string)
{
    struct t_hashtable *options;
    const char *ptr_directory;
    char *result;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier;

    options = NULL;
    ptr_directory = (modifier_data
                     && (strncmp (modifier_data, "directory=", 10) == 0)) ?
        modifier_data + 10 : NULL;
    if (ptr_directory)
    {
        options = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
        if (options)
            hashtable_set (options, "directory", ptr_directory);
    }

    result = string_eval_path_home (string, NULL, NULL, options);

    if (options)
        hashtable_free (options);

    return result;
}

/*
 * Moves item pointer to next item in an infolist.
 *
 * Returns:
 *   1: pointer is still OK
 *   0: end of infolist was reached
 */

int
plugin_api_infolist_next (struct t_infolist *infolist)
{
    if (!infolist || !infolist_valid (infolist))
        return 0;

    return (infolist_next (infolist)) ? 1 : 0;
}

/*
 * Moves pointer to previous item in an infolist.
 *
 * Returns:
 *   1: pointer is still OK
 *   0: beginning of infolist was reached
 */

int
plugin_api_infolist_prev (struct t_infolist *infolist)
{
    if (!infolist || !infolist_valid (infolist))
        return 0;

    return (infolist_prev (infolist)) ? 1 : 0;
}

/*
 * Resets item cursor in infolist.
 */

void
plugin_api_infolist_reset_item_cursor (struct t_infolist *infolist)
{
    if (infolist && infolist_valid (infolist))
    {
        infolist_reset_item_cursor (infolist);
    }
}

/*
 * Gets list of fields for current infolist item.
 */

const char *
plugin_api_infolist_fields (struct t_infolist *infolist)
{
    if (!infolist || !infolist_valid (infolist))
        return NULL;

    return infolist_fields (infolist);
}

/*
 * Gets integer value for a variable in current infolist item.
 */

int
plugin_api_infolist_integer (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return 0;

    return infolist_integer (infolist, var);
}

/*
 * Gets string value for a variable in current infolist item.
 */

const char *
plugin_api_infolist_string (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return NULL;

    return infolist_string (infolist, var);
}

/*
 * Gets pointer value for a variable in current infolist item.
 */

void *
plugin_api_infolist_pointer (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return NULL;

    return infolist_pointer (infolist, var);
}

/*
 * Gets buffer value for a variable in current infolist item.
 *
 * Argument "size" is set with the size of buffer.
 */

void *
plugin_api_infolist_buffer (struct t_infolist *infolist, const char *var,
                            int *size)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return NULL;

    return infolist_buffer (infolist, var, size);
}

/*
 * Gets time value for a variable in current infolist item.
 */

time_t
plugin_api_infolist_time (struct t_infolist *infolist, const char *var)
{
    if (!infolist || !infolist_valid (infolist) || !infolist->ptr_item)
        return 0;

    return infolist_time (infolist, var);
}

/*
 * Frees an infolist.
 */

void
plugin_api_infolist_free (struct t_infolist *infolist)
{
    if (infolist && infolist_valid (infolist))
        infolist_free (infolist);
}

/*
 * Initializes plugin API.
 */

void
plugin_api_init ()
{
    /* WeeChat core modifiers */
    hook_modifier (NULL, "color_decode_ansi",
                   &plugin_api_modifier_color_decode_ansi_cb, NULL, NULL);
    hook_modifier (NULL, "color_encode_ansi",
                   &plugin_api_modifier_color_encode_ansi_cb, NULL, NULL);
    hook_modifier (NULL, "eval_path_home",
                   &plugin_api_modifier_eval_path_home_cb, NULL, NULL);

    /* WeeChat core info/infolist hooks */
    plugin_api_info_init ();

    /* WeeChat core hdata */
    hook_hdata (NULL, "bar", N_("bar"),
                &gui_bar_hdata_bar_cb, NULL, NULL);
    hook_hdata (NULL, "bar_item", N_("bar item"),
                &gui_bar_item_hdata_bar_item_cb, NULL, NULL);
    hook_hdata (NULL, "bar_window", N_("bar window"),
                &gui_bar_window_hdata_bar_window_cb, NULL, NULL);
    hook_hdata (NULL, "buffer", N_("buffer"),
                &gui_buffer_hdata_buffer_cb, NULL, NULL);
    hook_hdata (NULL, "buffer_visited", N_("visited buffer"),
                &gui_buffer_hdata_buffer_visited_cb, NULL, NULL);
    hook_hdata (NULL, "completion", N_("structure with completion"),
                &gui_completion_hdata_completion_cb, NULL, NULL);
    hook_hdata (NULL, "completion_word",
                N_("structure with word found for a completion"),
                &gui_completion_hdata_completion_word_cb, NULL, NULL);
    hook_hdata (NULL, "config_file", N_("config file"),
                &config_file_hdata_config_file_cb, NULL, NULL);
    hook_hdata (NULL, "config_section", N_("config section"),
                &config_file_hdata_config_section_cb, NULL, NULL);
    hook_hdata (NULL, "config_option", N_("config option"),
                &config_file_hdata_config_option_cb, NULL, NULL);
    hook_hdata (NULL, "filter", N_("filter"),
                &gui_filter_hdata_filter_cb, NULL, NULL);
    hook_hdata (NULL, "history", N_("history of commands in buffer"),
                &gui_history_hdata_history_cb, NULL, NULL);
    hook_hdata (NULL, "hotlist", N_("hotlist"),
                &gui_hotlist_hdata_hotlist_cb, NULL, NULL);
    hook_hdata (NULL, "input_undo", N_("structure with undo for input line"),
                &gui_buffer_hdata_input_undo_cb, NULL, NULL);
    hook_hdata (NULL, "key", N_("a key (keyboard shortcut)"),
                &gui_key_hdata_key_cb, NULL, NULL);
    hook_hdata (NULL, "layout", N_("layout"),
                &gui_layout_hdata_layout_cb, NULL, NULL);
    hook_hdata (NULL, "layout_buffer", N_("buffer layout"),
                &gui_layout_hdata_layout_buffer_cb, NULL, NULL);
    hook_hdata (NULL, "layout_window", N_("window layout"),
                &gui_layout_hdata_layout_window_cb, NULL, NULL);
    hook_hdata (NULL, "lines", N_("structure with lines"),
                &gui_line_hdata_lines_cb, NULL, NULL);
    hook_hdata (NULL, "line", N_("structure with one line"),
                &gui_line_hdata_line_cb, NULL, NULL);
    hook_hdata (NULL, "line_data", N_("structure with one line data"),
                &gui_line_hdata_line_data_cb, NULL, NULL);
    hook_hdata (NULL, "nick_group", N_("group in nicklist"),
                &gui_nicklist_hdata_nick_group_cb, NULL, NULL);
    hook_hdata (NULL, "nick", N_("nick in nicklist"),
                &gui_nicklist_hdata_nick_cb, NULL, NULL);
    hook_hdata (NULL, "plugin", N_("plugin"),
                &plugin_hdata_plugin_cb, NULL, NULL);
    hook_hdata (NULL, "proxy", N_("proxy"),
                &proxy_hdata_proxy_cb, NULL, NULL);
    hook_hdata (NULL, "window", N_("window"),
                &gui_window_hdata_window_cb, NULL, NULL);
    hook_hdata (NULL, "window_scroll", N_("scroll info in window"),
                &gui_window_hdata_window_scroll_cb, NULL, NULL);
    hook_hdata (NULL, "window_tree", N_("tree of windows"),
                &gui_window_hdata_window_tree_cb, NULL, NULL);
}
