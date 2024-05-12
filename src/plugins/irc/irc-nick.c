/*
 * irc-nick.c - nick management for IRC plugin
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-nick.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-mode.h"
#include "irc-server.h"
#include "irc-channel.h"


/*
 * Checks if a nick pointer is valid.
 *
 * Returns:
 *   1: nick exists in channel
 *   0: nick does not exist in channel
 */

int
irc_nick_valid (struct t_irc_channel *channel, struct t_irc_nick *nick)
{
    struct t_irc_nick *ptr_nick;

    if (!channel || !nick)
        return 0;

    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        if (ptr_nick == nick)
            return 1;
    }

    /* nick not found */
    return 0;
}

/*
 * Checks if string is a valid nick string, using server UTF8MAPPING.
 *
 * Returns:
 *   1: string is a valid nick on this server
 *   0: string is not a valid nick on this server
 */

int
irc_nick_is_nick (struct t_irc_server *server, const char *string)
{
    const char *ptr_string, *ptr_prefix_chars, *ptr_chantypes;
    int utf8mapping;

    if (!string || !string[0])
        return 0;

    utf8mapping = (server) ? server->utf8mapping : IRC_SERVER_UTF8MAPPING_NONE;
    ptr_prefix_chars = (server && server->prefix_chars) ?
        server->prefix_chars : irc_server_prefix_chars_default;
    ptr_chantypes = irc_server_get_chantypes (server);

    /* check length of nick in bytes (if we have a limit in the server) */
    if (server && (server->nick_max_length > 0)
        && (int)strlen (string) > server->nick_max_length)
    {
        /* nick is too long */
        return 0;
    }

    /* check if nick is UTF-8 valid */
    if ((utf8mapping == IRC_SERVER_UTF8MAPPING_RFC8265)
        && !weechat_utf8_is_valid (string, -1, NULL))
    {
        /* invalid UTF-8 */
        return 0;
    }

    /* check the first char in the nick */
    if ((utf8mapping == IRC_SERVER_UTF8MAPPING_NONE)
        && strchr ("0123456789-", string[0]))
    {
        /* first char is invalid */
        return 0;
    }
    if (strchr (ptr_prefix_chars, string[0])
        || strchr (ptr_chantypes, string[0]))
    {
        return 0;
    }

    /* check if there are forbidden chars in nick */
    ptr_string = string;
    while (ptr_string && ptr_string[0])
    {
        if ((utf8mapping == IRC_SERVER_UTF8MAPPING_NONE)
            && !strchr (IRC_NICK_VALID_CHARS_RFC1459, ptr_string[0]))
        {
            return 0;
        }
        if ((utf8mapping == IRC_SERVER_UTF8MAPPING_RFC8265)
            && strchr (IRC_NICK_INVALID_CHARS_RFC8265, ptr_string[0]))
        {
            return 0;
        }
        ptr_string = weechat_utf8_next_char (ptr_string);
    }

    return 1;
}

/*
 * Finds a color code for a nick (according to nick letters).
 *
 * Returns a WeeChat color code (that can be used for display).
 */

char *
irc_nick_find_color (const char *nickname)
{
    return weechat_info_get ("nick_color", nickname);
}

/*
 * Finds a color name for a nick (according to nick letters).
 *
 * Returns the name of a color (for example: "green").
 */

char *
irc_nick_find_color_name (const char *nickname)
{
    return weechat_info_get ("nick_color_name", nickname);
}

/*
 * Sets current prefix, using higher prefix set in prefixes.
 */

void
irc_nick_set_current_prefix (struct t_irc_nick *nick)
{
    char *ptr_prefixes;

    if (!nick)
        return;

    nick->prefix[0] = ' ';
    for (ptr_prefixes = nick->prefixes; ptr_prefixes[0]; ptr_prefixes++)
    {
        if (ptr_prefixes[0] != ' ')
        {
            nick->prefix[0] = ptr_prefixes[0];
            break;
        }
    }
}

/*
 * Sets/unsets a prefix in prefixes.
 *
 * If set == 1, sets prefix (prefix is used).
 * If set == 0, unsets prefix (space is used).
 */

void
irc_nick_set_prefix (struct t_irc_server *server, struct t_irc_nick *nick,
                     int set, char prefix)
{
    int index;

    if (!nick)
        return;

    index = irc_server_get_prefix_char_index (server, prefix);
    if (index >= 0)
    {
        nick->prefixes[index] = (set) ? prefix : ' ';
        irc_nick_set_current_prefix (nick);
    }
}

/*
 * Sets prefixes for nick.
 */

void
irc_nick_set_prefixes (struct t_irc_server *server, struct t_irc_nick *nick,
                       const char *prefixes)
{
    const char *ptr_prefixes;

    if (!nick)
        return;

    /* reset all prefixes in nick */
    memset (nick->prefixes, ' ', strlen (nick->prefixes));

    /* add prefixes in nick */
    if (prefixes)
    {
        for (ptr_prefixes = prefixes; ptr_prefixes[0]; ptr_prefixes++)
        {
            irc_nick_set_prefix (server, nick, 1, ptr_prefixes[0]);
        }
    }

    /* set current prefix */
    irc_nick_set_current_prefix (nick);
}

/*
 * Sets host for nick.
 */

void
irc_nick_set_host (struct t_irc_nick *nick, const char *host)
{
    if (!nick)
        return;

    /* if host is the same, just return */
    if ((!nick->host && !host)
        || (nick->host && host && strcmp (nick->host, host) == 0))
    {
        return;
    }

    /* update the host in nick */
    free (nick->host);
    nick->host = (host) ? strdup (host) : NULL;
}

/*
 * Checks if nick is "op" or higher than "op", like channel admin/owner.
 *
 * Returns:
 *   1: nick is "op" or higher
 *   0: nick is not op
 */

int
irc_nick_is_op_or_higher (struct t_irc_server *server, struct t_irc_nick *nick)
{
    int index;

    if (nick->prefix[0] == ' ')
        return 0;

    index = irc_server_get_prefix_char_index (server, nick->prefix[0]);
    if (index < 0)
        return 0;

    return (index <= irc_server_get_prefix_mode_index (server, 'o')) ? 1 : 0;
}

/*
 * Checks if nick prefixes contains prefix for a given mode.
 *
 * For example if prefix_mode is 'o', searches for '@' in nick prefixes.
 *
 * Returns:
 *   1: prefixes contains prefix for the given mode
 *   0: prefixes does not contain prefix for the given mode.
 */

int
irc_nick_has_prefix_mode (struct t_irc_server *server, struct t_irc_nick *nick,
                          char prefix_mode)
{
    char prefix_char;

    prefix_char = irc_server_get_prefix_char_for_mode (server, prefix_mode);
    if (prefix_char == ' ')
        return 0;

    return (strchr (nick->prefixes, prefix_char)) ? 1 : 0;
}

/*
 * Gets nicklist group for a nick.
 */

struct t_gui_nick_group *
irc_nick_get_nicklist_group (struct t_irc_server *server,
                             struct t_gui_buffer *buffer,
                             struct t_irc_nick *nick)
{
    int index;
    char str_group[2];
    const char *prefix_modes;
    struct t_gui_nick_group *ptr_group;

    if (!server || !buffer || !nick)
        return NULL;

    ptr_group = NULL;
    index = irc_server_get_prefix_char_index (server, nick->prefix[0]);
    if (index < 0)
    {
        ptr_group = weechat_nicklist_search_group (buffer, NULL,
                                                   IRC_NICK_GROUP_OTHER_NAME);
    }
    else
    {
        prefix_modes = irc_server_get_prefix_modes (server);
        str_group[0] = prefix_modes[index];
        str_group[1] = '\0';
        ptr_group = weechat_nicklist_search_group (buffer, NULL, str_group);
    }

    return ptr_group;
}

/*
 * Gets name of prefix color for a nick.
 */

const char *
irc_nick_get_prefix_color_name (struct t_irc_server *server, char prefix)
{
    static char *default_color = "";
    const char *prefix_modes, *color;
    char mode[2];
    int index;

    if (irc_config_hashtable_nick_prefixes)
    {
        mode[0] = ' ';
        mode[1] = '\0';

        index = irc_server_get_prefix_char_index (server, prefix);
        if (index >= 0)
        {
            prefix_modes = irc_server_get_prefix_modes (server);
            mode[0] = prefix_modes[index];
            color = weechat_hashtable_get (irc_config_hashtable_nick_prefixes,
                                           mode);
            if (color)
                return color;
        }

        /* fallback to "*" if no color is found with mode */
        mode[0] = '*';
        color = weechat_hashtable_get (irc_config_hashtable_nick_prefixes,
                                       mode);
        if (color)
            return color;
    }

    /* no color by default */
    return default_color;
}

/*
 * Gets nick color for nicklist.
 */

char *
irc_nick_get_color_for_nicklist (struct t_irc_server *server,
                                 struct t_irc_nick *nick)
{
    static char *nick_color_bar_fg = "bar_fg";
    static char *nick_color_self = "weechat.color.chat_nick_self";
    static char *nick_color_away = "weechat.color.nicklist_away";

    if (nick->away)
        return strdup (nick_color_away);

    if (weechat_config_boolean (irc_config_look_color_nicks_in_nicklist))
    {
        if (irc_server_strcasecmp (server, nick->name, server->nick) == 0)
            return strdup (nick_color_self);
        else
            return irc_nick_find_color_name (nick->name);
    }

    return strdup (nick_color_bar_fg);
}

/*
 * Adds a nick to buffer nicklist.
 */

void
irc_nick_nicklist_add (struct t_irc_server *server,
                       struct t_irc_channel *channel,
                       struct t_irc_nick *nick)
{
    struct t_gui_nick_group *ptr_group;
    char *color;

    ptr_group = irc_nick_get_nicklist_group (server, channel->buffer, nick);
    color = irc_nick_get_color_for_nicklist (server, nick);
    weechat_nicklist_add_nick (channel->buffer, ptr_group,
                               nick->name,
                               color,
                               nick->prefix,
                               irc_nick_get_prefix_color_name (server, nick->prefix[0]),
                               1);
    free (color);
}

/*
 * Removes a nick from buffer nicklist.
 */

void
irc_nick_nicklist_remove (struct t_irc_server *server,
                          struct t_irc_channel *channel,
                          struct t_irc_nick *nick)
{
    struct t_gui_nick_group *ptr_group;

    ptr_group = irc_nick_get_nicklist_group (server, channel->buffer, nick);
    weechat_nicklist_remove_nick (channel->buffer,
                                  weechat_nicklist_search_nick (channel->buffer,
                                                                ptr_group,
                                                                nick->name));
}

/*
 * Sets a property for nick in buffer nicklist.
 */

void
irc_nick_nicklist_set (struct t_irc_channel *channel,
                       struct t_irc_nick *nick,
                       const char *property, const char *value)
{
    struct t_gui_nick *ptr_nick;

    ptr_nick = weechat_nicklist_search_nick (channel->buffer, NULL, nick->name);
    if (ptr_nick)
    {
        weechat_nicklist_nick_set (channel->buffer, ptr_nick, property, value);
    }
}

/*
 * Sets nick prefix colors in nicklist for all servers/channels.
 */

void
irc_nick_nicklist_set_prefix_color_all ()
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                 ptr_nick = ptr_nick->next_nick)
            {
                irc_nick_nicklist_set (ptr_channel, ptr_nick, "prefix_color",
                                       irc_nick_get_prefix_color_name (ptr_server,
                                                                       ptr_nick->prefix[0]));
            }
        }
    }
}

/*
 * Sets nick colors in nicklist for all servers/channels.
 */

void
irc_nick_nicklist_set_color_all ()
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    char *color;

    for (ptr_server = irc_servers; ptr_server;
         ptr_server = ptr_server->next_server)
    {
        for (ptr_channel = ptr_server->channels; ptr_channel;
             ptr_channel = ptr_channel->next_channel)
        {
            for (ptr_nick = ptr_channel->nicks; ptr_nick;
                 ptr_nick = ptr_nick->next_nick)
            {
                color = irc_nick_get_color_for_nicklist (ptr_server, ptr_nick);
                irc_nick_nicklist_set (ptr_channel, ptr_nick, "color", color);
                free (color);
            }
        }
    }
}

/*
 * Adds a new nick in channel, but do not update the buffer nicklist.
 * This function is only called by the function `irc_nick_new` (below) and
 * when restoring nicks after upgrade: in this case we want to just add nick
 * in channel nicks without changing anything in the buffer nicklist,
 * to preserve same identifiers).
 *
 * Returns pointer to new nick, NULL if error.
 */

struct t_irc_nick *
irc_nick_new_in_channel (struct t_irc_server *server,
                         struct t_irc_channel *channel,
                         const char *nickname, const char *host,
                         const char *prefixes, int away, const char *account,
                         const char *realname)
{
    struct t_irc_nick *new_nick;
    int length;

    /* alloc memory for new nick */
    if ((new_nick = malloc (sizeof (*new_nick))) == NULL)
        return NULL;

    /* initialize new nick */
    new_nick->name = strdup (nickname);
    new_nick->host = (host) ? strdup (host) : NULL;
    new_nick->account = (account) ? strdup (account) : NULL;
    new_nick->realname = (realname) ? strdup (realname) : NULL;
    length = strlen (irc_server_get_prefix_chars (server));
    new_nick->prefixes = malloc (length + 1);
    new_nick->prefix = malloc (2);
    if (!new_nick->name || !new_nick->prefixes || !new_nick->prefix)
    {
        free (new_nick->name);
        free (new_nick->host);
        free (new_nick->account);
        free (new_nick->realname);
        free (new_nick->prefixes);
        free (new_nick->prefix);
        free (new_nick);
        return NULL;
    }
    memset (new_nick->prefixes, ' ', length);
    new_nick->prefixes[length] = '\0';
    new_nick->prefix[0] = ' ';
    new_nick->prefix[1] = '\0';
    irc_nick_set_prefixes (server, new_nick, prefixes);
    new_nick->away = away;
    if (irc_server_strcasecmp (server, new_nick->name, server->nick) == 0)
        new_nick->color = strdup (IRC_COLOR_CHAT_NICK_SELF);
    else
        new_nick->color = irc_nick_find_color (new_nick->name);

    /* add nick to end of list */
    new_nick->prev_nick = channel->last_nick;
    if (channel->last_nick)
        channel->last_nick->next_nick = new_nick;
    else
        channel->nicks = new_nick;
    channel->last_nick = new_nick;
    new_nick->next_nick = NULL;

    channel->nicks_count++;

    channel->nick_completion_reset = 1;

    return new_nick;
}

/*
 * Adds a new nick in channel.
 *
 * Returns pointer to new nick, NULL if error.
 */

struct t_irc_nick *
irc_nick_new (struct t_irc_server *server, struct t_irc_channel *channel,
              const char *nickname, const char *host, const char *prefixes,
              int away, const char *account, const char *realname)
{
    struct t_irc_nick *new_nick, *ptr_nick;

    if (!nickname || !nickname[0])
        return NULL;

    if (!channel->nicks)
        irc_channel_add_nicklist_groups (server, channel);

    /* nick already exists on this channel? */
    ptr_nick = irc_nick_search (server, channel, nickname);
    if (ptr_nick)
    {
        /* remove old nick from nicklist */
        irc_nick_nicklist_remove (server, channel, ptr_nick);

        /* update nick prefixes */
        irc_nick_set_prefixes (server, ptr_nick, prefixes);

        /* add new nick in nicklist */
        irc_nick_nicklist_add (server, channel, ptr_nick);

        return ptr_nick;
    }

    new_nick = irc_nick_new_in_channel (server, channel, nickname, host,
                                        prefixes, away, account, realname);
    if (!new_nick)
        return NULL;

    /* add nick to buffer nicklist */
    irc_nick_nicklist_add (server, channel, new_nick);

    /* all is OK, return address of new nick */
    return new_nick;
}

/*
 * Changes nickname.
 */

void
irc_nick_change (struct t_irc_server *server, struct t_irc_channel *channel,
                 struct t_irc_nick *nick, const char *new_nick)
{
    int nick_is_me;

    /* remove nick from nicklist */
    irc_nick_nicklist_remove (server, channel, nick);

    /* update nicks speaking */
    nick_is_me = (irc_server_strcasecmp (server, new_nick, server->nick) == 0) ? 1 : 0;
    if (!nick_is_me)
        irc_channel_nick_speaking_rename (channel, nick->name, new_nick);

    /* change nickname */
    free (nick->name);
    nick->name = strdup (new_nick);
    free (nick->color);
    if (nick_is_me)
        nick->color = strdup (IRC_COLOR_CHAT_NICK_SELF);
    else
        nick->color = irc_nick_find_color (nick->name);

    /* add nick in nicklist */
    irc_nick_nicklist_add (server, channel, nick);
}

/*
 * Sets a mode for a nick.
 */

void
irc_nick_set_mode (struct t_irc_server *server, struct t_irc_channel *channel,
                   struct t_irc_nick *nick, int set, char mode)
{
    int index;
    const char *prefix_chars;

    index = irc_server_get_prefix_mode_index (server, mode);
    if (index < 0)
        return;

    /* remove nick from nicklist */
    irc_nick_nicklist_remove (server, channel, nick);

    /* set flag */
    prefix_chars = irc_server_get_prefix_chars (server);
    irc_nick_set_prefix (server, nick, set, prefix_chars[index]);

    /* add nick in nicklist */
    irc_nick_nicklist_add (server, channel, nick);

    if (irc_server_strcasecmp (server, nick->name, server->nick) == 0)
    {
        irc_server_set_buffer_input_prompt (server);
        weechat_bar_item_update ("irc_nick");
        weechat_bar_item_update ("irc_nick_host");
    }
}

/*
 * Reallocates the "prefixes" string in all nicks of all channels on the server
 * (after 005 has been received).
 */

void
irc_nick_realloc_prefixes (struct t_irc_server *server,
                           int old_length, int new_length)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    char *new_prefixes;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        for (ptr_nick = ptr_channel->nicks; ptr_nick;
             ptr_nick = ptr_nick->next_nick)
        {
            if (ptr_nick->prefixes)
            {
                new_prefixes = realloc (ptr_nick->prefixes, new_length + 1);
                if (new_prefixes)
                {
                    ptr_nick->prefixes = new_prefixes;
                    if (new_length > old_length)
                    {
                        memset (ptr_nick->prefixes + old_length,
                                ' ',
                                new_length - old_length);
                    }
                    ptr_nick->prefixes[new_length] = '\0';
                }
            }
            else
            {
                ptr_nick->prefixes = malloc (new_length + 1);
                if (ptr_nick->prefixes)
                {
                    memset (ptr_nick->prefixes, ' ', new_length);
                    ptr_nick->prefixes[new_length] = '\0';
                }
            }
        }
    }
}

/*
 * Removes a nick from a channel.
 */

void
irc_nick_free (struct t_irc_server *server, struct t_irc_channel *channel,
               struct t_irc_nick *nick)
{
    struct t_irc_nick *new_nicks;

    if (!channel || !nick)
        return;

    /* remove nick from nicklist */
    irc_nick_nicklist_remove (server, channel, nick);

    /* remove nick */
    if (channel->last_nick == nick)
        channel->last_nick = nick->prev_nick;
    if (nick->prev_nick)
    {
        (nick->prev_nick)->next_nick = nick->next_nick;
        new_nicks = channel->nicks;
    }
    else
        new_nicks = nick->next_nick;

    if (nick->next_nick)
        (nick->next_nick)->prev_nick = nick->prev_nick;

    channel->nicks_count--;

    /* free data */
    free (nick->name);
    free (nick->host);
    free (nick->prefixes);
    free (nick->prefix);
    free (nick->account);
    free (nick->realname);
    free (nick->color);

    free (nick);

    channel->nicks = new_nicks;
    channel->nick_completion_reset = 1;
}

/*
 * Removes all nicks from a channel.
 */

void
irc_nick_free_all (struct t_irc_server *server, struct t_irc_channel *channel)
{
    if (!channel)
        return;

    /* remove all nicks for the channel */
    while (channel->nicks)
    {
        irc_nick_free (server, channel, channel->nicks);
    }

    /* remove all groups in nicklist */
    weechat_nicklist_remove_all (channel->buffer);

    /* should be zero, but prevent any bug :D */
    channel->nicks_count = 0;

    irc_channel_set_buffer_modes (server, channel);
    irc_channel_set_buffer_input_prompt (server, channel);
}

/*
 * Searches for a nick in a channel.
 *
 * Returns pointer to nick found, NULL if error.
 */

struct t_irc_nick *
irc_nick_search (struct t_irc_server *server, struct t_irc_channel *channel,
                 const char *nickname)
{
    struct t_irc_nick *ptr_nick;

    if (!channel || !nickname)
        return NULL;

    for (ptr_nick = channel->nicks; ptr_nick;
         ptr_nick = ptr_nick->next_nick)
    {
        if (irc_server_strcasecmp (server, ptr_nick->name, nickname) == 0)
            return ptr_nick;
    }

    /* nick not found */
    return NULL;
}

/*
 * Returns number of nicks per mode on a channel, as an array of integers
 * whose size is the number of modes + 1 (for regular users).
 *
 * For example if modes == "ohv", the array returned has a size of 4, with:
 *   - array[0] = number of nicks with mode "o"
 *   - array[1] = number of nicks with mode "h"
 *   - array[2] = number of nicks with mode "v"
 *   - array[3] = number of nicks with no mode (regular users)
 *
 * The parameter *size is set with the array size (number of integers in the
 * array, NOT the size in bytes).
 *
 * Note: result must be freed after use (if not NULL).
 */

int *
irc_nick_count (struct t_irc_server *server, struct t_irc_channel *channel,
                int *size)
{
    struct t_irc_nick *ptr_nick;
    const char *ptr_prefix_modes;
    int i, *nicks_by_mode, mode_found;

    if (!server || !channel || !size)
        return NULL;

    *size = 0;

    ptr_prefix_modes = irc_server_get_prefix_modes (server);
    if (!ptr_prefix_modes)
        return NULL;

    *size = strlen (ptr_prefix_modes) + 1;
    nicks_by_mode = (int *)calloc (*size, sizeof (*nicks_by_mode));
    if (!nicks_by_mode)
    {
        *size = 0;
        return NULL;
    }

    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        mode_found = 0;
        for (i = 0; ptr_prefix_modes[i]; i++)
        {
            if (irc_nick_has_prefix_mode (server, ptr_nick, ptr_prefix_modes[i]))
            {
                nicks_by_mode[i]++;
                mode_found = 1;
                break;
            }
        }
        if (!mode_found)
        {
            /* regular user */
            nicks_by_mode[*size - 1]++;
        }
    }

    return nicks_by_mode;
}

/*
 * Sets/unsets away status for a nick.
 */

void
irc_nick_set_away (struct t_irc_server *server, struct t_irc_channel *channel,
                   struct t_irc_nick *nick, int is_away)
{
    char *color;

    if (is_away != nick->away)
    {
        nick->away = is_away;
        color = irc_nick_get_color_for_nicklist (server, nick);
        irc_nick_nicklist_set (channel, nick, "color", color);
        free (color);
    }
}

/*
 * Gets nick mode for display (color + mode).
 *
 * If prefix == 1, returns string for display in prefix, otherwise returns
 * string for display in action message (/me).
 */

const char *
irc_nick_mode_for_display (struct t_irc_server *server, struct t_irc_nick *nick,
                           int prefix)
{
    static char result[32];
    char str_prefix[2];
    int nick_mode;
    const char *str_prefix_color;

    str_prefix[0] = (nick) ? nick->prefix[0] : '\0';
    str_prefix[1] = '\0';

    nick_mode = weechat_config_enum (irc_config_look_nick_mode);
    if ((nick_mode == IRC_CONFIG_LOOK_NICK_MODE_BOTH)
        || (prefix && (nick_mode == IRC_CONFIG_LOOK_NICK_MODE_PREFIX))
        || (!prefix && (nick_mode == IRC_CONFIG_LOOK_NICK_MODE_ACTION)))
    {
        if (nick)
        {
            if ((str_prefix[0] == ' ')
                && (!prefix || !weechat_config_boolean (irc_config_look_nick_mode_empty)))
            {
                str_prefix[0] = '\0';
            }
            str_prefix_color = weechat_color (
                irc_nick_get_prefix_color_name (server, nick->prefix[0]));
        }
        else
        {
            str_prefix[0] = (prefix
                             && weechat_config_boolean (irc_config_look_nick_mode_empty)) ?
                ' ' : '\0';
            str_prefix_color = IRC_COLOR_RESET;
        }
    }
    else
    {
        str_prefix[0] = '\0';
        str_prefix_color = IRC_COLOR_RESET;
    }

    snprintf (result, sizeof (result), "%s%s", str_prefix_color, str_prefix);

    return result;
}

/*
 * Returns string with nick to display as prefix on buffer (returned string ends
 * by a tab).
 */

const char *
irc_nick_as_prefix (struct t_irc_server *server, struct t_irc_nick *nick,
                    const char *nickname, const char *force_color)
{
    static char result[256];
    char *color;

    if (force_color)
        color = strdup (force_color);
    else if (nick)
        color = strdup (nick->color);
    else if (nickname)
        color = irc_nick_find_color (nickname);
    else
        color = strdup (IRC_COLOR_CHAT_NICK);

    snprintf (result, sizeof (result), "%s%s%s\t",
              irc_nick_mode_for_display (server, nick, 1),
              color,
              (nick) ? nick->name : nickname);

    free (color);

    return result;
}

/*
 * Returns WeeChat color code for a nick.
 */

const char *
irc_nick_color_for_msg (struct t_irc_server *server, int server_message,
                        struct t_irc_nick *nick, const char *nickname)
{
    static char color[16][64];
    static int index_color = 0;
    char *color_found;

    if (server_message
        && !weechat_config_boolean (irc_config_look_color_nicks_in_server_messages))
    {
        return IRC_COLOR_CHAT_NICK;
    }

    if (nick)
        return nick->color;

    if (nickname)
    {
        if (server
            && (irc_server_strcasecmp (server, nickname, server->nick) == 0))
        {
            return IRC_COLOR_CHAT_NICK_SELF;
        }
        color_found = irc_nick_find_color (nickname);
        index_color = (index_color + 1) % 16;
        snprintf (color[index_color], sizeof (color[index_color]),
                  "%s",
                  color_found);
        free (color_found);
        return color[index_color];
    }

    return IRC_COLOR_CHAT_NICK;
}

/*
 * Returns string with color of nick for private.
 */

const char *
irc_nick_color_for_pv (struct t_irc_channel *channel, const char *nickname)
{
    if (weechat_config_boolean (irc_config_look_color_pv_nick_like_channel))
    {
        if (!channel->pv_remote_nick_color)
            channel->pv_remote_nick_color = irc_nick_find_color (nickname);
        if (channel->pv_remote_nick_color)
            return channel->pv_remote_nick_color;
    }

    return IRC_COLOR_CHAT_NICK_OTHER;
}

/*
 * Returns default ban mask for the nick.
 *
 * Note: result must be freed after use (if not NULL).
 */

char *
irc_nick_default_ban_mask (struct t_irc_nick *nick)
{
    const char *ptr_ban_mask;
    char *pos_hostname, user[128], ident[128], *res, *temp;

    if (!nick)
        return NULL;

    ptr_ban_mask = weechat_config_string (irc_config_network_ban_mask_default);

    pos_hostname = (nick->host) ? strchr (nick->host, '@') : NULL;

    if (!nick->host || !pos_hostname || !ptr_ban_mask || !ptr_ban_mask[0])
        return NULL;

    if (pos_hostname - nick->host > (int)sizeof (user) - 1)
        return NULL;

    strncpy (user, nick->host, pos_hostname - nick->host);
    user[pos_hostname - nick->host] = '\0';
    strcpy (ident, (user[0] != '~') ? user : "*");
    pos_hostname++;

    /* replace nick */
    temp = weechat_string_replace (ptr_ban_mask, "$nick", nick->name);
    if (!temp)
        return NULL;
    res = temp;

    /* replace user */
    temp = weechat_string_replace (res, "$user", user);
    free (res);
    if (!temp)
        return NULL;
    res = temp;

    /* replace ident */
    temp = weechat_string_replace (res, "$ident", ident);
    free (res);
    if (!temp)
        return NULL;
    res = temp;

    /* replace hostname */
    temp = weechat_string_replace (res, "$host", pos_hostname);
    free (res);
    if (!temp)
        return NULL;
    res = temp;

    return res;
}

/*
 * Returns hdata for nick.
 */

struct t_hdata *
irc_nick_hdata_nick_cb (const void *pointer, void *data,
                        const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_nick", "next_nick",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_nick, name, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_nick, host, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_nick, prefixes, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_nick, prefix, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_nick, away, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_nick, account, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_nick, realname, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_nick, color, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_nick, prev_nick, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_nick, next_nick, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Adds a nick in an infolist.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_nick_add_to_infolist (struct t_infolist *infolist,
                          struct t_irc_nick *nick)
{
    struct t_infolist_item *ptr_item;

    if (!infolist || !nick)
        return 0;

    ptr_item = weechat_infolist_new_item (infolist);
    if (!ptr_item)
        return 0;

    if (!weechat_infolist_new_var_string (ptr_item, "name", nick->name))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "host", nick->host))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "prefixes", nick->prefixes))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "prefix", nick->prefix))
        return 0;
    if (!weechat_infolist_new_var_integer (ptr_item, "away", nick->away))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "account", nick->account))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "realname", nick->realname))
        return 0;
    if (!weechat_infolist_new_var_string (ptr_item, "color", nick->color))
        return 0;

    return 1;
}

/*
 * Prints nick infos in WeeChat log file (usually for crash dump).
 */

void
irc_nick_print_log (struct t_irc_nick *nick)
{
    weechat_log_printf ("");
    weechat_log_printf ("    => nick %s (addr:%p):", nick->name, nick);
    weechat_log_printf ("         host . . . . . : '%s'", nick->host);
    weechat_log_printf ("         prefixes . . . : '%s'", nick->prefixes);
    weechat_log_printf ("         prefix . . . . : '%s'", nick->prefix);
    weechat_log_printf ("         away . . . . . : %d", nick->away);
    weechat_log_printf ("         account. . . . : '%s'", nick->account);
    weechat_log_printf ("         realname . . . : '%s'", nick->realname);
    weechat_log_printf ("         color. . . . . : '%s'", nick->color);
    weechat_log_printf ("         prev_nick. . . : %p", nick->prev_nick);
    weechat_log_printf ("         next_nick. . . : %p", nick->next_nick);
}
