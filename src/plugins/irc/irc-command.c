/*
 * irc-command.c - IRC commands
 *
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
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
#include <ctype.h>
#include <sys/time.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <regex.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-command.h"
#include "irc-ctcp.h"
#include "irc-buffer.h"
#include "irc-channel.h"
#include "irc-color.h"
#include "irc-config.h"
#include "irc-ignore.h"
#include "irc-input.h"
#include "irc-join.h"
#include "irc-list.h"
#include "irc-message.h"
#include "irc-mode.h"
#include "irc-modelist.h"
#include "irc-msgbuffer.h"
#include "irc-nick.h"
#include "irc-notify.h"
#include "irc-protocol.h"
#include "irc-raw.h"
#include "irc-sasl.h"
#include "irc-server.h"


/*
 * Sends mode change for many nicks on a channel.
 *
 * Argument "set" is "+" or "-", mode can be "o", "h", "v", or any other mode
 * supported by server.
 *
 * Many messages can be sent if the number of nicks is greater than the server
 * limit (number of modes allowed in a single message). In this case, the first
 * message is sent with high priority, and subsequent messages are sent with low
 * priority.
 */

void
irc_command_mode_nicks (struct t_irc_server *server,
                        struct t_irc_channel *channel,
                        const char *command,
                        const char *set, const char *mode,
                        int argc, char **argv)
{
    int i, arg_yes, max_modes, modes_added, msg_priority, prefix_found;
    char prefix, modes[128+1], nicks[1024];
    struct t_irc_nick *ptr_nick;
    struct t_hashtable *nicks_sent;

    if (argc < 2)
        return;

    arg_yes = 0;
    if ((argc > 2) && (strcmp (argv[argc - 1], "-yes") == 0))
    {
        argc--;
        arg_yes = 1;
    }

    if (!arg_yes)
    {
        for (i = 1; i < argc; i++)
        {
            if (strcmp (argv[i], "*") == 0)
            {
                weechat_printf (
                    NULL,
                    _("%s%s: \"-yes\" argument is required for nick \"*\" "
                      "(security reason), see /help %s"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, command);
                return;
            }
        }
    }

    /* get the max number of modes we can send in a message */
    max_modes = irc_server_get_max_modes (server);

    /* get prefix for the mode (example: prefix == '@' for mode 'o') */
    prefix = irc_server_get_prefix_char_for_mode (server, mode[0]);

    /*
     * first message has high priority and subsequent messages have low priority
     * (so for example in case of "/op *" sent as multiple messages, the user
     * can still send some messages which will have higher priority than the
     * "MODE" messages we are sending now)
     */
    msg_priority = IRC_SERVER_SEND_OUTQ_PRIO_HIGH;

    modes_added = 0;
    modes[0] = '\0';
    nicks[0] = '\0';

    nicks_sent = weechat_hashtable_new (128,
                                        WEECHAT_HASHTABLE_STRING,
                                        WEECHAT_HASHTABLE_STRING,
                                        NULL, NULL);
    if (!nicks_sent)
        return;

    for (ptr_nick = channel->nicks; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
        /* if nick was already sent, ignore it */
        if (weechat_hashtable_has_key (nicks_sent, ptr_nick->name))
            continue;

        for (i = 1; i < argc; i++)
        {
            if (weechat_string_match (ptr_nick->name, argv[i], 0))
            {
                /*
                 * self nick is excluded if both conditions are true:
                 * - set+mode is "-o" or "-h" (commands /deop, /dehalfop)
                 * - one wildcard is used in argument
                 *   (for example: "/deop *" or "/deop fl*")
                 */
                if (set[0] == '-'
                    && (mode[0] == 'o' || mode[0] == 'h')
                    && argv[i][0]
                    && strchr (argv[i], '*')
                    && (strcmp (server->nick, ptr_nick->name) == 0))
                {
                    continue;
                }

                /*
                 * check if the nick mode is already OK, according to
                 * set/mode asked: if already OK, then the nick is ignored
                 */
                if (prefix != ' ')
                {
                    prefix_found = (strchr (ptr_nick->prefixes, prefix) != NULL);
                    if (((set[0] == '+') && prefix_found)
                        || ((set[0] == '-') && !prefix_found))
                    {
                        /*
                         * mode +X and nick has already +X or mode -X and nick
                         * does not have +X
                         */
                        continue;
                    }
                }

                /*
                 * if we reached the max number of modes allowed, send the MODE
                 * command now and flush the modes/nicks strings
                 */
                if (modes_added == max_modes)
                {
                    irc_server_sendf (server, msg_priority, NULL,
                                      "MODE %s %s%s %s",
                                      channel->name, set, modes, nicks);
                    modes[0] = '\0';
                    nicks[0] = '\0';
                    modes_added = 0;
                    /* subsequent messages will have low priority */
                    msg_priority = IRC_SERVER_SEND_OUTQ_PRIO_LOW;
                }

                /* add one mode letter (after +/-) and add the nick in nicks */
                if (strlen (nicks) + 1 + strlen (ptr_nick->name) + 1 < sizeof (nicks))
                {
                    strcat (modes, mode);
                    if (nicks[0])
                        strcat (nicks, " ");
                    strcat (nicks, ptr_nick->name);
                    modes_added++;
                    weechat_hashtable_set (nicks_sent, ptr_nick->name, NULL);
                    /*
                     * nick just added, ignore other arguments that would add
                     * the same nick
                     */
                    break;
                }
            }
        }
    }

    /* send a final MODE command if some nicks are remaining */
    if (modes[0] && nicks[0])
    {
        irc_server_sendf (server, msg_priority, NULL,
                          "MODE %s %s%s %s",
                          channel->name, set, modes, nicks);
    }

    weechat_hashtable_free (nicks_sent);
}

/*
 * Returns arguments with ranges of numbers converted to individual numbers.
 * Arguments that are not range (format: "N1-N2") are kept as-is.
 *
 * For example: ["2" "5-8" "abc"] -> ["2" "5" "6" "7" "8" "abc"]
 */

char **
irc_command_mode_masks_convert_ranges (char **argv, int arg_start)
{
    char **str_masks, **masks, *error1, *error2, *pos, str_number[32];
    int i, length, added;
    long j, number1, number2;

    if (!argv || (arg_start < 0))
        return NULL;

    str_masks = weechat_string_dyn_alloc (128);
    if (!str_masks)
        return NULL;

    for (i = arg_start; argv[i]; i++)
    {
        added = 0;

        length = strlen (argv[i]);
        pos = strchr (argv[i], '-');
        if ((length > 2)
            && pos
            && (argv[i][0] != '-')
            && (argv[i][length - 1] != '-'))
        {
            pos[0] = '\0';
            error1 = NULL;
            number1 = strtol (argv[i], &error1, 10);
            error2 = NULL;
            number2 = strtol (pos + 1, &error2, 10);
            if (error1 && !error1[0]
                && error2 && !error2[0]
                && (number1 > 0) && (number1 < 128)
                && (number2 > 0) && (number2 < 128)
                && (number1 < number2))
            {
                for (j = number1; j <= number2; j++)
                {
                    if (*str_masks[0])
                        weechat_string_dyn_concat (str_masks, " ", -1);
                    snprintf (str_number, sizeof (str_number),
                              "%ld", j);
                    weechat_string_dyn_concat (str_masks, str_number, -1);
                }
                added = 1;
            }
            pos[0] = '-';
        }

        if (!added)
        {
            if (*str_masks[0])
                weechat_string_dyn_concat (str_masks, " ", -1);
            weechat_string_dyn_concat (str_masks, argv[i], -1);
        }
    }

    masks = weechat_string_split (*str_masks, " ", NULL, 0, 0, NULL);

    weechat_string_dyn_free (str_masks, 1);

    return masks;
}

/*
 * Sends mode change for many masks on a channel.
 *
 * Argument "set" is "+" or "-", mode can be "b", "q", or any other mode
 * supported by server.
 *
 * Many messages can be sent if the number of nicks is greater than the server
 * limit (number of modes allowed in a single message). In this case, the first
 * message is sent with high priority, and subsequent messages are sent with low
 * priority.
 */

void
irc_command_mode_masks (struct t_irc_server *server,
                        const char *channel_name,
                        const char *command,
                        const char *set, const char *mode,
                        char **argv, int pos_masks)
{
    int max_modes, modes_added, msg_priority;
    char **modes, **masks, *mask;
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    struct t_irc_modelist *ptr_modelist;
    struct t_irc_modelist_item *ptr_item;
    long number;
    char *error;

    if (irc_mode_get_chanmode_type (server, mode[0]) != 'A')
    {
        weechat_printf (
            NULL,
            _("%s%s: cannot execute command /%s, channel mode \"%s\" is not "
              "supported by server"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, command, mode);
        return;
    }

    modes = weechat_string_dyn_alloc (128);
    if (!modes)
        return;

    masks = weechat_string_dyn_alloc (512);
    if (!masks)
    {
        weechat_string_dyn_free (modes, 1);
        return;
    }

    /* get the max number of modes we can send in a message */
    max_modes = irc_server_get_max_modes (server);

    /*
     * first message has high priority and subsequent messages have low priority
     * (so for example in case of multiple messages, the user can still send
     * some messages which will have higher priority than the "MODE" messages
     * we are sending now)
     */
    msg_priority = IRC_SERVER_SEND_OUTQ_PRIO_HIGH;

    modes_added = 0;

    ptr_channel = irc_channel_search (server, channel_name);
    ptr_modelist = irc_modelist_search (ptr_channel, mode[0]);

    for (; argv[pos_masks]; pos_masks++)
    {
        mask = NULL;

        if (ptr_channel)
        {
            /* use modelist item for number arguments */
            if (ptr_modelist && (set[0] == '-'))
            {
                error = NULL;
                number = strtol (argv[pos_masks], &error, 10);
                if (error && !error[0])
                {
                    ptr_item = irc_modelist_item_search_number (ptr_modelist,
                                                                number - 1);
                    if (ptr_item)
                        mask = strdup (ptr_item->mask);
                }
            }

            /* use default_ban_mask for nick arguments */
            if (!mask
                && !strchr (argv[pos_masks], '!')
                && !strchr (argv[pos_masks], '@'))
            {
                ptr_nick = irc_nick_search (server, ptr_channel,
                                            argv[pos_masks]);
                if (ptr_nick)
                    mask = irc_nick_default_ban_mask (ptr_nick);
            }
        }

        /*
         * if we reached the max number of modes allowed, send the MODE
         * command now and flush the modes/masks strings
         */
        if (*modes[0] && (modes_added == max_modes))
        {
            irc_server_sendf (server, msg_priority, NULL,
                              "MODE %s %s%s %s",
                              channel_name, set, *modes, *masks);

            weechat_string_dyn_copy (modes, NULL);
            weechat_string_dyn_copy (masks, NULL);
            modes_added = 0;

            /* subsequent messages will have low priority */
            msg_priority = IRC_SERVER_SEND_OUTQ_PRIO_LOW;
        }

        /* add one mode letter (after +/-) and add the mask in masks */
        weechat_string_dyn_concat (modes, mode, -1);
        if (*masks[0])
            weechat_string_dyn_concat (masks, " ", -1);
        weechat_string_dyn_concat (masks, (mask) ? mask : argv[pos_masks], -1);
        modes_added++;

        free (mask);
    }

    /* send a final MODE command if some masks are remaining */
    if (*modes[0] && *masks[0])
    {
        irc_server_sendf (server, msg_priority, NULL,
                          "MODE %s %s%s %s",
                          channel_name, set, *modes, *masks);
    }

    weechat_string_dyn_free (modes, 1);
    weechat_string_dyn_free (masks, 1);
}

/*
 * Sends a CTCP ACTION to a channel for a single message
 * (internal function called by irc_command_me_channel).
 */

void
irc_command_me_channel_message (struct t_irc_server *server,
                                const char *channel_name,
                                const char *message)
{
    struct t_arraylist *list_messages;
    int i, list_size;

    list_messages = irc_server_sendf (
        server,
        IRC_SERVER_SEND_OUTQ_PRIO_HIGH | IRC_SERVER_SEND_RETURN_LIST
        | IRC_SERVER_SEND_MULTILINE,
        NULL,
        "PRIVMSG %s :\01ACTION%s%s\01",
        channel_name,
        (message && message[0]) ? " " : "",
        (message && message[0]) ? message : "");
    if (list_messages)
    {
        /* display only if capability "echo-message" is NOT enabled */
        if (!weechat_hashtable_has_key (server->cap_list, "echo-message"))
        {
            list_size = weechat_arraylist_size (list_messages);
            for (i = 0; i < list_size; i++)
            {
                irc_input_user_message_display (
                    server,
                    0,  /* date */
                    0,  /* date_usec */
                    NULL,  /* tags */
                    channel_name,
                    NULL,  /* address */
                    "privmsg",
                    "action",
                    (const char *)weechat_arraylist_get (list_messages, i),
                    1);  /* decode_colors */
            }
        }
        weechat_arraylist_free (list_messages);
    }
}

/*
 * Sends a CTCP ACTION to a channel.
 */

void
irc_command_me_channel (struct t_irc_server *server,
                        const char *channel_name,
                        const char *arguments)
{
    char **list_arguments;
    int i, count_arguments;

    list_arguments = weechat_string_split ((arguments) ? arguments : "",
                                           "\n", NULL, 0, 0, &count_arguments);
    if (list_arguments)
    {
        for (i = 0; i < count_arguments; i++)
        {
            irc_command_me_channel_message (server, channel_name,
                                            list_arguments[i]);
        }
    }
    else
    {
        irc_command_me_channel_message (server, channel_name, "");
    }

    weechat_string_free_split (list_arguments);
}

/*
 * Sends a CTCP ACTION to all channels of a server.
 */

void
irc_command_me_all_channels (struct t_irc_server *server, const char *arguments)
{
    struct t_irc_channel *ptr_channel;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            irc_command_me_channel (server, ptr_channel->name, arguments);
    }
}

/*
 * Callback for command "/action": sends an action message to a nick or channel.
 */

IRC_COMMAND_CALLBACK(action)
{
    char **targets;
    int num_targets, i, arg_target, arg_text;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    arg_target = 1;
    arg_text = 2;

    if ((argc >= 5) && (weechat_strcmp (argv[1], "-server") == 0))
    {
        ptr_server = irc_server_search (argv[2]);
        ptr_channel = NULL;
        arg_target = 3;
        arg_text = 4;
    }

    IRC_COMMAND_CHECK_SERVER("action", 1, 1);

    targets = weechat_string_split (argv[arg_target], ",", NULL,
                                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                    0, &num_targets);
    if (!targets)
        WEECHAT_COMMAND_ERROR;

    for (i = 0; i < num_targets; i++)
    {
        if (strcmp (targets[i], "*") == 0)
        {
            if (!ptr_channel
                || ((ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                    && (ptr_channel->type != IRC_CHANNEL_TYPE_PRIVATE)))
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can only be executed in a channel "
                      "or private buffer"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "action *");
            }
            else
            {
                irc_command_me_channel (ptr_server, ptr_channel->name,
                                        argv_eol[arg_text]);
            }
        }
        else
        {
            irc_command_me_channel (ptr_server, targets[i], argv_eol[arg_text]);
        }
    }

    weechat_string_free_split (targets);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/admin": finds information about the administrator of
 * the server.
 */

IRC_COMMAND_CALLBACK(admin)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("admin", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "ADMIN %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "ADMIN");
    }

    return WEECHAT_RC_OK;
}

/*
 * Executes a command on a list of IRC buffers.
 */

void
irc_command_exec_buffers (struct t_weelist *list_buffers,
                          const char *command)
{
    struct t_irc_server *ptr_server;
    struct t_irc_channel *ptr_channel;
    struct t_gui_buffer *ptr_buffer;
    struct t_hashtable *pointers;
    const char *ptr_buffer_name;
    char *cmd_vars_replaced, *cmd_eval;
    int i, list_size;

    list_size = weechat_list_size (list_buffers);
    if (list_size < 1)
        return;

    pointers = weechat_hashtable_new (32,
                                      WEECHAT_HASHTABLE_STRING,
                                      WEECHAT_HASHTABLE_POINTER,
                                      NULL,
                                      NULL);
    if (!pointers)
        return;

    for (i = 0; i < list_size; i++)
    {
        ptr_buffer_name = weechat_list_string (
            weechat_list_get (list_buffers, i));
        ptr_buffer = weechat_buffer_search ("==", ptr_buffer_name);
        if (!ptr_buffer)
            continue;
        irc_buffer_get_server_and_channel (ptr_buffer,
                                           &ptr_server, &ptr_channel);
        if (!ptr_server)
            continue;
        weechat_hashtable_set (pointers, "buffer", ptr_buffer);
        weechat_hashtable_set (pointers, "irc_server", ptr_server);
        if (ptr_channel)
            weechat_hashtable_set (pointers, "irc_channel", ptr_channel);
        else
            weechat_hashtable_remove (pointers, "irc_channel");
        cmd_vars_replaced = irc_message_replace_vars (
            ptr_server,
            (ptr_channel) ? ptr_channel->name : NULL,
            command);
        cmd_eval = weechat_string_eval_expression (
            (cmd_vars_replaced) ? cmd_vars_replaced : command,
            pointers,
            NULL,
            NULL);
        weechat_command (
            (ptr_channel) ? ptr_channel->buffer : ptr_server->buffer,
            (cmd_eval) ? cmd_eval : ((cmd_vars_replaced) ? cmd_vars_replaced : command));
        free (cmd_vars_replaced);
        free (cmd_eval);
    }

    weechat_hashtable_free (pointers);
}

/*
 * Executes a command on all channels (or queries).
 *
 * If server is NULL, executes command on all channels of all connected servers.
 * Special variables $server/$channel/$nick are replaced in command.
 */

void
irc_command_exec_all_channels (struct t_irc_server *server,
                               int channel_type,
                               int all_channels,
                               int parted_channels,
                               int inclusive,
                               const char *str_channels,
                               const char *command)
{
    struct t_irc_server *ptr_server, *next_server;
    struct t_irc_channel *ptr_channel, *next_channel;
    struct t_weelist *list_buffers;
    char **channels;
    int num_channels, picked, i, parted, state_ok;

    if (!command || !command[0])
        return;

    channels = (str_channels && str_channels[0]) ?
        weechat_string_split (str_channels, ",", NULL,
                              WEECHAT_STRING_SPLIT_STRIP_LEFT
                              | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                              | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                              0, &num_channels) : NULL;

    /* build a list of buffer names where the command will be executed */
    list_buffers = weechat_list_new ();
    ptr_server = irc_servers;
    while (ptr_server)
    {
        next_server = ptr_server->next_server;

        if (!server || (ptr_server == server))
        {
            if (ptr_server->is_connected)
            {
                ptr_channel = ptr_server->channels;
                while (ptr_channel)
                {
                    next_channel = ptr_channel->next_channel;

                    parted = ((ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
                              && !ptr_channel->nicks) ?
                        1 : 0;
                    state_ok = (all_channels
                                || (parted_channels && parted)
                                || (!parted_channels && !parted));

                    if ((ptr_channel->type == channel_type) && state_ok)
                    {
                        picked = (inclusive) ? 0 : 1;

                        if (channels)
                        {
                            for (i = 0; i < num_channels; i++)
                            {
                                if (weechat_string_match (ptr_channel->name,
                                                          channels[i], 0))
                                {
                                    picked = (inclusive) ? 1 : 0;
                                    break;
                                }
                            }
                        }

                        if (picked)
                        {
                            weechat_list_add (list_buffers,
                                              weechat_buffer_get_string (
                                                  ptr_channel->buffer,
                                                  "full_name"),
                                              WEECHAT_LIST_POS_END,
                                              NULL);
                        }
                    }

                    ptr_channel = next_channel;
                }
            }
        }

        ptr_server = next_server;
    }

    /* execute the command on channel/pv buffers */
    irc_command_exec_buffers (list_buffers, command);

    weechat_list_free (list_buffers);
    weechat_string_free_split (channels);
}

/*
 * Callback for command "/allchan": executes a command on all channels of all
 * connected servers.
 */

IRC_COMMAND_CALLBACK(allchan)
{
    int i, current_server, all_channels, parted_channels, inclusive;
    const char *ptr_channels, *ptr_command;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    current_server = 0;
    all_channels = 0;
    parted_channels = 0;
    ptr_channels = NULL;
    inclusive = 0;
    ptr_command = argv_eol[1];
    for (i = 1; i < argc; i++)
    {
        if (weechat_strcmp (argv[i], "-current") == 0)
        {
            if (!ptr_server)
            {
                weechat_printf (NULL,
                                _("%s%s: command \"%s\" with option "
                                  "\"%s\" must be executed on "
                                  "irc buffer (server, channel or private)"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "allchan", "-current");
                return WEECHAT_RC_OK;
            }
            current_server = 1;
            ptr_command = argv_eol[i + 1];
        }
        else if (weechat_strcmp (argv[i], "-all") == 0)
        {
            all_channels = 1;
            parted_channels = 0;
            ptr_command = argv_eol[i + 1];
        }
        else if (weechat_strcmp (argv[i], "-parted") == 0)
        {
            parted_channels = 1;
            all_channels = 0;
            ptr_command = argv_eol[i + 1];
        }
        else if (weechat_strncmp (argv[i], "-exclude=", 9) == 0)
        {
            ptr_channels = argv[i] + 9;
            ptr_command = argv_eol[i + 1];
            inclusive = 0;
        }
        else if (weechat_strncmp (argv[i], "-include=", 9) == 0)
        {
            ptr_channels = argv[i] + 9;
            ptr_command = argv_eol[i + 1];
            inclusive = 1;
        }
        else
            break;
    }

    if (ptr_command && ptr_command[0])
    {
        weechat_buffer_set (NULL, "hotlist", "-");
        irc_command_exec_all_channels ((current_server) ? ptr_server : NULL,
                                       IRC_CHANNEL_TYPE_CHANNEL,
                                       all_channels,
                                       parted_channels,
                                       inclusive,
                                       ptr_channels,
                                       ptr_command);
        weechat_buffer_set (NULL, "hotlist", "+");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/allpv": executes a command on all privates of all
 * connected servers.
 */

IRC_COMMAND_CALLBACK(allpv)
{
    int i, current_server, inclusive;
    const char *ptr_channels, *ptr_command;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    current_server = 0;
    ptr_channels = NULL;
    inclusive = 0;
    ptr_command = argv_eol[1];
    for (i = 1; i < argc; i++)
    {
        if (weechat_strcmp (argv[i], "-current") == 0)
        {
            if (!ptr_server)
            {
                weechat_printf (NULL,
                                _("%s%s: command \"%s\" with option "
                                  "\"%s\" must be executed on "
                                  "irc buffer (server, channel or private)"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                                "allpv", "-current");
                return WEECHAT_RC_OK;
            }
            current_server = 1;
            ptr_command = argv_eol[i + 1];
        }
        else if (weechat_strncmp (argv[i], "-exclude=", 9) == 0)
        {
            ptr_channels = argv[i] + 9;
            ptr_command = argv_eol[i + 1];
            inclusive = 0;
        }
        else if (weechat_strncmp (argv[i], "-include=", 9) == 0)
        {
            ptr_channels = argv[i] + 9;
            ptr_command = argv_eol[i + 1];
            inclusive = 1;
        }
        else
            break;
    }

    if (ptr_command && ptr_command[0])
    {
        weechat_buffer_set (NULL, "hotlist", "-");
        irc_command_exec_all_channels ((current_server) ? ptr_server : NULL,
                                       IRC_CHANNEL_TYPE_PRIVATE,
                                       1,  /* all channels */
                                       0,  /* parted channels */
                                       inclusive,
                                       ptr_channels,
                                       ptr_command);
        weechat_buffer_set (NULL, "hotlist", "+");
    }

    return WEECHAT_RC_OK;
}

/*
 * Executes a command on all connected channels.
 *
 * Special variables $server/$channel/$nick are replaced in command.
 */

void
irc_command_exec_all_servers (int inclusive, const char *str_servers, const char *command)
{
    struct t_irc_server *ptr_server, *next_server;
    struct t_weelist *list_buffers;
    char **servers;
    int num_servers, picked, i;

    if (!command || !command[0])
        return;

    servers = (str_servers && str_servers[0]) ?
        weechat_string_split (str_servers, ",", NULL,
                              WEECHAT_STRING_SPLIT_STRIP_LEFT
                              | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                              | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                              0, &num_servers) : NULL;

    /* build a list of buffer names where the command will be executed */
    list_buffers = weechat_list_new ();
    ptr_server = irc_servers;
    while (ptr_server)
    {
        next_server = ptr_server->next_server;

        if (ptr_server->is_connected)
        {
            picked = (inclusive) ? 0 : 1;

            if (servers)
            {
                for (i = 0; i < num_servers; i++)
                {
                    if (weechat_string_match (ptr_server->name,
                                              servers[i], 1))
                    {
                        picked = (inclusive) ? 1 : 0;
                        break;
                    }
                }
            }

            if (picked)
            {
                weechat_list_add (list_buffers,
                                  weechat_buffer_get_string (
                                      ptr_server->buffer,
                                      "full_name"),
                                  WEECHAT_LIST_POS_END,
                                  NULL);
            }
        }

        ptr_server = next_server;
    }

    /* execute the command on server buffers */
    irc_command_exec_buffers (list_buffers, command);

    weechat_list_free (list_buffers);
    weechat_string_free_split (servers);
}

/*
 * Callback for command "/allserv": executes a command on all connected servers.
 */

IRC_COMMAND_CALLBACK(allserv)
{
    int i, inclusive;
    const char *ptr_servers, *ptr_command;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    ptr_servers = NULL;
    inclusive = 0;
    ptr_command = argv_eol[1];
    for (i = 1; i < argc; i++)
    {
        if (weechat_strncmp (argv[i], "-exclude=", 9) == 0)
        {
            ptr_servers = argv[i] + 9;
            ptr_command = argv_eol[i + 1];
            inclusive = 0;
        }
        else if (weechat_strncmp (argv[i], "-include=", 9) == 0)
        {
            ptr_servers = argv[i] + 9;
            ptr_command = argv_eol[i + 1];
            inclusive = 1;
        }
        else
            break;
    }

    if (ptr_command && ptr_command[0])
    {
        weechat_buffer_set (NULL, "hotlist", "-");
        irc_command_exec_all_servers (inclusive, ptr_servers, ptr_command);
        weechat_buffer_set (NULL, "hotlist", "+");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/auth": authenticates with SASL.
 */

IRC_COMMAND_CALLBACK(auth)
{
    char str_msg_auth[512], *str_msg_auth_upper;
    int sasl_mechanism;

    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("auth", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (ptr_server->sasl_temp_username)
    {
        free (ptr_server->sasl_temp_username);
        ptr_server->sasl_temp_username = NULL;
    }
    if (ptr_server->sasl_temp_password)
    {
        free (ptr_server->sasl_temp_password);
        ptr_server->sasl_temp_password = NULL;
    }

    if ((argc < 3) && !irc_server_sasl_enabled (ptr_server))
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can only be executed if SASL is enabled "
              "via server options \"sasl_*\" (or you must give username and "
              "password)"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "auth");
        return WEECHAT_RC_OK;
    }

    if (weechat_hashtable_has_key (ptr_server->cap_list, "sasl"))
    {
        /* SASL capability already enabled, authenticate */
        sasl_mechanism = IRC_SERVER_OPTION_ENUM(
            ptr_server, IRC_SERVER_OPTION_SASL_MECHANISM);
        if ((sasl_mechanism >= 0)
            && (sasl_mechanism < IRC_NUM_SASL_MECHANISMS))
        {
            if (argc > 2)
            {
                ptr_server->sasl_temp_username = strdup (argv[1]);
                ptr_server->sasl_temp_password = strdup (argv_eol[2]);
            }
            snprintf (str_msg_auth, sizeof (str_msg_auth),
                      "AUTHENTICATE %s",
                      irc_sasl_mechanism_string[sasl_mechanism]);
            str_msg_auth_upper = weechat_string_toupper (str_msg_auth);
            if (str_msg_auth_upper)
            {
                irc_server_sendf (ptr_server,
                                  IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                                  str_msg_auth_upper);
                free (str_msg_auth_upper);
            }
        }
    }
    else
    {
        /* "sasl" capability supported by the server? */
        if (weechat_hashtable_has_key (ptr_server->cap_ls, "sasl"))
        {
            /*
             * request "sasl" capability, then the server should ask
             * immediately to authenticate by sending a message
             * "AUTHENTICATE +"
             */
            if (argc > 2)
            {
                ptr_server->sasl_temp_username = strdup (argv[1]);
                ptr_server->sasl_temp_password = strdup (argv_eol[2]);
            }
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "CAP REQ sasl");
        }
        else
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: SASL is not supported by the server"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/autojoin": configure the server option "autojoin".
 */

IRC_COMMAND_CALLBACK(autojoin)
{
    struct t_irc_channel *ptr_channel2;
    const char *ptr_autojoin;
    char *old_autojoin, *autojoin;
    enum t_irc_join_sort sort;
    int i;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("autojoin", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    ptr_autojoin = IRC_SERVER_OPTION_STRING(ptr_server,
                                            IRC_SERVER_OPTION_AUTOJOIN);

    /* join channels in server "autojoin" option */
    if (weechat_strcmp (argv[1], "join") == 0)
    {
        if (ptr_autojoin)
        {
            autojoin = irc_server_eval_expression (ptr_server, ptr_autojoin);
            if (autojoin && autojoin[0])
            {
                irc_command_join_server (ptr_server, autojoin, 0, 0);
            }
            free (autojoin);
        }
        return WEECHAT_RC_OK;
    }

    old_autojoin = strdup ((ptr_autojoin) ? ptr_autojoin : "");

    /* add channel(s) */
    if (weechat_strcmp (argv[1], "add") == 0)
    {
        if (argc < 3)
        {
            /* add current channel */
            if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can only be executed in a channel "
                      "buffer"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "autojoin add");
                goto end;
            }
            irc_join_add_channel_to_autojoin (ptr_server, ptr_channel->name,
                                              ptr_channel->key);
        }
        for (i = 2; i < argc; i++)
        {
            ptr_channel2 = irc_channel_search (ptr_server, argv[i]);
            if (ptr_channel2)
            {
                irc_join_add_channel_to_autojoin (ptr_server,
                                                  ptr_channel2->name,
                                                  ptr_channel2->key);
            }
            else
            {
                irc_join_add_channel_to_autojoin (ptr_server, argv[i], NULL);
            }
        }
        goto end;
    }

    /* add raw channel(s) */
    if (weechat_strcmp (argv[1], "addraw") == 0)
    {
        if (argc < 3)
        {
            free (old_autojoin);
            WEECHAT_COMMAND_MIN_ARGS(3, "addraw");
        }
        irc_join_add_channels_to_autojoin (ptr_server, argv_eol[2]);
        goto end;
    }

    /* delete channel(s) */
    if (weechat_strcmp (argv[1], "del") == 0)
    {
        if (argc < 3)
        {
            /* delete current channel */
            if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can only be executed in a channel "
                      "buffer"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "autojoin add");
                goto end;
            }
            irc_join_remove_channel_from_autojoin (ptr_server,
                                                   ptr_channel->name);
        }
        for (i = 2; i < argc; i++)
        {
            irc_join_remove_channel_from_autojoin (ptr_server, argv[i]);
        }
        goto end;
    }

    /* apply currently joined channels in server "autojoin" option */
    if (weechat_strcmp (argv[1], "apply") == 0)
    {
        irc_join_save_channels_to_autojoin (ptr_server);
        goto end;
    }

    /* sort channels */
    if (weechat_strcmp (argv[1], "sort") == 0)
    {
        sort = ((argc > 2) && (weechat_strcmp (argv[2], "buffer") == 0)) ?
            IRC_JOIN_SORT_BUFFER : IRC_JOIN_SORT_ALPHA;
        irc_join_sort_autojoin (ptr_server, sort);
        goto end;
    }

end:
    ptr_autojoin = IRC_SERVER_OPTION_STRING(ptr_server,
                                            IRC_SERVER_OPTION_AUTOJOIN);
    if ((old_autojoin && !ptr_autojoin) || (!old_autojoin && ptr_autojoin)
        || (old_autojoin && ptr_autojoin
            && (strcmp (old_autojoin, ptr_autojoin) != 0)))
    {
        if (old_autojoin && old_autojoin[0])
        {
            weechat_printf (ptr_server->buffer,
                            _("Autojoin changed from \"%s\" to \"%s\""),
                            old_autojoin,
                            ptr_autojoin);
        }
        else
        {
            weechat_printf (ptr_server->buffer,
                            _("Autojoin changed from empty value to \"%s\""),
                            ptr_autojoin);
        }
    }
    free (old_autojoin);

    return WEECHAT_RC_OK;
}

/*
 * Displays away on all channels of all servers.
 */

void
irc_command_display_away (struct t_irc_server *server, const char *string1,
                          const char *string2)
{
    struct t_irc_channel *ptr_channel;

    for (ptr_channel = server->channels; ptr_channel;
         ptr_channel = ptr_channel->next_channel)
    {
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            || (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE))
        {
            weechat_printf_date_tags (ptr_channel->buffer,
                                      0,
                                      "away_info",
                                      "%s[%s%s%s %s: %s%s]",
                                      IRC_COLOR_CHAT_DELIMITERS,
                                      IRC_COLOR_CHAT_NICK_SELF,
                                      server->nick,
                                      IRC_COLOR_RESET,
                                      string1,
                                      string2,
                                      IRC_COLOR_CHAT_DELIMITERS);
        }
    }
}

/*
 * Toggles away status for one server.
 */

void
irc_command_away_server (struct t_irc_server *server, const char *arguments,
                         int reset_unread_marker)
{
    char *string, buffer[4096];
    time_t time_now, elapsed;
    struct t_irc_channel *ptr_channel;

    if (!server)
        return;

    if (arguments)
    {
        free (server->away_message);
        server->away_message = strdup (arguments);

        /* if server is connected, send away command now */
        if (server->is_connected)
        {
            server->is_away = 1;
            server->away_time = time (NULL);
            irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "AWAY :%s", arguments);
            if (weechat_config_enum (irc_config_look_display_away) != IRC_CONFIG_DISPLAY_AWAY_OFF)
            {
                string = irc_color_decode (arguments,
                                           weechat_config_boolean (irc_config_network_colors_send));
                if (weechat_config_enum (irc_config_look_display_away) == IRC_CONFIG_DISPLAY_AWAY_LOCAL)
                {
                    irc_command_display_away (server, "away",
                                              (string) ? string : arguments);
                }
                else
                {
                    snprintf (buffer, sizeof (buffer), "is away: %s",
                              (string) ? string : arguments);
                    irc_command_me_all_channels (server, buffer);
                }
                free (string);
            }
            irc_server_set_away (server, server->nick, 1);

            /* reset "unread" indicator on server and channels/pv buffers */
            if (reset_unread_marker)
            {
                if (weechat_buffer_get_integer (server->buffer, "num_displayed") > 0)
                    weechat_buffer_set (server->buffer, "unread", "");
                for (ptr_channel = server->channels; ptr_channel;
                     ptr_channel = ptr_channel->next_channel)
                {
                    if (weechat_buffer_get_integer (ptr_channel->buffer, "num_displayed") > 0)
                        weechat_buffer_set (ptr_channel->buffer, "unread", "");
                }
            }

            /* ask refresh for "away" item */
            weechat_bar_item_update ("away");
        }
        else
        {
            /*
             * server not connected, store away for future usage
             * (when connecting to server)
             */
            string = irc_color_decode (arguments,
                                       weechat_config_boolean (irc_config_network_colors_send));
            weechat_printf (server->buffer,
                            _("%s: future away: %s"),
                            IRC_PLUGIN_NAME,
                            (string) ? string : arguments);
            free (string);
        }
    }
    else
    {
        if (server->away_message)
        {
            free (server->away_message);
            server->away_message = NULL;
        }

        /* if server is connected, send away command now */
        if (server->is_connected)
        {
            irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "AWAY");
            server->is_away = 0;
            if (server->away_time != 0)
            {
                time_now = time (NULL);
                elapsed = (time_now >= server->away_time) ?
                    time_now - server->away_time : 0;
                server->away_time = 0;
                if (weechat_config_enum (irc_config_look_display_away) != IRC_CONFIG_DISPLAY_AWAY_OFF)
                {
                    if (weechat_config_enum (irc_config_look_display_away) == IRC_CONFIG_DISPLAY_AWAY_LOCAL)
                    {
                        snprintf (buffer, sizeof (buffer),
                                  "gone %.2ld:%.2ld:%.2ld",
                                  (long int)(elapsed / 3600),
                                  (long int)((elapsed / 60) % 60),
                                  (long int)(elapsed % 60));
                        irc_command_display_away (server, "back", buffer);
                    }
                    else
                    {
                        snprintf (buffer, sizeof (buffer),
                                  "is back (gone %.2ld:%.2ld:%.2ld)",
                                  (long int)(elapsed / 3600),
                                  (long int)((elapsed / 60) % 60),
                                  (long int)(elapsed % 60));
                        irc_command_me_all_channels (server, buffer);
                    }
                }
            }
            irc_server_set_away (server, server->nick, 0);
        }
        else
        {
            /*
             * server not connected, remove away message but do not send
             * anything
             */
            weechat_printf (server->buffer,
                            _("%s: future away removed"),
                            IRC_PLUGIN_NAME);
        }

        /* ask refresh for "away" item */
        weechat_bar_item_update ("away");
    }
}

/*
 * Callback for command "/away": toggles away status.
 */

IRC_COMMAND_CALLBACK(away)
{
    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if ((argc >= 2) && (weechat_strcmp (argv[1], "-all") == 0))
    {
        weechat_buffer_set (NULL, "hotlist", "-");
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->buffer)
            {
                irc_command_away_server (ptr_server,
                                         (argc > 2) ? argv_eol[2] : NULL,
                                         1);
            }
        }
        weechat_buffer_set (NULL, "hotlist", "+");
    }
    else if (ptr_server)
    {
        weechat_buffer_set (NULL, "hotlist", "-");
        irc_command_away_server (ptr_server, argv_eol[1], 1);
        weechat_buffer_set (NULL, "hotlist", "+");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command /away when it's run ("command_run" hooked).
 */

int
irc_command_run_away (const void *pointer, void *data,
                      struct t_gui_buffer *buffer,
                      const char *command)
{
    int argc;
    char **argv, **argv_eol;

    argv = weechat_string_split (command, " ", NULL,
                                 WEECHAT_STRING_SPLIT_STRIP_LEFT
                                 | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                 | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                 0, &argc);
    argv_eol = weechat_string_split (command, " ", NULL,
                                     WEECHAT_STRING_SPLIT_STRIP_LEFT
                                     | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                     | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS
                                     | WEECHAT_STRING_SPLIT_KEEP_EOL,
                                     0, NULL);

    if (argv && argv_eol)
    {
        irc_command_away (pointer, data, buffer, argc, argv, argv_eol);
    }

    weechat_string_free_split (argv);
    weechat_string_free_split (argv_eol);

    return WEECHAT_RC_OK;
}

/*
 * Sends a ban/unban command to the server, as "MODE [+/-]b nick".
 *
 * Argument "mode" can be "+b" for ban or "-b" for unban.
 */

void
irc_command_send_ban (struct t_irc_server *server,
                      const char *channel_name,
                      const char *mode,
                      const char *nick)
{
    struct t_irc_channel *ptr_channel;
    struct t_irc_nick *ptr_nick;
    char *mask;

    mask = NULL;

    if (!strchr (nick, '!') && !strchr (nick, '@'))
    {
        ptr_channel = irc_channel_search (server, channel_name);
        if (ptr_channel)
        {
            ptr_nick = irc_nick_search (server, ptr_channel, nick);
            if (ptr_nick)
                mask = irc_nick_default_ban_mask (ptr_nick);
        }
    }

    irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "MODE %s %s %s",
                      channel_name,
                      mode,
                      (mask) ? mask : nick);

    free (mask);
}

/*
 * Callback for command "/ban": bans nicks or hosts.
 */

IRC_COMMAND_CALLBACK(ban)
{
    char *pos_channel;
    int pos_args;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("ban", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if (argc > 1)
    {
        if (irc_channel_is_channel (ptr_server, argv[1]))
        {
            pos_channel = argv[1];
            pos_args = 2;
        }
        else
        {
            pos_channel = NULL;
            pos_args = 1;
        }

        /* channel not given, use default buffer */
        if (!pos_channel)
        {
            if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
                pos_channel = ptr_channel->name;
            else
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can only be executed in a channel "
                      "buffer"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "ban");
                return WEECHAT_RC_OK;
            }
        }

        if (argv[pos_args])
        {
            irc_command_mode_masks (ptr_server, pos_channel,
                                    "ban", "+", "b",
                                    argv, pos_args);
        }
        else
        {
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "MODE %s +b",
                              pos_channel);
        }
    }
    else
    {
        if (!ptr_channel)
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can only be executed in a channel "
                  "buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "ban");
            return WEECHAT_RC_OK;
        }
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s +b", ptr_channel->name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/cap": client capability negotiation.
 *
 * Docs on capability negotiation:
 *   https://datatracker.ietf.org/doc/html/draft-mitchell-irc-capabilities-01
 *   https://ircv3.net/specs/extensions/capability-negotiation
 */

IRC_COMMAND_CALLBACK(cap)
{
    char *cap_cmd;

    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("cap", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc > 1)
    {
        cap_cmd = weechat_string_toupper (argv[1]);
        if (!cap_cmd)
            WEECHAT_COMMAND_ERROR;

        if ((strcmp (cap_cmd, "LS") == 0) && !argv_eol[2])
        {
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "CAP LS " IRC_SERVER_VERSION_CAP);
        }
        else
        {
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "CAP %s%s%s",
                              cap_cmd,
                              (argv_eol[2]) ? " :" : "",
                              (argv_eol[2]) ? argv_eol[2] : "");
        }

        free (cap_cmd);
    }
    else
    {
        /*
         * by default, show supported capabilities and capabilities currently
         * enabled
         */
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "CAP LS " IRC_SERVER_VERSION_CAP);
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "CAP LIST");
    }

    return WEECHAT_RC_OK;
}

/*
 * Connects to one server.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_command_connect_one_server (struct t_irc_server *server,
                                int switch_address, int no_join)
{
    if (!server)
        return 0;

    if (server->is_connected)
    {
        weechat_printf (
            NULL,
            _("%s%s: already connected to server \"%s\"!"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, server->name);
        return 0;
    }
    if (server->hook_connect)
    {
        weechat_printf (
            NULL,
            _("%s%s: currently connecting to server \"%s\"!"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, server->name);
        return 0;
    }

    if (switch_address)
        irc_server_switch_address (server, 0);

    server->disable_autojoin = no_join;

    if (irc_server_connect (server))
    {
        server->reconnect_delay = 0;
        server->reconnect_start = 0;
    }

    /* connect OK */
    return 1;
}

/*
 * Callback for command "/connect": connects to server(s).
 */

IRC_COMMAND_CALLBACK(connect)
{
    int i, nb_connect, connect_ok, all_servers, all_opened, switch_address;
    int no_join, autoconnect;
    char *name;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    connect_ok = 1;

    all_servers = 0;
    all_opened = 0;
    switch_address = 0;
    no_join = 0;
    autoconnect = 0;
    for (i = 1; i < argc; i++)
    {
        if (weechat_strcmp (argv[i], "-all") == 0)
            all_servers = 1;
        else if (weechat_strcmp (argv[i], "-open") == 0)
            all_opened = 1;
        else if (weechat_strcmp (argv[i], "-switch") == 0)
            switch_address = 1;
        else if (weechat_strcmp (argv[i], "-nojoin") == 0)
            no_join = 1;
        else if (weechat_strcmp (argv[i], "-auto") == 0)
            autoconnect = 1;
    }

    if (all_opened)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->buffer
                && !ptr_server->is_connected && (!ptr_server->hook_connect))
            {
                if (!irc_command_connect_one_server (ptr_server,
                                                     switch_address, no_join))
                {
                    connect_ok = 0;
                }
            }
        }
        return (connect_ok) ? WEECHAT_RC_OK : WEECHAT_RC_ERROR;
    }
    else if (all_servers)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (!ptr_server->is_connected && (!ptr_server->hook_connect))
            {
                if (!irc_command_connect_one_server (ptr_server,
                                                     switch_address, no_join))
                {
                    connect_ok = 0;
                }
            }
        }
        return (connect_ok) ? WEECHAT_RC_OK : WEECHAT_RC_ERROR;
    }
    else if (autoconnect)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (!ptr_server->is_connected && (!ptr_server->hook_connect)
                && (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_AUTOCONNECT)))
            {
                if (!irc_command_connect_one_server (ptr_server,
                                                     switch_address, no_join))
                {
                    connect_ok = 0;
                }
            }
        }
        return (connect_ok) ? WEECHAT_RC_OK : WEECHAT_RC_ERROR;
    }
    else
    {
        nb_connect = 0;
        for (i = 1; i < argc; i++)
        {
            if (argv[i][0] != '-')
            {
                nb_connect++;
                ptr_server = irc_server_search (argv[i]);
                if (ptr_server)
                {
                    irc_server_apply_command_line_options (ptr_server,
                                                           argc, argv);
                    if (!irc_command_connect_one_server (ptr_server,
                                                         switch_address,
                                                         no_join))
                    {
                        connect_ok = 0;
                    }
                }
                else if (weechat_config_boolean (irc_config_look_temporary_servers))
                {
                    if ((strncmp (argv[i], "irc", 3) == 0)
                        && strstr (argv[i], "://"))
                    {
                        /* read server using URL format */
                        ptr_server = irc_server_alloc_with_url (argv[i]);
                        if (ptr_server)
                        {
                            irc_server_apply_command_line_options (ptr_server,
                                                                   argc, argv);
                            if (!irc_command_connect_one_server (ptr_server, 0, 0))
                                connect_ok = 0;
                        }
                    }
                    else
                    {
                        /* add server with address */
                        name = irc_server_get_name_without_port (argv[i]);
                        ptr_server = irc_server_alloc ((name) ? name : argv[i]);
                        free (name);
                        if (ptr_server)
                        {
                            ptr_server->temp_server = 1;
                            weechat_config_option_set (
                                ptr_server->options[IRC_SERVER_OPTION_ADDRESSES],
                                argv[i], 1);
                            weechat_printf (
                                NULL,
                                _("%s: server added: %s%s%s%s%s"),
                                IRC_PLUGIN_NAME,
                                IRC_COLOR_CHAT_SERVER,
                                ptr_server->name,
                                IRC_COLOR_RESET,
                                _(" (temporary)"),
                                "");
                            irc_server_apply_command_line_options (ptr_server,
                                                                   argc, argv);
                            if (!irc_command_connect_one_server (ptr_server, 0, 0))
                                connect_ok = 0;
                        }
                    }
                    if (!ptr_server)
                    {
                        weechat_printf (
                            NULL,
                            _("%s%s: unable to add temporary server \"%s\" "
                              "(check if there is already a server with this "
                              "name)"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME, argv[i]);
                    }
                }
                else
                {
                    weechat_printf (
                        NULL,
                        _("%s%s: unable to add temporary server \"%s\" "
                          "because the addition of temporary servers with "
                          "command /connect is currently disabled"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, argv[i]);
                    weechat_printf (
                        NULL,
                        _("%s%s: if you want to add a standard server, "
                          "use the command \"/server add\" (see /help "
                          "server); if you really want to add a temporary "
                          "server (NOT SAVED), turn on the option "
                          "irc.look.temporary_servers"),
                        weechat_prefix ("error"),
                        IRC_PLUGIN_NAME);
                }
            }
            else
            {
                if (weechat_strcmp (argv[i], "-port") == 0)
                    i++;
            }
        }
        if (nb_connect == 0)
        {
            connect_ok = irc_command_connect_one_server (ptr_server,
                                                         switch_address,
                                                         no_join);
        }
    }

    return (connect_ok) ? WEECHAT_RC_OK : WEECHAT_RC_ERROR;
}

/*
 * Callback for command "/ctcp": sends a CTCP message.
 */

IRC_COMMAND_CALLBACK(ctcp)
{
    char **targets, *ctcp_type, str_time[512];
    const char *ctcp_target, *ctcp_args;
    int num_targets, arg_target, arg_type, arg_args, i;
    struct timeval tv;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(3, "");

    arg_target = 1;
    arg_type = 2;
    arg_args = 3;

    if ((argc >= 5) && (weechat_strcmp (argv[1], "-server") == 0))
    {
        ptr_server = irc_server_search (argv[2]);
        ptr_channel = NULL;
        arg_target = 3;
        arg_type = 4;
        arg_args = 5;
    }

    IRC_COMMAND_CHECK_SERVER("ctcp", 1, 1);

    targets = weechat_string_split (argv[arg_target], ",", NULL,
                                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                    0, &num_targets);
    if (!targets)
        WEECHAT_COMMAND_ERROR;

    ctcp_type = weechat_string_toupper (argv[arg_type]);
    if (!ctcp_type)
    {
        weechat_string_free_split (targets);
        WEECHAT_COMMAND_ERROR;
    }

    if ((strcmp (ctcp_type, "PING") == 0) && !argv_eol[arg_args])
    {
        /* generate argument for PING if not provided */
        gettimeofday (&tv, NULL);
        snprintf (str_time, sizeof (str_time), "%lld %ld",
                  (long long)tv.tv_sec, (long)tv.tv_usec);
        ctcp_args = str_time;
    }
    else
    {
        ctcp_args = argv_eol[arg_args];
    }

    for (i = 0; i < num_targets; i++)
    {
        ctcp_target = targets[i];

        if (strcmp (targets[i], "*") == 0)
        {
            if (!ptr_channel
                || ((ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                    && (ptr_channel->type != IRC_CHANNEL_TYPE_PRIVATE)))
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can only be executed in a channel "
                      "or private buffer"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "ctcp *");
                ctcp_target = NULL;
            }
            else
                ctcp_target = ptr_channel->name;
        }

        if (ctcp_target)
        {
            /* display only if capability "echo-message" is NOT enabled */
            if (!weechat_hashtable_has_key (ptr_server->cap_list, "echo-message"))
            {
                irc_input_user_message_display (
                    ptr_server,
                    0,  /* date */
                    0,  /* date_usec */
                    NULL,  /* tags */
                    ctcp_target,
                    NULL,  /* address */
                    "privmsg",
                    ctcp_type,
                    ctcp_args,
                    1);  /* decode_colors */
            }
            irc_ctcp_send (ptr_server, ctcp_target, ctcp_type, ctcp_args);
        }
    }

    free (ctcp_type);
    weechat_string_free_split (targets);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/cycle": leaves and rejoins a channel.
 */

IRC_COMMAND_CALLBACK(cycle)
{
    char *channel_name, *pos_args, *msg;
    const char *ptr_arg;
    char **channels;
    int i, num_channels;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("cycle", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        if (irc_channel_is_channel (ptr_server, argv[1]))
        {
            channel_name = argv[1];
            pos_args = argv_eol[2];
            channels = weechat_string_split (channel_name, ",", NULL,
                                             WEECHAT_STRING_SPLIT_STRIP_LEFT
                                             | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                             | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                             0, &num_channels);
            if (channels)
            {
                for (i = 0; i < num_channels; i++)
                {
                    ptr_channel = irc_channel_search (ptr_server, channels[i]);
                    /* mark channel as cycling */
                    if (ptr_channel &&
                        (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
                        ptr_channel->cycle = 1;
                }
                weechat_string_free_split (channels);
            }
        }
        else
        {
            if (!ptr_channel)
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can not be executed on a server "
                      "buffer"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "cycle");
                return WEECHAT_RC_OK;
            }

            /* does nothing on private buffer (cycle has no sense!) */
            if (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                return WEECHAT_RC_OK;

            channel_name = ptr_channel->name;
            pos_args = argv_eol[1];
            ptr_channel->cycle = 1;
        }
    }
    else
    {
        if (!ptr_channel)
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can not be executed on a server "
                  "buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "part");
            return WEECHAT_RC_OK;
        }

        /* does nothing on private buffer (cycle has no sense!) */
        if (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
            return WEECHAT_RC_OK;

        channel_name = ptr_channel->name;
        pos_args = NULL;
        ptr_channel->cycle = 1;
    }

    msg = NULL;
    ptr_arg = (pos_args) ?
        pos_args : IRC_SERVER_OPTION_STRING(ptr_server,
                                            IRC_SERVER_OPTION_MSG_PART);
    if (ptr_arg && ptr_arg[0])
    {
        msg = irc_server_get_default_msg (ptr_arg, ptr_server, channel_name,
                                          NULL);
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "PART %s :%s", channel_name, msg);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "PART %s", channel_name);
    }

    free (msg);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/dcc": DCC control (file or chat).
 */

IRC_COMMAND_CALLBACK(dcc)
{
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    char charset_modifier[1024];
    int rc;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("dcc", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(3, "");
    rc = WEECHAT_RC_ERROR;

    /* DCC SEND file */
    if (weechat_strcmp (argv[1], "send") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "send");
        infolist = weechat_infolist_new ();
        if (infolist)
        {
            item = weechat_infolist_new_item (infolist);
            if (item)
            {
                weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                weechat_infolist_new_var_string (item, "plugin_id", ptr_server->name);
                weechat_infolist_new_var_string (item, "type_string", "file_send_passive");
                weechat_infolist_new_var_string (item, "protocol_string", "dcc");
                weechat_infolist_new_var_string (item, "remote_nick", argv[2]);
                weechat_infolist_new_var_string (item, "local_nick", ptr_server->nick);
                weechat_infolist_new_var_string (item, "filename", argv_eol[3]);
                weechat_infolist_new_var_integer (item, "socket", ptr_server->sock);
                rc = weechat_hook_signal_send ("xfer_add",
                                               WEECHAT_HOOK_SIGNAL_POINTER,
                                               infolist);
            }
            weechat_infolist_free (infolist);
        }
        goto end;
    }

    /* DCC CHAT */
    if (weechat_strcmp (argv[1], "chat") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "chat");
        infolist = weechat_infolist_new ();
        if (infolist)
        {
            item = weechat_infolist_new_item (infolist);
            if (item)
            {
                weechat_infolist_new_var_string (item, "plugin_name", weechat_plugin->name);
                weechat_infolist_new_var_string (item, "plugin_id", ptr_server->name);
                weechat_infolist_new_var_string (item, "type_string", "chat_send");
                weechat_infolist_new_var_string (item, "remote_nick", argv[2]);
                weechat_infolist_new_var_string (item, "local_nick", ptr_server->nick);
                snprintf (charset_modifier, sizeof (charset_modifier),
                          "irc.%s.%s", ptr_server->name, argv[2]);
                weechat_infolist_new_var_string (item, "charset_modifier", charset_modifier);
                weechat_infolist_new_var_integer (item, "socket", ptr_server->sock);
                rc = weechat_hook_signal_send ("xfer_add",
                                               WEECHAT_HOOK_SIGNAL_POINTER,
                                               infolist);
            }
            weechat_infolist_free (infolist);
        }
        goto end;
    }

    WEECHAT_COMMAND_ERROR;

end:
    switch (rc)
    {
        case WEECHAT_RC_OK_EAT:
            /* signal has been properly handled by the xfer plugin */
            break;
        case WEECHAT_RC_ERROR:
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: unable to create DCC"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            break;
        default:
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: unable to create DCC, please check that "
                  "the \"xfer\" plugin is loaded"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            break;
    }
    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/dehalfop": removes half operator privileges from
 * nickname(s).
 */

IRC_COMMAND_CALLBACK(dehalfop)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("dehalfop", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can only be executed in a channel buffer"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "dehalfop");
        return WEECHAT_RC_OK;
    }

    if (argc < 2)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s -h %s",
                          ptr_channel->name,
                          ptr_server->nick);
    }
    else
    {
        irc_command_mode_nicks (ptr_server, ptr_channel,
                                "dehalfop", "-", "h", argc, argv);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/deop": removes operator privileges from nickname(s).
 */

IRC_COMMAND_CALLBACK(deop)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("deop", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can only be executed in a channel buffer"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "deop");
        return WEECHAT_RC_OK;
    }

    if (argc < 2)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s -o %s",
                          ptr_channel->name,
                          ptr_server->nick);
    }
    else
    {
        irc_command_mode_nicks (ptr_server, ptr_channel,
                                "deop", "-", "o", argc, argv);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/devoice": removes voice from nickname(s).
 */

IRC_COMMAND_CALLBACK(devoice)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("devoice", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can only be executed in a channel buffer"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "devoice");
        return WEECHAT_RC_OK;
    }

    if (argc < 2)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s -v %s",
                          ptr_channel->name,
                          ptr_server->nick);
    }
    else
    {
        irc_command_mode_nicks (ptr_server, ptr_channel,
                                "devoice", "-", "v", argc, argv);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/die": shutdowns the server.
 */

IRC_COMMAND_CALLBACK(die)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("die", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "DIE %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "DIE");
    }

    return WEECHAT_RC_OK;
}

/*
 * Sends QUIT to a server.
 */

void
irc_command_quit_server (struct t_irc_server *server, const char *arguments)
{
    const char *ptr_arg;
    char *msg;

    if (!server || !server->is_connected)
        return;

    msg = NULL;
    ptr_arg = (arguments) ?
        arguments : IRC_SERVER_OPTION_STRING(server,
                                             IRC_SERVER_OPTION_MSG_QUIT);
    if (ptr_arg && ptr_arg[0])
    {
        msg = irc_server_get_default_msg (ptr_arg, server, NULL, NULL);
        irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_IMMEDIATE, NULL,
                          "QUIT :%s", msg);
    }
    else
    {
        irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_IMMEDIATE, NULL,
                          "QUIT");
    }

    free (msg);
}

/*
 * Disconnects from a server.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_command_disconnect_one_server (struct t_irc_server *server,
                                   const char *reason)
{
    if (!server)
        return 0;

    if ((!server->is_connected) && (!server->hook_connect)
        && (!server->hook_fd) && (server->reconnect_start == 0))
    {
        weechat_printf (
            server->buffer,
            _("%s%s: not connected to server \"%s\"!"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, server->name);
        return 0;
    }
    if (server->reconnect_start > 0)
    {
        weechat_printf (
            server->buffer,
            _("%s: auto-reconnection is cancelled"),
            IRC_PLUGIN_NAME);
    }
    irc_command_quit_server (server, reason);
    irc_server_disconnect (server, 0, 0);

    /* ask refresh for "away" item */
    weechat_bar_item_update ("away");

    /* disconnect OK */
    return 1;
}

/*
 * Callback for command "/disconnect": disconnects from server(s).
 */

IRC_COMMAND_CALLBACK(disconnect)
{
    int disconnect_ok;
    const char *reason;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    reason = (argc > 2) ? argv_eol[2] : NULL;

    if (argc < 2)
    {
        disconnect_ok = irc_command_disconnect_one_server (ptr_server, reason);
    }
    else
    {
        disconnect_ok = 1;

        if (weechat_strcmp (argv[1], "-all") == 0)
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if ((ptr_server->is_connected) || (ptr_server->hook_connect)
                    || (ptr_server->hook_fd)
                    || (ptr_server->reconnect_start != 0))
                {
                    if (!irc_command_disconnect_one_server (ptr_server, reason))
                        disconnect_ok = 0;
                }
            }
        }
        else if (weechat_strcmp (argv[1], "-pending") == 0)
        {
            for (ptr_server = irc_servers; ptr_server;
                 ptr_server = ptr_server->next_server)
            {
                if (!ptr_server->is_connected
                    && (ptr_server->reconnect_start != 0))
                {
                    if (!irc_command_disconnect_one_server (ptr_server, reason))
                        disconnect_ok = 0;
                }
            }
        }
        else
        {
            ptr_server = irc_server_search (argv[1]);
            if (ptr_server)
            {
                if (!irc_command_disconnect_one_server (ptr_server, reason))
                    disconnect_ok = 0;
            }
            else
            {
                weechat_printf (
                    NULL,
                    _("%s%s: server \"%s\" not found"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, argv[1]);
                disconnect_ok = 0;
            }
        }
    }

    return (disconnect_ok) ? WEECHAT_RC_OK : WEECHAT_RC_ERROR;
}

/*
 * Callback for command "/halfop": gives half operator privileges to
 * nickname(s).
 */

IRC_COMMAND_CALLBACK(halfop)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("halfop", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can only be executed in a channel buffer"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "halfop");
        return WEECHAT_RC_OK;
    }

    if (argc < 2)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s +h %s",
                          ptr_channel->name,
                          ptr_server->nick);
    }
    else
    {
        irc_command_mode_nicks (ptr_server, ptr_channel,
                                "halfop", "+", "h", argc, argv);
    }

    return WEECHAT_RC_OK;
}

/*
 * Displays an ignore.
 */

void
irc_command_ignore_display (struct t_irc_ignore *ignore)
{
    weechat_printf (
        NULL,
        _("  %s[%s%d%s]%s mask: %s / server: %s / channel: %s"),
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_RESET,
        ignore->number,
        IRC_COLOR_CHAT_DELIMITERS,
        IRC_COLOR_RESET,
        ignore->mask,
        (ignore->server) ? ignore->server : "*",
        (ignore->channel) ? ignore->channel : "*");
}

/*
 * Callback for command "/ignore": adds or removes ignore.
 */

IRC_COMMAND_CALLBACK(ignore)
{
    struct t_irc_ignore *ptr_ignore;
    char *mask, *regex, *regex2, *ptr_regex, *pos, *server, *channel, *error;
    int length;
    long number;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) argv_eol;

    if ((argc == 1)
        || ((argc == 2) && (weechat_strcmp (argv[1], "list") == 0)))
    {
        /* display all ignores */
        if (irc_ignore_list)
        {
            weechat_printf (NULL, "");
            weechat_printf (NULL, _("%s: ignore list:"), IRC_PLUGIN_NAME);
            for (ptr_ignore = irc_ignore_list; ptr_ignore;
                 ptr_ignore = ptr_ignore->next_ignore)
            {
                irc_command_ignore_display (ptr_ignore);
            }
        }
        else
            weechat_printf (NULL, _("%s: no ignore in list"), IRC_PLUGIN_NAME);
        return WEECHAT_RC_OK;
    }

    /* add ignore */
    if (weechat_strcmp (argv[1], "add") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "add");

        mask = argv[2];
        server = (argc > 3) ? argv[3] : NULL;
        channel = (argc > 4) ? argv[4] : NULL;

        regex = NULL;
        regex2 = NULL;

        if (strncmp (mask, "re:", 3) == 0)
        {
            ptr_regex = mask + 3;
        }
        else
        {
            /* convert mask to regex (escape regex special chars) */
            regex = weechat_string_mask_to_regex (mask);
            ptr_regex = (regex) ? regex : mask;
        }

        /* add "^" and "$" around regex */
        if (strncmp (ptr_regex, "(?", 2) == 0)
        {
            /* add chars after the regex flags */
            pos = strchr (ptr_regex, ')');
            if (pos)
            {
                length = 1 + strlen (ptr_regex) + 1 + 1;
                regex2 = malloc (length);
                if (regex2)
                {
                    pos++;
                    memcpy (regex2, ptr_regex, pos - ptr_regex);
                    regex2[pos - ptr_regex] = '\0';
                    strcat (regex2, "^");
                    strcat (regex2, pos);
                    strcat (regex2, "$");
                    ptr_regex = regex2;
                }
            }
        }
        else
        {
            length = 1 + strlen (ptr_regex) + 1 + 1;
            regex2 = malloc (length);
            if (regex2)
            {
                snprintf (regex2, length, "^%s$", ptr_regex);
                ptr_regex = regex2;
            }
        }

        if (irc_ignore_search (ptr_regex, server, channel))
        {
            free (regex);
            free (regex2);
            weechat_printf (NULL,
                            _("%s%s: ignore already exists"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }

        ptr_ignore = irc_ignore_new (ptr_regex, server, channel);

        free (regex);
        free (regex2);

        if (ptr_ignore)
        {
            weechat_printf (NULL, "");
            weechat_printf (NULL, _("%s: ignore added:"), IRC_PLUGIN_NAME);
            irc_command_ignore_display (ptr_ignore);
        }
        else
        {
            weechat_printf (NULL, _("%s%s: error adding ignore"),
                            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        }

        return WEECHAT_RC_OK;
    }

    /* delete ignore */
    if (weechat_strcmp (argv[1], "del") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "del");

        if (weechat_strcmp (argv[2], "-all") == 0)
        {
            if (irc_ignore_list)
            {
                irc_ignore_free_all ();
                weechat_printf (NULL, _("%s: all ignores deleted"),
                                IRC_PLUGIN_NAME);
            }
            else
            {
                weechat_printf (NULL, _("%s: no ignore in list"),
                                IRC_PLUGIN_NAME);
            }
        }
        else
        {
            error = NULL;
            number = strtol (argv[2], &error, 10);
            if (error && !error[0])
            {
                ptr_ignore = irc_ignore_search_by_number (number);
                if (ptr_ignore)
                {
                    mask = strdup (ptr_ignore->mask);
                    irc_ignore_free (ptr_ignore);
                    weechat_printf (NULL, _("%s: ignore \"%s\" deleted"),
                                    IRC_PLUGIN_NAME, mask);
                    free (mask);
                }
                else
                {
                    weechat_printf (NULL,
                                    _("%s%s: ignore not found"),
                                    weechat_prefix ("error"), IRC_PLUGIN_NAME);
                    return WEECHAT_RC_OK;
                }
            }
            else
            {
                weechat_printf (NULL,
                                _("%s%s: wrong ignore number"),
                                weechat_prefix ("error"), IRC_PLUGIN_NAME);
                return WEECHAT_RC_OK;
            }
        }

        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Callback for command "/info": gets information describing the server.
 */

IRC_COMMAND_CALLBACK(info)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("info", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "INFO %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "INFO");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/invite": invites a nick on a channel.
 */

IRC_COMMAND_CALLBACK(invite)
{
    int i, arg_last_nick;
    char *ptr_channel_name;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("invite", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (argc > 2)
    {
        if (irc_channel_is_channel (ptr_server, argv[argc - 1]))
        {
            arg_last_nick = argc - 2;
            ptr_channel_name = argv[argc - 1];
        }
        else
        {
            if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            {
                arg_last_nick = argc - 1;
                ptr_channel_name = ptr_channel->name;
            }
            else
                goto error;
        }
        for (i = 1; i <= arg_last_nick; i++)
        {
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "INVITE %s %s", argv[i], ptr_channel_name);
        }
    }
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
        {
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "INVITE %s %s",
                              argv[1], ptr_channel->name);
        }
        else
            goto error;
    }

    return WEECHAT_RC_OK;

error:
    weechat_printf (
        ptr_server->buffer,
        _("%s%s: \"%s\" command can only be executed in a channel buffer"),
        weechat_prefix ("error"), IRC_PLUGIN_NAME, "invite");
    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/ison": checks if a nickname is currently on IRC.
 */

IRC_COMMAND_CALLBACK(ison)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("ison", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "ISON :%s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Sends JOIN command to a server.
 */

void
irc_command_join_server (struct t_irc_server *server, const char *arguments,
                         int manual_join, int noswitch)
{
    char *new_args, **channels, **keys, *pos_space, *pos_keys, *pos_channel;
    char *channel_name_lower;
    int i, num_channels, num_keys, length;
    time_t time_now;
    struct t_irc_channel *ptr_channel;

    if ((server->sock < 0) && !server->fake_server)
    {
        weechat_printf (
            NULL,
            _("%s%s: command \"%s\" must be executed on connected irc server"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "join");
        return;
    }

    /* split channels and keys */
    channels = NULL;
    num_channels = 0;
    keys = NULL;
    num_keys = 0;
    pos_space = strchr (arguments, ' ');
    pos_keys = NULL;
    if (pos_space)
    {
        new_args = weechat_strndup (arguments, pos_space - arguments);
        pos_keys = pos_space + 1;
        while (pos_keys[0] == ' ')
        {
            pos_keys++;
        }
        if (pos_keys[0])
        {
            keys = weechat_string_split (pos_keys, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &num_keys);
        }
    }
    else
        new_args = strdup (arguments);

    if (new_args)
    {
        channels = weechat_string_split (new_args, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &num_channels);
        free (new_args);
    }

    if (channels)
    {
        length = strlen (arguments) + num_channels + 1;
        new_args = malloc (length);
        if (new_args)
        {
            if (manual_join)
            {
                snprintf (new_args, length, "%s%s",
                          irc_channel_get_auto_chantype (server, channels[0]),
                          channels[0]);
                ptr_channel = irc_channel_search (server, new_args);
                if (ptr_channel)
                {
                    if (!noswitch)
                    {
                        weechat_buffer_set (ptr_channel->buffer,
                                            "display", "1");
                    }
                }
            }
            new_args[0] = '\0';
            time_now = time (NULL);
            for (i = 0; i < num_channels; i++)
            {
                if (i > 0)
                    strcat (new_args, ",");
                pos_channel = new_args + strlen (new_args);
                strcat (new_args,
                        irc_channel_get_auto_chantype (server, channels[i]));
                strcat (new_args, channels[i]);
                channel_name_lower = weechat_string_tolower (pos_channel);
                if (manual_join || noswitch)
                {
                    if (channel_name_lower)
                    {
                        if (manual_join)
                        {
                            weechat_hashtable_set (server->join_manual,
                                                   channel_name_lower,
                                                   &time_now);
                        }
                        if (noswitch)
                        {
                            weechat_hashtable_set (server->join_noswitch,
                                                   channel_name_lower,
                                                   &time_now);
                        }
                    }
                }
                if (keys && (i < num_keys))
                {
                    ptr_channel = irc_channel_search (server, pos_channel);
                    if (ptr_channel)
                    {
                        free (ptr_channel->key);
                        ptr_channel->key = strdup (keys[i]);
                    }
                    else if (channel_name_lower)
                    {
                        weechat_hashtable_set (server->join_channel_key,
                                               channel_name_lower, keys[i]);
                    }
                }
                if (manual_join
                    && (strcmp (pos_channel, "0") != 0))
                {
                    if (!irc_channel_search (server, pos_channel)
                        && weechat_config_boolean (irc_config_look_buffer_open_before_join))
                    {
                        /*
                         * open the channel buffer immediately (do not wait
                         * for the JOIN sent by server)
                         */
                        irc_channel_create_buffer (
                            server, IRC_CHANNEL_TYPE_CHANNEL, pos_channel,
                            1, 1);
                    }
                }
                free (channel_name_lower);
            }
            if (pos_space)
                strcat (new_args, pos_space);

            irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "JOIN %s", new_args);

            free (new_args);
        }
        weechat_string_free_split (channels);
    }

    weechat_string_free_split (keys);
}

/*
 * Callback for command "/join": joins a new channel.
 */

IRC_COMMAND_CALLBACK(join)
{
    int i, arg_channels, noswitch;
    const char *ptr_type, *ptr_server_name, *ptr_channel_name;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    noswitch = 0;
    arg_channels = 1;

    for (i = 1; i < argc; i++)
    {
        if (weechat_strcmp (argv[i], "-server") == 0)
        {
            if (argc <= i + 1)
                WEECHAT_COMMAND_ERROR;
            ptr_server = irc_server_search (argv[i + 1]);
            if (!ptr_server)
                WEECHAT_COMMAND_ERROR;
            arg_channels = i + 2;
            i++;
        }
        else if (weechat_strcmp (argv[i], "-noswitch") == 0)
        {
            noswitch = 1;
            arg_channels = i + 1;
        }
        else
        {
            arg_channels = i;
            break;
        }
    }

    if (!ptr_server)
    {
        if ((weechat_buffer_get_pointer (buffer,
                                         "plugin") == weechat_irc_plugin))
        {
            ptr_server_name = weechat_buffer_get_string (buffer,
                                                         "localvar_server");
            if (ptr_server_name)
                ptr_server = irc_server_search (ptr_server_name);
        }
    }

    IRC_COMMAND_CHECK_SERVER("join", 1, 1);

    if (arg_channels < argc)
    {
        irc_command_join_server (ptr_server, argv_eol[arg_channels],
                                 1, noswitch);
    }
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            && !ptr_channel->nicks)
        {
            irc_channel_rejoin (ptr_server, ptr_channel, 1, noswitch);
        }
        else
        {
            ptr_type = weechat_buffer_get_string (buffer, "localvar_type");
            ptr_channel_name = weechat_buffer_get_string (buffer,
                                                          "localvar_channel");
            if ((weechat_buffer_get_pointer (buffer,
                                             "plugin") == weechat_irc_plugin)
                && ptr_type && ptr_channel_name
                && (strcmp (ptr_type, "channel") == 0))
            {
                irc_command_join_server (ptr_server, ptr_channel_name,
                                         1, noswitch);
            }
            else
                WEECHAT_COMMAND_ERROR;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Sends a kick message to a channel.
 */

void
irc_command_kick_channel (struct t_irc_server *server,
                          const char *channel_name, const char *nick_name,
                          const char *message)
{
    const char *ptr_msg;
    char *msg;

    msg = NULL;
    ptr_msg = (message && message[0]) ?
        message : IRC_SERVER_OPTION_STRING(server,
                                           IRC_SERVER_OPTION_MSG_KICK);
    if (ptr_msg && ptr_msg[0])
    {
        msg = irc_server_get_default_msg (ptr_msg, server, channel_name,
                                          nick_name);
        irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "KICK %s %s :%s",
                          channel_name, nick_name, msg);
    }
    else
    {
        irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "KICK %s %s",
                          channel_name, nick_name);
    }

    free (msg);
}

/*
 * Callback for command "/kick": forcibly removes a user from a channel.
 */

IRC_COMMAND_CALLBACK(kick)
{
    char *pos_channel, *pos_nick, *pos_comment;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("kick", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (irc_channel_is_channel (ptr_server, argv[1]))
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "");
        pos_channel = argv[1];
        pos_nick = argv[2];
        pos_comment = argv_eol[3];
    }
    else
    {
        if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can only be executed in a channel "
                  "buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "kick");
            return WEECHAT_RC_OK;
        }
        pos_channel = ptr_channel->name;
        pos_nick = argv[1];
        pos_comment = argv_eol[2];
    }

    irc_command_kick_channel (ptr_server, pos_channel, pos_nick,
                              pos_comment);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/kickban": forcibly removes a user from a channel and
 * bans it.
 */

IRC_COMMAND_CALLBACK(kickban)
{
    char *pos_channel, *pos_nick, *nick_only, *pos_comment, *pos, *mask;
    int length;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("kickban", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (irc_channel_is_channel (ptr_server, argv[1]))
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "");
        pos_channel = argv[1];
        pos_nick = argv[2];
        pos_comment = argv_eol[3];
    }
    else
    {
        if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can only be executed in a channel "
                  "buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "kickban");
            return WEECHAT_RC_OK;
        }
        pos_channel = ptr_channel->name;
        pos_nick = argv[1];
        pos_comment = argv_eol[2];
    }

    /* kick nick from channel */
    nick_only = strdup (pos_nick);
    if (!nick_only)
        WEECHAT_COMMAND_ERROR;

    pos = strchr (nick_only, '@');
    if (pos)
        pos[0] = '\0';
    pos = strchr (nick_only, '!');
    if (pos)
        pos[0] = '\0';

    if (strcmp (nick_only, "*") == 0)
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: mask must begin with nick"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME);
        free (nick_only);
        return WEECHAT_RC_OK;
    }

    /* set ban for nick(+host) on channel */
    if (strchr (pos_nick, '@'))
    {
        length = strlen (pos_nick) + 16 + 1;
        mask = malloc (length);
        if (mask)
        {
            pos = strchr (pos_nick, '!');
            snprintf (mask, length, "*!%s",
                      (pos) ? pos + 1 : pos_nick);
            irc_server_sendf (ptr_server,
                              IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "MODE %s +b %s",
                              pos_channel, mask);
            free (mask);
        }
    }
    else
    {
        irc_command_send_ban (ptr_server, pos_channel, "+b",
                              pos_nick);
    }

    /* kick nick */
    irc_command_kick_channel (ptr_server, pos_channel, nick_only, pos_comment);

    free (nick_only);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/kill": closes client-server connection.
 */

IRC_COMMAND_CALLBACK(kill)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("kill", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (argc < 3)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "KILL %s", argv[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "KILL %s :%s", argv[1], argv_eol[2]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/knock": sends a notice to an invitation-only channel,
 * requesting an invite.
 */

IRC_COMMAND_CALLBACK(knock)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("knock", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (argc < 3)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "KNOCK %s", argv[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "KNOCK %s :%s", argv[1], argv_eol[2]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/links": lists all server names which are known by the
 * server answering the query.
 */

IRC_COMMAND_CALLBACK(links)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("links", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "LINKS %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "LINKS");
    }

    return WEECHAT_RC_OK;
}

/*
 * Gets an integer argument given to the /list command.
 */

int
irc_command_list_get_int_arg (int argc, char **argv, int arg_number,
                              int default_value)
{
    long value;
    char *error;

    value = default_value;
    if (argc > arg_number)
    {
        error = NULL;
        value = strtol (argv[arg_number], &error, 10);
        if (!error || error[0])
            value = default_value;
    }
    return (int)value;
}

/*
 * Callback for command "/list": lists channels and their topics.
 */

IRC_COMMAND_CALLBACK(list)
{
    struct t_hashtable *hashtable;
    char buf[512], *ptr_channel_name, *ptr_server_name, *ptr_regex;
    regex_t *new_regexp;
    int i, ret, value, use_list_buffer;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    ptr_channel_name = NULL;
    ptr_server_name = NULL;
    ptr_regex = NULL;
    new_regexp = NULL;
    use_list_buffer = weechat_config_boolean (irc_config_look_list_buffer);

    if ((argc > 0) && (weechat_strcmp (argv[1], "-up") == 0))
    {
        if (ptr_server && ptr_server->list->buffer)
        {
            irc_list_move_line_relative (
                ptr_server,
                -1 * irc_command_list_get_int_arg (argc, argv, 2, 1));
        }
        return WEECHAT_RC_OK;
    }
    if ((argc > 0) && (weechat_strcmp (argv[1], "-down") == 0))
    {
        if (ptr_server && ptr_server->list->buffer)
        {
            irc_list_move_line_relative (
                ptr_server,
                irc_command_list_get_int_arg (argc, argv, 2, 1));
        }
        return WEECHAT_RC_OK;
    }
    if ((argc > 0) && (weechat_strcmp (argv[1], "-go") == 0))
    {
        if (ptr_server && ptr_server->list->buffer)
        {
            if (argc < 3)
                WEECHAT_COMMAND_ERROR;
            value = (weechat_strcmp (argv[2], "end") == 0) ?
                -1 : irc_command_list_get_int_arg (argc, argv, 2, -2);
            if (value < -1)
                WEECHAT_COMMAND_ERROR;
            irc_list_move_line_absolute (ptr_server, value);
        }
        return WEECHAT_RC_OK;
    }
    if ((argc > 0) && (weechat_strcmp (argv[1], "-left") == 0))
    {
        if (ptr_server && ptr_server->list->buffer)
        {
            value = irc_command_list_get_int_arg (
                argc, argv, 2,
                weechat_config_integer (irc_config_look_list_buffer_scroll_horizontal));
            irc_list_scroll_horizontal (ptr_server, -1 * value);
        }
        return WEECHAT_RC_OK;
    }
    if ((argc > 0) && (weechat_strcmp (argv[1], "-right") == 0))
    {
        if (ptr_server && ptr_server->list->buffer)
        {
            value = irc_command_list_get_int_arg (
                argc, argv, 2,
                weechat_config_integer (irc_config_look_list_buffer_scroll_horizontal));
            irc_list_scroll_horizontal (ptr_server, value);
        }
        return WEECHAT_RC_OK;
    }
    if ((argc > 0) && (weechat_strcmp (argv[1], "-join") == 0))
    {
        if (ptr_server && ptr_server->list->buffer)
            irc_list_join_channel (ptr_server);
        return WEECHAT_RC_OK;
    }

    for (i = 1; i < argc; i++)
    {
        if (weechat_strcmp (argv[i], "-server") == 0)
        {
            if (argc <= i + 1)
                WEECHAT_COMMAND_ERROR;
            ptr_server = irc_server_search (argv[i + 1]);
            if (!ptr_server)
                WEECHAT_COMMAND_ERROR;
            i++;
        }
        else if (weechat_strcmp (argv[i], "-re") == 0)
        {
            if (argc <= i + 1)
                WEECHAT_COMMAND_ERROR;
            ptr_regex = argv_eol[i + 1];
            use_list_buffer = 0;
            i++;
        }
        else if (!ptr_channel_name)
            ptr_channel_name = argv[i];
        else if (!ptr_server_name)
            ptr_server_name = argv[i];
        else
            WEECHAT_COMMAND_ERROR;
    }

    IRC_COMMAND_CHECK_SERVER("list", 1, 1);

    if (ptr_regex)
    {
        new_regexp = malloc (sizeof (*new_regexp));
        if (!new_regexp)
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: not enough memory for regular expression"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }
        ret = weechat_string_regcomp (new_regexp,
                                      ptr_regex,
                                      REG_EXTENDED | REG_ICASE | REG_NOSUB);
        if (ret != 0)
        {
            regerror (ret, new_regexp, buf, sizeof (buf));
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" is not a valid regular expression "
                  "(%s)"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                ptr_regex, buf);
            free (new_regexp);
            return WEECHAT_RC_OK;
        }
        if (ptr_server->cmd_list_regexp)
        {
            regfree (ptr_server->cmd_list_regexp);
            free (ptr_server->cmd_list_regexp);
        }
        ptr_server->cmd_list_regexp = new_regexp;
    }
    else if (ptr_server->cmd_list_regexp)
    {
        regfree (ptr_server->cmd_list_regexp);
        free (ptr_server->cmd_list_regexp);
        ptr_server->cmd_list_regexp = NULL;
    }

    if (ptr_server->list && use_list_buffer)
    {
        hashtable = weechat_hashtable_new (32,
                                           WEECHAT_HASHTABLE_STRING,
                                           WEECHAT_HASHTABLE_STRING,
                                           NULL,
                                           NULL);
        if (hashtable)
        {
            weechat_hashtable_set (hashtable, "server", ptr_server->name);
            weechat_hashtable_set (hashtable, "pattern", "list");
            snprintf (buf, sizeof (buf), "server_%s", ptr_server->name);
            weechat_hashtable_set (hashtable, "signal", buf);
            weechat_hook_hsignal_send ("irc_redirect_command", hashtable);
            weechat_hashtable_free (hashtable);
        }

        irc_list_reset (ptr_server);

        if (ptr_server->list->buffer)
            weechat_buffer_clear (ptr_server->list->buffer);
        else
            ptr_server->list->buffer = irc_list_create_buffer (ptr_server);
        if (ptr_server->list->buffer)
        {
            weechat_printf_y (ptr_server->list->buffer, 1,
                              "%s",
                              _("Receiving list of channels, please wait..."));
            irc_list_buffer_set_title (ptr_server);
            weechat_buffer_set (ptr_server->list->buffer, "display", "1");
        }
    }

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "LIST%s%s%s%s",
                      (ptr_channel_name) ? " " : "",
                      (ptr_channel_name) ? ptr_channel_name : "",
                      (ptr_server_name) ? " " : "",
                      (ptr_server_name) ? ptr_server_name : "");

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/lusers": gets statistics about the size of the IRC
 * network.
 */

IRC_COMMAND_CALLBACK(lusers)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("lusers", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "LUSERS %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "LUSERS");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/map": shows a graphical map of the IRC network.
 */

IRC_COMMAND_CALLBACK(map)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("map", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MAP %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MAP");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/me": sends a CTCP ACTION to the current channel.
 */

IRC_COMMAND_CALLBACK(me)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("me", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (!ptr_channel)
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can not be executed on a server buffer"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "me");
        return WEECHAT_RC_OK;
    }

    irc_command_me_channel (ptr_server, ptr_channel->name,
                            (argc > 1) ? argv_eol[1] : NULL);

    return WEECHAT_RC_OK;
}

/*
 * Sends MODE command on a server.
 */

void
irc_command_mode_server (struct t_irc_server *server,
                         const char *command,
                         struct t_irc_channel *channel,
                         const char *arguments,
                         int flags)
{
    if (server && command && (channel || arguments))
    {
        if (channel && arguments)
        {
            irc_server_sendf (server, flags, NULL,
                              "%s %s %s",
                              command, channel->name, arguments);
        }
        else
        {
            irc_server_sendf (server, flags, NULL,
                              "%s %s",
                              command,
                              (channel) ? channel->name : arguments);
        }
    }
}

/*
 * Callback for command "/mode": changes mode for channel/nickname.
 */

IRC_COMMAND_CALLBACK(mode)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("mode", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc > 1)
    {
        if ((argv[1][0] == '+') || (argv[1][0] == '-'))
        {
            /* channel not specified, check we are on channel and use it */
            if (!ptr_channel)
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: you must specify channel for \"%s\" command if "
                      "you're not in a channel"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "mode");
                return WEECHAT_RC_OK;
            }
            irc_command_mode_server (ptr_server, "MODE", ptr_channel,
                                     argv_eol[1],
                                     IRC_SERVER_SEND_OUTQ_PRIO_HIGH);
        }
        else
        {
            /* user gives channel, use arguments as-is */
            irc_command_mode_server (ptr_server, "MODE", NULL, argv_eol[1],
                                     IRC_SERVER_SEND_OUTQ_PRIO_HIGH);
        }
    }
    else
    {
        if (ptr_channel)
        {
            irc_command_mode_server (ptr_server, "MODE", ptr_channel, NULL,
                                     IRC_SERVER_SEND_OUTQ_PRIO_HIGH);
        }
        else
        {
            irc_command_mode_server (ptr_server, "MODE", NULL,
                                     ptr_server->nick,
                                     IRC_SERVER_SEND_OUTQ_PRIO_HIGH);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/motd": gets the "Message Of The Day".
 */

IRC_COMMAND_CALLBACK(motd)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("motd", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MOTD %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MOTD");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/msg": sends a message to a nick or channel.
 */

IRC_COMMAND_CALLBACK(msg)
{
    char **targets;
    int num_targets, i, arg_target, arg_text;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(3, "");

    arg_target = 1;
    arg_text = 2;

    if ((argc >= 5) && (weechat_strcmp (argv[1], "-server") == 0))
    {
        ptr_server = irc_server_search (argv[2]);
        ptr_channel = NULL;
        arg_target = 3;
        arg_text = 4;
    }

    IRC_COMMAND_CHECK_SERVER("msg", 1, 1);

    targets = weechat_string_split (argv[arg_target], ",", NULL,
                                    WEECHAT_STRING_SPLIT_STRIP_LEFT
                                    | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                    | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                    0, &num_targets);
    if (!targets)
        WEECHAT_COMMAND_ERROR;

    for (i = 0; i < num_targets; i++)
    {
        if (strcmp (targets[i], "*") == 0)
        {
            if (!ptr_channel
                || ((ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)
                    && (ptr_channel->type != IRC_CHANNEL_TYPE_PRIVATE)))
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can only be executed in a channel "
                      "or private buffer"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "msg *");
            }
            else
            {
                /* display only if capability "echo-message" is NOT enabled */
                if (!weechat_hashtable_has_key (ptr_server->cap_list, "echo-message"))
                {
                    irc_input_user_message_display (
                        ptr_server,
                        0,  /* date */
                        0,  /* date_usec */
                        NULL,  /* tags */
                        ptr_channel->name,
                        NULL,  /* address */
                        "privmsg",
                        NULL,  /* ctcp_type */
                        argv_eol[arg_text],
                        1);  /* decode_colors */
                }
                irc_server_sendf (ptr_server,
                                  IRC_SERVER_SEND_OUTQ_PRIO_HIGH
                                  | IRC_SERVER_SEND_MULTILINE,
                                  NULL,
                                  "PRIVMSG %s :%s",
                                  ptr_channel->name, argv_eol[arg_text]);
            }
        }
        else
        {
            /* display only if capability "echo-message" is NOT enabled */
            if (!weechat_hashtable_has_key (ptr_server->cap_list, "echo-message"))
            {
                irc_input_user_message_display (
                    ptr_server,
                    0,  /* date */
                    0,  /* date_usec */
                    NULL,  /* tags */
                    targets[i],
                    NULL,  /* address */
                    "privmsg",
                    NULL,  /* ctcp_type */
                    argv_eol[arg_text],
                    1);  /* decode_colors */
            }
            irc_server_sendf (ptr_server,
                              IRC_SERVER_SEND_OUTQ_PRIO_HIGH
                              | IRC_SERVER_SEND_MULTILINE,
                              NULL,
                              "PRIVMSG %s :%s",
                              targets[i], argv_eol[arg_text]);
        }
    }

    weechat_string_free_split (targets);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/names": lists nicknames on channels.
 */

IRC_COMMAND_CALLBACK(names)
{
    int i, arg_channels;
    char filter[2], **channels, *channel_name_lower;
    int num_channels;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("names", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    arg_channels = argc;
    filter[0] = '\0';
    filter[1] = '\0';

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (weechat_strcmp (argv[i], "-count") == 0)
                filter[0] = '#';
            else if (argv[i][1])
                filter[0] = argv[i][1];
        }
        else
        {
            arg_channels = i;
            break;
        }
    }

    if ((arg_channels >= argc)
        && (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL)))
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can only be executed in a channel "
              "buffer"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "names");
        return WEECHAT_RC_OK;
    }

    if (filter[0])
    {
        channels = weechat_string_split (
            (arg_channels < argc) ? argv_eol[arg_channels] : ptr_channel->name,
            ",", NULL, 0, 0, &num_channels);
        if (channels)
        {
            for (i = 0; i < num_channels; i++)
            {
                channel_name_lower = weechat_string_tolower (channels[i]);
                if (channel_name_lower)
                {
                    weechat_hashtable_set (ptr_server->names_channel_filter,
                                           channel_name_lower,
                                           filter);
                    free (channel_name_lower);
                }
            }
            weechat_string_free_split (channels);
        }
    }

    irc_server_sendf (
        ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
        "NAMES %s",
        (arg_channels < argc) ? argv_eol[arg_channels] : ptr_channel->name);

    return WEECHAT_RC_OK;
}

/*
 * Changes nickname on a server.
 */

void
irc_send_nick_server (struct t_irc_server *server, const char *nickname)
{
    if (!server)
        return;

    if (server->is_connected)
    {
        irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "NICK %s%s",
                          (nickname && strchr (nickname, ':')) ? ":" : "",
                          nickname);
    }
    else
        irc_server_set_nick (server, nickname);
}

/*
 * Callback for command "/nick": changes nickname.
 */

IRC_COMMAND_CALLBACK(nick)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("nick", 0, 0);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (argc > 2)
    {
        if (weechat_strcmp (argv[1], "-all") != 0)
            WEECHAT_COMMAND_ERROR;
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            irc_send_nick_server (ptr_server, argv[2]);
        }
    }
    else
        irc_send_nick_server (ptr_server, argv[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/notice": sends notice message.
 */

IRC_COMMAND_CALLBACK(notice)
{
    const char *ptr_message;
    int i, arg_target, arg_text, list_size;
    struct t_arraylist *list_messages;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(3, "");

    arg_target = 1;
    arg_text = 2;
    if ((argc >= 5) && (weechat_strcmp (argv[1], "-server") == 0))
    {
        ptr_server = irc_server_search (argv[2]);
        arg_target = 3;
        arg_text = 4;
    }

    IRC_COMMAND_CHECK_SERVER("notice", 1, 1);

    list_messages = irc_server_sendf (
        ptr_server,
        IRC_SERVER_SEND_OUTQ_PRIO_HIGH | IRC_SERVER_SEND_RETURN_LIST
        | IRC_SERVER_SEND_MULTILINE,
        NULL,
        "NOTICE %s :%s",
        argv[arg_target], argv_eol[arg_text]);
    if (list_messages)
    {
        /* display only if capability "echo-message" is NOT enabled */
        if (!weechat_hashtable_has_key (ptr_server->cap_list, "echo-message"))
        {
            list_size = weechat_arraylist_size (list_messages);
            for (i = 0; i < list_size; i++)
            {
                ptr_message = (const char *)weechat_arraylist_get (list_messages, i);
                irc_input_user_message_display (
                    ptr_server,
                    0,  /* date */
                    0,  /* date_usec */
                    NULL,  /* tags */
                    argv[arg_target],
                    NULL,  /* address */
                    "notice",
                    NULL,  /* ctcp_type */
                    ptr_message,
                    1);  /* decode_colors */
            }
        }
        weechat_arraylist_free (list_messages);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/notify": adds or removes notify.
 */

IRC_COMMAND_CALLBACK(notify)
{
    struct t_irc_notify *ptr_notify;
    int i, check_away;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;
    (void) argv_eol;

    /* display notify status for users on server */
    if (argc == 1)
    {
        irc_notify_display_list (ptr_server);
        return WEECHAT_RC_OK;
    }

    /* add notify */
    if (weechat_strcmp (argv[1], "add") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "add");

        check_away = 0;

        if (argc > 3)
        {
            ptr_server = irc_server_search (argv[3]);
            if (!ptr_server)
            {
                weechat_printf (
                    NULL,
                    _("%s%s: server \"%s\" not found"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, argv[3]);
                return WEECHAT_RC_OK;
            }
        }

        if (!ptr_server)
        {
            weechat_printf (
                NULL,
                _("%s%s: server must be specified because you are not on an "
                  "irc server or channel"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }

        if (argc > 4)
        {
            for (i = 4; i < argc; i++)
            {
                if (weechat_strcmp (argv[i], "-away") == 0)
                    check_away = 1;
            }
        }

        ptr_notify = irc_notify_search (ptr_server, argv[2]);
        if (ptr_notify)
        {
            weechat_printf (
                NULL,
                _("%s%s: notify already exists"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }

        if ((ptr_server->monitor > 0)
            && (ptr_server->notify_count >= ptr_server->monitor))
        {
            weechat_printf (
                ptr_server->buffer,
                _("%sMonitor list is full (%d)"),
                weechat_prefix ("error"), ptr_server->monitor);
            return WEECHAT_RC_OK;
        }

        ptr_notify = irc_notify_new (ptr_server, argv[2], check_away);
        if (ptr_notify)
        {
            irc_notify_set_server_option (ptr_server);
            weechat_printf (
                ptr_server->buffer,
                _("%s: notification added for %s%s%s"),
                IRC_PLUGIN_NAME,
                irc_nick_color_for_msg (ptr_server, 1, NULL, ptr_notify->nick),
                ptr_notify->nick,
                weechat_color ("reset"));
            irc_notify_check_now (ptr_notify);
        }
        else
        {
            weechat_printf (
                NULL,
                _("%s%s: error adding notification"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
        }

        return WEECHAT_RC_OK;
    }

    /* delete notify */
    if (weechat_strcmp (argv[1], "del") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "del");

        if (argc > 3)
        {
            ptr_server = irc_server_search (argv[3]);
            if (!ptr_server)
            {
                weechat_printf (
                    NULL,
                    _("%s%s: server \"%s\" not found"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, argv[3]);
                return WEECHAT_RC_OK;
            }
        }

        if (!ptr_server)
        {
            weechat_printf (
                NULL,
                _("%s%s: server must be specified because you are not on an "
                  "irc server or channel"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }

        if (weechat_strcmp (argv[2], "-all") == 0)
        {
            if (ptr_server->notify_list)
            {
                irc_notify_free_all (ptr_server);
                irc_notify_set_server_option (ptr_server);
                weechat_printf (
                    NULL,
                    _("%s: all notifications deleted"),
                    IRC_PLUGIN_NAME);
            }
            else
            {
                weechat_printf (
                    NULL,
                    _("%s: no notification in list"),
                    IRC_PLUGIN_NAME);
            }
        }
        else
        {
            ptr_notify = irc_notify_search (ptr_server, argv[2]);
            if (ptr_notify)
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s: notification deleted for %s%s%s"),
                    IRC_PLUGIN_NAME,
                    irc_nick_color_for_msg (ptr_server, 1, NULL,
                                            ptr_notify->nick),
                    ptr_notify->nick,
                    weechat_color ("reset"));
                irc_notify_free (ptr_server, ptr_notify, 1);
                irc_notify_set_server_option (ptr_server);
            }
            else
            {
                weechat_printf (
                    NULL,
                    _("%s%s: notification not found"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME);
                return WEECHAT_RC_OK;
            }
        }

        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Callback for command "/op": gives operator privileges to nickname(s).
 */

IRC_COMMAND_CALLBACK(op)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("op", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can only be executed in a channel buffer"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "op");
        return WEECHAT_RC_OK;
    }

    if (argc < 2)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s +o %s",
                          ptr_channel->name,
                          ptr_server->nick);
    }
    else
    {
        irc_command_mode_nicks (ptr_server, ptr_channel,
                                "op", "+", "o", argc, argv);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/oper": gets oper privileges.
 */

IRC_COMMAND_CALLBACK(oper)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("oper", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(3, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "OPER %s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Sends a part message for a channel.
 */

void
irc_command_part_channel (struct t_irc_server *server, const char *channel_name,
                          const char *part_message)
{
    const char *ptr_arg;
    char *msg;

    msg = NULL;
    ptr_arg = (part_message) ?
        part_message : IRC_SERVER_OPTION_STRING(server,
                                                IRC_SERVER_OPTION_MSG_PART);
    if (ptr_arg && ptr_arg[0])
    {
        msg = irc_server_get_default_msg (ptr_arg, server, channel_name, NULL);
        irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "PART %s :%s", channel_name, msg);
    }
    else
    {
        irc_server_sendf (server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "PART %s", channel_name);
    }

    free (msg);
}

/*
 * Callback for command "/part": leaves a channel or close a private buffer.
 */

IRC_COMMAND_CALLBACK(part)
{
    char *channel_name, *pos_args, **channels;
    int i, num_channels;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("part", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc > 1)
    {
        if (irc_channel_is_channel (ptr_server, argv[1]))
        {
            ptr_channel = irc_channel_search (ptr_server, argv[1]);
            channel_name = argv[1];
            pos_args = argv_eol[2];
        }
        else
        {
            if (!ptr_channel)
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can only be executed in a channel "
                      "or private buffer"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "part");
                return WEECHAT_RC_OK;
            }
            channel_name = ptr_channel->name;
            pos_args = argv_eol[1];
        }
    }
    else
    {
        if (!ptr_channel)
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can only be executed in a channel or "
                  "private buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "part");
            return WEECHAT_RC_OK;
        }
        channel_name = ptr_channel->name;
        pos_args = NULL;
    }

    if (ptr_channel && !ptr_channel->nicks)
    {
        if ((ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE)
            || weechat_config_boolean (irc_config_look_part_closes_buffer))
        {
            weechat_buffer_close (ptr_channel->buffer);
        }
        return WEECHAT_RC_OK;
    }

    irc_command_part_channel (ptr_server, channel_name, pos_args);

    if (IRC_SERVER_OPTION_BOOLEAN(ptr_server, IRC_SERVER_OPTION_AUTOJOIN_DYNAMIC))
    {
        channels = weechat_string_split (channel_name, ",", NULL,
                                         WEECHAT_STRING_SPLIT_STRIP_LEFT
                                         | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                         | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                         0, &num_channels);
        if (channels)
        {
            for (i = 0; i < num_channels; i++)
            {
                irc_join_remove_channel_from_autojoin (ptr_server, channels[i]);
            }
            weechat_string_free_split (channels);
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/ping": pings a server.
 */

IRC_COMMAND_CALLBACK(ping)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("ping", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "PING %s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/pong": sends pong answer to a daemon.
 */

IRC_COMMAND_CALLBACK(pong)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("pong", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "PONG %s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/query": starts private conversation with a nick.
 */

IRC_COMMAND_CALLBACK(query)
{
    char **nicks;
    int i, arg_nick, arg_text, num_nicks, noswitch;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    noswitch = 0;
    arg_nick = 1;
    arg_text = 2;

    for (i = 1; i < argc; i++)
    {
        if (weechat_strcmp (argv[i], "-server") == 0)
        {
            if (argc <= i + 1)
                WEECHAT_COMMAND_ERROR;
            ptr_server = irc_server_search (argv[i + 1]);
            if (!ptr_server)
                WEECHAT_COMMAND_ERROR;
            arg_nick = i + 2;
            arg_text = i + 3;
            i++;
        }
        else if (weechat_strcmp (argv[i], "-noswitch") == 0)
        {
            noswitch = 1;
            arg_nick = i + 1;
            arg_text = i + 2;
        }
        else
        {
            arg_nick = i;
            arg_text = i + 1;
            break;
        }
    }

    if (arg_nick >= argc)
        WEECHAT_COMMAND_ERROR;

    IRC_COMMAND_CHECK_SERVER("query", 1, 1);

    nicks = weechat_string_split (argv[arg_nick], ",", NULL,
                                  WEECHAT_STRING_SPLIT_STRIP_LEFT
                                  | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                                  | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                                  0, &num_nicks);
    if (!nicks)
        WEECHAT_COMMAND_ERROR;

    for (i = 0; i < num_nicks; i++)
    {
        /* ensure the name is not a channel name */
        if (irc_channel_is_channel (ptr_server, nicks[i]))
        {
            weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can not be executed with a "
                      "channel name (\"%s\")"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "query",
                    nicks[i]);
            continue;
        }

        /* create private buffer if not already opened */
        ptr_channel = irc_channel_search (ptr_server, nicks[i]);
        if (!ptr_channel)
        {
            ptr_channel = irc_channel_new (ptr_server,
                                           IRC_CHANNEL_TYPE_PRIVATE,
                                           nicks[i],
                                           (noswitch) ? 0 : 1,
                                           0);
            if (!ptr_channel)
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: cannot create new private buffer \"%s\""),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, nicks[i]);
            }
        }

        if (ptr_channel)
        {
            /* switch to buffer */
            if (!noswitch)
                weechat_buffer_set (ptr_channel->buffer, "display", "1");

            /* display text if given */
            if (argv_eol[arg_text])
            {
                /* display only if capability "echo-message" is NOT enabled */
                if (!weechat_hashtable_has_key (ptr_server->cap_list, "echo-message"))
                {
                    irc_input_user_message_display (
                        ptr_server,
                        0,  /* date */
                        0,  /* date_usec */
                        NULL,  /* tags */
                        ptr_channel->name,
                        NULL,  /* address */
                        "privmsg",
                        NULL,  /* ctcp_type */
                        argv_eol[arg_text],
                        1);  /* decode_colors */
                }
                irc_server_sendf (ptr_server,
                                  IRC_SERVER_SEND_OUTQ_PRIO_HIGH
                                  | IRC_SERVER_SEND_MULTILINE,
                                  NULL,
                                  "PRIVMSG %s :%s",
                                  nicks[i], argv_eol[arg_text]);
            }
        }
    }

    weechat_string_free_split (nicks);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/quiet": quiets nicks or hosts.
 */

IRC_COMMAND_CALLBACK(quiet)
{
    char *pos_channel;
    int pos_args;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("quiet", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if (argc > 1)
    {
        if (irc_channel_is_channel (ptr_server, argv[1]))
        {
            pos_channel = argv[1];
            pos_args = 2;
        }
        else
        {
            pos_channel = NULL;
            pos_args = 1;
        }

        /* channel not given, use default buffer */
        if (!pos_channel)
        {
            if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
                pos_channel = ptr_channel->name;
            else
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: \"%s\" command can only be executed in a channel "
                      "buffer"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "quiet");
                return WEECHAT_RC_OK;
            }
        }

        if (argv[pos_args])
        {
            irc_command_mode_masks (ptr_server, pos_channel,
                                    "quiet", "+", "q",
                                    argv, pos_args);
        }
        else
        {
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "MODE %s +q",
                              pos_channel);
        }
    }
    else
    {
        if (!ptr_channel)
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can only be executed in a channel "
                  "buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "quiet");
            return WEECHAT_RC_OK;
        }
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s +q", ptr_channel->name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/quote": sends raw data to server.
 */

IRC_COMMAND_CALLBACK(quote)
{
    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if ((argc >= 4) && (weechat_strcmp (argv[1], "-server") == 0))
    {
        ptr_server = irc_server_search (argv[2]);
        if (!ptr_server || (ptr_server->sock < 0))
            WEECHAT_COMMAND_ERROR;
        irc_server_sendf (ptr_server,
                          IRC_SERVER_SEND_OUTQ_PRIO_HIGH
                          | IRC_SERVER_SEND_MULTILINE,
                          NULL,
                          "%s", argv_eol[3]);
    }
    else
    {
        if (!ptr_server || (ptr_server->sock < 0))
            WEECHAT_COMMAND_ERROR;
        irc_server_sendf (ptr_server,
                          IRC_SERVER_SEND_OUTQ_PRIO_HIGH
                          | IRC_SERVER_SEND_MULTILINE,
                          NULL,
                          "%s", argv_eol[1]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Reconnects to a server.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
irc_command_reconnect_one_server (struct t_irc_server *server,
                                  int switch_address, int no_join)
{
    int switch_done;

    if (!server)
        return 0;

    switch_done = 0;

    if ((server->is_connected) || (server->hook_connect) || (server->hook_fd))
    {
        /* disconnect from server */
        irc_command_quit_server (server, NULL);
        irc_server_disconnect (server, switch_address, 0);
        switch_done = 1;
    }

    if (switch_address && !switch_done)
        irc_server_switch_address (server, 0);

    server->disable_autojoin = no_join;

    if (irc_server_connect (server))
    {
        server->reconnect_delay = 0;
        server->reconnect_start = 0;
    }

    /* reconnect OK */
    return 1;
}

/*
 * Callback for command "/reconnect": reconnects to server(s).
 */

IRC_COMMAND_CALLBACK(reconnect)
{
    int i, nb_reconnect, reconnect_ok, all_servers, switch_address, no_join;

    IRC_BUFFER_GET_SERVER(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    reconnect_ok = 1;

    all_servers = 0;
    switch_address = 0;
    no_join = 0;
    for (i = 1; i < argc; i++)
    {
        if (weechat_strcmp (argv[i], "-all") == 0)
            all_servers = 1;
        else if (weechat_strcmp (argv[i], "-switch") == 0)
            switch_address = 1;
        else if (weechat_strcmp (argv[i], "-nojoin") == 0)
            no_join = 1;
    }

    if (all_servers)
    {
        for (ptr_server = irc_servers; ptr_server;
             ptr_server = ptr_server->next_server)
        {
            if (ptr_server->buffer)
            {
                if (!irc_command_reconnect_one_server (ptr_server,
                                                       switch_address,
                                                       no_join))
                {
                    reconnect_ok = 0;
                }
            }
        }
    }
    else
    {
        nb_reconnect = 0;
        for (i = 1; i < argc; i++)
        {
            if (argv[i][0] != '-')
            {
                nb_reconnect++;
                ptr_server = irc_server_search (argv[i]);
                if (ptr_server)
                {
                    if (ptr_server->buffer)
                    {
                        if (!irc_command_reconnect_one_server (ptr_server,
                                                               switch_address,
                                                               no_join))
                        {
                            reconnect_ok = 0;
                        }
                    }
                }
                else
                {
                    weechat_printf (
                        NULL,
                        _("%s%s: server \"%s\" not found"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME, argv[i]);
                    reconnect_ok = 0;
                }
            }
        }
        if (nb_reconnect == 0)
        {
            reconnect_ok = irc_command_reconnect_one_server (ptr_server,
                                                             switch_address,
                                                             no_join);
        }
    }

    return (reconnect_ok) ? WEECHAT_RC_OK : WEECHAT_RC_ERROR;
}

/*
 * Callback for command "/rehash": tells the server to reload its config file.
 */

IRC_COMMAND_CALLBACK(rehash)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("rehash", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "REHASH %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "REHASH");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/remove": remove a user from a channel.
 */

IRC_COMMAND_CALLBACK(remove)
{
    const char *ptr_channel_name;
    char *msg_vars_replaced;
    int index_nick;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("remove", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    ptr_channel_name = (ptr_channel) ? ptr_channel->name : NULL;
    index_nick = 1;

    if (irc_channel_is_channel (ptr_server, argv[1]))
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "");
        ptr_channel_name = argv[1];
        index_nick = 2;
    }

    if (!ptr_channel_name)
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can only be executed in a channel buffer"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "remove");
        return WEECHAT_RC_OK;
    }

    if (argc > index_nick + 1)
    {
        msg_vars_replaced = irc_message_replace_vars (ptr_server,
                                                      ptr_channel_name,
                                                      argv_eol[index_nick + 1]);
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "REMOVE %s %s :%s",
                          ptr_channel_name,
                          argv[index_nick],
                          (msg_vars_replaced) ? msg_vars_replaced : argv_eol[index_nick + 1]);
        free (msg_vars_replaced);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "REMOVE %s %s",
                          ptr_channel_name,
                          argv[index_nick]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/restart": tells the server to restart itself.
 */

IRC_COMMAND_CALLBACK(restart)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("restart", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "RESTART %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "RESTART");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/rules": requests the server rules
 */

IRC_COMMAND_CALLBACK(rules)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("rules", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argc;
    (void) argv;
    (void) argv_eol;

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "RULES");

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/sajoin": forces a user to join channel(s).
 */

IRC_COMMAND_CALLBACK(sajoin)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("sajoin", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(3, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "SAJOIN %s %s", argv[1], argv_eol[2]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/samode": changes mode on channel, without having
 * operator status.
 */

IRC_COMMAND_CALLBACK(samode)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("samode", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    if (argc > 1)
    {
        if ((argv[1][0] == '+') || (argv[1][0] == '-'))
        {
            /* channel not specified, check we are on channel and use it */
            if (!ptr_channel)
            {
                weechat_printf (
                    ptr_server->buffer,
                    _("%s%s: you must specify channel for \"%s\" command if "
                      "you're not in a channel"),
                    weechat_prefix ("error"), IRC_PLUGIN_NAME, "samode");
                return WEECHAT_RC_OK;
            }
            irc_command_mode_server (ptr_server, "SAMODE", ptr_channel,
                                     argv_eol[1],
                                     IRC_SERVER_SEND_OUTQ_PRIO_HIGH);
        }
        else
        {
            /* user gives channel, use arguments as-is */
            irc_command_mode_server (ptr_server, "SAMODE", NULL, argv_eol[1],
                                     IRC_SERVER_SEND_OUTQ_PRIO_HIGH);
        }
    }
    else
    {
        if (ptr_channel)
        {
            irc_command_mode_server (ptr_server, "SAMODE", ptr_channel, NULL,
                                     IRC_SERVER_SEND_OUTQ_PRIO_HIGH);
        }
        else
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: you must specify channel for \"%s\" command if "
                  "you're not in a channel"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "samode");
            return WEECHAT_RC_OK;
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/sanick": forces a user to use another nick.
 */

IRC_COMMAND_CALLBACK(sanick)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("sanick", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(3, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "SANICK %s %s", argv[1], argv_eol[2]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/sapart": forces a user to leave channel(s).
 */

IRC_COMMAND_CALLBACK(sapart)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("sapart", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(3, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "SAPART %s %s", argv[1], argv_eol[2]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/saquit": forces a user to quit server with a reason.
 */

IRC_COMMAND_CALLBACK(saquit)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("saquit", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(3, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "SAQUIT %s :%s", argv[1], argv_eol[2]);

    return WEECHAT_RC_OK;
}

/*
 * Displays server options.
 */

void
irc_command_display_server (struct t_irc_server *server, int with_detail)
{
    char *cmd_pwd_hidden, str_nick[1024];
    int num_channels, num_pv;

    str_nick[0] = '\0';
    if (server->nick)
    {
        snprintf (str_nick, sizeof (str_nick),
                  ", %s %s",
                  _("nick:"),
                  server->nick);
    }

    if (with_detail)
    {
        weechat_printf (NULL, "");
        weechat_printf (NULL, _("Server: %s%s %s[%s%s%s]%s%s%s%s"),
                        IRC_COLOR_CHAT_SERVER,
                        server->name,
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_RESET,
                        (server->is_connected) ?
                        _("connected") : _("not connected"),
                        IRC_COLOR_CHAT_DELIMITERS,
                        IRC_COLOR_RESET,
                        str_nick,
                        /* TRANSLATORS: "temporary IRC server" */
                        (server->temp_server) ? _(" (temporary)") : "",
                        /* TRANSLATORS: "fake IRC server" */
                        (server->fake_server) ? _(" (fake)") : "");
        /* addresses */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_ADDRESSES]))
            weechat_printf (NULL, "  addresses. . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_ADDRESSES));
        else
            weechat_printf (NULL, "  addresses. . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_ADDRESSES]));
        /* proxy */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_PROXY]))
            weechat_printf (NULL, "  proxy. . . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_PROXY));
        else
            weechat_printf (NULL, "  proxy. . . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_PROXY]));
        /* ipv6 */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_IPV6]))
            weechat_printf (NULL, "  ipv6 . . . . . . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_IPV6)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  ipv6 . . . . . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            (weechat_config_boolean (server->options[IRC_SERVER_OPTION_IPV6])) ?
                            _("on") : _("off"));
        /* tls */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_TLS]))
            weechat_printf (NULL, "  tls. . . . . . . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_TLS)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  tls. . . . . . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            (weechat_config_boolean (server->options[IRC_SERVER_OPTION_TLS])) ?
                            _("on") : _("off"));
        /* tls_cert */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_TLS_CERT]))
            weechat_printf (NULL, "  tls_cert . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_TLS_CERT));
        else
            weechat_printf (NULL, "  tls_cert . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_TLS_CERT]));
        /* tls_password */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_TLS_PASSWORD]))
            weechat_printf (NULL, "  tls_password . . . . :   %s",
                            _("(hidden)"));
        else
            weechat_printf (NULL, "  tls_password . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            _("(hidden)"));
        /* tls_priorities */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_TLS_PRIORITIES]))
            weechat_printf (NULL, "  tls_priorities . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_TLS_PRIORITIES));
        else
            weechat_printf (NULL, "  tls_priorities . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_TLS_PRIORITIES]));
        /* tls_dhkey_size */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_TLS_DHKEY_SIZE]))
            weechat_printf (NULL, "  tls_dhkey_size . . . :   (%d)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_TLS_DHKEY_SIZE));
        else
            weechat_printf (NULL, "  tls_dhkey_size . . . : %s%d",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_TLS_DHKEY_SIZE]));
        /* tls_fingerprint */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT]))
            weechat_printf (NULL, "  tls_fingerprint. . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_TLS_FINGERPRINT));
        else
            weechat_printf (NULL, "  tls_fingerprint. . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_TLS_FINGERPRINT]));
        /* tls_verify */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_TLS_VERIFY]))
            weechat_printf (NULL, "  tls_verify . . . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_TLS_VERIFY)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  tls_verify . . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            (weechat_config_boolean (server->options[IRC_SERVER_OPTION_TLS_VERIFY])) ?
                            _("on") : _("off"));
        /* password */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_PASSWORD]))
            weechat_printf (NULL, "  password . . . . . . :   %s",
                            _("(hidden)"));
        else
            weechat_printf (NULL, "  password . . . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            _("(hidden)"));
        /* client capabilities */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_CAPABILITIES]))
            weechat_printf (NULL, "  capabilities . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_CAPABILITIES));
        else
            weechat_printf (NULL, "  capabilities . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_CAPABILITIES]));
        /* sasl_mechanism */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_SASL_MECHANISM]))
            weechat_printf (NULL, "  sasl_mechanism . . . :   ('%s')",
                            irc_sasl_mechanism_string[IRC_SERVER_OPTION_ENUM(server, IRC_SERVER_OPTION_SASL_MECHANISM)]);
        else
            weechat_printf (NULL, "  sasl_mechanism . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            irc_sasl_mechanism_string[weechat_config_enum (server->options[IRC_SERVER_OPTION_SASL_MECHANISM])]);
        /* sasl_username */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_SASL_USERNAME]))
            weechat_printf (NULL, "  sasl_username. . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SASL_USERNAME));
        else
            weechat_printf (NULL, "  sasl_username. . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_SASL_USERNAME]));
        /* sasl_password */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_SASL_PASSWORD]))
            weechat_printf (NULL, "  sasl_password. . . . :   %s",
                            _("(hidden)"));
        else
            weechat_printf (NULL, "  sasl_password. . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            _("(hidden)"));
        /* sasl_key */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_SASL_KEY]))
            weechat_printf (NULL, "  sasl_key. .  . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_SASL_KEY));
        else
            weechat_printf (NULL, "  sasl_key. .  . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_SASL_KEY]));
        /* sasl_timeout */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_SASL_TIMEOUT]))
            weechat_printf (NULL, "  sasl_timeout . . . . :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SASL_TIMEOUT),
                            NG_("second", "seconds", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SASL_TIMEOUT)));
        else
            weechat_printf (NULL, "  sasl_timeout . . . . : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_SASL_TIMEOUT]),
                            NG_("second", "seconds", weechat_config_integer (server->options[IRC_SERVER_OPTION_SASL_TIMEOUT])));
        /* sasl_fail */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_SASL_FAIL]))
            weechat_printf (NULL, "  sasl_fail. . . . . . :   ('%s')",
                            irc_server_sasl_fail_string[IRC_SERVER_OPTION_ENUM(server, IRC_SERVER_OPTION_SASL_FAIL)]);
        else
            weechat_printf (NULL, "  sasl_fail. . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            irc_server_sasl_fail_string[weechat_config_enum (server->options[IRC_SERVER_OPTION_SASL_FAIL])]);
        /* autoconnect */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTOCONNECT]))
            weechat_printf (NULL, "  autoconnect. . . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTOCONNECT)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autoconnect. . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            (weechat_config_boolean (server->options[IRC_SERVER_OPTION_AUTOCONNECT])) ?
                            _("on") : _("off"));
        /* autoreconnect */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTORECONNECT]))
            weechat_printf (NULL, "  autoreconnect. . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTORECONNECT)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autoreconnect. . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            (weechat_config_boolean (server->options[IRC_SERVER_OPTION_AUTORECONNECT])) ?
                            _("on") : _("off"));
        /* autoreconnect_delay */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTORECONNECT_DELAY]))
            weechat_printf (NULL, "  autoreconnect_delay. :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTORECONNECT_DELAY),
                            NG_("second", "seconds", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTORECONNECT_DELAY)));
        else
            weechat_printf (NULL, "  autoreconnect_delay. : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_AUTORECONNECT_DELAY]),
                            NG_("second", "seconds", weechat_config_integer (server->options[IRC_SERVER_OPTION_AUTORECONNECT_DELAY])));
        /* nicks */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_NICKS]))
            weechat_printf (NULL, "  nicks. . . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_NICKS));
        else
            weechat_printf (NULL, "  nicks. . . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_NICKS]));
        /* nicks_alternate */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_NICKS_ALTERNATE]))
            weechat_printf (NULL, "  nicks_alternate. . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_NICKS_ALTERNATE)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  nicks_alternate. . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            (weechat_config_boolean (server->options[IRC_SERVER_OPTION_NICKS_ALTERNATE])) ?
                            _("on") : _("off"));
        /* username */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_USERNAME]))
            weechat_printf (NULL, "  username . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_USERNAME));
        else
            weechat_printf (NULL, "  username . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_USERNAME]));
        /* realname */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_REALNAME]))
            weechat_printf (NULL, "  realname . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_REALNAME));
        else
            weechat_printf (NULL, "  realname . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_REALNAME]));
        /* local_hostname */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_LOCAL_HOSTNAME]))
            weechat_printf (NULL, "  local_hostname . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_LOCAL_HOSTNAME));
        else
            weechat_printf (NULL, "  local_hostname . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_LOCAL_HOSTNAME]));
        /* usermode */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_USERMODE]))
            weechat_printf (NULL, "  usermode . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_USERMODE));
        else
            weechat_printf (NULL, "  usermode . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_USERMODE]));
        /* command_delay */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_COMMAND_DELAY]))
            weechat_printf (NULL, "  command_delay. . . . :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_COMMAND_DELAY),
                            NG_("second", "seconds", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_COMMAND_DELAY)));
        else
            weechat_printf (NULL, "  command_delay. . . . : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_COMMAND_DELAY]),
                            NG_("second", "seconds", weechat_config_integer (server->options[IRC_SERVER_OPTION_COMMAND_DELAY])));
        /* command */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_COMMAND]))
        {
            cmd_pwd_hidden = weechat_hook_modifier_exec ("irc_command_auth",
                                                         server->name,
                                                         IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_COMMAND));
            weechat_printf (NULL, "  command. . . . . . . :   ('%s')",
                            (cmd_pwd_hidden) ? cmd_pwd_hidden : IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_COMMAND));
            free (cmd_pwd_hidden);
        }
        else
        {
            cmd_pwd_hidden = weechat_hook_modifier_exec ("irc_command_auth",
                                                         server->name,
                                                         weechat_config_string (server->options[IRC_SERVER_OPTION_COMMAND]));
            weechat_printf (NULL, "  command. . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            (cmd_pwd_hidden) ? cmd_pwd_hidden : weechat_config_string (server->options[IRC_SERVER_OPTION_COMMAND]));
            free (cmd_pwd_hidden);
        }
        /* autojoin_delay */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTOJOIN_DELAY]))
            weechat_printf (NULL, "  autojoin_delay . . . :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTOJOIN_DELAY),
                            NG_("second", "seconds", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTOJOIN_DELAY)));
        else
            weechat_printf (NULL, "  autojoin_delay . . . : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_AUTOJOIN_DELAY]),
                            NG_("second", "seconds", weechat_config_integer (server->options[IRC_SERVER_OPTION_AUTOJOIN_DELAY])));
        /* autojoin */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTOJOIN]))
            weechat_printf (NULL, "  autojoin . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_AUTOJOIN));
        else
            weechat_printf (NULL, "  autojoin . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_AUTOJOIN]));

        /* autojoin_dynamic */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTOJOIN_DYNAMIC]))
            weechat_printf (NULL, "  autojoin_dynamic . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTOJOIN_DYNAMIC)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autojoin_dynamic . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            (weechat_config_boolean (server->options[IRC_SERVER_OPTION_AUTOJOIN_DYNAMIC])) ?
                            _("on") : _("off"));

        /* autorejoin */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTOREJOIN]))
            weechat_printf (NULL, "  autorejoin . . . . . :   (%s)",
                            (IRC_SERVER_OPTION_BOOLEAN(server, IRC_SERVER_OPTION_AUTOREJOIN)) ?
                            _("on") : _("off"));
        else
            weechat_printf (NULL, "  autorejoin . . . . . : %s%s",
                            IRC_COLOR_CHAT_VALUE,
                            (weechat_config_boolean (server->options[IRC_SERVER_OPTION_AUTOREJOIN])) ?
                            _("on") : _("off"));
        /* autorejoin_delay */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AUTOREJOIN_DELAY]))
            weechat_printf (NULL, "  autorejoin_delay . . :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTOREJOIN_DELAY),
                            NG_("second", "seconds", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AUTOREJOIN_DELAY)));
        else
            weechat_printf (NULL, "  autorejoin_delay . . : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_AUTOREJOIN_DELAY]),
                            NG_("second", "seconds", weechat_config_integer (server->options[IRC_SERVER_OPTION_AUTOREJOIN_DELAY])));
        /* connection_timeout */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_CONNECTION_TIMEOUT]))
            weechat_printf (NULL, "  connection_timeout . :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_CONNECTION_TIMEOUT),
                            NG_("second", "seconds", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_CONNECTION_TIMEOUT)));
        else
            weechat_printf (NULL, "  connection_timeout . : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_CONNECTION_TIMEOUT]),
                            NG_("second", "seconds", weechat_config_integer (server->options[IRC_SERVER_OPTION_CONNECTION_TIMEOUT])));
        /* anti_flood */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_ANTI_FLOOD]))
            weechat_printf (NULL, "  anti_flood . . . . . :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_ANTI_FLOOD),
                            NG_("second", "seconds", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_ANTI_FLOOD)));
        else
            weechat_printf (NULL, "  anti_flood . . . . . : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_ANTI_FLOOD]),
                            NG_("second", "seconds", weechat_config_integer (server->options[IRC_SERVER_OPTION_ANTI_FLOOD])));
        /* away_check */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AWAY_CHECK]))
            weechat_printf (NULL, "  away_check . . . . . :   (%d %s)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK),
                            NG_("minute", "minutes", IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK)));
        else
            weechat_printf (NULL, "  away_check . . . . . : %s%d %s",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_AWAY_CHECK]),
                            NG_("minute", "minutes", weechat_config_integer (server->options[IRC_SERVER_OPTION_AWAY_CHECK])));
        /* away_check_max_nicks */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS]))
            weechat_printf (NULL, "  away_check_max_nicks :   (%d)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS));
        else
            weechat_printf (NULL, "  away_check_max_nicks : %s%d",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_AWAY_CHECK_MAX_NICKS]));
        /* msg_kick */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_MSG_KICK]))
            weechat_printf (NULL, "  msg_kick . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_MSG_KICK));
        else
            weechat_printf (NULL, "  msg_kick . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_MSG_KICK]));
        /* msg_part */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_MSG_PART]))
            weechat_printf (NULL, "  msg_part . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_MSG_PART));
        else
            weechat_printf (NULL, "  msg_part . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_MSG_PART]));
        /* msg_quit */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_MSG_QUIT]))
            weechat_printf (NULL, "  msg_quit . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_MSG_QUIT));
        else
            weechat_printf (NULL, "  msg_quit . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_MSG_QUIT]));
        /* notify */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_NOTIFY]))
            weechat_printf (NULL, "  notify . . . . . . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_NOTIFY));
        else
            weechat_printf (NULL, "  notify . . . . . . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_NOTIFY]));
        /* split_msg_max_length */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_SPLIT_MSG_MAX_LENGTH]))
            weechat_printf (NULL, "  split_msg_max_length :   (%d)",
                            IRC_SERVER_OPTION_INTEGER(server, IRC_SERVER_OPTION_SPLIT_MSG_MAX_LENGTH));
        else
            weechat_printf (NULL, "  split_msg_max_length : %s%d",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_integer (server->options[IRC_SERVER_OPTION_SPLIT_MSG_MAX_LENGTH]));
        /* charset_message */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_CHARSET_MESSAGE]))
            weechat_printf (NULL, "  charset_message. . . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_CHARSET_MESSAGE));
        else
            weechat_printf (NULL, "  charset_message. . . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_CHARSET_MESSAGE]));
        /* default_chantypes */
        if (weechat_config_option_is_null (server->options[IRC_SERVER_OPTION_DEFAULT_CHANTYPES]))
            weechat_printf (NULL, "  default_chantypes. . :   ('%s')",
                            IRC_SERVER_OPTION_STRING(server, IRC_SERVER_OPTION_DEFAULT_CHANTYPES));
        else
            weechat_printf (NULL, "  default_chantypes. . : %s'%s'",
                            IRC_COLOR_CHAT_VALUE,
                            weechat_config_string (server->options[IRC_SERVER_OPTION_DEFAULT_CHANTYPES]));
    }
    else
    {
        if (server->is_connected)
        {
            num_channels = irc_server_get_channel_count (server);
            num_pv = irc_server_get_pv_count (server);
            weechat_printf (
                NULL,
                " %s %s%s %s[%s%s%s]%s%s%s%s, %d %s, %d pv",
                (server->is_connected) ? "*" : " ",
                IRC_COLOR_CHAT_SERVER,
                server->name,
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_RESET,
                (server->is_connected) ? _("connected") : _("not connected"),
                IRC_COLOR_CHAT_DELIMITERS,
                IRC_COLOR_RESET,
                str_nick,
                /* TRANSLATORS: "temporary IRC server" */
                (server->temp_server) ? _(" (temporary)") : "",
                /* TRANSLATORS: "fake IRC server" */
                (server->fake_server) ? _(" (fake)") : "",
                num_channels,
                NG_("channel", "channels", num_channels),
                num_pv);
        }
        else
        {
            weechat_printf (
                NULL,
                "   %s%s%s%s%s",
                IRC_COLOR_CHAT_SERVER,
                server->name,
                IRC_COLOR_RESET,
                /* TRANSLATORS: "temporary IRC server" */
                (server->temp_server) ? _(" (temporary)") : "",
                /* TRANSLATORS: "fake IRC server" */
                (server->fake_server) ? _(" (fake)") : "");
        }
    }
}

/*
 * Callback for command "/server": manages IRC servers.
 */

IRC_COMMAND_CALLBACK(server)
{
    int i, detailed_list, one_server_found, length, count, refresh;
    struct t_irc_server *ptr_server2, *server_found, *new_server;
    char *server_name, *msg_no_quotes, *message, *description;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) buffer;

    if ((argc == 1)
        || (weechat_strcmp (argv[1], "list") == 0)
        || (weechat_strcmp (argv[1], "listfull") == 0))
    {
        /* list servers */
        server_name = NULL;
        detailed_list = 0;
        for (i = 1; i < argc; i++)
        {
            if (weechat_strcmp (argv[i], "list") == 0)
                continue;
            if (weechat_strcmp (argv[i], "listfull") == 0)
            {
                detailed_list = 1;
                continue;
            }
            if (!server_name)
                server_name = argv[i];
        }
        if (!server_name)
        {
            if (irc_servers)
            {
                weechat_printf (NULL, "");
                weechat_printf (NULL, _("All servers:"));
                for (ptr_server2 = irc_servers; ptr_server2;
                     ptr_server2 = ptr_server2->next_server)
                {
                    irc_command_display_server (ptr_server2, detailed_list);
                }
            }
            else
                weechat_printf (NULL, _("No server"));
        }
        else
        {
            one_server_found = 0;
            for (ptr_server2 = irc_servers; ptr_server2;
                 ptr_server2 = ptr_server2->next_server)
            {
                if (strstr (ptr_server2->name, server_name))
                {
                    if (!one_server_found)
                    {
                        weechat_printf (NULL, "");
                        weechat_printf (NULL,
                                        _("Servers with \"%s\":"),
                                        server_name);
                    }
                    one_server_found = 1;
                    irc_command_display_server (ptr_server2, detailed_list);
                }
            }
            if (!one_server_found)
                weechat_printf (NULL,
                                _("No server found with \"%s\""),
                                server_name);
        }
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "add") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "add");

        ptr_server2 = irc_server_search (argv[2]);
        if (ptr_server2)
        {
            weechat_printf (
                NULL,
                _("%s%s: server \"%s\" already exists, can't add it!"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, ptr_server2->name);
            return WEECHAT_RC_OK;
        }

        new_server = irc_server_alloc (argv[2]);
        if (!new_server)
        {
            weechat_printf (
                NULL,
                _("%s%s: unable to add server"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME);
            return WEECHAT_RC_OK;
        }

        weechat_config_option_set (
            new_server->options[IRC_SERVER_OPTION_ADDRESSES], argv[3], 1);
        irc_server_apply_command_line_options (new_server, argc, argv);

        description = irc_server_get_short_description (new_server);

        weechat_printf (
            NULL,
            _("%s: server added: %s%s%s -> %s"),
            IRC_PLUGIN_NAME,
            IRC_COLOR_CHAT_SERVER,
            new_server->name,
            IRC_COLOR_RESET,
            description);

        free (description);

        /* do not connect to server after adding it */
        /*
        if (IRC_SERVER_OPTION_BOOLEAN(new_server, IRC_SERVER_OPTION_AUTOCONNECT))
            irc_server_connect (new_server);
        */

        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "copy") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "copy");

        /* look for server by name */
        server_found = irc_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (
                NULL,
                _("%s%s: server \"%s\" not found for \"%s\" command"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                argv[2], "server copy");
            return WEECHAT_RC_OK;
        }

        /* check if target name already exists */
        ptr_server2 = irc_server_search (argv[3]);
        if (ptr_server2)
        {
            weechat_printf (
                NULL,
                _("%s%s: server \"%s\" already exists for \"%s\" command"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                ptr_server2->name, "server copy");
            return WEECHAT_RC_OK;
        }

        /* copy server */
        new_server = irc_server_copy (server_found, argv[3]);
        if (new_server)
        {
            weechat_printf (
                NULL,
                _("%s: server %s%s%s has been copied to %s%s%s"),
                IRC_PLUGIN_NAME,
                IRC_COLOR_CHAT_SERVER,
                argv[2],
                IRC_COLOR_RESET,
                IRC_COLOR_CHAT_SERVER,
                argv[3],
                IRC_COLOR_RESET);
            return WEECHAT_RC_OK;
        }

        WEECHAT_COMMAND_ERROR;
    }

    if (weechat_strcmp (argv[1], "rename") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(4, "rename");

        /* look for server by name */
        server_found = irc_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (
                NULL,
                _("%s%s: server \"%s\" not found for \"%s\" command"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                argv[2], "server rename");
            return WEECHAT_RC_OK;
        }

        /* check if target name already exists */
        ptr_server2 = irc_server_search (argv[3]);
        if (ptr_server2)
        {
            weechat_printf (
                NULL,
                _("%s%s: server \"%s\" already exists for \"%s\" command"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                ptr_server2->name, "server rename");
            return WEECHAT_RC_OK;
        }

        /* rename server */
        if (irc_server_rename (server_found, argv[3]))
        {
            weechat_printf (
                NULL,
                _("%s: server %s%s%s has been renamed to %s%s%s"),
                IRC_PLUGIN_NAME,
                IRC_COLOR_CHAT_SERVER,
                argv[2],
                IRC_COLOR_RESET,
                IRC_COLOR_CHAT_SERVER,
                argv[3],
                IRC_COLOR_RESET);
            return WEECHAT_RC_OK;
        }

        WEECHAT_COMMAND_ERROR;
    }

    if (weechat_strcmp (argv[1], "reorder") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "reorder");

        count = irc_server_reorder (((const char **)argv) + 2, argc - 2);
        weechat_printf (NULL,
                        NG_("%d server moved", "%d servers moved", count),
                        count);

        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "open") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "open");

        if (weechat_strcmp (argv[2], "-all") == 0)
        {
            for (ptr_server2 = irc_servers; ptr_server2;
                 ptr_server2 = ptr_server2->next_server)
            {
                if (!ptr_server2->buffer)
                {
                    if (irc_server_create_buffer (ptr_server2))
                    {
                        weechat_buffer_set (ptr_server2->buffer,
                                            "display", "auto");
                    }
                }
            }
        }
        else
        {
            for (i = 2; i < argc; i++)
            {
                ptr_server2 = irc_server_search (argv[i]);
                if (ptr_server2)
                {
                    if (!ptr_server2->buffer)
                    {
                        if (irc_server_create_buffer (ptr_server2))
                        {
                            weechat_buffer_set (ptr_server2->buffer,
                                                "display", "auto");
                        }
                    }
                }
                else
                {
                    weechat_printf (
                        NULL,
                        _("%s%s: server \"%s\" not found for \"%s\" command"),
                        weechat_prefix ("error"), IRC_PLUGIN_NAME,
                        argv[i], "server open");
                }
            }
        }

        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "keep") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "keep");

        /* look for server by name */
        server_found = irc_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (
                NULL,
                _("%s%s: server \"%s\" not found for \"%s\" command"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                argv[2], "server keep");
            return WEECHAT_RC_OK;
        }

        /* check that is it temporary server */
        if (!server_found->temp_server)
        {
            weechat_printf (
                NULL,
                _("%s%s: server \"%s\" is not a temporary server"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                argv[2], "server keep");
            return WEECHAT_RC_OK;
        }

        /* remove temporary flag on server */
        server_found->temp_server = 0;

        weechat_printf (
            NULL,
            _("%s: server %s%s%s is not temporary any more"),
            IRC_PLUGIN_NAME,
            IRC_COLOR_CHAT_SERVER,
            argv[2],
            IRC_COLOR_RESET);

        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "del") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "del");

        /* look for server by name */
        server_found = irc_server_search (argv[2]);
        if (!server_found)
        {
            weechat_printf (
                NULL,
                _("%s%s: server \"%s\" not found for \"%s\" command"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME,
                argv[2], "server del");
            return WEECHAT_RC_OK;
        }
        if (server_found->is_connected)
        {
            weechat_printf (
                NULL,
                _("%s%s: you can not delete server \"%s\" because you are "
                  "connected to. Try \"/disconnect %s\" before."),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, argv[2], argv[2]);
            return WEECHAT_RC_OK;
        }

        server_name = strdup (server_found->name);
        irc_server_free (server_found);
        weechat_printf (
            NULL,
            _("%s: server %s%s%s has been deleted"),
            IRC_PLUGIN_NAME,
            IRC_COLOR_CHAT_SERVER,
            (server_name) ? server_name : "???",
            IRC_COLOR_RESET);
        free (server_name);

        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "deloutq") == 0)
    {
        for (ptr_server2 = irc_servers; ptr_server2;
             ptr_server2 = ptr_server2->next_server)
        {
            for (i = 0; i < IRC_SERVER_NUM_OUTQUEUES_PRIO; i++)
            {
                irc_server_outqueue_free_all (ptr_server2, i);
            }
        }
        weechat_printf (
            NULL,
            _("%s: messages outqueue DELETED for all servers. Some messages "
              "from you or WeeChat may have been lost!"),
            IRC_PLUGIN_NAME);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "raw") == 0)
    {
        refresh = irc_raw_buffer && (argc > 2);
        if (argc > 2)
            irc_raw_set_filter (argv_eol[2]);
        irc_raw_open (1);
        if (refresh)
            irc_raw_refresh (1);
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "jump") == 0)
    {
        if (ptr_server && ptr_server->buffer)
            weechat_buffer_set (ptr_server->buffer, "display", "1");
        return WEECHAT_RC_OK;
    }

    if (weechat_strcmp (argv[1], "fakerecv") == 0)
    {
        WEECHAT_COMMAND_MIN_ARGS(3, "fakerecv");
        IRC_COMMAND_CHECK_SERVER("server fakerecv", 0, 1);
        msg_no_quotes = weechat_string_remove_quotes (argv_eol[2], "\"");
        length = strlen (msg_no_quotes);
        if (length > 0)
        {
            /* allocate length + 2 (CR-LF) + 1 (final '\0') */
            message = malloc (length + 2 + 1);
            if (message)
            {
                strcpy (message, msg_no_quotes);
                strcat (message, "\r\n");
                irc_server_msgq_add_buffer (ptr_server, message);
                irc_server_msgq_flush ();
                free (message);
            }
        }
        return WEECHAT_RC_OK;
    }

    WEECHAT_COMMAND_ERROR;
}

/*
 * Callback for command "/service": registers a new service.
 */

IRC_COMMAND_CALLBACK(service)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("service", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "SERVICE %s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/servlist": lists services currently connected to the
 * network.
 */

IRC_COMMAND_CALLBACK(servlist)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("servlist", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "SERVLIST %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "SERVLIST");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/squery": delivers a message to a service.
 */

IRC_COMMAND_CALLBACK(squery)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("squery", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (argc > 2)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "SQUERY %s :%s", argv[1], argv_eol[2]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "SQUERY %s", argv_eol[1]);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/setname": set real name.
 */

IRC_COMMAND_CALLBACK(setname)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("setname", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "SETNAME :%s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/squit": disconnects server links.
 */

IRC_COMMAND_CALLBACK(squit)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("squit", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_IMMEDIATE, NULL,
                      "SQUIT %s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/stats": queries statistics about server.
 */

IRC_COMMAND_CALLBACK(stats)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("stats", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "STATS %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "STATS");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/summon": gives users who are on a host running an IRC
 * server a message asking them to please join IRC.
 */

IRC_COMMAND_CALLBACK(summon)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("summon", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "SUMMON %s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/time": queries local time from server.
 */

IRC_COMMAND_CALLBACK(time)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("time", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "TIME %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "TIME");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/topic": gets/sets topic for a channel.
 */

IRC_COMMAND_CALLBACK(topic)
{
    char *channel_name, *new_topic, *new_topic_color;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("topic", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    channel_name = NULL;
    new_topic = NULL;

    if (argc > 1)
    {
        if (irc_channel_is_channel (ptr_server, argv[1]))
        {
            channel_name = argv[1];
            new_topic = argv_eol[2];
        }
        else
            new_topic = argv_eol[1];
    }

    /* look for current channel if not specified */
    if (!channel_name)
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
        {
            channel_name = ptr_channel->name;
        }
        else
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can only be executed in a channel "
                  "buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "topic");
            return WEECHAT_RC_OK;
        }
    }

    if (new_topic)
    {
        if (weechat_strcmp (new_topic, "-delete") == 0)
        {
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "TOPIC %s :", channel_name);
        }
        else
        {
            new_topic_color = irc_color_encode (
                new_topic,
                weechat_config_boolean (irc_config_network_colors_send));
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "TOPIC %s :%s",
                              channel_name,
                              (new_topic_color) ? new_topic_color : new_topic);
            free (new_topic_color);
        }
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "TOPIC %s", channel_name);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/trace": finds the route to specific server.
 */

IRC_COMMAND_CALLBACK(trace)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("trace", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "TRACE %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "TRACE");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/unban": unbans nicks or hosts.
 */

IRC_COMMAND_CALLBACK(unban)
{
    char *pos_channel, **masks;
    int pos_args;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("unban", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (irc_channel_is_channel (ptr_server, argv[1]))
    {
        pos_channel = argv[1];
        pos_args = 2;
    }
    else
    {
        pos_channel = NULL;
        pos_args = 1;
    }

    /* channel not given, use default buffer */
    if (!pos_channel)
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            pos_channel = ptr_channel->name;
        else
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can only be executed in a channel "
                  "buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "unban");
            return WEECHAT_RC_OK;
        }
    }

    masks = irc_command_mode_masks_convert_ranges (argv, pos_args);
    if (masks)
    {
        irc_command_mode_masks (ptr_server, pos_channel,
                                "unban", "-", "b",
                                masks, 0);
        weechat_string_free_split (masks);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/unquiet": unquiets nicks or hosts.
 */

IRC_COMMAND_CALLBACK(unquiet)
{
    char *pos_channel, **masks;
    int pos_args;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("unquiet", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (irc_channel_is_channel (ptr_server, argv[1]))
    {
        pos_channel = argv[1];
        pos_args = 2;
    }
    else
    {
        pos_channel = NULL;
        pos_args = 1;
    }

    /* channel not given, use default buffer */
    if (!pos_channel)
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            pos_channel = ptr_channel->name;
        else
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can only be executed in a channel "
                  "buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "unquiet");
            return WEECHAT_RC_OK;
        }
    }

    if (argv[pos_args])
    {
        masks = irc_command_mode_masks_convert_ranges (argv, pos_args);
        if (masks)
        {
            irc_command_mode_masks (ptr_server, pos_channel,
                                    "unquiet", "-", "q",
                                    masks, 0);
            weechat_string_free_split (masks);
        }
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s -q",
                          pos_channel);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/userhost": returns a list of information about
 * nicknames.
 */

IRC_COMMAND_CALLBACK(userhost)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("userhost", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "USERHOST %s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/users": list of users logged into the server.
 */

IRC_COMMAND_CALLBACK(users)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("users", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "USERS %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "USERS");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/version": gives the version info of nick or server
 * (current or specified).
 */

IRC_COMMAND_CALLBACK(version)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("version", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if (argc > 1)
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL)
            && irc_nick_search (ptr_server, ptr_channel, argv[1]))
        {
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "PRIVMSG %s :\01VERSION\01", argv[1]);
        }
        else
        {
            irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                              "VERSION %s", argv[1]);
        }
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "VERSION");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/voice": gives voice to nickname(s).
 */

IRC_COMMAND_CALLBACK(voice)
{
    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("voice", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv_eol;

    if (!ptr_channel || (ptr_channel->type != IRC_CHANNEL_TYPE_CHANNEL))
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: \"%s\" command can only be executed in a channel buffer"),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, "voice");
        return WEECHAT_RC_OK;
    }

    if (argc < 2)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "MODE %s +v %s",
                          ptr_channel->name,
                          ptr_server->nick);
    }
    else
    {
        irc_command_mode_nicks (ptr_server, ptr_channel,
                                "voice", "+", "v", argc, argv);
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/wallchops": sends a notice to channel ops.
 */

IRC_COMMAND_CALLBACK(wallchops)
{
    char *pos_channel;
    int pos_args;
    const char *support_wallchops, *support_statusmsg;
    struct t_irc_nick *ptr_nick;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("wallchops", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    if (irc_channel_is_channel (ptr_server, argv[1]))
    {
        pos_channel = argv[1];
        pos_args = 2;
    }
    else
    {
        pos_channel = NULL;
        pos_args = 1;
    }

    /* channel not given, use default buffer */
    if (!pos_channel)
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_CHANNEL))
            pos_channel = ptr_channel->name;
        else
        {
            weechat_printf (
                ptr_server->buffer,
                _("%s%s: \"%s\" command can only be executed in a channel "
                  "buffer"),
                weechat_prefix ("error"), IRC_PLUGIN_NAME, "wallchops");
            return WEECHAT_RC_OK;
        }
    }

    ptr_channel = irc_channel_search (ptr_server, pos_channel);
    if (!ptr_channel)
    {
        weechat_printf (
            ptr_server->buffer,
            _("%s%s: you are not on channel \"%s\""),
            weechat_prefix ("error"), IRC_PLUGIN_NAME, pos_channel);
        return WEECHAT_RC_OK;
    }

    weechat_printf (
        ptr_channel->buffer,
        "%s%s%sOp%s -> %s%s%s: %s",
        weechat_prefix ("network"),
        IRC_COLOR_NOTICE,
        /* TRANSLATORS: "Notice" is command name in IRC protocol (translation is frequently the same word) */
        _("Notice"),
        IRC_COLOR_RESET,
        IRC_COLOR_CHAT_CHANNEL,
        ptr_channel->name,
        IRC_COLOR_RESET,
        argv_eol[pos_args]);

    support_wallchops = irc_server_get_isupport_value (ptr_server,
                                                       "WALLCHOPS");
    support_statusmsg = irc_server_get_isupport_value (ptr_server,
                                                       "STATUSMSG");
    if (support_wallchops
        || (support_statusmsg && strchr (support_statusmsg, '@')))
    {
        /*
         * if WALLCHOPS is supported, or if STATUSMSG includes '@',
         * then send a notice to @#channel
         */
        irc_server_sendf (ptr_server,
                          IRC_SERVER_SEND_OUTQ_PRIO_HIGH
                          | IRC_SERVER_SEND_MULTILINE,
                          NULL,
                          "NOTICE @%s :%s",
                          ptr_channel->name, argv_eol[pos_args]);
    }
    else
    {
        /*
         * if WALLCHOPS is not supported and '@' not in STATUSMSG,
         * then send a notice to each op of channel
         */
        for (ptr_nick = ptr_channel->nicks; ptr_nick;
             ptr_nick = ptr_nick->next_nick)
        {
            if (irc_nick_is_op_or_higher (ptr_server, ptr_nick)
                && (irc_server_strcasecmp (ptr_server,
                                           ptr_nick->name,
                                           ptr_server->nick) != 0))
            {
                irc_server_sendf (ptr_server,
                                  IRC_SERVER_SEND_OUTQ_PRIO_HIGH
                                  | IRC_SERVER_SEND_MULTILINE,
                                  NULL,
                                  "NOTICE %s :%s",
                                  ptr_nick->name, argv_eol[pos_args]);
            }
        }
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/wallops": sends a message to all currently connected
 * users who have set the 'w' user mode for themselves.
 */

IRC_COMMAND_CALLBACK(wallops)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("wallops", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "WALLOPS :%s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/who": generates a query which returns a list of
 * information.
 */

IRC_COMMAND_CALLBACK(who)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("who", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    if (argc > 1)
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "WHO %s", argv_eol[1]);
    }
    else
    {
        irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                          "WHO");
    }

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/whois": queries information about user(s).
 */

IRC_COMMAND_CALLBACK(whois)
{
    int double_nick;
    const char *ptr_nick;

    IRC_BUFFER_GET_SERVER_CHANNEL(buffer);
    IRC_COMMAND_CHECK_SERVER("whois", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    double_nick = weechat_config_boolean (irc_config_network_whois_double_nick);
    ptr_nick = NULL;

    if (argc > 1)
    {
        if ((argc > 2) || strchr (argv_eol[1], ','))
        {
            /* do not double nick if we have more than one argument or a comma */
            double_nick = 0;
            ptr_nick = argv_eol[1];
        }
        else
            ptr_nick = argv[1];
    }
    else
    {
        if (ptr_channel && (ptr_channel->type == IRC_CHANNEL_TYPE_PRIVATE))
            ptr_nick = ptr_channel->name;
        else if (ptr_server->nick)
            ptr_nick = ptr_server->nick;
    }

    if (!ptr_nick)
        WEECHAT_COMMAND_ERROR;

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "WHOIS %s%s%s",
                      ptr_nick,
                      (double_nick) ? " " : "",
                      (double_nick) ? ptr_nick : "");

    return WEECHAT_RC_OK;
}

/*
 * Callback for command "/whowas": asks for information about a nickname which
 * no longer exists.
 */

IRC_COMMAND_CALLBACK(whowas)
{
    IRC_BUFFER_GET_SERVER(buffer);
    IRC_COMMAND_CHECK_SERVER("whowas", 1, 1);

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) argv;

    WEECHAT_COMMAND_MIN_ARGS(2, "");

    irc_server_sendf (ptr_server, IRC_SERVER_SEND_OUTQ_PRIO_HIGH, NULL,
                      "WHOWAS %s", argv_eol[1]);

    return WEECHAT_RC_OK;
}

/*
 * Hooks IRC commands.
 */

void
irc_command_init ()
{
    weechat_hook_command (
        "action",
        N_("send a CTCP action to a nick or channel"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-server <server>] <target>[,<target>...] <text>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("server: send to this server (internal name)"),
            N_("target: nick or channel (may be mask, \"*\" = current channel)"),
            N_("text: text to send")),
        "-server %(irc_servers) %(nicks)|*"
        " || %(nicks)|*",
        &irc_command_action, NULL, NULL);
    weechat_hook_command (
        "admin",
        N_("find information about the administrator of the server"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name")),
        NULL, &irc_command_admin, NULL, NULL);
    weechat_hook_command (
        "allchan",
        N_("execute a command on all channels of all connected servers"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-current] [-parted|-all] [-exclude=<channel>[,<channel>...]] <command>"
           " || [-current] [-parted|-all] -include=<channel>[,<channel>...] <command>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[-current]: execute command for channels of current server only"),
            N_("raw[-parted]: execute command on parted channels "
               "(by default: execute command on active channels only)"),
            N_("raw[-all]: execute command on all channels (active and parted)"),
            N_("raw[-exclude]: exclude some channels (wildcard \"*\" is allowed)"),
            N_("raw[-include]: include only some channels (wildcard \"*\" is allowed)"),
            N_("command: command to execute (or text to send to buffer if "
               "command does not start with \"/\")"),
            "",
            N_("Command and arguments are evaluated (see /help eval), the following "
               "variables are replaced:"),
            N_("  $server: server name"),
            N_("  $channel: channel name"),
            N_("  $nick: nick on server"),
            N_("  ${irc_server.xxx}: variable xxx in server"),
            N_("  ${irc_channel.xxx}: variable xxx in channel"),
            "",
            N_("Examples:"),
            N_("  execute \"/me is testing\" on all channels:"),
            N_("    /allchan /me is testing"),
            N_("  say \"hello\" everywhere but not on #weechat:"),
            N_("    /allchan -exclude=#weechat hello"),
            N_("  say \"hello\" everywhere but not on #weechat and channels "
               "beginning with #linux:"),
            N_("    /allchan -exclude=#weechat,#linux* hello"),
            N_("  say \"hello\" on all channels beginning with #linux:"),
            N_("    /allchan -include=#linux* hello"),
            N_("  close all buffers with parted channels:"),
            AI("    /allchan -parted /close")),
        "-current|-parted|-all", &irc_command_allchan, NULL, NULL);
    weechat_hook_command (
        "allpv",
        N_("execute a command on all private buffers of all connected servers"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-current] [-exclude=<nick>[,<nick>...]] <command>"
           " || [-current] -include=<nick>[,<nick>...] <command>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[-current]: execute command for private buffers of current server "
               "only"),
            N_("raw[-exclude]: exclude some nicks (wildcard \"*\" is allowed)"),
            N_("raw[-include]: include only some nicks (wildcard \"*\" is allowed)"),
            N_("command: command to execute (or text to send to buffer if "
               "command does not start with \"/\")"),
            "",
            N_("Command and arguments are evaluated (see /help eval), the following "
               "variables are replaced:"),
            N_("  $server: server name"),
            N_("  $channel: channel name"),
            N_("  $nick: nick on server"),
            N_("  ${irc_server.xxx}: variable xxx in server"),
            N_("  ${irc_channel.xxx}: variable xxx in channel"),
            "",
            N_("Examples:"),
            N_("  execute \"/me is testing\" on all private buffers:"),
            N_("    /allpv /me is testing"),
            N_("  say \"hello\" everywhere but not for nick foo:"),
            N_("    /allpv -exclude=foo hello"),
            N_("  say \"hello\" everywhere but not for nick foo and nicks "
               "beginning with bar:"),
            N_("    /allpv -exclude=foo,bar* hello"),
            N_("  say \"hello\" for all nicks beginning with bar:"),
            N_("    /allpv -include=bar* hello"),
            N_("  close all private buffers:"),
            AI("    /allpv /close")),
        "-current", &irc_command_allpv, NULL, NULL);
    weechat_hook_command (
        "allserv",
        N_("execute a command on all connected servers"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-exclude=<server>[,<server>...]] <command>"
           " || -include=<server>[,<server>...] "
           "<command>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[-exclude]: exclude some servers (wildcard \"*\" is allowed)"),
            N_("raw[-include]: include only some servers (wildcard \"*\" is allowed)"),
            N_("command: command to execute (or text to send to buffer if "
               "command does not start with \"/\")"),
            "",
            N_("Command and arguments are evaluated (see /help eval), the following "
               "variables are replaced:"),
            N_("  $server: server name"),
            N_("  $nick: nick on server"),
            N_("  ${irc_server.xxx}: variable xxx in server"),
            "",
            N_("Examples:"),
            N_("  change nick on all servers:"),
            AI("    /allserv /nick newnick"),
            N_("  do a whois on my nick on all servers:"),
            AI("    /allserv /whois $nick")),
        NULL, &irc_command_allserv, NULL, NULL);
    weechat_hook_command (
        "auth",
        N_("authenticate with SASL"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<username> <password>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("username: SASL username (content is evaluated, see /help eval; "
               "server options are evaluated with ${irc_server.xxx} and ${server} "
               "is replaced by the server name)"),
            N_("password: SASL password or path to file with private key "
               "(content is evaluated, see /help eval; server options are "
               "evaluated with ${irc_server.xxx} and ${server} is replaced by the "
               "server name)"),
            "",
            N_("If username and password are not provided, the values from server "
               "options \"sasl_username\" and \"sasl_password\" (or \"sasl_key\") "
               "are used."),
            "",
            N_("Examples:"),
            N_("  authenticate with username/password defined in the server:"),
            AI("    /auth"),
            N_("  authenticate as a different user:"),
            AI("    /auth user2 password2"),
            N_("  authenticate as a different user with mechanism ecdsa-nist256p-challenge:"),
            AI("    /auth user2 ${weechat_config_dir}/ecdsa2.pem")),
        NULL, &irc_command_auth, NULL, NULL);
    weechat_hook_command (
        "autojoin",
        N_("configure the \"autojoin\" server option"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("add [<channel1> [<channel2>...]]"
           " || addraw <channel1>[,<channel2>...] [<key1>[,<key2>...]]"
           " || del [<channel1> [<channel2>...]]"
           " || apply"
           " || join"
           " || sort [buffer]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[add]: add current channel or a list of channels (with optional "
               "keys) to the autojoin option; if you are on the channel and the "
               "key is not provided, the key is read in the channel"),
            N_("raw[addraw]: use the IRC raw format (same as /join command): all "
               "channels separated by commas, optional keys separated by commas"),
            N_("raw[del]: delete current channel or a list of channels from the "
               "autojoin option"),
            N_("channel: channel name"),
            N_("key: key for the channel"),
            N_("raw[apply]: set currently joined channels in the autojoin option"),
            N_("raw[join]: join the channels in the autojoin option"),
            N_("raw[sort]: sort alphabetically channels in the autojoin option; "
               "with \"buffer\": first sort by buffer number, then alphabetically"),
            "",
            N_("Examples:"),
            AI("  /autojoin add"),
            AI("  /autojoin add #test"),
            AI("  /autojoin add #chan1 #chan2"),
            AI("  /allchan /autojoin add"),
            AI("  /autojoin addraw #chan1,#chan2,#chan3 key1,key2"),
            AI("  /autojoin del"),
            AI("  /autojoin del #chan1"),
            AI("  /autojoin apply"),
            AI("  /autojoin join"),
            AI("  /autojoin sort"),
            AI("  /autojoin sort buffer")),
        "add %(irc_channels)|%*"
        " || addraw %(irc_channels) %-"
        " || del %(irc_channels_autojoin)|%*"
        " || apply"
        " || join"
        " || sort buffer",
        &irc_command_autojoin, NULL, NULL);
    weechat_hook_command_run ("/away", &irc_command_run_away, NULL, NULL);
    weechat_hook_command (
        "ban",
        N_("ban nicks or hosts"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] [<nick> [<nick>...]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("nick: nick or host"),
            "",
            N_("Without argument, this command displays the ban list for current "
               "channel.")),
        "%(irc_channel_nicks_hosts)", &irc_command_ban, NULL, NULL);
    weechat_hook_command (
        "cap",
        N_("client capability negotiation"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("ls"
           " || list"
           " || req|ack [<capability> [<capability>...]]"
           " || end"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[ls]: list the capabilities supported by the server"),
            N_("raw[list]: list the capabilities currently enabled"),
            N_("raw[req]: request a new capability or remove a capability "
               "(if starting with \"-\", for example: \"-multi-prefix\")"),
            N_("raw[ack]: acknowledge capabilities which require client-side "
               "acknowledgement"),
            N_("raw[end]: end the capability negotiation"),
            "",
            N_("Without argument, \"ls\" and \"list\" are sent."),
            "",
            N_("Capabilities supported by WeeChat are: "
               "account-notify, account-tag, away-notify, batch, cap-notify, "
               "chghost, draft/multiline, echo-message, extended-join, "
               "invite-notify, message-tags, multi-prefix, server-time, setname, "
               "userhost-in-names."),
            "",
            N_("The capabilities to automatically enable on servers can be set "
               "in option irc.server_default.capabilities (or by server in "
               "option irc.server.xxx.capabilities)."),
            "",
            N_("Examples:"),
            N_("  display supported and enabled capabilities:"),
            AI("    /cap"),
            N_("  request capabilities multi-prefix and away-notify:"),
            AI("    /cap req multi-prefix away-notify"),
            N_("  request capability extended-join, remove capability multi-prefix:"),
            AI("    /cap req extended-join -multi-prefix"),
            N_("  remove capability away-notify:"),
            AI("    /cap req -away-notify")),
        "ls"
        " || list"
        " || req " IRC_COMMAND_CAP_SUPPORTED "|%*"
        " || ack " IRC_COMMAND_CAP_SUPPORTED "|%*"
        " || end",
        &irc_command_cap, NULL, NULL);
    weechat_hook_command (
        "connect",
        N_("connect to IRC server(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<server> [<server>...]] [-<option>[=<value>]] [-no<option>] "
           "[-nojoin] [-switch]"
           " || -all|-auto|-open [-nojoin] [-switch]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("server: server name, which can be:"),
            N_("> - internal server name (added by /server add, "
               "recommended usage)"),
            N_("> - hostname/port or IP/port, port is 6697 by default "
               "for TLS, 6667 otherwise"),
            N_("> - URL with format: irc[6][s]://[nickname[:password]@]"
               "irc.example.org[:port][/#channel1][,#channel2[...]]"),
            N_("> Note: for an address/IP/URL, a temporary server is "
               "added (NOT SAVED), see /help irc.look.temporary_servers"),
            N_("option: set option for server (for boolean option, value can be "
               "omitted)"),
            N_("raw[nooption]: set boolean option to \"off\" (for example: -notls)"),
            N_("raw[-all]: connect to all servers defined in configuration"),
            N_("raw[-auto]: connect to servers with autoconnect enabled"),
            N_("raw[-open]: connect to all opened servers that are not currently connected"),
            N_("raw[-nojoin]: do not join any channel (even if autojoin is enabled on server)"),
            N_("raw[-switch]: switch to next server address"),
            "",
            N_("To disconnect from a server or stop any connection attempt, use "
               "command /disconnect."),
            "",
            N_("Examples:"),
            AI("  /connect libera"),
            AI("  /connect irc.oftc.net"),
            AI("  /connect irc.oftc.net/6667 -notls"),
            AI("  /connect irc6.oftc.net/9999 -ipv6"),
            AI("  /connect my.server.org -password=test"),
            AI("  /connect irc://nick@irc.oftc.net/#channel"),
            AI("  /connect -switch")),
        "%(irc_servers)|-all|-auto|-open|-nojoin|-switch|%*",
        &irc_command_connect, NULL, NULL);
    weechat_hook_command (
        "ctcp",
        N_("send a CTCP message (Client-To-Client Protocol)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-server <server>] <target>[,<target>...] <type> [<arguments>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("server: send to this server (internal name)"),
            N_("target: nick or channel (\"*\" = current channel)"),
            N_("type: CTCP type (examples: \"version\", \"ping\", etc.)"),
            N_("arguments: arguments for CTCP"),
            "",
            N_("Examples:"),
            AI("  /ctcp toto time"),
            AI("  /ctcp toto version"),
            AI("  /ctcp * version")),
        "-server %(irc_servers) %(irc_channel)|%(nicks)|* "
        IRC_COMMAND_CTCP_SUPPORTED_COMPLETION
        " || %(irc_channel)|%(nicks)|* "
        IRC_COMMAND_CTCP_SUPPORTED_COMPLETION,
        &irc_command_ctcp, NULL, NULL);
    weechat_hook_command (
        "cycle",
        N_("leave and rejoin a channel"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>[,<channel>...]] [<message>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("message: part message (displayed to other users)")),
        "%(irc_msg_part)", &irc_command_cycle, NULL, NULL);
    weechat_hook_command (
        "dcc",
        N_("start a DCC (passive file transfer or direct chat)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("chat <nick> || send <nick> <file>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick"),
            N_("file: filename (on local host)"),
            "",
            N_("Examples:"),
            AI("  /dcc chat toto"),
            AI("  /dcc send toto /home/foo/bar.txt")),
        "chat %(nicks)"
        " || send %(nicks) "
        "%(filename:${modifier:eval_path_home,directory=data,"
        "${xfer.file.upload_path}})",
        &irc_command_dcc, NULL, NULL);
    weechat_hook_command (
        "dehalfop",
        N_("remove channel half-operator status from nick(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<nick>...] || * -yes"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick or mask (wildcard \"*\" is allowed)"),
            N_("*: remove channel half-operator status from everybody on channel "
               "except yourself")),
        "%(nicks)|%*", &irc_command_dehalfop, NULL, NULL);
    weechat_hook_command (
        "deop",
        N_("remove channel operator status from nick(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<nick>...] || * -yes"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick or mask (wildcard \"*\" is allowed)"),
            N_("*: remove channel operator status from everybody on channel except yourself")),
        "%(nicks)|%*", &irc_command_deop, NULL, NULL);
    weechat_hook_command (
        "devoice",
        N_("remove voice from nick(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<nick>...] || * -yes"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick or mask (wildcard \"*\" is allowed)"),
            N_("*: remove voice from everybody on channel")),
        "%(nicks)|%*", &irc_command_devoice, NULL, NULL);
    weechat_hook_command (
        "die",
        N_("shutdown the server"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name")),
        NULL, &irc_command_die, NULL, NULL);
    weechat_hook_command (
        "disconnect",
        N_("disconnect from one or all IRC servers"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<server>|-all|-pending [<reason>]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("server: internal server name"),
            N_("raw[-all]: disconnect from all servers"),
            N_("raw[-pending]: cancel auto-reconnection on servers currently reconnecting"),
            N_("reason: reason for the \"quit\"")),
        "%(irc_servers)|-all|-pending",
        &irc_command_disconnect, NULL, NULL);
    weechat_hook_command (
        "halfop",
        N_("give channel half-operator status to nick(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<nick>...] || * -yes"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick or mask (wildcard \"*\" is allowed)"),
            N_("*: give channel half-operator status to everybody on channel")),
        "%(nicks)|%*", &irc_command_halfop, NULL, NULL);
    weechat_hook_command (
        "ignore",
        N_("ignore nicks/hosts from servers or channels"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("list"
           " || add [re:]<nick> [<server> [<channel>]]"
           " || del <number>|-all"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[list]: list all ignores"),
            N_("raw[add]: add an ignore"),
            N_("nick: nick or hostname; can be a POSIX extended regular expression "
               "if \"re:\" is given or a mask using \"*\" to replace zero or more "
               "chars (the regular expression can start with \"(?-i)\" to become "
               "case sensitive)"),
            N_("raw[del]: delete an ignore"),
            N_("number: number of ignore to delete (look at list to find it)"),
            N_("raw[-all]: delete all ignores"),
            N_("server: internal server name where ignore is working"),
            N_("channel: channel name where ignore is working"),
            "",
            N_("Note: if option irc.look.ignore_tag_messages is enabled, the "
               "ignored messages are just tagged with \"irc_ignored\" instead "
               "of being completely removed."),
            "",
            N_("Examples:"),
            AI("  /ignore add toto"),
            AI("  /ignore add toto@domain.com libera"),
            AI("  /ignore add toto*@*.domain.com libera #weechat")),
        "list"
        " || add %(irc_channel_nicks_hosts) %(irc_servers) %(irc_channels) %-"
        " || del %(irc_ignores_numbers)|-all %-",
        &irc_command_ignore, NULL, NULL);
    weechat_hook_command (
        "info",
        N_("get information describing the server"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name")),
        NULL, &irc_command_info, NULL, NULL);
    weechat_hook_command (
        "invite",
        N_("invite a nick on a channel"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<nick>...] [<channel>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick"),
            N_("channel: channel name")),
        "%(nicks) %(irc_server_channels)", &irc_command_invite, NULL, NULL);
    weechat_hook_command (
        "ison",
        N_("check if a nick is currently on IRC"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<nick>...]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick")),
        "%(nicks)|%*", &irc_command_ison, NULL, NULL);
    weechat_hook_command (
        "join",
        N_("join a channel"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-noswitch] [-server <server>] "
           "[<channel1>[,<channel2>...]] [<key1>[,<key2>...]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[-noswitch]: do not switch to new buffer"),
            N_("server: send to this server (internal name)"),
            N_("channel: channel name"),
            N_("key: key to join the channel (channels with a key must be the "
               "first in list)"),
            "",
            N_("Examples:"),
            AI("  /join #weechat"),
            AI("  /join #protectedchan,#weechat key"),
            AI("  /join -server libera #weechat"),
            AI("  /join -noswitch #weechat")),
        "%(irc_channels)|-noswitch|-server|%(irc_servers)|%*",
        &irc_command_join, NULL, NULL);
    weechat_hook_command (
        "kick",
        N_("kick a user out of a channel"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] <nick> [<reason>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("nick: nick"),
            N_("reason: reason (evaluated, see /help eval; special variables "
               "${nick} (self nick), ${target} (target nick), ${channel} and "
               "${server} are replaced by their values)")),
        "%(nicks) %(irc_msg_kick) %-", &irc_command_kick, NULL, NULL);
    weechat_hook_command (
        "kickban",
        N_("kick a user out of a channel and ban the host"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] <nick> [<reason>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("nick: nick"),
            N_("reason: reason (evaluated, see /help eval; special variables "
               "${nick} (self nick), ${target} (target nick), ${channel} and "
               "${server} are replaced by their values)"),
            "",
            N_("It is possible to kick/ban with a mask, nick will be extracted from "
               "mask and replaced by \"*\"."),
            "",
            N_("Example:"),
            AI("  /kickban toto!*@host.com")),
        "%(irc_channel_nicks_hosts) %(irc_msg_kick) %-",
        &irc_command_kickban, NULL, NULL);
    weechat_hook_command (
        "kill",
        N_("close client-server connection"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<reason>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick"),
            N_("reason: reason")),
        "%(nicks) %-", &irc_command_kill, NULL, NULL);
    weechat_hook_command (
        "knock",
        N_("send a notice to an invitation-only channel, requesting an invite"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<channel> [<message>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("message: message to send")),
        "%(irc_channels)",
        &irc_command_knock, NULL, NULL);
    weechat_hook_command (
        "links",
        N_("list all server names which are known by the server answering the "
           "query"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[[<target>] <server_mask>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: this remote server should answer the query"),
            N_("server_mask: list of servers must match this mask")),
        NULL, &irc_command_links, NULL, NULL);
    weechat_hook_command (
        "list",
        N_("list channels and their topics"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-server <server>] [-re <regex>] [<channel>[,<channel>...]] "
           "[<target>]"
           " || -up|-down [<number>]"
           " || -left|-right [<percent>]"
           " || -go <line>|end"
           " || -join"),
        WEECHAT_CMD_ARGS_DESC(
            N_("server: send to this server (internal name)"),
            N_("regex: POSIX extended regular expression used to filter results "
               "(case insensitive, can start by \"(?-i)\" to become case "
               "sensitive); when a regular expression is used, the result is "
               "displayed on server buffer instead of a dedicated buffer"),
            N_("channel: channel name"),
            N_("target: server name"),
            N_("raw[-up]: move the selected line up by \"number\" lines"),
            N_("raw[-down]: move the selected line down by \"number\" lines"),
            N_("raw[-left]: scroll the buffer by \"percent\" of width on the left"),
            N_("raw[-right]: scroll the buffer by \"percent\" of width on the right"),
            N_("raw[-go]: select a line by number, first line number is 0 "
               "(\"end\" to select the last line)"),
            N_("raw[-join]: join the channel on the selected line"),
            "",
            N_("For keys, input and mouse actions on the buffer, "
               "see key bindings in User's guide."),
            "",
            N_("Sort keys on /list buffer:"),
            N_("  raw[name]: channel name (eg: \"##test\")"),
            N_("  raw[name2]: channel name without prefix (eg: \"test\")"),
            N_("  raw[users]: number of users on channel"),
            N_("  raw[topic]: channel topic"),
            "",
            N_("Examples:"),
            N_("  list all channels on server and display them in a dedicated buffer "
               "(can be slow on large networks):"),
            AI("    /list"),
            N_("  list channel #weechat:"),
            AI("    /list #weechat"),
            N_("  list all channels beginning with \"#weechat\" (can be very slow "
               "on large networks):"),
            AI("    /list -re #weechat.*"),
            N_("  on /list buffer:"),
            N_("    channels with \"weechat\" in name:"),
            AI("      n:weechat"),
            N_("    channels with at least 100 users:"),
            AI("      u:100"),
            N_("    channels with \"freebsd\" (case insensitive) in topic and more than 10 users:"),
            AI("      c:${topic} =- freebsd && ${users} > 10"),
            N_("    sort channels by users (big channels first), then name2 (name without prefix):"),
            AI("      s:-users,name2")),
        "-server %(irc_servers)"
        " || -re"
        " || -up 1|2|3|4|5"
        " || -down 1|2|3|4|5"
        " || -left 10|20|30|40|50|60|70|80|90|100"
        " || -right 10|20|30|40|50|60|70|80|90|100"
        " || -go 0|end"
        " || -join",
        &irc_command_list, NULL, NULL);
    weechat_hook_command (
        "lusers",
        N_("get statistics about the size of the IRC network"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<mask> [<target>]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("mask: servers matching the mask only"),
            N_("target: server for forwarding request")),
        NULL, &irc_command_lusers, NULL, NULL);
    weechat_hook_command (
        "map",
        N_("show a graphical map of the IRC network"),
        "",
        "",
        NULL, &irc_command_map, NULL, NULL);
    weechat_hook_command (
        "me",
        N_("send a CTCP action to the current channel"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<message>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("message: message to send")),
        NULL, &irc_command_me, NULL, NULL);
    weechat_hook_command (
        "mode",
        N_("change channel or user mode"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] [+|-]o|p|s|i|t|n|m|l|b|e|v|k [<arguments>]"
           " || <nick> [+|-]i|s|w|o"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name to modify (default is current one)"),
            "",
            N_("Channel modes:"),
            N_("  o: give/take channel operator privileges"),
            N_("  p: private channel"),
            N_("  s: secret channel"),
            N_("  i: invite-only channel"),
            N_("  t: topic settable by channel operator only"),
            N_("  n: no messages to channel from clients on the outside"),
            N_("  m: moderated channel"),
            N_("  l: set the user limit to channel"),
            N_("  b: set a ban mask to keep users out"),
            N_("  e: set exception mask"),
            N_("  v: give/take the ability to speak on a moderated channel"),
            N_("  k: set a channel key (password)"),
            "",
            N_("User modes:"),
            N_("  nick: nick to modify"),
            N_("  i: invisible"),
            N_("  s: user receives server notices"),
            N_("  w: user receives wallops"),
            N_("  o: operator"),
            "",
            N_("List of modes is not comprehensive, you should read documentation "
               "about your server to see all possible modes."),
            "",
            N_("Examples:"),
            AI("  /mode #weechat +t"),
            AI("  /mode nick +i")),
        "%(irc_channel)|%(irc_server_nick)", &irc_command_mode, NULL, NULL);
    weechat_hook_command (
        "motd",
        N_("get the \"Message Of The Day\""),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name")),
        NULL, &irc_command_motd, NULL, NULL);
    weechat_hook_command (
        "msg",
        N_("send message to a nick or channel"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-server <server>] <target>[,<target>...] <text>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("server: send to this server (internal name)"),
            N_("target: nick or channel (may be mask, \"*\" = current channel)"),
            N_("text: text to send")),
        "-server %(irc_servers) %(nicks)|*"
        " || %(nicks)|*",
        &irc_command_msg, NULL, NULL);
    weechat_hook_command (
        "names",
        N_("list nicks on channels"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-count | -x] [<channel>[,<channel>...]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[-count]: display only number of users"),
            N_("raw[-x]: display only users with this mode: -o for ops, "
               "-h for halfops, -v for voiced, etc. and -* for regular users"),
            N_("channel: channel name")),
        "-count|%(irc_server_prefix_modes_filter) %(irc_channels)"
        " || %(irc_channels)", &irc_command_names, NULL, NULL);
    weechat_hook_command (
        "nick",
        N_("change current nick"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-all] <nick>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[-all]: set new nick for all connected servers"),
            N_("nick: new nick")),
        "-all %(irc_server_nick)"
        " || %(irc_server_nick)",
        &irc_command_nick, NULL, NULL);
    weechat_hook_command (
        "notice",
        N_("send notice message to user"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-server <server>] <target> <text>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("server: send to this server (internal name)"),
            N_("target: nick or channel name"),
            N_("text: text to send")),
        "-server %(irc_servers) %(nicks)"
        " || %(nicks)",
        &irc_command_notice, NULL, NULL);
    weechat_hook_command (
        "notify",
        N_("add a notification for presence or away status of nicks on servers"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("add <nick> [<server> [-away]]"
           " || del <nick>|-all [<server>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[add]: add a notification"),
            N_("nick: nick"),
            N_("server: internal server name (by default current server)"),
            N_("raw[-away]: notify when away message is changed (by doing whois on nick)"),
            N_("raw[del]: delete a notification"),
            N_("raw[-all]: delete all notifications"),
            "",
            N_("Without argument, this command displays notifications for current "
               "server (or all servers if command is issued on core buffer)."),
            "",
            N_("Examples:"),
            AI("  /notify add toto"),
            AI("  /notify add toto libera"),
            AI("  /notify add toto libera -away")),
        "add %(irc_channel_nicks) %(irc_servers) -away %-"
        " || del -all|%(irc_notify_nicks) %(irc_servers) %-",
        &irc_command_notify, NULL, NULL);
    weechat_hook_command (
        "op",
        N_("give channel operator status to nick(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<nick>...] || * -yes"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick or mask (wildcard \"*\" is allowed)"),
            N_("*: give channel operator status to everybody on channel")),
        "%(nicks)|%*", &irc_command_op, NULL, NULL);
    weechat_hook_command (
        "oper",
        N_("get operator privileges"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<user> <password>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("user: user"),
            N_("password: password")),
        NULL, &irc_command_oper, NULL, NULL);
    weechat_hook_command (
        "part",
        N_("leave a channel"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>[,<channel>...]] [<message>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("message: part message (displayed to other users)")),
        "%(irc_msg_part)", &irc_command_part, NULL, NULL);
    weechat_hook_command (
        "ping",
        N_("send a ping to server"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<target1> [<target2>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target1: server"),
            N_("target2: forward ping to this server")),
        NULL, &irc_command_ping, NULL, NULL);
    weechat_hook_command (
        "pong",
        N_("answer to a ping message"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<daemon> [<daemon2>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("daemon: daemon who has responded to Ping message"),
            N_("daemon2: forward message to this daemon")),
        NULL, &irc_command_pong, NULL, NULL);
    weechat_hook_command (
        "query",
        N_("send a private message to a nick"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-noswitch] [-server <server>] <nick>[,<nick>...] [<text>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[-noswitch]: do not switch to new buffer"),
            N_("server: send to this server (internal name)"),
            N_("nick: nick"),
            N_("text: text to send")),
        "-noswitch|-server %(irc_servers) %(nicks)"
        " || %(nicks)",
        &irc_command_query, NULL, NULL);
    weechat_hook_command (
        "quiet",
        N_("quiet nicks or hosts"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] [<nick> [<nick>...]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("nick: nick or host"),
            "",
            N_("Without argument, this command displays the quiet list for "
               "current channel.")),
        "%(irc_channel_nicks_hosts)", &irc_command_quiet, NULL, NULL);
    weechat_hook_command (
        "quote",
        N_("send raw data to server without parsing"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[-server <server>] <data>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("server: send to this server (internal name)"),
            N_("data: raw data to send")),
        "-server %(irc_servers)", &irc_command_quote, NULL, NULL);
    weechat_hook_command (
        "reconnect",
        N_("reconnect to server(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<server> [<server>...] [-nojoin] [-switch]"
           " || -all [-nojoin] [-switch]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("server: internal server name"),
            N_("raw[-all]: reconnect to all servers"),
            N_("raw[-nojoin]: do not join any channel (even if autojoin is enabled on server)"),
            N_("raw[-switch]: switch to next server address")),
        "%(irc_servers)|-all|-nojoin|-switch|%*",
        &irc_command_reconnect, NULL, NULL);
    weechat_hook_command (
        "rehash",
        N_("tell the server to reload its config file"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<option>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("option: extra option, for some servers")),
        NULL, &irc_command_rehash, NULL, NULL);
    weechat_hook_command (
        "remove",
        N_("force a user to leave a channel"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] <nick> [<reason>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("nick: nick"),
            N_("reason: reason (special variables $nick, $channel and $server are "
               "replaced by their values)")),
        "%(irc_channel)|%(nicks) %(nicks)", &irc_command_remove, NULL, NULL);
    weechat_hook_command (
        "restart",
        N_("tell the server to restart itself"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name")),
        NULL, &irc_command_restart, NULL, NULL);
    weechat_hook_command (
        "rules",
        N_("request the server rules"),
        "",
        "",
        NULL, &irc_command_rules, NULL, NULL);
    weechat_hook_command (
        "sajoin",
        N_("force a user to join channel(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> <channel>[,<channel>...]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick"),
            N_("channel: channel name")),
        "%(nicks) %(irc_server_channels)", &irc_command_sajoin, NULL, NULL);
    weechat_hook_command (
        "samode",
        N_("change mode on channel, without having operator status"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] <mode>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("mode: mode for channel")),
        "%(irc_server_channels)", &irc_command_samode, NULL, NULL);
    weechat_hook_command (
        "sanick",
        N_("force a user to use another nick"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> <new_nick>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick"),
            N_("new_nick: new nick")),
        "%(nicks) %(nicks)", &irc_command_sanick, NULL, NULL);
    weechat_hook_command (
        "sapart",
        N_("force a user to leave channel(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> <channel>[,<channel>...]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick"),
            N_("channel: channel name")),
        "%(nicks) %(irc_server_channels)", &irc_command_sapart, NULL, NULL);
    weechat_hook_command (
        "saquit",
        N_("force a user to quit server with a reason"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> <reason>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick"),
            N_("reason: reason")),
        "%(nicks)", &irc_command_saquit, NULL, NULL);
    weechat_hook_command (
        "service",
        N_("register a new service"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> <reserved> <distribution> <type> <reserved> <info>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("distribution: visibility of service"),
            N_("type: reserved for future usage")),
        NULL, &irc_command_service, NULL, NULL);
    weechat_hook_command (
        "server",
        N_("list, add or remove IRC servers"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("list|listfull [<name>]"
           " || add <name> <hostname>[/<port>] [-temp] [-<option>[=<value>]] "
           "[-no<option>]"
           " || copy|rename <name> <new_name>"
           " || reorder <name> [<name>...]"
           " || open <name>|-all [<name>...]"
           " || del|keep <name>"
           " || deloutq|jump"
           " || raw [<filter>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("raw[list]: list servers (without argument, this list is displayed)"),
            N_("raw[listfull]: list servers with detailed info for each server"),
            N_("raw[add]: add a new server"),
            N_("name: server name, for internal and display use; this name "
               "is used to connect to the server (/connect name) and to set server "
               "options: irc.server.name.xxx"),
            N_("hostname: name or IP address of server, with optional port "
               "(default: 6697 for TLS, 6667 otherwise), many addresses can be "
               "separated by a comma"),
            N_("raw[-temp]: add a temporary server (not saved)"),
            N_("option: set option for server (for boolean option, value can be omitted)"),
            N_("raw[nooption]: set boolean option to \"off\" (for example: -notls)"),
            N_("raw[copy]: duplicate a server"),
            N_("raw[rename]: rename a server"),
            N_("raw[reorder]: reorder list of servers"),
            N_("raw[open]: open the server buffer without connecting"),
            N_("raw[keep]: keep server in config file (for temporary servers only)"),
            N_("raw[del]: delete a server"),
            N_("raw[deloutq]: delete messages out queue for all servers (all messages "
               "WeeChat is currently sending)"),
            N_("raw[jump]: jump to server buffer"),
            N_("raw[raw]: open buffer with raw IRC data"),
            N_("filter: set a new filter to see only matching messages (this "
               "filter can be used as input in raw IRC data buffer as well); "
               "allowed formats are:"),
            N_("> `*`: show all messages (no filter)"),
            N_("> `xxx`: show only messages containing \"xxx\""),
            N_("> `s:xxx`: show only messages for server \"xxx\""),
            N_("> `f:xxx`: show only messages with a flag: recv (message "
               "received), sent (message sent), modified (message modified by "
               "a modifier), redirected (message redirected)"),
            N_("> `m:xxx`: show only IRC command \"xxx\""),
            N_("> `c:xxx`: show only messages matching the evaluated "
               "condition \"xxx\", using following variables: output of function "
               "irc_message_parse (like nick, command, channel, text, etc., see "
               "function info_get_hashtable in plugin API reference for the list "
               "of all variables), date (format: \"%FT%T.%f\", see function "
               "util_strftimeval in Plugin API reference), server, recv, sent, "
               "modified, redirected"),
            "",
            N_("Examples:"),
            AI("  /server listfull"),
            AI("  /server add libera irc.libera.chat"),
            AI("  /server add libera irc.libera.chat/6667 -notls -autoconnect"),
            AI("  /server add chatspike irc.chatspike.net/6667,"
               "irc.duckspike.net/6667 -notls"),
            AI("  /server copy libera libera-test"),
            AI("  /server rename libera-test libera2"),
            AI("  /server reorder libera2 libera"),
            AI("  /server del libera"),
            AI("  /server deloutq"),
            AI("  /server raw"),
            AI("  /server raw s:libera"),
            AI("  /server raw c:${recv} && ${command}==PRIVMSG && ${nick}==foo")),
        "list %(irc_servers)"
        " || listfull %(irc_servers)"
        " || add %(irc_servers)"
        " || copy %(irc_servers) %(irc_servers)"
        " || rename %(irc_servers) %(irc_servers)"
        " || keep %(irc_servers)"
        " || reorder %(irc_servers)|%*"
        " || open %(irc_servers)|-all %(irc_servers)|%*"
        " || del %(irc_servers)"
        " || deloutq"
        " || jump"
        " || raw %(irc_raw_filters)",
        &irc_command_server, NULL, NULL);
    weechat_hook_command (
        "servlist",
        N_("list services currently connected to the network"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<mask> [<type>]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("mask: list only services matching this mask"),
            N_("type: list only services of this type")),
        NULL, &irc_command_servlist, NULL, NULL);
    weechat_hook_command (
        "squery",
        N_("deliver a message to a service"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<service> <text>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("service: name of service"),
            N_("text: text to send")),
        NULL, &irc_command_squery, NULL, NULL);
    weechat_hook_command (
        "setname",
        N_("set real name"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<realname>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("realname: new real name")),
        NULL, &irc_command_setname, NULL, NULL);
    weechat_hook_command (
        "squit",
        N_("disconnect server links"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<target> <comment>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name"),
            N_("comment: comment")),
        NULL, &irc_command_squit, NULL, NULL);
    weechat_hook_command (
        "stats",
        N_("query statistics about server"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<query> [<target>]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("query: c/h/i/k/l/m/o/y/u (see RFC1459)"),
            N_("target: server name")),
        NULL, &irc_command_stats, NULL, NULL);
    weechat_hook_command (
        "summon",
        N_("give users who are on a host running an IRC "
           "server a message asking them to please join "
           "IRC"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<user> [<target> [<channel>]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("user: username"),
            N_("target: server name"),
            N_("channel: channel name")),
        NULL, &irc_command_summon, NULL, NULL);
    weechat_hook_command (
        "time",
        N_("query local time from server"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: query time from specified server")),
        NULL, &irc_command_time, NULL, NULL);
    weechat_hook_command (
        "topic",
        N_("get/set channel topic"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] [<topic>|-delete]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("topic: new topic"),
            N_("raw[-delete]: delete channel topic")),
        "%(irc_channel_topic)|-delete", &irc_command_topic, NULL, NULL);
    weechat_hook_command (
        "trace",
        N_("find the route to specific server"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name")),
        NULL, &irc_command_trace, NULL, NULL);
    weechat_hook_command (
        "unban",
        N_("unban nicks or hosts"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] <nick>|<number>|<n1>-<n2> [<nick>|<number>|<n1>-<n2>...]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("nick: nick or host"),
            N_("number: ban number (as displayed by command /ban)"),
            N_("n1: interval start number"),
            N_("n2: interval end number")),
        "%(irc_modelist_masks:b)|%(irc_modelist_numbers:b)",
        &irc_command_unban, NULL, NULL);
    weechat_hook_command (
        "unquiet",
        N_("unquiet nicks or hosts"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] <nick>|<number>|<n1>-<n2> [<nick>|<number>|<n1>-<n2>...]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("nick: nick or host"),
            N_("number: quiet number (as displayed by command /quiet)"),
            N_("n1: interval start number"),
            N_("n2: interval end number")),
        "%(irc_modelist_masks:q)|%(irc_modelist_numbers:q)",
        &irc_command_unquiet, NULL, NULL);
    weechat_hook_command (
        "userhost",
        N_("return a list of information about nicks"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<nick>...]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick")),
        "%(nicks)", &irc_command_userhost, NULL, NULL);
    weechat_hook_command (
        "users",
        N_("list of users logged into the server"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name")),
        NULL, &irc_command_users, NULL, NULL);
    weechat_hook_command (
        "version",
        N_("give the version info of nick or server (current or specified)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>|<nick>]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name"),
            N_("nick: nick")),
        "%(nicks)", &irc_command_version, NULL, NULL);
    weechat_hook_command (
        "voice",
        N_("give voice to nick(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick> [<nick>...] || * -yes"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick or mask (wildcard \"*\" is allowed)"),
            N_("*: give voice to everybody on channel")),
        "%(nicks)|%*", &irc_command_voice, NULL, NULL);
    weechat_hook_command (
        "wallchops",
        N_("send a notice to channel ops"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<channel>] <text>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("channel: channel name"),
            N_("text: text to send")),
        NULL, &irc_command_wallchops, NULL, NULL);
    weechat_hook_command (
        "wallops",
        N_("send a message to all currently connected users who have set the "
           "\"w\" user mode for themselves"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<text>"),
        WEECHAT_CMD_ARGS_DESC(
            N_("text: text to send")),
        NULL, &irc_command_wallops, NULL, NULL);
    weechat_hook_command (
        "who",
        N_("generate a query which returns a list of information"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<mask> [o]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("mask: only information which match this mask"),
            N_("o: only operators are returned according to the mask supplied")),
        "%(irc_channels)", &irc_command_who, NULL, NULL);
    weechat_hook_command (
        "whois",
        N_("query information about user(s)"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("[<target>] [<nick>[,<nick>...]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("target: server name"),
            N_("nick: nick (may be a mask)"),
            "",
            N_("Without argument, this command will do a whois on:"),
            N_("  - your own nick if buffer is a server/channel"),
            N_("  - remote nick if buffer is a private."),
            "",
            N_("If option irc.network.whois_double_nick is enabled, two nicks are "
               "sent (if only one nick is given), to get idle time in answer.")),
        "%(nicks)", &irc_command_whois, NULL, NULL);
    weechat_hook_command (
        "whowas",
        N_("ask for information about a nick which no longer exists"),
        /* TRANSLATORS: only text between angle brackets (eg: "<name>") must be translated */
        N_("<nick>[,<nick>...] [<count> [<target>]]"),
        WEECHAT_CMD_ARGS_DESC(
            N_("nick: nick"),
            N_("count: number of replies to return (full search if negative number)"),
            N_("target: reply should match this mask")),
        "%(nicks)", &irc_command_whowas, NULL, NULL);
}
