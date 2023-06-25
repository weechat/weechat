/*
 * wee-config.c - WeeChat configuration options (file weechat.conf)
 *
 * Copyright (C) 2003-2023 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2006 Emmanuel Bouthenot <kolter@openics.org>
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
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <regex.h>

#include "weechat.h"
#include "wee-config.h"
#include "wee-eval.h"
#include "wee-hashtable.h"
#include "wee-hook.h"
#include "wee-log.h"
#include "wee-network.h"
#include "wee-utf8.h"
#include "wee-util.h"
#include "wee-list.h"
#include "wee-proxy.h"
#include "wee-string.h"
#include "wee-version.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-bar-item-custom.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-line.h"
#include "../gui/gui-main.h"
#include "../gui/gui-mouse.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


/* weechat config */
struct t_config_file *weechat_config_file = NULL;

/* sections */
struct t_config_section *weechat_config_section_debug = NULL;
struct t_config_section *weechat_config_section_startup = NULL;
struct t_config_section *weechat_config_section_look = NULL;
struct t_config_section *weechat_config_section_palette = NULL;
struct t_config_section *weechat_config_section_color = NULL;
struct t_config_section *weechat_config_section_completion = NULL;
struct t_config_section *weechat_config_section_history = NULL;
struct t_config_section *weechat_config_section_proxy = NULL;
struct t_config_section *weechat_config_section_network = NULL;
struct t_config_section *weechat_config_section_plugin = NULL;
struct t_config_section *weechat_config_section_signal = NULL;
struct t_config_section *weechat_config_section_bar = NULL;
struct t_config_section *weechat_config_section_custom_bar_item = NULL;
struct t_config_section *weechat_config_section_layout = NULL;
struct t_config_section *weechat_config_section_notify = NULL;
struct t_config_section *weechat_config_section_filter = NULL;
struct t_config_section *weechat_config_section_key[GUI_KEY_NUM_CONTEXTS] = {
    NULL, NULL, NULL, NULL,
};

/* config, startup section */

struct t_config_option *config_startup_command_after_plugins = NULL;
struct t_config_option *config_startup_command_before_plugins = NULL;
struct t_config_option *config_startup_display_logo = NULL;
struct t_config_option *config_startup_display_version = NULL;
struct t_config_option *config_startup_sys_rlimit = NULL;

/* config, look & feel section */

struct t_config_option *config_look_align_end_of_lines = NULL;
struct t_config_option *config_look_align_multiline_words = NULL;
struct t_config_option *config_look_bar_more_down = NULL;
struct t_config_option *config_look_bar_more_left = NULL;
struct t_config_option *config_look_bar_more_right = NULL;
struct t_config_option *config_look_bar_more_up = NULL;
struct t_config_option *config_look_bare_display_exit_on_input = NULL;
struct t_config_option *config_look_bare_display_time_format = NULL;
struct t_config_option *config_look_buffer_auto_renumber = NULL;
struct t_config_option *config_look_buffer_notify_default = NULL;
struct t_config_option *config_look_buffer_position = NULL;
struct t_config_option *config_look_buffer_search_case_sensitive = NULL;
struct t_config_option *config_look_buffer_search_force_default = NULL;
struct t_config_option *config_look_buffer_search_regex = NULL;
struct t_config_option *config_look_buffer_search_where = NULL;
struct t_config_option *config_look_buffer_time_format = NULL;
struct t_config_option *config_look_buffer_time_same = NULL;
struct t_config_option *config_look_chat_space_right = NULL;
struct t_config_option *config_look_color_basic_force_bold = NULL;
struct t_config_option *config_look_color_inactive_buffer = NULL;
struct t_config_option *config_look_color_inactive_message = NULL;
struct t_config_option *config_look_color_inactive_prefix = NULL;
struct t_config_option *config_look_color_inactive_prefix_buffer = NULL;
struct t_config_option *config_look_color_inactive_time = NULL;
struct t_config_option *config_look_color_inactive_window = NULL;
struct t_config_option *config_look_color_nick_offline = NULL;
struct t_config_option *config_look_color_pairs_auto_reset = NULL;
struct t_config_option *config_look_color_real_white = NULL;
struct t_config_option *config_look_command_chars = NULL;
struct t_config_option *config_look_command_incomplete = NULL;
struct t_config_option *config_look_confirm_quit = NULL;
struct t_config_option *config_look_confirm_upgrade = NULL;
struct t_config_option *config_look_day_change = NULL;
struct t_config_option *config_look_day_change_message_1date = NULL;
struct t_config_option *config_look_day_change_message_2dates = NULL;
struct t_config_option *config_look_eat_newline_glitch = NULL;
struct t_config_option *config_look_emphasized_attributes = NULL;
struct t_config_option *config_look_highlight = NULL;
struct t_config_option *config_look_highlight_disable_regex = NULL;
struct t_config_option *config_look_highlight_regex = NULL;
struct t_config_option *config_look_highlight_tags = NULL;
struct t_config_option *config_look_hotlist_add_conditions = NULL;
struct t_config_option *config_look_hotlist_buffer_separator = NULL;
struct t_config_option *config_look_hotlist_count_max = NULL;
struct t_config_option *config_look_hotlist_count_min_msg = NULL;
struct t_config_option *config_look_hotlist_names_count = NULL;
struct t_config_option *config_look_hotlist_names_length = NULL;
struct t_config_option *config_look_hotlist_names_level = NULL;
struct t_config_option *config_look_hotlist_names_merged_buffers = NULL;
struct t_config_option *config_look_hotlist_prefix = NULL;
struct t_config_option *config_look_hotlist_remove = NULL;
struct t_config_option *config_look_hotlist_short_names = NULL;
struct t_config_option *config_look_hotlist_sort = NULL;
struct t_config_option *config_look_hotlist_suffix = NULL;
struct t_config_option *config_look_hotlist_unique_numbers = NULL;
struct t_config_option *config_look_hotlist_update_on_buffer_switch = NULL;
struct t_config_option *config_look_input_cursor_scroll = NULL;
struct t_config_option *config_look_input_multiline_lead_linebreak = NULL;
struct t_config_option *config_look_input_share = NULL;
struct t_config_option *config_look_input_share_overwrite = NULL;
struct t_config_option *config_look_input_undo_max = NULL;
struct t_config_option *config_look_item_away_message = NULL;
struct t_config_option *config_look_item_buffer_filter = NULL;
struct t_config_option *config_look_item_buffer_zoom = NULL;
struct t_config_option *config_look_item_mouse_status = NULL;
struct t_config_option *config_look_item_time_format = NULL;
struct t_config_option *config_look_jump_current_to_previous_buffer = NULL;
struct t_config_option *config_look_jump_previous_buffer_when_closing = NULL;
struct t_config_option *config_look_jump_smart_back_to_buffer = NULL;
struct t_config_option *config_look_key_bind_safe = NULL;
struct t_config_option *config_look_key_grab_delay = NULL;
struct t_config_option *config_look_mouse = NULL;
struct t_config_option *config_look_mouse_timer_delay = NULL;
struct t_config_option *config_look_nick_color_force = NULL;
struct t_config_option *config_look_nick_color_hash = NULL;
struct t_config_option *config_look_nick_color_hash_salt = NULL;
struct t_config_option *config_look_nick_color_stop_chars = NULL;
struct t_config_option *config_look_nick_prefix = NULL;
struct t_config_option *config_look_nick_suffix = NULL;
struct t_config_option *config_look_paste_bracketed = NULL;
struct t_config_option *config_look_paste_bracketed_timer_delay = NULL;
struct t_config_option *config_look_paste_max_lines = NULL;
struct t_config_option *config_look_prefix[GUI_CHAT_NUM_PREFIXES] = {
    NULL, NULL, NULL, NULL, NULL,
};
struct t_config_option *config_look_prefix_align = NULL;
struct t_config_option *config_look_prefix_align_max = NULL;
struct t_config_option *config_look_prefix_align_min = NULL;
struct t_config_option *config_look_prefix_align_more = NULL;
struct t_config_option *config_look_prefix_align_more_after = NULL;
struct t_config_option *config_look_prefix_buffer_align = NULL;
struct t_config_option *config_look_prefix_buffer_align_max = NULL;
struct t_config_option *config_look_prefix_buffer_align_more = NULL;
struct t_config_option *config_look_prefix_buffer_align_more_after = NULL;
struct t_config_option *config_look_prefix_same_nick = NULL;
struct t_config_option *config_look_prefix_same_nick_middle = NULL;
struct t_config_option *config_look_prefix_suffix = NULL;
struct t_config_option *config_look_quote_nick_prefix = NULL;
struct t_config_option *config_look_quote_nick_suffix = NULL;
struct t_config_option *config_look_quote_time_format = NULL;
struct t_config_option *config_look_read_marker = NULL;
struct t_config_option *config_look_read_marker_always_show = NULL;
struct t_config_option *config_look_read_marker_string = NULL;
struct t_config_option *config_look_read_marker_update_on_buffer_switch = NULL;
struct t_config_option *config_look_save_config_on_exit = NULL;
struct t_config_option *config_look_save_config_with_fsync = NULL;
struct t_config_option *config_look_save_layout_on_exit = NULL;
struct t_config_option *config_look_scroll_amount = NULL;
struct t_config_option *config_look_scroll_bottom_after_switch = NULL;
struct t_config_option *config_look_scroll_page_percent = NULL;
struct t_config_option *config_look_search_text_not_found_alert = NULL;
struct t_config_option *config_look_separator_horizontal = NULL;
struct t_config_option *config_look_separator_vertical = NULL;
struct t_config_option *config_look_tab_width = NULL;
struct t_config_option *config_look_time_format = NULL;
struct t_config_option *config_look_window_auto_zoom = NULL;
struct t_config_option *config_look_window_separator_horizontal = NULL;
struct t_config_option *config_look_window_separator_vertical = NULL;
struct t_config_option *config_look_window_title = NULL;
struct t_config_option *config_look_word_chars_highlight = NULL;
struct t_config_option *config_look_word_chars_input = NULL;

/* config, colors section */

struct t_config_option *config_color_bar_more = NULL;
struct t_config_option *config_color_chat = NULL;
struct t_config_option *config_color_chat_bg = NULL;
struct t_config_option *config_color_chat_buffer = NULL;
struct t_config_option *config_color_chat_channel = NULL;
struct t_config_option *config_color_chat_day_change = NULL;
struct t_config_option *config_color_chat_delimiters = NULL;
struct t_config_option *config_color_chat_highlight = NULL;
struct t_config_option *config_color_chat_highlight_bg = NULL;
struct t_config_option *config_color_chat_host = NULL;
struct t_config_option *config_color_chat_inactive_buffer = NULL;
struct t_config_option *config_color_chat_inactive_window = NULL;
struct t_config_option *config_color_chat_nick = NULL;
struct t_config_option *config_color_chat_nick_colors = NULL;
struct t_config_option *config_color_chat_nick_offline = NULL;
struct t_config_option *config_color_chat_nick_offline_highlight = NULL;
struct t_config_option *config_color_chat_nick_offline_highlight_bg = NULL;
struct t_config_option *config_color_chat_nick_other = NULL;
struct t_config_option *config_color_chat_nick_prefix = NULL;
struct t_config_option *config_color_chat_nick_self = NULL;
struct t_config_option *config_color_chat_nick_suffix = NULL;
struct t_config_option *config_color_chat_prefix[GUI_CHAT_NUM_PREFIXES] = {
    NULL, NULL, NULL, NULL, NULL,
};
struct t_config_option *config_color_chat_prefix_buffer = NULL;
struct t_config_option *config_color_chat_prefix_buffer_inactive_buffer = NULL;
struct t_config_option *config_color_chat_prefix_more = NULL;
struct t_config_option *config_color_chat_prefix_suffix = NULL;
struct t_config_option *config_color_chat_read_marker = NULL;
struct t_config_option *config_color_chat_read_marker_bg = NULL;
struct t_config_option *config_color_chat_server = NULL;
struct t_config_option *config_color_chat_status_disabled = NULL;
struct t_config_option *config_color_chat_status_enabled = NULL;
struct t_config_option *config_color_chat_tags = NULL;
struct t_config_option *config_color_chat_text_found = NULL;
struct t_config_option *config_color_chat_text_found_bg = NULL;
struct t_config_option *config_color_chat_time = NULL;
struct t_config_option *config_color_chat_time_delimiters = NULL;
struct t_config_option *config_color_chat_value = NULL;
struct t_config_option *config_color_chat_value_null = NULL;
struct t_config_option *config_color_emphasized = NULL;
struct t_config_option *config_color_emphasized_bg = NULL;
struct t_config_option *config_color_input_actions = NULL;
struct t_config_option *config_color_input_text_not_found = NULL;
struct t_config_option *config_color_item_away = NULL;
struct t_config_option *config_color_nicklist_away = NULL;
struct t_config_option *config_color_nicklist_group = NULL;
struct t_config_option *config_color_separator = NULL;
struct t_config_option *config_color_status_count_highlight = NULL;
struct t_config_option *config_color_status_count_msg = NULL;
struct t_config_option *config_color_status_count_other = NULL;
struct t_config_option *config_color_status_count_private = NULL;
struct t_config_option *config_color_status_data_highlight = NULL;
struct t_config_option *config_color_status_data_msg = NULL;
struct t_config_option *config_color_status_data_other = NULL;
struct t_config_option *config_color_status_data_private = NULL;
struct t_config_option *config_color_status_filter = NULL;
struct t_config_option *config_color_status_more = NULL;
struct t_config_option *config_color_status_mouse = NULL;
struct t_config_option *config_color_status_name = NULL;
struct t_config_option *config_color_status_name_insecure = NULL;
struct t_config_option *config_color_status_name_tls = NULL;
struct t_config_option *config_color_status_nicklist_count = NULL;
struct t_config_option *config_color_status_number = NULL;
struct t_config_option *config_color_status_time = NULL;

/* config, completion section */

struct t_config_option *config_completion_base_word_until_cursor = NULL;
struct t_config_option *config_completion_command_inline = NULL;
struct t_config_option *config_completion_default_template = NULL;
struct t_config_option *config_completion_nick_add_space = NULL;
struct t_config_option *config_completion_nick_case_sensitive = NULL;
struct t_config_option *config_completion_nick_completer = NULL;
struct t_config_option *config_completion_nick_first_only = NULL;
struct t_config_option *config_completion_nick_ignore_chars = NULL;
struct t_config_option *config_completion_partial_completion_alert = NULL;
struct t_config_option *config_completion_partial_completion_command = NULL;
struct t_config_option *config_completion_partial_completion_command_arg = NULL;
struct t_config_option *config_completion_partial_completion_count = NULL;
struct t_config_option *config_completion_partial_completion_other = NULL;
struct t_config_option *config_completion_partial_completion_templates = NULL;

/* config, history section */

struct t_config_option *config_history_display_default = NULL;
struct t_config_option *config_history_max_buffer_lines_minutes = NULL;
struct t_config_option *config_history_max_buffer_lines_number = NULL;
struct t_config_option *config_history_max_commands = NULL;
struct t_config_option *config_history_max_visited_buffers = NULL;

/* config, network section */

struct t_config_option *config_network_connection_timeout = NULL;
struct t_config_option *config_network_gnutls_ca_system = NULL;
struct t_config_option *config_network_gnutls_ca_user = NULL;
struct t_config_option *config_network_gnutls_handshake_timeout = NULL;
struct t_config_option *config_network_proxy_curl = NULL;

/* config, plugin section */

struct t_config_option *config_plugin_autoload = NULL;
struct t_config_option *config_plugin_extension = NULL;
struct t_config_option *config_plugin_path = NULL;
struct t_config_option *config_plugin_save_config_on_unload = NULL;

/* config, signal section */

struct t_config_option *config_signal_sighup = NULL;
struct t_config_option *config_signal_sigquit = NULL;
struct t_config_option *config_signal_sigterm = NULL;
struct t_config_option *config_signal_sigusr1 = NULL;
struct t_config_option *config_signal_sigusr2 = NULL;

/* other */

int config_loading = 0;
int config_length_nick_prefix_suffix = 0;
int config_length_prefix_same_nick = 0;
int config_length_prefix_same_nick_middle = 0;
struct t_hook *config_day_change_timer = NULL;
int config_day_change_old_day = -1;
int config_emphasized_attributes = 0;
regex_t *config_highlight_disable_regex = NULL;
regex_t *config_highlight_regex = NULL;
char ***config_highlight_tags = NULL;
int config_num_highlight_tags = 0;
char **config_plugin_extensions = NULL;
int config_num_plugin_extensions = 0;
char config_tab_spaces[TAB_MAX_WIDTH + 1];
struct t_config_look_word_char_item *config_word_chars_highlight = NULL;
int config_word_chars_highlight_count = 0;
struct t_config_look_word_char_item *config_word_chars_input = NULL;
int config_word_chars_input_count = 0;
char **config_nick_colors = NULL;
int config_num_nick_colors = 0;
struct t_hashtable *config_hashtable_nick_color_force = NULL;
char *config_item_time_evaluated = NULL;
char *config_buffer_time_same_evaluated = NULL;
struct t_hashtable *config_hashtable_completion_partial_templates = NULL;


/*
 * Callback for changes on option "weechat.startup.sys_rlimit".
 */

void
config_change_sys_rlimit (const void *pointer, void *data,
                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_init_ok)
        util_setrlimit ();
}

/*
 * Callback for changes on options "weechat.look.save_{config|layout}_on_exit".
 */

void
config_change_save_config_layout_on_exit ()
{
    if (gui_init_ok && !CONFIG_BOOLEAN(config_look_save_config_on_exit)
        && (CONFIG_INTEGER(config_look_save_layout_on_exit) != CONFIG_LOOK_SAVE_LAYOUT_ON_EXIT_NONE))
    {
        gui_chat_printf (NULL,
                         _("Warning: option weechat.look.save_config_on_exit "
                           "is disabled, so the option "
                           "weechat.look.save_layout_on_exit is ignored"));
    }
}

/*
 * Callback for changes on option "weechat.look.save_config_on_exit".
 */

void
config_change_save_config_on_exit (const void *pointer, void *data,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_init_ok && !CONFIG_BOOLEAN(config_look_save_config_on_exit))
    {
        gui_chat_printf (NULL,
                         _("Warning: you should now issue /save to write "
                           "option weechat.look.save_config_on_exit in "
                           "configuration file"));
    }

    config_change_save_config_layout_on_exit ();
}

/*
 * Callback for changes on option "weechat.look.save_layout_on_exit".
 */

void
config_change_save_layout_on_exit (const void *pointer, void *data,
                                   struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_change_save_config_layout_on_exit ();
}

/*
 * Callback for changes on option "weechat.look.window_title".
 */

void
config_change_window_title (const void *pointer, void *data,
                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_init_ok
        || (CONFIG_STRING(config_look_window_title)
            && CONFIG_STRING(config_look_window_title)[0]))
    {
        gui_window_set_title (CONFIG_STRING(config_look_window_title));
    }
}

/*
 * Sets word chars array with a word chars option.
 */

void
config_set_word_chars (const char *str_word_chars,
                       struct t_config_look_word_char_item **word_chars,
                       int *word_chars_count)
{
    char **items, *item, *item2, *ptr_item, *pos;
    int i;

    if (*word_chars)
    {
        free (*word_chars);
        *word_chars = NULL;
    }
    *word_chars_count = 0;

    if (!str_word_chars || !str_word_chars[0])
        return;

    items = string_split (str_word_chars, ",", NULL,
                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                          0, word_chars_count);
    if (!items)
    {
        *word_chars_count = 0;
        return;
    }
    if (*word_chars_count == 0)
        return;

    *word_chars = malloc (
        sizeof ((*word_chars)[0]) * (*word_chars_count));
    if (!*word_chars)
        return;

    for (i = 0; i < *word_chars_count; i++)
    {
        /* init structure */
        (*word_chars)[i].exclude = 0;
        (*word_chars)[i].wc_class = (wctype_t)0;
        (*word_chars)[i].char1 = 0;
        (*word_chars)[i].char2 = 0;

        ptr_item = items[i];
        if ((ptr_item[0] == '!') && ptr_item[1])
        {
            (*word_chars)[i].exclude = 1;
            ptr_item++;
        }

        if (strcmp (ptr_item, "*") != 0)
        {
            pos = strchr (ptr_item, '-');
            if (pos && (pos > ptr_item) && pos[1])
            {
                /* range: char1 -> char2 */
                /* char1 */
                item = string_strndup (ptr_item, pos - ptr_item);
                item2 = string_convert_escaped_chars (item);
                (*word_chars)[i].char1 = utf8_char_int (item2);
                if (item)
                    free (item);
                if (item2)
                    free (item2);
                /* char2 */
                item = strdup (pos + 1);
                item2 = string_convert_escaped_chars (item);
                (*word_chars)[i].char2 = utf8_char_int (item2);
                if (item)
                    free (item);
                if (item2)
                    free (item2);
            }
            else
            {
                /* one char or wide character class */
                (*word_chars)[i].wc_class = wctype (ptr_item);
                if ((*word_chars)[i].wc_class == (wctype_t)0)
                {
                    item = string_convert_escaped_chars (ptr_item);
                    (*word_chars)[i].char1 = utf8_char_int (item);
                    (*word_chars)[i].char2 = (*word_chars)[i].char1;
                    if (item)
                        free (item);
                }
            }
        }
    }

    string_free_split (items);
}

/*
 * Callback for changes on option "weechat.look.word_chars_highlight".
 */

void
config_change_word_chars_highlight (const void *pointer, void *data,
                                    struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_set_word_chars (CONFIG_STRING(config_look_word_chars_highlight),
                           &config_word_chars_highlight,
                           &config_word_chars_highlight_count);
}

/*
 * Callback for changes on option "weechat.look.word_chars_input".
 */

void
config_change_word_chars_input (const void *pointer, void *data,
                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_set_word_chars (CONFIG_STRING(config_look_word_chars_input),
                           &config_word_chars_input,
                           &config_word_chars_input_count);
}

/*
 * Callback for changes on options that require a refresh of buffers.
 */

void
config_change_buffers (const void *pointer, void *data,
                       struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on options that require a refresh of content of buffer.
 */

void
config_change_buffer_content (const void *pointer, void *data,
                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_init_ok)
        gui_current_window->refresh_needed = 1;
}

/*
 * Callback for changes on option "weechat.look.mouse".
 */

void
config_change_mouse (const void *pointer, void *data,
                     struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_init_ok)
    {
        if (CONFIG_BOOLEAN(config_look_mouse))
            gui_mouse_enable ();
        else
            gui_mouse_disable ();
    }
}

/*
 * Callback for changes on option "weechat.look.buffer_auto_renumber".
 */

void
config_change_buffer_auto_renumber (const void *pointer, void *data,
                                    struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_buffers && CONFIG_BOOLEAN(config_look_buffer_auto_renumber))
        gui_buffer_renumber (-1, -1, 1);
}

/*
 * Callback for changes on option "weechat.look.buffer_notify_default".
 */

void
config_change_buffer_notify_default (const void *pointer, void *data,
                                     struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    gui_buffer_notify_set_all ();
}

/*
 * Callback for changes on option "weechat.look.buffer_time_format".
 */

void
config_change_buffer_time_format (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    gui_chat_time_length = gui_chat_get_time_length ();
    gui_chat_change_time_format ();
    if (gui_init_ok)
        gui_window_ask_refresh (1);
}

/*
 * Callback for changes on option "weechat.look.buffer_time_same".
 */

void
config_change_buffer_time_same (const void *pointer, void *data,
                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (config_buffer_time_same_evaluated)
        free (config_buffer_time_same_evaluated);
    config_buffer_time_same_evaluated = eval_expression (
        CONFIG_STRING(config_look_buffer_time_same), NULL, NULL, NULL);

    gui_window_ask_refresh (1);
}

/*
 * Computes the "prefix_max_length" on all buffers.
 */

void
config_compute_prefix_max_length_all_buffers ()
{
    struct t_gui_buffer *ptr_buffer;

    for (ptr_buffer = gui_buffers; ptr_buffer;
         ptr_buffer = ptr_buffer->next_buffer)
    {
        if (ptr_buffer->own_lines)
            ptr_buffer->own_lines->prefix_max_length_refresh = 1;
        if (ptr_buffer->mixed_lines)
            ptr_buffer->mixed_lines->prefix_max_length_refresh = 1;
    }
}

/*
 * Sets nick colors using option "weechat.color.chat_nick_colors".
 */

void
config_set_nick_colors ()
{
    if (config_nick_colors)
    {
        string_free_split (config_nick_colors);
        config_nick_colors = NULL;
        config_num_nick_colors = 0;
    }

    config_nick_colors = string_split (
        CONFIG_STRING(config_color_chat_nick_colors),
        ",",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0, &config_num_nick_colors);
}

/*
 * Callback for changes on option "weechat.look.nick_color_force".
 */

void
config_change_look_nick_color_force (const void *pointer, void *data,
                                     struct t_config_option *option)
{
    char **items, *pos;
    int num_items, i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (!config_hashtable_nick_color_force)
    {
        config_hashtable_nick_color_force = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_STRING,
            NULL, NULL);
    }
    else
    {
        hashtable_remove_all (config_hashtable_nick_color_force);
    }

    items = string_split (CONFIG_STRING(config_look_nick_color_force),
                          ";",
                          NULL,
                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                          0,
                          &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            pos = strchr (items[i], ':');
            if (pos)
            {
                pos[0] = '\0';
                hashtable_set (config_hashtable_nick_color_force,
                               items[i],
                               pos + 1);
            }
        }
        string_free_split (items);
    }
}

/*
 * Callback for changes on options "weechat.look.nick_prefix" and
 * "weechat.look.nick_suffix".
 */

void
config_change_nick_prefix_suffix (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_length_nick_prefix_suffix =
        gui_chat_strlen_screen (CONFIG_STRING(config_look_nick_prefix))
        + gui_chat_strlen_screen (CONFIG_STRING(config_look_nick_suffix));

    config_compute_prefix_max_length_all_buffers ();
    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on option "weechat.look.prefix_same_nick".
 */

void
config_change_prefix_same_nick (const void *pointer, void *data,
                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_length_prefix_same_nick =
        gui_chat_strlen_screen (CONFIG_STRING(config_look_prefix_same_nick));

    config_compute_prefix_max_length_all_buffers ();
    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on option "weechat.look.prefix_same_nick_middle".
 */

void
config_change_prefix_same_nick_middle (const void *pointer, void *data,
                                       struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_length_prefix_same_nick_middle =
        gui_chat_strlen_screen (CONFIG_STRING(config_look_prefix_same_nick_middle));

    config_compute_prefix_max_length_all_buffers ();
    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on option "weechat.look.eat_newline_glitch".
 */

void
config_change_eat_newline_glitch (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_init_ok)
    {
        if (CONFIG_BOOLEAN(config_look_eat_newline_glitch))
        {
            gui_chat_printf (NULL,
                             _("WARNING: this option can cause serious display "
                               "bugs, if you have such problems, you must "
                               "turn off this option."));
            gui_term_set_eat_newline_glitch (0);
        }
        else
            gui_term_set_eat_newline_glitch (1);
    }
}

/*
 * Callback for changes on option "weechat.look.emphasized_attributes".
 */

void
config_change_emphasized_attributes (const void *pointer, void *data,
                                     struct t_config_option *option)
{
    int i;
    const char *ptr_attr;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_emphasized_attributes = 0;

    ptr_attr = CONFIG_STRING(config_look_emphasized_attributes);
    if (ptr_attr)
    {
        for (i = 0; ptr_attr[i]; i++)
        {
            config_emphasized_attributes |= gui_color_attr_get_flag (ptr_attr[i]);
        }
    }

    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on option "weechat.look.highlight_disable_regex".
 */

void
config_change_highlight_disable_regex (const void *pointer, void *data,
                                       struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (config_highlight_disable_regex)
    {
        regfree (config_highlight_disable_regex);
        free (config_highlight_disable_regex);
        config_highlight_disable_regex = NULL;
    }

    if (CONFIG_STRING(config_look_highlight_disable_regex)
        && CONFIG_STRING(config_look_highlight_disable_regex)[0])
    {
        config_highlight_disable_regex = malloc (sizeof (*config_highlight_disable_regex));
        if (config_highlight_disable_regex)
        {
            if (string_regcomp (config_highlight_disable_regex,
                                CONFIG_STRING(config_look_highlight_disable_regex),
                                REG_EXTENDED | REG_ICASE) != 0)
            {
                free (config_highlight_disable_regex);
                config_highlight_disable_regex = NULL;
            }
        }
    }
}

/*
 * Callback for changes on option "weechat.look.highlight_regex".
 */

void
config_change_highlight_regex (const void *pointer, void *data,
                               struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (config_highlight_regex)
    {
        regfree (config_highlight_regex);
        free (config_highlight_regex);
        config_highlight_regex = NULL;
    }

    if (CONFIG_STRING(config_look_highlight_regex)
        && CONFIG_STRING(config_look_highlight_regex)[0])
    {
        config_highlight_regex = malloc (sizeof (*config_highlight_regex));
        if (config_highlight_regex)
        {
            if (string_regcomp (config_highlight_regex,
                                CONFIG_STRING(config_look_highlight_regex),
                                REG_EXTENDED | REG_ICASE) != 0)
            {
                free (config_highlight_regex);
                config_highlight_regex = NULL;
            }
        }
    }
}

/*
 * Callback for changes on option "weechat.look.highlight_tags".
 */

void
config_change_highlight_tags (const void *pointer, void *data,
                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (config_highlight_tags)
    {
        string_free_split_tags (config_highlight_tags);
        config_highlight_tags = NULL;
    }
    config_num_highlight_tags = 0;

    if (CONFIG_STRING(config_look_highlight_tags)
        && CONFIG_STRING(config_look_highlight_tags)[0])
    {
        config_highlight_tags = string_split_tags (
            CONFIG_STRING(config_look_highlight_tags),
            &config_num_highlight_tags);
    }
}

/*
 * Callback for changes on option "weechat.look.hotlist_sort".
 */

void
config_change_hotlist_sort (const void *pointer, void *data,
                            struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    gui_hotlist_resort ();
}

/*
 * Callback for changes on options "weechat.look.item_away_message"
 * and "weechat.color.item_away".
 */

void
config_change_item_away (const void *pointer, void *data,
                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    gui_bar_item_update ("away");
}

/*
 * Callback for changes on options "weechat.look.item_time_format".
 */

void
config_change_item_time_format (const void *pointer, void *data,
                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (config_item_time_evaluated)
        free (config_item_time_evaluated);
    config_item_time_evaluated = eval_expression (
        CONFIG_STRING(config_look_item_time_format), NULL, NULL, NULL);

    config_change_buffer_content (NULL, NULL, NULL);
}

/*
 * Gets the current time formatted for the bar item status.
 */

void
config_get_item_time (char *text_time, int max_length)
{
    time_t date;
    struct tm *local_time;

    if (!config_item_time_evaluated)
        config_change_item_time_format (NULL, NULL, NULL);

    text_time[0] = '\0';

    date = time (NULL);
    local_time = localtime (&date);
    if (strftime (text_time, max_length,
                  config_item_time_evaluated,
                  local_time) == 0)
        text_time[0] = '\0';
}

/*
 * Callback for changes on option "weechat.look.paste_bracketed".
 */

void
config_change_paste_bracketed (const void *pointer, void *data,
                               struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_init_ok)
        gui_window_set_bracketed_paste_mode (CONFIG_BOOLEAN(config_look_paste_bracketed));
}

/*
 * Callback for changes on option "weechat.look.read_marker".
 */

void
config_change_read_marker (const void *pointer, void *data,
                           struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on a prefix option.
 */

void
config_change_prefix (const void *pointer, void *data,
                      struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    gui_chat_prefix_build ();
}

/*
 * Callback for changes on option "weechat.look.prefix_align_min".
 */

void
config_change_prefix_align_min (const void *pointer, void *data,
                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_compute_prefix_max_length_all_buffers ();
    gui_window_ask_refresh (1);
}

/*
 * Checks option "weechat.look.prefix_align_more".
 */

int
config_check_prefix_align_more (const void *pointer, void *data,
                                struct t_config_option *option,
                                const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    return (utf8_strlen_screen (value) == 1) ? 1 : 0;
}

/*
 * Checks option "weechat.look.prefix_buffer_align_more".
 */

int
config_check_prefix_buffer_align_more (const void *pointer, void *data,
                                       struct t_config_option *option,
                                       const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    return (utf8_strlen_screen (value) == 1) ? 1 : 0;
}

/*
 * Checks options "weechat.look.separator_{horizontal|vertical}".
 */

int
config_check_separator (const void *pointer, void *data,
                        struct t_config_option *option,
                        const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    return (utf8_strlen_screen (value) <= 1) ? 1 : 0;
}

/*
 * Callback for changes on option "weechat.look.tab_width".
 */

void
config_change_tab_width (const void *pointer, void *data,
                         struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    memset (config_tab_spaces, ' ', CONFIG_INTEGER(config_look_tab_width));
    config_tab_spaces[CONFIG_INTEGER(config_look_tab_width)] = '\0';

    gui_window_ask_refresh (1);
}

/*
 * Callback for changes on a color option.
 */

void
config_change_color (const void *pointer, void *data,
                     struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_init_ok)
    {
        gui_color_init_weechat ();
        gui_window_ask_refresh (1);
    }
}

/*
 * Callback for changes on option "weechat.color.chat_nick_colors".
 */

void
config_change_nick_colors (const void *pointer, void *data,
                           struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_set_nick_colors ();
    gui_color_buffer_display ();
}

/*
 * Callback for changes on option
 * "weechat.completion.partial_completion_templates".
 */

void
config_change_completion_partial_completion_templates (const void *pointer,
                                                       void *data,
                                                       struct t_config_option *option)
{
    char **items;
    int num_items, i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (!config_hashtable_completion_partial_templates)
    {
        config_hashtable_completion_partial_templates = hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_POINTER,
            NULL, NULL);
    }
    else
    {
        hashtable_remove_all (config_hashtable_completion_partial_templates);
    }

    items = string_split (
        CONFIG_STRING(config_completion_partial_completion_templates),
        ",",
        NULL,
        WEECHAT_STRING_SPLIT_STRIP_LEFT
        | WEECHAT_STRING_SPLIT_STRIP_RIGHT
        | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
        0,
        &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            hashtable_set (config_hashtable_completion_partial_templates,
                           items[i], NULL);
        }
        string_free_split (items);
    }
}

/*
 * Callback for changes on options "weechat.network.gnutls_ca_system"
 * and "weechat.network.gnutls_ca_user".
 */

void
config_change_network_gnutls_ca (const void *pointer, void *data,
                                 struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (network_init_gnutls_ok)
        network_reload_ca_files (1);
}

/*
 * Checks option "weechat.network.proxy_curl".
 */

int
config_check_proxy_curl (const void *pointer, void *data,
                         struct t_config_option *option,
                         const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (gui_init_ok && value && value[0] && !proxy_search (value))
    {
        gui_chat_printf (NULL,
                         _("%sWarning: proxy \"%s\" does not exist (you can "
                           "add it with command /proxy)"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR], value);
    }

    return 1;
}

/*
 * Callback for changes on option "weechat.plugin.extension".
 */

void
config_change_plugin_extension (const void *pointer, void *data,
                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (config_plugin_extensions)
    {
        string_free_split (config_plugin_extensions);
        config_plugin_extensions = NULL;
    }
    config_num_plugin_extensions = 0;

    if (CONFIG_STRING(config_plugin_extension)
        && CONFIG_STRING(config_plugin_extension)[0])
    {
        config_plugin_extensions = string_split (
            CONFIG_STRING(config_plugin_extension),
            ",",
            NULL,
            WEECHAT_STRING_SPLIT_STRIP_LEFT
            | WEECHAT_STRING_SPLIT_STRIP_RIGHT
            | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
            0,
            &config_num_plugin_extensions);
    }
}

/*
 * Timer called each minute: checks if the day has changed, and if yes:
 * - refreshes screen (if needed)
 * - sends signal "day_changed"
 */

int
config_day_change_timer_cb (const void *pointer, void *data,
                            int remaining_calls)
{
    struct timeval tv_time;
    struct tm *local_time;
    time_t seconds;
    int new_mday;
    char str_time[256];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) remaining_calls;

    gettimeofday (&tv_time, NULL);
    seconds = tv_time.tv_sec;
    local_time = localtime (&seconds);
    new_mday = local_time->tm_mday;

    if ((config_day_change_old_day >= 0)
        && (new_mday != config_day_change_old_day))
    {
        if (CONFIG_BOOLEAN(config_look_day_change))
        {
            /*
             * refresh all windows so that the message with new day will be
             * displayed
             */
            gui_window_ask_refresh (1);
        }

        /* send signal "day_changed" */
        if (strftime (str_time, sizeof (str_time), "%Y-%m-%d", local_time) == 0)
            str_time[0] = '\0';
        (void) hook_signal_send ("day_changed",
                                 WEECHAT_HOOK_SIGNAL_STRING, str_time);
    }

    config_day_change_old_day = new_mday;

    return WEECHAT_RC_OK;
}

/*
 * Initializes some things after reading/reloading WeeChat configuration file.
 */

void
config_weechat_init_after_read ()
{
    int context;

    util_setrlimit ();

    gui_buffer_notify_set_all ();

    proxy_use_temp_proxies ();

    gui_bar_use_temp_bars ();
    if (gui_bars)
    {
        /*
         * at least one bar defined => just ensure that at least one bar is
         * using item "input_text"
         */
        gui_bar_create_default_input ();
    }
    else
    {
        /* no bar defined => create default bars */
        gui_bar_create_default ();
    }

    gui_bar_item_custom_use_temp_items ();

    /* if no key was found configuration file, then we use default bindings */
    for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
    {
        if (!gui_keys[context])
            gui_key_default_bindings (context, 1);
    }

    /* apply filters on all buffers */
    gui_filter_all_buffers (NULL);

    config_change_look_nick_color_force (NULL, NULL, NULL);
}

/*
 * Updates options in configuration file while reading the file.
 */

struct t_hashtable *
config_weechat_update_cb (const void *pointer, void *data,
                          struct t_config_file *config_file,
                          int version_read,
                          struct t_hashtable *data_read)
{
    const char *ptr_section, *ptr_option, *ptr_value;
    char *new_commands[][2] = {
        /* old command, new command */
        { "/input jump_smart", "/buffer jump smart" },
        { "/input jump_last_buffer", "/buffer +" },
        { "/window ${_window_number};/input jump_last_buffer", "/window ${_window_number};/buffer +" },
        { "/input jump_last_buffer_displayed", "/buffer jump last_displayed" },
        { "/input jump_previously_visited_buffer", "/buffer jump prev_visited" },
        { "/input jump_next_visited_buffer", "/buffer jump next_visited" },
        { "/input hotlist_clear", "/hotlist clear" },
        { "/input hotlist_remove_buffer", "/hotlist remove" },
        { "/input hotlist_restore_buffer", "/hotlist restore" },
        { "/input hotlist_restore_all", "/hotlist restore -all" },
        { "/input set_unread_current_buffer", "/buffer set unread" },
        { "/input set_unread", "/allbuf /buffer set unread" },
        { "/input switch_active_buffer", "/buffer switch" },
        { "/input switch_active_buffer_previous", "/buffer switch -previous" },
        { "/input zoom_merged_buffer", "/buffer zoom" },
        { NULL, NULL },
    };
    char *new_option;
    int changes, i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;

    /* nothing to do if the config file is already up-to-date */
    if (version_read >= WEECHAT_CONFIG_VERSION)
        return NULL;

    changes = 0;

    if (version_read < 2)
    {
        /*
         * changes in v2:
         *   - new format for keys (eg: meta2-1;3D -> meta-left)
         *   - keys removed: "meta2-200~" and "meta2-201~"
         */
        ptr_section = hashtable_get (data_read, "section");
        ptr_option = hashtable_get (data_read, "option");
        ptr_value = hashtable_get (data_read, "value");
        if (ptr_section
            && ptr_option
            && ((strcmp (ptr_section, "key") == 0)
                || (strcmp (ptr_section, "key_search") == 0)
                || (strcmp (ptr_section, "key_cursor") == 0)
                || (strcmp (ptr_section, "key_mouse") == 0)))
        {
            /*
             * remove some obsolete keys:
             *   - "meta2-200~": start paste
             *   - "meta2-201~": end paste
             *   - "meta2-G": page down (only if bound to "/window page_down")
             *   - "meta2-I": page up (only if bound to "/window page_up")
             */
            if ((strcmp (ptr_option, "meta2-200~") == 0)
                || (strcmp (ptr_option, "meta2-201~") == 0)
                || ((strcmp (ptr_option, "meta2-G") == 0)
                    && ptr_value
                    && (strcmp (ptr_value, "/window page_down") == 0))
                || ((strcmp (ptr_option, "meta2-I") == 0)
                    && ptr_value
                    && (strcmp (ptr_value, "/window page_up") == 0)))
            {
                gui_chat_printf (NULL,
                                 _("Legacy key removed: \"%s\""),
                                 ptr_option);
                hashtable_set (data_read, "option", "");
                changes++;
            }
            else
            {
                new_option = gui_key_legacy_to_alias (ptr_option);
                if (new_option)
                {
                    if (strcmp (ptr_option, new_option) != 0)
                    {
                        gui_chat_printf (
                            NULL,
                            _("Legacy key converted: \"%s\" => \"%s\""),
                            ptr_option, new_option);
                        hashtable_set (data_read, "option", new_option);
                        changes++;
                        if (ptr_section
                            && (strcmp (ptr_section, "key") == 0)
                            && (strcmp (new_option, "return") == 0)
                            && (!ptr_value
                                || (strcmp (ptr_value, "/input return") != 0)))
                        {
                            gui_chat_printf (
                                NULL,
                                _("Command converted for key \"%s\": \"%s\" => \"%s\""),
                                "return", ptr_value, "/input return");
                            hashtable_set (data_read, "value", "/input return");
                        }
                    }
                    free (new_option);
                }
            }
            for (i = 0; new_commands[i][0]; i++)
            {
                if (ptr_value && (strcmp (ptr_value, new_commands[i][0]) == 0))
                {
                    gui_chat_printf (
                        NULL,
                        _("Command converted for key \"%s\": \"%s\" => \"%s\""),
                        hashtable_get (data_read, "option"),
                        new_commands[i][0],
                        new_commands[i][1]);
                    hashtable_set (data_read, "value", new_commands[i][1]);
                    changes++;
                    break;
                }
            }
        }
    }

    return (changes) ? data_read : NULL;
}

/*
 * Reloads WeeChat configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_READ_OK: OK
 *   WEECHAT_CONFIG_READ_MEMORY_ERROR: not enough memory
 *   WEECHAT_CONFIG_READ_FILE_NOT_FOUND: file not found
 */

int
config_weechat_reload_cb (const void *pointer, void *data,
                          struct t_config_file *config_file)
{
    int context, rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* remove all keys */
    for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
    {
        gui_key_free_all (context,
                          &gui_keys[context],
                          &last_gui_key[context],
                          &gui_keys_count[context],
                          1);
    }

    /* remove all proxies */
    proxy_free_all ();

    /* remove all bars */
    gui_bar_free_all ();

    /* remove layouts and reset layout stuff in buffers/windows */
    gui_layout_remove_all ();
    gui_layout_buffer_reset ();
    gui_layout_window_reset ();

    /* remove all notify levels */
    config_file_section_free_options (weechat_config_section_notify);

    /* remove all filters */
    gui_filter_free_all ();

    config_loading = 1;
    rc = config_file_reload (config_file);
    config_loading = 0;

    config_weechat_init_after_read ();

    return rc;
}

/*
 * Gets debug level for a plugin (or "core").
 */

struct t_config_option *
config_weechat_debug_get (const char *plugin_name)
{
    return config_file_search_option (weechat_config_file,
                                      weechat_config_section_debug,
                                      plugin_name);
}

/*
 * Sets debug level for "core" and all plugins, using values from section
 * "debug".
 */

void
config_weechat_debug_set_all ()
{
    struct t_config_option *ptr_option;
    struct t_weechat_plugin *ptr_plugin;

    /* set debug for core */
    ptr_option = config_weechat_debug_get (PLUGIN_CORE);
    weechat_debug_core = (ptr_option) ? CONFIG_INTEGER(ptr_option) : 0;

    /* set debug for plugins */
    for (ptr_plugin = weechat_plugins; ptr_plugin;
         ptr_plugin = ptr_plugin->next_plugin)
    {
        ptr_option = config_weechat_debug_get (ptr_plugin->name);
        ptr_plugin->debug = (ptr_option) ? CONFIG_INTEGER(ptr_option) : 0;
    }
}

/*
 * Callback for changes on a debug option.
 */

void
config_weechat_debug_change_cb (const void *pointer, void *data,
                                struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    config_weechat_debug_set_all ();
}

/*
 * Callback called when an option is created in section "debug".
 */

int
config_weechat_debug_create_option_cb (const void *pointer, void *data,
                                       struct t_config_file *config_file,
                                       struct t_config_section *section,
                                       const char *option_name,
                                       const char *value)
{
    struct t_config_option *ptr_option;
    int rc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = config_file_search_option (config_file, section,
                                                option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = config_file_option_set (ptr_option, value, 1);
            else
            {
                config_file_option_free (ptr_option, 1);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                ptr_option = config_file_new_option (
                    config_file, section,
                    option_name, "integer",
                    _("debug level for plugin (\"core\" for WeeChat core)"),
                    NULL, 0, 32, "0", value, 0,
                    NULL, NULL, NULL,
                    &config_weechat_debug_change_cb, NULL, NULL,
                    NULL, NULL, NULL);
                rc = (ptr_option) ?
                    WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
            }
            else
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
    }

    /* set debug level for "core" and all plugins */
    config_weechat_debug_set_all ();

    return rc;
}

/*
 * Callback called when an option is deleted in section "debug".
 */

int
config_weechat_debug_delete_option_cb (const void *pointer, void *data,
                                       struct t_config_file *config_file,
                                       struct t_config_section *section,
                                       struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    config_file_option_free (option, 1);

    config_weechat_debug_set_all ();

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Sets debug level for a plugin (or "core").
 */

int
config_weechat_debug_set (const char *plugin_name, const char *value)
{
    return config_weechat_debug_create_option_cb (NULL, NULL,
                                                  weechat_config_file,
                                                  weechat_config_section_debug,
                                                  plugin_name,
                                                  value);
}

/*
 * Callback for changes on a palette option.
 */

void
config_weechat_palette_change_cb (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    char *error;
    int number;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    error = NULL;
    number = (int)strtol (option->name, &error, 10);
    if (error && !error[0])
    {
        gui_color_palette_add (number, CONFIG_STRING(option));
    }
}

/*
 * Callback called when an option is created in section "palette".
 */

int
config_weechat_palette_create_option_cb (const void *pointer, void *data,
                                         struct t_config_file *config_file,
                                         struct t_config_section *section,
                                         const char *option_name,
                                         const char *value)
{
    struct t_config_option *ptr_option;
    char *error;
    int rc, number;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    error = NULL;
    number = (int)strtol (option_name, &error, 10);
    if (error && !error[0])
    {
        if (option_name)
        {
            ptr_option = config_file_search_option (config_file, section,
                                                    option_name);
            if (ptr_option)
            {
                if (value)
                    rc = config_file_option_set (ptr_option, value, 1);
                else
                {
                    config_file_option_free (ptr_option, 1);
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
                }
            }
            else
            {
                if (value)
                {
                    ptr_option = config_file_new_option (
                        config_file, section,
                        option_name, "string",
                        _("alias for color"),
                        NULL, 0, 0, "", value, 0,
                        NULL, NULL, NULL,
                        &config_weechat_palette_change_cb, NULL, NULL,
                        NULL, NULL, NULL);
                    rc = (ptr_option) ?
                        WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
                    if (ptr_option)
                        gui_color_palette_add (number, value);
                }
                else
                    rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
    }
    else
    {
        gui_chat_printf (NULL,
                         _("%sPalette option must be numeric"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    }

    return rc;
}

/*
 * Callback called when an option is deleted in section "palette".
 */

int
config_weechat_palette_delete_option_cb (const void *pointer, void *data,
                                         struct t_config_file *config_file,
                                         struct t_config_section *section,
                                         struct t_config_option *option)
{
    char *error;
    int number;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    error = NULL;
    number = (int)strtol (option->name, &error, 10);
    if (error && !error[0])
        gui_color_palette_remove (number);

    config_file_option_free (option, 1);

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Reads a proxy option in WeeChat configuration file.
 */

int
config_weechat_proxy_read_cb (const void *pointer, void *data,
                              struct t_config_file *config_file,
                              struct t_config_section *section,
                              const char *option_name, const char *value)
{
    char *pos_option, *proxy_name;
    struct t_proxy *ptr_temp_proxy;
    int index_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;

    if (!option_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option = strchr (option_name, '.');
    if (!pos_option)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    proxy_name = string_strndup (option_name, pos_option - option_name);
    if (!proxy_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option++;

    /* search temporary proxy */
    for (ptr_temp_proxy = weechat_temp_proxies; ptr_temp_proxy;
         ptr_temp_proxy = ptr_temp_proxy->next_proxy)
    {
        if (strcmp (ptr_temp_proxy->name, proxy_name) == 0)
            break;
    }
    if (!ptr_temp_proxy)
    {
        /* create new temporary proxy */
        ptr_temp_proxy = proxy_alloc (proxy_name);
        if (ptr_temp_proxy)
        {
            /* add new proxy at the end */
            ptr_temp_proxy->prev_proxy = last_weechat_temp_proxy;
            ptr_temp_proxy->next_proxy = NULL;
            if (last_weechat_temp_proxy)
                last_weechat_temp_proxy->next_proxy = ptr_temp_proxy;
            else
                weechat_temp_proxies = ptr_temp_proxy;
            last_weechat_temp_proxy = ptr_temp_proxy;
        }
    }

    if (ptr_temp_proxy)
    {
        index_option = proxy_search_option (pos_option);
        if (index_option >= 0)
        {
            proxy_create_option_temp (ptr_temp_proxy, index_option,
                                      value);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sWarning: unknown option for section \"%s\": "
                               "%s (value: \"%s\")"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             section->name, option_name, value);
        }
    }

    free (proxy_name);

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Reads a bar option in WeeChat configuration file.
 */

int
config_weechat_bar_read_cb (const void *pointer, void *data,
                            struct t_config_file *config_file,
                            struct t_config_section *section,
                            const char *option_name, const char *value)
{
    char *pos_option, *bar_name;
    struct t_gui_bar *ptr_temp_bar;
    int index_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    if (!option_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option = strchr (option_name, '.');
    if (!pos_option)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    bar_name = string_strndup (option_name, pos_option - option_name);
    if (!bar_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option++;

    /* search temporary bar */
    for (ptr_temp_bar = gui_temp_bars; ptr_temp_bar;
         ptr_temp_bar = ptr_temp_bar->next_bar)
    {
        if (strcmp (ptr_temp_bar->name, bar_name) == 0)
            break;
    }
    if (!ptr_temp_bar)
    {
        /* create new temporary bar */
        ptr_temp_bar = gui_bar_alloc (bar_name);
        if (ptr_temp_bar)
        {
            /* add new bar at the end */
            ptr_temp_bar->prev_bar = last_gui_temp_bar;
            ptr_temp_bar->next_bar = NULL;
            if (last_gui_temp_bar)
                last_gui_temp_bar->next_bar = ptr_temp_bar;
            else
                gui_temp_bars = ptr_temp_bar;
            last_gui_temp_bar = ptr_temp_bar;
        }
    }

    if (ptr_temp_bar)
    {
        index_option = gui_bar_search_option (pos_option);
        if (index_option >= 0)
        {
            gui_bar_create_option_temp (ptr_temp_bar, index_option,
                                        value);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sWarning: unknown option for section \"%s\": "
                               "%s (value: \"%s\")"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             section->name, option_name, value);
        }
    }

    free (bar_name);

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Reads a custom bar item option in WeeChat configuration file.
 */

int
config_weechat_custom_bar_item_read_cb (const void *pointer, void *data,
                                        struct t_config_file *config_file,
                                        struct t_config_section *section,
                                        const char *option_name, const char *value)
{
    char *pos_option, *item_name;
    struct t_gui_bar_item_custom *ptr_temp_item;
    int index_option;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    if (!option_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option = strchr (option_name, '.');
    if (!pos_option)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    item_name = string_strndup (option_name, pos_option - option_name);
    if (!item_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    pos_option++;

    /* search temporary custom bar item */
    for (ptr_temp_item = gui_temp_custom_bar_items; ptr_temp_item;
         ptr_temp_item = ptr_temp_item->next_item)
    {
        if (strcmp (ptr_temp_item->name, item_name) == 0)
            break;
    }
    if (!ptr_temp_item)
    {
        /* create new temporary custom bar item */
        ptr_temp_item = gui_bar_item_custom_alloc (item_name);
        if (ptr_temp_item)
        {
            /* add new custom bar item at the end */
            ptr_temp_item->prev_item = last_gui_temp_custom_bar_item;
            ptr_temp_item->next_item = NULL;
            if (last_gui_temp_custom_bar_item)
                last_gui_temp_custom_bar_item->next_item = ptr_temp_item;
            else
                gui_temp_custom_bar_items = ptr_temp_item;
            last_gui_temp_custom_bar_item = ptr_temp_item;
        }
    }

    if (ptr_temp_item)
    {
        index_option = gui_bar_item_custom_search_option (pos_option);
        if (index_option >= 0)
        {
            gui_bar_item_custom_create_option_temp (ptr_temp_item, index_option,
                                                    value);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sWarning: unknown option for section \"%s\": "
                               "%s (value: \"%s\")"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             section->name, option_name, value);
        }
    }

    free (item_name);

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Reads a layout option in WeeChat configuration file.
 */

int
config_weechat_layout_read_cb (const void *pointer, void *data,
                               struct t_config_file *config_file,
                               struct t_config_section *section,
                               const char *option_name, const char *value)
{
    int argc, force_current_layout;
    char **argv, *pos, *layout_name, *error1, *error2, *error3, *error4;
    const char *ptr_option_name;
    long number1, number2, number3, number4;
    struct t_gui_layout *ptr_layout;
    struct t_gui_layout_window *parent;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    if (!option_name || !value || !value[0])
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    force_current_layout = 0;

    pos = strrchr (option_name, '.');
    if (pos)
    {
        layout_name = string_strndup (option_name, pos - option_name);
        ptr_option_name = pos + 1;
    }
    else
    {
        /*
         * old config file (WeeChat <= 0.4.0): no "." in name, use default
         * layout name
         */
        layout_name = strdup (GUI_LAYOUT_DEFAULT_NAME);
        ptr_option_name = option_name;
        force_current_layout = 1;
    }

    if (!layout_name)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    ptr_layout = gui_layout_search (layout_name);
    if (!ptr_layout)
    {
        ptr_layout = gui_layout_alloc (layout_name);
        if (!ptr_layout)
        {
            free (layout_name);
            return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
        }
        gui_layout_add (ptr_layout);
    }

    if (strcmp (ptr_option_name, "buffer") == 0)
    {
        argv = string_split (value, ";", NULL,
                             WEECHAT_STRING_SPLIT_STRIP_LEFT
                             | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                             | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                             0, &argc);
        if (argv)
        {
            if (argc >= 3)
            {
                error1 = NULL;
                number1 = strtol (argv[2], &error1, 10);
                if (error1 && !error1[0])
                    gui_layout_buffer_add (ptr_layout, argv[0], argv[1], number1);
            }
            string_free_split (argv);
        }
    }
    else if (strcmp (ptr_option_name, "window") == 0)
    {
        argv = string_split (value, ";", NULL,
                             WEECHAT_STRING_SPLIT_STRIP_LEFT
                             | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                             | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                             0, &argc);
        if (argv)
        {
            if (argc >= 6)
            {
                error1 = NULL;
                number1 = strtol (argv[0], &error1, 10);
                error2 = NULL;
                number2 = strtol (argv[1], &error2, 10);
                error3 = NULL;
                number3 = strtol (argv[2], &error3, 10);
                error4 = NULL;
                number4 = strtol (argv[3], &error4, 10);
                if (error1 && !error1[0] && error2 && !error2[0]
                    && error3 && !error3[0] && error4 && !error4[0])
                {
                    parent = gui_layout_window_search_by_id (ptr_layout->layout_windows,
                                                             number2);
                    gui_layout_window_add (&ptr_layout->layout_windows,
                                           number1,
                                           parent,
                                           number3,
                                           number4,
                                           (strcmp (argv[4], "-") != 0) ?
                                           argv[4] : NULL,
                                           (strcmp (argv[4], "-") != 0) ?
                                           argv[5] : NULL);
                }
            }
            string_free_split (argv);
        }
    }
    else if (strcmp (ptr_option_name, "current") == 0)
    {
        if (config_file_string_to_boolean (value))
            gui_layout_current = ptr_layout;
    }

    if (force_current_layout)
        gui_layout_current = ptr_layout;

    free (layout_name);

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Writes layout of windows in WeeChat configuration file.
 *
 * Returns:
 *   1: OK
 *   0: write error
 */

int
config_weechat_layout_write_tree (struct t_config_file *config_file,
                                  const char *option_name,
                                  struct t_gui_layout_window *layout_window)
{
    if (!config_file_write_line (config_file, option_name,
                                 "\"%d;%d;%d;%d;%s;%s\"",
                                 layout_window->internal_id,
                                 (layout_window->parent_node) ?
                                 layout_window->parent_node->internal_id : 0,
                                 layout_window->split_pct,
                                 layout_window->split_horiz,
                                 (layout_window->plugin_name) ?
                                 layout_window->plugin_name : "-",
                                 (layout_window->buffer_name) ?
                                 layout_window->buffer_name : "-"))
        return 0;

    if (layout_window->child1)
    {
        if (!config_weechat_layout_write_tree (config_file, option_name,
                                               layout_window->child1))
            return 0;
    }

    if (layout_window->child2)
    {
        if (!config_weechat_layout_write_tree (config_file, option_name,
                                               layout_window->child2))
            return 0;
    }

    return 1;
}

/*
 * Writes section "layout" in WeeChat configuration file.
 */

int
config_weechat_layout_write_cb (const void *pointer, void *data,
                                struct t_config_file *config_file,
                                const char *section_name)
{
    struct t_gui_layout *ptr_layout;
    struct t_gui_layout_buffer *ptr_layout_buffer;
    char option_name[1024];

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!config_file_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (ptr_layout = gui_layouts; ptr_layout;
         ptr_layout = ptr_layout->next_layout)
    {
        /* write layout for buffers */
        for (ptr_layout_buffer = ptr_layout->layout_buffers; ptr_layout_buffer;
             ptr_layout_buffer = ptr_layout_buffer->next_layout)
        {
            snprintf (option_name, sizeof (option_name),
                      "%s.buffer", ptr_layout->name);
            if (!config_file_write_line (config_file, option_name,
                                         "\"%s;%s;%d\"",
                                         ptr_layout_buffer->plugin_name,
                                         ptr_layout_buffer->buffer_name,
                                         ptr_layout_buffer->number))
                return WEECHAT_CONFIG_WRITE_ERROR;
        }

        /* write layout for windows */
        if (ptr_layout->layout_windows)
        {
            snprintf (option_name, sizeof (option_name),
                      "%s.window", ptr_layout->name);
            if (!config_weechat_layout_write_tree (config_file, option_name,
                                                   ptr_layout->layout_windows))
                return WEECHAT_CONFIG_WRITE_ERROR;
        }

        /* write "current = on" if it is current layout */
        if (ptr_layout == gui_layout_current)
        {
            snprintf (option_name, sizeof (option_name),
                      "%s.current", ptr_layout->name);
            if (!config_file_write_line (config_file, option_name, "on"))
                return WEECHAT_CONFIG_WRITE_ERROR;
        }
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Checks notify option value.
 *
 * Returns:
 *   1: value OK
 *   0: invalid value
 */

int
config_weechat_notify_check_cb (const void *pointer, void *data,
                                 struct t_config_option *option,
                                 const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    return (gui_buffer_search_notify (value) >= 0) ? 1 : 0;
}

/*
 * Callback for changes on a notify option.
 */

void
config_weechat_notify_change_cb (const void *pointer, void *data,
                                 struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    gui_buffer_notify_set_all ();
}

/*
 * Callback called when an option is created in section "notify".
 */

int
config_weechat_notify_create_option_cb (const void *pointer, void *data,
                                        struct t_config_file *config_file,
                                        struct t_config_section *section,
                                        const char *option_name,
                                        const char *value)
{
    struct t_config_option *ptr_option;
    int rc, notify;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;

    if (option_name)
    {
        ptr_option = config_file_search_option (config_file, section,
                                                option_name);
        if (ptr_option)
        {
            if (value && value[0])
                rc = config_file_option_set (ptr_option, value, 1);
            else
            {
                config_file_option_free (ptr_option, 1);
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
        else
        {
            if (value && value[0])
            {
                notify = gui_buffer_search_notify (value);
                if (notify >= 0)
                {
                    ptr_option = config_file_new_option (
                        config_file, section,
                        option_name, "integer", _("Notify level for buffer"),
                        "none|highlight|message|all",
                        0, 0, "", value, 0,
                        &config_weechat_notify_check_cb, NULL, NULL,
                        &config_weechat_notify_change_cb, NULL, NULL,
                        NULL, NULL, NULL);
                    rc = (ptr_option) ?
                        WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE : WEECHAT_CONFIG_OPTION_SET_ERROR;
                }
                else
                {
                    rc = WEECHAT_CONFIG_OPTION_SET_ERROR;
                }
            }
            else
            {
                rc = WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
            }
        }
    }

    if (rc != WEECHAT_CONFIG_OPTION_SET_ERROR)
        gui_buffer_notify_set_all ();

    return rc;
}

/*
 * Callback called when an option is deleted in section "notify".
 */

int
config_weechat_notify_delete_option_cb (const void *pointer, void *data,
                                        struct t_config_file *config_file,
                                        struct t_config_section *section,
                                        struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    config_file_option_free (option, 1);

    gui_buffer_notify_set_all ();

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Sets a notify level for a buffer.
 *
 * A negative value resets notify for buffer to its default value (and
 * removes buffer from config file).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
config_weechat_notify_set (struct t_gui_buffer *buffer, const char *notify)
{
    int rc;

    if (!buffer || !notify)
        return 0;

    /* create/update option */
    rc = config_weechat_notify_create_option_cb (
        NULL, NULL,
        weechat_config_file,
        weechat_config_section_notify,
        buffer->full_name,
        (strcmp (notify, "reset") == 0) ? "none" : notify);

    return (rc != WEECHAT_CONFIG_OPTION_SET_ERROR) ? 1 : 0;
}

/*
 * Reads a filter option in WeeChat configuration file.
 */

int
config_weechat_filter_read_cb (const void *pointer, void *data,
                               struct t_config_file *config_file,
                               struct t_config_section *section,
                               const char *option_name, const char *value)
{
    char **argv, **argv_eol;
    int argc;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;
    (void) section;

    if (option_name && value && value[0])
    {
        argv = string_split (value, ";", NULL,
                             WEECHAT_STRING_SPLIT_STRIP_LEFT
                             | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                             | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                             0, &argc);
        argv_eol = string_split (value, ";", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
                                 | WEECHAT_STRING_SPLIT_KEEP_EOL,
                                 0, NULL);
        if (argv && argv_eol && (argc >= 4))
        {
            gui_filter_new ((string_strcasecmp (argv[0], "on") == 0) ? 1 : 0,
                            option_name, argv[1], argv[2], argv_eol[3]);
        }
        if (argv)
            string_free_split (argv);
        if (argv_eol)
            string_free_split (argv_eol);
    }

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Writes section "filter" in WeeChat configuration file.
 */

int
config_weechat_filter_write_cb (const void *pointer, void *data,
                                struct t_config_file *config_file,
                                const char *section_name)
{
    struct t_gui_filter *ptr_filter;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (!config_file_write_line (config_file, section_name, NULL))
        return WEECHAT_CONFIG_WRITE_ERROR;

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (!config_file_write_line (config_file,
                                     ptr_filter->name,
                                     "%s;%s;%s;%s",
                                     (ptr_filter->enabled) ? "on" : "off",
                                     ptr_filter->buffer_name,
                                     ptr_filter->tags,
                                     ptr_filter->regex))
            return WEECHAT_CONFIG_WRITE_ERROR;
    }

    return WEECHAT_CONFIG_WRITE_OK;
}

/*
 * Searches key context with the section pointer.
 *
 * Returns key context, -1 if not found.
 */

int
config_weechat_get_key_context (struct t_config_section *section)
{
    int context;

    for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
    {
        if (section == weechat_config_section_key[context])
            return context;
    }

    /* this should never happen */
    return -1;
}

/*
 * Callback called when an option is created in one of these sections:
 *   key
 *   key_search
 *   key_cursor
 *   key_mouse
 */

int
config_weechat_key_create_option_cb (const void *pointer, void *data,
                                     struct t_config_file *config_file,
                                     struct t_config_section *section,
                                     const char *option_name,
                                     const char *value)
{
    int context;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;

    if (!value)
        return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;

    context = config_weechat_get_key_context (section);

    if (config_loading)
    {
        /* don't check key when loading config */
        (void) gui_key_bind (NULL, context, option_name, value, 0);
    }
    else
    {
        /* enable verbose and check key if option is manually created */
        gui_key_verbose = 1;
        (void) gui_key_bind (NULL, context, option_name, value, 1);
        gui_key_verbose = 0;
    }

    return WEECHAT_CONFIG_OPTION_SET_OK_SAME_VALUE;
}

/*
 * Callback called when an option is deleted in one of these sections:
 *   key
 *   key_search
 *   key_cursor
 *   key_mouse
 */

int
config_weechat_key_delete_option_cb (const void *pointer, void *data,
                                     struct t_config_file *config_file,
                                     struct t_config_section *section,
                                     struct t_config_option *option)
{
    int context;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) config_file;

    context = config_weechat_get_key_context (section);
    gui_key_unbind (NULL, context, option->name);

    return WEECHAT_CONFIG_OPTION_UNSET_OK_REMOVED;
}

/*
 * Creates options in WeeChat configuration.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
config_weechat_init_options ()
{
    int context;
    char section_name[128];

    weechat_config_file = config_file_new (
        NULL, WEECHAT_CONFIG_PRIO_NAME, &config_weechat_reload_cb, NULL, NULL);
    if (!weechat_config_file)
        return 0;

    if (!config_file_set_version (weechat_config_file, WEECHAT_CONFIG_VERSION,
                                  &config_weechat_update_cb, NULL, NULL))
    {
        config_file_free (weechat_config_file);
        weechat_config_file = NULL;
        return 0;
    }

    /* debug */
    weechat_config_section_debug = config_file_new_section (
        weechat_config_file, "debug",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &config_weechat_debug_create_option_cb, NULL, NULL,
        &config_weechat_debug_delete_option_cb, NULL, NULL);

    /* startup */
    weechat_config_section_startup = config_file_new_section (
        weechat_config_file, "startup",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (weechat_config_section_startup)
    {
        config_startup_command_after_plugins = config_file_new_option (
            weechat_config_file, weechat_config_section_startup,
            "command_after_plugins", "string",
            N_("command executed when WeeChat starts, after loading plugins; "
               "multiple commands can be separated by semicolons "
               "(note: commands are evaluated, see /help eval)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_startup_command_before_plugins = config_file_new_option (
            weechat_config_file, weechat_config_section_startup,
            "command_before_plugins", "string",
            N_("command executed when WeeChat starts, before loading plugins; "
               "multiple commands can be separated by semicolons "
               "(note: commands are evaluated, see /help eval)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_startup_display_logo = config_file_new_option (
            weechat_config_file, weechat_config_section_startup,
            "display_logo", "boolean",
            N_("display WeeChat logo at startup"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_startup_display_version = config_file_new_option (
            weechat_config_file, weechat_config_section_startup,
            "display_version", "boolean",
            N_("display WeeChat version at startup"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_startup_sys_rlimit = config_file_new_option (
            weechat_config_file, weechat_config_section_startup,
            "sys_rlimit", "string",
            N_("set resource limits for WeeChat process, format is: "
               "\"res1:limit1,res2:limit2\"; resource name is the end of "
               "constant (RLIMIT_XXX) in lower case (see man setrlimit for "
               "values); limit -1 means \"unlimited\"; example: set unlimited "
               "size for core file and max 1GB of virtual memory: "
               "\"core:-1,as:1000000000\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_sys_rlimit, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* look */
    weechat_config_section_look = config_file_new_section (
        weechat_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (weechat_config_section_look)
    {
        config_look_align_end_of_lines = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "align_end_of_lines", "integer",
            N_("alignment for end of lines (all lines after the first): they "
               "are starting under this data (time, buffer, prefix, suffix, "
               "message (default))"),
            "time|buffer|prefix|suffix|message", 0, 0, "message", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_align_multiline_words = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "align_multiline_words", "boolean",
            N_("alignment for multiline words according to option "
               "weechat.look.align_end_of_lines; if disabled, the multiline "
               "words will not be aligned, which can be useful to not break "
               "long URLs"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_bar_more_down = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "bar_more_down", "string",
            N_("string displayed when bar can be scrolled down "
               "(for bars with filling different from \"horizontal\")"),
            NULL, 0, 0, "++", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_bar_more_left = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "bar_more_left", "string",
            N_("string displayed when bar can be scrolled to the left "
               "(for bars with filling \"horizontal\")"),
            NULL, 0, 0, "<<", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_bar_more_right = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "bar_more_right", "string",
            N_("string displayed when bar can be scrolled to the right "
               "(for bars with filling \"horizontal\")"),
            NULL, 0, 0, ">>", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_bar_more_up = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "bar_more_up", "string",
            N_("string displayed when bar can be scrolled up "
               "(for bars with filling different from \"horizontal\")"),
            NULL, 0, 0, "--", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_bare_display_exit_on_input = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "bare_display_exit_on_input", "boolean",
            N_("exit the bare display mode on any changes in input"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_bare_display_time_format = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "bare_display_time_format", "string",
            N_("time format in bare display mode (see man strftime for "
               "date/time specifiers)"),
            NULL, 0, 0, "%H:%M", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_buffer_auto_renumber = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "buffer_auto_renumber", "boolean",
            N_("automatically renumber buffers to have only consecutive "
               "numbers and start with number 1; if disabled, gaps between "
               "buffer numbers are allowed and the first buffer can have a "
               "number greater than 1"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_auto_renumber, NULL, NULL,
            NULL, NULL, NULL);
        config_look_buffer_notify_default = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "buffer_notify_default", "integer",
            N_("default notify level for buffers (used to tell WeeChat if "
               "buffer must be displayed in hotlist or not, according to "
               "importance of message): all=all messages (default), "
               "message=messages+highlights, highlight=highlights only, "
               "none=never display in hotlist"),
            "none|highlight|message|all", 0, 0, "all", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_notify_default, NULL, NULL,
            NULL, NULL, NULL);
        config_look_buffer_position = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "buffer_position", "integer",
            N_("position of a new buffer: end = after the end of list (number = "
               "last number + 1) (default), first_gap = at first available "
               "number in the list (after the end of list if no number is "
               "available); this option is used only if the buffer has no "
               "layout number"),
            "end|first_gap", 0, 0, "end", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_buffer_search_case_sensitive = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "buffer_search_case_sensitive", "boolean",
            N_("default text search in buffer: case sensitive or not"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_buffer_search_force_default = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "buffer_search_force_default", "boolean",
            N_("force default values for text search in buffer (instead of "
               "using values from last search in buffer)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_buffer_search_regex = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "buffer_search_regex", "boolean",
            N_("default text search in buffer: if enabled, search POSIX "
               "extended regular expression, otherwise search simple string"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_buffer_search_where = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "buffer_search_where", "integer",
            N_("default text search in buffer: in message, prefix, prefix and "
               "message"),
            "prefix|message|prefix_message", 0, 0, "prefix_message",
            NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_buffer_time_format = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "buffer_time_format", "string",
            /* TRANSLATORS: string "${color:xxx}" must NOT be translated */
            N_("time format for each line displayed in buffers (see man "
               "strftime for date/time specifiers) (note: content is evaluated, "
               "so you can use colors with format \"${color:xxx}\", see /help "
               "eval); for example time using grayscale: "
               "\"${color:252}%H${color:243}%M${color:237}%S\""),
            NULL, 0, 0, "%H:%M:%S", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_time_format, NULL, NULL,
            NULL, NULL, NULL);
        config_look_buffer_time_same = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "buffer_time_same", "string",
            /* TRANSLATORS: string "${color:xxx}" must NOT be translated */
            N_("time displayed for a message with same time as previous message: "
               "use a space \" \" to hide time, another string to display this "
               "string instead of time, or an empty string to disable feature "
               "(display time) (note: content is evaluated, so you can use "
               "colors with format \"${color:xxx}\", see /help eval)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_time_same, NULL, NULL,
            NULL, NULL, NULL);
        config_look_chat_space_right = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "chat_space_right", "boolean",
            N_("keep a space on the right side of chat area if there is a bar "
               "displayed on the right (for both text and read marker)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_color_basic_force_bold = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_basic_force_bold", "boolean",
            N_("force \"bold\" attribute for light colors and \"darkgray\" in "
               "basic colors (this option is disabled by default: bold is used "
               "only if terminal has less than 16 colors)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_look_color_inactive_buffer = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_inactive_buffer", "boolean",
            N_("use a different color for lines in inactive buffer (when line "
               "is from a merged buffer not selected)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_color_inactive_message = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_inactive_message", "boolean",
            N_("use a different color for inactive message (when window is not "
               "current window, or if line is from a merged buffer not selected)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_color_inactive_prefix = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_inactive_prefix", "boolean",
            N_("use a different color for inactive prefix (when window is not "
               "current window, or if line is from a merged buffer not selected)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_color_inactive_prefix_buffer = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_inactive_prefix_buffer", "boolean",
            N_("use a different color for inactive buffer name in prefix (when "
               "window is not current window, or if line is from a merged buffer "
               "not selected)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_color_inactive_time = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_inactive_time", "boolean",
            N_("use a different color for inactive time (when window is not "
               "current window, or if line is from a merged buffer not "
               "selected)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_color_inactive_window = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_inactive_window", "boolean",
            N_("use a different color for lines in inactive window (when window "
               "is not current window)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_color_nick_offline = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_nick_offline", "boolean",
            N_("use a different color for offline nicks (not in nicklist any "
               "more)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_color_pairs_auto_reset = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_pairs_auto_reset", "integer",
            N_("automatically reset table of color pairs when number of "
               "available pairs is lower or equal to this number (-1 = "
               "disable automatic reset, and then a manual \"/color reset\" "
               "is needed when table is full)"),
            NULL, -1, 256, "5", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_color_real_white = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "color_real_white", "boolean",
            N_("if set, uses real white color, disabled by default "
               "for terms with white background (if you never use "
               "white background, you should turn on this option to "
               "see real white instead of default term foreground "
               "color)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_look_command_chars = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "command_chars", "string",
            N_("chars used to determine if input string is a command or not: "
               "input must start with one of these chars; the slash (\"/\") is "
               "always considered as command prefix (example: \".$\")"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_command_incomplete = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "command_incomplete", "boolean",
            N_("if set, incomplete and unambiguous commands are allowed, for "
               "example /he for /help"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_confirm_quit = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "confirm_quit", "boolean",
            N_("if set, /quit command must be confirmed with extra argument "
               "\"-yes\" (see /help quit)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_confirm_upgrade = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "confirm_upgrade", "boolean",
            N_("if set, /upgrade command must be confirmed with extra argument "
               "\"-yes\" (see /help upgrade)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_day_change = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "day_change", "boolean",
            N_("display special message when day changes"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_day_change_message_1date = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "day_change_message_1date", "string",
            /* TRANSLATORS: string "${color:xxx}" must NOT be translated */
            N_("message displayed when the day has changed, with one date "
               "displayed (for example at beginning of buffer) (see man "
               "strftime for date/time specifiers) (note: content is "
               "evaluated, so you can use colors with format \"${color:xxx}\", "
               "see /help eval)"),
            NULL, 0, 0, "-- %a, %d %b %Y --", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_day_change_message_2dates = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "day_change_message_2dates", "string",
            /* TRANSLATORS: string "${color:xxx}" must NOT be translated */
            N_("message displayed when the day has changed, with two dates "
               "displayed (between two messages); the second date specifiers "
               "must start with two \"%\" because strftime is called two "
               "times on this string (see man strftime for date/time "
               "specifiers) (note: content is evaluated, so you can use "
               "colors with format \"${color:xxx}\", see /help eval)"),
            NULL, 0, 0, "-- %%a, %%d %%b %%Y (%a, %d %b %Y) --", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_eat_newline_glitch = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "eat_newline_glitch", "boolean",
            N_("if set, the eat_newline_glitch will be set to 0; this is used "
               "to not add new line char at end of each line, and then not "
               "break text when you copy/paste text from WeeChat to another "
               "application (this option is disabled by default because it "
               "can cause serious display bugs)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &config_change_eat_newline_glitch, NULL, NULL,
            NULL, NULL, NULL);
        config_look_emphasized_attributes = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "emphasized_attributes", "string",
            N_("attributes for emphasized text: one or more attribute chars ("
               "\"%\" for blink, "
               "\".\" for \"dim\" (half bright), "
               "\"*\" for bold, "
               "\"!\" for reverse, "
               "\"/\" for italic, "
               "\"_\" for underline); "
               "if the string is empty, "
               "the colors weechat.color.emphasized* are used"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_emphasized_attributes, NULL, NULL,
            NULL, NULL, NULL);
        config_look_highlight = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "highlight", "string",
            N_("comma separated list of words to highlight; case insensitive "
               "comparison (use \"(?-i)\" at beginning of words to make them "
               "case sensitive), words may begin or end with \"*\" for partial "
               "match; example: \"test,(?-i)*toto*,flash*\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_highlight_disable_regex = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "highlight_disable_regex", "string",
            N_("POSIX extended regular expression used to prevent any highlight "
               "from a message: this option has higher priority over other "
               "highlight options (if the string is found in the message, the "
               "highlight is disabled and the other options are ignored), "
               "regular expression is case insensitive (use \"(?-i)\" at "
               "beginning to make it case sensitive), examples: "
               "\"<flash.*>\", \"(?-i)<Flash.*>\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_highlight_disable_regex, NULL, NULL,
            NULL, NULL, NULL);
        config_look_highlight_regex = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "highlight_regex", "string",
            N_("POSIX extended regular expression used to check if a message "
               "has highlight or not, at least one match in string must be "
               "surrounded by delimiters (chars different from: alphanumeric, "
               "\"-\", \"_\" and \"|\"), regular expression is case insensitive "
               "(use \"(?-i)\" at beginning to make it case sensitive), "
               "examples: \"flashcode|flashy\", \"(?-i)FlashCode|flashy\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_highlight_regex, NULL, NULL,
            NULL, NULL, NULL);
        config_look_highlight_tags = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "highlight_tags", "string",
            N_("comma separated list of tags to highlight; case insensitive "
               "comparison; wildcard \"*\" is allowed in each tag; many tags "
               "can be separated by \"+\" to make a logical \"and\" between "
               "tags; examples: \"nick_flashcode\" for messages from nick "
               "\"FlashCode\", \"irc_notice+nick_toto*\" for notices from a "
               "nick starting with \"toto\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_highlight_tags, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_add_conditions = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_add_conditions", "string",
            N_("conditions to add a buffer in hotlist (if notify level is OK "
               "for the buffer); you can use in these conditions: \"window\" "
               "(current window pointer), \"buffer\" (buffer pointer to add "
               "in hotlist), \"priority\" (0 = low, 1 = message, 2 = private, "
               "3 = highlight); by default a buffer is added to hotlist if "
               "you are away, or if the buffer is not visible on screen (not "
               "displayed in any window), or if at least one relay client is "
               "connected via the weechat protocol"),
            NULL, 0, 0,
            "${away} "
            "|| ${buffer.num_displayed} == 0 "
            "|| ${info:relay_client_count,weechat,connected} > 0",
            NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_hotlist_buffer_separator = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_buffer_separator", "string",
            N_("string displayed between buffers in hotlist"),
            NULL, 0, 0, ", ", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_count_max = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_count_max", "integer",
            N_("max number of messages count to display in hotlist for a buffer: "
               "0 = never display messages count, "
               "other number = display max N messages count (from the highest "
               "to lowest priority)"),
            NULL, 0, GUI_HOTLIST_NUM_PRIORITIES, "2", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_count_min_msg = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_count_min_msg", "integer",
            N_("display messages count if number of messages is greater or "
               "equal to this value"),
            NULL, 1, 100, "2", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_names_count = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_names_count", "integer",
            N_("max number of names in hotlist (0 = no name displayed, only "
               "buffer numbers)"),
            NULL, 0, GUI_BUFFERS_MAX, "3", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_names_length = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_names_length", "integer",
            N_("max length of names in hotlist (0 = no limit)"),
            NULL, 0, 32, "0", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_names_level = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_names_level", "integer",
            N_("level for displaying names in hotlist (combination "
               "of: 1=join/part, 2=message, 4=private, 8=highlight, "
               "for example: 12=private+highlight)"),
            NULL, 1, GUI_HOTLIST_MASK_MAX, "12", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_names_merged_buffers = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_names_merged_buffers", "boolean",
            N_("if set, force display of names in hotlist for merged buffers"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_prefix = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_prefix", "string",
            N_("text displayed at the beginning of the hotlist"),
            NULL, 0, 0, "H: ", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_remove = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_remove", "integer",
            N_("remove buffers in hotlist: buffer = remove buffer by buffer, "
               "merged = remove all visible merged buffers at once"),
            "buffer|merged",
            0, 0, "merged", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_hotlist_short_names = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_short_names", "boolean",
            N_("if set, uses short names to display buffer names in hotlist "
               "(start after first \".\" in name)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_sort = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_sort", "integer",
            N_("sort of hotlist: group_time_*: group by notify level "
               "(highlights first) then sort by time, group_number_*: group "
               "by notify level (highlights first) then sort by number, "
               "number_*: sort by number; asc = ascending sort, desc = "
               "descending sort"),
            "group_time_asc|group_time_desc|group_number_asc|"
            "group_number_desc|number_asc|number_desc",
            0, 0, "group_time_asc", NULL, 0,
            NULL, NULL, NULL,
            &config_change_hotlist_sort, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_suffix = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_suffix", "string",
            N_("text displayed at the end of the hotlist"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_unique_numbers = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_unique_numbers", "boolean",
            N_("keep only unique numbers in hotlist (this applies only on "
               "hotlist items where name is NOT displayed after number)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_hotlist_update_on_buffer_switch = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "hotlist_update_on_buffer_switch", "boolean",
            N_("update the hotlist when switching buffers"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_input_cursor_scroll = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "input_cursor_scroll", "integer",
            N_("number of chars displayed after end of input line when "
               "scrolling to display end of line"),
            NULL, 0, 100, "20", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_input_multiline_lead_linebreak = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "input_multiline_lead_linebreak", "boolean",
            N_("start the input text on a new line when the input contains "
               "multiple lines, so that the start of the lines align"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_input_share = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "input_share", "integer",
            N_("share commands, text, or both in input for all buffers (there "
               "is still local history for each buffer)"),
            "none|commands|text|all",
            0, 0, "none", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_input_share_overwrite = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "input_share_overwrite", "boolean",
            N_("if set and input is shared, always overwrite input in target "
               "buffer"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_input_undo_max = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "input_undo_max", "integer",
            N_("max number of \"undo\" for command line, by buffer (0 = undo "
               "disabled)"),
            NULL, 0, 65535, "32", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_item_away_message = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "item_away_message", "boolean",
            N_("display server away message in away bar item"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_item_away, NULL, NULL,
            NULL, NULL, NULL);
        config_look_item_buffer_filter = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "item_buffer_filter", "string",
            N_("string used to show that some lines are filtered in current "
               "buffer (bar item \"buffer_filter\")"),
            NULL, 0, 0, "*", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_item_buffer_zoom = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "item_buffer_zoom", "string",
            N_("string used to show zoom on merged buffer "
               "(bar item \"buffer_zoom\")"),
            NULL, 0, 0, "!", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_item_mouse_status = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "item_mouse_status", "string",
            N_("string used to show if mouse is enabled "
               "(bar item \"mouse_status\")"),
            NULL, 0, 0, "M", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_item_time_format = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "item_time_format", "string",
            N_("time format for \"time\" bar item (see man strftime for "
               "date/time specifiers) (note: content is evaluated, so you can "
               "use colors with format \"${color:xxx}\", see /help eval)"),
            NULL, 0, 0, "%H:%M", NULL, 0,
            NULL, NULL, NULL,
            &config_change_item_time_format, NULL, NULL,
            NULL, NULL, NULL);
        config_look_jump_current_to_previous_buffer = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "jump_current_to_previous_buffer", "boolean",
            N_("jump to previous buffer displayed when jumping to current "
               "buffer number with /buffer *N (where N is a buffer number), to "
               "easily switch to another buffer, then come back to current "
               "buffer"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_jump_previous_buffer_when_closing = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "jump_previous_buffer_when_closing", "boolean",
            N_("jump to previously visited buffer when closing a buffer (if "
               "disabled, then jump to buffer number - 1)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_jump_smart_back_to_buffer = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "jump_smart_back_to_buffer", "boolean",
            N_("jump back to initial buffer after reaching end of hotlist"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_key_bind_safe = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "key_bind_safe", "boolean",
            N_("allow only binding of \"safe\" keys (beginning with a ctrl or "
               "meta code)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_key_grab_delay = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "key_grab_delay", "integer",
            N_("default delay (in milliseconds) to grab a key (using default "
               "key alt-k); this delay can be overridden in the /input command "
               "(see /help input)"),
            NULL, 1, 10000, "800", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_mouse = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "mouse", "boolean",
            N_("enable mouse support"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &config_change_mouse, NULL, NULL,
            NULL, NULL, NULL);
        config_look_mouse_timer_delay = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "mouse_timer_delay", "integer",
            N_("delay (in milliseconds) to grab a mouse event: WeeChat will "
               "wait this delay before processing event"),
            NULL, 1, 10000, "100", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_nick_color_force = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "nick_color_force", "string",
            N_("force color for some nicks: hash computed with nickname "
               "to find color will not be used for these nicks (format is: "
               "\"nick1:color1;nick2:color2\"); look up for nicks is with "
               "exact case then lower case, so it's possible to use only lower "
               "case for nicks in this option; color can include background with "
               "the format \"text,background\", for example \"yellow,red\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_look_nick_color_force, NULL, NULL,
            NULL, NULL, NULL);
        config_look_nick_color_hash = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "nick_color_hash", "integer",
            N_("hash algorithm used to find the color for a nick: djb2 = variant "
               "of djb2 (position of letters matters: anagrams of a nick have "
               "different color), djb2_32 = variant of djb2 using 32-bit instead "
               "of 64-bit integer, sum = sum of letters, sum_32 = sum of letters "
               "using 32-bit instead of 64-bit integer"),
            "djb2|sum|djb2_32|sum_32", 0, 0, "djb2", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_nick_color_hash_salt = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "nick_color_hash_salt", "string",
            N_("salt for the hash algorithm used to find nick colors "
               "(the nickname is appended to this salt and the hash algorithm "
               "operates on this string); modifying this shuffles nick colors"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_nick_color_stop_chars = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "nick_color_stop_chars", "string",
            N_("chars used to stop in nick when computing color with letters of "
               "nick (at least one char outside this list must be in string "
               "before stopping) (example: nick \"|nick|away\" with \"|\" in "
               "chars will return color of nick \"|nick\"); this option has an "
               "impact on option weechat.look.nick_color_force, so the nick "
               "for the forced color must not contain the chars ignored by "
               "this option"),
            NULL, 0, 0, "_|[", NULL, 0,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
        config_look_nick_prefix = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "nick_prefix", "string",
            N_("text to display before nick in prefix of message, example: "
               "\"<\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_nick_prefix_suffix, NULL, NULL,
            NULL, NULL, NULL);
        config_look_nick_suffix = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "nick_suffix", "string",
            N_("text to display after nick in prefix of message, example: "
               "\">\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_nick_prefix_suffix, NULL, NULL,
            NULL, NULL, NULL);
        config_look_paste_bracketed = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "paste_bracketed", "boolean",
            N_("enable terminal \"bracketed paste mode\" (not supported in all "
               "terminals/multiplexers): in this mode, pasted text is bracketed "
               "with control sequences so that WeeChat can differentiate pasted "
               "text from typed-in text (\"ESC[200~\", followed by the pasted "
               "text, followed by \"ESC[201~\")"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_paste_bracketed, NULL, NULL,
            NULL, NULL, NULL);
        config_look_paste_bracketed_timer_delay = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "paste_bracketed_timer_delay", "integer",
            N_("force end of bracketed paste after this delay (in seconds) if "
               "the control sequence for end of bracketed paste (\"ESC[201~\") "
               "was not received in time"),
            NULL, 1, 60, "10", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_paste_max_lines = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "paste_max_lines", "integer",
            N_("max number of lines for paste without asking user "
               "(-1 = disable this feature); this option is used only if the "
               "bar item \"input_paste\" is used in at least one bar (by default "
               "it is used in \"input\" bar)"),
            NULL, -1, INT_MAX, "100", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_prefix[GUI_CHAT_PREFIX_ERROR] = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_error", "string",
            /* TRANSLATORS: string "${color:xxx}" must NOT be translated */
            N_("prefix for error messages (note: content is evaluated, so you "
               "can use colors with format \"${color:xxx}\", see /help eval)"),
            NULL, 0, 0, GUI_CHAT_PREFIX_ERROR_DEFAULT, NULL, 0,
            NULL, NULL, NULL,
            &config_change_prefix, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix[GUI_CHAT_PREFIX_NETWORK] = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_network", "string",
            /* TRANSLATORS: string "${color:xxx}" must NOT be translated */
            N_("prefix for network messages (note: content is evaluated, so you "
               "can use colors with format \"${color:xxx}\", see /help eval)"),
            NULL, 0, 0, GUI_CHAT_PREFIX_NETWORK_DEFAULT, NULL, 0,
            NULL, NULL, NULL,
            &config_change_prefix, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix[GUI_CHAT_PREFIX_ACTION] = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_action", "string",
            /* TRANSLATORS: string "${color:xxx}" must NOT be translated */
            N_("prefix for action messages (note: content is evaluated, so you "
               "can use colors with format \"${color:xxx}\", see /help eval)"),
            NULL, 0, 0, GUI_CHAT_PREFIX_ACTION_DEFAULT, NULL, 0,
            NULL, NULL, NULL,
            &config_change_prefix, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix[GUI_CHAT_PREFIX_JOIN] = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_join", "string",
            /* TRANSLATORS: string "${color:xxx}" must NOT be translated */
            N_("prefix for join messages (note: content is evaluated, so you "
               "can use colors with format \"${color:xxx}\", see /help eval)"),
            NULL, 0, 0, GUI_CHAT_PREFIX_JOIN_DEFAULT, NULL, 0,
            NULL, NULL, NULL,
            &config_change_prefix, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix[GUI_CHAT_PREFIX_QUIT] = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_quit", "string",
            /* TRANSLATORS: string "${color:xxx}" must NOT be translated */
            N_("prefix for quit messages (note: content is evaluated, so you "
               "can use colors with format \"${color:xxx}\", see /help eval)"),
            NULL, 0, 0, GUI_CHAT_PREFIX_QUIT_DEFAULT, NULL, 0,
            NULL, NULL, NULL,
            &config_change_prefix, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_align = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_align", "integer",
            N_("prefix alignment (none, left, right (default))"),
            "none|left|right", 0, 0, "right", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_align_max = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_align_max", "integer",
            N_("max size for prefix (0 = no max size)"),
            NULL, 0, 128, "0", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_align_min = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_align_min", "integer",
            N_("min size for prefix"),
            NULL, 0, 128, "0", NULL, 0,
            NULL, NULL, NULL,
            &config_change_prefix_align_min, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_align_more = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_align_more", "string",
            N_("char to display if prefix is truncated (must be exactly one "
               "char on screen)"),
            NULL, 0, 0, "+", NULL, 0,
            &config_check_prefix_align_more, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_align_more_after = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_align_more_after", "boolean",
            N_("display the truncature char (by default \"+\") after the text "
               "(by replacing the space that should be displayed here); if "
               "disabled, the truncature char replaces last char of text"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_buffer_align = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_buffer_align", "integer",
            N_("prefix alignment for buffer name, when many buffers are merged "
               "with same number (none, left, right (default))"),
            "none|left|right", 0, 0, "right", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_buffer_align_max = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_buffer_align_max", "integer",
            N_("max size for buffer name, when many buffers are merged with "
               "same number (0 = no max size)"),
            NULL, 0, 128, "0", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_buffer_align_more = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_buffer_align_more", "string",
            N_("char to display if buffer name is truncated (when many buffers "
               "are merged with same number) (must be exactly one char on "
               "screen)"),
            NULL, 0, 0, "+", NULL, 0,
            &config_check_prefix_buffer_align_more, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL,  NULL);
        config_look_prefix_buffer_align_more_after = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_buffer_align_more_after", "boolean",
            N_("display the truncature char (by default \"+\") after the text "
               "(by replacing the space that should be displayed here); if "
               "disabled, the truncature char replaces last char of text"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_same_nick = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_same_nick", "string",
            N_("prefix displayed for a message with same nick as previous "
               "but not next message: use a space \" \" to hide prefix, another "
               "string to display this string instead of prefix, or an empty "
               "string to disable feature (display prefix)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_prefix_same_nick, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_same_nick_middle = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_same_nick_middle", "string",
            N_("prefix displayed for a message with same nick as previous "
               "and next message: use a space \" \" to hide prefix, another "
               "string to display this string instead of prefix, or an empty "
               "string to disable feature (display prefix)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_prefix_same_nick_middle, NULL, NULL,
            NULL, NULL, NULL);
        config_look_prefix_suffix = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "prefix_suffix", "string",
            N_("string displayed after prefix"),
            NULL, 0, 0, "│", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_quote_nick_prefix = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "quote_nick_prefix", "string",
            N_("text to display before nick when quoting a message (see /help "
               "cursor)"),
            NULL, 0, 0, "<", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_quote_nick_suffix = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "quote_nick_suffix", "string",
            N_("text to display after nick when quoting a message (see /help "
               "cursor)"),
            NULL, 0, 0, ">", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_quote_time_format = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "quote_time_format", "string",
            N_("time format when quoting a message (see /help cursor)"),
            NULL, 0, 0, "%H:%M:%S", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_read_marker = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "read_marker", "integer",
            N_("use a marker (line or char) on buffers to show first unread "
               "line"),
            "none|line|char",
            0, 0, "line", NULL, 0,
            NULL, NULL, NULL,
            &config_change_read_marker, NULL, NULL,
            NULL, NULL, NULL);
        config_look_read_marker_always_show = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "read_marker_always_show", "boolean",
            N_("always show read marker, even if it is after last buffer line"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_read_marker_string = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "read_marker_string", "string",
            N_("string used to draw read marker line (string is repeated until "
               "end of line)"),
            NULL, 0, 0, "- ", NULL, 0,
            NULL, NULL, NULL,
            &config_change_read_marker, NULL, NULL,
            NULL, NULL, NULL);
        config_look_read_marker_update_on_buffer_switch = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "read_marker_update_on_buffer_switch", "boolean",
            N_("update the read marker when switching buffers"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_save_config_on_exit = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "save_config_on_exit", "boolean",
            N_("save configuration file on exit"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_save_config_on_exit, NULL, NULL,
            NULL, NULL, NULL);
        config_look_save_config_with_fsync = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "save_config_with_fsync", "boolean",
            N_("use fsync to synchronize the configuration file with the "
               "storage device (see man fsync); this is slower but should "
               "prevent any data loss in case of power failure during the "
               "save of configuration file"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL,
            &config_change_save_config_on_exit, NULL, NULL,
            NULL, NULL, NULL);
        config_look_save_layout_on_exit = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "save_layout_on_exit", "integer",
            N_("save layout on exit (buffers, windows, or both)"),
            "none|buffers|windows|all", 0, 0, "none", NULL, 0,
            NULL, NULL, NULL,
            &config_change_save_layout_on_exit, NULL, NULL,
            NULL, NULL, NULL);
        config_look_scroll_amount = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "scroll_amount", "integer",
            N_("how many lines to scroll by with scroll_up and "
               "scroll_down"),
            NULL, 1, INT_MAX, "3", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffer_content, NULL, NULL,
            NULL, NULL, NULL);
        config_look_scroll_bottom_after_switch = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "scroll_bottom_after_switch", "boolean",
            N_("scroll to bottom of window after switch to another buffer (do "
               "not remember scroll position in windows); the scroll is done "
               "only for buffers with formatted content (not free content)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_scroll_page_percent = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "scroll_page_percent", "integer",
            N_("percent of screen to scroll when scrolling one page up or "
               "down (for example 100 means one page, 50 half-page)"),
            NULL, 1, 100, "100", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_search_text_not_found_alert = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "search_text_not_found_alert", "boolean",
            N_("alert user when text sought is not found in buffer"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_separator_horizontal = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "separator_horizontal", "string",
            N_("char used to draw horizontal separators around bars and windows "
               "(empty value will draw a real line with ncurses, but may cause "
               "bugs with URL selection under some terminals); "
               "width on screen must be exactly one char"),
            NULL, 0, 0, "-", NULL, 0,
            &config_check_separator, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_separator_vertical = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "separator_vertical", "string",
            N_("char used to draw vertical separators around bars and windows "
               "(empty value will draw a real line with ncurses); "
               "width on screen must be exactly one char"),
            NULL, 0, 0, "", NULL, 0,
            &config_check_separator, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_tab_width = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "tab_width", "integer",
            N_("number of spaces used to display tabs in messages"),
            NULL, 1, TAB_MAX_WIDTH, "1", NULL, 0,
            NULL, NULL, NULL,
            &config_change_tab_width, NULL, NULL,
            NULL, NULL, NULL);
        config_look_time_format = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "time_format", "string",
            N_("time format for dates converted to strings and displayed in "
               "messages (see man strftime for date/time specifiers)"),
            NULL, 0, 0, "%a, %d %b %Y %T", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_window_auto_zoom = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "window_auto_zoom", "boolean",
            N_("automatically zoom on current window if the terminal becomes "
               "too small to display all windows (use alt-z to unzoom windows "
               "when the terminal is big enough)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_look_window_separator_horizontal = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "window_separator_horizontal", "boolean",
            N_("display an horizontal separator between windows"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_window_separator_vertical = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "window_separator_vertical", "boolean",
            N_("display a vertical separator between windows"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_buffers, NULL, NULL,
            NULL, NULL, NULL);
        config_look_window_title = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "window_title", "string",
            N_("title for window (terminal for Curses GUI), set on startup; "
               "an empty string will keep title unchanged "
               "(note: content is evaluated, see /help eval); example: "
               "\"WeeChat ${info:version}\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_window_title, NULL, NULL,
            NULL, NULL, NULL);
        config_look_word_chars_highlight = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "word_chars_highlight", "string",
            N_("comma-separated list of chars (or range of chars) that are "
               "considered part of words for highlights; "
               "each item can be a single char, a range of chars (format: a-z), "
               "a class of wide character (for example \"alnum\", "
               "see man wctype); a \"!\" before the item makes it negative "
               "(ie the char is NOT considered part of words); the value \"*\" "
               "matches any char; unicode chars are allowed with the format "
               "\\u1234, for example \\u00A0 for unbreakable space "
               "(see /help print for supported formats)"),
            NULL, 0, 0, "!\\u00A0,-,_,|,alnum", NULL, 0,
            NULL, NULL, NULL,
            &config_change_word_chars_highlight, NULL, NULL,
            NULL, NULL, NULL);
        config_look_word_chars_input = config_file_new_option (
            weechat_config_file, weechat_config_section_look,
            "word_chars_input", "string",
            N_("comma-separated list of chars (or range of chars) that are "
               "considered part of words for command line; "
               "each item can be a single char, a range of chars (format: a-z), "
               "a class of wide character (for example \"alnum\", "
               "see man wctype); a \"!\" before the item makes it negative "
               "(ie the char is NOT considered part of words); the value \"*\" "
               "matches any char; unicode chars are allowed with the format "
               "\\u1234, for example \\u00A0 for unbreakable space "
               "(see /help print for supported formats)"),
            NULL, 0, 0, "!\\u00A0,-,_,|,alnum", NULL, 0,
            NULL, NULL, NULL,
            &config_change_word_chars_input, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* palette */
    weechat_config_section_palette = config_file_new_section (
        weechat_config_file, "palette",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &config_weechat_palette_create_option_cb, NULL, NULL,
        &config_weechat_palette_delete_option_cb, NULL, NULL);

    /* colors */
    weechat_config_section_color = config_file_new_section (
        weechat_config_file, "color",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (weechat_config_section_color)
    {
        /* bar colors */
        config_color_bar_more = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "bar_more", "color",
            N_("text color for \"+\" when scrolling bars"),
            NULL, -1, 0, "lightmagenta", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        /* chat area */
        config_color_chat = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat", "color",
            N_("text color for chat"),
            NULL, GUI_COLOR_CHAT, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_bg = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_bg", "color",
            N_("background color for chat"),
            NULL, -1, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_buffer = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_buffer", "color",
            N_("text color for buffer names"),
            NULL, GUI_COLOR_CHAT_BUFFER, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_channel = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_channel", "color",
            N_("text color for channel names"),
            NULL, GUI_COLOR_CHAT_CHANNEL, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_day_change = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_day_change", "color",
            N_("text color for message displayed when the day has changed"),
            NULL, GUI_COLOR_CHAT_DAY_CHANGE, 0, "cyan", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_delimiters = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_delimiters", "color",
            N_("text color for delimiters"),
            NULL, GUI_COLOR_CHAT_DELIMITERS, 0, "22", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_highlight = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_highlight", "color",
            N_("text color for highlighted prefix"),
            NULL, GUI_COLOR_CHAT_HIGHLIGHT, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_highlight_bg = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_highlight_bg", "color",
            N_("background color for highlighted prefix"),
            NULL, -1, 0, "124", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_host = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_host", "color",
            N_("text color for hostnames"),
            NULL, GUI_COLOR_CHAT_HOST, 0, "cyan", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_inactive_buffer = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_inactive_buffer", "color",
            N_("text color for chat when line is inactive (buffer is merged "
               "with other buffers and is not selected)"),
            NULL, GUI_COLOR_CHAT_INACTIVE_BUFFER, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_inactive_window = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_inactive_window", "color",
            N_("text color for chat when window is inactive (not current "
               "selected window)"),
            NULL, GUI_COLOR_CHAT_INACTIVE_WINDOW, 0, "240", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_nick = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_nick", "color",
            N_("text color for nicks in chat window: used in some server "
               "messages and as fallback when a nick color is not found; most "
               "of times nick color comes from option "
               "weechat.color.chat_nick_colors"),
            NULL, GUI_COLOR_CHAT_NICK, 0, "lightcyan", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_nick_colors = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_nick_colors", "string",
            /* TRANSLATORS: please do not translate "lightred:blue" */
            N_("text color for nicks (comma separated list of colors, "
               "background is allowed with format: \"fg:bg\", for example: "
               "\"lightred:blue\")"),
            NULL, 0, 0,
            "cyan,magenta,green,brown,lightblue,lightcyan,lightmagenta,"
            "lightgreen,31,35,38,40,49,63,70,80,92,99,112,126,130,138,142,148,"
            "160,162,167,169,174,176,178,184,186,210,212,215,248",
            NULL, 0,
            NULL, NULL, NULL,
            &config_change_nick_colors, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_nick_offline = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_nick_offline", "color",
            N_("text color for offline nick (not in nicklist any more); this "
               "color is used only if option weechat.look.color_nick_offline is "
               "enabled"),
            NULL, GUI_COLOR_CHAT_NICK_OFFLINE, 0, "242", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_nick_offline_highlight = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_nick_offline_highlight", "color",
            N_("text color for offline nick with highlight; this color is used "
               "only if option weechat.look.color_nick_offline is enabled"),
            NULL, -1, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_nick_offline_highlight_bg = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_nick_offline_highlight_bg", "color",
            N_("background color for offline nick with highlight; this color is "
               "used only if option weechat.look.color_nick_offline is enabled"),
            NULL, -1, 0, "17", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_nick_other = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_nick_other", "color",
            N_("text color for other nick in private buffer"),
            NULL, GUI_COLOR_CHAT_NICK_OTHER, 0, "cyan", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_nick_prefix = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_nick_prefix", "color",
            N_("color for nick prefix (string displayed before nick in prefix)"),
            NULL, GUI_COLOR_CHAT_NICK_PREFIX, 0, "green", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_nick_self = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_nick_self", "color",
            N_("text color for local nick in chat window"),
            NULL, GUI_COLOR_CHAT_NICK_SELF, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_nick_suffix = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_nick_suffix", "color",
            N_("color for nick suffix (string displayed after nick in prefix)"),
            NULL, GUI_COLOR_CHAT_NICK_SUFFIX, 0, "green", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_prefix[GUI_CHAT_PREFIX_ERROR] = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_prefix_error", "color",
            N_("text color for error prefix"),
            NULL, GUI_COLOR_CHAT_PREFIX_ERROR, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_prefix_buffer = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_prefix_buffer", "color",
            N_("text color for buffer name (before prefix, when many buffers "
               "are merged with same number)"),
            NULL, GUI_COLOR_CHAT_PREFIX_BUFFER, 0, "180", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_prefix_buffer_inactive_buffer = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_prefix_buffer_inactive_buffer", "color",
            N_("text color for inactive buffer name (before prefix, when many "
               "buffers are merged with same number and if buffer is not "
               "selected)"),
            NULL, GUI_COLOR_CHAT_PREFIX_BUFFER_INACTIVE_BUFFER, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_prefix[GUI_CHAT_PREFIX_NETWORK] = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_prefix_network", "color",
            N_("text color for network prefix"),
            NULL, GUI_COLOR_CHAT_PREFIX_NETWORK, 0, "magenta", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_prefix[GUI_CHAT_PREFIX_ACTION] = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_prefix_action", "color",
            N_("text color for action prefix"),
            NULL, GUI_COLOR_CHAT_PREFIX_ACTION, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_prefix[GUI_CHAT_PREFIX_JOIN] = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_prefix_join", "color",
            N_("text color for join prefix"),
            NULL, GUI_COLOR_CHAT_PREFIX_JOIN, 0, "lightgreen", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_prefix[GUI_CHAT_PREFIX_QUIT] = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_prefix_quit", "color",
            N_("text color for quit prefix"),
            NULL, GUI_COLOR_CHAT_PREFIX_QUIT, 0, "lightred", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_prefix_more = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_prefix_more", "color",
            N_("text color for \"+\" when prefix is too long"),
            NULL, GUI_COLOR_CHAT_PREFIX_MORE, 0, "lightmagenta", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_prefix_suffix = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_prefix_suffix", "color",
            N_("text color for suffix (after prefix)"),
            NULL, GUI_COLOR_CHAT_PREFIX_SUFFIX, 0, "24", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_read_marker = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_read_marker", "color",
            N_("text color for unread data marker"),
            NULL, GUI_COLOR_CHAT_READ_MARKER, 0, "magenta", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_read_marker_bg = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_read_marker_bg", "color",
            N_("background color for unread data marker"),
            NULL, -1, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_server = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_server", "color",
            N_("text color for server names"),
            NULL, GUI_COLOR_CHAT_SERVER, 0, "brown", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_status_disabled = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_status_disabled", "color",
            N_("text color for \"disabled\" status"),
            NULL, GUI_COLOR_CHAT_STATUS_DISABLED, 0, "red", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_status_enabled = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_status_enabled", "color",
            N_("text color for \"enabled\" status"),
            NULL, GUI_COLOR_CHAT_STATUS_ENABLED, 0, "green", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_tags = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_tags", "color",
            N_("text color for tags after messages (displayed with command "
               "/debug tags)"),
            NULL, GUI_COLOR_CHAT_TAGS, 0, "red", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_text_found = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_text_found", "color",
            N_("text color for marker on lines where text sought is found"),
            NULL, GUI_COLOR_CHAT_TEXT_FOUND, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_text_found_bg = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_text_found_bg", "color",
            N_("background color for marker on lines where text sought is "
               "found"),
            NULL, -1, 0, "lightmagenta", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_time = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_time", "color",
            N_("text color for time in chat window"),
            NULL, GUI_COLOR_CHAT_TIME, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_time_delimiters = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_time_delimiters", "color",
            N_("text color for time delimiters"),
            NULL, GUI_COLOR_CHAT_TIME_DELIMITERS, 0, "brown", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_value = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_value", "color",
            N_("text color for values"),
            NULL, GUI_COLOR_CHAT_VALUE, 0, "cyan", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_chat_value_null = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "chat_value_null", "color",
            N_("text color for null values (undefined)"),
            NULL, GUI_COLOR_CHAT_VALUE_NULL, 0, "blue", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        /* emphasis (chat/bars) */
        config_color_emphasized = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "emphasized", "color",
            N_("text color for emphasized text (for example when searching "
               "text); this option is used only if option "
               "weechat.look.emphasized_attributes is an empty string "
               "(default value)"),
            NULL, GUI_COLOR_EMPHASIS, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_emphasized_bg = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "emphasized_bg", "color",
            N_("background color for emphasized text (for example when searching "
               "text); used only if option weechat.look.emphasized_attributes "
               "is an empty string (default value)"),
            NULL, -1, 0, "54", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        /* input bar */
        config_color_input_actions = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "input_actions", "color",
            N_("text color for actions in input line"),
            NULL, -1, 0, "lightgreen", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_input_text_not_found = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "input_text_not_found", "color",
            N_("text color for unsuccessful text search in input line"),
            NULL, -1, 0, "red", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        /* items */
        config_color_item_away = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "item_away", "color",
            N_("text color for away item"),
            NULL, -1, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &config_change_item_away, NULL, NULL,
            NULL, NULL, NULL);
        /* nicklist bar */
        config_color_nicklist_away = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "nicklist_away", "color",
            N_("text color for away nicknames"),
            NULL, -1, 0, "240", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_nicklist_group = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "nicklist_group", "color",
            N_("text color for groups in nicklist"),
            NULL, -1, 0, "green", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        /* general color settings */
        config_color_separator = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "separator", "color",
            N_("color for window separators (when split) and separators beside "
               "bars (like nicklist)"),
            NULL, GUI_COLOR_SEPARATOR, 0, "236", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        /* status bar */
        config_color_status_count_highlight = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_count_highlight", "color",
            N_("text color for count of highlight messages in hotlist "
               "(status bar)"),
            NULL, -1, 0, "magenta", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_count_msg = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_count_msg", "color",
            N_("text color for count of messages in hotlist (status bar)"),
            NULL, -1, 0, "brown", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_count_other = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_count_other", "color",
            N_("text color for count of other messages in hotlist (status bar)"),
            NULL, -1, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_count_private = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_count_private", "color",
            N_("text color for count of private messages in hotlist "
               "(status bar)"),
            NULL, -1, 0, "green", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_data_highlight = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_data_highlight", "color",
            N_("text color for buffer with highlight (status bar)"),
            NULL, -1, 0, "lightmagenta", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_data_msg = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_data_msg", "color",
            N_("text color for buffer with new messages (status bar)"),
            NULL, -1, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_data_other = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_data_other", "color",
            N_("text color for buffer with new data (not messages) "
               "(status bar)"),
            NULL, -1, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_data_private = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_data_private", "color",
            N_("text color for buffer with private message (status bar)"),
            NULL, -1, 0, "lightgreen", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_filter = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_filter", "color",
            N_("text color for filter indicator in status bar"),
            NULL, -1, 0, "green", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_more = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_more", "color",
            N_("text color for buffer with new data (status bar)"),
            NULL, -1, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_mouse = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_mouse", "color",
            N_("text color for mouse indicator in status bar"),
            NULL, -1, 0, "lightgreen", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_name = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_name", "color",
            N_("text color for current buffer name in status bar"),
            NULL, -1, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_name_insecure = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_name_insecure", "color",
            N_("text color for current buffer name in status bar, if data are "
               "exchanged and not secured with a protocol like TLS"),
            NULL, -1, 0, "lightmagenta", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_name_tls = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_name_tls", "color",
            N_("text color for current buffer name in status bar, if data are "
               "exchanged and secured with a protocol like TLS"),
            NULL, -1, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_nicklist_count = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_nicklist_count", "color",
            N_("text color for number of nicks in nicklist (status bar)"),
            NULL, -1, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_number = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_number", "color",
            N_("text color for current buffer number in status bar"),
            NULL, -1, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
        config_color_status_time = config_file_new_option (
            weechat_config_file, weechat_config_section_color,
            "status_time", "color",
            N_("text color for time (status bar)"),
            NULL, -1, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &config_change_color, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* completion */
    weechat_config_section_completion = config_file_new_section (
        weechat_config_file, "completion",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (weechat_config_section_completion)
    {
        config_completion_base_word_until_cursor = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "base_word_until_cursor", "boolean",
            N_("if enabled, the base word to complete ends at char before cursor; "
               "otherwise the base word ends at first space after cursor"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_command_inline = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "command_inline", "boolean",
            N_("if enabled, the commands inside command line are completed (the "
               "command at beginning of line has higher priority and is used "
               "first); note: when this option is enabled, there is no more "
               "automatic completion of paths beginning with \"/\" (outside "
               "commands arguments)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_default_template = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "default_template", "string",
            N_("default completion template (please see documentation for "
               "template codes and values: plugin API reference, function "
               "\"weechat_hook_command\")"),
            NULL, 0, 0, "%(nicks)|%(irc_channels)", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_nick_add_space = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "nick_add_space", "boolean",
            N_("add space after nick completion (when nick is not first word "
               "on command line)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_nick_case_sensitive = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "nick_case_sensitive", "boolean",
            N_("case sensitive completion for nicks"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_nick_completer = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "nick_completer", "string",
            N_("string inserted after nick completion (when nick is first word "
               "on command line)"),
            NULL, 0, 0, ": ", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_nick_first_only = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "nick_first_only", "boolean",
            N_("complete only with first nick found"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_nick_ignore_chars = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "nick_ignore_chars", "string",
            N_("chars ignored for nick completion"),
            NULL, 0, 0, "[]`_-^", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_partial_completion_alert = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "partial_completion_alert", "boolean",
            N_("send alert (BEL) when a partial completion occurs"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_partial_completion_command = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "partial_completion_command", "boolean",
            N_("partially complete command names (stop when many commands found "
               "begin with same letters)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_partial_completion_command_arg = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "partial_completion_command_arg", "boolean",
            N_("partially complete command arguments (stop when many arguments "
               "found begin with same prefix)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_partial_completion_count = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "partial_completion_count", "boolean",
            N_("display count for each partial completion in bar item"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_partial_completion_other = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "partial_completion_other", "boolean",
            N_("partially complete outside commands (stop when many words found "
               "begin with same letters)"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_completion_partial_completion_templates = config_file_new_option (
            weechat_config_file, weechat_config_section_completion,
            "partial_completion_templates", "string",
            N_("comma-separated list of templates for which partial completion "
               "is enabled by default (with Tab key instead of shift-Tab); "
               "the list of templates is in documentation: plugin API reference, "
               "function \"weechat_hook_command\""),
            NULL, 0, 0, "config_options", NULL, 0,
            NULL, NULL, NULL,
            &config_change_completion_partial_completion_templates, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* history */
    weechat_config_section_history = config_file_new_section (
        weechat_config_file, "history",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (weechat_config_section_history)
    {
        config_history_display_default = config_file_new_option (
            weechat_config_file, weechat_config_section_history,
            "display_default", "integer",
            N_("maximum number of commands to display by default in "
               "history listing (0 = unlimited)"),
            NULL, 0, INT_MAX, "5", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_history_max_buffer_lines_minutes = config_file_new_option (
            weechat_config_file, weechat_config_section_history,
            "max_buffer_lines_minutes", "integer",
            N_("maximum number of minutes in history per buffer "
               "(0 = unlimited); examples: 1440 = one day, 10080 = one week, "
               "43200 = one month, 525600 = one year; use 0 ONLY if option "
               "weechat.history.max_buffer_lines_number is NOT set to 0"),
            NULL, 0, INT_MAX, "0", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_history_max_buffer_lines_number = config_file_new_option (
            weechat_config_file, weechat_config_section_history,
            "max_buffer_lines_number", "integer",
            N_("maximum number of lines in history per buffer "
               "(0 = unlimited); use 0 ONLY if option "
               "weechat.history.max_buffer_lines_minutes is NOT set to 0"),
            NULL, 0, INT_MAX, "4096", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_history_max_commands = config_file_new_option (
            weechat_config_file, weechat_config_section_history,
            "max_commands", "integer",
            N_("maximum number of user commands in history (0 = "
               "unlimited, NOT RECOMMENDED: no limit in memory usage)"),
            NULL, 0, INT_MAX, "100", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_history_max_visited_buffers = config_file_new_option (
            weechat_config_file, weechat_config_section_history,
            "max_visited_buffers", "integer",
            N_("maximum number of visited buffers to keep in memory"),
            NULL, 0, 1000, "50", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* proxies */
    weechat_config_section_proxy = config_file_new_section (
        weechat_config_file, "proxy",
        0, 0,
        &config_weechat_proxy_read_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);

    /* network */
    weechat_config_section_network = config_file_new_section (
        weechat_config_file, "network",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (weechat_config_section_network)
    {
        config_network_connection_timeout = config_file_new_option (
            weechat_config_file, weechat_config_section_network,
            "connection_timeout", "integer",
            N_("timeout (in seconds) for connection to a remote host (made in a "
               "child process)"),
            NULL, 1, INT_MAX, "60", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_network_gnutls_ca_system = config_file_new_option (
            weechat_config_file, weechat_config_section_network,
            "gnutls_ca_system", "boolean",
            N_("load system's default trusted certificate authorities on startup; "
               "this can be turned off to save some memory only if you are not "
               "using TLS connections at all"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &config_change_network_gnutls_ca, NULL, NULL,
            NULL, NULL, NULL);
        config_network_gnutls_ca_user = config_file_new_option (
            weechat_config_file, weechat_config_section_network,
            "gnutls_ca_user", "string",
            N_("extra file(s) with certificate authorities; multiple files must "
               "be separated by colons "
               "(each path is evaluated, see function string_eval_path_home in "
               "plugin API reference)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &config_change_network_gnutls_ca, NULL, NULL,
            NULL, NULL, NULL);
        config_network_gnutls_handshake_timeout = config_file_new_option (
            weechat_config_file, weechat_config_section_network,
            "gnutls_handshake_timeout", "integer",
            N_("timeout (in seconds) for gnutls handshake"),
            NULL, 1, INT_MAX, "30", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_network_proxy_curl = config_file_new_option (
            weechat_config_file, weechat_config_section_network,
            "proxy_curl", "string",
            N_("name of proxy used for download of URLs with Curl (used to "
               "download list of scripts and in scripts calling function "
               "hook_process); the proxy must be defined with command /proxy"),
            NULL, 0, 0, "", NULL, 0,
            &config_check_proxy_curl, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* plugin */
    weechat_config_section_plugin = config_file_new_section (
        weechat_config_file, "plugin",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (weechat_config_section_plugin)
    {
        config_plugin_autoload = config_file_new_option (
            weechat_config_file, weechat_config_section_plugin,
            "autoload", "string",
            N_("comma separated list of plugins to load automatically "
               "at startup, \"*\" means all plugins found, a name beginning with "
               "\"!\" is a negative value to prevent a plugin from being loaded, "
               "wildcard \"*\" is allowed in names (examples: \"*\" or "
               "\"*,!lua,!tcl\")"),
            NULL, 0, 0, "*", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_plugin_extension = config_file_new_option (
            weechat_config_file, weechat_config_section_plugin,
            "extension", "string",
            N_("comma separated list of file name extensions for plugins"),
            NULL, 0, 0, ".so,.dll", NULL, 0,
            NULL, NULL, NULL,
            &config_change_plugin_extension, NULL, NULL,
            NULL, NULL, NULL);
        config_plugin_path = config_file_new_option (
            weechat_config_file, weechat_config_section_plugin,
            "path", "string",
            N_("path for searching plugins "
               "(path is evaluated, see function string_eval_path_home in "
               "plugin API reference)"),
            NULL, 0, 0, "${weechat_data_dir}/plugins", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_plugin_save_config_on_unload = config_file_new_option (
            weechat_config_file, weechat_config_section_plugin,
            "save_config_on_unload", "boolean",
            N_("save configuration files when unloading plugins"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* signal */
    weechat_config_section_signal = config_file_new_section (
        weechat_config_file, "signal",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (weechat_config_section_signal)
    {
        config_signal_sighup = config_file_new_option (
            weechat_config_file, weechat_config_section_signal,
            "sighup", "string",
            N_("command to execute when the signal is received, "
               "multiple commands can be separated by semicolons "
               "(note: commands are evaluated, see /help eval)"),
            NULL, 0, 0,
            "${if:${info:weechat_headless}?/reload:/quit -yes}", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_signal_sigquit = config_file_new_option (
            weechat_config_file, weechat_config_section_signal,
            "sigquit", "string",
            N_("command to execute when the signal is received, "
               "multiple commands can be separated by semicolons "
               "(note: commands are evaluated, see /help eval)"),
            NULL, 0, 0, "/quit -yes", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_signal_sigterm = config_file_new_option (
            weechat_config_file, weechat_config_section_signal,
            "sigterm", "string",
            N_("command to execute when the signal is received, "
               "multiple commands can be separated by semicolons "
               "(note: commands are evaluated, see /help eval)"),
            NULL, 0, 0, "/quit -yes", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_signal_sigusr1 = config_file_new_option (
            weechat_config_file, weechat_config_section_signal,
            "sigusr1", "string",
            N_("command to execute when the signal is received, "
               "multiple commands can be separated by semicolons "
               "(note: commands are evaluated, see /help eval)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        config_signal_sigusr2 = config_file_new_option (
            weechat_config_file, weechat_config_section_signal,
            "sigusr2", "string",
            N_("command to execute when the signal is received, "
               "multiple commands can be separated by semicolons "
               "(note: commands are evaluated, see /help eval)"),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    /* bars */
    weechat_config_section_bar = config_file_new_section (
        weechat_config_file, "bar",
        0, 0,
        &config_weechat_bar_read_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);

    /* custom bar items */
    weechat_config_section_custom_bar_item = config_file_new_section (
        weechat_config_file, "custom_bar_item",
        0, 0,
        &config_weechat_custom_bar_item_read_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);

    /* layout */
    weechat_config_section_layout = config_file_new_section (
        weechat_config_file, "layout",
        0, 0,
        &config_weechat_layout_read_cb, NULL, NULL,
        &config_weechat_layout_write_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);

    /* notify */
    weechat_config_section_notify = config_file_new_section (
        weechat_config_file, "notify",
        1, 1,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        &config_weechat_notify_create_option_cb, NULL, NULL,
        &config_weechat_notify_delete_option_cb, NULL, NULL);

    /* filters */
    weechat_config_section_filter = config_file_new_section (
        weechat_config_file, "filter",
        0, 0,
        &config_weechat_filter_read_cb, NULL, NULL,
        &config_weechat_filter_write_cb, NULL, NULL,
        &config_weechat_filter_write_cb, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);

    /* keys */
    for (context = 0; context < GUI_KEY_NUM_CONTEXTS; context++)
    {
        snprintf (
            section_name, sizeof (section_name),
            "key%s%s",
            (context == GUI_KEY_CONTEXT_DEFAULT) ? "" : "_",
            (context == GUI_KEY_CONTEXT_DEFAULT) ?
            "" : gui_key_context_string[context]);
        weechat_config_section_key[context] = config_file_new_section (
            weechat_config_file, section_name,
            1, 1,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            NULL, NULL, NULL,
            &config_weechat_key_create_option_cb, NULL, NULL,
            &config_weechat_key_delete_option_cb, NULL, NULL);
    }

    return 1;
}

/*
 * Initializes WeeChat configuration.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
config_weechat_init ()
{
    int rc;
    struct timeval tv_time;
    struct tm *local_time;
    time_t seconds;

    snprintf (config_tab_spaces, sizeof (config_tab_spaces), " ");

    rc = config_weechat_init_options ();

    if (!rc)
    {
        gui_chat_printf (NULL,
                         _("FATAL: error initializing configuration options"));
    }

    if (!config_day_change_timer)
    {
        /* create timer to check if day has changed */
        gettimeofday (&tv_time, NULL);
        seconds = tv_time.tv_sec;
        local_time = localtime (&seconds);
        config_day_change_old_day = local_time->tm_mday;
        config_day_change_timer = hook_timer (NULL,
                                              60 * 1000, /* each minute */
                                              60, /* when second is 00 */
                                              0,
                                              &config_day_change_timer_cb,
                                              NULL, NULL);
    }
    if (!config_highlight_disable_regex)
        config_change_highlight_disable_regex (NULL, NULL, NULL);
    if (!config_highlight_regex)
        config_change_highlight_regex (NULL, NULL, NULL);
    if (!config_highlight_tags)
        config_change_highlight_tags (NULL, NULL, NULL);
    if (!config_plugin_extensions)
        config_change_plugin_extension (NULL, NULL, NULL);
    if (!config_word_chars_highlight)
        config_change_word_chars_highlight (NULL, NULL, NULL);
    if (!config_word_chars_input)
        config_change_word_chars_input (NULL, NULL, NULL);
    if (!config_hashtable_completion_partial_templates)
        config_change_completion_partial_completion_templates (NULL, NULL, NULL);

    return rc;
}

/*
 * Reads WeeChat configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_READ_OK: OK
 *   WEECHAT_CONFIG_READ_MEMORY_ERROR: not enough memory
 *   WEECHAT_CONFIG_READ_FILE_NOT_FOUND: file not found
 */

int
config_weechat_read ()
{
    int rc;

    config_loading = 1;
    rc = config_file_read (weechat_config_file);
    config_loading = 0;

    config_weechat_init_after_read ();

    return rc;
}

/*
 * Writes WeeChat configuration file.
 *
 * Returns:
 *   WEECHAT_CONFIG_WRITE_OK: OK
 *   WEECHAT_CONFIG_WRITE_ERROR: error
 *   WEECHAT_CONFIG_WRITE_MEMORY_ERROR: not enough memory
 */

int
config_weechat_write ()
{
    return config_file_write (weechat_config_file);
}

/*
 * Frees WeeChat configuration file and variables.
 */

void
config_weechat_free ()
{
    config_file_free (weechat_config_file);

    if (config_highlight_disable_regex)
    {
        regfree (config_highlight_disable_regex);
        free (config_highlight_disable_regex);
        config_highlight_disable_regex = NULL;
    }

    if (config_highlight_regex)
    {
        regfree (config_highlight_regex);
        free (config_highlight_regex);
        config_highlight_regex = NULL;
    }

    if (config_highlight_tags)
    {
        string_free_split_tags (config_highlight_tags);
        config_highlight_tags = NULL;
    }
    config_num_highlight_tags = 0;

    if (config_plugin_extensions)
    {
        string_free_split (config_plugin_extensions);
        config_plugin_extensions = NULL;
        config_num_plugin_extensions = 0;
    }

    if (config_word_chars_highlight)
    {
        free (config_word_chars_highlight);
        config_word_chars_highlight = NULL;
        config_word_chars_highlight_count = 0;
    }

    if (config_word_chars_input)
    {
        free (config_word_chars_input);
        config_word_chars_input = NULL;
        config_word_chars_input_count = 0;
    }

    if (config_nick_colors)
    {
        string_free_split (config_nick_colors);
        config_nick_colors = NULL;
        config_num_nick_colors = 0;
    }

    if (config_hashtable_nick_color_force)
    {
        hashtable_free (config_hashtable_nick_color_force);
        config_hashtable_nick_color_force = NULL;
    }

    if (config_item_time_evaluated)
    {
        free (config_item_time_evaluated);
        config_item_time_evaluated = NULL;
    }

    if (config_buffer_time_same_evaluated)
    {
        free (config_buffer_time_same_evaluated);
        config_buffer_time_same_evaluated = NULL;
    }

    if (config_hashtable_completion_partial_templates)
    {
        hashtable_free (config_hashtable_completion_partial_templates);
        config_hashtable_completion_partial_templates = NULL;
    }
}
