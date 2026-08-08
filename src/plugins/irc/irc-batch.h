/*
 * SPDX-FileCopyrightText: 2023-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_IRC_BATCH_H
#define WEECHAT_PLUGIN_IRC_BATCH_H

#include <time.h>

struct t_hashtable;
struct t_irc_server;

struct t_irc_batch
{
    char *reference;                    /* batch reference                  */
    char *parent_ref;                   /* ref of parent batch (optional)   */
    char *type;                         /* type                             */
    char *parameters;                   /* parameters                       */
    struct t_hashtable *tags;           /* batch message tags               */
    time_t start_time;                  /* start time (to auto-purge if     */
                                        /* batch end is not received)       */
    char **messages;                    /* messages separated by '\n'       */
    int end_received;                   /* batch end reference received     */
    int messages_processed;             /* 1 if msgs have been processed    */
    struct t_irc_batch *prev_batch;     /* link to previous batch           */
    struct t_irc_batch *next_batch;     /* link to next batch               */
};

extern void irc_batch_generate_random_ref (char *string, int size);
extern struct t_irc_batch *irc_batch_search (struct t_irc_server *server,
                                             const char *reference);
extern struct t_irc_batch *irc_batch_start_batch (struct t_irc_server *server,
                                                  const char *reference,
                                                  const char *parent_ref,
                                                  const char *type,
                                                  const char *parameters,
                                                  struct t_hashtable *tags);
extern int irc_batch_add_message (struct t_irc_server *server,
                                  const char *reference,
                                  const char *irc_message);
extern void irc_batch_end_batch (struct t_irc_server *server,
                                 const char *reference);
extern void irc_batch_free (struct t_irc_server *server,
                            struct t_irc_batch *batch);
extern void irc_batch_free_all (struct t_irc_server *server);
extern char *irc_batch_modifier_cb (const void *pointer, void *data,
                                    const char *modifier,
                                    const char *modifier_data,
                                    const char *string);

extern struct t_hdata *irc_batch_hdata_batch_cb (const void *pointer,
                                                 void *data,
                                                 const char *hdata_name);
extern void irc_batch_print_log (struct t_irc_server *server);

#endif /* WEECHAT_PLUGIN_IRC_BATCH_H */
