/*
 * script-config.c - script configuration options (file script.conf)
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-config.h"
#include "script-buffer.h"
#include "script-repo.h"


struct t_config_file *script_config_file = NULL;

/* sections */

struct t_config_section *script_config_section_look = NULL;
struct t_config_section *script_config_section_color = NULL;
struct t_config_section *script_config_section_scripts = NULL;

/* script config, look section */

struct t_config_option *script_config_look_columns = NULL;
struct t_config_option *script_config_look_diff_color = NULL;
struct t_config_option *script_config_look_diff_command = NULL;
struct t_config_option *script_config_look_display_source = NULL;
struct t_config_option *script_config_look_quiet_actions = NULL;
struct t_config_option *script_config_look_sort = NULL;
struct t_config_option *script_config_look_translate_description = NULL;
struct t_config_option *script_config_look_use_keys = NULL;

/* script config, color section */

struct t_config_option *script_config_color_status_autoloaded = NULL;
struct t_config_option *script_config_color_status_held = NULL;
struct t_config_option *script_config_color_status_installed = NULL;
struct t_config_option *script_config_color_status_obsolete = NULL;
struct t_config_option *script_config_color_status_popular = NULL;
struct t_config_option *script_config_color_status_running = NULL;
struct t_config_option *script_config_color_status_unknown = NULL;
struct t_config_option *script_config_color_text = NULL;
struct t_config_option *script_config_color_text_bg = NULL;
struct t_config_option *script_config_color_text_bg_selected = NULL;
struct t_config_option *script_config_color_text_date = NULL;
struct t_config_option *script_config_color_text_date_selected = NULL;
struct t_config_option *script_config_color_text_delimiters = NULL;
struct t_config_option *script_config_color_text_description = NULL;
struct t_config_option *script_config_color_text_description_selected = NULL;
struct t_config_option *script_config_color_text_extension = NULL;
struct t_config_option *script_config_color_text_extension_selected = NULL;
struct t_config_option *script_config_color_text_name = NULL;
struct t_config_option *script_config_color_text_name_selected = NULL;
struct t_config_option *script_config_color_text_selected = NULL;
struct t_config_option *script_config_color_text_tags = NULL;
struct t_config_option *script_config_color_text_tags_selected = NULL;
struct t_config_option *script_config_color_text_version = NULL;
struct t_config_option *script_config_color_text_version_loaded = NULL;
struct t_config_option *script_config_color_text_version_loaded_selected = NULL;
struct t_config_option *script_config_color_text_version_selected = NULL;

/* script config, scripts section */

struct t_config_option *script_config_scripts_autoload = NULL;
struct t_config_option *script_config_scripts_cache_expire = NULL;
struct t_config_option *script_config_scripts_download_enabled = NULL;
struct t_config_option *script_config_scripts_download_timeout = NULL;
struct t_config_option *script_config_scripts_hold = NULL;
struct t_config_option *script_config_scripts_path = NULL;
struct t_config_option *script_config_scripts_url = NULL;


/*
 * Gets the diff command (option "script.look.diff_command").
 *
 * If option is "auto", try to find git, and fallbacks on "diff" if not found.
 *
 * Returns NULL if no diff command is set.
 */

const char *
script_config_get_diff_command (void)
{
    const char *diff_command;
    char *dir_separator;
    static char result[64];
    struct stat st;
    char *path, **paths, bin[4096];
    int num_paths, i, rc;

    diff_command = weechat_config_string (script_config_look_diff_command);
    if (!diff_command || !diff_command[0])
        return NULL;

    if (strcmp (diff_command, "auto") == 0)
    {
        dir_separator = weechat_info_get ("dir_separator", "");
        path = getenv ("PATH");
        result[0] = '\0';
        if (dir_separator && path)
        {
            paths = weechat_string_split (path, ":", NULL,
                                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                          0, &num_paths);
            if (paths)
            {
                for (i = 0; i < num_paths; i++)
                {
                    snprintf (bin, sizeof (bin), "%s%s%s",
                              paths[i], dir_separator, "git");
                    rc = stat (bin, &st);
                    if ((rc == 0) && (S_ISREG(st.st_mode)))
                    {
                        snprintf (result, sizeof (result),
                                  "git diff --no-index");
                        break;
                    }
                }
                weechat_string_free_split (paths);
            }
        }
        free (dir_separator);
        if (!result[0])
            snprintf (result, sizeof (result), "diff");
        return result;
    }

    return diff_command;
}

/*
 * Gets filename with list of scripts.
 *
 * Note: result must be freed after use.
 */

char *
script_config_get_xml_filename (void)
{
    char *path, *filename;
    struct t_hashtable *options;

    options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (options)
        weechat_hashtable_set (options, "directory", "cache");
    path = weechat_string_eval_path_home (
        weechat_config_string (script_config_scripts_path), NULL, NULL, options);
    weechat_hashtable_free (options);
    weechat_asprintf (&filename, "%s/plugins.xml.gz", path);
    free (path);
    return filename;
}

/*
 * Gets filename for a script to download.
 * If suffix is not NULL, it is added to filename.
 *
 * Note: result must be freed after use.
 */

char *
script_config_get_script_download_filename (struct t_script_repo *script,
                                            const char *suffix)
{
    char *path, *filename;
    struct t_hashtable *options;

    options = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (options)
        weechat_hashtable_set (options, "directory", "cache");
    path = weechat_string_eval_path_home (
        weechat_config_string (script_config_scripts_path), NULL, NULL, options);
    weechat_hashtable_free (options);
    weechat_asprintf (&filename,
                      "%s/%s%s",
                      path,
                      script->name_with_extension,
                      (suffix) ? suffix : "");
    free (path);
    return filename;
}

/*
 * Callback for changes on options that require a refresh of script buffer.
 */

void
script_config_refresh_cb (const void *pointer, void *data,
                          struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (script_buffer)
        script_buffer_refresh (0);
}

/*
 * Callback for changes on options that require a reload of list of scripts
 * (file plugins.xml.gz).
 */

void
script_config_reload_scripts_cb (const void *pointer, void *data,
                                 struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (scripts_repo)
    {
        script_repo_remove_all ();
        script_repo_file_read (1);
        script_buffer_refresh (1);
    }
}

/*
 * Callback for changes on option "script.look.use_keys".
 */

void
script_config_change_use_keys_cb (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    if (script_buffer)
        script_buffer_set_keys (NULL);
}

/*
 * Callback for changes on option "script.scripts.hold".
 */

void
script_config_change_hold_cb (const void *pointer, void *data,
                              struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    script_repo_update_status_all ();
    if (script_buffer)
        script_buffer_refresh (0);
}

/*
 * Holds a script.
 *
 * Note: the option is changed, but the status "held" in script is NOT updated
 * by this function.
 */

void
script_config_hold (const char *name_with_extension)
{
    char **items, **hold;
    int num_items, i;

    hold = weechat_string_dyn_alloc (256);
    if (!hold)
        return;

    items = weechat_string_split (
        weechat_config_string (script_config_scripts_hold),
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
            if (strcmp (items[i], name_with_extension) != 0)
            {
                if ((*hold)[0])
                    weechat_string_dyn_concat (hold, ",", -1);
                weechat_string_dyn_concat (hold, items[i], -1);
            }
        }
        weechat_string_free_split (items);
    }
    if ((*hold)[0])
        weechat_string_dyn_concat (hold, ",", -1);
    weechat_string_dyn_concat (hold, name_with_extension, -1);

    weechat_config_option_set (script_config_scripts_hold, *hold, 0);

    weechat_string_dyn_free (hold, 1);
}

/*
 * Unholds a script.
 *
 * Note: the option is changed, but the status "held" in script is NOT updated
 * by this function.
 */

void
script_config_unhold (const char *name_with_extension)
{
    char **items, **hold;
    int num_items, i;

    hold = weechat_string_dyn_alloc (256);
    if (!hold)
        return;

    items = weechat_string_split (
        weechat_config_string (script_config_scripts_hold),
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
            if (strcmp (items[i], name_with_extension) != 0)
            {
                if ((*hold)[0])
                    weechat_string_dyn_concat (hold, ",", -1);
                weechat_string_dyn_concat (hold, items[i], -1);
            }
        }
        weechat_string_free_split (items);
    }

    weechat_config_option_set (script_config_scripts_hold, *hold, 0);

    weechat_string_dyn_free (hold, 1);
}

/*
 * Reloads script configuration file.
 */

int
script_config_reload (const void *pointer, void *data,
                      struct t_config_file *config_file)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;

    return weechat_config_reload (config_file);
}

/*
 * Initializes script configuration file.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
script_config_init (void)
{
    script_config_file = weechat_config_new (
        SCRIPT_CONFIG_PRIO_NAME,
        &script_config_reload, NULL, NULL);
    if (!script_config_file)
        return 0;

    /* look */
    script_config_section_look = weechat_config_new_section (
        script_config_file, "look",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (script_config_section_look)
    {
        script_config_look_columns = weechat_config_new_option (
            script_config_file, script_config_section_look,
            "columns", "string",
            N_("format of columns displayed in script buffer: following column "
               "identifiers are replaced by their values: %a=author, "
               "%d=description, %D=date added, %e=extension, %l=language, "
               "%L=license, %n=name with extension, %N=name, %r=requirements, "
               "%s=status, %t=tags, %u=date updated, %v=version, "
               "%V=version loaded, %w=min_weechat, %W=max_weechat)"),
            NULL, 0, 0, "%s %n %V %v %u | %d | %t", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_look_diff_color = weechat_config_new_option (
            script_config_file, script_config_section_look,
            "diff_color", "boolean",
            N_("colorize output of diff"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        script_config_look_diff_command = weechat_config_new_option (
            script_config_file, script_config_section_look,
            "diff_command", "string",
            N_("command used to show differences between script installed and "
               "the new version in repository (\"auto\" = auto detect diff "
               "command (git or diff), empty value = disable diff, other "
               "string = name of command, for example \"diff\")"),
            NULL, 0, 0, "auto", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        script_config_look_display_source = weechat_config_new_option (
            script_config_file, script_config_section_look,
            "display_source", "boolean",
            N_("display source code of script on buffer with detail on a script "
               "(script is downloaded in a temporary file when detail on script "
               "is displayed)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        script_config_look_quiet_actions = weechat_config_new_option (
            script_config_file, script_config_section_look,
            "quiet_actions", "boolean",
            N_("quiet actions on script buffer: do not display messages on core "
               "buffer when scripts are installed/removed/loaded/unloaded (only "
               "errors are displayed)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        script_config_look_sort = weechat_config_new_option (
            script_config_file, script_config_section_look,
            "sort", "string",
            N_("default sort keys for scripts: comma-separated list of "
               "identifiers: a=author, A=autoloaded, d=date added, e=extension, "
               "i=installed, l=language, n=name, o=obsolete, p=popularity, "
               "r=running, u=date updated; char \"-\" can be used before "
               "identifier to reverse order; example: \"i,u\": installed "
               "scripts first, sorted by update date"),
            NULL, 0, 0, "i,p,n", NULL, 0,
            NULL, NULL, NULL,
            &script_config_reload_scripts_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_look_translate_description = weechat_config_new_option (
            script_config_file, script_config_section_look,
            "translate_description", "boolean",
            N_("translate description of scripts (if translation is available "
               "in your language, otherwise English version is used)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &script_config_reload_scripts_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_look_use_keys = weechat_config_new_option (
            script_config_file, script_config_section_look,
            "use_keys", "boolean",
            N_("use keys alt+X in script buffer to do actions on scripts "
               "(alt+i = install, alt+r = remove, ...); if disabled, only the "
               "input is allowed: i, r, ..."),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL,
            &script_config_change_use_keys_cb, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* color */
    script_config_section_color = weechat_config_new_section (
        script_config_file, "color",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (script_config_section_color)
    {
        script_config_color_status_autoloaded = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "status_autoloaded", "color",
            N_("color for status \"autoloaded\" (\"a\")"),
            NULL, 0, 0, "39", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_status_held = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "status_held", "color",
            N_("color for status \"held\" (\"H\")"),
            NULL, 0, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_status_installed = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "status_installed", "color",
            N_("color for status \"installed\" (\"i\")"),
            NULL, 0, 0, "lightcyan", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_status_obsolete = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "status_obsolete", "color",
            N_("color for status \"obsolete\" (\"N\")"),
            NULL, 0, 0, "lightmagenta", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_status_popular = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "status_popular", "color",
            N_("color for status \"popular\" (\"*\")"),
            NULL, 0, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_status_running = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "status_running", "color",
            N_("color for status \"running\" (\"r\")"),
            NULL, 0, 0, "lightgreen", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_status_unknown = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "status_unknown", "color",
            N_("color for status \"unknown\" (\"?\")"),
            NULL, 0, 0, "lightred", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text", "color",
            N_("text color in script buffer"),
            NULL, 0, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_bg = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_bg", "color",
            N_("background color in script buffer"),
            NULL, 0, 0, "default", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_bg_selected = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_bg_selected", "color",
            N_("background color for selected line in script buffer"),
            NULL, 0, 0, "24", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_date = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_date", "color",
            N_("text color of dates in script buffer"),
            NULL, 0, 0, "65", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_date_selected = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_date_selected", "color",
            N_("text color of dates for selected line in script buffer"),
            NULL, 0, 0, "50", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_delimiters = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_delimiters", "color",
            N_("text color of delimiters in script buffer"),
            NULL, 0, 0, "240", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_description = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_description", "color",
            N_("text color of description in script buffer"),
            NULL, 0, 0, "249", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_description_selected = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_description_selected", "color",
            N_("text color of description for selected line in script buffer"),
            NULL, 0, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_extension = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_extension", "color",
            N_("text color of extension in script buffer"),
            NULL, 0, 0, "242", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_extension_selected = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_extension_selected", "color",
            N_("text color of extension for selected line in script buffer"),
            NULL, 0, 0, "248", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_name = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_name", "color",
            N_("text color of script name in script buffer"),
            NULL, 0, 0, "73", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_name_selected = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_name_selected", "color",
            N_("text color of script name for selected line in script buffer"),
            NULL, 0, 0, "51", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_selected = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_selected", "color",
            N_("text color for selected line in script buffer"),
            NULL, 0, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_tags = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_tags", "color",
            N_("text color of tags in script buffer"),
            NULL, 0, 0, "brown", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_tags_selected = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_tags_selected", "color",
            N_("text color of tags for selected line in script buffer"),
            NULL, 0, 0, "yellow", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_version = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_version", "color",
            N_("text color of version in script buffer"),
            NULL, 0, 0, "100", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_version_loaded = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_version_loaded", "color",
            N_("text color of version loaded in script buffer"),
            NULL, 0, 0, "246", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_version_loaded_selected = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_version_loaded_selected", "color",
            N_("text color of version loaded for selected line in script buffer"),
            NULL, 0, 0, "white", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_color_text_version_selected = weechat_config_new_option (
            script_config_file, script_config_section_color,
            "text_version_selected", "color",
            N_("text color of version for selected line in script buffer"),
            NULL, 0, 0, "228", NULL, 0,
            NULL, NULL, NULL,
            &script_config_refresh_cb, NULL, NULL,
            NULL, NULL, NULL);
    }

    /* scripts */
    script_config_section_scripts = weechat_config_new_section (
        script_config_file, "scripts",
        0, 0,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL,
        NULL, NULL, NULL);
    if (script_config_section_scripts)
    {
        script_config_scripts_autoload = weechat_config_new_option (
            script_config_file, script_config_section_scripts,
            "autoload", "boolean",
            N_("autoload scripts installed (make a link in \"autoload\" directory "
               "to script in parent directory)"),
            NULL, 0, 0, "on", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        script_config_scripts_cache_expire = weechat_config_new_option (
            script_config_file, script_config_section_scripts,
            "cache_expire", "integer",
            N_("local cache expiration time, in minutes (-1 = never expires, "
               "0 = always expire)"),
            NULL, -1, 525600, "1440", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        script_config_scripts_download_enabled = weechat_config_new_option (
            script_config_file, script_config_section_scripts,
            "download_enabled", "boolean",
            N_("enable download of files from the scripts repository when the "
               "/script command is used (list of scripts and scripts "
               "themselves); the list of scripts is downloaded from the URL "
               "specified in the option script.scripts.url; WeeChat will "
               "sometimes download again the list of scripts when you use the "
               "/script command, even if you don't install a script"),
            NULL, 0, 0, "off", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        script_config_scripts_download_timeout = weechat_config_new_option (
            script_config_file, script_config_section_scripts,
            "download_timeout", "integer",
            N_("timeout (in seconds) for download of scripts and list of "
               "scripts"),
            NULL, 1, 3600, "30", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        script_config_scripts_hold = weechat_config_new_option (
            script_config_file, script_config_section_scripts,
            "hold", "string",
            N_("scripts to \"hold\": comma-separated list of scripts which "
               "will never been upgraded and cannot be removed, for example: "
               "\"go.py,urlserver.py\""),
            NULL, 0, 0, "", NULL, 0,
            NULL, NULL, NULL,
            &script_config_change_hold_cb, NULL, NULL,
            NULL, NULL, NULL);
        script_config_scripts_path = weechat_config_new_option (
            script_config_file, script_config_section_scripts,
            "path", "string",
            N_("local cache directory for scripts "
               "(path is evaluated, see function string_eval_path_home in "
               "plugin API reference)"),
            NULL, 0, 0, "${weechat_cache_dir}/script", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        script_config_scripts_url = weechat_config_new_option (
            script_config_file, script_config_section_scripts,
            "url", "string",
            N_("URL for file with list of scripts"),
            NULL, 0, 0, "https://weechat.org/files/plugins.xml.gz", NULL, 0,
            NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    }

    return 1;
}

/*
 * Reads script configuration file.
 */

int
script_config_read (void)
{
    return weechat_config_read (script_config_file);
}

/*
 * Writes script configuration file.
 */

int
script_config_write (void)
{
    return weechat_config_write (script_config_file);
}

/*
 * Frees script configuration file.
 */

void
script_config_free (void)
{
    weechat_config_free (script_config_file);
    script_config_file = NULL;
}
