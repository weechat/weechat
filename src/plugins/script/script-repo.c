/*
 * script-repo.c - download and read repository file (plugins.xml.gz)
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

/*
 * this define is needed for strptime()
 * but is not used for OpenBSD, it causes bug with gcrypt (see bug #37373)
 */
#if !defined(__OpenBSD__) && !defined(__sun)
#define _XOPEN_SOURCE 700
#endif

#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <zlib.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-repo.h"
#include "script-action.h"
#include "script-buffer.h"
#include "script-config.h"


struct t_script_repo *scripts_repo = NULL;
struct t_script_repo *last_script_repo = NULL;
int script_repo_count = 0;
int script_repo_count_displayed = 0;
struct t_hashtable *script_repo_max_length_field = NULL;
char *script_repo_filter = NULL;


/*
 * Checks if a script pointer is valid.
 *
 * Returns:
 *   1: script exists
 *   0: script does not exist
 */

int
script_repo_script_valid (struct t_script_repo *script)
{
    struct t_script_repo *ptr_script;

    if (!script)
        return 0;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (ptr_script == script)
            return 1;
    }

    /* script not found */
    return 0;
}

/*
 * Searches for a script displayed by number (first script displayed is 0).
 *
 * Returns pointer to script found, NULL if not found.
 */

struct t_script_repo *
script_repo_search_displayed_by_number (int number)
{
    struct t_script_repo *ptr_script;
    int i;

    if (number < 0)
        return NULL;

    i = 0;
    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (ptr_script->displayed)
        {
            if (i == number)
                return ptr_script;
            i++;
        }
    }

    /* script not found */
    return NULL;
}

/*
 * Searches for a script by name (example: "iset").
 *
 * Returns pointer to script found, NULL if not found.
 */

struct t_script_repo *
script_repo_search_by_name (const char *name)
{
    struct t_script_repo *ptr_script;

    if (!name)
        return NULL;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (strcmp (ptr_script->name, name) == 0)
            return ptr_script;
    }

    /* script not found */
    return NULL;
}

/*
 * Searches for a script by name/extension (example: "go.py").
 *
 * Returns pointer to script found, NULL if not found.
 */

struct t_script_repo *
script_repo_search_by_name_ext (const char *name_with_extension)
{
    struct t_script_repo *ptr_script;

    if (!name_with_extension)
        return NULL;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (strcmp (ptr_script->name_with_extension, name_with_extension) == 0)
            return ptr_script;
    }

    /* script not found */
    return NULL;
}

/*
 * Gets filename of a loaded script (returns name of file and not the link, if
 * there is a symbolic to file).
 *
 * Note: result must be freed after use.
 */

char *
script_repo_get_filename_loaded (struct t_script_repo *script)
{
    char *weechat_data_dir, *filename, resolved_path[PATH_MAX];
    int length;
    struct stat st;

    weechat_data_dir = weechat_info_get ("weechat_data_dir", NULL);
    length = strlen (weechat_data_dir) + strlen (script->name_with_extension) + 64;
    filename = malloc (length);
    if (!filename)
    {
        if (weechat_data_dir)
            free (weechat_data_dir);
        return NULL;
    }

    snprintf (filename, length, "%s/%s/autoload/%s",
              weechat_data_dir,
              script_language[script->language],
              script->name_with_extension);
    if (stat (filename, &st) != 0)
    {
        snprintf (filename, length, "%s/%s/%s",
                  weechat_data_dir,
                  script_language[script->language],
                  script->name_with_extension);
        if (stat (filename, &st) != 0)
        {
            filename[0] = '\0';
        }
    }

    if (weechat_data_dir)
        free (weechat_data_dir);

    if (!filename[0])
    {
        free (filename);
        return NULL;
    }

    if (realpath (filename, resolved_path))
    {
        if (strcmp (filename, resolved_path) != 0)
        {
            free (filename);
            return strdup (resolved_path);
        }
    }

    return filename;
}

/*
 * Gets status for display.
 *
 * Argument "list" is the codes of status to display:
 *   *  popular
 *   i  installed
 *   a  autoloaded
 *   ?  unknown
 *   H  held
 *   r  running
 *   N  obsolete
 */

const char *
script_repo_get_status_for_display (struct t_script_repo *script,
                                    const char *list,
                                    int collapse)
{
    static char str_status[128];
    const char *ptr_list;
    char str_space[2];

    str_space[0] = (collapse) ? '\0' : ' ';
    str_space[1] = '\0';

    str_status[0] = '\0';

    for (ptr_list = list; ptr_list[0]; ptr_list++)
    {
        switch (ptr_list[0])
        {
            case '*':
                strcat (str_status, weechat_color (weechat_config_string (script_config_color_status_popular)));
                strcat (str_status, (script && (script->popularity > 0)) ? "*" : str_space);
                break;
            case 'i':
                strcat (str_status, weechat_color (weechat_config_string (script_config_color_status_installed)));
                strcat (str_status, (script && (script->status & SCRIPT_STATUS_INSTALLED)) ? "i" : str_space);
                break;
            case 'a':
                strcat (str_status, weechat_color (weechat_config_string (script_config_color_status_autoloaded)));
                strcat (str_status, (script && (script->status & SCRIPT_STATUS_AUTOLOADED)) ? "a" : str_space);
                break;
            case '?':
                strcat (str_status, weechat_color (weechat_config_string (script_config_color_status_unknown)));
                strcat (str_status, (script) ? str_space : "?");
                break;
            case 'H':
                strcat (str_status, weechat_color (weechat_config_string (script_config_color_status_held)));
                strcat (str_status, (script && (script->status & SCRIPT_STATUS_HELD)) ? "H" : str_space);
                break;
            case 'r':
                strcat (str_status, weechat_color (weechat_config_string (script_config_color_status_running)));
                strcat (str_status, (script && (script->status & SCRIPT_STATUS_RUNNING)) ? "r" : str_space);
                break;
            case 'N':
                strcat (str_status, weechat_color (weechat_config_string (script_config_color_status_obsolete)));
                strcat (str_status, (script && (script->status & SCRIPT_STATUS_NEW_VERSION)) ? "N" : str_space);
                break;
        }
    }

    return str_status;
}

/*
 * Gets status description for display.
 *
 * Argument "list" is the codes of status to display (see function
 * script_repo_get_status_for_display() above).
 */

const char *
script_repo_get_status_desc_for_display (struct t_script_repo *script,
                                         const char *list)
{
    static char str_status[256];
    const char *ptr_list;

    str_status[0] = '\0';

    if (!script)
        return str_status;

    for (ptr_list = list; ptr_list[0]; ptr_list++)
    {
        switch (ptr_list[0])
        {
            case '*':
                if (script->popularity > 0)
                {
                    if (str_status[0])
                        strcat (str_status, " ");
                    /* TRANSLATORS: translation must be one short word without spaces (replace spaces by underscores if needed) */
                    strcat (str_status, _("popular"));
                }
                break;
            case 'i':
                if (script->status & SCRIPT_STATUS_INSTALLED)
                {
                    if (str_status[0])
                        strcat (str_status, " ");
                    /* TRANSLATORS: translation must be one short word without spaces (replace spaces by underscores if needed) */
                    strcat (str_status, _("installed"));
                }
                break;
            case 'a':
                if (script->status & SCRIPT_STATUS_AUTOLOADED)
                {
                    if (str_status[0])
                        strcat (str_status, " ");
                    /* TRANSLATORS: translation must be one short word without spaces (replace spaces by underscores if needed) */
                    strcat (str_status, _("autoloaded"));
                }
                break;
            case 'H':
                if (script->status & SCRIPT_STATUS_HELD)
                {
                    if (str_status[0])
                        strcat (str_status, " ");
                    /* TRANSLATORS: translation must be one short word without spaces (replace spaces by underscores if needed) */
                    strcat (str_status, _("held"));
                }
                break;
            case 'r':
                if (script->status & SCRIPT_STATUS_RUNNING)
                {
                    if (str_status[0])
                        strcat (str_status, " ");
                    /* TRANSLATORS: translation must be one short word without spaces (replace spaces by underscores if needed) */
                    strcat (str_status, _("running"));
                }
                break;
            case 'N':
                if (script->status & SCRIPT_STATUS_NEW_VERSION)
                {
                    if (str_status[0])
                        strcat (str_status, " ");
                    /* TRANSLATORS: translation must be one short word without spaces (replace spaces by underscores if needed) */
                    strcat (str_status, _("obsolete"));
                }
                break;
        }
    }

    return str_status;
}

/*
 * Allocates a script structure.
 *
 * Returns pointer to new script, NULL if error.
 */

struct t_script_repo *
script_repo_alloc ()
{
    struct t_script_repo *new_script;

    new_script = malloc (sizeof (*new_script));
    if (new_script)
    {
        new_script->name = NULL;
        new_script->name_with_extension = NULL;
        new_script->language = -1;
        new_script->author = NULL;
        new_script->mail = NULL;
        new_script->version = NULL;
        new_script->license = NULL;
        new_script->description = NULL;
        new_script->tags = NULL;
        new_script->requirements = NULL;
        new_script->min_weechat = NULL;
        new_script->max_weechat = NULL;
        new_script->sha512sum = NULL;
        new_script->url = NULL;
        new_script->popularity = 0;
        new_script->date_added = 0;
        new_script->date_updated = 0;
        new_script->status = 0;
        new_script->version_loaded = NULL;
        new_script->displayed = 1;
        new_script->install_order = 0;
        new_script->prev_script = NULL;
        new_script->next_script = NULL;
    }

    return new_script;
}

/*
 * Compares two scripts using sort key(s) (from option script.look.sort).
 *
 * Returns:
 *   < 0: script1 < script2
 *     0: script1 == script2
 *   > 0: script1 > script2
 */

int
script_repo_compare_scripts (struct t_script_repo *script1,
                             struct t_script_repo *script2)
{
    const char *ptr_sort;
    int cmp, reverse;

    reverse = 1;
    ptr_sort = weechat_config_string (script_config_look_sort);
    while (ptr_sort[0])
    {
        cmp = 0;
        switch (ptr_sort[0])
        {
            case '-': /* reverse order */
                reverse = -1;
                break;
            case 'a': /* author */
                cmp = strcmp (script1->author, script2->author);
                break;
            case 'A': /* status autoloaded */
                if ((script1->status & SCRIPT_STATUS_AUTOLOADED)
                    && !(script2->status & SCRIPT_STATUS_AUTOLOADED))
                    cmp = -1;
                else if (!(script1->status & SCRIPT_STATUS_AUTOLOADED)
                         && (script2->status & SCRIPT_STATUS_AUTOLOADED))
                    cmp = 1;
                break;
            case 'd': /* date added */
                if (script1->date_added > script2->date_added)
                    cmp = -1;
                else if (script1->date_added < script2->date_added)
                    cmp = 1;
                break;
            case 'e': /* extension */
                cmp = strcmp (script_extension[script1->language],
                              script_extension[script2->language]);
                break;
            case 'i': /* status "installed" */
                if ((script1->status & SCRIPT_STATUS_INSTALLED)
                    && !(script2->status & SCRIPT_STATUS_INSTALLED))
                    cmp = -1;
                else if (!(script1->status & SCRIPT_STATUS_INSTALLED)
                         && (script2->status & SCRIPT_STATUS_INSTALLED))
                    cmp = 1;
                break;
            case 'l': /* language */
                cmp = strcmp (script_language[script1->language],
                              script_language[script2->language]);
                break;
            case 'n': /* name */
                cmp = strcmp (script1->name, script2->name);
                break;
            case 'o': /* status "new version" (script obsolete) */
                if ((script1->status & SCRIPT_STATUS_NEW_VERSION)
                    && !(script2->status & SCRIPT_STATUS_NEW_VERSION))
                    cmp = -1;
                else if (!(script1->status & SCRIPT_STATUS_NEW_VERSION)
                         && (script2->status & SCRIPT_STATUS_NEW_VERSION))
                    cmp = 1;
                break;
            case 'p': /* popularity */
                if (script1->popularity > script2->popularity)
                    cmp = -1;
                else if (script1->popularity < script2->popularity)
                    cmp = 1;
                break;
            case 'r': /* status "running" */
                if ((script1->status & SCRIPT_STATUS_RUNNING)
                    && !(script2->status & SCRIPT_STATUS_RUNNING))
                    cmp = -1;
                else if (!(script1->status & SCRIPT_STATUS_RUNNING)
                         && (script2->status & SCRIPT_STATUS_RUNNING))
                    cmp = 1;
                break;
            case 'u': /* date updated */
                if (script1->date_updated > script2->date_updated)
                    cmp = -1;
                else if (script1->date_updated < script2->date_updated)
                    cmp = 1;
                break;
            default:
                reverse = 1;
                break;
        }
        if (cmp != 0)
            return cmp * reverse;
        ptr_sort++;
    }

    return 0;
}

/*
 * Searches for position for script in list (to keep list sorted).
 */

struct t_script_repo *
script_repo_find_pos (struct t_script_repo *script)
{
    struct t_script_repo *ptr_script;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        if (script_repo_compare_scripts (ptr_script, script) > 0)
            return ptr_script;
    }

    /* position not found, add to the end */
    return NULL;
}

/*
 * Sets max length for a field in hashtable "script_repo_max_length_field".
 */

void
script_repo_set_max_length_field (const char *field, int length)
{
    int *value;

    value = weechat_hashtable_get (script_repo_max_length_field, field);
    if (!value || (length > *value))
        weechat_hashtable_set (script_repo_max_length_field, field, &length);
}

/*
 * Adds a script to list of scripts.
 */

void
script_repo_add (struct t_script_repo *script)
{
    struct t_script_repo *ptr_script;

    ptr_script = script_repo_find_pos (script);
    if (ptr_script)
    {
        /* insert script before script found */
        script->prev_script = ptr_script->prev_script;
        script->next_script = ptr_script;
        if (ptr_script->prev_script)
            (ptr_script->prev_script)->next_script = script;
        else
            scripts_repo = script;
        ptr_script->prev_script = script;
    }
    else
    {
        /* add script to the end */
        script->prev_script = last_script_repo;
        script->next_script = NULL;
        if (last_script_repo)
            last_script_repo->next_script = script;
        else
            scripts_repo = script;
        last_script_repo = script;
    }

    /* set max length for fields */
    if (script->name)
        script_repo_set_max_length_field ("N", weechat_utf8_strlen_screen (script->name));
    if (script->name_with_extension)
        script_repo_set_max_length_field ("n", weechat_utf8_strlen_screen (script->name_with_extension));
    if (script->language >= 0)
    {
        script_repo_set_max_length_field ("l", weechat_utf8_strlen_screen (script_language[script->language]));
        script_repo_set_max_length_field ("e", weechat_utf8_strlen_screen (script_extension[script->language]));
    }
    if (script->author)
        script_repo_set_max_length_field ("a", weechat_utf8_strlen_screen (script->author));
    if (script->version)
        script_repo_set_max_length_field ("v", weechat_utf8_strlen_screen (script->version));
    if (script->version_loaded)
        script_repo_set_max_length_field ("V", weechat_utf8_strlen_screen (script->version_loaded));
    if (script->license)
        script_repo_set_max_length_field ("L", weechat_utf8_strlen_screen (script->license));
    if (script->description)
        script_repo_set_max_length_field ("d", weechat_utf8_strlen_screen (script->description));
    if (script->tags)
        script_repo_set_max_length_field ("t", weechat_utf8_strlen_screen (script->tags));
    if (script->requirements)
        script_repo_set_max_length_field ("r", weechat_utf8_strlen_screen (script->requirements));
    if (script->min_weechat)
        script_repo_set_max_length_field ("w", weechat_utf8_strlen_screen (script->min_weechat));
    if (script->max_weechat)
        script_repo_set_max_length_field ("W", weechat_utf8_strlen_screen (script->max_weechat));

    script_repo_count++;
    if (script->displayed)
        script_repo_count_displayed++;
}

/*
 * Frees data in a script.
 */

void
script_repo_free (struct t_script_repo *script)
{
    if (!script)
        return;

    if (script->name)
        free (script->name);
    if (script->name_with_extension)
        free (script->name_with_extension);
    if (script->author)
        free (script->author);
    if (script->mail)
        free (script->mail);
    if (script->version)
        free (script->version);
    if (script->license)
        free (script->license);
    if (script->description)
        free (script->description);
    if (script->tags)
        free (script->tags);
    if (script->requirements)
        free (script->requirements);
    if (script->min_weechat)
        free (script->min_weechat);
    if (script->max_weechat)
        free (script->max_weechat);
    if (script->sha512sum)
        free (script->sha512sum);
    if (script->url)
        free (script->url);
    if (script->version_loaded)
        free (script->version_loaded);

    free (script);
 }

/*
 * Removes a script from list.
 */

void
script_repo_remove (struct t_script_repo *script)
{
    struct t_script_repo *new_scripts_repo;

    /* unlink script from buffer (if it is used) */
    if (script_buffer_detail_script == script)
        script_buffer_detail_script = NULL;

    /* remove script from list */
    if (last_script_repo == script)
        last_script_repo = script->prev_script;
    if (script->prev_script)
    {
        (script->prev_script)->next_script = script->next_script;
        new_scripts_repo = scripts_repo;
    }
    else
        new_scripts_repo = script->next_script;
    if (script->next_script)
        (script->next_script)->prev_script = script->prev_script;

    /* free data */
    if (script->displayed)
        script_repo_count_displayed--;
    script_repo_free (script);

    scripts_repo = new_scripts_repo;

    script_repo_count--;

    if (script_buffer_selected_line >= script_repo_count_displayed)
    {
        script_buffer_selected_line = (script_repo_count_displayed == 0) ?
            0 : script_repo_count_displayed - 1;
    }
}

/*
 * Removes all scripts from list.
 */

void
script_repo_remove_all ()
{
    while (scripts_repo)
    {
        script_repo_remove (scripts_repo);
    }
    if (script_repo_max_length_field)
    {
        weechat_hashtable_free (script_repo_max_length_field);
        script_repo_max_length_field = NULL;
    }
}

/*
 * Checks if a script is held.
 *
 * Returns:
 *   1: script is held
 *   0: script is not held
 */

int
script_repo_script_is_held (struct t_script_repo *script)
{
    const char *hold;
    char *pos;
    int length;

    hold = weechat_config_string (script_config_scripts_hold);
    length = strlen (script->name_with_extension);
    pos = strstr (hold, script->name_with_extension);
    while (pos)
    {
        if (((pos == hold) || (*(pos - 1) == ','))
            && ((pos[length] == ',') || !pos[length]))
        {
            /* script held */
            return 1;
        }
        pos = strstr (pos + 1, script->name_with_extension);
    }

    /* script not held */
    return 0;
}

/*
 * Computes SHA-512 checksum for the content of a file.
 *
 * Note: result must be freed after use.
 */

char *
script_repo_sha512sum_file (const char *filename)
{
    char hash[512 / 8], hash_hexa[((512 / 8) * 2) + 1];
    int hash_size;

    if (!weechat_crypto_hash_file (filename, "sha512", hash, &hash_size))
        return NULL;

    weechat_string_base_encode (16, hash, hash_size, hash_hexa);

    return weechat_string_tolower (hash_hexa);
}

/*
 * Updates following status of a script:
 *   - script installed?
 *   - script running?
 *   - new version available?
 */

void
script_repo_update_status (struct t_script_repo *script)
{
    const char *version;
    char *weechat_data_dir, *filename, *sha512sum;
    struct stat st;
    int length;
    struct t_script_repo *ptr_script;

    script->status = 0;
    sha512sum = NULL;

    /* check if script is installed (file found on disk) */
    weechat_data_dir = weechat_info_get ("weechat_data_dir", NULL);
    length = strlen (weechat_data_dir) + strlen (script->name_with_extension) + 64;
    filename = malloc (length);
    if (filename)
    {
        snprintf (filename, length, "%s/%s/autoload/%s",
                  weechat_data_dir,
                  script_language[script->language],
                  script->name_with_extension);
        if (stat (filename, &st) == 0)
        {
            script->status |= SCRIPT_STATUS_INSTALLED;
            script->status |= SCRIPT_STATUS_AUTOLOADED;
            sha512sum = script_repo_sha512sum_file (filename);
        }
        else
        {
            snprintf (filename, length, "%s/%s/%s",
                      weechat_data_dir,
                      script_language[script->language],
                      script->name_with_extension);
            if (stat (filename, &st) == 0)
            {
                script->status |= SCRIPT_STATUS_INSTALLED;
                sha512sum = script_repo_sha512sum_file (filename);
            }
        }
        free (filename);
    }

    if (weechat_data_dir)
        free (weechat_data_dir);

    /* check if script is held */
    if (script_repo_script_is_held (script))
        script->status |= SCRIPT_STATUS_HELD;

    /* check if script is running (loaded) */
    version = weechat_hashtable_get (script_loaded, script->name_with_extension);
    if (version)
    {
        script->status |= SCRIPT_STATUS_RUNNING;
        if (script->version_loaded)
            free (script->version_loaded);
        script->version_loaded = strdup (version);
    }
    else
    {
        if (script->version_loaded)
        {
            free (script->version_loaded);
            script->version_loaded = NULL;
        }
    }

    /* check if script has new version (script is obsolete) */
    if (sha512sum && script->sha512sum
        && (strcmp (script->sha512sum, sha512sum) != 0))
    {
        script->status |= SCRIPT_STATUS_NEW_VERSION;
    }

    /* recompute max length for version loaded (for display) */
    if (script_repo_max_length_field)
    {
        length = 0;
        weechat_hashtable_set (script_repo_max_length_field, "V", &length);
        for (ptr_script = scripts_repo; ptr_script;
             ptr_script = ptr_script->next_script)
        {
            if (ptr_script->version_loaded)
                script_repo_set_max_length_field ("V", weechat_utf8_strlen_screen (ptr_script->version_loaded));
        }
    }

    if (sha512sum)
        free (sha512sum);
}

/*
 * Updates status of all scripts.
 */

void
script_repo_update_status_all ()
{
    struct t_script_repo *ptr_script;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        script_repo_update_status (ptr_script);
    }
}

/*
 * Sets filter for scripts.
 */

void
script_repo_set_filter (const char *filter)
{
    if (script_repo_filter)
        free (script_repo_filter);
    script_repo_filter = (filter) ? strdup (filter) : NULL;

    script_buffer_set_localvar_filter ();
}

/*
 * Checks if a script is matching a filter string.
 *
 * Returns:
 *   1: script is matching filter string
 *   0: script does not match filter string
 */

int
script_repo_match_filter (struct t_script_repo *script)
{
    char **words, **tags;
    int num_words, num_tags, has_tag, match, i, j;

    if (!script_repo_filter || strcmp (script_repo_filter, "*") == 0)
        return 1;

    words = weechat_string_split (script_repo_filter, " ", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &num_words);
    tags = weechat_string_split ((script->tags) ? script->tags : "",
                                 ",",
                                 NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0,
                                 &num_tags);
    if (words)
    {
        for (i = 0; i < num_words; i++)
        {
            has_tag = 0;
            if (tags)
            {
                for (j = 0; j < num_tags; j++)
                {
                    if (weechat_strcasecmp (tags[j], words[i]) == 0)
                    {
                        has_tag = 1;
                        break;
                    }
                }
            }
            if (!has_tag)
            {
                match = 0;

                if (script->name_with_extension
                    && weechat_strcasestr (script->name_with_extension,
                                           words[i]))
                {
                    match = 1;
                }

                if (!match
                    && (weechat_strcasecmp (script_language[script->language],
                                            words[i]) == 0))
                {
                    match = 1;
                }

                if (!match
                    && (weechat_strcasecmp (script_extension[script->language],
                                            words[i]) == 0))
                {
                    match = 1;
                }

                if (!match && script->description
                    && weechat_strcasestr (script->description, words[i]))
                {
                    match = 1;
                }

                if (!match)
                {
                    weechat_string_free_split (words);
                    weechat_string_free_split (tags);
                    return 0;
                }
            }
        }
    }

    if (words)
        weechat_string_free_split (words);
    if (tags)
        weechat_string_free_split (tags);

    return 1;
}

/*
 * Filters scripts (search string in name/description/tags) and marks scripts
 * found as "displayed" (0 in displayed for non-matching scripts).
 */

void
script_repo_filter_scripts (const char *search)
{
    struct t_script_repo *ptr_script;

    script_repo_set_filter (search);

    script_repo_count_displayed = 0;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        ptr_script->displayed = (script_repo_match_filter (ptr_script));
        if (ptr_script->displayed)
            script_repo_count_displayed++;
    }

    script_buffer_refresh (1);
}

/*
 * Checks if repository file (plugins.xml.gz) exists.
 *
 * Returns:
 *   1: repository file exists
 *   0: repository file does not exist
 */

int
script_repo_file_exists ()
{
    char *filename;
    int rc;
    struct stat st;

    filename = script_config_get_xml_filename ();
    if (!filename)
        return 0;

    rc = 0;
    if (stat (filename, &st) == 0)
        rc = 1;

    free (filename);

    return rc;
}

/*
 * Checks if repository file (plugins.xml.gz) is up-to-date (file exists and it
 * not outdated).
 *
 * Returns:
 *   1: repository file is up-to-date
 *   0: repository file does not exist or is outdated
 *      (file has to be downloaded)
 */

int
script_repo_file_is_uptodate ()
{
    char *filename;
    struct stat st;
    int cache_expire;
    time_t current_time;

    cache_expire = weechat_config_integer (script_config_scripts_cache_expire);

    /* cache always expires? => NOT up-to-date */
    if (cache_expire == 0)
        return 0;

    filename = script_config_get_xml_filename ();

    /* filename not found? => NOT up-to-date */
    if (!filename)
        return 0;

    /* file does not exist? => NOT up-to-date */
    if (stat (filename, &st) == -1)
    {
        free (filename);
        return 0;
    }

    /* file is empty? => NOT up-to-date */
    if (st.st_size == 0)
    {
        free (filename);
        return 0;
    }

    /* cache never expires? => OK, up-to-date! */
    if (cache_expire < 0)
    {
        free (filename);
        return 1;
    }

    current_time = time (NULL);

    /* cache has expired? => NOT up-to-date */
    if (current_time > st.st_mtime + (cache_expire * 60))
    {
        free (filename);
        return 0;
    }

    /* OK, up-to-date! */
    free (filename);
    return 1;
}

/*
 * Reads scripts in repository file (plugins.xml.gz).
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
script_repo_file_read (int quiet)
{
    char *filename, *ptr_line, line[4096], *pos, *pos2, *pos3;
    char *name, *value1, *value2, *value3, *value, *error;
    char *info_locale, *locale, *locale_language, *version;
    const char *ptr_desc;
    gzFile file;
    struct t_script_repo *script;
    int version_number, version_ok, script_ok, length;
    struct tm tm_script;
    struct t_hashtable *descriptions;

    script_get_loaded_plugins ();
    script_get_scripts ();

    script_repo_remove_all ();

    if (!script_repo_max_length_field)
    {
        script_repo_max_length_field = weechat_hashtable_new (
            32,
            WEECHAT_HASHTABLE_STRING,
            WEECHAT_HASHTABLE_INTEGER,
            NULL, NULL);
    }
    else
        weechat_hashtable_remove_all (script_repo_max_length_field);

    version = weechat_info_get ("version", NULL);
    version_number = weechat_util_version_number (version);
    if (version)
        free (version);

    filename = script_config_get_xml_filename ();
    if (!filename)
    {
        weechat_printf (NULL, _("%s%s: error reading list of scripts"),
                        weechat_prefix ("error"),
                        SCRIPT_PLUGIN_NAME);
        return 0;
    }

    script = NULL;
    file = gzopen (filename, "r");
    free (filename);
    if (!file)
    {
        weechat_printf (NULL, _("%s%s: error reading list of scripts"),
                        weechat_prefix ("error"),
                        SCRIPT_PLUGIN_NAME);
        return 0;
    }

    /*
     * get locale and locale_languages
     * example: if LANG=fr_FR.UTF-8, result is:
     *   locale          = "fr_FR"
     *   locale_language = "fr"
     */
    locale = NULL;
    locale_language = NULL;
    info_locale = weechat_info_get ("locale", NULL);
    if (info_locale)
    {
        pos = strchr (info_locale, '.');
        if (pos)
            locale = weechat_strndup (info_locale, pos - info_locale);
        else
            locale = strdup (info_locale);
        free (info_locale);
    }
    if (locale)
    {
        pos = strchr (locale, '_');
        if (pos)
            locale_language = weechat_strndup (locale, pos - locale);
        else
            locale_language = strdup (locale);
    }

    descriptions = weechat_hashtable_new (32,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL, NULL);

    /* read plugins.xml.gz */
    while (!gzeof (file))
    {
        ptr_line = gzgets (file, line, sizeof (line) - 1);
        if (ptr_line)
        {
            if (strstr (ptr_line, "<plugin id="))
            {
                if (script)
                    script_repo_free (script);
                script = script_repo_alloc ();
                weechat_hashtable_remove_all (descriptions);
            }
            else if (strstr (ptr_line, "</plugin>"))
            {
                if (script)
                {
                    script_ok = 0;
                    if (script->name && (script->language >= 0))
                    {
                        version_ok = 1;
                        if (script->min_weechat)
                        {
                            if (weechat_util_version_number (script->min_weechat) > version_number)
                                version_ok = 0;
                        }
                        if (version_ok && script->max_weechat)
                        {
                            if (weechat_util_version_number (script->max_weechat) < version_number)
                                version_ok = 0;
                        }
                        if (version_ok)
                        {
                            ptr_desc = NULL;
                            if (weechat_config_boolean (script_config_look_translate_description))
                            {
                                /* try translated description (format "fr_FR") */
                                ptr_desc = weechat_hashtable_get (descriptions,
                                                                  locale);
                                if (!ptr_desc)
                                {
                                    /* try translated description (format "fr") */
                                    ptr_desc = weechat_hashtable_get (descriptions,
                                                                      locale_language);
                                }
                            }
                            if (!ptr_desc)
                            {
                                /* default description (English) */
                                ptr_desc = weechat_hashtable_get (descriptions,
                                                                  "en");
                            }
                            if (ptr_desc)
                            {
                                script->description = strdup (ptr_desc);
                                length = strlen (script->name) + 1 +
                                    strlen (script_extension[script->language]) + 1;
                                script->name_with_extension = malloc (length);
                                if (script->name_with_extension)
                                {
                                    snprintf (script->name_with_extension,
                                              length,
                                              "%s.%s",
                                              script->name,
                                              script_extension[script->language]);
                                }
                                script_repo_update_status (script);
                                script->displayed = (script_repo_match_filter (script));
                                script_repo_add (script);
                                script_ok = 1;
                            }
                        }
                    }
                    if (!script_ok)
                    {
                        script_repo_free (script);
                    }
                    script = NULL;
                }
            }
            else if (script)
            {
                pos = strchr (ptr_line, '<');
                if (pos)
                {
                    pos2 = strchr (pos + 1, '>');
                    if (pos2 && (pos2 > pos + 1))
                    {
                        pos3 = strstr (pos2 + 1, "</");
                        if (pos3 && (pos3 > pos2 + 1))
                        {
                            name = weechat_strndup (pos + 1, pos2 - pos - 1);
                            value1 = weechat_strndup (pos2 + 1, pos3 - pos2 - 1);
                            value2 = weechat_string_replace (value1, "&amp;", "&");
                            value3 = weechat_string_replace (value2, "&gt;", ">");
                            value = weechat_string_replace (value3, "&lt;", "<");
                            if (name && value)
                            {
                                if (strcmp (name, "name") == 0)
                                    script->name = strdup (value);
                                else if (strcmp (name, "language") == 0)
                                    script->language = script_language_search (value);
                                else if (strcmp (name, "author") == 0)
                                    script->author = strdup (value);
                                else if (strcmp (name, "mail") == 0)
                                    script->mail = strdup (value);
                                else if (strcmp (name, "version") == 0)
                                    script->version = strdup (value);
                                else if (strcmp (name, "license") == 0)
                                    script->license = strdup (value);
                                else if (strncmp (name, "desc_", 5) == 0)
                                {
                                    /*
                                     * store translated description in hashtable
                                     * (will be used later, by choosing
                                     * appropriate language according to locale)
                                     */
                                    weechat_hashtable_set (descriptions,
                                                           name + 5,
                                                           value);
                                }
                                else if (strcmp (name, "tags") == 0)
                                    script->tags = strdup (value);
                                else if (strcmp (name, "requirements") == 0)
                                    script->requirements = strdup (value);
                                else if (strcmp (name, "min_weechat") == 0)
                                    script->min_weechat = strdup (value);
                                else if (strcmp (name, "max_weechat") == 0)
                                    script->max_weechat = strdup (value);
                                else if (strcmp (name, "sha512sum") == 0)
                                    script->sha512sum = strdup (value);
                                else if (strcmp (name, "url") == 0)
                                    script->url = strdup (value);
                                else if (strcmp (name, "popularity") == 0)
                                {
                                    error = NULL;
                                    script->popularity = (int)strtol (value,
                                                                      &error,
                                                                      10);
                                    if (!error || error[0])
                                        script->popularity = 0;
                                }
                                else if (strcmp (name, "added") == 0)
                                {
                                    /*
                                     * initialize structure, because strptime
                                     * does not do it
                                     */
                                    memset (&tm_script, 0, sizeof (tm_script));
                                    error = strptime (value,
                                                      "%Y-%m-%d %H:%M:%S",
                                                      &tm_script);
                                    if (error && !error[0])
                                        script->date_added = mktime (&tm_script);
                                }
                                else if (strcmp (name, "updated") == 0)
                                {
                                    /*
                                     * initialize structure, because strptime
                                     * does not do it
                                     */
                                    memset (&tm_script, 0, sizeof (tm_script));
                                    error = strptime (value,
                                                      "%Y-%m-%d %H:%M:%S",
                                                      &tm_script);
                                    if (error && !error[0])
                                        script->date_updated = mktime (&tm_script);
                                }
                            }
                            if (name)
                                free (name);
                            if (value1)
                                free (value1);
                            if (value2)
                                free (value2);
                            if (value3)
                                free (value3);
                            if (value)
                                free (value);
                        }
                    }
                }
            }
        }
    }

    gzclose (file);

    if (scripts_repo && !quiet)
    {
        version = weechat_info_get ("version", NULL);
        weechat_printf (NULL,
                        _("%s: %d scripts for WeeChat %s"),
                        SCRIPT_PLUGIN_NAME, script_repo_count,
                        version);
        if (version)
            free (version);
    }

    if (!scripts_repo)
    {
        weechat_printf (NULL,
                        _("%s%s: list of scripts is empty (repository file "
                          "is broken, or download has failed)"),
                        weechat_prefix ("error"),
                        SCRIPT_PLUGIN_NAME);
    }

    if (script)
        script_repo_free (script);
    if (locale)
        free (locale);
    if (locale_language)
        free (locale_language);
    if (descriptions)
        weechat_hashtable_free (descriptions);

    return 1;
}

/*
 * Callback called when list of scripts is downloaded.
 */

int
script_repo_file_update_process_cb (const void *pointer, void *data,
                                    const char *command,
                                    int return_code, const char *out,
                                    const char *err)
{
    int quiet;

    /* make C compiler happy */
    (void) data;
    (void) command;
    (void) out;

    quiet = (pointer) ? 1 : 0;

    if (return_code >= 0)
    {
        if (err && err[0])
        {
            weechat_printf (NULL,
                            _("%s%s: error downloading list of scripts: %s"),
                            weechat_prefix ("error"),
                            SCRIPT_PLUGIN_NAME,
                            err);
            return WEECHAT_RC_OK;
        }

        if (script_repo_file_read (quiet) && scripts_repo)
        {
            if (script_buffer)
                script_buffer_refresh (1);
            if (!script_action_run_all ())
                script_buffer_refresh (1);
        }
        else
            script_buffer_refresh (1);
    }

    return WEECHAT_RC_OK;
}

/*
 * Updates repository file and reads it.
 *
 * Returns:
 *   0: error
 *   1: OK
 */

int
script_repo_file_update (int quiet)
{
    char *filename, *url;
    struct t_hashtable *options;

    if (!script_download_enabled (1))
        return 0;

    script_repo_remove_all ();

    filename = script_config_get_xml_filename ();
    if (!filename)
        return 0;

    options = weechat_hashtable_new (32,
                                     WEECHAT_HASHTABLE_STRING,
                                     WEECHAT_HASHTABLE_STRING,
                                     NULL, NULL);
    if (options)
    {
        url = script_build_download_url (
            weechat_config_string (script_config_scripts_url));
        if (url)
        {
            if (!quiet)
            {
                weechat_printf (NULL,
                                _("%s: downloading list of scripts..."),
                                SCRIPT_PLUGIN_NAME);
            }
            weechat_hashtable_set (options, "file_out", filename);
            weechat_hook_process_hashtable (
                url,
                options,
                weechat_config_integer (script_config_scripts_download_timeout) * 1000,
                &script_repo_file_update_process_cb,
                (quiet) ? (void *)1 : (void *)0,
                NULL);
            free (url);
        }
        weechat_hashtable_free (options);
    }

    free (filename);

    return 1;
}

/*
 * Returns hdata for script.
 */

struct t_hdata *
script_repo_hdata_script_cb (const void *pointer, void *data,
                             const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_script", "next_script",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_script_repo, name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, name_with_extension, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, language, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, author, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, mail, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, version, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, license, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, description, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, tags, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, requirements, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, min_weechat, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, max_weechat, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, sha512sum, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, url, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, popularity, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, date_added, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, date_updated, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, status, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, version_loaded, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, displayed, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, install_order, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_script_repo, prev_script, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_script_repo, next_script, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_LIST(scripts_repo, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        WEECHAT_HDATA_LIST(last_script_repo, 0);
    }
    return hdata;
}

/*
 * Adds a script in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
script_repo_add_to_infolist (struct t_infolist *infolist,
                             struct t_script_repo *script)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !script)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "name", script->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "name_with_extension", script->name_with_extension))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "language", script->language))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "author", script->author))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "mail", script->mail))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "version", script->version))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "license", script->license))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "description", script->description))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "tags", script->tags))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "requirements", script->requirements))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "min_weechat", script->min_weechat))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "max_weechat", script->max_weechat))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "sha512sum", script->sha512sum))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "url", script->url))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "popularity", script->popularity))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "date_added", script->date_added))
        return 0;
    if (!weechat_infolist_new_var_time (ptr_item, "date_updated", script->date_updated))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "status", script->status))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "version_loaded", script->version_loaded))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "displayed", script->displayed))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "install_order", script->install_order))
        return 0;

    return 1;
}

/*
 * Prints scripts in WeeChat log file (usually for crash dump).
 */

void
script_repo_print_log ()
{
    struct t_script_repo *ptr_script;

    for (ptr_script = scripts_repo; ptr_script;
         ptr_script = ptr_script->next_script)
    {
        weechat_log_printf ("");
        weechat_log_printf ("[script (addr:0x%lx)]", ptr_script);
        weechat_log_printf ("  name. . . . . . . . . : '%s'",  ptr_script->name);
        weechat_log_printf ("  name_with_extension . : '%s'",  ptr_script->name_with_extension);
        weechat_log_printf ("  language. . . . . . . : %d",    ptr_script->language);
        weechat_log_printf ("  author. . . . . . . . : '%s'",  ptr_script->author);
        weechat_log_printf ("  mail. . . . . . . . . : '%s'",  ptr_script->mail);
        weechat_log_printf ("  version . . . . . . . : '%s'",  ptr_script->version);
        weechat_log_printf ("  license . . . . . . . : '%s'",  ptr_script->license);
        weechat_log_printf ("  description . . . . . : '%s'",  ptr_script->description);
        weechat_log_printf ("  tags. . . . . . . . . : '%s'",  ptr_script->tags);
        weechat_log_printf ("  requirements. . . . . : '%s'",  ptr_script->requirements);
        weechat_log_printf ("  min_weechat . . . . . : '%s'",  ptr_script->min_weechat);
        weechat_log_printf ("  max_weechat . . . . . : '%s'",  ptr_script->max_weechat);
        weechat_log_printf ("  sha512sum . . . . . . : '%s'",  ptr_script->sha512sum);
        weechat_log_printf ("  url . . . . . . . . . : '%s'",  ptr_script->url);
        weechat_log_printf ("  popularity. . . . . . : %d",    ptr_script->popularity);
        weechat_log_printf ("  date_added. . . . . . : %lld",  (long long)ptr_script->date_added);
        weechat_log_printf ("  date_updated. . . . . : %lld",  (long long)ptr_script->date_updated);
        weechat_log_printf ("  status. . . . . . . . : %d (%s%s%s%s%s )",
                            ptr_script->status,
                            (ptr_script->status & SCRIPT_STATUS_INSTALLED) ? " installed": "",
                            (ptr_script->status & SCRIPT_STATUS_AUTOLOADED) ? " autoloaded": "",
                            (ptr_script->status & SCRIPT_STATUS_HELD) ? " held": "",
                            (ptr_script->status & SCRIPT_STATUS_RUNNING) ? " running": "",
                            (ptr_script->status & SCRIPT_STATUS_NEW_VERSION) ? " new_version": "");
        weechat_log_printf ("  version_loaded. . . . : '%s'",  ptr_script->version_loaded);
        weechat_log_printf ("  displayed . . . . . . : %d",    ptr_script->displayed);
        weechat_log_printf ("  install_order . . . . : %d",    ptr_script->install_order);
        weechat_log_printf ("  prev_script . . . . . : 0x%lx", ptr_script->prev_script);
        weechat_log_printf ("  next_script . . . . . : 0x%lx", ptr_script->next_script);
    }
}
