/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* gui-nicklist.c: nicklist functions, used by all GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>

#include "../core/weechat.h"
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
 * gui_nicklist_changed_signal: send "nicklist_changed" signal
 */

void
gui_nicklist_changed_signal ()
{
    hook_signal_send ("nicklist_changed", WEECHAT_HOOK_SIGNAL_STRING, NULL);
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
                pos_group->prev_group->next_group = group;
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
 * gui_nicklist_search_group: search a group in buffer nicklist
 */

struct t_gui_nick_group *
gui_nicklist_search_group (struct t_gui_buffer *buffer,
                           struct t_gui_nick_group *from_group,
                           const char *name)
{
    struct t_gui_nick_group *ptr_group;

    if (!from_group)
        from_group = buffer->nicklist_root;
    
    if (!from_group)
        return NULL;
    
    if (from_group->childs)
    {
        ptr_group = gui_nicklist_search_group (buffer, from_group->childs, name);
        if (ptr_group)
            return ptr_group;
    }
    
    ptr_group = from_group;
    while (ptr_group)
    {
        if (strcmp (ptr_group->name, name) == 0)
            return ptr_group;
        ptr_group = ptr_group->next_group;
    }
    
    /* group not found */
    return NULL;
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
    
    if (!name || gui_nicklist_search_group (buffer, parent_group, name))
        return NULL;
    
    new_group = malloc (sizeof (*new_group));
    if (!new_group)
        return NULL;
    
    new_group->name = strdup (name);
    new_group->color = (color) ? strdup (color) : NULL;
    new_group->visible = visible;
    new_group->parent = (parent_group) ? parent_group : buffer->nicklist_root;
    new_group->level = (new_group->parent) ? new_group->parent->level + 1 : 0;
    new_group->childs = NULL;
    new_group->last_child = NULL;
    new_group->nicks = NULL;
    new_group->last_nick = NULL;
    new_group->prev_group = NULL;
    new_group->next_group = NULL;
    
    if (new_group->parent)
    {
        gui_nicklist_insert_group_sorted (&(new_group->parent->childs),
                                          &(new_group->parent->last_child),
                                          new_group);
    }
    else
    {
        buffer->nicklist_root = new_group;
    }
    
    if (buffer->nicklist_display_groups && visible)
        buffer->nicklist_visible_count++;
    
    gui_nicklist_changed_signal ();
    
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
                pos_nick->prev_nick->next_nick = nick;
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
    
    for (ptr_nick = (from_group) ? from_group->nicks : buffer->nicklist_root->nicks;
         ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (strcmp (ptr_nick->name, name) == 0)
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
                       struct t_gui_nick_group *group, const char *name,
                       const char *color, char prefix, const char *prefix_color,
                       int visible)
{
    struct t_gui_nick *new_nick;
    
    if (!name || gui_nicklist_search_nick (buffer, NULL, name))
        return NULL;
    
    new_nick = malloc (sizeof (*new_nick));
    if (!new_nick)
        return NULL;
    
    new_nick->group = (group) ? group : buffer->nicklist_root;
    new_nick->name = strdup (name);
    new_nick->color = (color) ? strdup (color) : NULL;
    new_nick->prefix = prefix;
    new_nick->prefix_color = (prefix_color) ? strdup (prefix_color) : NULL;
    new_nick->visible = visible;
    
    gui_nicklist_insert_nick_sorted (new_nick->group, new_nick);
    
    if (visible)
        buffer->nicklist_visible_count++;
    
    gui_nicklist_changed_signal ();
    
    return new_nick;
}

/*
 * gui_nicklist_remove_nick: remove a nick from a group
 */

void
gui_nicklist_remove_nick (struct t_gui_buffer *buffer,
                          struct t_gui_nick *nick)
{
    if (!nick)
        return;
    
    /* remove nick from list */
    if (nick->prev_nick)
        nick->prev_nick->next_nick = nick->next_nick;
    if (nick->next_nick)
        nick->next_nick->prev_nick = nick->prev_nick;
    if (nick->group->nicks == nick)
        nick->group->nicks = nick->next_nick;
    if (nick->group->last_nick == nick)
        nick->group->last_nick = nick->prev_nick;
    
    /* free data */
    if (nick->name)
        free (nick->name);
    if (nick->color)
        free (nick->color);
    if (nick->prefix_color)
        free (nick->prefix_color);
    
    if (nick->visible)
    {
        if (buffer->nicklist_visible_count > 0)
            buffer->nicklist_visible_count--;
    }
    
    free (nick);
    
    gui_nicklist_changed_signal ();
}

/*
 * gui_nicklist_remove_group: remove a group from nicklist
 */

void
gui_nicklist_remove_group (struct t_gui_buffer *buffer,
                           struct t_gui_nick_group *group)
{
    if (!group)
        return;
    
    /* remove childs first */
    while (group->childs)
    {
        gui_nicklist_remove_group (buffer, group->childs);
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
            group->prev_group->next_group = group->next_group;
        if (group->next_group)
            group->next_group->prev_group = group->prev_group;
        if (group->parent->childs == group)
            group->parent->childs = group->next_group;
        if (group->parent->last_child == group)
            group->parent->last_child = group->prev_group;
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
    
    gui_nicklist_changed_signal ();
}

/*
 * gui_nicklist_remove_all: remove all nicks in nicklist
 */

void
gui_nicklist_remove_all (struct t_gui_buffer *buffer)
{
    while (buffer->nicklist_root)
    {
        gui_nicklist_remove_group (buffer, buffer->nicklist_root);
    }
    gui_nicklist_add_group (buffer, NULL, "root", NULL, 0);
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
        if ((*group)->childs)
        {
            *group = (*group)->childs;
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

char *
gui_nicklist_get_group_start (const char *name)
{
    const char *ptr_name;
    
    ptr_name = name;
    while (isdigit (ptr_name[0]))
    {
        if (ptr_name[0] == '|')
            break;
        ptr_name++;
    }
    if ((ptr_name[0] == '|') && (ptr_name != name))
        return (char *)ptr_name + 1;
    else
        return (char *)name;
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
        if (ptr_group->childs)
        {
            length = gui_nicklist_get_max_length (buffer, ptr_group->childs);
            if (length > max_length)
                max_length = length;
        }
    }
    return max_length;
}

/*
 * gui_nicklist_compute_visible_count: compute visible_count variable for a buffer
 */

void
gui_nicklist_compute_visible_count (struct t_gui_buffer *buffer,
                                    struct t_gui_nick_group *group)
{
    if (!group)
        return;
    
    /* count for childs */
    if (group->childs)
        gui_nicklist_compute_visible_count (buffer, group->childs);
    
    /* count current group */
    if (buffer->nicklist_display_groups && group->visible)
        buffer->nicklist_visible_count++;
}

/*
 * gui_nicklist_add_to_infolist: add a nicklist in an infolist
 *                               return 1 if ok, 0 if error
 */

int
gui_nicklist_add_to_infolist (struct t_infolist *infolist,
                              struct t_gui_buffer *buffer)
{
    struct t_infolist_item *ptr_item;
    struct t_gui_nick_group *ptr_group;
    struct t_gui_nick *ptr_nick;
    char prefix[2];
    
    if (!infolist || !buffer)
        return 0;
    
    ptr_group = NULL;
    ptr_nick = NULL;
    gui_nicklist_get_next_item (buffer, &ptr_group, &ptr_nick);
    while (ptr_group || ptr_nick)
    {
        ptr_item = infolist_new_item (infolist);
        if (!ptr_item)
            return 0;
        
        if (ptr_nick)
        {
            if (!infolist_new_var_string (ptr_item, "type", "nick"))
                return 0;
            if (ptr_nick->group)
            {
                if (!infolist_new_var_string (ptr_item, "group_name", ptr_nick->group->name))
                    return 0;
            }
            if (!infolist_new_var_string (ptr_item, "name", ptr_nick->name))
                return 0;
            if (!infolist_new_var_string (ptr_item, "color", ptr_nick->color))
                return 0;
            prefix[0] = ptr_nick->prefix;
            prefix[1] = '\0';
            if (!infolist_new_var_string (ptr_item, "prefix", prefix))
                return 0;
            if (!infolist_new_var_string (ptr_item, "prefix_color", ptr_nick->prefix_color))
                return 0;
            if (!infolist_new_var_integer (ptr_item, "visible", ptr_nick->visible))
                return 0;
        }
        else
        {
            if (!infolist_new_var_string (ptr_item, "type", "group"))
                return 0;
            if (ptr_group->parent)
            {
                if (!infolist_new_var_string (ptr_item, "parent_name", ptr_group->parent->name))
                    return 0;
            }
            if (!infolist_new_var_string (ptr_item, "name", ptr_group->name))
                return 0;
            if (!infolist_new_var_string (ptr_item, "color", ptr_group->color))
                return 0;
            if (!infolist_new_var_integer (ptr_item, "visible", ptr_group->visible))
                return 0;
            if (!infolist_new_var_integer (ptr_item, "level", ptr_group->level))
                return 0;
        }
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
              "%%-%ds=> group (addr:0x%%X)",
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
              "%%-%dsparent. . . : 0x%%X",
              (indent * 2) + 6);
    log_printf (format, " ", group->parent);
    snprintf (format, sizeof (format),
              "%%-%dschilds. . . : 0x%%X",
              (indent * 2) + 6);
    log_printf (format, " ", group->childs);
    snprintf (format, sizeof (format),
              "%%-%dslast_child. : 0x%%X",
              (indent * 2) + 6);
    log_printf (format, " ", group->last_child);
    snprintf (format, sizeof (format),
              "%%-%dsnicks . . . : 0x%%X",
              (indent * 2) + 6);
    log_printf (format, " ", group->nicks);
    snprintf (format, sizeof (format),
              "%%-%dslast_nick . : 0x%%X",
              (indent * 2) + 6);
    log_printf (format, " ", group->last_nick);
    snprintf (format, sizeof (format),
              "%%-%dsprev_group. : 0x%%X",
              (indent * 2) + 6);
    log_printf (format, " ", group->prev_group);
    snprintf (format, sizeof (format),
              "%%-%dsnext_group. : 0x%%X",
              (indent * 2) + 6);
    log_printf (format, " ", group->next_group);
    
    /* display child groups first */
    if (group->childs)
    {
        for (ptr_group = group->childs; ptr_group;
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
                  "%%-%ds=> nick (addr:0x%%X)",
                  (indent * 2) + 4);
        log_printf (format, " ", ptr_nick);
        snprintf (format, sizeof (format),
                  "%%-%dsgroup . . . . . : 0x%%X",
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
                  "%%-%dsprefix. . . . . : '%%c'",
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
                  "%%-%dsprev_nick . . . : 0x%%X",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->prev_nick);
        snprintf (format, sizeof (format),
                  "%%-%dsnext_nick . . . : 0x%%X",
                  (indent * 2) + 6);
        log_printf (format, " ", ptr_nick->next_nick);
    }
}
