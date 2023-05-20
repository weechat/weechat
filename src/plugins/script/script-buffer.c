/*
 * script-buffer.c - display scripts on script buffer
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <libgen.h>

#include "../weechat-plugin.h"
#include "script.h"
#include "script-buffer.h"
#include "script-config.h"
#include "script-repo.h"


struct t_gui_buffer *script_buffer = NULL;
int script_buffer_selected_line = 0;
struct t_script_repo *script_buffer_detail_script = NULL;
int script_buffer_detail_script_last_line = 0;
int script_buffer_detail_script_line_diff = -1;


/*
 * Displays a line with script.
 */

void
script_buffer_display_line_script (int line, struct t_script_repo *script)
{
    char str_line[16384], str_item[1024], str_color_name[256], str_color[32];
    char str_format[256], str_date[64], str_key[2], utf_char[16], *tags;
    const char *columns, *ptr_col;
    int char_size, *ptr_max_length, max_length, num_spaces, unknown;
    struct tm *tm;

    snprintf (str_color_name, sizeof (str_color_name),
              "%s,%s",
              (line == script_buffer_selected_line) ?
              weechat_config_string (script_config_color_text_selected) :
              weechat_config_string (script_config_color_text),
              (line == script_buffer_selected_line) ?
              weechat_config_string (script_config_color_text_bg_selected) :
              weechat_config_string (script_config_color_text_bg));
    snprintf (str_color, sizeof (str_color),
              "%s", weechat_color (str_color_name));

    columns = weechat_config_string (script_config_look_columns);
    ptr_col = columns;

    str_line[0] = '\0';
    while (ptr_col[0])
    {
        unknown = 0;
        str_item[0] = '\0';
        num_spaces = 0;
        char_size = weechat_utf8_char_size (ptr_col);
        memcpy (utf_char, ptr_col, char_size);
        utf_char[char_size] = '\0';
        if (utf_char[0] == '%')
        {
            ptr_col += char_size;
            char_size = weechat_utf8_char_size (ptr_col);
            memcpy (utf_char, ptr_col, char_size);
            utf_char[char_size] = '\0';

            str_key[0] = ptr_col[0];
            str_key[1] = '\0';
            ptr_max_length = weechat_hashtable_get (script_repo_max_length_field,
                                                    str_key);
            max_length = (ptr_max_length) ? *ptr_max_length : 0;
            num_spaces = max_length;

            switch (utf_char[0])
            {
                case 'a': /* author */
                    if (script->author)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->author);
                        snprintf (str_item, sizeof (str_item),
                                  "%s", script->author);
                    }
                    break;
                case 'd': /* description */
                    if (script->description)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->description);
                        snprintf (str_item, sizeof (str_item),
                                  "%s%s",
                                  weechat_color (
                                      weechat_config_string (
                                          (line == script_buffer_selected_line) ?
                                          script_config_color_text_description_selected :
                                          script_config_color_text_description)),
                                  script->description);
                    }
                    break;
                case 'D': /* date added */
                    if (script->date_added > 0)
                    {
                        tm = localtime (&script->date_added);
                        if (strftime (str_date, sizeof (str_date),
                                      "%Y-%m-%d", tm) == 0)
                            str_date[0] = '\0';
                        snprintf (str_item, sizeof (str_item),
                                  "%s%s",
                                  weechat_color (
                                      weechat_config_string (
                                          (line == script_buffer_selected_line) ?
                                          script_config_color_text_date_selected :
                                          script_config_color_text_date)),
                                  str_date);
                    }
                    else
                        num_spaces = 10;
                    break;
                case 'e': /* file extension */
                    if (script->language >= 0)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script_extension[script->language]);
                        snprintf (str_item, sizeof (str_item),
                                  "%s%s",
                                  weechat_color (
                                      weechat_config_string (
                                          (line == script_buffer_selected_line) ?
                                          script_config_color_text_extension_selected :
                                          script_config_color_text_extension)),
                                  script_extension[script->language]);
                    }
                    break;
                case 'l': /* language */
                    if (script->language >= 0)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script_language[script->language]);
                        snprintf (str_item, sizeof (str_item),
                                  "%s", script_language[script->language]);
                    }
                    break;
                case 'L': /* license */
                    if (script->license)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->license);
                        snprintf (str_item, sizeof (str_item),
                                  "%s", script->license);
                    }
                    break;
                case 'n': /* name + extension */
                    if (script->name_with_extension)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->name_with_extension);
                        snprintf (str_item, sizeof (str_item),
                                  "%s%s%s.%s",
                                  weechat_color (
                                      weechat_config_string (
                                          (line == script_buffer_selected_line) ?
                                          script_config_color_text_name_selected :
                                          script_config_color_text_name)),
                                  script->name,
                                  weechat_color (
                                      weechat_config_string (
                                          (line == script_buffer_selected_line) ?
                                          script_config_color_text_extension_selected :
                                          script_config_color_text_extension)),
                                  script_extension[script->language]);
                    }
                    break;
                case 'N': /* name (without extension) */
                    if (script->name)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->name);
                        snprintf (str_item, sizeof (str_item),
                                  "%s%s",
                                  weechat_color (
                                      weechat_config_string (
                                          (line == script_buffer_selected_line) ?
                                          script_config_color_text_name_selected :
                                          script_config_color_text_name)),
                                  script->name);
                    }
                    break;
                case 'r': /* requirements */
                    if (script->requirements)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->requirements);
                        snprintf (str_item, sizeof (str_item),
                                  "%s", script->requirements);
                    }
                    break;
                case 's': /* status */
                    snprintf (str_item, sizeof (str_item),
                              "%s",
                              script_repo_get_status_for_display (script,
                                                                  "*iaHrN", 0));
                    break;
                case 't': /* tags */
                    if (script->tags)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->tags);
                        tags = weechat_string_replace (script->tags, ",", " ");
                        if (tags)
                        {
                            snprintf (str_item, sizeof (str_item),
                                      "%s%s",
                                      weechat_color (
                                          weechat_config_string (
                                              (line == script_buffer_selected_line) ?
                                              script_config_color_text_tags_selected :
                                              script_config_color_text_tags)),
                                      tags);
                            free (tags);
                        }
                    }
                    break;
                case 'u': /* date updated */
                    if (script->date_updated > 0)
                    {
                        tm = localtime (&script->date_updated);
                        if (strftime (str_date, sizeof (str_date),
                                      "%Y-%m-%d", tm) == 0)
                            str_date[0] = '\0';
                        snprintf (str_item, sizeof (str_item),
                                  "%s%s",
                                  weechat_color (
                                      weechat_config_string (
                                          (line == script_buffer_selected_line) ?
                                          script_config_color_text_date_selected :
                                          script_config_color_text_date)),
                                  str_date);
                    }
                    else
                        num_spaces = 10;
                    break;
                case 'v': /* version */
                    if (script->version)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->version);
                        snprintf (str_item, sizeof (str_item),
                                  "%s%s",
                                  weechat_color (
                                      weechat_config_string (
                                          (line == script_buffer_selected_line) ?
                                          script_config_color_text_version_selected :
                                          script_config_color_text_version)),
                                  script->version);
                    }
                    break;
                case 'V': /* version loaded */
                    if (script->version_loaded)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->version_loaded);
                        snprintf (str_item, sizeof (str_item),
                                  "%s%s",
                                  weechat_color (
                                      weechat_config_string (
                                          (line == script_buffer_selected_line) ?
                                          script_config_color_text_version_loaded_selected :
                                          script_config_color_text_version_loaded)),
                                  script->version_loaded);
                    }
                    break;
                case 'w': /* min_weechat */
                    if (script->min_weechat)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->min_weechat);
                        snprintf (str_item, sizeof (str_item),
                                  "%s", script->min_weechat);
                    }
                    break;
                case 'W': /* max_weechat */
                    if (script->max_weechat)
                    {
                        num_spaces = max_length - weechat_utf8_strlen_screen (script->max_weechat);
                        snprintf (str_item, sizeof (str_item),
                                  "%s", script->max_weechat);
                    }
                    break;
                case '%': /* "%%" will display a single "%" */
                    snprintf (str_item, sizeof (str_item),
                              "%s%%",
                              weechat_color (weechat_config_string (script_config_color_text_delimiters)));
                    break;
                default:
                    unknown = 1;
                    break;
            }
        }
        else
        {
            snprintf (str_item, sizeof (str_item),
                      "%s%s",
                      weechat_color (weechat_config_string (script_config_color_text_delimiters)),
                      utf_char);
        }
        if (!unknown)
        {
            if (str_item[0])
            {
                strcat (str_line, str_color);
                strcat (str_line, str_item);
            }
            if (num_spaces > 0)
            {
                snprintf (str_format, sizeof (str_format),
                          "%%-%ds",
                          num_spaces);
                snprintf (str_item, sizeof (str_item),
                          str_format, " ");
                strcat (str_line, str_item);
            }
        }
        ptr_col += char_size;
    }

    weechat_printf_y (script_buffer, line, "%s", str_line);
}

/*
 * Gets header of a line for detail of script.
 *
 * Returns a string aligned on the right.
 */

const char *
script_buffer_detail_label (const char *text, int max_length)
{
    char str_format[128];
    static char result[1024];
    int num_spaces;

    num_spaces = max_length - weechat_utf8_strlen_screen (text);
    snprintf (str_format, sizeof (str_format),
              "%%-%ds%%s", num_spaces);
    snprintf (result, sizeof (result),
              str_format,
              (num_spaces > 0) ? " " : "",
              text);

    return result;
}

/*
 * Gets pointer to a script (to the script managed by the appropriate plugin,
 * for example python).
 */

struct t_plugin_script *
script_buffer_get_script_pointer (struct t_script_repo *script,
                                  struct t_hdata *hdata_script)
{
    char *filename, *ptr_base_name;
    const char *ptr_filename;
    void *ptr_script;

    ptr_script = weechat_hdata_get_list (hdata_script, "scripts");
    while (ptr_script)
    {
        ptr_filename = weechat_hdata_string (hdata_script,
                                             ptr_script, "filename");
        if (ptr_filename)
        {
            filename = strdup (ptr_filename);
            if (filename)
            {
                ptr_base_name = basename (filename);
                if (strcmp (ptr_base_name, script->name_with_extension) == 0)
                {
                    free (filename);
                    return ptr_script;
                }
                free (filename);
            }
        }
        ptr_script = weechat_hdata_move (hdata_script, ptr_script, 1);
    }

    /* script not found */
    return NULL;
}

/*
 * Gets a list with usage of the script (commands, config options...).
 */

struct t_weelist *
script_buffer_get_script_usage (struct t_script_repo *script)
{
    struct t_weelist *list;
    char hdata_name[128], str_option[256], str_info[1024];
    int config_files;
    struct t_hdata *hdata_script, *hdata_config, *hdata_bar_item;
    void *ptr_script, *callback_pointer;
    struct t_config_file *ptr_config;
    struct t_gui_bar_item *ptr_bar_item;
    struct t_infolist *infolist;

    config_files = 0;

    snprintf (hdata_name, sizeof (hdata_name),
              "%s_script", script_language[script->language]);
    hdata_script = weechat_hdata_get (hdata_name);
    if (!hdata_script)
        return NULL;

    ptr_script = script_buffer_get_script_pointer (script, hdata_script);
    if (!ptr_script)
        return NULL;

    list = weechat_list_new ();

    /* get configuration files created by the script */
    hdata_config = weechat_hdata_get ("config_file");
    ptr_config = weechat_hdata_get_list (hdata_config, "config_files");
    while (ptr_config)
    {
        callback_pointer = weechat_hdata_pointer (
            hdata_config, ptr_config, "callback_reload_pointer");
        if (callback_pointer == ptr_script)
        {
            snprintf (str_info, sizeof (str_info),
                      _("configuration file \"%s\" (options %s.*)"),
                      weechat_hdata_string (hdata_config, ptr_config,
                                            "filename"),
                      weechat_hdata_string (hdata_config, ptr_config,
                                            "name"));
            weechat_list_add (list, str_info, WEECHAT_LIST_POS_END, NULL);
            config_files++;
        }
        ptr_config = weechat_hdata_move (hdata_config, ptr_config, 1);
    }

    /* get the commands created by the script */
    infolist = weechat_infolist_get ("hook", NULL, "command");
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            callback_pointer = weechat_infolist_pointer (infolist,
                                                         "callback_pointer");
            if (callback_pointer == ptr_script)
            {
                snprintf (str_info, sizeof (str_info),
                          _("command /%s"),
                          weechat_infolist_string (infolist,
                                                   "command"));
                weechat_list_add (list, str_info, WEECHAT_LIST_POS_END, NULL);
            }
        }
        weechat_infolist_free (infolist);
    }

    /* get the completions created by the script */
    infolist = weechat_infolist_get ("hook", NULL, "completion");
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            callback_pointer = weechat_infolist_pointer (infolist,
                                                         "callback_pointer");
            if (callback_pointer == ptr_script)
            {
                snprintf (str_info, sizeof (str_info),
                          _("completion %%(%s)"),
                          weechat_infolist_string (infolist,
                                                   "completion_item"));
                weechat_list_add (list, str_info, WEECHAT_LIST_POS_END, NULL);
            }
        }
        weechat_infolist_free (infolist);
    }

    /* get the infos created by the script */
    infolist = weechat_infolist_get ("hook", NULL, "info");
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            callback_pointer = weechat_infolist_pointer (infolist,
                                                         "callback_pointer");
            if (callback_pointer == ptr_script)
            {
                snprintf (str_info, sizeof (str_info),
                          "info \"%s\"",
                          weechat_infolist_string (infolist,
                                                   "info_name"));
                weechat_list_add (list, str_info, WEECHAT_LIST_POS_END, NULL);
            }
        }
        weechat_infolist_free (infolist);
    }

    /* get the infos (hashtable) created by the script */
    infolist = weechat_infolist_get ("hook", NULL, "info_hashtable");
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            callback_pointer = weechat_infolist_pointer (infolist,
                                                         "callback_pointer");
            if (callback_pointer == ptr_script)
            {
                snprintf (str_info, sizeof (str_info),
                          "info_hashtable \"%s\"",
                          weechat_infolist_string (infolist,
                                                   "info_name"));
                weechat_list_add (list, str_info, WEECHAT_LIST_POS_END, NULL);
            }
        }
        weechat_infolist_free (infolist);
    }

    /* get the infolists created by the script */
    infolist = weechat_infolist_get ("hook", NULL, "infolist");
    if (infolist)
    {
        while (weechat_infolist_next (infolist))
        {
            callback_pointer = weechat_infolist_pointer (infolist,
                                                         "callback_pointer");
            if (callback_pointer == ptr_script)
            {
                snprintf (str_info, sizeof (str_info),
                          "infolist \"%s\"",
                          weechat_infolist_string (infolist,
                                                   "infolist_name"));
                weechat_list_add (list, str_info, WEECHAT_LIST_POS_END, NULL);
            }
        }
        weechat_infolist_free (infolist);
    }

    /* get the bar items created by the script */
    hdata_bar_item = weechat_hdata_get ("bar_item");
    ptr_bar_item = weechat_hdata_get_list (hdata_bar_item, "gui_bar_items");
    while (ptr_bar_item)
    {
        callback_pointer = weechat_hdata_pointer (hdata_bar_item, ptr_bar_item,
                                                  "build_callback_pointer");
        if (callback_pointer == ptr_script)
        {
            snprintf (str_info, sizeof (str_info),
                      _("bar item \"%s\""),
                      weechat_hdata_string (hdata_bar_item,
                                            ptr_bar_item,
                                            "name"));
            weechat_list_add (list, str_info, WEECHAT_LIST_POS_END, NULL);
        }
        ptr_bar_item = weechat_hdata_move (hdata_bar_item, ptr_bar_item, 1);
    }

    /* get the script options (in plugins.var) */
    snprintf (str_option, sizeof (str_option),
              "plugins.var.%s.%s.*",
              script_language[script->language],
              weechat_hdata_string (hdata_script, ptr_script, "name"));
    infolist = weechat_infolist_get ("option", NULL, str_option);
    if (infolist)
    {
        if (weechat_infolist_next (infolist))
        {
            snprintf (str_info, sizeof (str_info),
                      _("options %s%s%s"),
                      str_option,
                      (config_files > 0) ? " " : "",
                      (config_files > 0) ? _("(old options?)") : "");
            weechat_list_add (list, str_info,
                              WEECHAT_LIST_POS_END, NULL);
        }
        weechat_infolist_free (infolist);
    }

    return list;
}

/*
 * Displays detail on a script.
 */

void
script_buffer_display_detail_script (struct t_script_repo *script)
{
    struct tm *tm;
    char str_time[1024];
    char *labels[] = { N_("Script"), N_("Version"), N_("Version loaded"),
                       N_("Author"), N_("License"), N_("Description"),
                       N_("Tags"), N_("Status"),  N_("Date added"),
                       N_("Date updated"), N_("URL"), N_("SHA-512"),
                       N_("Requires"), N_("Min WeeChat"), N_("Max WeeChat"),
                       NULL };
    int i, length, max_length, line;
    struct t_weelist *list;
    struct t_weelist_item *ptr_item;

    max_length = 0;
    for (i = 0; labels[i]; i++)
    {
        length = weechat_utf8_strlen_screen (_(labels[i]));
        if (length > max_length)
            max_length = length;
    }

    line = 0;

    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s%s%s.%s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      weechat_color (weechat_config_string (script_config_color_text_name)),
                      script->name,
                      weechat_color (weechat_config_string (script_config_color_text_extension)),
                      script_extension[script->language]);
    line++;
    weechat_printf_y (script_buffer, line + 1, "%s: %s%s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      weechat_color (weechat_config_string (script_config_color_text_version)),
                      script->version);
    line++;
    weechat_printf_y (script_buffer, line + 1, "%s: %s%s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      weechat_color (weechat_config_string (script_config_color_text_version_loaded)),
                      (script->version_loaded) ? script->version_loaded : "-");
    line++;
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s <%s>",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      script->author,
                      script->mail);
    line++;
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      script->license);
    line++;
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      script->description);
    line++;
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      script->tags);
    line++;
    if ((script->popularity == 0) && (script->status == 0))
    {
        weechat_printf_y (script_buffer, line + 1,
                          "%s: -",
                          script_buffer_detail_label (_(labels[line]), max_length));
    }
    else
    {
        weechat_printf_y (script_buffer, line + 1,
                          "%s: %s%s (%s)",
                          script_buffer_detail_label (_(labels[line]), max_length),
                          script_repo_get_status_for_display (script, "*iaHrN", 1),
                          weechat_color ("chat"),
                          script_repo_get_status_desc_for_display (script, "*iaHrN"));
    }
    line++;
    tm = localtime (&script->date_added);
    if (strftime (str_time, sizeof (str_time), "%Y-%m-%d %H:%M:%S", tm) == 0)
        str_time[0] = '\0';
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      str_time);
    line++;
    tm = localtime (&script->date_updated);
    if (strftime (str_time, sizeof (str_time), "%Y-%m-%d %H:%M:%S", tm) == 0)
        str_time[0] = '\0';
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      str_time);
    line++;
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      script->url);
    line++;
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      (script->sha512sum) ? script->sha512sum : "-");
    line++;
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      (script->requirements) ? script->requirements : "-");
    line++;
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      (script->min_weechat) ? script->min_weechat : "-");
    line++;
    weechat_printf_y (script_buffer, line + 1,
                      "%s: %s",
                      script_buffer_detail_label (_(labels[line]), max_length),
                      (script->max_weechat) ? script->max_weechat : "-");
    line++;

    if (script->status & SCRIPT_STATUS_RUNNING)
    {
        list = script_buffer_get_script_usage (script);
        if (list)
        {
            line++;
            weechat_printf_y (script_buffer, line + 1,
                              _("Script has defined:"));
            i = 0;
            ptr_item = weechat_list_get (list, 0);
            while (ptr_item)
            {
                line++;
                weechat_printf_y (script_buffer, line + 1,
                                  "  %s", weechat_list_string (ptr_item));
                ptr_item = weechat_list_next (ptr_item);
                i++;
            }
            if (i == 0)
            {
                line++;
                weechat_printf_y (script_buffer, line + 1,
                                  "  %s", _("(nothing)"));
            }
            line++;
            weechat_list_free (list);
        }
    }

    script_buffer_detail_script_last_line = line + 2;
    script_buffer_detail_script_line_diff = -1;
}

/*
 * Updates list of scripts in script buffer.
 */

void
script_buffer_refresh (int clear)
{
    struct t_script_repo *ptr_script;
    int line;
    char str_title[1024];

    if (!script_buffer)
        return;

    if (clear)
    {
        weechat_buffer_clear (script_buffer);
        script_buffer_selected_line = (script_repo_count_displayed > 0) ? 0 : -1;
    }

    if (script_buffer_detail_script)
    {
        snprintf (str_title, sizeof (str_title),
                  "%s",
                  _("Alt+key/input: v=back to list d=jump to diff"));
    }
    else
    {
        snprintf (str_title, sizeof (str_title),
                  _("%d/%d scripts (filter: %s) | Sort: %s | "
                    "Alt+key/input: i=install, r=remove, l=load, L=reload, "
                    "u=unload, A=autoload, h=(un)hold, v=view script | "
                    "Input: q=close, $=refresh, s:x,y=sort, words=filter, "
                    "*=reset filter | Mouse: left=select, right=install/remove"),
                  script_repo_count_displayed,
                  script_repo_count,
                  (script_repo_filter) ? script_repo_filter : "*",
                  weechat_config_string (script_config_look_sort));
    }
    weechat_buffer_set (script_buffer, "title", str_title);

    if (script_buffer_detail_script)
    {
        /* detail on a script */
        script_buffer_display_detail_script (script_buffer_detail_script);
    }
    else
    {
        /* list of scripts */
        line = 0;
        for (ptr_script = scripts_repo; ptr_script;
             ptr_script = ptr_script->next_script)
        {
            if (ptr_script->displayed)
            {
                script_buffer_display_line_script (line, ptr_script);
                line++;
            }
        }
    }
}

/*
 * Sets current selected line.
 */

void
script_buffer_set_current_line (int line)
{
    int old_line;

    if ((line >= 0) && (line < script_repo_count_displayed))
    {
        old_line = script_buffer_selected_line;
        script_buffer_selected_line = line;

        script_buffer_display_line_script (old_line,
                                           script_repo_search_displayed_by_number (old_line));
        script_buffer_display_line_script (script_buffer_selected_line,
                                           script_repo_search_displayed_by_number (script_buffer_selected_line));
    }
}

/*
 * Shows detailed info on a script.
 */

void
script_buffer_show_detail_script (struct t_script_repo *script)
{
    if (!script_buffer)
        return;

    if (script_buffer_detail_script == script)
        script_buffer_detail_script = NULL;
    else
        script_buffer_detail_script = script;

    weechat_buffer_clear (script_buffer);
    script_buffer_refresh (0);

    if (!script_buffer_detail_script)
        script_buffer_check_line_outside_window ();
}

/*
 * Gets info about a window.
 */

void
script_buffer_get_window_info (struct t_gui_window *window,
                               int *start_line_y, int *chat_height)
{
    struct t_hdata *hdata_window, *hdata_window_scroll, *hdata_line;
    struct t_hdata *hdata_line_data;
    void *window_scroll, *start_line, *line_data;

    hdata_window = weechat_hdata_get ("window");
    hdata_window_scroll = weechat_hdata_get ("window_scroll");
    hdata_line = weechat_hdata_get ("line");
    hdata_line_data = weechat_hdata_get ("line_data");
    *start_line_y = 0;
    window_scroll = weechat_hdata_pointer (hdata_window, window, "scroll");
    if (window_scroll)
    {
        start_line = weechat_hdata_pointer (hdata_window_scroll, window_scroll,
                                            "start_line");
        if (start_line)
        {
            line_data = weechat_hdata_pointer (hdata_line, start_line, "data");
            if (line_data)
            {
                *start_line_y = weechat_hdata_integer (hdata_line_data,
                                                       line_data, "y");
            }
        }
    }
    *chat_height = weechat_hdata_integer (hdata_window, window,
                                          "win_chat_height");
}

/*
 * Checks if current line is outside window.
 *
 * Returns:
 *   1: line is outside window
 *   0: line is inside window
 */

void
script_buffer_check_line_outside_window ()
{
    struct t_gui_window *window;
    int start_line_y, chat_height;
    char str_command[256];

    window = weechat_window_search_with_buffer (script_buffer);
    if (!window)
        return;

    script_buffer_get_window_info (window, &start_line_y, &chat_height);
    if ((start_line_y > script_buffer_selected_line)
        || (start_line_y <= script_buffer_selected_line - chat_height))
    {
        snprintf (str_command, sizeof (str_command),
                  "/window scroll -window %d %s%d",
                  weechat_window_get_integer (window, "number"),
                  (start_line_y > script_buffer_selected_line) ? "-" : "+",
                  (start_line_y > script_buffer_selected_line) ?
                  start_line_y - script_buffer_selected_line :
                  script_buffer_selected_line - start_line_y - chat_height + 1);
        weechat_command (script_buffer, str_command);
    }
}

/*
 * Callback for signal "window_scrolled".
 */

int
script_buffer_window_scrolled_cb (const void *pointer, void *data,
                                  const char *signal, const char *type_data,
                                  void *signal_data)
{
    int start_line_y, chat_height, line;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) signal;
    (void) type_data;

    /* scrolled another window/buffer? then just ignore */
    if (weechat_window_get_pointer (signal_data, "buffer") != script_buffer)
        return WEECHAT_RC_OK;

    /* ignore if detail of a script is displayed */
    if (script_buffer_detail_script)
        return WEECHAT_RC_OK;

    script_buffer_get_window_info (signal_data, &start_line_y, &chat_height);

    line = script_buffer_selected_line;
    while (line < start_line_y)
    {
        line += chat_height;
    }
    while (line >= start_line_y + chat_height)
    {
        line -= chat_height;
    }
    if (line < start_line_y)
        line = start_line_y;
    if (line >= script_repo_count_displayed)
        line = script_repo_count_displayed - 1;
    script_buffer_set_current_line (line);

    return WEECHAT_RC_OK;
}

/*
 * Callback for user data in script buffer.
 */

int
script_buffer_input_cb (const void *pointer, void *data,
                        struct t_gui_buffer *buffer,
                        const char *input_data)
{
    char *actions[][2] = { { "A", "toggleautoload" },
                           { "l", "load"           },
                           { "u", "unload"         },
                           { "L", "reload"         },
                           { "i", "install"        },
                           { "r", "remove"         },
                           { "h", "hold"           },
                           { "v", "show"           },
                           { "d", "showdiff"       },
                           { NULL, NULL            } };
    char str_command[64];
    int i;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    /* close buffer */
    if (strcmp (input_data, "q") == 0)
    {
        weechat_buffer_close (buffer);
        return WEECHAT_RC_OK;
    }

    if (!script_buffer_detail_script)
    {
        /* change sort keys on buffer */
        if (strncmp (input_data, "s:", 2) == 0)
        {
            if (input_data[2])
                weechat_config_option_set (script_config_look_sort, input_data + 2, 1);
            else
                weechat_config_option_reset (script_config_look_sort, 1);
            return WEECHAT_RC_OK;
        }

        /* refresh buffer */
        if (strcmp (input_data, "$") == 0)
        {
            script_get_loaded_plugins ();
            script_get_scripts ();
            script_repo_remove_all ();
            script_repo_file_read (1);
            script_buffer_refresh (1);
            return WEECHAT_RC_OK;
        }
    }

    /* execute action on a script */
    for (i = 0; actions[i][0]; i++)
    {
        if (strcmp (input_data, actions[i][0]) == 0)
        {
            snprintf (str_command, sizeof (str_command),
                      "/script %s", actions[i][1]);
            weechat_command (buffer, str_command);
            return WEECHAT_RC_OK;
        }
    }

    /* filter scripts with given text */
    if (!script_buffer_detail_script)
        script_repo_filter_scripts (input_data);

    return WEECHAT_RC_OK;
}

/*
 * Callback called when script buffer is closed.
 */

int
script_buffer_close_cb (const void *pointer, void *data,
                        struct t_gui_buffer *buffer)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    script_buffer = NULL;
    script_buffer_selected_line = 0;
    script_buffer_detail_script = NULL;

    return WEECHAT_RC_OK;
}

/*
 * Restore buffer callbacks (input and close) for buffer created by script
 * plugin.
 */

void
script_buffer_set_callbacks ()
{
    struct t_gui_buffer *ptr_buffer;

    ptr_buffer = weechat_buffer_search (SCRIPT_PLUGIN_NAME, SCRIPT_BUFFER_NAME);
    if (ptr_buffer)
    {
        script_buffer = ptr_buffer;
        weechat_buffer_set_pointer (script_buffer, "close_callback", &script_buffer_close_cb);
        weechat_buffer_set_pointer (script_buffer, "input_callback", &script_buffer_input_cb);
    }
}

/*
 * Sets keys on script buffer.
 */

void
script_buffer_set_keys ()
{
    char *keys[][2] = {
        { "meta-A", "toggleautoload" },
        { "meta-l", "load"           },
        { "meta-u", "unload"         },
        { "meta-L", "reload"         },
        { "meta-i", "install"        },
        { "meta-r", "remove"         },
        { "meta-h", "hold"           },
        { "meta-v", "show"           },
        { "meta-d", "showdiff"       },
        { NULL, NULL },
    };
    char str_key[64], str_command[64];
    int i;

    weechat_buffer_set (script_buffer, "key_bind_up", "/script up");
    weechat_buffer_set (script_buffer, "key_bind_down", "/script down");
    for (i = 0; keys[i][0]; i++)
    {
        if (weechat_config_boolean (script_config_look_use_keys))
        {
            snprintf (str_key, sizeof (str_key), "key_bind_%s", keys[i][0]);
            snprintf (str_command, sizeof (str_command), "/script %s", keys[i][1]);
            weechat_buffer_set (script_buffer, str_key, str_command);
        }
        else
        {
            snprintf (str_key, sizeof (str_key), "key_unbind_%s", keys[i][0]);
            weechat_buffer_set (script_buffer, str_key, "");
        }
    }
}

/*
 * Sets the local variable "filter" in the script buffer.
 */

void
script_buffer_set_localvar_filter ()
{
    if (!script_buffer)
        return;

    weechat_buffer_set (script_buffer, "localvar_set_filter",
                        (script_repo_filter) ? script_repo_filter : "*");
}

/*
 * Opens script buffer.
 */

void
script_buffer_open ()
{
    struct t_hashtable *buffer_props;

    if (script_buffer)
        return;

    buffer_props = weechat_hashtable_new (
        32,
        WEECHAT_HASHTABLE_STRING,
        WEECHAT_HASHTABLE_STRING,
        NULL, NULL);
    if (buffer_props)
    {
        weechat_hashtable_set (buffer_props, "type", "free");
        weechat_hashtable_set (buffer_props, "title", _("Scripts"));
        weechat_hashtable_set (buffer_props, "localvar_set_type", "script");
    }

    script_buffer = weechat_buffer_new_props (
        SCRIPT_BUFFER_NAME,
        buffer_props,
        &script_buffer_input_cb, NULL, NULL,
        &script_buffer_close_cb, NULL, NULL);

    if (buffer_props)
        weechat_hashtable_free (buffer_props);

    if (!script_buffer)
        return;

    script_buffer_set_keys ();
    script_buffer_set_localvar_filter ();

    script_buffer_selected_line = 0;
    script_buffer_detail_script = NULL;
}
