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
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/core/core-infolist.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-signal.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/xfer/xfer.h"
#include "src/plugins/xfer/xfer-config.h"

extern int xfer_search_type (const char *type);
extern int xfer_search_protocol (const char *protocol);
extern int xfer_port_in_use (int port);
extern int xfer_nick_auto_accepted (const char *server, const char *nick);
extern struct t_xfer *xfer_new (const char *plugin_name,
                                const char *plugin_id,
                                enum t_xfer_type type,
                                enum t_xfer_protocol protocol,
                                const char *remote_nick,
                                const char *local_nick,
                                const char *charset_modifier,
                                const char *filename,
                                unsigned long long size, const char *proxy,
                                struct sockaddr *remote_address,
                                socklen_t remote_address_length,
                                struct sockaddr *local_address,
                                socklen_t local_address_length,
                                int port, int sock,
                                const char *local_filename,
                                const char *token);
extern void xfer_disconnect_all (void);
extern int xfer_add_cb (const void *pointer, void *data,
                        const char *signal, const char *type_data,
                        void *signal_data);
extern int xfer_signal_upgrade_cb (const void *pointer, void *data,
                                   const char *signal, const char *type_data,
                                   void *signal_data);
extern void xfer_print_log (void);

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
     *   - a file sent.
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
    /* Invalid arguments */
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

    /* Plugin name, plugin id, type and port are compared. */
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

    /* fer found */
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
     * The status must be compared: a transfer waiting for the manual
     * acceptance by the user must not be returned when a connecting transfer
     * is searched.
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

    /* The status of xfers must not be changed by the search. */
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
    int count_signals;
    struct t_hook *hook_signals;

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

        record_start ();
    }

    void teardown ()
    {
        struct t_xfer *ptr_xfer;

        record_stop ();

        unhook (hook_signals);

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
 *   xfer_add_cb (errors)
 */

TEST(XferAdd, AddCbErrors)
{
    struct t_infolist *infolist;

    /* No infolist. */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                xfer_add_cb (NULL, NULL, "xfer_add",
                             WEECHAT_HOOK_SIGNAL_POINTER, NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat", "missing arguments \\(xfer_add\\)");

    /* Empty infolist. */
    infolist = infolist_new (NULL);
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                xfer_add_cb (NULL, NULL, "xfer_add",
                             WEECHAT_HOOK_SIGNAL_POINTER, infolist));
    infolist_free (infolist);

    /* Missing arguments. */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "chat_recv",
                          "remote_nick", "alice", NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_active",
                          "remote_nick", "alice", "local_nick", "bob",
                          "protocol_string", "dcc", NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_active",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test.txt", NULL));

    /* File received without size. */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_active",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test.txt",
                          "remote_address", "127.0.0.1", "port", "5000",
                          NULL));
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test.txt",
                          "remote_address", "127.0.0.1", NULL));

    /* Unknown type and protocol. */
    record_start ();
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "xxx",
                          "remote_nick", "alice", "local_nick", "bob", NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat", "unknown xfer type \"xxx\"");
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_active",
                          "protocol_string", "xxx",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test.txt", NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat", "unknown xfer protocol \"xxx\"");

    /* Invalid remote address. */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_active",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test.txt", "size", "10",
                          "remote_address", "abc", "port", "5000", NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat", "invalid address \"abc\"");
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test.txt", "size", "10",
                          "remote_address", "abc", NULL));

    /* File to send not found. */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_send_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "/nonexistent_dir/file.txt", NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "cannot access file \"/nonexistent_dir/file.txt\"");

    /* Invalid socket of IRC server (local address not found). */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "chat_send",
                          "remote_nick", "alice", "local_nick", "bob",
                          "socket", "-1", NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "cannot get local address for xfer: error");

    POINTERS_EQUAL(saved_xfer_list, xfer_list);
}

/*
 * Test functions:
 *   xfer_add_cb (file received, active)
 *   xfer_new
 */

TEST(XferAdd, AddCbFileRecvActive)
{
    struct t_xfer *ptr_xfer;

    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_active",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "my file.txt", "size", "1000",
                          "remote_address", "3232235778", "port", "5000",
                          NULL));
    ptr_xfer = xfer_list;
    CHECK(ptr_xfer != saved_xfer_list);
    STRCMP_EQUAL("irc", ptr_xfer->plugin_name);
    STRCMP_EQUAL("libera", ptr_xfer->plugin_id);
    LONGS_EQUAL(XFER_TYPE_FILE_RECV_ACTIVE, ptr_xfer->type);
    LONGS_EQUAL(XFER_PROTOCOL_DCC, ptr_xfer->protocol);
    LONGS_EQUAL(XFER_STATUS_WAITING, ptr_xfer->status);
    STRCMP_EQUAL("alice", ptr_xfer->remote_nick);
    STRCMP_EQUAL("bob", ptr_xfer->local_nick);
    STRCMP_EQUAL("my_file.txt", ptr_xfer->filename);
    LONGS_EQUAL(1000, ptr_xfer->size);
    STRCMP_EQUAL("192.168.1.2", ptr_xfer->remote_address_str);
    STRCMP_EQUAL("", ptr_xfer->local_address_str);
    LONGS_EQUAL(5000, ptr_xfer->port);
    LONGS_EQUAL(-1, ptr_xfer->sock);
    CHECK(ptr_xfer->local_filename);
    CHECK(strstr (ptr_xfer->local_filename, "/xfer/alice.my_file.txt"));
    POINTERS_EQUAL(NULL, ptr_xfer->hash_handle);
    LONGS_EQUAL(0, count_signals);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "incoming file from alice \\(192.168.1.2, "
                           "irc.libera\\), name: my_file.txt, size: 1000 "
                           "\\(protocol: dcc\\)");

    /* Keep spaces in filename, check CRC32 found in filename. */
    config_file_option_set (xfer_config_file_convert_spaces, "off", 1);
    config_file_option_set (xfer_config_file_auto_check_crc32, "on", 1);
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_active",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "my file_1A2B3C4D.txt", "size", "1000",
                          "remote_address", "127.0.0.1", "port", "5001",
                          NULL));
    ptr_xfer = xfer_list;
    STRCMP_EQUAL("my file_1A2B3C4D.txt", ptr_xfer->filename);
    CHECK(ptr_xfer->hash_handle);
    STRCMP_EQUAL("1A2B3C4D", ptr_xfer->hash_target);
    LONGS_EQUAL(XFER_HASH_STATUS_IN_PROGRESS, ptr_xfer->hash_status);
}

TEST(XferAdd, AddCbFileRecvActiveResume)
{
    struct t_xfer *ptr_xfer;
    char *local_filename, *temp_filename;

    /* Partial file already received (temporary file). */
    local_filename = string_eval_path_home (
        "${weechat_data_dir}/xfer/alice.resume.txt", NULL, NULL, NULL);
    string_asprintf (&temp_filename, "%s.part", local_filename);
    create_file (temp_filename, "0123456789");

    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_recv_active",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "resume.txt", "size", "100",
                          "remote_address", "127.0.0.1", "port", "5000",
                          NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(10, ptr_xfer->start_resume);
    STRCMP_EQUAL(local_filename, ptr_xfer->local_filename);
    STRCMP_EQUAL(temp_filename, ptr_xfer->temp_local_filename);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "file resume.txt \\(local filename: .*"
                           "alice.resume.txt\\) will be resumed at "
                           "position 10");

    unlink (temp_filename);
    free (temp_filename);
    free (local_filename);
}

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
                          "remote_address", "127.0.0.1",
                          "socket", str_sock_server (), "token", "42",
                          NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(XFER_TYPE_FILE_RECV_PASSIVE, ptr_xfer->type);
    LONGS_EQUAL(XFER_STATUS_WAITING, ptr_xfer->status);
    STRCMP_EQUAL("127.0.0.1", ptr_xfer->remote_address_str);
    STRCMP_EQUAL("127.0.0.1", ptr_xfer->local_address_str);
    STRCMP_EQUAL("42", ptr_xfer->token);
    CHECK(ptr_xfer->sock >= 0);
    CHECK(ptr_xfer->port > 0);
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
 *   xfer_add_cb (file sent, passive)
 *   xfer_new
 */

TEST(XferAdd, AddCbFileSendPassive)
{
    struct t_xfer *ptr_xfer;
    char *path;

    create_file (filename, "hello");

    /* Absolute path. */
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_send_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", filename,
                          "socket", str_sock_server (), NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(XFER_TYPE_FILE_SEND_PASSIVE, ptr_xfer->type);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, ptr_xfer->status);
    STRCMP_EQUAL("test_xfer_upload.txt", ptr_xfer->filename);
    STRCMP_EQUAL(filename, ptr_xfer->local_filename);
    LONGS_EQUAL(5, ptr_xfer->size);
    STRCMP_EQUAL("", ptr_xfer->remote_address_str);
    STRCMP_EQUAL("127.0.0.1", ptr_xfer->local_address_str);
    CHECK(ptr_xfer->sock >= 0);
    CHECK(ptr_xfer->port > 0);
    CHECK(ptr_xfer->hook_fd);
    CHECK(ptr_xfer->hook_timer);
    /* Signal sent twice: by xfer_network_connect and xfer_add_cb. */
    CHECK(count_signals >= 1);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "offering file to alice \\(irc.libera\\), "
                           "name: test_xfer_upload.txt \\(local filename: "
                           ".*test_xfer_upload.txt\\), size: 5 "
                           "\\(protocol: dcc\\)");

    /* Path relative to upload directory. */
    path = string_eval_path_home ("${weechat_data_dir}", NULL, NULL, NULL);
    config_file_option_set (xfer_config_file_upload_path, path, 1);
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_send_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "test_xfer_upload.txt",
                          "socket", str_sock_server (), NULL));
    ptr_xfer = xfer_list;
    STRCMP_EQUAL(rel_filename, ptr_xfer->local_filename);
    free (path);

    /* Path relative to home. */
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_send_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", "~/weechat_nonexistent_file.txt",
                          "socket", str_sock_server (), NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "cannot access file \".*/"
                           "weechat_nonexistent_file.txt\"");
}

/*
 * Test functions:
 *   xfer_add_cb (chat)
 *   xfer_new
 */

TEST(XferAdd, AddCbChat)
{
    struct t_xfer *ptr_xfer;

    /* Chat received. */
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "chat_recv",
                          "remote_nick", "alice", "local_nick", "bob",
                          "remote_address", "127.0.0.1", "port", "5000",
                          NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(XFER_TYPE_CHAT_RECV, ptr_xfer->type);
    LONGS_EQUAL(XFER_STATUS_WAITING, ptr_xfer->status);
    STRCMP_EQUAL("xfer chat", ptr_xfer->filename);
    STRCMP_EQUAL("127.0.0.1", ptr_xfer->remote_address_str);
    POINTERS_EQUAL(NULL, ptr_xfer->local_filename);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "incoming chat request from alice \\(127.0.0.1, "
                           "irc.libera\\)");

    /* Chat sent, with own IP. */
    config_file_option_set (xfer_config_network_own_ip, "127.0.0.1", 1);
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "chat_send",
                          "remote_nick", "alice", "local_nick", "bob", NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(XFER_TYPE_CHAT_SEND, ptr_xfer->type);
    LONGS_EQUAL(XFER_STATUS_WAITING, ptr_xfer->status);
    STRCMP_EQUAL("127.0.0.1", ptr_xfer->local_address_str);
    CHECK(ptr_xfer->sock >= 0);
    CHECK(ptr_xfer->port > 0);
    CHECK(ptr_xfer->hook_fd);
    LONGS_EQUAL(1, count_signals);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "sending chat request to alice \\(irc.libera\\)");
}

/*
 * Test functions:
 *   xfer_add_cb (port range)
 *   xfer_port_in_use
 */

TEST(XferAdd, AddCbPortRange)
{
    struct t_xfer *ptr_xfer;
    char str_range[64];
    int port;

    port = free_port ();
    CHECK(port > 0);

    /* Single port. */
    snprintf (str_range, sizeof (str_range), "%d", port);
    config_file_option_set (xfer_config_network_port_range, str_range, 1);
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "chat_send",
                          "remote_nick", "alice", "local_nick", "bob",
                          "socket", str_sock_server (), NULL));
    ptr_xfer = xfer_list;
    LONGS_EQUAL(port, ptr_xfer->port);
    LONGS_EQUAL(1, xfer_port_in_use (port));

    /* Port already used by the xfer: no port available. */
    snprintf (str_range, sizeof (str_range), "%d-%d", port, port);
    config_file_option_set (xfer_config_network_port_range, str_range, 1);
    LONGS_EQUAL(WEECHAT_RC_ERROR,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "chat_send",
                          "remote_nick", "alice", "local_nick", "bob",
                          "socket", str_sock_server (), NULL));
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "cannot find available port for xfer");

    /* Port not in use any more when the xfer has ended. */
    xfer_close (ptr_xfer, XFER_STATUS_ABORTED);
    LONGS_EQUAL(0, xfer_port_in_use (port));
}

/*
 * Test functions:
 *   xfer_new (invalid address)
 */

TEST(XferAdd, NewInvalidAddress)
{
    struct t_xfer *ptr_xfer;
    struct sockaddr_storage addr;

    memset (&addr, 0, sizeof (addr));
    ptr_xfer = xfer_new ("irc", "libera", XFER_TYPE_CHAT_RECV,
                         XFER_NO_PROTOCOL, "alice", "bob", NULL, NULL, 0,
                         NULL, (struct sockaddr *)&addr, 0, NULL, 0,
                         5000, -1, NULL, NULL);
    CHECK(ptr_xfer);
    STRCMP_EQUAL("?", ptr_xfer->remote_address_str);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to interpret address");
}

/*
 * Test functions:
 *   xfer_new (socket closed on error)
 */

TEST(XferAdd, NewSocketClosedOnError)
{
    struct sockaddr_in addr;
    struct t_xfer *ptr_xfer;
    int fd_pipe[2];

    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);

    /* Not a socket: listen fails, xfer fails and the socket is closed. */
    LONGS_EQUAL(0, pipe (fd_pipe));
    close (fd_pipe[1]);
    ptr_xfer = xfer_new ("irc", "libera", XFER_TYPE_CHAT_SEND,
                         XFER_NO_PROTOCOL, "alice", "bob", NULL, NULL, 0,
                         NULL, NULL, 0, (struct sockaddr *)&addr,
                         sizeof (addr), 5000, fd_pipe[0], NULL, NULL);
    POINTERS_EQUAL(NULL, ptr_xfer);
    LONGS_EQUAL(-1, fcntl (fd_pipe[0], F_GETFD));
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer_list->status);
    LONGS_EQUAL(-1, xfer_list->sock);
}

/*
 * Test functions:
 *   xfer_valid
 *   xfer_search_type
 *   xfer_search_protocol
 *   xfer_search_by_number
 *   xfer_search_by_buffer
 */

TEST(XferAdd, ValidSearch)
{
    struct t_xfer *ptr_xfer, xfer_stack;

    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "chat_recv",
                          "remote_nick", "alice", "local_nick", "bob",
                          "remote_address", "127.0.0.1", "port", "5000",
                          NULL));
    ptr_xfer = xfer_list;

    LONGS_EQUAL(0, xfer_valid (NULL));
    LONGS_EQUAL(0, xfer_valid (&xfer_stack));
    LONGS_EQUAL(1, xfer_valid (ptr_xfer));

    LONGS_EQUAL(-1, xfer_search_type (NULL));
    LONGS_EQUAL(-1, xfer_search_type ("xxx"));
    LONGS_EQUAL(XFER_TYPE_CHAT_SEND, xfer_search_type ("chat_send"));
    LONGS_EQUAL(-1, xfer_search_protocol (NULL));
    LONGS_EQUAL(-1, xfer_search_protocol ("xxx"));
    LONGS_EQUAL(XFER_PROTOCOL_DCC, xfer_search_protocol ("dcc"));

    POINTERS_EQUAL(ptr_xfer, xfer_search_by_number (0));
    POINTERS_EQUAL(NULL, xfer_search_by_number (xfer_count));

    POINTERS_EQUAL(NULL, xfer_search_by_buffer (NULL));
    POINTERS_EQUAL(NULL, xfer_search_by_buffer (gui_buffers));
    ptr_xfer->buffer = gui_buffers;
    POINTERS_EQUAL(ptr_xfer, xfer_search_by_buffer (gui_buffers));
    ptr_xfer->buffer = NULL;
}

/*
 * Test functions:
 *   xfer_nick_auto_accepted
 */

TEST(XferAdd, NickAutoAccepted)
{
    LONGS_EQUAL(0, xfer_nick_auto_accepted ("libera", "alice"));

    config_file_option_set (xfer_config_file_auto_accept_nicks,
                            "alice,libera.bob", 1);
    LONGS_EQUAL(1, xfer_nick_auto_accepted ("libera", "alice"));
    LONGS_EQUAL(1, xfer_nick_auto_accepted ("oftc", "ALICE"));
    LONGS_EQUAL(1, xfer_nick_auto_accepted ("libera", "bob"));
    LONGS_EQUAL(1, xfer_nick_auto_accepted ("LIBERA", "Bob"));
    LONGS_EQUAL(0, xfer_nick_auto_accepted ("oftc", "bob"));
    LONGS_EQUAL(0, xfer_nick_auto_accepted ("libera", "carol"));

    /* The whole server name must match. */
    LONGS_EQUAL(0, xfer_nick_auto_accepted ("libera2", "bob"));
    LONGS_EQUAL(0, xfer_nick_auto_accepted ("liberatest", "bob"));
    LONGS_EQUAL(0, xfer_nick_auto_accepted ("liber", "bob"));
    LONGS_EQUAL(0, xfer_nick_auto_accepted ("", "bob"));
}

/*
 * Test functions:
 *   xfer_disconnect_all
 *   xfer_signal_upgrade_cb
 *   xfer_print_log
 */

TEST(XferAdd, DisconnectAllUpgrade)
{
    struct t_xfer *ptr_xfer_chat, *ptr_xfer_file;
    char *upgrade_filename;

    create_file (filename, "hello");
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "file_send_passive",
                          "protocol_string", "dcc",
                          "remote_nick", "alice", "local_nick", "bob",
                          "filename", filename,
                          "socket", str_sock_server (), NULL));
    ptr_xfer_file = xfer_list;
    LONGS_EQUAL(WEECHAT_RC_OK_EAT,
                add_xfer ("plugin_name", "irc", "plugin_id", "libera",
                          "type_string", "chat_send",
                          "remote_nick", "carol", "local_nick", "bob",
                          "socket", str_sock_server (), NULL));
    ptr_xfer_chat = xfer_list;
    ptr_xfer_chat->status = XFER_STATUS_ACTIVE;

    xfer_print_log ();

    upgrade_filename = string_eval_path_home (
        "${weechat_data_dir}/xfer.upgrade", NULL, NULL, NULL);

    /* Save only: xfers are not disconnected. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                xfer_signal_upgrade_cb (NULL, NULL, "upgrade",
                                        WEECHAT_HOOK_SIGNAL_STRING,
                                        (void *)"save"));
    LONGS_EQUAL(XFER_STATUS_ACTIVE, ptr_xfer_chat->status);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, ptr_xfer_file->status);

    /* Upgrade: all xfers are disconnected. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                xfer_signal_upgrade_cb (NULL, NULL, "upgrade",
                                        WEECHAT_HOOK_SIGNAL_STRING, NULL));
    LONGS_EQUAL(XFER_STATUS_ABORTED, ptr_xfer_chat->status);
    LONGS_EQUAL(-1, ptr_xfer_chat->sock);
    LONGS_EQUAL(XFER_STATUS_FAILED, ptr_xfer_file->status);
    LONGS_EQUAL(-1, ptr_xfer_file->sock);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "aborting active xfer: \"xfer chat\" from carol");

    unlink (upgrade_filename);
    free (upgrade_filename);
}
