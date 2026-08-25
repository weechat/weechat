/*
 * test-xfer.cpp - test xfer functions
 *
 * Copyright (C) 2026 Sébastien Helleu <flashcode@flashtux.org>
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

extern "C"
{
#include <string.h>
#include "src/plugins/xfer/xfer.h"

extern struct t_xfer *xfer_search (const char *plugin_name,
                                   const char *plugin_id,
                                   enum t_xfer_type type,
                                   enum t_xfer_status status,
                                   int port);
}

TEST_GROUP(Xfer)
{
    struct t_xfer *saved_xfer_list, *saved_last_xfer;
    struct t_xfer xfer_recv_waiting, xfer_recv_connecting, xfer_send;

    /*
     * Build a list with three xfers, used by the tests:
     *   - a file received, waiting for the manual acceptance by the user
     *   - a file received, accepted by the user
     *   - a file sent
     */

    void setup ()
    {
        saved_xfer_list = xfer_list;
        saved_last_xfer = last_xfer;

        memset (&xfer_recv_waiting, 0, sizeof (xfer_recv_waiting));
        xfer_recv_waiting.plugin_name = (char *)"irc";
        xfer_recv_waiting.plugin_id = (char *)"libera";
        xfer_recv_waiting.type = XFER_TYPE_FILE_RECV_ACTIVE;
        xfer_recv_waiting.status = XFER_STATUS_WAITING;
        xfer_recv_waiting.port = 1234;

        memset (&xfer_recv_connecting, 0, sizeof (xfer_recv_connecting));
        xfer_recv_connecting.plugin_name = (char *)"irc";
        xfer_recv_connecting.plugin_id = (char *)"libera";
        xfer_recv_connecting.type = XFER_TYPE_FILE_RECV_ACTIVE;
        xfer_recv_connecting.status = XFER_STATUS_CONNECTING;
        xfer_recv_connecting.port = 5678;

        memset (&xfer_send, 0, sizeof (xfer_send));
        xfer_send.plugin_name = (char *)"irc";
        xfer_send.plugin_id = (char *)"oftc";
        xfer_send.type = XFER_TYPE_FILE_SEND_PASSIVE;
        xfer_send.status = XFER_STATUS_CONNECTING;
        xfer_send.port = 1234;

        xfer_recv_waiting.next_xfer = &xfer_recv_connecting;
        xfer_recv_connecting.prev_xfer = &xfer_recv_waiting;
        xfer_recv_connecting.next_xfer = &xfer_send;
        xfer_send.prev_xfer = &xfer_recv_connecting;

        xfer_list = &xfer_recv_waiting;
        last_xfer = &xfer_send;
    }

    void teardown ()
    {
        xfer_list = saved_xfer_list;
        last_xfer = saved_last_xfer;
    }
};

/*
 * Tests functions:
 *   xfer_search
 */

TEST(Xfer, Search)
{
    /* invalid arguments */
    POINTERS_EQUAL(
        NULL,
        xfer_search (NULL, NULL,
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_WAITING, 1234));
    POINTERS_EQUAL(
        NULL,
        xfer_search (NULL, "libera",
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_WAITING, 1234));
    POINTERS_EQUAL(
        NULL,
        xfer_search ("irc", NULL,
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_WAITING, 1234));

    /* plugin name, plugin id, type and port are compared */
    POINTERS_EQUAL(
        NULL,
        xfer_search ("IRC", "libera",
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_WAITING, 1234));
    POINTERS_EQUAL(
        NULL,
        xfer_search ("irc", "oftc",
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_WAITING, 1234));
    POINTERS_EQUAL(
        NULL,
        xfer_search ("irc", "libera",
                     XFER_TYPE_CHAT_RECV, XFER_STATUS_WAITING, 1234));
    POINTERS_EQUAL(
        NULL,
        xfer_search ("irc", "libera",
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_WAITING, 4321));

    /* xfer found */
    POINTERS_EQUAL(
        &xfer_recv_waiting,
        xfer_search ("irc", "libera",
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_WAITING, 1234));
    POINTERS_EQUAL(
        &xfer_recv_connecting,
        xfer_search ("irc", "libera",
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_CONNECTING, 5678));
    POINTERS_EQUAL(
        &xfer_send,
        xfer_search ("irc", "oftc",
                     XFER_TYPE_FILE_SEND_PASSIVE, XFER_STATUS_CONNECTING, 1234));
}

/*
 * Tests functions:
 *   xfer_search (status is compared and left unchanged)
 */

TEST(Xfer, SearchStatus)
{
    /*
     * the status must be compared: a transfer waiting for the manual
     * acceptance by the user must not be returned when a connecting transfer
     * is searched
     */
    POINTERS_EQUAL(
        NULL,
        xfer_search ("irc", "libera",
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_CONNECTING, 1234));
    POINTERS_EQUAL(
        NULL,
        xfer_search ("irc", "libera",
                     XFER_TYPE_FILE_RECV_ACTIVE, XFER_STATUS_ACTIVE, 5678));
    POINTERS_EQUAL(
        NULL,
        xfer_search ("irc", "oftc",
                     XFER_TYPE_FILE_SEND_PASSIVE, XFER_STATUS_WAITING, 1234));

    /* the status of xfers must not be changed by the search */
    LONGS_EQUAL(XFER_STATUS_WAITING, xfer_recv_waiting.status);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer_recv_connecting.status);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer_send.status);
}
