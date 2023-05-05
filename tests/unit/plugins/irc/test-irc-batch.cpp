/*
 * test-irc-batch.cpp - test IRC batch functions
 *
 * Copyright (C) 2023 Sébastien Helleu <flashcode@flashtux.org>
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

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include <string.h>
#include "src/plugins/irc/irc-batch.h"
#include "src/plugins/irc/irc-server.h"
}

TEST_GROUP(IrcBatch)
{
};

/*
 * Tests functions:
 *   irc_batch_search
 */

TEST(IrcBatch, Search)
{
    struct t_irc_server *server;
    struct t_irc_batch *batch1, *batch2;

    server = irc_server_alloc ("server");
    CHECK(server);

    batch1 = irc_batch_start_batch (server, "ref1", "parent_ref", "type", "params");
    CHECK(batch1);

    batch2 = irc_batch_start_batch (server, "ref2", "parent_ref", "type", "params");
    CHECK(batch2);

    POINTERS_EQUAL(NULL, irc_batch_search (NULL, NULL));
    POINTERS_EQUAL(NULL, irc_batch_search (NULL, ""));
    POINTERS_EQUAL(NULL, irc_batch_search (server, ""));
    POINTERS_EQUAL(NULL, irc_batch_search (server, "does_not_exist"));
    POINTERS_EQUAL(NULL, irc_batch_search (server, "REF1"));
    POINTERS_EQUAL(NULL, irc_batch_search (server, "REF2"));

    POINTERS_EQUAL(batch1, irc_batch_search (server, "ref1"));
    POINTERS_EQUAL(batch2, irc_batch_search (server, "ref2"));

    irc_batch_end_batch (server, "ref1");
    irc_batch_end_batch (server, "ref2");

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_batch_generate_random_ref
 */

TEST(IrcBatch, GenerateRandomRef)
{
    char str[16 + 1];

    strcpy (str, "ABC");
    irc_batch_generate_random_ref (NULL, 8);
    irc_batch_generate_random_ref (str, -1);
    STRCMP_EQUAL("ABC", str);

    strcpy (str, "ABC");
    irc_batch_generate_random_ref (str, 0);
    LONGS_EQUAL(0, strlen (str));
    str[0] = '\0';
    irc_batch_generate_random_ref (str, 8);
    LONGS_EQUAL(8, strlen (str));
    str[0] = '\0';
    irc_batch_generate_random_ref (str, 16);
    LONGS_EQUAL(16, strlen (str));
}

/*
 * Tests functions:
 *   irc_batch_add_to_list
 *   irc_batch_start_batch
 *   irc_batch_free
 */

TEST(IrcBatch, StartBatch)
{
    struct t_irc_server *server;
    struct t_irc_batch *batch;

    server = irc_server_alloc ("server");
    CHECK(server);

    POINTERS_EQUAL(NULL, server->batches);

    batch = irc_batch_start_batch (server, "ref", NULL, "type", NULL);
    CHECK(batch);
    POINTERS_EQUAL(batch, server->batches);
    STRCMP_EQUAL("ref", batch->reference);
    POINTERS_EQUAL(NULL, batch->parent_ref);
    STRCMP_EQUAL("type", batch->type);
    POINTERS_EQUAL(NULL, batch->parameters);
    CHECK(batch->start_time > 0);
    POINTERS_EQUAL(NULL, batch->messages);
    LONGS_EQUAL(0, batch->end_received);
    LONGS_EQUAL(0, batch->messages_processed);
    irc_batch_free (server, batch);

    POINTERS_EQUAL(NULL, server->batches);

    batch = irc_batch_start_batch (server, "ref", "parent_ref", "type", "params");
    CHECK(batch);
    POINTERS_EQUAL(batch, server->batches);
    STRCMP_EQUAL("ref", batch->reference);
    STRCMP_EQUAL("parent_ref", batch->parent_ref);
    STRCMP_EQUAL("type", batch->type);
    STRCMP_EQUAL("params", batch->parameters);
    CHECK(batch->start_time > 0);
    POINTERS_EQUAL(NULL, batch->messages);

    LONGS_EQUAL(0, batch->end_received);
    LONGS_EQUAL(0, batch->messages_processed);
    irc_batch_free (server, batch);

    POINTERS_EQUAL(NULL, server->batches);

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_batch_add_message
 *   irc_batch_free
 */

TEST(IrcBatch, AddMessage)
{
    struct t_irc_server *server;
    struct t_irc_batch *batch;

    server = irc_server_alloc ("server");
    CHECK(server);

    batch = irc_batch_start_batch (server, "ref", "parent_ref", "type", "params");
    CHECK(batch);

    irc_batch_add_message (server, "ref", ":alice PRIVMSG #test: test1");
    STRCMP_EQUAL(*batch->messages, ":alice PRIVMSG #test: test1");
    irc_batch_add_message (server, "ref", ":alice PRIVMSG #test: test2");
    STRCMP_EQUAL(*batch->messages,
                 ":alice PRIVMSG #test: test1\n"
                 ":alice PRIVMSG #test: test2");

    irc_batch_free (server, batch);

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_batch_free_all
 */

TEST(IrcBatch, FreeAll)
{
    struct t_irc_server *server;
    struct t_irc_batch *batch1, *batch2;

    server = irc_server_alloc ("server");
    CHECK(server);

    batch1 = irc_batch_start_batch (server, "ref1", "parent_ref", "type", "params");
    CHECK(batch1);

    batch2 = irc_batch_start_batch (server, "ref2", "parent_ref", "type", "params");
    CHECK(batch2);

    POINTERS_EQUAL(batch1, server->batches);
    POINTERS_EQUAL(batch2, server->batches->next_batch);

    irc_batch_free_all (server);

    POINTERS_EQUAL(NULL, server->batches);

    irc_server_free (server);
}

/*
 * Tests functions:
 *   irc_batch_process_messages
 */

TEST(IrcBatch, ProcessMessages)
{
    /* tested in test-irc-protocol.cpp */
}

/*
 * Tests functions:
 *   irc_batch_end_batch
 */

TEST(IrcBatch, EndBatch)
{
    /* tested in test-irc-protocol.cpp */
}

/*
 * Tests functions:
 *   irc_batch_process_multiline
 */

TEST(IrcBatch, ProcessMultiline)
{
    /* tested in test-irc-protocol.cpp */
}

/*
 * Tests functions:
 *   irc_batch_hdata_batch_cb
 */

TEST(IrcBatch, HdataBatchCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_batch_print_log
 */

TEST(IrcBatch, PrintLog)
{
    /* TODO: write tests */
}
