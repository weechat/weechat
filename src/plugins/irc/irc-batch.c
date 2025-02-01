/*
 * irc-batch.c - functions for managing batched events
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "irc.h"
#include "irc-batch.h"
#include "irc-message.h"
#include "irc-protocol.h"
#include "irc-raw.h"
#include "irc-server.h"
#include "irc-tag.h"


/*
 * Searches a batch reference.
 *
 * Returns pointer to batch, NULL if not found.
 */

struct t_irc_batch *
irc_batch_search (struct t_irc_server *server, const char *reference)
{
    struct t_irc_batch *ptr_batch;

    if (!server || !reference)
        return NULL;

    for (ptr_batch = server->batches; ptr_batch;
         ptr_batch = ptr_batch->next_batch)
    {
        if (strcmp (ptr_batch->reference, reference) == 0)
            return ptr_batch;
    }

    /* batch not found */
    return NULL;
}

/*
 * Generates a random batch reference with `size` chars (the next one is the
 * final '\0', so the string must be at least size + 1 bytes long).
 */

void
irc_batch_generate_random_ref (char *string, int size)
{
    const char *chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    int i, length_chars;

    if (!string || (size < 0))
        return;

    length_chars = strlen (chars);
    for (i = 0; i < size; i++)
    {
        string[i] = chars[rand () % length_chars];
    }
    string[size] = '\0';
}

/*
 * Adds a batch to list of batched events.
 */

void
irc_batch_add_to_list (struct t_irc_server *server, struct t_irc_batch *batch)
{
    if (server->last_batch)
        server->last_batch->next_batch = batch;
    else
        server->batches = batch;
    batch->prev_batch = server->last_batch;
    batch->next_batch = NULL;
    server->last_batch = batch;
}

/*
 * Starts a batch.
 *
 * Returns pointer to new batch, NULL if error.
 */

struct t_irc_batch *
irc_batch_start_batch (struct t_irc_server *server, const char *reference,
                       const char *parent_ref, const char *type,
                       const char *parameters, struct t_hashtable *tags)
{
    struct t_irc_batch *ptr_batch;

    if (!server || !reference || !type)
        return NULL;

    /* check if reference already exists */
    ptr_batch = irc_batch_search (server, reference);
    if (ptr_batch)
        return NULL;

    ptr_batch = malloc (sizeof (*ptr_batch));
    if (!ptr_batch)
        return NULL;

    ptr_batch->reference = strdup (reference);
    ptr_batch->parent_ref = (parent_ref) ? strdup (parent_ref) : NULL;
    ptr_batch->type = strdup (type);
    ptr_batch->parameters = (parameters) ? strdup (parameters) : NULL;
    ptr_batch->tags = (tags) ? weechat_hashtable_dup (tags) : NULL;
    ptr_batch->start_time = time (NULL);
    ptr_batch->messages = NULL;
    ptr_batch->end_received = 0;
    ptr_batch->messages_processed = 0;

    irc_batch_add_to_list (server, ptr_batch);

    return ptr_batch;
}

/*
 * Adds an IRC message to a batch reference.
 *
 * Returns:
 *   1: OK, message added
 *   0: error, message not added
 */

int
irc_batch_add_message (struct t_irc_server *server, const char *reference,
                       const char *irc_message)
{
    struct t_irc_batch *ptr_batch;

    if (!server || !reference || !irc_message)
        return 0;

    ptr_batch = irc_batch_search (server, reference);
    if (!ptr_batch)
        return 0;

    if (!ptr_batch->messages)
        ptr_batch->messages = weechat_string_dyn_alloc (256);
    if (!ptr_batch->messages)
        return 0;

    if ((*(ptr_batch->messages))[0])
        weechat_string_dyn_concat (ptr_batch->messages, "\n", -1);
    weechat_string_dyn_concat (ptr_batch->messages, irc_message, -1);

    return 1;
}

/*
 * Frees a batch.
 */

void
irc_batch_free (struct t_irc_server *server, struct t_irc_batch *batch)
{
    free (batch->reference);
    free (batch->parent_ref);
    free (batch->type);
    free (batch->parameters);
    weechat_hashtable_free (batch->tags);
    weechat_string_dyn_free (batch->messages, 1);

    /* remove batch from list */
    if (batch->prev_batch)
        (batch->prev_batch)->next_batch = batch->next_batch;
    if (batch->next_batch)
        (batch->next_batch)->prev_batch = batch->prev_batch;
    if (server->batches == batch)
        server->batches = batch->next_batch;
    if (server->last_batch == batch)
        server->last_batch = batch->prev_batch;

    free (batch);
}

/*
 * Frees all batches from server.
 */

void
irc_batch_free_all (struct t_irc_server *server)
{
    while (server->batches)
    {
        irc_batch_free (server, server->batches);
    }
}

/*
 * Processes messages in a batch.
 */

void
irc_batch_process_messages (struct t_irc_server *server,
                            struct t_irc_batch *batch)
{
    char **list_messages, *command, *channel, modifier_data[1024], *new_messages;
    char *message, *message2;
    int i, count_messages;

    if (!batch || !batch->messages)
        return;

    snprintf (modifier_data, sizeof (modifier_data),
              "%s,%s,%s",
              server->name,
              batch->type,
              batch->parameters);
    new_messages = weechat_hook_modifier_exec ("irc_batch", modifier_data,
                                               *(batch->messages));

    /* no changes in new messages */
    if (new_messages && (strcmp (*(batch->messages), new_messages) == 0))
    {
        free (new_messages);
        new_messages = NULL;
    }

    /* messages not dropped? */
    if (!new_messages || new_messages[0])
    {
        list_messages = weechat_string_split (
            (new_messages) ? new_messages : *(batch->messages),
            "\n", NULL, 0, 0, &count_messages);
        if (list_messages)
        {
            for (i = 0; i < count_messages; i++)
            {
                message = weechat_string_replace (list_messages[i], "\r", "\n");
                if (!message)
                    continue;

                message2 = irc_tag_add_tags_to_message (message,
                                                        batch->tags);
                if (!message2)
                    continue;

                irc_message_parse (server,
                                   message2,
                                   NULL,   /* tags */
                                   NULL,   /* message_without_tags */
                                   NULL,   /* nick */
                                   NULL,   /* user */
                                   NULL,   /* host */
                                   &command,
                                   &channel,
                                   NULL,   /* arguments */
                                   NULL,   /* text */
                                   NULL,   /* params */
                                   NULL,   /* num_params */
                                   NULL,   /* pos_command */
                                   NULL,   /* pos_arguments */
                                   NULL,   /* pos_channel */
                                   NULL);  /* pos_text */

                /* add raw message */
                irc_raw_print (server, IRC_RAW_FLAG_RECV, message2);

                /* call receive callback, ignoring batch tags */
                irc_protocol_recv_command (server, message2, command, channel, 1);

                free (message);
                free (message2);
                free (command);
                free (channel);
            }
            weechat_string_free_split (list_messages);
        }
    }

    free (new_messages);
}

/*
 * Ends a batch reference.
 */

void
irc_batch_end_batch (struct t_irc_server *server, const char *reference)
{
    struct t_irc_batch *ptr_batch, *ptr_next_batch, *ptr_parent_batch;
    int num_processed;

    if (!server || !reference)
        return;

    ptr_batch = irc_batch_search (server, reference);
    if (!ptr_batch)
        return;

    ptr_batch->end_received = 1;

    /*
     * process messages in all batches, if these conditions are met:
     *   - end_received = 1
     *   - no parent or the parent has messages_processed = 1
     */
    while (1)
    {
        num_processed = 0;
        for (ptr_batch = server->batches; ptr_batch;
             ptr_batch = ptr_batch->next_batch)
        {
            if (!ptr_batch->end_received || ptr_batch->messages_processed)
                continue;
            ptr_parent_batch = irc_batch_search (server, ptr_batch->parent_ref);
            if (!ptr_parent_batch || ptr_parent_batch->messages_processed)
            {
                irc_batch_process_messages (server, ptr_batch);
                ptr_batch->messages_processed = 1;
                num_processed++;
            }
        }
        if (num_processed == 0)
            break;
    }

    /* remove all batches that are processed */
    ptr_batch = server->batches;
    while (ptr_batch)
    {
        ptr_next_batch = ptr_batch->next_batch;
        if (ptr_batch->messages_processed)
            irc_batch_free (server, ptr_batch);
        ptr_batch = ptr_next_batch;
    }
}

/*
 * Processes multiline batch: convert multiple messages into a single one,
 * that can include newline chars ("\r" that are converted later to "\n").
 *
 * Parameter "target" is the batch target (channel or nick name).
 *
 * Note: result must be freed after use.
 */

char *
irc_batch_process_multiline (struct t_irc_server *server,
                             const char *messages, const char *target)
{
    char **result, **list_messages;
    char *tags, *host, *command, *channel, *text;
    int i, count_messages;
    struct t_hashtable *hash_tags;

    result = weechat_string_dyn_alloc (256);

    hash_tags = NULL;

    list_messages = weechat_string_split (messages, "\n", NULL, 0, 0,
                                          &count_messages);
    if (!list_messages)
        goto end;

    hash_tags = weechat_hashtable_new (32,
                                       WEECHAT_HASHTABLE_STRING,
                                       WEECHAT_HASHTABLE_STRING,
                                       NULL, NULL);

    for (i = 0; i < count_messages; i++)
    {
        irc_message_parse (server,
                           list_messages[i],
                           &tags,
                           NULL,   /* message_without_tags */
                           NULL,   /* nick */
                           NULL,   /* user */
                           &host,
                           &command,
                           &channel,
                           NULL,   /* arguments */
                           &text,
                           NULL,   /* params */
                           NULL,   /* num_params */
                           NULL,   /* pos_command */
                           NULL,   /* pos_arguments */
                           NULL,   /* pos_channel */
                           NULL);  /* pos_text */
        if (host
            && command
            && ((strcmp (command, "PRIVMSG") == 0)
                || (strcmp (command, "NOTICE") == 0))
            && channel
            && (strcmp (channel, target) == 0))
        {
            if (hash_tags)
            {
                weechat_hashtable_remove_all (hash_tags);
                if (tags && tags[0])
                    irc_tag_parse (tags, hash_tags, NULL);
            }
            if ((*result)[0])
            {
                if (!hash_tags
                    || !weechat_hashtable_has_key (hash_tags,
                                                   "draft/multiline-concat"))
                {
                    weechat_string_dyn_concat (result, "\r", -1);
                }
            }
            else
            {
                if (tags && tags[0])
                {
                    weechat_string_dyn_concat (result, "@", -1);
                    weechat_string_dyn_concat (result, tags, -1);
                    weechat_string_dyn_concat (result, " ", -1);
                }
                weechat_string_dyn_concat (result, ":", -1);
                weechat_string_dyn_concat (result, host, -1);
                weechat_string_dyn_concat (result, " ", -1);
                weechat_string_dyn_concat (result, command, -1);
                weechat_string_dyn_concat (result, " ", -1);
                weechat_string_dyn_concat (result, target, -1);
                weechat_string_dyn_concat (result, " :", -1);
            }
            if (text)
                weechat_string_dyn_concat (result, text, -1);
        }
        free (tags);
        free (host);
        free (command);
        free (channel);
        free (text);
    }

end:
    weechat_hashtable_free (hash_tags);
    weechat_string_free_split (list_messages);

    return weechat_string_dyn_free (result, 0);
}

/*
 * Callback for modifier "irc_batch".
 */

char *
irc_batch_modifier_cb (const void *pointer, void *data,
                       const char *modifier, const char *modifier_data,
                       const char *string)
{
    struct t_irc_server *ptr_server;
    char **items, *result;
    int num_items;

    /* make C compiler happy */
    (void) pointer;
    (void) data;
    (void) modifier;

    result = NULL;

    if (!modifier_data)
        return NULL;

    items = weechat_string_split (modifier_data, ",", NULL, 0, 3, &num_items);
    if (!items)
        return NULL;

    if (items && (num_items > 1))
    {
        ptr_server = irc_server_search (items[0]);
        if (ptr_server
            && (num_items > 2)
            && (strcmp (items[1], "draft/multiline") == 0)
            && weechat_hashtable_has_key (ptr_server->cap_list, "draft/multiline"))
        {
            result = irc_batch_process_multiline (ptr_server, string, items[2]);
        }
    }
    weechat_string_free_split (items);

    return (result) ? result : strdup (string);
}

/*
 * Returns hdata for batch.
 */

struct t_hdata *
irc_batch_hdata_batch_cb (const void *pointer, void *data,
                          const char *hdata_name)
{
    struct t_hdata *hdata;

    /* make C compiler happy */
    (void) pointer;
    (void) data;

    hdata = weechat_hdata_new (hdata_name, "prev_batch", "next_batch",
                               0, 0, NULL, NULL);
    if (hdata)
    {
        WEECHAT_HDATA_VAR(struct t_irc_batch, reference, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_batch, parent_ref, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_batch, type, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_batch, parameters, STRING, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_batch, start_time, TIME, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_batch, messages, POINTER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_batch, end_received, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_batch, messages_processed, INTEGER, 0, NULL, NULL);
        WEECHAT_HDATA_VAR(struct t_irc_batch, prev_batch, POINTER, 0, NULL, hdata_name);
        WEECHAT_HDATA_VAR(struct t_irc_batch, next_batch, POINTER, 0, NULL, hdata_name);
    }
    return hdata;
}

/*
 * Prints batch infos in WeeChat log file (usually for crash dump).
 */

void
irc_batch_print_log (struct t_irc_server *server)
{
    struct t_irc_batch *ptr_batch;

    for (ptr_batch = server->batches; ptr_batch;
         ptr_batch = ptr_batch->next_batch)
    {
        weechat_log_printf ("");
        weechat_log_printf ("  => batch (addr:%p):", ptr_batch);
        weechat_log_printf ("       reference . . . . . : '%s'", ptr_batch->reference);
        weechat_log_printf ("       parent_ref. . . . . : '%s'", ptr_batch->parent_ref);
        weechat_log_printf ("       type. . . . . . . . : '%s'", ptr_batch->type);
        weechat_log_printf ("       parameters. . . . . : '%s'", ptr_batch->parameters);
        weechat_log_printf ("       tags. . . . . . . . : %p (hashtable: '%s')",
                            ptr_batch->tags,
                            weechat_hashtable_get_string (ptr_batch->tags,
                                                          "keys_values"));
        weechat_log_printf ("       start_time. . . . . : %lld", (long long)ptr_batch->start_time);
        weechat_log_printf ("       message . . . . . . : %p ('%s')",
                            ptr_batch->messages,
                            (ptr_batch->messages) ? *(ptr_batch->messages) : NULL);
        weechat_log_printf ("       end_received. . . . : %d", ptr_batch->end_received);
        weechat_log_printf ("       messages_processed. : %d", ptr_batch->messages_processed);
        weechat_log_printf ("       prev_batch. . . . . : %p", ptr_batch->prev_batch);
        weechat_log_printf ("       next_batch. . . . . : %p", ptr_batch->next_batch);
    }
}
