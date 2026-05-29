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
#include <unistd.h>

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
 * Searches for a theme by name in the in-memory registry.
 *
 * Returns pointer to theme found, NULL if not found.
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
 * Builds a "YYYY-MM-DD HH:MM:SS" timestamp string for "now" (local time).
 *
 * Returned string is allocated; caller frees.
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
 * Allocates a new theme with name and empty metadata; does not link it
 * into the registry.
 *
 * Returns the new theme, NULL on error.
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
    if (!new_theme->name || !new_theme->description
        || !new_theme->date || !new_theme->weechat_version)
    {
        free (new_theme->name);
        free (new_theme->description);
        free (new_theme->date);
        free (new_theme->weechat_version);
        free (new_theme);
        return NULL;
    }
    return new_theme;
}

/*
 * Frees one contribution (does not unlink from the parent theme list).
 */

void
theme_contribution_free (struct t_theme_contribution *contribution)
{
    if (!contribution)
        return;
    hashtable_free (contribution->overrides);
    free (contribution);
}

/*
 * Frees a theme (does not unlink from registry; caller handles that).
 */

void
theme_free (struct t_theme *theme)
{
    struct t_theme_contribution *ptr_contribution, *next_contribution;

    if (!theme)
        return;
    ptr_contribution = theme->contributions;
    while (ptr_contribution)
    {
        next_contribution = ptr_contribution->next_contribution;
        theme_contribution_free (ptr_contribution);
        ptr_contribution = next_contribution;
    }
    free (theme->name);
    free (theme->description);
    free (theme->date);
    free (theme->weechat_version);
    free (theme);
}

/*
 * Merges entries from src into dst (overwrites duplicate keys).
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
 * Searches the contribution belonging to the given (plugin, script) pair
 * in a theme's contribution list. Returns NULL if not found.
 */

struct t_theme_contribution *
theme_search_contribution (struct t_theme *theme,
                           struct t_weechat_plugin *plugin,
                           const void *script)
{
    struct t_theme_contribution *ptr;

    if (!theme)
        return NULL;
    for (ptr = theme->contributions; ptr; ptr = ptr->next_contribution)
    {
        if ((ptr->plugin == plugin) && (ptr->script == script))
            return ptr;
    }
    return NULL;
}

/*
 * Allocates a new contribution and appends it to a theme's list.
 *
 * Returns the new contribution, NULL on error.
 */

struct t_theme_contribution *
theme_contribution_new (struct t_theme *theme,
                        struct t_weechat_plugin *plugin,
                        const void *script)
{
    struct t_theme_contribution *new_contribution;

    new_contribution = calloc (1, sizeof (*new_contribution));
    if (!new_contribution)
        return NULL;
    new_contribution->plugin = plugin;
    new_contribution->script = script;
    new_contribution->overrides = hashtable_new (32,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 WEECHAT_HASHTABLE_STRING,
                                                 NULL, NULL);
    if (!new_contribution->overrides)
    {
        free (new_contribution);
        return NULL;
    }
    new_contribution->prev_contribution = theme->last_contribution;
    new_contribution->next_contribution = NULL;
    if (theme->last_contribution)
        theme->last_contribution->next_contribution = new_contribution;
    else
        theme->contributions = new_contribution;
    theme->last_contribution = new_contribution;
    return new_contribution;
}

/*
 * Registers a contribution to a theme.
 *
 * Identity is the (plugin, script) pair:
 *   - plugin == NULL && script == NULL  => core
 *   - plugin != NULL && script == NULL  => plugin-level
 *   - plugin != NULL && script != NULL  => individual script
 *
 * If a contribution for the same (plugin, script) already exists under
 * the named theme, the new overrides are merged into it (later keys
 * win). Otherwise a new contribution is appended. Across distinct
 * contributors, contributions are applied in list order at apply-time
 * (later contributions override earlier ones for duplicate keys).
 *
 * The "overrides" hashtable passed in is read-only here; the caller
 * retains ownership and may free it after the call.
 *
 * Returns pointer to the theme (existing or newly created), NULL on
 * error.
 */

struct t_theme *
theme_register (struct t_weechat_plugin *plugin,
                const void *script,
                const char *name,
                struct t_hashtable *overrides)
{
    struct t_theme *theme;
    struct t_theme_contribution *contribution;

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
        contribution = theme_search_contribution (theme, plugin, script);
        if (!contribution)
        {
            contribution = theme_contribution_new (theme, plugin, script);
            if (!contribution)
                return theme;  /* theme exists but contribution failed */
        }
        hashtable_map (overrides,
                       &theme_merge_overrides_cb,
                       contribution->overrides);
    }

    return theme;
}

/*
 * Drops one contribution from a theme (unlinks and frees it).
 */

void
theme_drop_contribution (struct t_theme *theme,
                         struct t_theme_contribution *contribution)
{
    if (!theme || !contribution)
        return;
    if (contribution->prev_contribution)
    {
        contribution->prev_contribution->next_contribution =
            contribution->next_contribution;
    }
    else
    {
        theme->contributions = contribution->next_contribution;
    }
    if (contribution->next_contribution)
    {
        contribution->next_contribution->prev_contribution =
            contribution->prev_contribution;
    }
    else
    {
        theme->last_contribution = contribution->prev_contribution;
    }
    theme_contribution_free (contribution);
}

/*
 * Drops all contributions owned by a given plugin (across every theme).
 * Called from the plugin-unload lifecycle (next commit).
 *
 * Contributions whose script is non-NULL are kept (they belong to
 * individual scripts and are cleaned up separately on script unload).
 */

void
theme_unregister_plugin (struct t_weechat_plugin *plugin)
{
    struct t_theme *ptr_theme;
    struct t_theme_contribution *ptr_contribution, *next_contribution;

    if (!plugin)
        return;
    for (ptr_theme = themes; ptr_theme; ptr_theme = ptr_theme->next_theme)
    {
        ptr_contribution = ptr_theme->contributions;
        while (ptr_contribution)
        {
            next_contribution = ptr_contribution->next_contribution;
            if ((ptr_contribution->plugin == plugin)
                && (ptr_contribution->script == NULL))
            {
                theme_drop_contribution (ptr_theme, ptr_contribution);
            }
            ptr_contribution = next_contribution;
        }
    }
}

/*
 * Drops all contributions owned by a given script (across every theme).
 * Called from the script-unload lifecycle (a later commit).
 */

void
theme_unregister_script (struct t_weechat_plugin *plugin,
                         const void *script)
{
    struct t_theme *ptr_theme;
    struct t_theme_contribution *ptr_contribution, *next_contribution;

    if (!plugin || !script)
        return;
    for (ptr_theme = themes; ptr_theme; ptr_theme = ptr_theme->next_theme)
    {
        ptr_contribution = ptr_theme->contributions;
        while (ptr_contribution)
        {
            next_contribution = ptr_contribution->next_contribution;
            if ((ptr_contribution->plugin == plugin)
                && (ptr_contribution->script == script))
            {
                theme_drop_contribution (ptr_theme, ptr_contribution);
            }
            ptr_contribution = next_contribution;
        }
    }
}

/*
 * Returns the total number of overrides across all contributions of a
 * theme. Duplicate keys (across contributions) are counted multiple
 * times; the actual merged-unique count is at most this number.
 */

int
theme_overrides_count (struct t_theme *theme)
{
    struct t_theme_contribution *ptr;
    int n;

    if (!theme)
        return 0;
    n = 0;
    for (ptr = theme->contributions; ptr; ptr = ptr->next_contribution)
        n += ptr->overrides->items_count;
    return n;
}

/*
 * Returns the effective value of an option override across the theme's
 * contributions (later contributions win). Returns NULL if no
 * contribution provides the key.
 */

const char *
theme_get_override (struct t_theme *theme, const char *option_name)
{
    struct t_theme_contribution *ptr;
    const char *value, *latest;

    if (!theme || !option_name)
        return NULL;
    latest = NULL;
    for (ptr = theme->contributions; ptr; ptr = ptr->next_contribution)
    {
        value = (const char *)hashtable_get (ptr->overrides, option_name);
        if (value)
            latest = value;
    }
    return latest;
}

/*
 * Compares two themes by name (callback used by arraylist sort).
 *
 * Returns negative, zero, or positive value (like strcmp).
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
 * Returns an arraylist of t_theme * for all registered themes (built-ins).
 *
 * The returned arraylist owns no data; callers must not free its items.
 * Returns NULL on allocation failure.
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
 * Builds the on-disk path for a user theme:
 * "<weechat_config_dir>/themes/<name>.theme".
 *
 * Returned string is allocated; caller frees. Returns NULL on error.
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
 * Builds a unique backup theme name "backup-YYYYMMDD-HHMMSS-uuuuuu".
 *
 * Returned string is allocated; caller frees. Returns NULL on error.
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
 * Writes a snapshot of themable options to a .theme file at
 * "<weechat_config_dir>/themes/<name>.theme".
 *
 * The themes directory is created if missing. The file contains an
 * [info] section (name, description, date, weechat version) followed by
 * an [options] section.
 *
 * If "diff_only" is non-zero, only options whose value differs from
 * their default (config_file_option_has_changed) are written. If zero,
 * every themable option is written (full snapshot).
 *
 * Returns path to saved file on success, NULL on error.
 *
 * Note: result must be freed after use.
 */

char *
theme_write_file (const char *name, const char *description, int diff_only)
{
    char *path, *dir, *value, *now;
    FILE *file;
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;

    if (!name || !name[0])
        return NULL;

    path = NULL;
    dir = NULL;
    string_asprintf (&dir, "%s/themes", weechat_config_dir);
    if (!dir)
        return NULL;
    dir_mkdir (dir, 0755);
    free (dir);

    path = theme_user_file_path (name);
    if (!path)
        return NULL;

    file = fopen (path, "w");
    if (!file)
    {
        free (path);
        return NULL;
    }

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
                if (diff_only && !config_file_option_has_changed (ptr_option))
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
    return path;
}

/*
 * Creates a timestamped backup theme file with the current themable state.
 *
 * Returned string is the backup name (caller frees), NULL on failure.
 */

char *
theme_make_backup (void)
{
    char *name, *path;

    name = theme_make_backup_name ();
    if (!name)
        return NULL;
    path = theme_write_file (
        name,
        _("Automatic backup written before /theme apply"),
        0);  /* full snapshot: backups must round-trip exactly */
    if (!path)
    {
        free (name);
        return NULL;
    }
    free (path);
    return name;
}

/*
 * Applies one override entry (callback for hashtable_map during apply).
 *
 * Refuses entries pointing to options that do not exist or that are not
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
 * Strips one optional pair of matching surrounding quotes (' or ") from
 * the in-place string; returns a pointer that may differ from the input
 * (advances past an opening quote).
 */

char *
theme_file_strip_quotes (char *value)
{
    size_t len;

    if (!value)
        return value;
    len = strlen (value);
    if ((len >= 2)
        && (((value[0] == '"') && (value[len - 1] == '"'))
            || ((value[0] == '\'') && (value[len - 1] == '\''))))
    {
        value[len - 1] = '\0';
        return value + 1;
    }
    return value;
}

/*
 * Parses a .theme file into a transient t_theme.
 *
 * The file uses two INI-like sections: [info] (keys: name, description,
 * date, weechat) and [options] (key = full option name like
 * "irc.color.input_nick", value = string). Unknown [info] keys produce a
 * warning and are ignored; unknown sections produce a warning and the
 * lines in them are skipped.
 *
 * Returns a heap-allocated t_theme (caller frees with theme_free), or
 * NULL if the file cannot be opened.
 */

struct t_theme *
theme_file_parse (const char *path)
{
    FILE *file;
    char line[8192], *ptr, *end, *eq, *key, *value;
    int line_number, in_options;
    struct t_theme *theme;
    struct t_theme_contribution *contribution;

    if (!path)
        return NULL;

    file = fopen (path, "r");
    if (!file)
        return NULL;

    theme = theme_alloc ("");
    if (!theme)
    {
        fclose (file);
        return NULL;
    }
    /* file themes carry a single anonymous (plugin=NULL, script=NULL)
       contribution holding everything in the [options] section */
    contribution = theme_contribution_new (theme, NULL, NULL);
    if (!contribution)
    {
        theme_free (theme);
        fclose (file);
        return NULL;
    }
    /* clear the placeholder name; the file should provide it */
    free (theme->name);
    theme->name = NULL;
    /* description/date/weechat_version come from the file too */
    free (theme->description);
    theme->description = NULL;
    free (theme->date);
    theme->date = NULL;
    free (theme->weechat_version);
    theme->weechat_version = NULL;

    line_number = 0;
    in_options = 0;
    while (fgets (line, sizeof (line) - 1, file))
    {
        line_number++;

        /* trim trailing CR / LF */
        end = strchr (line, '\r');
        if (end)
            *end = '\0';
        end = strchr (line, '\n');
        if (end)
            *end = '\0';

        /* skip leading whitespace */
        ptr = line;
        while ((ptr[0] == ' ') || (ptr[0] == '\t'))
            ptr++;

        /* skip empty lines and comments */
        if (!ptr[0] || (ptr[0] == '#'))
            continue;

        /* section header */
        if (ptr[0] == '[')
        {
            end = strchr (ptr, ']');
            if (!end)
            {
                gui_chat_printf (
                    NULL,
                    _("%s%s: line %d: malformed section header"),
                    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                    path, line_number);
                continue;
            }
            *end = '\0';
            if (strcmp (ptr + 1, "info") == 0)
            {
                in_options = 0;
            }
            else if (strcmp (ptr + 1, "options") == 0)
            {
                in_options = 1;
            }
            else
            {
                gui_chat_printf (
                    NULL,
                    _("%s%s: line %d: ignoring unknown section \"%s\""),
                    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                    path, line_number, ptr + 1);
                in_options = -1;  /* skip lines until next known section */
            }
            continue;
        }

        if (in_options < 0)
            continue;

        /* "key = value" */
        eq = strchr (ptr, '=');
        if (!eq)
        {
            gui_chat_printf (
                NULL,
                _("%s%s: line %d: missing '=' separator"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                path, line_number);
            continue;
        }

        /* trim key */
        key = ptr;
        end = eq - 1;
        while ((end > key) && ((end[0] == ' ') || (end[0] == '\t')))
            end--;
        end[1] = '\0';

        /* trim value */
        value = eq + 1;
        while ((value[0] == ' ') || (value[0] == '\t'))
            value++;
        end = value + strlen (value) - 1;
        while ((end > value) && ((end[0] == ' ') || (end[0] == '\t')))
            end--;
        end[1] = '\0';

        value = theme_file_strip_quotes (value);

        if (in_options)
        {
            hashtable_set (contribution->overrides, key, value);
        }
        else
        {
            /* [info] section */
            if (strcmp (key, "name") == 0)
            {
                free (theme->name);
                theme->name = strdup (value);
            }
            else if (strcmp (key, "description") == 0)
            {
                free (theme->description);
                theme->description = strdup (value);
            }
            else if (strcmp (key, "date") == 0)
            {
                free (theme->date);
                theme->date = strdup (value);
            }
            else if (strcmp (key, "weechat") == 0)
            {
                free (theme->weechat_version);
                theme->weechat_version = strdup (value);
            }
            else
            {
                gui_chat_printf (
                    NULL,
                    _("%s%s: line %d: ignoring unknown [info] key \"%s\""),
                    gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                    path, line_number, key);
            }
        }
    }
    fclose (file);

    if (!theme->name)
        theme->name = strdup ("");
    if (!theme->description)
        theme->description = strdup ("");
    if (!theme->date)
        theme->date = strdup ("");
    if (!theme->weechat_version)
        theme->weechat_version = strdup ("");

    return theme;
}

/*
 * Applies a theme registered in memory.
 *
 * If weechat.look.theme_backup is on (and the target name does not begin
 * with "backup-"), a backup file is written first; on backup failure the
 * apply is aborted before any option is changed.
 *
 * Iterates the theme's overrides with theme_applying=1 so the per-option
 * change callbacks skip their gui refresh; a single refresh is performed
 * at the end.
 *
 * Returns WEECHAT_RC_OK on success, WEECHAT_RC_ERROR if the theme name
 * is unknown or the backup could not be created.
 */

int
theme_apply (const char *name)
{
    struct t_theme *file_theme = NULL;
    struct t_theme *theme = NULL;
    struct t_theme_contribution *ptr_contribution;
    char *path = NULL;
    char *backup_name = NULL;

    if (!name || !name[0])
        return WEECHAT_RC_ERROR;

    /* Resolution: a user file with the given name shadows any built-in
       of the same name. Read the file transiently (parse, apply, free)
       so user themes have no steady-state memory footprint. */
    path = theme_user_file_path (name);
    if (path && (access (path, R_OK) == 0))
    {
        file_theme = theme_file_parse (path);
        if (!file_theme)
        {
            gui_chat_printf (NULL,
                             _("%sFailed to parse theme file \"%s\""),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             path);
            free (path);
            return WEECHAT_RC_ERROR;
        }
        theme = file_theme;
    }
    else
    {
        theme = theme_search (name);
        if (!theme)
        {
            gui_chat_printf (NULL,
                             _("%sTheme \"%s\" not found"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             name);
            free (path);
            return WEECHAT_RC_ERROR;
        }
    }
    free (path);

    /* create a backup of current themable state, if enabled */
    if (CONFIG_BOOLEAN(config_look_theme_backup)
        && (strncmp (name, "backup-", 7) != 0))
    {
        backup_name = theme_make_backup ();
        if (!backup_name)
        {
            theme_free (file_theme);
            gui_chat_printf (
                NULL,
                _("%sUnable to create theme backup; aborting apply "
                  "(disable option weechat.look.theme_backup to force)"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_ERROR;
        }
    }

    /* Apply each contribution in order; per-option refreshes are
       suppressed via the theme_applying flag (see config_change_color).
       Later contributions naturally win for duplicate keys because
       config_file_option_set is called for each in sequence. */
    theme_applying = 1;
    for (ptr_contribution = theme->contributions; ptr_contribution;
         ptr_contribution = ptr_contribution->next_contribution)
    {
        hashtable_map (ptr_contribution->overrides,
                       &theme_apply_set_option_cb, NULL);
    }
    theme_applying = 0;

    /* file_theme (if any) is transient: discard now */
    theme_free (file_theme);

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
 * Resets every themable option to its default value.
 *
 * Same backup-first safety as theme_apply: if weechat.look.theme_backup
 * is on, a backup file is written before any option is touched, and the
 * reset is aborted if the backup cannot be written. The active-theme
 * label (weechat.look.theme) is reset to its default (empty string).
 *
 * Returns WEECHAT_RC_OK on success, WEECHAT_RC_ERROR if the backup is
 * required but failed.
 */

int
theme_reset (void)
{
    struct t_config_file *ptr_config;
    struct t_config_section *ptr_section;
    struct t_config_option *ptr_option;
    char *backup_name = NULL;

    if (CONFIG_BOOLEAN(config_look_theme_backup))
    {
        backup_name = theme_make_backup ();
        if (!backup_name)
        {
            gui_chat_printf (
                NULL,
                _("%sUnable to create theme backup; aborting reset "
                  "(disable option weechat.look.theme_backup to force)"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
            return WEECHAT_RC_ERROR;
        }
    }

    /* reset every themable option to its default value; per-option gui
       refreshes are suppressed via theme_applying */
    theme_applying = 1;
    for (ptr_config = config_files; ptr_config;
         ptr_config = ptr_config->next_config)
    {
        for (ptr_section = ptr_config->sections; ptr_section;
             ptr_section = ptr_section->next_section)
        {
            for (ptr_option = ptr_section->options; ptr_option;
                 ptr_option = ptr_option->next_option)
            {
                if (ptr_option->themable)
                    config_file_option_reset (ptr_option, 1);
            }
        }
    }
    theme_applying = 0;

    if (gui_init_ok)
    {
        gui_color_init_weechat ();
        gui_window_ask_refresh (1);
    }

    /* clear active-theme label */
    config_file_option_reset (config_look_theme, 1);

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
                      WEECHAT_HOOK_SIGNAL_STRING, (char *)"");

    return WEECHAT_RC_OK;
}

/*
 * Saves the current themable options to a user theme file.
 *
 * Refuses names that match a built-in theme (registered via API) or
 * that start with "backup-" (reserved for automatic backups). If
 * "full" is non-zero, every themable option is written; otherwise
 * only options whose value differs from their default are written.
 *
 * Returns WEECHAT_RC_OK on success, WEECHAT_RC_ERROR on validation or
 * I/O failure.
 */

int
theme_save (const char *name, int full)
{
    char *path;

    if (!name || !name[0])
        return WEECHAT_RC_ERROR;

    if (strncmp (name, "backup-", 7) == 0)
    {
        gui_chat_printf (
            NULL,
            _("%sName \"%s\" is reserved for automatic backups"),
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
            name);
        return WEECHAT_RC_ERROR;
    }

    if (theme_search (name))
    {
        gui_chat_printf (
            NULL,
            _("%sName \"%s\" is reserved for a built-in theme"),
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
            name);
        return WEECHAT_RC_ERROR;
    }

    path = theme_write_file (name, NULL, (full) ? 0 : 1);
    if (!path)
    {
        gui_chat_printf (NULL,
                         _("%sFailed to save theme \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
        return WEECHAT_RC_ERROR;
    }

    gui_chat_printf (NULL, _("Theme saved to: %s"), path);
    free (path);
    return WEECHAT_RC_OK;
}

/*
 * Deletes a user theme file.
 *
 * Refuses names registered as built-in themes (they have no file).
 * Returns WEECHAT_RC_OK on success, WEECHAT_RC_ERROR otherwise.
 */

int
theme_delete (const char *name)
{
    char *path;

    if (!name || !name[0])
        return WEECHAT_RC_ERROR;

    if (theme_search (name))
    {
        gui_chat_printf (
            NULL,
            _("%sCannot delete built-in theme \"%s\""),
            gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
            name);
        return WEECHAT_RC_ERROR;
    }

    path = theme_user_file_path (name);
    if (!path)
        return WEECHAT_RC_ERROR;

    if (unlink (path) != 0)
    {
        gui_chat_printf (NULL,
                         _("%sFailed to delete theme \"%s\""),
                         gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                         name);
        free (path);
        return WEECHAT_RC_ERROR;
    }

    gui_chat_printf (NULL,
                     _("Theme deleted: %s"),
                     name);
    free (path);
    return WEECHAT_RC_OK;
}

/*
 * Initializes the theme subsystem.
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
 * Frees all registered themes and clears the registry.
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
