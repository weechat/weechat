/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test xfer functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <string.h>
#include "src/core/core-infolist.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/xfer/xfer.h"

extern struct t_xfer *xfer_search (const char *plugin_name,
                                   const char *plugin_id,
                                   enum t_xfer_type type,
                                   enum t_xfer_status status,
                                   int port);
extern struct t_xfer *xfer_search_resume (const char *plugin_name,
                                          const char *plugin_id,
                                          enum t_xfer_type type,
                                          int port,
                                          const char *token);
extern int xfer_start_resume_cb (const void *pointer, void *data,
                                 const char *signal, const char *type_data,
                                 void *signal_data);
extern int xfer_accept_resume_cb (const void *pointer, void *data,
                                  const char *signal, const char *type_data,
                                  void *signal_data);
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
 * Test functions:
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
 * Test functions:
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

TEST_GROUP(XferResume)
{
    struct t_xfer *saved_xfer_list, *saved_last_xfer;
    struct t_xfer xfer_recv_active, xfer_recv_passive, xfer_send;

    /*
     * Build a list with three xfers, used by the tests:
     *   - a file received (active), waiting for DCC ACCEPT
     *   - a file received (passive, with token), waiting for DCC ACCEPT
     *   - a file sent, waiting for the connection of the receiver.
     */

    void init_xfer (struct t_xfer *xfer, enum t_xfer_type type, int port,
                    unsigned long long size, unsigned long long start_resume)
    {
        memset (xfer, 0, sizeof (*xfer));
        xfer->plugin_name = (char *)"irc";
        xfer->plugin_id = (char *)"libera";
        xfer->type = type;
        xfer->status = XFER_STATUS_CONNECTING;
        xfer->remote_nick = (char *)"bob";
        xfer->filename = (char *)"file.txt";
        xfer->remote_address_str = (char *)"127.0.0.1";
        xfer->port = port;
        xfer->size = size;
        xfer->start_resume = start_resume;
        xfer->pos = start_resume;
        xfer->ack = 0;
        xfer->sock = -1;
        xfer->file = -1;
        xfer->child_read = -1;
        xfer->child_write = -1;
    }

    void setup ()
    {
        saved_xfer_list = xfer_list;
        saved_last_xfer = last_xfer;

        init_xfer (&xfer_recv_active, XFER_TYPE_FILE_RECV_ACTIVE,
                   5678, 5000, 1000);
        init_xfer (&xfer_recv_passive, XFER_TYPE_FILE_RECV_PASSIVE,
                   0, 5000, 2000);
        xfer_recv_passive.token = (char *)"42";
        init_xfer (&xfer_send, XFER_TYPE_FILE_SEND_PASSIVE,
                   1234, 5000, 0);

        xfer_recv_active.next_xfer = &xfer_recv_passive;
        xfer_recv_passive.prev_xfer = &xfer_recv_active;
        xfer_recv_passive.next_xfer = &xfer_send;
        xfer_send.prev_xfer = &xfer_recv_passive;

        xfer_list = &xfer_recv_active;
        last_xfer = &xfer_send;
    }

    void teardown ()
    {
        xfer_list = saved_xfer_list;
        last_xfer = saved_last_xfer;
    }

    /*
     * Build an infolist like the one sent by IRC plugin in signals
     * "xfer_start_resume" and "xfer_accept_resume".
     */

    struct t_infolist *build_infolist (const char *filename, int port,
                                       const char *start_resume,
                                       const char *token)
    {
        struct t_infolist *infolist;
        struct t_infolist_item *item;

        infolist = infolist_new (NULL);
        item = infolist_new_item (infolist);
        infolist_new_var_string (item, "plugin_name", "irc");
        infolist_new_var_string (item, "plugin_id", "libera");
        infolist_new_var_string (item, "filename", filename);
        infolist_new_var_integer (item, "port", port);
        infolist_new_var_string (item, "start_resume", start_resume);
        if (token)
            infolist_new_var_string (item, "token", token);
        return infolist;
    }

    int start_resume (const char *filename, int port,
                      const char *start_resume, const char *token)
    {
        struct t_infolist *infolist;
        int rc;

        infolist = build_infolist (filename, port, start_resume, token);
        rc = xfer_start_resume_cb (NULL, NULL, "xfer_start_resume",
                                   WEECHAT_HOOK_SIGNAL_POINTER, infolist);
        infolist_free (infolist);
        return rc;
    }

    int accept_resume (const char *filename, int port,
                       const char *start_resume)
    {
        struct t_infolist *infolist;
        int rc;

        infolist = build_infolist (filename, port, start_resume, NULL);
        rc = xfer_accept_resume_cb (NULL, NULL, "xfer_accept_resume",
                                    WEECHAT_HOOK_SIGNAL_POINTER, infolist);
        infolist_free (infolist);
        return rc;
    }
};

/*
 * Test functions:
 *   xfer_search_resume
 */

TEST(XferResume, SearchResume)
{
    /* Invalid arguments */
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume (NULL, NULL, XFER_TYPE_FILE_RECV_ACTIVE, 5678, NULL));
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume (NULL, "libera", XFER_TYPE_FILE_RECV_ACTIVE, 5678,
                            NULL));
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", NULL, XFER_TYPE_FILE_RECV_ACTIVE, 5678,
                            NULL));

    /* Plugin name, plugin id, type and port are compared. */
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("IRC", "libera", XFER_TYPE_FILE_RECV_ACTIVE, 5678,
                            NULL));
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", "oftc", XFER_TYPE_FILE_RECV_ACTIVE, 5678,
                            NULL));
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_SEND_PASSIVE, 1234,
                            NULL));
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_RECV_ACTIVE, 4321,
                            NULL));

    /* Token is compared. */
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_RECV_ACTIVE, 5678,
                            "42"));
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_RECV_PASSIVE, 0,
                            NULL));
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_RECV_PASSIVE, 0,
                            "43"));

    /* Xfer found */
    POINTERS_EQUAL(
        &xfer_recv_active,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_RECV_ACTIVE, 5678,
                            NULL));
    POINTERS_EQUAL(
        &xfer_recv_passive,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_RECV_PASSIVE, 0,
                            "42"));

    /* Xfer must be connecting. */
    xfer_recv_active.status = XFER_STATUS_WAITING;
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_RECV_ACTIVE, 5678,
                            NULL));
    xfer_recv_active.status = XFER_STATUS_CONNECTING;

    /* Xfer must be waiting for a resume. */
    xfer_recv_active.start_resume = 0;
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_RECV_ACTIVE, 5678,
                            NULL));
    xfer_recv_active.start_resume = 5000;
    POINTERS_EQUAL(
        NULL,
        xfer_search_resume ("irc", "libera", XFER_TYPE_FILE_RECV_ACTIVE, 5678,
                            NULL));
}

/*
 * Test functions:
 *   xfer_start_resume_cb
 */

TEST(XferResume, StartResumeCb)
{
    struct t_infolist *infolist;

    /* Invalid arguments */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                xfer_start_resume_cb (NULL, NULL, "xfer_start_resume",
                                      WEECHAT_HOOK_SIGNAL_POINTER, NULL));
    infolist = infolist_new (NULL);
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                xfer_start_resume_cb (NULL, NULL, "xfer_start_resume",
                                      WEECHAT_HOOK_SIGNAL_POINTER, infolist));
    infolist_free (infolist);
    LONGS_EQUAL(WEECHAT_RC_ERROR, start_resume (NULL, 5678, "1000", NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR, start_resume ("file.txt", 5678, NULL, NULL));

    /* Invalid resume position */
    LONGS_EQUAL(WEECHAT_RC_ERROR, start_resume ("file.txt", 5678, "", NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR, start_resume ("file.txt", 5678, "abc", NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                start_resume ("file.txt", 5678, "1000abc", NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR, start_resume ("file.txt", 5678, "-1", NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                start_resume ("file.txt", 5678,
                              "99999999999999999999999", NULL));
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer_recv_active.status);

    /* Xfer not found */
    LONGS_EQUAL(WEECHAT_RC_OK, start_resume ("file.txt", 4321, "1000", NULL));
    LONGS_EQUAL(WEECHAT_RC_OK, start_resume ("file.txt", 0, "2000", "43"));
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer_recv_active.status);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer_recv_passive.status);

    /* Unexpected resume position: the xfer fails. */
    LONGS_EQUAL(WEECHAT_RC_OK, start_resume ("file.txt", 5678, "5000", NULL));
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer_recv_active.status);
    CHECK(xfer_recv_active.start_resume == 1000);
    CHECK(xfer_recv_active.pos == 1000);
    CHECK(xfer_recv_active.ack == 0);
    LONGS_EQUAL(WEECHAT_RC_OK, start_resume ("file.ext", 0, "1999", "42"));
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer_recv_passive.status);
    CHECK(xfer_recv_passive.start_resume == 2000);
    CHECK(xfer_recv_passive.pos == 2000);
    CHECK(xfer_recv_passive.ack == 0);
}

/*
 * Test functions:
 *   xfer_accept_resume_cb
 */

TEST(XferResume, AcceptResumeCb)
{
    struct t_infolist *infolist;

    /* Invalid arguments */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                xfer_accept_resume_cb (NULL, NULL, "xfer_accept_resume",
                                       WEECHAT_HOOK_SIGNAL_POINTER, NULL));
    infolist = infolist_new (NULL);
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                xfer_accept_resume_cb (NULL, NULL, "xfer_accept_resume",
                                       WEECHAT_HOOK_SIGNAL_POINTER, infolist));
    infolist_free (infolist);
    LONGS_EQUAL(WEECHAT_RC_ERROR, accept_resume (NULL, 1234, "1000"));
    LONGS_EQUAL(WEECHAT_RC_ERROR, accept_resume ("file.txt", 1234, NULL));

    /* Invalid resume position */
    LONGS_EQUAL(WEECHAT_RC_ERROR, accept_resume ("file.txt", 1234, ""));
    LONGS_EQUAL(WEECHAT_RC_ERROR, accept_resume ("file.txt", 1234, "abc"));
    LONGS_EQUAL(WEECHAT_RC_ERROR, accept_resume ("file.txt", 1234, "1000abc"));
    LONGS_EQUAL(WEECHAT_RC_ERROR, accept_resume ("file.txt", 1234, "-1"));
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                accept_resume ("file.txt", 1234, "99999999999999999999999"));

    /* Resume position not lower than the file size */
    LONGS_EQUAL(WEECHAT_RC_ERROR, accept_resume ("file.txt", 1234, "5000"));
    LONGS_EQUAL(WEECHAT_RC_ERROR, accept_resume ("file.txt", 1234, "6000"));

    /* Xfer not found */
    LONGS_EQUAL(WEECHAT_RC_OK, accept_resume ("file.txt", 4321, "1000"));

    /* The xfer must not be changed. */
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer_send.status);
    CHECK(xfer_send.start_resume == 0);
    CHECK(xfer_send.pos == 0);
    CHECK(xfer_send.ack == 0);
}
