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

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "weechat.h"
#include "core-arraylist.h"
#include "core-hashtable.h"
#include "core-string.h"
#include "core-theme.h"
#include "core-version.h"
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
 * Frees a theme (does not unlink from registry; caller handles that).
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
 * Registers a theme by name with a set of option overrides.
 *
 * If a theme with the given name already exists, the provided overrides
 * are merged into the existing theme's hashtable (later registrations
 * override earlier ones for duplicate keys). This lets plugins/scripts
 * register their per-theme contributions without coordinating with core.
 *
 * The "overrides" hashtable passed in is read-only from this function's
 * perspective; the caller retains ownership and may free it.
 *
 * Returns pointer to theme (existing or newly created), NULL on error.
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
