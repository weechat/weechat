/*
 * plugin-api-info.c - extra info functions for plugin API
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

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-crypto.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-proxy.h"
#include "../core/wee-secure.h"
#include "../core/wee-string.h"
#include "../core/wee-url.h"
#include "../core/wee-util.h"
#include "../core/wee-version.h"
#include "../gui/gui-bar.h"
#include "../gui/gui-bar-item.h"
#include "../gui/gui-bar-window.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-color.h"
#include "../gui/gui-cursor.h"
#include "../gui/gui-filter.h"
#include "../gui/gui-focus.h"
#include "../gui/gui-history.h"
#include "../gui/gui-hotlist.h"
#include "../gui/gui-key.h"
#include "../gui/gui-layout.h"
#include "../gui/gui-line.h"
#include "../gui/gui-nick.h"
#include "../gui/gui-nicklist.h"
#include "../gui/gui-window.h"
#include "plugin.h"


/*
 * Returns WeeChat info "version".
 */

char *
plugin_api_info_version_cb (const void *pointer, void *data,
                            const char *info_name,
                            const char *arguments)
{
    const char *version;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    version = version_get_version ();
    return (version) ? strdup (version) : NULL;
}

/*
 * Returns WeeChat info "version_number".
 */

char *
plugin_api_info_version_number_cb (const void *pointer, void *data,
                                   const char *info_name,
                                   const char *arguments)
{
    char version_number[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (
        version_number, sizeof (version_number), "%d",
        util_version_number (
            (arguments && arguments[0]) ? arguments : version_get_version ()));
    return strdup (version_number);
}

/*
 * Returns WeeChat info "version_git".
 */

char *
plugin_api_info_version_git_cb (const void *pointer, void *data,
                                const char *info_name,
                                const char *arguments)
{
    const char *version;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    version = version_get_git ();
    return (version) ? strdup (version) : NULL;
}

/*
 * Returns WeeChat info "date".
 */

char *
plugin_api_info_date_cb (const void *pointer, void *data,
                         const char *info_name,
                         const char *arguments)
{
    const char *version;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    version = version_get_compilation_date_time ();
    return (version) ? strdup (version) : NULL;
}

/*
 * Returns WeeChat info "pid".
 */

char *
plugin_api_info_pid_cb (const void *pointer, void *data,
                        const char *info_name,
                        const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", (int)getpid ());
    return strdup (value);
}

/*
 * Returns WeeChat info "dir_separator".
 */

char *
plugin_api_info_dir_separator_cb (const void *pointer, void *data,
                                  const char *info_name,
                                  const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (DIR_SEPARATOR);
}

/*
 * Returns an absolute path to directory.
 *
 * Note: result must be freed after use.
 */

char *
plugin_api_info_absolute_path (const char *directory)
{
    char absolute_path[PATH_MAX];

    if (!realpath (directory, absolute_path))
        return NULL;
    return strdup ((absolute_path[0]) ? absolute_path : directory);
}

/*
 * Returns WeeChat info "weechat_config_dir".
 */

char *
plugin_api_info_weechat_config_dir_cb (const void *pointer, void *data,
                                       const char *info_name,
                                       const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return plugin_api_info_absolute_path (weechat_config_dir);
}

/*
 * Returns WeeChat info "weechat_data_dir".
 */

char *
plugin_api_info_weechat_data_dir_cb (const void *pointer, void *data,
                                     const char *info_name,
                                     const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return plugin_api_info_absolute_path (weechat_data_dir);
}

/*
 * Returns WeeChat info "weechat_cache_dir".
 */

char *
plugin_api_info_weechat_cache_dir_cb (const void *pointer, void *data,
                                      const char *info_name,
                                      const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return plugin_api_info_absolute_path (weechat_cache_dir);
}

/*
 * Returns WeeChat info "weechat_runtime_dir".
 */

char *
plugin_api_info_weechat_runtime_dir_cb (const void *pointer, void *data,
                                        const char *info_name,
                                        const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return plugin_api_info_absolute_path (weechat_runtime_dir);
}

/*
 * Returns WeeChat info "weechat_libdir".
 */

char *
plugin_api_info_weechat_libdir_cb (const void *pointer, void *data,
                                   const char *info_name,
                                   const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (WEECHAT_LIBDIR);
}

/*
 * Returns WeeChat info "weechat_sharedir".
 */

char *
plugin_api_info_weechat_sharedir_cb (const void *pointer, void *data,
                                     const char *info_name,
                                     const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (WEECHAT_SHAREDIR);
}

/*
 * Returns WeeChat info "weechat_localedir".
 */

char *
plugin_api_info_weechat_localedir_cb (const void *pointer, void *data,
                                      const char *info_name,
                                      const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (LOCALEDIR);
}

/*
 * Returns WeeChat info "weechat_site".
 */

char *
plugin_api_info_weechat_site_cb (const void *pointer, void *data,
                                 const char *info_name,
                                 const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (WEECHAT_WEBSITE);
}

/*
 * Returns WeeChat info "weechat_site_download".
 */

char *
plugin_api_info_weechat_site_download_cb (const void *pointer, void *data,
                                          const char *info_name,
                                          const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (WEECHAT_WEBSITE_DOWNLOAD);
}

/*
 * Returns WeeChat info "weechat_upgrading".
 */

char *
plugin_api_info_weechat_upgrading_cb (const void *pointer, void *data,
                                      const char *info_name,
                                      const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", weechat_upgrading);
    return strdup (value);
}

/*
 * Returns WeeChat info "weechat_headless".
 */

char *
plugin_api_info_weechat_headless_cb (const void *pointer, void *data,
                                     const char *info_name,
                                     const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", weechat_headless);
    return strdup (value);
}

/*
 * Returns WeeChat info "weechat_daemon".
 */

char *
plugin_api_info_weechat_daemon_cb (const void *pointer, void *data,
                                   const char *info_name,
                                   const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", weechat_daemon);
    return strdup (value);
}

/*
 * Returns WeeChat info "auto_connect".
 */

char *
plugin_api_info_auto_connect_cb (const void *pointer, void *data,
                                 const char *info_name,
                                 const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", weechat_auto_connect);
    return strdup (value);
}

/*
 * Returns WeeChat info "auto_load_scripts".
 */

char *
plugin_api_info_auto_load_scripts_cb (const void *pointer, void *data,
                                      const char *info_name,
                                      const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", weechat_auto_load_scripts);
    return strdup (value);
}

/*
 * Returns WeeChat info "charset_terminal".
 */

char *
plugin_api_info_charset_terminal_cb (const void *pointer, void *data,
                                     const char *info_name,
                                     const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return (weechat_local_charset) ? strdup (weechat_local_charset) : NULL;
}

/*
 * Returns WeeChat info "charset_internal".
 */

char *
plugin_api_info_charset_internal_cb (const void *pointer, void *data,
                                     const char *info_name,
                                     const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (WEECHAT_INTERNAL_CHARSET);
}

/*
 * Returns WeeChat info "locale".
 */

char *
plugin_api_info_locale_cb (const void *pointer, void *data,
                           const char *info_name,
                           const char *arguments)
{
    char *locale;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    locale = setlocale (LC_MESSAGES, NULL);
    return (locale) ? strdup (locale) : NULL;
}

/*
 * Returns WeeChat info "inactivity".
 */

char *
plugin_api_info_inactivity_cb (const void *pointer, void *data,
                               const char *info_name,
                               const char *arguments)
{
    time_t inactivity;
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    if (gui_key_last_activity_time == 0)
        inactivity = 0;
    else
        inactivity = time (NULL) - gui_key_last_activity_time;

    snprintf (value, sizeof (value), "%lld", (long long)inactivity);

    return strdup (value);
}

/*
 * Returns WeeChat info "filters_enabled".
 */

char *
plugin_api_info_filters_enabled_cb (const void *pointer, void *data,
                                    const char *info_name,
                                    const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", gui_filters_enabled);
    return strdup (value);
}

/*
 * Returns WeeChat info "cursor_mode".
 */

char *
plugin_api_info_cursor_mode_cb (const void *pointer, void *data,
                                const char *info_name,
                                const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", gui_cursor_mode);
    return strdup (value);
}

/*
 * Returns WeeChat info "term_width".
 */

char *
plugin_api_info_term_width_cb (const void *pointer, void *data,
                               const char *info_name,
                               const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", gui_window_get_width ());
    return strdup (value);
}

/*
 * Returns WeeChat info "term_height".
 */

char *
plugin_api_info_term_height_cb (const void *pointer, void *data,
                                const char *info_name,
                                const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", gui_window_get_height ());
    return strdup (value);
}

/*
 * Returns WeeChat info "term_colors".
 */

char *
plugin_api_info_term_colors_cb (const void *pointer, void *data,
                                const char *info_name,
                                const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", gui_color_get_term_colors ());
    return strdup (value);
}

/*
 * Returns WeeChat info "term_color_pairs".
 */

char *
plugin_api_info_term_color_pairs_cb (const void *pointer, void *data,
                                     const char *info_name,
                                     const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    snprintf (value, sizeof (value), "%d", gui_color_get_term_color_pairs ());
    return strdup (value);
}

/*
 * Returns WeeChat info "color_ansi_regex".
 */

char *
plugin_api_info_color_ansi_regex_cb (const void *pointer, void *data,
                                     const char *info_name,
                                     const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) arguments;

    return strdup (GUI_COLOR_REGEX_ANSI_DECODE);
}

/*
 * Returns WeeChat info "color_term2rgb".
 */

char *
plugin_api_info_color_term2rgb_cb (const void *pointer, void *data,
                                   const char *info_name,
                                   const char *arguments)
{
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    snprintf (value, sizeof (value),
              "%d",
              gui_color_convert_term_to_rgb (atoi (arguments)));
    return strdup (value);
}

/*
 * Returns WeeChat info "color_rgb2term".
 */

char *
plugin_api_info_color_rgb2term_cb (const void *pointer, void *data,
                                   const char *info_name,
                                   const char *arguments)
{
    char value[32], *pos, *color;
    int rgb, limit;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    if (!arguments || !arguments[0])
        return NULL;

    limit = 256;
    pos = strchr (arguments, ',');
    if (pos)
    {
        color = string_strndup (arguments, pos - arguments);
        if (!color)
            return NULL;
        rgb = atoi (color);
        limit = atoi (pos + 1);
        free (color);
    }
    else
    {
        rgb = atoi (arguments);
    }

    snprintf (value, sizeof (value),
              "%d",
              gui_color_convert_rgb_to_term (rgb, limit));

    return strdup (value);
}

/*
 * Returns nick color code for a nickname.
 */

char *
plugin_api_info_nick_color_cb (const void *pointer, void *data,
                               const char *info_name,
                               const char *arguments)
{
    char **items, *result;
    int num_items;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    items = string_split (arguments, ";", NULL, 0, 2, &num_items);

    result = gui_nick_find_color (
        (num_items >= 1) ? items[0] : NULL,
        (num_items >= 2) ? items[1] : NULL);

    if (items)
        string_free_split (items);

    return result;
}

/*
 * Returns nick color name for a nickname.
 */

char *
plugin_api_info_nick_color_name_cb (const void *pointer, void *data,
                                    const char *info_name,
                                    const char *arguments)
{
    char **items, *result;
    int num_items;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    items = string_split (arguments, ";", NULL, 0, 2, &num_items);

    result = gui_nick_find_color_name (
        (num_items >= 1) ? items[0] : NULL,
        (num_items >= 2) ? items[1] : NULL);

    if (items)
        string_free_split (items);

    return result;
}

/*
 * Returns uptime according to the start date and arguments.
 */

char *
plugin_api_info_build_uptime (time_t start_time, const char *arguments)
{
    char value[128];
    time_t total_seconds;
    int days, hours, minutes, seconds;

    if (!arguments || !arguments[0])
    {
        /* return uptime with format: "days:hh:mm:ss" */
        util_get_time_diff (start_time, time (NULL),
                            NULL, &days, &hours, &minutes, &seconds);
        snprintf (value, sizeof (value), "%d:%02d:%02d:%02d",
                  days, hours, minutes, seconds);
        return strdup (value);
    }

    if (strcmp (arguments, "days") == 0)
    {
        /* return the number of days */
        util_get_time_diff (start_time, time (NULL),
                            NULL, &days, NULL, NULL, NULL);
        snprintf (value, sizeof (value), "%d", days);
        return strdup (value);
    }

    if (strcmp (arguments, "seconds") == 0)
    {
        /* return the number of seconds */
        util_get_time_diff (start_time, time (NULL),
                            &total_seconds, NULL, NULL, NULL, NULL);
        snprintf (value, sizeof (value), "%lld", (long long)total_seconds);
        return strdup (value);
    }

    return NULL;
}

/*
 * Returns WeeChat info "uptime".
 */

char *
plugin_api_info_uptime_cb (const void *pointer, void *data,
                           const char *info_name,
                           const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    return plugin_api_info_build_uptime (weechat_first_start_time, arguments);
}

/*
 * Returns WeeChat info "uptime_current" (current run: from last start,
 * upgrades are ignored).
 */

char *
plugin_api_info_uptime_current_cb (const void *pointer, void *data,
                                   const char *info_name,
                                   const char *arguments)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    return plugin_api_info_build_uptime (
        (time_t)weechat_current_start_timeval.tv_sec,
        arguments);
}

/*
 * Returns WeeChat info "totp_generate": generates a Time-based One-Time
 * Password (TOTP).
 *
 * Arguments: "secret,timestamp,digits" (timestamp and digits are optional).
 */

char *
plugin_api_info_totp_generate_cb (const void *pointer, void *data,
                                  const char *info_name,
                                  const char *arguments)
{
    char **argv, *ptr_secret, *error, *totp;
    int argc, digits;
    long number;
    time_t totp_time;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    argv = NULL;
    totp = NULL;

    if (!arguments || !arguments[0])
        goto error;

    argv = string_split (arguments, ",", NULL,
                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                         0, &argc);
    if (!argv || (argc < 1))
        goto error;

    ptr_secret = argv[0];
    totp_time = 0;
    digits = 6;

    if (argc > 1)
    {
        error = NULL;
        number = (int)strtol (argv[1], &error, 10);
        if (!error || error[0] || (number < 0))
            goto error;
        totp_time = (time_t)number;
    }
    if (argc > 2)
    {
        error = NULL;
        number = (int)strtol (argv[2], &error, 10);
        if (!error || error[0] || (number < 0))
            goto error;
        digits = number;
    }

    totp = weecrypto_totp_generate (ptr_secret, totp_time, digits);
    if (!totp)
        goto error;

    string_free_split (argv);

    return totp;

error:
    if (argv)
        string_free_split (argv);
    if (totp)
        free (totp);
    return NULL;
}

/*
 * Returns WeeChat info "totp_validate": validates a Time-based One-Time
 * Password (TOTP).
 *
 * Arguments: "secret,otp,timestamp,window" (timestamp and window are optional).
 */

char *
plugin_api_info_totp_validate_cb (const void *pointer, void *data,
                                  const char *info_name,
                                  const char *arguments)
{
    char value[16], **argv, *ptr_secret, *ptr_otp, *error;
    int argc, window, rc;
    long number;
    time_t totp_time;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;

    argv = NULL;

    if (!arguments || !arguments[0])
        goto error;

    argv = string_split (arguments, ",", NULL,
                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                         0, &argc);
    if (!argv || (argc < 2))
        goto error;

    ptr_secret = argv[0];
    ptr_otp = argv[1];
    totp_time = 0;
    window = 0;

    if (argc > 2)
    {
        error = NULL;
        number = (int)strtol (argv[2], &error, 10);
        if (!error || error[0] || (number < 0))
            goto error;
        totp_time = (time_t)number;
    }
    if (argc > 3)
    {
        error = NULL;
        number = (int)strtol (argv[3], &error, 10);
        if (!error || error[0] || (number < 0))
            goto error;
        window = number;
    }

    rc = weecrypto_totp_validate (ptr_secret, totp_time, window, ptr_otp);

    snprintf (value, sizeof (value), "%d", rc);

    string_free_split (argv);

    return strdup (value);

error:
    if (argv)
        string_free_split (argv);
    return NULL;
}

/*
 * Returns secured data hashtable.
 */

struct t_hashtable *
plugin_api_info_hashtable_secured_data_cb (const void *pointer, void *data,
                                           const char *info_name,
                                           struct t_hashtable *hashtable)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) info_name;
    (void) hashtable;

    return hashtable_dup (secure_hashtable_data);
}

/*
 * Returns WeeChat infolist "bar".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_bar_cb (const void *pointer, void *data,
                            const char *infolist_name,
                            void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    /* invalid bar pointer ? */
    if (obj_pointer && (!gui_bar_valid (obj_pointer)))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one bar */
        if (!gui_bar_add_to_infolist (ptr_infolist, obj_pointer))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all bars matching arguments */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if (!arguments || !arguments[0]
                || string_match (ptr_bar->name, arguments, 1))
            {
                if (!gui_bar_add_to_infolist (ptr_infolist, ptr_bar))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns WeeChat infolist "bar_item".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_bar_item_cb (const void *pointer, void *data,
                                 const char *infolist_name,
                                 void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_bar_item *ptr_bar_item;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    /* invalid bar item pointer ? */
    if (obj_pointer && (!gui_bar_item_valid (obj_pointer)))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one bar item */
        if (!gui_bar_item_add_to_infolist (ptr_infolist, obj_pointer))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all bar items matching arguments */
        for (ptr_bar_item = gui_bar_items; ptr_bar_item;
             ptr_bar_item = ptr_bar_item->next_item)
        {
            if (!arguments || !arguments[0]
                || string_match (ptr_bar_item->name, arguments, 1))
            {
                if (!gui_bar_item_add_to_infolist (ptr_infolist, ptr_bar_item))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns WeeChat infolist "bar_window".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_bar_window_cb (const void *pointer, void *data,
                                   const char *infolist_name,
                                   void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_window *ptr_bar_window;
    struct t_gui_window *ptr_window;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) arguments;

    /* invalid bar window pointer ? */
    if (obj_pointer && (!gui_bar_window_valid (obj_pointer)))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one bar window */
        if (!gui_bar_window_add_to_infolist (ptr_infolist, obj_pointer))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all bar windows (from root and window bars) */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if (ptr_bar->bar_window)
            {
                if (!gui_bar_window_add_to_infolist (ptr_infolist, ptr_bar->bar_window))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            for (ptr_bar_window = ptr_window->bar_windows;
                 ptr_bar_window;
                 ptr_bar_window = ptr_bar_window->next_bar_window)
            {
                if (!gui_bar_window_add_to_infolist (ptr_infolist, ptr_bar_window))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns WeeChat infolist "buffer".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_buffer_cb (const void *pointer, void *data,
                               const char *infolist_name,
                               void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_buffer *ptr_buffer;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    /* invalid buffer pointer ? */
    if (obj_pointer && (!gui_buffer_valid (obj_pointer)))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one buffer */
        if (!gui_buffer_add_to_infolist (ptr_infolist, obj_pointer))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all buffers matching arguments */
        for (ptr_buffer = gui_buffers; ptr_buffer;
             ptr_buffer = ptr_buffer->next_buffer)
        {
            if (!arguments || !arguments[0]
                || string_match (ptr_buffer->full_name, arguments, 1))
            {
                if (!gui_buffer_add_to_infolist (ptr_infolist, ptr_buffer))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns WeeChat infolist "buffer_lines".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_buffer_lines_cb (const void *pointer, void *data,
                                     const char *infolist_name,
                                     void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_line *ptr_line;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) arguments;

    if (!obj_pointer)
        obj_pointer = gui_buffers;
    else
    {
        /* invalid buffer pointer ? */
        if (!gui_buffer_valid (obj_pointer))
            return NULL;
    }

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    for (ptr_line = ((struct t_gui_buffer *)obj_pointer)->own_lines->first_line;
         ptr_line; ptr_line = ptr_line->next_line)
    {
        if (!gui_line_add_to_infolist (ptr_infolist,
                                       ((struct t_gui_buffer *)obj_pointer)->own_lines,
                                       ptr_line))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "filter".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_filter_cb (const void *pointer, void *data,
                               const char *infolist_name,
                               void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_filter *ptr_filter;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) obj_pointer;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    for (ptr_filter = gui_filters; ptr_filter;
         ptr_filter = ptr_filter->next_filter)
    {
        if (!arguments || !arguments[0]
            || string_match (ptr_filter->name, arguments, 1))
        {
            if (!gui_filter_add_to_infolist (ptr_infolist, ptr_filter))
            {
                infolist_free (ptr_infolist);
                return NULL;
            }
        }
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "history".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_history_cb (const void *pointer, void *data,
                                const char *infolist_name,
                                void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_history *ptr_history;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) arguments;

    /* invalid buffer pointer ? */
    if (obj_pointer && (!gui_buffer_valid (obj_pointer)))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    for (ptr_history = (obj_pointer) ?
             ((struct t_gui_buffer *)obj_pointer)->history : gui_history;
         ptr_history; ptr_history = ptr_history->next_history)
    {
        if (!gui_history_add_to_infolist (ptr_infolist, ptr_history))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "hook".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_hook_cb (const void *pointer, void *data,
                             const char *infolist_name,
                             void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    /* invalid hook pointer ? */
    if (obj_pointer && !hook_valid (obj_pointer))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (!hook_add_to_infolist (ptr_infolist, obj_pointer, arguments))
    {
        infolist_free (ptr_infolist);
        return NULL;
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "hotlist".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_hotlist_cb (const void *pointer, void *data,
                                const char *infolist_name,
                                void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_hotlist *ptr_hotlist;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) obj_pointer;
    (void) arguments;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    for (ptr_hotlist = gui_hotlist; ptr_hotlist;
         ptr_hotlist = ptr_hotlist->next_hotlist)
    {
        if (!gui_hotlist_add_to_infolist (ptr_infolist, ptr_hotlist))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "key".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_key_cb (const void *pointer, void *data,
                            const char *infolist_name,
                            void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_key *ptr_key;
    int context;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) obj_pointer;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (arguments && arguments[0])
        context = gui_key_search_context (arguments);
    else
        context = GUI_KEY_CONTEXT_DEFAULT;
    if (context >= 0)
    {
        for (ptr_key = gui_keys[context]; ptr_key;
             ptr_key = ptr_key->next_key)
        {
            if (!gui_key_add_to_infolist (ptr_infolist, ptr_key))
            {
                infolist_free (ptr_infolist);
                return NULL;
            }
        }
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "layout".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_layout_cb (const void *pointer, void *data,
                               const char *infolist_name,
                               void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_layout *ptr_layout;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) obj_pointer;
    (void) arguments;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    for (ptr_layout = gui_layouts; ptr_layout;
         ptr_layout = ptr_layout->next_layout)
    {
        if (!gui_layout_add_to_infolist (ptr_infolist,ptr_layout))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "nicklist".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_nicklist_cb (const void *pointer, void *data,
                                 const char *infolist_name,
                                 void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    /* invalid buffer pointer ? */
    if (!obj_pointer || (!gui_buffer_valid (obj_pointer)))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (!gui_nicklist_add_to_infolist (ptr_infolist, obj_pointer, arguments))
    {
        infolist_free (ptr_infolist);
        return NULL;
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "option".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_option_cb (const void *pointer, void *data,
                               const char *infolist_name,
                               void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) obj_pointer;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (!config_file_add_to_infolist (ptr_infolist, arguments))
    {
        infolist_free (ptr_infolist);
        return NULL;
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "plugin".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_plugin_cb (const void *pointer, void *data,
                               const char *infolist_name,
                               void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_weechat_plugin *ptr_plugin;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    /* invalid plugin pointer ? */
    if (obj_pointer && (!plugin_valid (obj_pointer)))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one plugin */
        if (!plugin_add_to_infolist (ptr_infolist, obj_pointer))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all plugins matching arguments */
        for (ptr_plugin = weechat_plugins; ptr_plugin;
             ptr_plugin = ptr_plugin->next_plugin)
        {
            if (!arguments || !arguments[0]
                || string_match (ptr_plugin->name, arguments, 1))
            {
                if (!plugin_add_to_infolist (ptr_infolist, ptr_plugin))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns WeeChat infolist "proxy".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_proxy_cb (const void *pointer, void *data,
                              const char *infolist_name,
                              void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_proxy *ptr_proxy;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    /* invalid proxy pointer ? */
    if (obj_pointer && (!proxy_valid (obj_pointer)))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one proxy */
        if (!proxy_add_to_infolist (ptr_infolist, obj_pointer))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        /* build list with all proxies matching arguments */
        for (ptr_proxy = weechat_proxies; ptr_proxy;
             ptr_proxy = ptr_proxy->next_proxy)
        {
            if (!arguments || !arguments[0]
                || string_match (ptr_proxy->name, arguments, 1))
            {
                if (!proxy_add_to_infolist (ptr_infolist, ptr_proxy))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
        }
        return ptr_infolist;
    }

    return NULL;
}

/*
 * Returns WeeChat infolist "url_options".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_url_options_cb (const void *pointer, void *data,
                                    const char *infolist_name,
                                    void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;
    (void) obj_pointer;
    (void) arguments;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    for (i = 0; url_options[i].name; i++)
    {
        if (!weeurl_option_add_to_infolist (ptr_infolist, &url_options[i]))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
    }
    return ptr_infolist;
}

/*
 * Returns WeeChat infolist "window".
 *
 * Note: result must be freed after use with function weechat_infolist_free().
 */

struct t_infolist *
plugin_api_infolist_window_cb (const void *pointer, void *data,
                               const char *infolist_name,
                               void *obj_pointer, const char *arguments)
{
    struct t_infolist *ptr_infolist;
    struct t_gui_window *ptr_window;
    int number;
    char *error;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) infolist_name;

    /* invalid window pointer ? */
    if (obj_pointer && (!gui_window_valid (obj_pointer)))
        return NULL;

    ptr_infolist = infolist_new (NULL);
    if (!ptr_infolist)
        return NULL;

    if (obj_pointer)
    {
        /* build list with only one window */
        if (!gui_window_add_to_infolist (ptr_infolist, obj_pointer))
        {
            infolist_free (ptr_infolist);
            return NULL;
        }
        return ptr_infolist;
    }
    else
    {
        if (arguments && arguments[0])
        {
            if ((string_strcasecmp (arguments, "current") == 0))
            {
                if (gui_current_window)
                {
                    if (!gui_window_add_to_infolist (ptr_infolist,
                                                     gui_current_window))
                    {
                        infolist_free (ptr_infolist);
                        return NULL;
                    }
                    return ptr_infolist;
                }
                return NULL;
            }
            /* check if argument is a window number */
            error = NULL;
            number = (int)strtol (arguments, &error, 10);
            if (error && !error[0])
            {
                ptr_window = gui_window_search_by_number (number);
                if (ptr_window)
                {
                    if (!gui_window_add_to_infolist (ptr_infolist,
                                                     ptr_window))
                    {
                        infolist_free (ptr_infolist);
                        return NULL;
                    }
                    return ptr_infolist;
                }
            }
            return NULL;
        }
        else
        {
            /* build list with all windows */
            for (ptr_window = gui_windows; ptr_window;
                 ptr_window = ptr_window->next_window)
            {
                if (!gui_window_add_to_infolist (ptr_infolist,
                                                 ptr_window))
                {
                    infolist_free (ptr_infolist);
                    return NULL;
                }
            }
            return ptr_infolist;
        }
    }

    return NULL;
}

/*
 * Initializes info/infolist plugin API.
 */

void
plugin_api_info_init ()
{
    /* WeeChat core info hooks */
    hook_info (NULL, "version",
               N_("WeeChat version"),
               NULL, &plugin_api_info_version_cb, NULL, NULL);
    hook_info (NULL, "version_number",
               N_("WeeChat version (as number)"),
               N_("version (optional, by default the version of the running "
                  "WeeChat is returned)"),
               &plugin_api_info_version_number_cb, NULL, NULL);
    hook_info (NULL, "version_git",
               N_("WeeChat git version (output of command \"git describe\" "
                  "for a development version only, empty for a stable "
                  "release)"),
               NULL, &plugin_api_info_version_git_cb, NULL, NULL);
    hook_info (NULL, "date",
               N_("WeeChat compilation date/time"),
               NULL, &plugin_api_info_date_cb, NULL, NULL);
    hook_info (NULL, "pid",
               N_("WeeChat PID (process ID)"),
               NULL, &plugin_api_info_pid_cb, NULL, NULL);
    hook_info (NULL, "dir_separator",
               N_("directory separator"),
               NULL, &plugin_api_info_dir_separator_cb, NULL, NULL);
    hook_info (NULL, "weechat_dir",
               N_("WeeChat directory "
                  "(*deprecated* since version 3.2, replaced by "
                  "\"weechat_config_dir\", \"weechat_data_dir\", "
                  "\"weechat_cache_dir\" and \"weechat_runtime_dir\")"),
               NULL, &plugin_api_info_weechat_data_dir_cb, NULL, NULL);
    hook_info (NULL, "weechat_config_dir",
               N_("WeeChat config directory"),
               NULL, &plugin_api_info_weechat_config_dir_cb, NULL, NULL);
    hook_info (NULL, "weechat_data_dir",
               N_("WeeChat data directory"),
               NULL, &plugin_api_info_weechat_data_dir_cb, NULL, NULL);
    hook_info (NULL, "weechat_cache_dir",
               N_("WeeChat cache directory"),
               NULL, &plugin_api_info_weechat_cache_dir_cb, NULL, NULL);
    hook_info (NULL, "weechat_runtime_dir",
               N_("WeeChat runtime directory"),
               NULL, &plugin_api_info_weechat_runtime_dir_cb, NULL, NULL);
    hook_info (NULL, "weechat_libdir",
               N_("WeeChat \"lib\" directory"),
               NULL, &plugin_api_info_weechat_libdir_cb, NULL, NULL);
    hook_info (NULL, "weechat_sharedir",
               N_("WeeChat \"share\" directory"),
               NULL, &plugin_api_info_weechat_sharedir_cb, NULL, NULL);
    hook_info (NULL, "weechat_localedir",
               N_("WeeChat \"locale\" directory"),
               NULL, &plugin_api_info_weechat_localedir_cb, NULL, NULL);
    hook_info (NULL, "weechat_site",
               N_("WeeChat site"),
               NULL, &plugin_api_info_weechat_site_cb, NULL, NULL);
    hook_info (NULL, "weechat_site_download",
               N_("WeeChat site, download page"),
               NULL, &plugin_api_info_weechat_site_download_cb, NULL, NULL);
    hook_info (NULL, "weechat_upgrading",
               N_("1 if WeeChat is upgrading (command `/upgrade`)"),
               NULL, &plugin_api_info_weechat_upgrading_cb, NULL, NULL);
    hook_info (NULL, "weechat_headless",
               N_("1 if WeeChat is running headless"),
               NULL, &plugin_api_info_weechat_headless_cb, NULL, NULL);
    hook_info (NULL, "weechat_daemon",
               N_("1 if WeeChat is running in daemon mode "
                  "(headless, in background)"),
               NULL, &plugin_api_info_weechat_daemon_cb, NULL, NULL);
    hook_info (NULL, "auto_connect",
               N_("1 if automatic connection to servers is enabled, "
                  "0 if it has been disabled by the user "
                  "(option \"-a\" or \"--no-connect\")"),
               NULL, &plugin_api_info_auto_connect_cb, NULL, NULL);
    hook_info (NULL, "auto_load_scripts",
               N_("1 if scripts are automatically loaded, "
                  "0 if the auto-load has been disabled by the user "
                  "(option \"-s\" or \"--no-script\")"),
               NULL, &plugin_api_info_auto_load_scripts_cb, NULL, NULL);
    hook_info (NULL, "charset_terminal",
               N_("terminal charset"),
               NULL, &plugin_api_info_charset_terminal_cb, NULL, NULL);
    hook_info (NULL, "charset_internal",
               N_("WeeChat internal charset"),
               NULL, &plugin_api_info_charset_internal_cb, NULL, NULL);
    hook_info (NULL, "locale",
               N_("locale used for translating messages"),
               NULL, &plugin_api_info_locale_cb, NULL, NULL);
    hook_info (NULL, "inactivity",
               N_("keyboard inactivity (seconds)"),
               NULL, &plugin_api_info_inactivity_cb, NULL, NULL);
    hook_info (NULL, "filters_enabled",
               N_("1 if filters are enabled"),
               NULL, &plugin_api_info_filters_enabled_cb, NULL, NULL);
    hook_info (NULL, "cursor_mode",
               N_("1 if cursor mode is enabled"),
               NULL, &plugin_api_info_cursor_mode_cb, NULL, NULL);
    hook_info (NULL, "term_width",
               N_("width of terminal"),
               NULL, &plugin_api_info_term_width_cb, NULL, NULL);
    hook_info (NULL, "term_height",
               N_("height of terminal"),
               NULL, &plugin_api_info_term_height_cb, NULL, NULL);
    hook_info (NULL, "term_colors",
               N_("number of colors supported in terminal"),
               NULL, &plugin_api_info_term_colors_cb, NULL, NULL);
    hook_info (NULL, "term_color_pairs",
               N_("number of color pairs supported in terminal"),
               NULL, &plugin_api_info_term_color_pairs_cb, NULL, NULL);
    hook_info (NULL, "color_ansi_regex",
               N_("POSIX extended regular expression to search ANSI escape "
                  "codes"),
               NULL, &plugin_api_info_color_ansi_regex_cb, NULL, NULL);
    hook_info (NULL, "color_term2rgb",
               N_("terminal color (0-255) converted to RGB color"),
               N_("color (terminal color: 0-255)"),
               &plugin_api_info_color_term2rgb_cb, NULL, NULL);
    hook_info (NULL, "color_rgb2term",
               N_("RGB color converted to terminal color (0-255)"),
               N_("rgb,limit (limit is optional and is set to 256 by default)"),
               &plugin_api_info_color_rgb2term_cb, NULL, NULL);
    hook_info (NULL, "nick_color",
               N_("get nick color code"),
               N_("nickname;colors (colors is an optional comma-separated "
                  "list of colors to use; background is allowed for a color "
                  "with format text:background; if colors is present, WeeChat "
                  "options with nick colors and forced nick colors are "
                  "ignored)"),
               &plugin_api_info_nick_color_cb, NULL, NULL);
    hook_info (NULL, "nick_color_name",
               N_("get nick color name"),
               N_("nickname;colors (colors is an optional comma-separated "
                  "list of colors to use; background is allowed for a color "
                  "with format text:background; if colors is present, WeeChat "
                  "options with nick colors and forced nick colors are "
                  "ignored)"),
               &plugin_api_info_nick_color_name_cb, NULL, NULL);
    hook_info (NULL, "uptime",
               N_("WeeChat uptime (format: \"days:hh:mm:ss\")"),
               N_("\"days\" (number of days) or \"seconds\" (number of "
                  "seconds) (optional)"),
               &plugin_api_info_uptime_cb, NULL, NULL);
    hook_info (NULL, "uptime_current",
               N_("WeeChat uptime for the current process only (upgrades with "
                  "/upgrade command are ignored) (format: \"days:hh:mm:ss\")"),
               N_("\"days\" (number of days) or \"seconds\" (number of "
                  "seconds) (optional)"),
               &plugin_api_info_uptime_current_cb, NULL, NULL);
    hook_info (NULL, "totp_generate",
               N_("generate a Time-based One-Time Password (TOTP)"),
               N_("secret (in base32), timestamp (optional, current time by "
                  "default), number of digits (optional, between 4 and 10, "
                  "6 by default)"),
               &plugin_api_info_totp_generate_cb, NULL, NULL);
    hook_info (NULL, "totp_validate",
               N_("validate a Time-based One-Time Password (TOTP): 1 if TOTP "
                  "is correct, otherwise 0"),
               N_("secret (in base32), one-time password, "
                  "timestamp (optional, current time by default), number of "
                  "passwords before/after to test (optional, 0 by default)"),
               &plugin_api_info_totp_validate_cb, NULL, NULL);

    /* WeeChat core info_hashtable hooks */
    hook_info_hashtable (NULL,
        "focus_info",
        N_("get focus info"),
        /* TRANSLATORS: please do not translate key names (enclosed by quotes) */
        N_("\"x\": x coordinate (string with integer >= 0), "
           "\"y\": y coordinate (string with integer >= 0)"),
        N_("see function \"hook_focus\" in Plugin API reference"),
        &gui_focus_info_hashtable_gui_focus_info_cb, NULL, NULL);
    /* info (hashtable) with the secured data */
    hook_info_hashtable (
        NULL,
        "secured_data",
        N_("secured data"),
        NULL,
        N_("secured data: names and values (be careful: the values are "
           "sensitive data: do NOT print/log them anywhere)"),
        &plugin_api_info_hashtable_secured_data_cb, NULL, NULL);

    /* WeeChat core infolist hooks */
    hook_infolist (NULL, "bar",
                   N_("list of bars"),
                   N_("bar pointer (optional)"),
                   N_("bar name (wildcard \"*\" is allowed) (optional)"),
                   &plugin_api_infolist_bar_cb, NULL, NULL);
    hook_infolist (NULL, "bar_item",
                   N_("list of bar items"),
                   N_("bar item pointer (optional)"),
                   N_("bar item name (wildcard \"*\" is allowed) (optional)"),
                   &plugin_api_infolist_bar_item_cb, NULL, NULL);
    hook_infolist (NULL, "bar_window",
                   N_("list of bar windows"),
                   N_("bar window pointer (optional)"),
                   NULL,
                   &plugin_api_infolist_bar_window_cb, NULL, NULL);
    hook_infolist (NULL, "buffer",
                   N_("list of buffers"),
                   N_("buffer pointer (optional)"),
                   N_("buffer name (wildcard \"*\" is allowed) (optional)"),
                   &plugin_api_infolist_buffer_cb, NULL, NULL);
    hook_infolist (NULL, "buffer_lines",
                   N_("lines of a buffer"),
                   N_("buffer pointer"),
                   NULL,
                   &plugin_api_infolist_buffer_lines_cb, NULL, NULL);
    hook_infolist (NULL, "filter",
                   N_("list of filters"),
                   NULL,
                   N_("filter name (wildcard \"*\" is allowed) (optional)"),
                   &plugin_api_infolist_filter_cb, NULL, NULL);
    hook_infolist (NULL, "history",
                   N_("history of commands"),
                   N_("buffer pointer (if not set, return global history) (optional)"),
                   NULL,
                   &plugin_api_infolist_history_cb, NULL, NULL);
    hook_infolist (NULL, "hook",
                   N_("list of hooks"),
                   N_("hook pointer (optional)"),
                   N_("type,arguments (type is command/timer/.., arguments to "
                      "get only some hooks (wildcard \"*\" is allowed), "
                      "both are optional)"),
                   &plugin_api_infolist_hook_cb, NULL, NULL);
    hook_infolist (NULL, "hotlist",
                   N_("list of buffers in hotlist"),
                   NULL,
                   NULL,
                   &plugin_api_infolist_hotlist_cb, NULL, NULL);
    hook_infolist (NULL, "key",
                   N_("list of key bindings"),
                   NULL,
                   N_("context (\"default\", \"search\", \"cursor\" or "
                      "\"mouse\") (optional)"),
                   &plugin_api_infolist_key_cb, NULL, NULL);
    hook_infolist (NULL, "layout",
                   N_("list of layouts"),
                   NULL,
                   NULL,
                   &plugin_api_infolist_layout_cb, NULL, NULL);
    hook_infolist (NULL, "nicklist",
                   N_("nicks in nicklist for a buffer"),
                   N_("buffer pointer"),
                   N_("nick_xxx or group_xxx to get only nick/group xxx "
                      "(optional)"),
                   &plugin_api_infolist_nicklist_cb, NULL, NULL);
    hook_infolist (NULL, "option",
                   N_("list of options"),
                   NULL,
                   N_("option name (wildcard \"*\" is allowed) (optional)"),
                   &plugin_api_infolist_option_cb, NULL, NULL);
    hook_infolist (NULL, "plugin",
                   N_("list of plugins"),
                   N_("plugin pointer (optional)"),
                   N_("plugin name (wildcard \"*\" is allowed) (optional)"),
                   &plugin_api_infolist_plugin_cb, NULL, NULL);
    hook_infolist (NULL, "proxy",
                   N_("list of proxies"),
                   N_("proxy pointer (optional)"),
                   N_("proxy name (wildcard \"*\" is allowed) (optional)"),
                   &plugin_api_infolist_proxy_cb, NULL, NULL);
    hook_infolist (NULL, "url_options",
                   N_("options for URL"),
                   NULL,
                   NULL,
                   &plugin_api_infolist_url_options_cb, NULL, NULL);
    hook_infolist (NULL, "window",
                   N_("list of windows"),
                   N_("window pointer (optional)"),
                   N_("\"current\" for current window or a window number (optional)"),
                   &plugin_api_infolist_window_cb, NULL, NULL);
}
