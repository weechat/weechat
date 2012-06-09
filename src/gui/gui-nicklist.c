/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * gui-nicklist.c: nicklist functions (used by all GUI)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "../core/weechat.h"
#include "../core/wee-config.h"
#include "../core/wee-hdata.h"
#include "../core/wee-hook.h"
#include "../core/wee-infolist.h"
#include "../core/wee-log.h"
#include "../core/wee-string.h"
#include "../core/wee-utf8.h"
#include "../plugins/plugin.h"
#include "gui-nicklist.h"
#include "gui-buffer.h"
#include "gui-color.h"


/*
 * gui_nicklist_send_signal: send a signal when something has changed in
 *                           nicklist
 */

void
gui_nicklist_send_signal (const char *signal, struct t_gui_buffer *buffer,
                          const char *arguments)
{
    char *str_args;
    int length;

    if (buffer)
    {
        length = 128 + ((arguments) ? strlen (arguments) : 0) + 1 + 1;
        str_args = malloc (length);
        if (str_args)
        {
            snprintf (str_args, length,
                      "0x%lx,%s",
                      (long unsigned int)(buffer),
                      (arguments) ? arguments : "");
            hook_signal_send (signal, WEECHAT_HOOK_SIGNAL_STRING, str_args);
            free (str_args);
        }
    }
    else
    {
        hook_signal_send (signal, WEECHAT_HOOK_SIGNAL_STRING,
                          (char *)arguments);
    }
}

/*
 * gui_nicklist_find_pos_group: find position for a group (for sorting nicklist)
 */

struct t_gui_nick_group *
gui_nicklist_find_pos_group (struct t_gui_nick_group *groups,
                             struct t_gui_nick_group *group)
{
    struct t_gui_nick_group *ptr_group;

    for (ptr_group = groups; ptr_group; ptr_group = ptr_group->next_group)
    {
        if (string_strcasecmp (group->name, ptr_group->name) < 0)
            return ptr_group;
    }

    /* group will be inserted at end of list */
    return NULL;
}

/*
 * gui_nicklist_insert_group_sorted: insert group into sorted list
 */

void
gui_nicklist_insert_group_sorted (struct t_gui_nick_group **groups,
                                  struct t_gui_nick_group **last_group,
                                  struct t_gui_nick_group *group)
{
    struct t_gui_nick_group *pos_group;

    if (*groups)
    {
        pos_group = gui_nicklist_find_pos_group (*groups, group);

        if (pos_group)
        {
            /* insert group into the list (before group found) */
            group->prev_group = pos_group->prev_group;
            group->next_group = pos_group;
            if (pos_group->prev_group)
                (pos_group->prev_group)->next_group = group;
            else
                *groups = group;
            pos_group->prev_group = group;
        }
        else
        {
            /* add group to the end */
            group->prev_group = *last_group;
            group->next_group = NULL;
            (*last_group)->next_group = group;
            *last_group = group;
        }
    }
    else
    {
        group->prev_group = NULL;
        group->next_group = NULL;
        *groups = group;
        *last_group = group;
    }
}

/*
 * gui_nicklist_search_group_internal: search a group in buffer nicklist
 */

struct t_gui_nick_group *
gui_nicklist_search_group_internal (struct t_gui_buffer *buffer,
                                    struct t_gui_nick_group *from_group,
                                    const char *name,
                                    int skip_digits)
{
    struct t_gui_nick_group *ptr_group;
    const char *ptr_name;

    if (!buffer)
        return NULL;

    if (!from_group)
        from_group = buffer->nicklist_root;

    if (!from_group)
        return NULL;

    if (from_group->children)
    {
        ptr_group = gui_nicklist_search_group_internal (buffer,
                                                        from_group->children,
                                                        name,
                                                        skip_digits);
        if (ptr_group)
            return ptr_group;
    }

    ptr_group = from_group;
    while (ptr_group)
    {
        ptr_name = (skip_digits) ?
            gui_nicklist_get_group_start(ptr_group->name) : ptr_group->name;
        if (strcmp (ptr_name, name) == 0)
            return ptr_group;
        ptr_group = ptr_group->next_group;
    }

    /* group not found */
    return NULL;
}

/*
 * gui_nicklist_search_group: search a group in buffer nicklist
 */

struct t_gui_nick_group *
gui_nicklist_search_group (struct t_gui_buffer *buffer,
                           struct t_gui_nick_group *from_group,
                           const char *name)
{
    const char *ptr_name;

    ptr_name = gui_nicklist_get_group_start (name);

    return gui_nicklist_search_group_internal (buffer, from_group, name,
                                               (ptr_name == name) ? 1 : 0);
}

/*
 * gui_nicklist_add_group: add a group to nicklist for a buffer
 */

struct t_gui_nick_group *
gui_nicklist_add_group (struct t_gui_buffer *buffer,
                        struct t_gui_nick_group *parent_group, const char *name,
                        const char *color, int visible)
{
    struct t_gui_nick_group *new_group;

    if (!buffer || !name || gui_nicklist_search_group (buffer, parent_group, name))
        return NULL;

    new_group = malloc (sizeof (*new_group));
    if (!new_group)
        return NULL;

    new_group->name = strdup (name);
    new_group->color = (color) ? strdup (color) : NULL;
    new_group->visible = visible;
    new_group->parent = (parent_group) ? parent_group : buffer->nicklist_root;
    new_group->level = (new_group->parent) ? new_group->parent->level + 1 : 0;
    new_group->children = NULL;
    new_group->last_child = NULL;
    new_group->nicks = NULL;
    new_group->last_nick = NULL;
    new_group->prev_group = NULL;
    new_group->next_group = NULL;

    if (new_group->parent)
    {
        gui_nicklist_insert_group_sorted (&(new_group->parent->children),
                                          &(new_group->parent->last_child),
                                          new_group);
    }
    else
    {
        buffer->nicklist_root = new_group;
    }

    if (buffer->nicklist_display_groups && visible)
        buffer->nicklist_visible_count++;

    gui_nicklist_send_signal ("nicklist_group_added", buffer, name);

    return new_group;
}

/*
 * gui_nicklist_find_pos_nick: find position for a nick (for sorting nicklist)
 */

struct t_gui_nick *
gui_nicklist_find_pos_nick (struct t_gui_nick_group *group,
                            struct t_gui_nick *nick)
{
    struct t_gui_nick *ptr_nick;

    if (!group)
        return NULL;

    for (ptr_nick = group->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (string_strcasecmp (nick->name, ptr_nick->name) < 0)
            return ptr_nick;
    }

    /* nick will be inserted at end of list */
    return NULL;
}

/*
 * gui_nicklist_insert_nick_sorted: insert nick into sorted list
 */

void
gui_nicklist_insert_nick_sorted (struct t_gui_nick_group *group,
                                 struct t_gui_nick *nick)
{
    struct t_gui_nick *pos_nick;

    if (group->nicks)
    {
        pos_nick = gui_nicklist_find_pos_nick (group, nick);

        if (pos_nick)
        {
            /* insert nick into the list (before nick found) */
            nick->prev_nick = pos_nick->prev_nick;
            nick->next_nick = pos_nick;
            if (pos_nick->prev_nick)
                (pos_nick->prev_nick)->next_nick = nick;
            else
                group->nicks = nick;
            pos_nick->prev_nick = nick;
        }
        else
        {
            /* add nick to the end */
            nick->prev_nick = group->last_nick;
            nick->next_nick = NULL;
            group->last_nick->next_nick = nick;
            group->last_nick = nick;
        }
    }
    else
    {
        nick->prev_nick = NULL;
        nick->next_nick = NULL;
        group->nicks = nick;
        group->last_nick = nick;
    }
}

/*
 * gui_nicklist_search_nick: search a nick in buffer nicklist
 */

struct t_gui_nick *
gui_nicklist_search_nick (struct t_gui_buffer *buffer,
                          struct t_gui_nick_group *from_group,
                          const char *name)
{
    struct t_gui_nick *ptr_nick;
    struct t_gui_nick_group *ptr_group;

    if (!buffer && !from_group)
        return NULL;

    if (!from_group && !buffer->nicklist_root)
        return NULL;

    for (ptr_nick = (from_group) ? from_group->nicks : buffer->nicklist_root->nicks;
         ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (strcmp (ptr_nick->name, name) == 0)
            return ptr_nick;
    }

    /* search nick in child groups */
    for (ptr_group = (from_group) ? from_group->children : buffer->nicklist_root->children;
         ptr_group; ptr_group = ptr_group->next_group)
    {
        ptr_nick = gui_nicklist_search_nick (buffer, ptr_group, name);
        if (ptr_nick)
            return ptr_nick;
    }

    /* nick not found */
    return NULL;
}

/*
 * gui_nicklist_add_nick: add a nick to nicklist for a buffer
 */

struct t_gui_nick *
gui_nicklist_add_nick (struct t_gui_buffer *buffer,
                       struct t_gui_nick_group *group,
                       const char *name, const char *color,
                       const char *prefix, const char *prefix_color,
                       int visible)
{
    struct t_gui_nick *new_nick;

    if (!buffer || !name || gui_nicklist_search_nick (buffer, NULL, name))
        return NULL;

    new_nick = malloc (sizeof (*new_nick));
    if (!new_nick)
        return NULL;

    new_nick->group = (group) ? group : buffer->nicklist_root;
    new_nick->name = strdup (name);
    new_nick->color = (color) ? strdup (color) : NULL;
    new_nick->prefix = (prefix) ? strdup (prefix) : NULL;
    new_nick->prefix_color = (prefix_color) ? strdup (prefix_color) : NULL;
    new_nick->visible = visible;

    gui_nicklist_insert_nick_sorted (new_nick->group, new_nick);

    if (visible)
        buffer->nicklist_visible_count++;

    if (CONFIG_BOOLEAN(config_look_color_nick_offline))
        gui_buffer_ask_chat_refresh (buffer, 1);

    gui_nicklist_send_signal ("nicklist_nick_added", buffer, name);

    return new_nick;
}

/*
 * gui_nicklist_remove_nick: remove a nick from a group
 */

void
gui_nicklist_remove_nick (struct t_gui_buffer *buffer,
                          struct t_gui_nick *nick)
{
    char *nick_removed;

    if (!buffer || !nick)
        return;

    nick_removed = (nick->name) ? strdup (nick->name) : NULL;

    /* remove nick from list */
    if (nick->prev_nick)
        (nick->prev_nick)->next_nick = nick->next_nick;
    if (nick->next_nick)
        (nick->next_nick)->prev_nick = nick->prev_nick;
    if ((nick->group)->nicks == nick)
        (nick->group)->nicks = nick->next_nick;
    if ((nick->group)->last_nick == nick)
        (nick->group)->last_nick = nick->prev_nick;

    /* free data */
    if (nick->name)
        free (nick->name);
    if (nick->color)
        free (nick->color);
    if (nick->prefix)
        free (nick->prefix);
    if (nick->prefix_color)
        free (nick->prefix_color);

    if (nick->visible)
    {
        if (buffer->nicklist_visible_count > 0)
            buffer->nicklist_visible_count--;
    }

    free (nick);

    if (CONFIG_BOOLEAN(config_look_color_nick_offline))
        gui_buffer_ask_chat_refresh (buffer, 1);

    gui_nicklist_send_signal ("nicklist_nick_removed", buffer, nick_removed);

    if (nick_removed)
        free (nick_removed);
}

/*
 * gui_nicklist_remove_group: remove a group from nicklist
 */

void
gui_nicklist_remove_group (struct t_gui_buffer *buffer,
                           struct t_gui_nick_group *group)
{
    char *group_removed;

    if (!buffer || !group)
        return;

    group_removed = (group->name) ? strdup (group->name) : NULL;

    /* remove children first */
    while (group->children)
    {
        gui_nicklist_remove_group (buffer, group->children);
    }

    /* remove nicks from group */
    while (group->nicks)
    {
        gui_nicklist_remove_nick (buffer, group->nicks);
    }

    if (group->parent)
    {
        /* remove group from list */
        if (group->prev_group)
            (group->prev_group)->next_group = group->next_group;
        if (group->next_group)
            (group->next_group)->prev_group = group->prev_group;
        if ((group->parent)->children == group)
            (group->parent)->children = group->next_group;
        if ((group->parent)->last_child == group)
            (group->parent)->last_child = group->prev_group;
    }
    else
    {
        buffer->nicklist_root = NULL;
    }

    /* free data */
    if (group->name)
        free (group->name);
    if (group->color)
        free (group->color);

    if (group->visible)
    {
        if (buffer->nicklist_display_groups
            && (buffer->nicklist_visible_count > 0))
            buffer->nicklist_visible_count--;
    }

    free (group);

    gui_nicklist_send_signal ("nicklist_group_removed", buffer, group_removed);

    if (group_removed)
        free (group_removed);
}

/*
 * gui_nicklist_remove_all: remove all nicks in nicklist
 */

void
gui_nicklist_remove_all (struct t_gui_buffer *buffer)
{
    if (buffer)
    {
        while (buffer->nicklist_root)
        {
            gui_nicklist_remove_group (buffer, buffer->nicklist_root);
        }
        gui_nicklist_add_group (buffer, NULL, "root", NULL, 0);
    }
}

/*
 * gui_nicklist_get_next_item: get next item (group or nick) of a group/nick
 */

void
gui_nicklist_get_next_item (struct t_gui_buffer *buffer,
                            struct t_gui_nick_group **group,
                            struct t_gui_nick **nick)
{
    struct t_gui_nick_group *ptr_group;

    if (!buffer)
        return;

    /* root group */
    if (!*group && !*nick)
    {
        *group = buffer->nicklist_root;
        return;
    }

    /* next nick */
    if (*nick && (*nick)->next_nick)
    {
        *nick = (*nick)->next_nick;
        return;
    }

    if (*group && !*nick)
    {
        /* first child */
        if ((*group)->children)
        {
            *group = (*group)->children;
            return;
        }
        /* first nick of current group */
        if ((*group)->nicks)
        {
            *nick = (*group)->nicks;
            return;
        }
        if ((*group)->next_group)
        {
            *group = (*group)->next_group;
            return;
        }
    }

    *nick = NULL;
    ptr_group = (*group) ? *group : buffer->nicklist_root;

    /* next group */
    if (ptr_group->next_group)
    {
        *group = ptr_group->next_group;
        return;
    }

    /* find next group by parents */
    while ((ptr_group = ptr_group->parent))
    {
        if (ptr_group->next_group)
        {
            *group = ptr_group->next_group;
            return;
        }
    }

    /* nothing found */
    *group = NULL;
}

/*
 * gui_nicklist_get_group_start: return first char of a group that will be
 *                               displayed on screen: if name begins with
 *                               some digits followed by '|', then start is
 *                               after '|', otherwise it's beginning of name
 */

const char *
gui_nicklist_get_group_start (const char *name)
{
    const char *ptr_name;

    ptr_name = name;
    while (isdigit ((unsigned char)ptr_name[0]))
    {
        if (ptr_name[0] == '|')
            break;
        ptr_name++;
    }
    if ((ptr_name[0] == '|') && (ptr_name != name))
        return ptr_name + 1;
    else
        return name;
}

/*
 * gui_nicklist_get_max_length: return longer nickname on a buffer
 */

int
gui_nicklist_get_max_length (struct t_gui_buffer *buffer,
                             struct t_gui_nick_group *group)
{
    int length, max_length;
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;

    if (!buffer)
        return 0;

    max_length = 0;
    for (ptr_group = (group) ? group : buffer->nicklist_root;
         ptr_group; ptr_group = ptr_group->next_group)
    {
        if (buffer->nicklist_display_groups && ptr_group->visible)
        {
            length = utf8_strlen_screen (gui_nicklist_get_group_start (ptr_group->name)) +
                                         ptr_group->level - 1;
            if (length > max_length)
                max_length = length;
        }
        for (ptr_nick = ptr_group->nicks; ptr_nick;
             ptr_nick = ptr_nick->next_nick)
        {
            if (ptr_nick->visible)
            {
                if (buffer->nicklist_display_groups)
                    length = utf8_strlen_screen (ptr_nick->name) + ptr_group->level + 1;
                else
                    length = utf8_strlen_screen (ptr_nick->name) + 1;
                if (length > max_length)
                    max_length = length;
            }
        }
        if (ptr_group->children)
        {
            length = gui_nicklist_get_max_length (buffer, ptr_group->children);
            if (length > max_length)
                max_length = length;
        }
    }
    return max_length;
}

/*
 * gui_nicklist_compute_visible_count: compute visible_count variable for a
 *                                     buffer
 */

void
gui_nicklist_compute_visible_count (struct t_gui_buffer *buffer,
                                    struct t_gui_nick_group *group)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;

    if (!buffer || !group)
        return;

    /* count for children */
    for (ptr_group = group->children; ptr_group;
         ptr_group = ptr_group->next_group)
    {
        gui_nicklist_compute_visible_count (buffer, ptr_group);
    }

    /* count current group */
    if (buffer->nicklist_display_groups && group->visible)
        buffer->nicklist_visible_count++;

    /* count nicks in group */
    for (ptr_nick = group->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        buffer->nicklist_visible_count++;
    }
}

/*
 * gui_nicklist_group_get_integer: get a group property as integer
 */

int
gui_nicklist_group_get_integer (struct t_gui_buffer *buffer,
                                struct t_gui_nick_group *group,
                                const char *property)
{
    /* make C compiler happy */
    (void) buffer;

    if (group && property)
    {
        if (string_strcasecmp (property, "visible") == 0)
            return group->visible;
        else if (string_strcasecmp (property, "level") == 0)
            return group->level;
    }

    return 0;
}

/*
 * gui_nicklist_group_get_string: get a group property as string
 */

const char *
gui_nicklist_group_get_string (struct t_gui_buffer *buffer,
                               struct t_gui_nick_group *group,
                               const char *property)
{
    /* make C compiler happy */
    (void) buffer;

    if (group && property)
    {
        if (string_strcasecmp (property, "name") == 0)
            return group->name;
        else if (string_strcasecmp (property, "color") == 0)
            return group->color;
    }

    return NULL;
}

/*
 * gui_nicklist_group_get_pointer: get a group property as pointer
 */

void *
gui_nicklist_group_get_pointer (struct t_gui_buffer *buffer,
                                struct t_gui_nick_group *group,
                                const char *property)
{
    /* make C compiler happy */
    (void) buffer;

    if (group && property)
    {
        if (string_strcasecmp (property, "parent") == 0)
            return group->parent;
    }

    return NULL;
}

/*
 * gui_nicklist_group_set: set a group property (string)
 */

void
gui_nicklist_group_set (struct t_gui_buffer *buffer,
                        struct t_gui_nick_group *group,
                        const char *property, const char *value)
{
    long number;
    char *error;
    int group_changed;

    if (!buffer || !group || !property || !value)
        return;

    group_changed = 0;

    if (string_strcasecmp (property, "color") == 0)
    {
        if (group->color)
            free (group->color);
        group->color = (value[0]) ? strdup (value) : NULL;
        group_changed = 1;
    }
    else if (string_strcasecmp (property, "visible") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            group->visible = (number) ? 1 : 0;
        group_changed = 1;
    }

    if (group_changed)
    {
        gui_nicklist_send_signal ("nicklist_group_changed", buffer,
                                  group->name);
    }
}

/*
 * gui_nicklist_nick_get_integer: get a nick property as integer
 */

int
gui_nicklist_nick_get_integer (struct t_gui_buffer *buffer,
                               struct t_gui_nick *nick,
                               const char *property)
{
    /* make C compiler happy */
    (void) buffer;

    if (nick && property)
    {
        if (string_strcasecmp (property, "visible") == 0)
            return nick->visible;
    }

    return 0;
}

/*
 * gui_nicklist_nick_get_string: get a nick property as string
 */

const char *
gui_nicklist_nick_get_string (struct t_gui_buffer *buffer,
                              struct t_gui_nick *nick,
                              const char *property)
{
    /* make C compiler happy */
    (void) buffer;

    if (nick && property)
    {
        if (string_strcasecmp (property, "name") == 0)
            return nick->name;
        else if (string_strcasecmp (property, "color") == 0)
            return nick->color;
        else if (string_strcasecmp (property, "prefix") == 0)
            return nick->prefix;
        else if (string_strcasecmp (property, "prefix_color") == 0)
            return nick->prefix_color;
    }

    return NULL;
}

/*
 * gui_nicklist_nick_get_pointer: get a nick property as pointer
 */

void *
gui_nicklist_nick_get_pointer (struct t_gui_buffer *buffer,
                               struct t_gui_nick *nick,
                               const char *property)
{
    /* make C compiler happy */
    (void) buffer;

    if (nick && property)
    {
        if (string_strcasecmp (property, "group") == 0)
            return nick->group;
    }

    return NULL;
}

/*
 * gui_nicklist_nick_set: set a nick property (string)
 */

void
gui_nicklist_nick_set (struct t_gui_buffer *buffer,
                       struct t_gui_nick *nick,
                       const char *property, const char *value)
{
    long number;
    char *error;
    int nick_changed;

    if (!buffer || !nick || !property || !value)
        return;

    nick_changed = 0;

    if (string_strcasecmp (property, "color") == 0)
    {
        if (nick->color)
            free (nick->color);
        nick->color = (value[0]) ? strdup (value) : NULL;
        nick_changed = 1;
    }
    else if (string_strcasecmp (property, "prefix") == 0)
    {
        if (nick->prefix)
            free (nick->prefix);
        nick->prefix = (value[0]) ? strdup (value) : NULL;
        nick_changed = 1;
    }
    else if (string_strcasecmp (property, "prefix_color") == 0)
    {
        if (nick->prefix_color)
            free (nick->prefix_color);
        nick->prefix_color = (value[0]) ? strdup (value) : NULL;
        nick_changed = 1;
    }
    else if (string_strcasecmp (property, "visible") == 0)
    {
        error = NULL;
        number = strtol (value, &error, 10);
        if (error && !error[0])
            nick->visible = (number) ? 1 : 0;
        nick_changed = 1;
    }

    if (nick_changed)
    {
        gui_nicklist_send_signal ("nicklist_nick_changed", buffer,
                                  nick->name);
    }
}

/*
 * gui_nicklist_hdata_nick_group_cb: return hdata for nick_group
 */

struct t_hdata *
gui_nicklist_hdata_nick_group_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_group", "next_group");
    if (hdata)
    {
        HDATA_VAR(struct t_gui_nick_group, name, STRING, NULL);
        HDATA_VAR(struct t_gui_nick_group, color, STRING, NULL);
        HDATA_VAR(struct t_gui_nick_group, visible, INTEGER, NULL);
        HDATA_VAR(struct t_gui_nick_group, level, INTEGER, NULL);
        HDATA_VAR(struct t_gui_nick_group, parent, POINTER, hdata_name);
        HDATA_VAR(struct t_gui_nick_group, children, POINTER, hdata_name);
        HDATA_VAR(struct t_gui_nick_group, last_child, POINTER, hdata_name);
        HDATA_VAR(struct t_gui_nick_group, nicks, POINTER, "nick");
        HDATA_VAR(struct t_gui_nick_group, last_nick, POINTER, "nick");
        HDATA_VAR(struct t_gui_nick_group, prev_group, POINTER, hdata_name);
        HDATA_VAR(struct t_gui_nick_group, next_group, POINTER, hdata_name);
    }
    return hdata;
}

/*
 * gui_nicklist_hdata_nick_cb: return hdata for nick
 */

struct t_hdata *
gui_nicklist_hdata_nick_cb (void *data, const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) data;

    hdata = hdata_new (NULL, hdata_name, "prev_nick", "next_nick");
    if (hdata)
    {
        HDATA_VAR(struct t_gui_nick, group, POINTER, "nick_group");
        HDATA_VAR(struct t_gui_nick, name, STRING, NULL);
        HDATA_VAR(struct t_gui_nick, color, STRING, NULL);
        HDATA_VAR(struct t_gui_nick, prefix, STRING, NULL);
        HDATA_VAR(struct t_gui_nick, prefix_color, STRING, NULL);
        HDATA_VAR(struct t_gui_nick, visible, INTEGER, NULL);
        HDATA_VAR(struct t_gui_nick, prev_nick, POINTER, hdata_name);
        HDATA_VAR(struct t_gui_nick, next_nick, POINTER, hdata_name);
    }
    return hdata;
}

/*
 * gui_nicklist_add_group_to_infolist: add a group in an infolist
 *                                     return 1 if ok, 0 if error
 */

int
gui_nicklist_add_group_to_infolist (struct t_infolist *infolist,
                                    struct t_gui_nick_group *group)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !group)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_string (ptr_item, "type", "group"))
        return 0;
    if (group->parent)
    {
        if (!infolist_new_var_string (ptr_item, "parent_name", group->parent->name))
            return 0;
    }
    if (!infolist_new_var_string (ptr_item, "name", group->name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color", group->color))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "visible", group->visible))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "level", group->level))
        return 0;

    return 1;
}

/*
 * gui_nicklist_add_nick_to_infolist: add a nick in an infolist
 *                                    return 1 if ok, 0 if error
 */

int
gui_nicklist_add_nick_to_infolist (struct t_infolist *infolist,
                                   struct t_gui_nick *nick)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !nick)
        return 0;

    ptr_item = infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!infolist_new_var_string (ptr_item, "type", "nick"))
        return 0;
    if (nick->group)
    {
        if (!infolist_new_var_string (ptr_item, "group_name", nick->group->name))
            return 0;
    }
    if (!infolist_new_var_string (ptr_item, "name", nick->name))
        return 0;
    if (!infolist_new_var_string (ptr_item, "color", nick->color))
        return 0;
    if (!infolist_new_var_string (ptr_item, "prefix", nick->prefix))
        return 0;
    if (!infolist_new_var_string (ptr_item, "prefix_color", nick->prefix_color))
        return 0;
    if (!infolist_new_var_integer (ptr_item, "visible", nick->visible))
        return 0;

    return 1;
}

/*
 * gui_nicklist_add_to_infolist: add a nicklist in an infolist
 *                               return 1 if ok, 0 if error
 */

int
gui_nicklist_add_to_infolist (struct t_infolist *infolist,
                              struct t_gui_buffer *buffer,
                              const char *name)
{
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;

    if (!infolist || !buffer)
        return 0;

    /* add only one nick if asked */
    if (name && (strncmp (name, "nick_", 5) == 0))
    {
        ptr_nick = gui_nicklist_search_nick (buffer, NULL, name + 5);
        if (!ptr_nick)
            return 0;
        return gui_nicklist_add_nick_to_infolist (infolist, ptr_nick);
    }

    /* add only one group if asked */
    if (name && (strncmp (name, "group_", 6) == 0))
    {
        ptr_group = gui_nicklist_search_group (buffer, NULL, name + 6);
        if (!ptr_group)
            return 0;
        return gui_nicklist_add_group_to_infolist (infolist, ptr_group);
    }

    ptr_group = NULL;
    ptr_nick = NULL;
    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    while (ptr_group || ptr_nick)
    {
        if (ptr_nick)
            gui_nicklist_add_nick_to_infolist (infolist, ptr_nick);
        else
            gui_nicklist_add_group_to_infolist (infolist, ptr_group);

        gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    }

    return 1;
}

/*
 * gui_nicklist_print_log: print nicklist infos in log (usually for crash dump)
 */

void
gui_nicklist_print_log (struct t_gui_nick_group *group, int indent)
{
    char format[128];
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;

    snprintf (format, sizeof (format),
              "%%-%ds=> group (addr:0x%%lx)",
              (indent * 2) + 4);
    log_printf (format, " ", group);
    snprintf (format, sizeof (format),
              "%%-%dsname. . . . : '%%s'",
              (indent * 2) + 6);
    log_printf (format, " ", group->name);
    snprintf (format, sizeof (format),
              "%%-%dscolor . . . : '%%s'",
              (indent * 2) + 6);
    log_printf (format, " ", group->color);
    snprintf (format, sizeof (format),
              "%%-%dsvisible . . : %%d",
              (indent * 2) + 6);
    log_printf (format, " ", group->visible);
    snprintf (format, sizeof (format),
              "%%-%dsparent. . . : 0x%%lx",
              (indent * 2) + 6);
    log_printf (format, " ", group->parent);
    snprintf (format, sizeof (format),
              "%%-%dschildren. . : 0x%%lx",
              (indent * 2) + 6);
    log_printf (format, " ", group->children);
    snprintf (format, sizeof (format),
              "%%-%dslast_child. : 0x%%lx",
              (indent * 2) + 6);
    log_printf (format, " ", group->last_child);
    snprintf (format, sizeof (format),
              "%%-%dsnicks . . . : 0x%%lx",
              (indent * 2) + 6);
    log_printf (format, " ", group->nicks);
    snprintf (format, sizeof (format),
              "%%-%dslast_nick . : 0x%%lx",
              (indent * 2) + 6);
    log_printf (format, " ", group->last_nick);
    snprintf (format, sizeof (format),
              "%%-%dsprev_group. : 0x%%lx",
              (indent * 2) + 6);
    log_printf (format, " ", group->prev_group);
    snprintf (format, sizeof (format),
              "%%-%dsnext_group. : 0x%%lx",
              (indent * 2) + 6);
    log_printf (format, " ", group->next_group);

    /* display child groups first */
    if (group->children)
    {
        for (ptr_group = group->children; ptr_group;
             ptr_group = ptr_group->next_group)
        {
            gui_nicklist_print_log (ptr_group, indent + 1);
        }
    }

    /* then display nicks in group */
    for (ptr_nick = group->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        snprintf (format, sizeof (format),
                  "%%-%ds=> nick (addr:0x%%lx)",
                  (indent * 2) + 4);
        log_printf (format, " ", ptr_nick);
        snprintf (format, sizeof (format),
                  "%%-%dsgroup . . . . . : 0x%%lx",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->group);
        snprintf (format, sizeof (format),
                  "%%-%dsname. . . . . . : '%%s'",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->name);
        snprintf (format, sizeof (format),
                  "%%-%dscolor . . . . . : '%%s'",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->color);
        snprintf (format, sizeof (format),
                  "%%-%dsprefix. . . . . : '%%s'",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->prefix);
        snprintf (format, sizeof (format),
                  "%%-%dsprefix_color. . : '%%s'",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->prefix_color);
        snprintf (format, sizeof (format),
                  "%%-%dsvisible . . . . : %%d",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->visible);
        snprintf (format, sizeof (format),
                  "%%-%dsprev_nick . . . : 0x%%lx",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->prev_nick);
        snprintf (format, sizeof (format),
                  "%%-%dsnext_nick . . . : 0x%%lx",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->next_nick);
    }
}
