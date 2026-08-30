/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test xfer chat functions */

#include "CppUTest/TestHarness.h"

#include "tests-record.h"

extern "C"
{
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/xfer/xfer.h"
#include "src/plugins/xfer/xfer-chat.h"
}

TEST_GROUP(XferChat)
{
    struct t_xfer xfer;
    int sock[2];

    void setup ()
    {
        memset (&xfer, 0, sizeof (xfer));
        xfer.type = XFER_TYPE_CHAT_RECV;
        xfer.status = XFER_STATUS_ACTIVE;
        xfer.remote_nick = strdup ("alice");
        xfer.local_nick = strdup ("bob");

        sock[0] = -1;
        sock[1] = -1;
        LONGS_EQUAL(0, socketpair (AF_UNIX, SOCK_STREAM, 0, sock));
        xfer.sock = sock[1];

        /* Messages are displayed on core buffer (xfer.buffer is NULL). */
        record_start ();
    }

    void teardown ()
    {
        record_stop ();

        free (xfer.remote_nick);
        free (xfer.local_nick);
        free (xfer.unterminated_message);

        if (sock[0] >= 0)
            close (sock[0]);
        if (xfer.sock >= 0)
            close (xfer.sock);
    }

    /*
     * Set the partial message to a string of "size" bytes.
     *
     * Note: the partial message is set directly instead of being accumulated
     * with many calls to the receive callback, because a single call reads at
     * most 4096 bytes from the socket.
     */

    void set_partial_message (size_t size)
    {
        free (xfer.unterminated_message);
        xfer.unterminated_message = (char *)malloc (size + 1);
        CHECK(xfer.unterminated_message);
        memset (xfer.unterminated_message, 'a', size);
        xfer.unterminated_message[size] = '\0';
    }

    /* Send data to the remote host socket and run the receive callback. */

    void recv_data (const char *data)
    {
        size_t size;

        size = strlen (data);
        LONGS_EQUAL(size, write (sock[0], data, size));
        LONGS_EQUAL(WEECHAT_RC_OK,
                    xfer_chat_recv_cb (&xfer, NULL, xfer.sock));
    }

    /* Send "size" bytes without any end-of-line and run the callback. */

    void recv_bytes (size_t size)
    {
        char *data;

        data = (char *)malloc (size + 1);
        CHECK(data);
        memset (data, 'b', size);
        data[size] = '\0';
        recv_data (data);
        free (data);
    }

    size_t partial_message_length ()
    {
        return (xfer.unterminated_message) ?
            strlen (xfer.unterminated_message) : 0;
    }
};

/*
 * Test functions:
 *   xfer_chat_recv_cb
 */

TEST(XferChat, RecvCb)
{
    /* Data without any end-of-line is kept as partial message. */
    recv_data ("hello");
    STRCMP_EQUAL("hello", xfer.unterminated_message);

    /* Data without any end-of-line is appended to the partial message. */
    recv_data (" world");
    STRCMP_EQUAL("hello world", xfer.unterminated_message);

    /* Data ending with end-of-line: the message is displayed, nothing kept. */
    recv_data ("!\r\n");
    POINTERS_EQUAL(NULL, xfer.unterminated_message);
    CHECK(record_search ("core.weechat", "alice", "hello world!", NULL));

    /* Complete line then partial data: the remaining data is kept. */
    recv_data ("line1\nline2\npartial");
    STRCMP_EQUAL("partial", xfer.unterminated_message);
    CHECK(record_search ("core.weechat", "alice", "line1", NULL));
    CHECK(record_search ("core.weechat", "alice", "line2", NULL));
}

/*
 * Test functions:
 *   xfer_chat_recv_cb (the partial message accumulated is bounded)
 *
 * Check that data received without any end-of-line does not grow the partial
 * message without limit, that the chat is not closed when the limit is
 * reached, and that data received afterwards is accumulated again.
 */

TEST(XferChat, RecvCbLimit)
{
    /* 100 bytes are missing to reach the limit: 50 bytes are accumulated. */
    set_partial_message (XFER_CHAT_PARTIAL_MESSAGE_MAX_LENGTH - 100);
    recv_bytes (50);
    LONGS_EQUAL(XFER_CHAT_PARTIAL_MESSAGE_MAX_LENGTH - 50,
                partial_message_length ());

    /* The next 50 bytes fit exactly in the limit. */
    recv_bytes (50);
    LONGS_EQUAL(XFER_CHAT_PARTIAL_MESSAGE_MAX_LENGTH,
                partial_message_length ());

    /* The partial message is full: it is discarded with the data received. */
    recv_bytes (1);
    POINTERS_EQUAL(NULL, xfer.unterminated_message);

    /* The chat must still be open. */
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer.status);
    CHECK(xfer.sock >= 0);

    /* Data received after the limit is accumulated again. */
    recv_data ("hello");
    STRCMP_EQUAL("hello", xfer.unterminated_message);

    /* Data not fitting in the partial message is discarded with it. */
    set_partial_message (XFER_CHAT_PARTIAL_MESSAGE_MAX_LENGTH - 10);
    recv_bytes (20);
    POINTERS_EQUAL(NULL, xfer.unterminated_message);
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer.status);
    CHECK(xfer.sock >= 0);

    /* The chat is still usable: a complete message is displayed. */
    recv_data ("hello world\n");
    POINTERS_EQUAL(NULL, xfer.unterminated_message);
    CHECK(record_search ("core.weechat", "alice", "hello world", NULL));
}
