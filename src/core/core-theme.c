/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* Themes: named bundles of option overrides applied via /theme command */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "weechat.h"
#include "core-arraylist.h"
#include "core-config.h"
#include "core-config-file.h"
#include "core-dir.h"
#include "core-hashtable.h"
#include "core-hook.h"
#include "core-string.h"
#include "core-theme.h"
#include "core-version.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-color.h"
#include "../gui/gui-window.h"
#include "../plugins/weechat-plugin.h"


struct t_theme *themes = NULL;
struct t_theme *last_theme = NULL;
int theme_applying = 0;


/*
 * Search for a theme by name in the in-memory registry.
 *
 * Return pointer to theme found, NULL if not found.
 */

struct t_theme *
theme_search (const char *name)
{
    struct t_theme *ptr_theme;

    if (!name)
        return NULL;

    for (ptr_theme = themes; ptr_theme; ptr_theme = ptr_theme->next_theme)
    {
        if (strcmp (ptr_theme->name, name) == 0)
            return ptr_theme;
    }
    return NULL;
}

/*
 * Build a "YYYY-MM-DD HH:MM:SS" timestamp string for "now" (local time).
 *
 * Note: result must be freed after use.
 */

char *
theme_format_now (void)
{
    time_t time_now;
    struct tm *local_time;
    char buf[32];

    time_now = time (NULL);
    local_time = localtime (&time_now);
    if (!local_time)
        return strdup ("");
    if (strftime (buf, sizeof (buf), "%Y-%m-%d %H:%M:%S", local_time) == 0)
        return strdup ("");
    return strdup (buf);
}

/*
 * Allocate a new theme with name and empty metadata; do not link it into
 * the registry.
 *
 * Return the new theme, NULL on error.
 */

struct t_theme *
theme_alloc (const char *name)
{
    struct t_theme *new_theme;

    new_theme = calloc (1, sizeof (*new_theme));
    if (!new_theme)
        return NULL;
    new_theme->name = strdup (name);
    new_theme->description = strdup ("");
    new_theme->date = theme_format_now ();
    new_theme->weechat_version = strdup (version_get_version ());
    new_theme->overrides = hashtable_new (32,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL, NULL);
    if (!new_theme->name || !new_theme->description
        || !new_theme->date || !new_theme->weechat_version
        || !new_theme->overrides)
    {
        free (new_theme->name);
        free (new_theme->description);
        free (new_theme->date);
        free (new_theme->weechat_version);
        hashtable_free (new_theme->overrides);
        free (new_theme);
        return NULL;
    }
    return new_theme;
}

/*
 * Free a theme (do not unlink from registry; caller handles that).
 */

void
theme_free (struct t_theme *theme)
{
    if (!theme)
        return;
    free (theme->name);
    free (theme->description);
    free (theme->date);
    free (theme->weechat_version);
    hashtable_free (theme->overrides);
    free (theme);
}

/*
 * Merge entries from src into dst (overwrite duplicate keys).
 */

void
theme_merge_overrides_cb (void *data,
                          struct t_hashtable *hashtable,
                          const void *key,
                          const void *value)
{
    struct t_hashtable *dst = (struct t_hashtable *)data;

    /* make C compiler happy */
    (void) hashtable;

    hashtable_set (dst, (const char *)key, (const char *)value);
}

/*
 * Register a theme by name with a set of option overrides.
 *
 * If a theme with the given name already exists, the provided overrides
 * are merged into the existing theme's hashtable (later registrations
 * override earlier ones for duplicate keys). This lets plugins/scripts
 * register their per-theme contributions without coordinating with core.
 *
 * The "overrides" hashtable passed in is read-only from this function's
 * perspective; the caller retains ownership and may free it.
 *
 * Return pointer to theme (existing or newly created), NULL on error.
 */

struct t_theme *
theme_register (const char *name, struct t_hashtable *overrides)
{
    struct t_theme *theme;

    if (!name || !name[0])
        return NULL;

    theme = theme_search (name);
    if (!theme)
    {
        theme = theme_alloc (name);
        if (!theme)
            return NULL;
        theme->prev_theme = last_theme;
        theme->next_theme = NULL;
        if (last_theme)
            last_theme->next_theme = theme;
        else
            themes = theme;
        last_theme = theme;
    }

    if (overrides)
    {
        hashtable_map (overrides,
                       &theme_merge_overrides_cb,
                       theme->overrides);
    }

    return theme;
}

/*
 * Compare two themes by name (callback used by arraylist sort).
 *
 * Return negative, zero, or positive value (like strcmp).
 */

int
theme_list_cmp_cb (void *data, struct t_arraylist *arraylist,
                   void *pointer1, void *pointer2)
{
    /* make C compiler happy */
    (void) data;
    (void) arraylist;

    return strcmp (((struct t_theme *)pointer1)->name,
                   ((struct t_theme *)pointer2)->name);
}

/*
 * Return an arraylist of t_theme * for all registered themes (built-ins).
 *
 * The returned arraylist owns no data; callers must not free its items.
 * Return NULL on allocation failure.
 */

struct t_arraylist *
theme_list (void)
{
    struct t_arraylist *list;
    struct t_theme *ptr_theme;

    list = arraylist_new (8, 1, 0, &theme_list_cmp_cb, NULL, NULL, NULL);
    if (!list)
        return NULL;

    for (ptr_theme = themes; ptr_theme; ptr_theme = ptr_theme->next_theme)
        arraylist_add (list, ptr_theme);

    return list;
}

/*
 * Build the on-disk path for a user theme:
 * "<weechat_config_dir>/themes/<name>.theme".
 *
 * Return NULL on error.
 *
 * Note: result must be freed after use.
 */

char *
theme_user_file_path (const char *name)
{
    char *path = NULL;

    if (!name || !name[0])
        return NULL;
    string_asprintf (&path, "%s/themes/%s.theme",
                     weechat_config_dir, name);
    return path;
}

/*
 * Build a unique backup theme name "backup-YYYYMMDD-HHMMSS-uuuuuu".
 *
 * Return NULL on error.
 *
 * Note: result must be freed after use.
 */

char *
theme_make_backup_name (void)
{
    struct timeval tv;
    struct tm *local_time;
    char buf[128];

    if (gettimeofday (&tv, NULL) != 0)
        return NULL;
    local_time = localtime (&tv.tv_sec);
    if (!local_time)
        return NULL;
    snprintf (buf, sizeof (buf),
              "backup-%04d%02d%02d-%02d%02d%02d-%06ld",
              local_time->tm_year + 1900,
              local_time->tm_mon + 1,
              local_time->tm_mday,
              local_time->tm_hour,
              local_time->tm_min,
              local_time->tm_sec,
              (long)tv.tv_usec);
    return strdup (buf);
}

/*
 * Write a full snapshot of every themable option to a .theme file at
 * "<weechat_config_dir>/themes/<name>.theme".
 *
 * The themes directory is created if missing. The file contains an
 * [info] section (name, description, date, weechat version) followed by
 * an [options] section listing every themable option's current value.
 *
 * Return:
 *   1: success
 *   0: error
 */

int
theme_write_file_full (const char *name, const char *description)
{
    char *path, *dir, *value, *now;
    FILE *file;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    if (!name || !name[0])
        return 0;

    path = NULL;
    dir = NULL;
    string_asprintf (&dir, "%s/themes", weechat_config_dir);
    if (!dir)
        return 0;
    dir_mkdir (dir, 0755);
    free (dir);

    path = theme_user_file_path (name);
    if (!path)
        return 0;

    file = fopen (path, "w");
    free (path);
    if (!file)
        return 0;

    now = theme_format_now ();
    fprintf (file, "[info]\n");
    fprintf (file, "name = \"%s\"\n", name);
    fprintf (file, "description = \"%s\"\n",
             (description) ? description : "");
    fprintf (file, "date = \"%s\"\n", (now) ? now : "");
    fprintf (file, "weechat = \"%s\"\n", version_get_version ());
    fprintf (file, "\n[options]\n");
    free (now);

    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        for (ptr_section = ptr_config->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if (!ptr_option->themable)
                    continue;
                value = config_file_option_value_to_string (
                    ptr_option, 0, 0, 1);
                fprintf (file, "%s.%s.%s = %s\n",
                         ptr_config->name, ptr_section->name,
                         ptr_option->name,
                         (value) ? value : "\"\"");
                free (value);
            }
        }
    }

    fclose (file);
    return 1;
}

/*
 * Create a timestamped backup theme file with the current themable state.
 *
 * Return the backup name, NULL on failure.
 *
 * Note: result must be freed after use.
 */

char *
theme_make_backup (void)
{
    char *name;

    name = theme_make_backup_name ();
    if (!name)
        return NULL;
    if (!theme_write_file_full (
            name,
            _("Automatic backup written before /theme apply")))
    {
        free (name);
        return NULL;
    }
    return name;
}

/*
 * Apply one override entry (callback for hashtable_map during apply).
 *
 * Refuse entries pointing to options that do not exist or that are not
 * themable, logging a warning to the core buffer; the apply itself still
 * proceeds with the remaining entries.
 */

void
theme_apply_set_option_cb (void *data,
                           struct t_hashtable *hashtable,
                           const void *key,
                           const void *value)
{
    struct t_config_option *option = NULL;

    /* make C compiler happy */
    (void) data;
    (void) hashtable;

    config_file_search_with_string ((const char *)key,
                                    NULL, NULL, &option, NULL);
    if (!option)
    {
        gui_chat_printf (NULL,
                         _("%sTheme: option \"%s\" not found, skipped"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         (const char *)key);
        return;
    }
    if (!option->themable)
    {
        gui_chat_printf (
            NULL,
            _("%sTheme: option \"%s\" is not themable, skipped"),
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
            (const char *)key);
        return;
    }
    config_file_option_set (option, (const char *)value, 1);
}

/*
 * Apply a theme registered in memory.
 *
 * If weechat.look.theme_backup is on (and the target name does not begin
 * with "backup-"), a backup file is written first; on backup failure the
 * apply is aborted before any option is changed.
 *
 * Iterate the theme's overrides with theme_applying=1 so the per-option
 * change callbacks skip their gui refresh; a single refresh is performed
 * at the end.
 *
 * Return:
 *   WEECHAT_RC_OK: success
 *   WEECHAT_RC_ERROR: theme name is unknown or the backup could not be created
 */

int
theme_apply (const char *name)
{
    struct t_theme *theme;
    char *backup_name = NULL;

    if (!name || !name[0])
        return WEECHAT_RC_ERROR;

    theme = theme_search (name);
    if (!theme)
    {
        gui_chat_printf (NULL,
                         _("%sTheme \"%s\" not found"),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
        return WEECHAT_RC_ERROR;
    }

    /* create a backup of current themable state, if enabled */
    if (CONFIG_BOOLEAN(config_look_theme_backup)
        && (strncmp (name, "backup-", 7) != 0))
    {
        backup_name = theme_make_backup ();
        if (!backup_name)
        {
            gui_chat_printf (
                NULL,
                _("%sUnable to create theme backup; aborting apply "
                  "(disable option weechat.look.theme_backup to force)"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_ERROR;
        }
    }

    /* apply each override; per-option refreshes are suppressed via the
       theme_applying flag (see config_change_color) */
    theme_applying = 1;
    hashtable_map (theme->overrides, &theme_apply_set_option_cb, NULL);
    theme_applying = 0;

    /* single refresh at the end */
    if (gui_init_ok)
    {
        gui_color_init_weechat ();
        gui_window_ask_refresh (1);
    }

    /* persist the active theme label */
    config_file_option_set (config_look_theme, name, 1);

    /* tell the user about the backup */
    if (backup_name)
    {
        gui_chat_printf (
            NULL,
            _("Previous state saved as theme \"%s\"; to restore: "
              "/theme apply %s"),
            backup_name, backup_name);
        free (backup_name);
    }

    hook_signal_send ("theme_applied",
                      WEECHAT_HOOK_SIGNAL_STRING, (char *)name);

    return WEECHAT_RC_OK;
}

/*
 * Initialize the theme subsystem.
 *
 * The registry starts empty; built-in themes are registered later (by
 * core and by plugins/scripts at their own init time).
 */

void
theme_init (void)
{
    themes = NULL;
    last_theme = NULL;
    theme_applying = 0;
}

/*
 * Free all registered themes and clear the registry.
 */

void
theme_end (void)
{
    struct t_theme *ptr_theme, *next_theme;

    ptr_theme = themes;
    while (ptr_theme)
    {
        next_theme = ptr_theme->next_theme;
        theme_free (ptr_theme);
        ptr_theme = next_theme;
    }
    themes = NULL;
    last_theme = NULL;
}
