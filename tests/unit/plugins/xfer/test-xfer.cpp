/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test xfer functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-signal.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/xfer/xfer.h"
#include "src/plugins/xfer/xfer-config.h"
#include "src/plugins/xfer/xfer-network.h"

extern int xfer_add_cb (const void *pointer, void *data,
                        const char *signal, const char *type_data,
                        void *signal_data);
extern int xfer_network_timer_cb (const void *pointer, void *data,
                                  int remaining_calls);

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

TEST_GROUP(XferAdd)
{
    struct t_xfer *saved_xfer_list;
    int sock_server;
    char *filename, *rel_filename;
    int count_signals, count_resume;
    struct t_hook *hook_signals, *hook_resume;
    char *temp_resume_filename;

    static int signal_cb (const void *pointer, void *data,
                          const char *signal, const char *type_data,
                          void *signal_data)
    {
        (void) data;
        (void) signal;
        (void) type_data;
        (void) signal_data;

        (*((int *)pointer))++;

        return WEECHAT_RC_OK;
    }

    void setup ()
    {
        struct sockaddr_in addr;

        saved_xfer_list = xfer_list;

        config_file_option_set (xfer_config_look_auto_open_buffer, "off", 1);

        /* Socket of IRC server (used to find the local address). */
        sock_server = socket (AF_INET, SOCK_STREAM, 0);
        CHECK(sock_server >= 0);
        memset (&addr, 0, sizeof (addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
        LONGS_EQUAL(0, bind (sock_server, (struct sockaddr *)&addr,
                             sizeof (addr)));

        /* File to send (absolute path). */
        rel_filename = string_eval_path_home (
            "${weechat_data_dir}/test_xfer_upload.txt", NULL, NULL, NULL);
        CHECK(rel_filename);
        if (rel_filename[0] == '/')
        {
            filename = strdup (rel_filename);
        }
        else
        {
            char cwd[4096];
            CHECK(getcwd (cwd, sizeof (cwd)));
            string_asprintf (&filename, "%s/%s", cwd, rel_filename);
        }
        unlink (filename);

        count_signals = 0;
        hook_signals = hook_signal (NULL, "xfer_send_ready", &signal_cb,
                                    &count_signals, NULL);
        count_resume = 0;
        hook_resume = hook_signal (NULL, "xfer_resume_ready", &signal_cb,
                                   &count_resume, NULL);

        temp_resume_filename = NULL;

        record_start ();
    }

    void teardown ()
    {
        struct t_xfer *ptr_xfer;

        record_stop ();

        unhook (hook_signals);
        unhook (hook_resume);

        if (temp_resume_filename)
        {
            unlink (temp_resume_filename);
            free (temp_resume_filename);
        }

        /* Free xfers created by the test (added at the beginning). */
        while (xfer_list && (xfer_list != saved_xfer_list))
        {
            ptr_xfer = xfer_list;
            if (ptr_xfer->sock >= 0)
                close (ptr_xfer->sock);
            xfer_free (ptr_xfer);
        }

        close (sock_server);

        unlink (filename);
        free (filename);
        free (rel_filename);

        config_file_option_reset (xfer_config_look_auto_open_buffer, 1);
        config_file_option_reset (xfer_config_file_auto_accept_nicks, 1);
        config_file_option_reset (xfer_config_file_auto_check_crc32, 1);
        config_file_option_reset (xfer_config_file_convert_spaces, 1);
        config_file_option_reset (xfer_config_file_upload_path, 1);
        config_file_option_reset (xfer_config_network_own_ip, 1);
        config_file_option_reset (xfer_config_network_port_range, 1);
    }

    /*
     * Send signal "xfer_add" with an infolist built with the given
     * key/value pairs (ending with NULL); keys "port" and "socket" are
     * integers.
     */
    int add_xfer (const char *key, ...)
    {
        struct t_infolist *infolist;
        struct t_infolist_item *item;
        const char *ptr_key, *ptr_value;
        va_list args;
        int rc;

        infolist = infolist_new (NULL);
        item = infolist_new_item (infolist);
        va_start (args, key);
        for (ptr_key = key; ptr_key; ptr_key = va_arg (args, const char *))
        {
            ptr_value = va_arg (args, const char *);
            if ((strcmp (ptr_key, "port") == 0)
                || (strcmp (ptr_key, "socket") == 0))
            {
                infolist_new_var_integer (item, ptr_key, atoi (ptr_value));
            }
            else
            {
                infolist_new_var_string (item, ptr_key, ptr_value);
            }
        }
        va_end (args);
        rc = xfer_add_cb (NULL, NULL, "xfer_add",
                          WEECHAT_HOOK_SIGNAL_POINTER, infolist);
        infolist_free (infolist);
        return rc;
    }

    /* Return the server socket as string. */
    const char *str_sock_server ()
    {
        static char str_sock[32];

        snprintf (str_sock, sizeof (str_sock), "%d", sock_server);
        return str_sock;
    }

    /* Create a file with content. */
    void create_file (const char *path, const char *content)
    {
        FILE *file;

        file = fopen (path, "w");
        CHECK(file);
        fputs (content, file);
        fclose (file);
    }

    /* Return a free TCP port. */
    int free_port ()
    {
        struct sockaddr_in addr;
        socklen_t length;
        int sock, port;

        sock = socket (AF_INET, SOCK_STREAM, 0);
        memset (&addr, 0, sizeof (addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        bind (sock, (struct sockaddr *)&addr, sizeof (addr));
        length = sizeof (addr);
        getsockname (sock, (struct sockaddr *)&addr, &length);
        port = ntohs (addr.sin_port);
        close (sock);
        return port;
    }
};

/*
 * Test functions:
 *   xfer_add_cb (file received, passive)
 *   xfer_new
 */

TEST(XferAdd, AddCbFileRecvPassive)
{
    struct t_xfer *ptr_xfer;

    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test.txt", "size", "10",
                          "remote_address", "127.0.0.1", "port", "5000",
                          "socket", str_sock_server (), "token", "42",
                          NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(XFER_TYPE_FILE_RECV_PASSIVE, ptr_xfer->type);
    LONGS_EQUAL(XFER_STATUS_WAITING, ptr_xfer->status);
    STRCMP_EQUAL("127.0.0.1", ptr_xfer->remote_address_str);
    STRCMP_EQUAL("127.0.0.1", ptr_xfer->local_address_str);
    STRCMP_EQUAL("42", ptr_xfer->token);
    LONGS_EQUAL(-1, ptr_xfer->sock);
    LONGS_EQUAL(0, ptr_xfer->port);
    LONGS_EQUAL(0, count_signals);

    /* Nick auto-accepted: xfer accepted, listening socket. */
    config_file_option_set (xfer_config_file_auto_accept_nicks,
                            "libera.alice", 1);
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test2.txt", "size", "10",
                          "remote_address", "127.0.0.1",
                          "socket", str_sock_server (), "token", "43",
                          NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(XFER_STATUS_CONNECTING, ptr_xfer->status);
    CHECK(ptr_xfer->hook_fd);
    CHECK(ptr_xfer->hook_timer);
    LONGS_EQUAL(1, count_signals);
}

/*
 * Test functions:
 *   xfer_add_cb (invalid port range)
 *   xfer_network_bind_socket (invalid port range)
 */

TEST(XferAdd, AddCbPortRangeInvalid)
{
    const char *ranges[] = { "0", "0-10", "65536", "65535-65540",
                             "70000-2147483647", "5010-5000", NULL };
    int i;

    for (i = 0; ranges[i]; i++)
    {
        config_file_option_set (xfer_config_network_port_range, ranges[i], 1);
        record_start ();
        LONGS_EQUAL(WEECHAT_RC_ERROR,
                    add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                              "type_string", "chat_send",
                              "remote_nick", "alice", "local_nick", "bob",
                              "socket", str_sock_server (), NULL));
        CHECK(record_search_msg_regex ("core.weechat",
                                       "cannot find available port for xfer"));
    }
}

/*
 * Test functions:
 *   xfer_network_accept (file received, passive, with port range)
 *   xfer_network_bind_socket
 */

TEST(XferAdd, AcceptFileRecvPassivePortRange)
{
    struct t_xfer *ptr_xfer, *ptr_xfer2;
    char str_range[64];
    int port;

    port = free_port ();
    CHECK(port > 0);

    snprintf (str_range, sizeof (str_range), "%d", port);
    config_file_option_set (xfer_config_network_port_range, str_range, 1);

    /* Offer pending: no socket, port not reserved. */
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test.txt", "size", "10",
                          "remote_address", "127.0.0.1",
                          "socket", str_sock_server (), "token", "42",
                          NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(XFER_STATUS_WAITING, ptr_xfer->status);
    LONGS_EQUAL(-1, ptr_xfer->sock);
    LONGS_EQUAL(0, ptr_xfer->port);
    LONGS_EQUAL(0, xfer_port_in_use (port));

    /* Second offer, also pending. */
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test2.txt", "size", "10",
                          "remote_address", "127.0.0.1",
                          "socket", str_sock_server (), "token", "43",
                          NULL));
    ptr_xfer2 = xfer_list;
    LONGS_EQUAL(-1, ptr_xfer2->sock);
    LONGS_EQUAL(0, ptr_xfer2->port);

    /* Accept first offer: socket bound on the port of the range. */
    xfer_network_accept (ptr_xfer);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, ptr_xfer->status);
    CHECK(ptr_xfer->sock >= 0);
    LONGS_EQUAL(port, ptr_xfer->port);
    CHECK(ptr_xfer->hook_fd);
    LONGS_EQUAL(1, xfer_port_in_use (port));
    LONGS_EQUAL(1, count_signals);

    /* Accept second offer: no port available in the range. */
    xfer_network_accept (ptr_xfer2);
    LONGS_EQUAL(XFER_STATUS_FAILED, ptr_xfer2->status);
    LONGS_EQUAL(-1, ptr_xfer2->sock);
    LONGS_EQUAL(0, ptr_xfer2->port);
    CHECK(record_search_msg_regex ("core.weechat",
                                   "cannot find available port for xfer"));
    LONGS_EQUAL(1, count_signals);
}

/*
 * Test functions:
 *   xfer_network_accept (file received, passive, resumed, with port range)
 *   xfer_start_resume_cb (file received, passive)
 */

TEST(XferAdd, AcceptFileRecvPassiveResumePortRange)
{
    struct t_xfer *ptr_xfer;
    struct t_infolist *infolist;
    struct t_infolist_item *item;
    char str_range[64], *local_filename;
    int port, sock;

    port = free_port ();
    CHECK(port > 0);

    snprintf (str_range, sizeof (str_range), "%d", port);
    config_file_option_set (xfer_config_network_port_range, str_range, 1);

    /* Partial file already received (temporary file, removed in teardown). */
    local_filename = string_eval_path_home (
        "${weechat_data_dir}/xfer/alice.resume_passive.txt", NULL, NULL, NULL);
    string_asprintf (&temp_resume_filename, "%s.part", local_filename);
    free (local_filename);
    create_file (temp_resume_filename, "0123456789");

    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "resume_passive.txt", "size", "100",
                          "remote_address", "127.0.0.1",
                          "socket", str_sock_server (), "token", "44",
                          NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(10, ptr_xfer->start_resume);
    LONGS_EQUAL(-1, ptr_xfer->sock);
    LONGS_EQUAL(0, ptr_xfer->port);

    /*
     * Accept: socket bound on the port of the range before DCC RESUME is
     * sent (with this port), but not listening yet.
     */
    xfer_network_accept (ptr_xfer);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, ptr_xfer->status);
    CHECK(ptr_xfer->sock >= 0);
    LONGS_EQUAL(port, ptr_xfer->port);
    POINTERS_EQUAL(NULL, ptr_xfer->hook_fd);
    CHECK(ptr_xfer->hook_timer);
    LONGS_EQUAL(1, count_resume);
    LONGS_EQUAL(0, count_signals);
    POINTERS_EQUAL(ptr_xfer,
                   xfer_search_resume ("irc", "libera",
                                       XFER_TYPE_FILE_RECV_PASSIVE,
                                       port, "44"));

    /* DCC ACCEPT received: same socket, now listening. */
    sock = ptr_xfer->sock;
    infolist = infolist_new (NULL);
    item = infolist_new_item (infolist);
    infolist_new_var_string (item, "plugin_name", "irc");
    infolist_new_var_string (item, "plugin_id", "libera");
    infolist_new_var_string (item, "filename", "resume_passive.txt");
    infolist_new_var_integer (item, "port", port);
    infolist_new_var_string (item, "start_resume", "10");
    infolist_new_var_string (item, "token", "44");
    LONGS_EQUAL(WEECHAT_RC_OK,
                xfer_start_resume_cb (NULL, NULL, "xfer_start_resume",
                                      WEECHAT_HOOK_SIGNAL_POINTER, infolist));
    infolist_free (infolist);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, ptr_xfer->status);
    LONGS_EQUAL(sock, ptr_xfer->sock);
    LONGS_EQUAL(port, ptr_xfer->port);
    CHECK(ptr_xfer->hook_fd);
    CHECK(ptr_xfer->hook_timer);
    LONGS_EQUAL(1, count_signals);
}

/*
 * Test functions:
 *   xfer_network_accept (file received, passive, resumed, timeout)
 *   xfer_network_timer_cb
 */

TEST(XferAdd, AcceptFileRecvPassiveResumeTimeout)
{
    struct t_xfer *ptr_xfer;
    char str_range[64], *local_filename;
    int port;

    port = free_port ();
    CHECK(port > 0);

    snprintf (str_range, sizeof (str_range), "%d", port);
    config_file_option_set (xfer_config_network_port_range, str_range, 1);

    /* Partial file already received (temporary file, removed in teardown). */
    local_filename = string_eval_path_home (
        "${weechat_data_dir}/xfer/alice.resume_timeout.txt", NULL, NULL, NULL);
    string_asprintf (&temp_resume_filename, "%s.part", local_filename);
    free (local_filename);
    create_file (temp_resume_filename, "0123456789");

    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "resume_timeout.txt", "size", "100",
                          "remote_address", "127.0.0.1",
                          "socket", str_sock_server (), "token", "45",
                          NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(10, ptr_xfer->start_resume);

    /* Accept: DCC RESUME sent, waiting for DCC ACCEPT with a timeout. */
    xfer_network_accept (ptr_xfer);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, ptr_xfer->status);
    CHECK(ptr_xfer->sock >= 0);
    LONGS_EQUAL(port, ptr_xfer->port);
    CHECK(ptr_xfer->hook_timer);
    LONGS_EQUAL(1, count_resume);
    LONGS_EQUAL(1, xfer_port_in_use (port));

    /* DCC ACCEPT not received: socket closed and port released. */
    record_start ();
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_timer_cb (ptr_xfer, NULL, 0));
    CHECK(record_search_msg_regex ("core.weechat",
                                   "timeout for \"resume_timeout.txt\" "
                                   "with alice"));
    LONGS_EQUAL(XFER_STATUS_FAILED, ptr_xfer->status);
    LONGS_EQUAL(-1, ptr_xfer->sock);
    POINTERS_EQUAL(NULL, ptr_xfer->hook_timer);
    LONGS_EQUAL(0, xfer_port_in_use (port));
    LONGS_EQUAL(0, count_signals);
}
