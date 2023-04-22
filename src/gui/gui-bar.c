/*
 * gui-bar.c - bar functions (used by all GUI)
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
#include <stddef.h>
#include <string.h>
#include <limits.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-eval.h"
#include "../core/wee-hashtable.h"
#include "../core/wee-hdata.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../plugins/plugin.h"
#include "gui-bar.h"
#include "gui-bar-item.h"
#include "gui-bar-window.h"
#include "gui-buffer.h"
#include "gui-chat.h"
#include "gui-color.h"
#include "gui-window.h"


char *gui_bar_option_string[GUI_BAR_NUM_OPTIONS] =
{ "hidden", "priority", "type", "conditions", "position", "filling_top_bottom",
  "filling_left_right", "size", "size_max", "color_fg", "color_delim",
  "color_bg", "color_bg_inactive", "separator", "items" };
char *gui_bar_option_default[GUI_BAR_NUM_OPTIONS] =
{ "off", "0", "root", "", "top", "horizontal",
  "vertical", "0", "0", "default", "default",
  "default", "default", "off", "" };
char *gui_bar_type_string[GUI_BAR_NUM_TYPES] =
{ "root", "window" };
char *gui_bar_position_string[GUI_BAR_NUM_POSITIONS] =
{ "bottom", "top", "left", "right" };
char *gui_bar_filling_string[GUI_BAR_NUM_FILLING] =
{ "horizontal", "vertical", "columns_horizontal", "columns_vertical" };

/* default bars */
char *gui_bar_default_name[GUI_BAR_NUM_DEFAULT_BARS] =
{ "input", "title", "status", "nicklist" };
char *gui_bar_default_values[GUI_BAR_NUM_DEFAULT_BARS][GUI_BAR_NUM_OPTIONS] =
{
    /* input */
    { "off", "1000", "window", "", "bottom", "horizontal", "vertical",
      "0", "0", "default", "cyan", "default", "default", "off",
      "[input_prompt]+(away),[input_search],[input_paste],input_text" },
    /* title */
    { "off", "500", "window", "", "top", "horizontal", "vertical",
      "1", "0", "default", "cyan", "234", "232", "off",
      "buffer_title" },
    /* status */
    { "off", "500", "window", "", "bottom", "horizontal", "vertical",
      "1", "0", "default", "cyan", "234", "232", "off",
      "[time],[buffer_last_number],[buffer_plugin],buffer_number+:+"
      "buffer_name+(buffer_modes)+{buffer_nicklist_count}+buffer_zoom+"
      "buffer_filter,mouse_status,scroll,[lag],[hotlist],[typing],"
      "completion" },
    /* nicklist */
    { "off", "200", "window", "${nicklist}", "right",
      "columns_vertical", "vertical",
      "0", "0", "default", "cyan", "default", "default", "on",
      "buffer_nicklist" },
};


struct t_gui_bar *gui_bars = NULL;         /* first bar                     */
struct t_gui_bar *last_gui_bar = NULL;     /* last bar                      */

struct t_gui_bar *gui_temp_bars = NULL;    /* bars used when reading config */
struct t_gui_bar *last_gui_temp_bar = NULL;


void gui_bar_free_bar_windows (struct t_gui_bar *bar);


/*
 * Checks if a bar pointer is valid.
 *
 * Returns:
 *   1: bar exists
 *   0: bar does not exist
 */

int
gui_bar_valid (struct t_gui_bar *bar)
{
    struct t_gui_bar *ptr_bar;

    if (!bar)
        return 0;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (ptr_bar == bar)
            return 1;
    }

    /* bar not found */
    return 0;
}

/*
 * Searches for a default bar name.
 *
 * Returns index of default bar in enum t_gui_bar_default, -1 if not found.
 */

int
gui_bar_search_default_bar (const char *bar_name)
{
    int i;

    if (!bar_name)
        return -1;

    for (i = 0; i < GUI_BAR_NUM_DEFAULT_BARS; i++)
    {
        if (strcmp (gui_bar_default_name[i], bar_name) == 0)
            return i;
    }

    /* default bar not found */
    return -1;
}

/*
 * Searches for a bar option name.
 *
 * Returns index of option in enum t_gui_bar_option, -1 if not found.
 */

int
gui_bar_search_option (const char *option_name)
{
    int i;

    if (!option_name)
        return -1;

    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        if (strcmp (gui_bar_option_string[i], option_name) == 0)
            return i;
    }

    /* bar option not found */
    return -1;
}

/*
 * Searches for a bar type.
 *
 * Returns index of type in enum t_gui_bar_type, -1 if not found.
 */

int
gui_bar_search_type (const char *type)
{
    int i;

    if (!type)
        return -1;

    for (i = 0; i < GUI_BAR_NUM_TYPES; i++)
    {
        if (strcmp (gui_bar_type_string[i], type) == 0)
            return i;
    }

    /* type not found */
    return -1;
}

/*
 * Searches for a bar position.
 *
 * Returns index of position in enum t_gui_bar_position, -1 if not found.
 */

int
gui_bar_search_position (const char *position)
{
    int i;

    if (!position)
        return -1;

    for (i = 0; i < GUI_BAR_NUM_POSITIONS; i++)
    {
        if (strcmp (gui_bar_position_string[i], position) == 0)
            return i;
    }

    /* position not found */
    return -1;
}

/*
 * Checks if "add_size" is OK for bar.
 *
 * Returns:
 *   1: new size is OK
 *   0: new size is too big
 */

int
gui_bar_check_size_add (struct t_gui_bar *bar, int add_size)
{
    struct t_gui_window *ptr_window;
    int sub_width, sub_height;

    sub_width = 0;
    sub_height = 0;

    switch (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_POSITION]))
    {
        case GUI_BAR_POSITION_BOTTOM:
        case GUI_BAR_POSITION_TOP:
            sub_height = add_size;
            break;
        case GUI_BAR_POSITION_LEFT:
        case GUI_BAR_POSITION_RIGHT:
            sub_width = add_size;
            break;
        case GUI_BAR_NUM_POSITIONS:
            break;
    }

    for (ptr_window = gui_windows; ptr_window;
         ptr_window = ptr_window->next_window)
    {
        if ((CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
            || (gui_bar_window_search_bar (ptr_window, bar)))
        {
            if ((ptr_window->win_chat_width - sub_width < 1)
                || (ptr_window->win_chat_height - sub_height < 1))
                return 0;
        }
    }

    /* new size OK */
    return 1;
}

/*
 * Returns filling option for bar, according to filling for current bar
 * position.
 */

enum t_gui_bar_filling
gui_bar_get_filling (struct t_gui_bar *bar)
{
    if ((CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_BOTTOM)
        || (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_POSITION]) == GUI_BAR_POSITION_TOP))
        return CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM]);

    return CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT]);
}

/*
 * Searches for position of a bar in list (to keep list sorted by priority).
 */

struct t_gui_bar *
gui_bar_find_pos (struct t_gui_bar *bar)
{
    struct t_gui_bar *ptr_bar;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_PRIORITY]) >= CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_PRIORITY]))
            return ptr_bar;
    }

    /* bar not found, add to end of list */
    return NULL;
}

/*
 * Inserts a bar to the list (at good position, according to priority).
 */

void
gui_bar_insert (struct t_gui_bar *bar)
{
    struct t_gui_bar *pos_bar;

    if (gui_bars)
    {
        pos_bar = gui_bar_find_pos (bar);
        if (pos_bar)
        {
            /* insert bar into the list (before position found) */
            bar->prev_bar = pos_bar->prev_bar;
            bar->next_bar = pos_bar;
            if (pos_bar->prev_bar)
                (pos_bar->prev_bar)->next_bar = bar;
            else
                gui_bars = bar;
            pos_bar->prev_bar = bar;
        }
        else
        {
            /* add bar to the end */
            bar->prev_bar = last_gui_bar;
            bar->next_bar = NULL;
            last_gui_bar->next_bar = bar;
            last_gui_bar = bar;
        }
    }
    else
    {
        bar->prev_bar = NULL;
        bar->next_bar = NULL;
        gui_bars = bar;
        last_gui_bar = bar;
    }
}

/*
 * Checks if bar must be displayed in window according to conditions.
 *
 * If window is NULL (case of root bars), the current window is used.
 *
 * Returns:
 *   1: bar must be displayed
 *   0: bar must not be displayed
 */

int
gui_bar_check_conditions (struct t_gui_bar *bar,
                          struct t_gui_window *window)
{
    int rc;
    char str_modifier[256], str_window[128], *str_displayed, *result;
    const char *conditions;
    struct t_hashtable *pointers, *extra_vars, *options;

    if (!window)
        window = gui_current_window;

    /* check bar condition(s) */
    conditions = CONFIG_STRING(bar->options[GUI_BAR_OPTION_CONDITIONS]);
    if (string_strcmp (conditions, "active") == 0)
    {
        if (gui_current_window && (gui_current_window != window))
            return 0;
    }
    else if (string_strcmp (conditions, "inactive") == 0)
    {
        if (!gui_current_window || (gui_current_window == window))
            return 0;
    }
    else if (string_strcmp (conditions, "nicklist") == 0)
    {
        if (window && window->buffer && !window->buffer->nicklist)
            return 0;
    }
    else if (conditions[0])
    {
        pointers = hashtable_new (32,
                                  WEECHAT_HASHTABLE_STRING,
                                  WEECHAT_HASHTABLE_POINTER,
                                  NULL, NULL);
        if (pointers)
        {
            hashtable_set (pointers, "window", window);
            if (window)
                hashtable_set (pointers, "buffer", window->buffer);
        }
        extra_vars = hashtable_new (32,
                                    WEECHAT_HASHTABLE_STRING,
                                    WEECHAT_HASHTABLE_STRING,
                                    NULL, NULL);
        if (extra_vars)
        {
            hashtable_set (extra_vars, "active",
                           (gui_current_window && (gui_current_window == window)) ? "1" : "0");
            hashtable_set (extra_vars, "inactive",
                           (gui_current_window && (gui_current_window != window)) ? "1" : "0");
            hashtable_set (extra_vars, "nicklist",
                           (window && window->buffer && window->buffer->nicklist) ? "1" : "0");
        }
        options = hashtable_new (32,
                                 WEECHAT_HASHTABLE_STRING,
                                 WEECHAT_HASHTABLE_STRING,
                                 NULL, NULL);
        if (options)
            hashtable_set (options, "type", "condition");

        result = eval_expression (conditions, pointers, extra_vars, options);

        rc = eval_is_true (result);
        if (result)
            free (result);
        if (pointers)
            hashtable_free (pointers);
        if (extra_vars)
            hashtable_free (extra_vars);
        if (options)
            hashtable_free (options);
        if (!rc)
            return 0;
    }

    /*
     * call a modifier that will tell us if bar is displayed or not,
     * for example it can be used to display nicklist on some buffers
     * only, using a script that implements this modifier and return "1"
     * to display bar, "0" to hide it
     */
    snprintf (str_modifier, sizeof (str_modifier),
              "bar_condition_%s", bar->name);
    snprintf (str_window, sizeof (str_window),
              "0x%lx", (unsigned long)(window));
    str_displayed = hook_modifier_exec (NULL,
                                        str_modifier,
                                        str_window,
                                        "");
    if (str_displayed && strcmp (str_displayed, "0") == 0)
        rc = 0;
    else
        rc = 1;

    if (str_displayed)
        free (str_displayed);

    return rc;
}

/*
 * Gets total bar size ("root" type) for a position.
 */

int
gui_bar_root_get_size (struct t_gui_bar *bar, enum t_gui_bar_position position)
{
    struct t_gui_bar *ptr_bar;
    int total_size;

    total_size = 0;
    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (bar && (ptr_bar == bar))
            return total_size;

        if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])
            && ptr_bar->bar_window)
        {
            if ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
                && (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION]) == (int)position))
            {
                total_size += gui_bar_window_get_current_size (ptr_bar->bar_window);
                if (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SEPARATOR]))
                    total_size++;
            }
        }
    }
    return total_size;
}

/*
 * Searches for a bar by name.
 *
 * Returns pointer to bar found, NULL if not found.
 */

struct t_gui_bar *
gui_bar_search (const char *name)
{
    struct t_gui_bar *ptr_bar;

    if (!name || !name[0])
        return NULL;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (strcmp (ptr_bar->name, name) == 0)
            return ptr_bar;
    }

    /* bar not found */
    return NULL;
}

/*
 * Searches for a bar with name of option (like "status.type").
 *
 * Returns pointer to bar found, NULL if not found.
 */

struct t_gui_bar *
gui_bar_search_with_option_name (const char *option_name)
{
    char *bar_name, *pos_option;
    struct t_gui_bar *ptr_bar;

    if (!option_name)
        return NULL;

    ptr_bar = NULL;

    pos_option = strchr (option_name, '.');
    if (pos_option)
    {
        bar_name = string_strndup (option_name, pos_option - option_name);
        if (bar_name)
        {
            for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
            {
                if (strcmp (ptr_bar->name, bar_name) == 0)
                    break;
            }
            free (bar_name);
        }
    }

    return ptr_bar;
}

/*
 * Rebuilds content of bar windows for a bar.
 */

void
gui_bar_content_build_bar_windows (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_window;
    struct t_gui_bar_window *ptr_bar_window;

    if (!bar)
        return;

    if (bar->bar_window)
    {
        gui_bar_window_content_build (bar->bar_window, NULL);
    }
    else
    {
        for (ptr_window = gui_windows; ptr_window;
             ptr_window = ptr_window->next_window)
        {
            for (ptr_bar_window = ptr_window->bar_windows; ptr_bar_window;
                 ptr_bar_window = ptr_bar_window->next_bar_window)
            {
                if (ptr_bar_window->bar == bar)
                    gui_bar_window_content_build (ptr_bar_window, ptr_window);
            }
        }
    }
}

/*
 * Asks refresh for bar.
 */

void
gui_bar_ask_refresh (struct t_gui_bar *bar)
{
    bar->bar_refresh_needed = 1;
}

/*
 * Asks for bar refresh on screen (for all windows where bar is).
 */

void
gui_bar_refresh (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;

    if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
        gui_window_ask_refresh (1);
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            if (gui_bar_window_search_bar (ptr_win, bar))
                ptr_win->refresh_needed = 1;
        }
    }
}

/*
 * Draws a bar.
 */

void
gui_bar_draw (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;

    if (!CONFIG_BOOLEAN(bar->options[GUI_BAR_OPTION_HIDDEN]))
    {
        if (bar->bar_window)
        {
            /* root bar */
            gui_bar_window_draw (bar->bar_window, NULL);
        }
        else
        {
            /* bar on each window */
            for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
            {
                for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
                     ptr_bar_win = ptr_bar_win->next_bar_window)
                {
                    if (ptr_bar_win->bar == bar)
                    {
                        gui_bar_window_draw (ptr_bar_win, ptr_win);
                    }
                }
            }
        }
    }
    bar->bar_refresh_needed = 0;
}

/*
 * Applies new size for all bar windows of bar.
 */

void
gui_bar_apply_current_size (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win;

    if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
    {
        gui_bar_window_set_current_size (bar->bar_window,
                                         NULL,
                                         CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE]));
        gui_window_ask_refresh (1);
    }
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            for (ptr_bar_win = ptr_win->bar_windows; ptr_bar_win;
                 ptr_bar_win = ptr_bar_win->next_bar_window)
            {
                if (ptr_bar_win->bar == bar)
                {
                    gui_bar_window_set_current_size (ptr_bar_win,
                                                     ptr_win,
                                                     CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE]));
                }
            }
        }
    }
}

/*
 * Frees arrays with items for a bar.
 */

void
gui_bar_free_items_arrays (struct t_gui_bar *bar)
{
    int i, j;

    for (i = 0; i < bar->items_count; i++)
    {
        if (bar->items_array[i])
            string_free_split (bar->items_array[i]);
        for (j = 0; j < bar->items_subcount[i]; j++)
        {
            if (bar->items_buffer[i][j])
                free (bar->items_buffer[i][j]);
            if (bar->items_prefix[i][j])
                free (bar->items_prefix[i][j]);
            if (bar->items_name[i][j])
                free (bar->items_name[i][j]);
            if (bar->items_suffix[i][j])
                free (bar->items_suffix[i][j]);
        }
        if (bar->items_buffer[i])
            free (bar->items_buffer[i]);
        if (bar->items_prefix[i])
            free (bar->items_prefix[i]);
        if (bar->items_name[i])
            free (bar->items_name[i]);
        if (bar->items_suffix[i])
            free (bar->items_suffix[i]);
    }
    if (bar->items_array)
    {
        free (bar->items_array);
        bar->items_array = NULL;
    }
    if (bar->items_buffer)
    {
        free (bar->items_buffer);
        bar->items_buffer = NULL;
    }
    if (bar->items_prefix)
    {
        free (bar->items_prefix);
        bar->items_prefix = NULL;
    }
    if (bar->items_name)
    {
        free (bar->items_name);
        bar->items_name = NULL;
    }
    if (bar->items_suffix)
    {
        free (bar->items_suffix);
        bar->items_suffix = NULL;
    }
    if (bar->items_subcount)
    {
        free (bar->items_subcount);
        bar->items_subcount = NULL;
    }
    bar->items_count = 0;
}

/*
 * Builds array with items for a bar.
 */

void
gui_bar_set_items_array (struct t_gui_bar *bar, const char *items)
{
    int i, j, count;
    char **tmp_array;

    gui_bar_free_items_arrays (bar);

    if (items && items[0])
    {
        tmp_array = string_split (items, ",", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &count);
        if (count > 0)
        {
            bar->items_count = count;
            bar->items_subcount = malloc (count * sizeof (*bar->items_subcount));
            bar->items_array = malloc (count * sizeof (*bar->items_array));
            bar->items_buffer = malloc (count * sizeof (*bar->items_buffer));
            bar->items_prefix = malloc (count * sizeof (*bar->items_prefix));
            bar->items_name = malloc (count * sizeof (*bar->items_name));
            bar->items_suffix = malloc (count * sizeof (*bar->items_suffix));
            for (i = 0; i < count; i++)
            {
                bar->items_array[i] = string_split (
                    tmp_array[i],
                    "+",
                    NULL,
                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                    0,
                    &(bar->items_subcount[i]));
                if (bar->items_subcount[i] > 0)
                {
                    bar->items_buffer[i] = malloc (bar->items_subcount[i] *
                                                   sizeof (*(bar->items_buffer[i])));
                    bar->items_prefix[i] = malloc (bar->items_subcount[i] *
                                                   sizeof (*(bar->items_prefix[i])));
                    bar->items_name[i] = malloc (bar->items_subcount[i] *
                                                 sizeof (*(bar->items_name[i])));
                    bar->items_suffix[i] = malloc (bar->items_subcount[i] *
                                                   sizeof (*(bar->items_suffix[i])));
                    for (j = 0; j < bar->items_subcount[i]; j++)
                    {
                        gui_bar_item_get_vars (bar->items_array[i][j],
                                               &bar->items_buffer[i][j],
                                               &bar->items_prefix[i][j],
                                               &bar->items_name[i][j],
                                               &bar->items_suffix[i][j]);
                    }
                }
            }
        }
        string_free_split (tmp_array);
    }
}

/*
 * Callback for checking bar type before changing it.
 *
 * Returns always 0 because changing the type of a bar is not allowed.
 */

int
gui_bar_config_check_type (const void *pointer, void *data,
                           struct t_config_option *option,
                           const char *value)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;
    (void) value;

    gui_chat_printf (NULL,
                     _("%sUnable to change bar type: you must delete bar "
                       "and create another to do that"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR]);
    return 0;
}

/*
 * Callback called when "hidden" flag is changed.
 */

void
gui_bar_config_change_hidden (const void *pointer, void *data,
                              struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win, *next_bar_win;
    int bar_window_exists;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        if (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
        {
            if (CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
            {
                if (ptr_bar->bar_window)
                    gui_bar_window_free (ptr_bar->bar_window, NULL);
            }
            else
            {
                if (!ptr_bar->bar_window)
                    gui_bar_window_new (ptr_bar, NULL);
            }
        }
        else
        {
            for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
            {
                bar_window_exists = 0;
                ptr_bar_win = ptr_win->bar_windows;
                while (ptr_bar_win)
                {
                    next_bar_win = ptr_bar_win->next_bar_window;

                    if (ptr_bar_win->bar == ptr_bar)
                    {
                        if (CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
                            gui_bar_window_free (ptr_bar_win, ptr_win);
                        else
                            bar_window_exists = 1;
                    }

                    ptr_bar_win = next_bar_win;
                }
                if (!bar_window_exists
                    && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
                {
                    gui_bar_window_new (ptr_bar, ptr_win);
                }
            }
        }
        gui_window_ask_refresh (1);
    }
}

/*
 * Callback called when "priority" is changed.
 */

void
gui_bar_config_change_priority (const void *pointer, void *data,
                                struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *bar_windows, *ptr_bar_win, *next_bar_win;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        /* remove bar from list */
        if (ptr_bar == gui_bars)
        {
            gui_bars = ptr_bar->next_bar;
            gui_bars->prev_bar = NULL;
        }
        if (ptr_bar == last_gui_bar)
        {
            last_gui_bar = ptr_bar->prev_bar;
            last_gui_bar->next_bar = NULL;
        }
        if (ptr_bar->prev_bar)
            (ptr_bar->prev_bar)->next_bar = ptr_bar->next_bar;
        if (ptr_bar->next_bar)
            (ptr_bar->next_bar)->prev_bar = ptr_bar->prev_bar;

        gui_bar_insert (ptr_bar);

        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            bar_windows = ptr_win->bar_windows;
            ptr_win->bar_windows = NULL;
            ptr_win->last_bar_window = NULL;
            ptr_bar_win = bar_windows;
            while (ptr_bar_win)
            {
                next_bar_win = ptr_bar_win->next_bar_window;

                gui_bar_window_insert (ptr_bar_win, ptr_win);

                ptr_bar_win = next_bar_win;
            }
        }

        gui_window_ask_refresh (1);
    }
}

/*
 * Callback called when "conditions" is changed.
 */

void
gui_bar_config_change_conditions (const void *pointer, void *data,
                                  struct t_config_option *option)
{
    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    gui_window_ask_refresh (1);
}

/*
 * Callback called when "position" is changed.
 */

void
gui_bar_config_change_position (const void *pointer, void *data,
                                struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        gui_bar_refresh (ptr_bar);

    gui_window_ask_refresh (1);
}

/*
 * Callback called when "filling" is changed.
 */

void
gui_bar_config_change_filling (const void *pointer, void *data,
                               struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        gui_bar_refresh (ptr_bar);

    gui_window_ask_refresh (1);
}

/*
 * Callback for checking bar size before changing it.
 *
 * Returns:
 *   1: new size is OK
 *   0: new size is NOT OK
 */

int
gui_bar_config_check_size (const void *pointer, void *data,
                           struct t_config_option *option,
                           const char *value)
{
    struct t_gui_bar *ptr_bar;
    long number;
    char *error;
    int new_value, current_size;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        new_value = -1;
        if (strncmp (value, "++", 2) == 0)
        {
            error = NULL;
            number = strtol (value + 2, &error, 10);
            if (error && !error[0])
            {
                new_value = CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]) + number;
            }
        }
        else if (strncmp (value, "--", 2) == 0)
        {
            error = NULL;
            number = strtol (value + 2, &error, 10);
            if (error && !error[0])
            {
                new_value = CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]) - number;
            }
        }
        else
        {
            error = NULL;
            number = strtol (value, &error, 10);
            if (error && !error[0])
            {
                new_value = number;
            }
        }
        if (new_value < 0)
            return 0;

        if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        {
            current_size = CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]);
            if ((current_size == 0) && ptr_bar->bar_window)
                current_size = ptr_bar->bar_window->current_size;
            if ((new_value > 0) && (new_value > current_size)
                && !gui_bar_check_size_add (ptr_bar, new_value - current_size))
            {
                return 0;
            }
        }

        return 1;
    }

    return 0;
}

/*
 * Callback called when "size" is changed.
 */

void
gui_bar_config_change_size (const void *pointer, void *data,
                            struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
    {
        gui_bar_apply_current_size (ptr_bar);
        gui_bar_refresh (ptr_bar);
    }
}

/*
 * Callback called when "size_max" is changed.
 */

void
gui_bar_config_change_size_max (const void *pointer, void *data,
                                struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;
    char value[32];

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) option;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
    {
        if ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE_MAX]) > 0)
            && (CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]) >
                CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE_MAX])))
        {
            snprintf (value, sizeof (value), "%d",
                      CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE_MAX]));
            config_file_option_set (ptr_bar->options[GUI_BAR_OPTION_SIZE], value, 1);
        }
        gui_window_ask_refresh (1);
    }
}

/*
 * Callback when color (fg or bg) is changed.
 */

void
gui_bar_config_change_color (const void *pointer, void *data,
                             struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        gui_bar_refresh (ptr_bar);
}

/*
 * Callback called when "separator" is changed.
 */

void
gui_bar_config_change_separator (const void *pointer, void *data,
                                 struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar && !CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
        gui_bar_refresh (ptr_bar);
}

/*
 * Callback called when "items" is changed.
 */

void
gui_bar_config_change_items (const void *pointer, void *data,
                             struct t_config_option *option)
{
    struct t_gui_bar *ptr_bar;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_bar = gui_bar_search_with_option_name (option->name);
    if (ptr_bar)
    {
        gui_bar_set_items_array (ptr_bar, CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]));
        gui_bar_content_build_bar_windows (ptr_bar);
        if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]))
            gui_bar_ask_refresh (ptr_bar);
    }
}

/*
 * Sets name for a bar.
 */

void
gui_bar_set_name (struct t_gui_bar *bar, const char *name)
{
    char option_name[4096];
    int i;

    if (!name || !name[0])
        return;

    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        snprintf (option_name, sizeof (option_name),
                  "%s.%s", name, gui_bar_option_string[i]);
        config_file_option_rename (bar->options[i], option_name);
    }

    if (bar->name)
        free (bar->name);
    bar->name = strdup (name);
}

/*
 * Sets a property for a bar.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_bar_set (struct t_gui_bar *bar, const char *property, const char *value)
{
    if (!bar || !property || !value)
        return 0;

    if (strcmp (property, "name") == 0)
    {
        gui_bar_set_name (bar, value);
        return 1;
    }
    else if (strcmp (property, "hidden") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_HIDDEN], value, 1);
        return 1;
    }
    else if (strcmp (property, "priority") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_PRIORITY], value, 1);
        return 1;
    }
    else if (strcmp (property, "conditions") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_CONDITIONS], value, 1);
        return 1;
    }
    else if (strcmp (property, "position") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_POSITION], value, 1);
        return 1;
    }
    else if (strcmp (property, "filling_top_bottom") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM], value, 1);
        return 1;
    }
    else if (strcmp (property, "filling_left_right") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT], value, 1);
        return 1;
    }
    else if (strcmp (property, "size") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_SIZE], value, 1);
        return 1;
    }
    else if (strcmp (property, "size_max") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_SIZE_MAX], value, 1);
        return 1;
    }
    else if (strcmp (property, "color_fg") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_COLOR_FG], value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (strcmp (property, "color_delim") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_COLOR_DELIM], value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (strcmp (property, "color_bg") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_COLOR_BG], value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (strcmp (property, "color_bg_inactive") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_COLOR_BG_INACTIVE], value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (strcmp (property, "separator") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_SEPARATOR], value, 1);
        gui_bar_refresh (bar);
        return 1;
    }
    else if (strcmp (property, "items") == 0)
    {
        config_file_option_set (bar->options[GUI_BAR_OPTION_ITEMS], value, 1);
        gui_bar_draw (bar);
        return 1;
    }

    return 0;
}

/*
 * Creates an option for a bar.
 *
 * Returns pointer to new option, NULL if error.
 */

struct t_config_option *
gui_bar_create_option (const char *bar_name, int index_option, const char *value)
{
    struct t_config_option *ptr_option;
    const char *default_value;
    char option_name[4096];
    int index_bar;

    ptr_option = NULL;

    snprintf (option_name, sizeof (option_name),
              "%s.%s",
              bar_name, gui_bar_option_string[index_option]);

    index_bar = gui_bar_search_default_bar (bar_name);
    default_value = (index_bar >= 0) ?
        gui_bar_default_values[index_bar][index_option] : value;

    switch (index_option)
    {
        case GUI_BAR_OPTION_HIDDEN:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "boolean",
                N_("true if bar is hidden, false if it is displayed"),
                NULL, 0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_hidden, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_PRIORITY:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "integer",
                N_("bar priority (high number means bar displayed first)"),
                NULL, 0, INT_MAX, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_priority, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_TYPE:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "integer",
                N_("bar type (root, window, window_active, window_inactive)"),
                "root|window|window_active|window_inactive",
                0, 0, default_value, value, 0,
                &gui_bar_config_check_type, NULL, NULL,
                NULL, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_CONDITIONS:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "string",
                N_("conditions to display the bar: a simple condition: "
                   "\"active\", \"inactive\", \"nicklist\" (window must be "
                   "active/inactive, buffer must have a nicklist), or an "
                   "expression with condition(s) (see /help eval), "
                   "like: \"${nicklist} && ${info:term_width} > 100\" "
                   "(local variables for expression are ${active}, "
                   "${inactive} and ${nicklist})"),
                NULL, 0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_conditions, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_POSITION:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "integer",
                N_("bar position (bottom, top, left, right)"),
                "bottom|top|left|right", 0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_position, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_FILLING_TOP_BOTTOM:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "integer",
                N_("bar filling direction (\"horizontal\" (from left to right) "
                   "or \"vertical\" (from top to bottom)) when bar position is "
                   "top or bottom"),
                "horizontal|vertical|columns_horizontal|columns_vertical",
                0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_filling, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_FILLING_LEFT_RIGHT:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "integer",
                N_("bar filling direction (\"horizontal\" (from left to right) "
                   "or \"vertical\" (from top to bottom)) when bar position is "
                   "left or right"),
                "horizontal|vertical|columns_horizontal|columns_vertical",
                0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_filling, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_SIZE:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "integer",
                N_("bar size in chars (left/right bars) "
                   "or lines (top/bottom bars) (0 = auto size)"),
                NULL, 0, INT_MAX, default_value, value, 0,
                &gui_bar_config_check_size, NULL, NULL,
                &gui_bar_config_change_size, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_SIZE_MAX:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "integer",
                N_("max bar size in chars (left/right bars) "
                   "or lines (top/bottom bars) (0 = no limit)"),
                NULL, 0, INT_MAX, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_size_max, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_COLOR_FG:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "color",
                N_("default text color for bar"),
                NULL, 0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_color, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_COLOR_DELIM:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "color",
                N_("default delimiter color for bar"),
                NULL, 0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_color, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_COLOR_BG:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "color",
                N_("default background color for bar"),
                NULL, 0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_color, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_COLOR_BG_INACTIVE:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "color",
                N_("background color for a bar with type \"window\" which is "
                   "not displayed in the active window"),
                NULL, 0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_color, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_SEPARATOR:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "boolean",
                N_("separator line between bar and other bars/windows"),
                NULL, 0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_separator, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_OPTION_ITEMS:
            ptr_option = config_file_new_option (
                weechat_config_file, weechat_config_section_bar,
                option_name, "string",
                N_("items of bar, they can be separated by comma (space "
                   "between items) or \"+\" (glued items); special syntax "
                   "\"@buffer:item\" can be used to force buffer used when "
                   "displaying the bar item"),
                NULL, 0, 0, default_value, value, 0,
                NULL, NULL, NULL,
                &gui_bar_config_change_items, NULL, NULL,
                NULL, NULL, NULL);
            break;
        case GUI_BAR_NUM_OPTIONS:
            break;
    }

    return ptr_option;
}

/*
 * Creates option for a temporary bar (when reading configuration file).
 */

void
gui_bar_create_option_temp (struct t_gui_bar *temp_bar, int index_option,
                            const char *value)
{
    struct t_config_option *new_option;

    new_option = gui_bar_create_option (temp_bar->name,
                                        index_option,
                                        value);
    if (new_option
        && (index_option >= 0) && (index_option < GUI_BAR_NUM_OPTIONS))
    {
        temp_bar->options[index_option] = new_option;
    }
}

/*
 * Allocates and initializes new bar structure.
 *
 * Returns pointer to new bar, NULL if error.
 */

struct t_gui_bar *
gui_bar_alloc (const char *name)
{
    struct t_gui_bar *new_bar;
    int i;

    new_bar = malloc (sizeof (*new_bar));
    if (!new_bar)
        return NULL;

    new_bar->name = strdup (name);
    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        new_bar->options[i] = NULL;
    }
    new_bar->items_count = 0;
    new_bar->items_array = NULL;
    new_bar->items_buffer = NULL;
    new_bar->items_prefix = NULL;
    new_bar->items_name = NULL;
    new_bar->items_suffix = NULL;
    new_bar->bar_window = NULL;
    new_bar->bar_refresh_needed = 0;
    new_bar->prev_bar = NULL;
    new_bar->next_bar = NULL;

    return new_bar;
}

/*
 * Sets default value for a bar option.
 */

void
gui_bar_set_default_value (struct t_gui_bar *bar, int index_option,
                           const char *value)
{
    struct t_config_option *ptr_option;
    char option_name[4096];

    snprintf (option_name, sizeof (option_name),
              "%s.%s",
              bar->name,
              gui_bar_option_string[index_option]);

    ptr_option = config_file_search_option (
        weechat_config_file,
        weechat_config_section_bar,
        option_name);

    if (ptr_option)
        config_file_option_set_default (ptr_option, value, 1);
}

/*
 * Creates a new bar with options.
 *
 * Returns pointer to new bar, NULL if error.
 */

struct t_gui_bar *
gui_bar_new_with_options (const char *name,
                          struct t_config_option *hidden,
                          struct t_config_option *priority,
                          struct t_config_option *type,
                          struct t_config_option *conditions,
                          struct t_config_option *position,
                          struct t_config_option *filling_top_bottom,
                          struct t_config_option *filling_left_right,
                          struct t_config_option *size,
                          struct t_config_option *size_max,
                          struct t_config_option *color_fg,
                          struct t_config_option *color_delim,
                          struct t_config_option *color_bg,
                          struct t_config_option *color_bg_inactive,
                          struct t_config_option *separator,
                          struct t_config_option *items)
{
    struct t_gui_bar *new_bar;
    struct t_gui_window *ptr_win;

    /* create bar */
    new_bar = gui_bar_alloc (name);
    if (!new_bar)
        return NULL;

    new_bar->options[GUI_BAR_OPTION_HIDDEN] = hidden;
    new_bar->options[GUI_BAR_OPTION_PRIORITY] = priority;
    new_bar->options[GUI_BAR_OPTION_TYPE] = type;
    new_bar->options[GUI_BAR_OPTION_CONDITIONS] = conditions;
    new_bar->options[GUI_BAR_OPTION_POSITION] = position;
    new_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM] = filling_top_bottom;
    new_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT] = filling_left_right;
    new_bar->options[GUI_BAR_OPTION_SIZE] = size;
    new_bar->options[GUI_BAR_OPTION_SIZE_MAX] = size_max;
    new_bar->options[GUI_BAR_OPTION_COLOR_FG] = color_fg;
    new_bar->options[GUI_BAR_OPTION_COLOR_DELIM] = color_delim;
    new_bar->options[GUI_BAR_OPTION_COLOR_BG] = color_bg;
    new_bar->options[GUI_BAR_OPTION_COLOR_BG_INACTIVE] = color_bg_inactive;
    new_bar->options[GUI_BAR_OPTION_SEPARATOR] = separator;
    new_bar->options[GUI_BAR_OPTION_ITEMS] = items;
    new_bar->items_count = 0;
    new_bar->items_subcount = NULL;
    new_bar->items_array = NULL;
    new_bar->items_buffer = NULL;
    new_bar->items_prefix = NULL;
    new_bar->items_name = NULL;
    new_bar->items_suffix = NULL;
    gui_bar_set_items_array (new_bar, CONFIG_STRING(items));
    new_bar->bar_window = NULL;
    new_bar->bar_refresh_needed = 1;

    /* add bar to bars list */
    gui_bar_insert (new_bar);

    /* add window bar */
    if (CONFIG_INTEGER(new_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
    {
        /* create only one window for bar */
        gui_bar_window_new (new_bar, NULL);
        gui_window_ask_refresh (1);
    }
    else
    {
        /* create bar window for all opened windows */
        for (ptr_win = gui_windows; ptr_win;
             ptr_win = ptr_win->next_window)
        {
            gui_bar_window_new (new_bar, ptr_win);
        }
    }

    return new_bar;
}

/*
 * Creates a new bar.
 *
 * Returns pointer to new bar, NULL if error.
 */

struct t_gui_bar *
gui_bar_new (const char *name, const char *hidden, const char *priority,
             const char *type, const char *conditions, const char *position,
             const char *filling_top_bottom, const char *filling_left_right,
             const char *size, const char *size_max,
             const char *color_fg, const char *color_delim,
             const char *color_bg, const char *color_bg_inactive,
             const char *separator, const char *items)
{
    struct t_config_option *option_hidden, *option_priority, *option_type;
    struct t_config_option *option_conditions, *option_position;
    struct t_config_option *option_filling_top_bottom, *option_filling_left_right;
    struct t_config_option *option_size, *option_size_max;
    struct t_config_option *option_color_fg, *option_color_delim;
    struct t_config_option *option_color_bg, *option_color_bg_inactive;
    struct t_config_option *option_separator, *option_items;
    struct t_gui_bar *ptr_bar;

    if (!name || !name[0])
        return NULL;

    /* look for type */
    if (gui_bar_search_type (type) < 0)
        return NULL;

    /* look for position */
    if (gui_bar_search_position (position) < 0)
        return NULL;

    ptr_bar = gui_bar_search (name);
    if (ptr_bar)
    {
        /* bar already exists: just update default value of options */
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_HIDDEN, hidden);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_PRIORITY, priority);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_TYPE, type);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_CONDITIONS, conditions);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_POSITION, position);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_FILLING_TOP_BOTTOM, filling_top_bottom);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_FILLING_LEFT_RIGHT, filling_left_right);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_SIZE, size);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_SIZE_MAX, size_max);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_COLOR_FG, color_fg);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_COLOR_DELIM, color_delim);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_COLOR_BG, color_bg);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_COLOR_BG_INACTIVE, color_bg_inactive);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_SEPARATOR, separator);
        gui_bar_set_default_value (ptr_bar, GUI_BAR_OPTION_ITEMS, items);
    }
    else
    {
        /* create bar options */
        option_hidden = gui_bar_create_option (
            name, GUI_BAR_OPTION_HIDDEN, hidden);
        option_priority = gui_bar_create_option (
            name, GUI_BAR_OPTION_PRIORITY, priority);
        option_type = gui_bar_create_option (
            name, GUI_BAR_OPTION_TYPE, type);
        option_conditions = gui_bar_create_option (
            name, GUI_BAR_OPTION_CONDITIONS, conditions);
        option_position = gui_bar_create_option (
            name, GUI_BAR_OPTION_POSITION, position);
        option_filling_top_bottom = gui_bar_create_option (
            name, GUI_BAR_OPTION_FILLING_TOP_BOTTOM, filling_top_bottom);
        option_filling_left_right = gui_bar_create_option (
            name, GUI_BAR_OPTION_FILLING_LEFT_RIGHT, filling_left_right);
        option_size = gui_bar_create_option (
            name, GUI_BAR_OPTION_SIZE, size);
        option_size_max = gui_bar_create_option (
            name, GUI_BAR_OPTION_SIZE_MAX, size_max);
        option_color_fg = gui_bar_create_option (
            name, GUI_BAR_OPTION_COLOR_FG, color_fg);
        option_color_delim = gui_bar_create_option (
            name, GUI_BAR_OPTION_COLOR_DELIM, color_delim);
        option_color_bg = gui_bar_create_option (name, GUI_BAR_OPTION_COLOR_BG,
                                                 color_bg);
        option_color_bg_inactive = gui_bar_create_option (
            name, GUI_BAR_OPTION_COLOR_BG_INACTIVE, color_bg_inactive);
        option_separator = gui_bar_create_option (
            name, GUI_BAR_OPTION_SEPARATOR,
            (config_file_string_to_boolean (separator)) ? "on" : "off");
        option_items = gui_bar_create_option (
            name, GUI_BAR_OPTION_ITEMS, items);

        /* add bar */
        ptr_bar = gui_bar_new_with_options (
            name, option_hidden,
            option_priority, option_type,
            option_conditions, option_position,
            option_filling_top_bottom,
            option_filling_left_right,
            option_size, option_size_max,
            option_color_fg, option_color_delim,
            option_color_bg, option_color_bg_inactive,
            option_separator,
            option_items);
        if (!ptr_bar)
        {
            if (option_hidden)
                config_file_option_free (option_hidden, 0);
            if (option_priority)
                config_file_option_free (option_priority, 0);
            if (option_type)
                config_file_option_free (option_type, 0);
            if (option_conditions)
                config_file_option_free (option_conditions, 0);
            if (option_position)
                config_file_option_free (option_position, 0);
            if (option_filling_top_bottom)
                config_file_option_free (option_filling_top_bottom, 0);
            if (option_filling_left_right)
                config_file_option_free (option_filling_left_right, 0);
            if (option_size)
                config_file_option_free (option_size, 0);
            if (option_size_max)
                config_file_option_free (option_size_max, 0);
            if (option_color_fg)
                config_file_option_free (option_color_fg, 0);
            if (option_color_delim)
                config_file_option_free (option_color_delim, 0);
            if (option_color_bg)
                config_file_option_free (option_color_bg, 0);
            if (option_separator)
                config_file_option_free (option_separator, 0);
            if (option_items)
                config_file_option_free (option_items, 0);
        }
    }

    return ptr_bar;
}

/*
 * Creates a default bar.
 *
 * Returns pointer to new bar, NULL if error.
 */

struct t_gui_bar *
gui_bar_new_default (enum t_gui_bar_default bar)
{
    return gui_bar_new (
        gui_bar_default_name[bar],
        gui_bar_default_values[bar][GUI_BAR_OPTION_HIDDEN],
        gui_bar_default_values[bar][GUI_BAR_OPTION_PRIORITY],
        gui_bar_default_values[bar][GUI_BAR_OPTION_TYPE],
        gui_bar_default_values[bar][GUI_BAR_OPTION_CONDITIONS],
        gui_bar_default_values[bar][GUI_BAR_OPTION_POSITION],
        gui_bar_default_values[bar][GUI_BAR_OPTION_FILLING_TOP_BOTTOM],
        gui_bar_default_values[bar][GUI_BAR_OPTION_FILLING_LEFT_RIGHT],
        gui_bar_default_values[bar][GUI_BAR_OPTION_SIZE],
        gui_bar_default_values[bar][GUI_BAR_OPTION_SIZE_MAX],
        gui_bar_default_values[bar][GUI_BAR_OPTION_COLOR_FG],
        gui_bar_default_values[bar][GUI_BAR_OPTION_COLOR_DELIM],
        gui_bar_default_values[bar][GUI_BAR_OPTION_COLOR_BG],
        gui_bar_default_values[bar][GUI_BAR_OPTION_COLOR_BG_INACTIVE],
        gui_bar_default_values[bar][GUI_BAR_OPTION_SEPARATOR],
        gui_bar_default_values[bar][GUI_BAR_OPTION_ITEMS]);
}

/*
 * Uses temporary bars (created by reading configuration file).
 */

void
gui_bar_use_temp_bars ()
{
    struct t_gui_bar *ptr_temp_bar, *next_temp_bar;
    int i, num_options_ok;

    for (ptr_temp_bar = gui_temp_bars; ptr_temp_bar;
         ptr_temp_bar = ptr_temp_bar->next_bar)
    {
        num_options_ok = 0;
        for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
        {
            if (!ptr_temp_bar->options[i])
            {
                ptr_temp_bar->options[i] = gui_bar_create_option (
                    ptr_temp_bar->name,
                    i,
                    gui_bar_option_default[i]);
            }
            if (ptr_temp_bar->options[i])
                num_options_ok++;
        }

        if (num_options_ok == GUI_BAR_NUM_OPTIONS)
        {
            gui_bar_new_with_options (
                ptr_temp_bar->name,
                ptr_temp_bar->options[GUI_BAR_OPTION_HIDDEN],
                ptr_temp_bar->options[GUI_BAR_OPTION_PRIORITY],
                ptr_temp_bar->options[GUI_BAR_OPTION_TYPE],
                ptr_temp_bar->options[GUI_BAR_OPTION_CONDITIONS],
                ptr_temp_bar->options[GUI_BAR_OPTION_POSITION],
                ptr_temp_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM],
                ptr_temp_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT],
                ptr_temp_bar->options[GUI_BAR_OPTION_SIZE],
                ptr_temp_bar->options[GUI_BAR_OPTION_SIZE_MAX],
                ptr_temp_bar->options[GUI_BAR_OPTION_COLOR_FG],
                ptr_temp_bar->options[GUI_BAR_OPTION_COLOR_DELIM],
                ptr_temp_bar->options[GUI_BAR_OPTION_COLOR_BG],
                ptr_temp_bar->options[GUI_BAR_OPTION_COLOR_BG_INACTIVE],
                ptr_temp_bar->options[GUI_BAR_OPTION_SEPARATOR],
                ptr_temp_bar->options[GUI_BAR_OPTION_ITEMS]);
        }
        else
        {
            for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
            {
                if (ptr_temp_bar->options[i])
                {
                    config_file_option_free (ptr_temp_bar->options[i], 0);
                    ptr_temp_bar->options[i] = NULL;
                }
            }
        }
    }

    /* free all temporary bars */
    while (gui_temp_bars)
    {
        next_temp_bar = gui_temp_bars->next_bar;

        if (gui_temp_bars->name)
            free (gui_temp_bars->name);
        free (gui_temp_bars);

        gui_temp_bars = next_temp_bar;
    }
    last_gui_temp_bar = NULL;
}

/*
 * Creates default input bar if it does not exist.
 */

void
gui_bar_create_default_input ()
{
    struct t_gui_bar *ptr_bar;
    int length;
    char *buf;

    /* search an input_text item */
    if (!gui_bar_item_used_in_at_least_one_bar (gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT],
                                                1, 0))
    {
        ptr_bar = gui_bar_search (gui_bar_default_name[GUI_BAR_DEFAULT_INPUT]);
        if (ptr_bar)
        {
            /* add item "input_text" to input bar */
            length = 1;
            if (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]))
                length += strlen (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]));
            length += 1; /* "," */
            length += strlen (gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
            buf = malloc (length);
            if (buf)
            {
                snprintf (buf, length, "%s,%s",
                          (CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS])) ?
                          CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]) : "",
                          gui_bar_item_names[GUI_BAR_ITEM_INPUT_TEXT]);
                config_file_option_set (ptr_bar->options[GUI_BAR_OPTION_ITEMS], buf, 1);
                gui_chat_printf (NULL, _("Bar \"%s\" updated"),
                                 gui_bar_default_name[GUI_BAR_DEFAULT_INPUT]);
                gui_bar_draw (ptr_bar);
                free (buf);
            }
        }
        else
        {
            /* create input bar */
            if (gui_bar_new_default (GUI_BAR_DEFAULT_INPUT))
            {
                gui_chat_printf (NULL, _("Bar \"%s\" created"),
                                 gui_bar_default_name[GUI_BAR_DEFAULT_INPUT]);
            }
        }
    }
}

/*
 * Creates default title bar if it does not exist.
 */

void
gui_bar_create_default_title ()
{
    struct t_gui_bar *ptr_bar;

    /* search title bar */
    ptr_bar = gui_bar_search (gui_bar_default_name[GUI_BAR_DEFAULT_TITLE]);
    if (!ptr_bar)
    {
        /* create title bar */
        if (gui_bar_new_default (GUI_BAR_DEFAULT_TITLE))
        {
            gui_chat_printf (NULL, _("Bar \"%s\" created"),
                             gui_bar_default_name[GUI_BAR_DEFAULT_TITLE]);
        }
    }
}

/*
 * Creates default status bar if it does not exist.
 */

void
gui_bar_create_default_status ()
{
    struct t_gui_bar *ptr_bar;

    /* search status bar */
    ptr_bar = gui_bar_search (gui_bar_default_name[GUI_BAR_DEFAULT_STATUS]);
    if (!ptr_bar)
    {
        /* create status bar */
        if (gui_bar_new_default (GUI_BAR_DEFAULT_STATUS))
        {
            gui_chat_printf (NULL, _("Bar \"%s\" created"),
                             gui_bar_default_name[GUI_BAR_DEFAULT_STATUS]);
        }
    }
}

/*
 * Creates default nicklist bar if it does not exist.
 */

void
gui_bar_create_default_nicklist ()
{
    struct t_gui_bar *ptr_bar;

    /* search nicklist bar */
    ptr_bar = gui_bar_search (gui_bar_default_name[GUI_BAR_DEFAULT_NICKLIST]);
    if (!ptr_bar)
    {
        /* create nicklist bar */
        if (gui_bar_new_default (GUI_BAR_DEFAULT_NICKLIST))
        {
            gui_chat_printf (NULL, _("Bar \"%s\" created"),
                             gui_bar_default_name[GUI_BAR_DEFAULT_NICKLIST]);
        }
    }
}

/*
 * Creates default bars if they do not exist.
 */

void
gui_bar_create_default ()
{
    gui_bar_create_default_input ();
    gui_bar_create_default_title ();
    gui_bar_create_default_status ();
    gui_bar_create_default_nicklist ();
}

/*
 * Updates a bar on screen.
 */

void
gui_bar_update (const char *name)
{
    struct t_gui_bar *ptr_bar;

    if (!name)
        return;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        if (!CONFIG_BOOLEAN(ptr_bar->options[GUI_BAR_OPTION_HIDDEN])
            && (strcmp (ptr_bar->name, name) == 0))
        {
            gui_bar_ask_refresh (ptr_bar);
        }
    }
}

/*
 * Scrolls a bar.
 *
 * Returns:
 *   1: scroll OK
 *   0: error
 */

int
gui_bar_scroll (struct t_gui_bar *bar, struct t_gui_window *window,
                const char *scroll)
{
    struct t_gui_bar_window *ptr_bar_win;
    long number;
    char *str, *error;
    int length, add_x, add, percent, scroll_beginning, scroll_end;

    if (!bar)
        return 0;

    if (CONFIG_BOOLEAN(bar->options[GUI_BAR_OPTION_HIDDEN]))
        return 1;

    add_x = 0;
    str = NULL;
    number = 0;
    add = 0;
    percent = 0;
    scroll_beginning = 0;
    scroll_end = 0;

    if ((scroll[0] == 'x') || (scroll[0] == 'X'))
    {
        add_x = 1;
        scroll++;
    }
    else if ((scroll[0] == 'y') || (scroll[0] == 'Y'))
    {
        scroll++;
    }
    else
    {
        /* auto-detect if we scroll X/Y, according to filling */
        if (gui_bar_get_filling (bar) == GUI_BAR_FILLING_HORIZONTAL)
            add_x = 1;
    }

    if ((scroll[0] == 'b') || (scroll[0] == 'B'))
    {
        scroll_beginning = 1;
    }
    else if ((scroll[0] == 'e') || (scroll[0] == 'E'))
    {
        scroll_end = 1;
    }
    else
    {
        if (scroll[0] == '+')
        {
            add = 1;
            scroll++;
        }
        else if (scroll[0] == '-')
        {
            scroll++;
        }
        else
            return 0;

        length = strlen (scroll);
        if (length == 0)
            return 0;

        if (scroll[length - 1] == '%')
        {
            str = string_strndup (scroll, length - 1);
            percent = 1;
        }
        else
            str = strdup (scroll);
        if (!str)
            return 0;

        error = NULL;
        number = strtol (str, &error, 10);

        if (!error || error[0] || (number <= 0))
        {
            free (str);
            return 0;
        }
    }

    if (CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
    {
        gui_bar_window_scroll (bar->bar_window, NULL,
                               add_x, scroll_beginning, scroll_end,
                               add, percent, number);
    }
    else if (window)
    {
        for (ptr_bar_win = window->bar_windows; ptr_bar_win;
             ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            if (ptr_bar_win->bar == bar)
            {
                gui_bar_window_scroll (ptr_bar_win, window,
                                       add_x, scroll_beginning, scroll_end,
                                       add, percent, number);
            }
        }
    }

    if (str)
        free (str);

    return 1;
}

/*
 * Deletes a bar.
 */

void
gui_bar_free (struct t_gui_bar *bar)
{
    int i;

    if (!bar)
        return;

    /* remove bar window(s) */
    if (bar->bar_window)
    {
        gui_bar_window_free (bar->bar_window, NULL);
        gui_window_ask_refresh (1);
    }
    else
        gui_bar_free_bar_windows (bar);

    /* remove bar from bars list */
    if (bar->prev_bar)
        (bar->prev_bar)->next_bar = bar->next_bar;
    if (bar->next_bar)
        (bar->next_bar)->prev_bar = bar->prev_bar;
    if (gui_bars == bar)
        gui_bars = bar->next_bar;
    if (last_gui_bar == bar)
        last_gui_bar = bar->prev_bar;

    /* free data */
    if (bar->name)
        free (bar->name);
    for (i = 0; i < GUI_BAR_NUM_OPTIONS; i++)
    {
        if (bar->options[i])
            config_file_option_free (bar->options[i], 1);
    }
    gui_bar_free_items_arrays (bar);

    free (bar);
}

/*
 * Deletes all bars.
 */

void
gui_bar_free_all ()
{
    while (gui_bars)
    {
        gui_bar_free (gui_bars);
    }
}

/*
 * Frees bar windows for a bar.
 */

void
gui_bar_free_bar_windows (struct t_gui_bar *bar)
{
    struct t_gui_window *ptr_win;
    struct t_gui_bar_window *ptr_bar_win, *next_bar_win;

    if (bar->bar_window)
    {
        gui_bar_window_free (bar->bar_window, NULL);
    }
    else
    {
        for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
        {
            ptr_bar_win = ptr_win->bar_windows;
            while (ptr_bar_win)
            {
                next_bar_win = ptr_bar_win->next_bar_window;

                if (ptr_bar_win->bar == bar)
                    gui_bar_window_free (ptr_bar_win, ptr_win);

                ptr_bar_win = next_bar_win;
            }
        }
    }
}

/*
 * Returns hdata for bar.
 */

struct t_hdata *
gui_bar_hdata_bar_cb (const void *pointer, void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_bar", "next_bar",
                       0, 0, NULL, NULL);
    if (hdata)
    {
        HDATA_VAR(struct t_gui_bar, name, STRING, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, options, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, items_count, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, items_subcount, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, items_array, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, items_buffer, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, items_prefix, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, items_name, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, items_suffix, POINTER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, bar_window, POINTER, 0, NULL, "bar_window");
        HDATA_VAR(struct t_gui_bar, bar_refresh_needed, INTEGER, 0, NULL, NULL);
        HDATA_VAR(struct t_gui_bar, prev_bar, POINTER, 0, NULL, hdata_name);
        HDATA_VAR(struct t_gui_bar, next_bar, POINTER, 0, NULL, hdata_name);
        HDATA_LIST(gui_bars, WEECHAT_HDATA_LIST_CHECK_POINTERS);
        HDATA_LIST(last_gui_bar, 0);
    }
    return hdata;
}

/*
 * Adds a bar in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
gui_bar_add_to_infolist (struct t_infolist *infolist,
                         struct t_gui_bar *bar)
{
    struct t_infolist_item *ptr_item;
    int i, j;
    char option_name[64];

    if (!infolist || !bar)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_string (ptr_item, "name", bar->name))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "hidden", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_HIDDEN])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "priority", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_PRIORITY])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "type", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_TYPE])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "conditions", CONFIG_STRING(bar->options[GUI_BAR_OPTION_CONDITIONS])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "position", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_POSITION])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "filling_top_bottom", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "filling_left_right", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "size", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "size_max", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SIZE_MAX])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_fg", gui_color_get_name (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_FG]))))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_delim", gui_color_get_name (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_DELIM]))))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_bg", gui_color_get_name (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_BG]))))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color_bg_inactive", gui_color_get_name (CONFIG_COLOR(bar->options[GUI_BAR_OPTION_COLOR_BG_INACTIVE]))))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "separator", CONFIG_INTEGER(bar->options[GUI_BAR_OPTION_SEPARATOR])))
        return 0;
    if (!infolist_new_var_string (ptr_item, "items", CONFIG_STRING(bar->options[GUI_BAR_OPTION_ITEMS])))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "items_count", bar->items_count))
        return 0;
    for (i = 0; i < bar->items_count; i++)
    {
        for (j = 0; j < bar->items_subcount[i]; j++)
        {
            snprintf (option_name, sizeof (option_name),
                      "items_array_%05d_%05d", i + 1, j + 1);
            if (!infolist_new_var_string (ptr_item, option_name,
                                          bar->items_array[i][j]))
                return 0;
            snprintf (option_name, sizeof (option_name),
                      "items_buffer_%05d_%05d", i + 1, j + 1);
            if (!infolist_new_var_string (ptr_item, option_name,
                                          bar->items_buffer[i][j]))
                return 0;
            snprintf (option_name, sizeof (option_name),
                      "items_prefix_%05d_%05d", i + 1, j + 1);
            if (!infolist_new_var_string (ptr_item, option_name,
                                          bar->items_prefix[i][j]))
                return 0;
            snprintf (option_name, sizeof (option_name),
                      "items_suffix_%05d_%05d", i + 1, j + 1);
            if (!infolist_new_var_string (ptr_item, option_name,
                                          bar->items_suffix[i][j]))
                return 0;
        }
    }
    if (!infolist_new_var_pointer (ptr_item, "bar_window", bar->bar_window))
        return 0;

    return 1;
}

/*
 * Prints bar infos in WeeChat log file (usually for crash dump).
 */

void
gui_bar_print_log ()
{
    struct t_gui_bar *ptr_bar;
    int i, j;

    for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
    {
        log_printf ("");
        log_printf ("[bar (addr:0x%lx)]", ptr_bar);
        log_printf ("  name . . . . . . . . . : '%s'",  ptr_bar->name);
        log_printf ("  hidden . . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_HIDDEN]));
        log_printf ("  priority . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_PRIORITY]));
        log_printf ("  type . . . . . . . . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]),
                    gui_bar_type_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE])]);
        log_printf ("  conditions . . . . . . : '%s'",  CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_CONDITIONS]));
        log_printf ("  position . . . . . . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION]),
                    gui_bar_position_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_POSITION])]);
        log_printf ("  filling_top_bottom . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM]),
                    gui_bar_filling_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_TOP_BOTTOM])]);
        log_printf ("  filling_left_right . . : %d (%s)",
                    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT]),
                    gui_bar_filling_string[CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_FILLING_LEFT_RIGHT])]);
        log_printf ("  size . . . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE]));
        log_printf ("  size_max . . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SIZE_MAX]));
        log_printf ("  color_fg . . . . . . . : %d (%s)",
                    CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_FG]),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_FG])));
        log_printf ("  color_delim. . . . . . : %d (%s)",
                    CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_DELIM]),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_DELIM])));
        log_printf ("  color_bg . . . . . . . : %d (%s)",
                    CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_BG]),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_BG])));
        log_printf ("  color_bg_inactive. . . : %d (%s)",
                    CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_BG_INACTIVE]),
                    gui_color_get_name (CONFIG_COLOR(ptr_bar->options[GUI_BAR_OPTION_COLOR_BG_INACTIVE])));
        log_printf ("  separator. . . . . . . : %d",    CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_SEPARATOR]));
        log_printf ("  items. . . . . . . . . : '%s'",  CONFIG_STRING(ptr_bar->options[GUI_BAR_OPTION_ITEMS]));
        log_printf ("  items_count. . . . . . : %d",    ptr_bar->items_count);
        for (i = 0; i < ptr_bar->items_count; i++)
        {
            log_printf ("    items_subcount[%03d]. : %d",
                        i, ptr_bar->items_subcount[i]);
            for (j = 0; j < ptr_bar->items_subcount[i]; j++)
            {
                log_printf ("    items_array[%03d][%03d]: '%s' "
                            "(buffer: '%s', prefix: '%s', name: '%s', suffix: '%s')",
                            i, j,
                            ptr_bar->items_array[i][j],
                            ptr_bar->items_buffer[i][j],
                            ptr_bar->items_prefix[i][j],
                            ptr_bar->items_name[i][j],
                            ptr_bar->items_suffix[i][j]);
            }
        }
        log_printf ("  bar_window . . . . . . : 0x%lx", ptr_bar->bar_window);
        log_printf ("  bar_refresh_needed . . : %d",    ptr_bar->bar_refresh_needed);
        log_printf ("  prev_bar . . . . . . . : 0x%lx", ptr_bar->prev_bar);
        log_printf ("  next_bar . . . . . . . : 0x%lx", ptr_bar->next_bar);

        if (ptr_bar->bar_window)
            gui_bar_window_print_log (ptr_bar->bar_window);
    }
}
