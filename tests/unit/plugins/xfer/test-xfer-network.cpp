/*
 * SPDX-FileCopyrightText: 2022-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test xfer network functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"
#include "tests-record.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "src/core/core-config-file.h"
#include "src/core/core-hook.h"
#include "src/core/core-string.h"
#include "src/core/hook/hook-signal.h"
#include "src/core/hook/hook-timer.h"
#include "src/gui/gui-buffer.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/xfer/xfer.h"
#include "src/plugins/xfer/xfer-config.h"
#include "src/plugins/xfer/xfer-network.h"

extern struct t_xfer *xfer_alloc (void);
extern char *xfer_network_convert_integer_to_ipv4 (const char *str_address);
extern int xfer_network_create_pipe (struct t_xfer *xfer);
extern void xfer_network_write_pipe (struct t_xfer *xfer, int status,
                                     int error);
extern int xfer_network_child_read_cb (const void *pointer, void *data,
                                       int fd);
extern void xfer_network_send_file_fork (struct t_xfer *xfer);
extern void xfer_network_recv_file_fork (struct t_xfer *xfer);
extern int xfer_network_fd_cb (const void *pointer, void *data, int fd);
extern int xfer_network_timer_cb (const void *pointer, void *data,
                                  int remaining_calls);
extern int xfer_network_connect_chat_recv_cb (const void *pointer, void *data,
                                              int status, int gnutls_rc,
                                              int sock, const char *error,
                                              const char *ip_address);
extern int xfer_network_connect (struct t_xfer *xfer);
extern void xfer_network_connect_init (struct t_xfer *xfer);
extern void xfer_network_accept (struct t_xfer *xfer);
}

TEST_GROUP(XferNetwork)
{
};

TEST_GROUP(XferNetworkWithXfer)
{
    struct t_xfer *xfer;
    int sock_client;
    char *filename, *temp_filename;
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

        xfer = xfer_alloc ();
        CHECK(xfer);
        xfer->plugin_name = strdup ("irc");
        xfer->plugin_id = strdup ("test");
        xfer->protocol = XFER_NO_PROTOCOL;
        xfer->remote_nick = strdup ("alice");
        xfer->local_nick = strdup ("bob");
        xfer->filename = strdup ("file.txt");
        xfer->remote_address_str = strdup ("127.0.0.1");
        xfer->port = 1234;

        memset (&addr, 0, sizeof (addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
        xfer_set_local_address (xfer, (struct sockaddr *)&addr, sizeof (addr),
                                "127.0.0.1");

        sock_client = -1;

        filename = string_eval_path_home (
            "${weechat_data_dir}/test_xfer_send.txt", NULL, NULL, NULL);
        temp_filename = string_eval_path_home (
            "${weechat_data_dir}/test_xfer_recv.txt", NULL, NULL, NULL);
        unlink (filename);
        unlink (temp_filename);

        count_signals = 0;
        hook_signals = NULL;

        record_start ();
    }

    void teardown ()
    {
        record_stop ();

        if (hook_signals)
            unhook (hook_signals);

        wait_child ();

        if (xfer->buffer)
            gui_buffer_close ((struct t_gui_buffer *)xfer->buffer);
        if (xfer->sock >= 0)
            close (xfer->sock);
        if (xfer->child_read >= 0)
            close (xfer->child_read);
        if (xfer->child_write >= 0)
            close (xfer->child_write);
        if (xfer->file >= 0)
            close (xfer->file);
        xfer_free (xfer);

        if (sock_client >= 0)
            close (sock_client);

        unlink (filename);
        unlink (temp_filename);
        free (filename);
        free (temp_filename);

        config_file_option_reset (xfer_config_network_timeout, 1);

    }

    /* Wait for end of child process (it exits immediately). */
    void wait_child ()
    {
        int i;

        if (xfer->child_pid <= 0)
            return;

        for (i = 0; i < 500; i++)
        {
            if (waitpid (xfer->child_pid, NULL, WNOHANG) != 0)
                break;
            usleep (10000);
        }
        if (i >= 500)
            kill (xfer->child_pid, SIGKILL);
        xfer->child_pid = 0;
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

    /*
     * Create a non-blocking socket listening on loopback (in xfer->sock) and
     * connect a client to it (accept will not block).
     */
    void listen_and_connect ()
    {
        struct sockaddr_in addr;
        socklen_t length;
        struct pollfd poll_fd;

        xfer->sock = socket (AF_INET, SOCK_STREAM, 0);
        CHECK(xfer->sock >= 0);
        memset (&addr, 0, sizeof (addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
        LONGS_EQUAL(0, bind (xfer->sock, (struct sockaddr *)&addr,
                             sizeof (addr)));
        LONGS_EQUAL(0, listen (xfer->sock, 1));
        fcntl (xfer->sock, F_SETFL, fcntl (xfer->sock, F_GETFL) | O_NONBLOCK);
        length = sizeof (addr);
        LONGS_EQUAL(0, getsockname (xfer->sock, (struct sockaddr *)&addr,
                                    &length));

        sock_client = socket (AF_INET, SOCK_STREAM, 0);
        CHECK(sock_client >= 0);
        LONGS_EQUAL(0, connect (sock_client, (struct sockaddr *)&addr,
                                sizeof (addr)));

        /*
         * Wait for the pending connection on the listening socket (on macOS
         * it is not immediately available after the connect).
         */
        poll_fd.fd = xfer->sock;
        poll_fd.events = POLLIN;
        poll_fd.revents = 0;
        LONGS_EQUAL(1, poll (&poll_fd, 1, 5000));
    }
};

/*
 * Test functions:
 *   xfer_network_convert_integer_to_ipv4
 */

TEST(XferNetwork, ConvertIntegerToIpv4)
{
    char *str;

    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 (NULL));
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 (""));
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("abc"));
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("0"));
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("-1"));

    /* Too big: UINT32_MAX + 1 = 4294967296 */
    STRCMP_EQUAL(NULL, xfer_network_convert_integer_to_ipv4 ("4294967296"));

    WEE_TEST_STR("0.0.0.1", xfer_network_convert_integer_to_ipv4 ("1"));
    WEE_TEST_STR("0.0.1.0", xfer_network_convert_integer_to_ipv4 ("256"));
    WEE_TEST_STR("0.1.0.0", xfer_network_convert_integer_to_ipv4 ("65536"));
    WEE_TEST_STR("1.0.0.0", xfer_network_convert_integer_to_ipv4 ("16777216"));
    WEE_TEST_STR("127.0.0.1", xfer_network_convert_integer_to_ipv4 ("2130706433"));
    WEE_TEST_STR("192.168.1.2", xfer_network_convert_integer_to_ipv4 ("3232235778"));
}

/*
 * Test functions:
 *   xfer_network_resolve_addr
 */

TEST(XferNetwork, ResolveAddr)
{
    struct sockaddr_storage addr;
    struct sockaddr_in *addr4;
    struct sockaddr_in6 *addr6;
    socklen_t length;
    char str_ip[INET6_ADDRSTRLEN];
    int flags;

    /* Numeric addresses only (no DNS query). */
    flags = AI_NUMERICSERV | AI_NUMERICHOST;

    record_start ();

    length = sizeof (addr);
    LONGS_EQUAL(1, xfer_network_resolve_addr ("127.0.0.1", "6667",
                                              (struct sockaddr *)&addr,
                                              &length, flags));
    LONGS_EQUAL(sizeof (struct sockaddr_in), length);
    addr4 = (struct sockaddr_in *)&addr;
    LONGS_EQUAL(AF_INET, addr4->sin_family);
    LONGS_EQUAL(6667, ntohs (addr4->sin_port));
    STRCMP_EQUAL("127.0.0.1",
                 inet_ntop (AF_INET, &addr4->sin_addr, str_ip,
                            sizeof (str_ip)));

    length = sizeof (addr);
    LONGS_EQUAL(1, xfer_network_resolve_addr ("::1", NULL,
                                              (struct sockaddr *)&addr,
                                              &length, flags));
    LONGS_EQUAL(sizeof (struct sockaddr_in6), length);
    addr6 = (struct sockaddr_in6 *)&addr;
    LONGS_EQUAL(AF_INET6, addr6->sin6_family);
    STRCMP_EQUAL("::1",
                 inet_ntop (AF_INET6, &addr6->sin6_addr, str_ip,
                            sizeof (str_ip)));

    /* IPv4 address as integer (as sent in DCC requests). */
    length = sizeof (addr);
    LONGS_EQUAL(1, xfer_network_resolve_addr ("3232235778", "1234",
                                              (struct sockaddr *)&addr,
                                              &length, flags));
    addr4 = (struct sockaddr_in *)&addr;
    LONGS_EQUAL(AF_INET, addr4->sin_family);
    STRCMP_EQUAL("192.168.1.2",
                 inet_ntop (AF_INET, &addr4->sin_addr, str_ip,
                            sizeof (str_ip)));

    /* Buffer too small for the address. */
    length = sizeof (struct sockaddr_in);
    LONGS_EQUAL(0, xfer_network_resolve_addr ("::1", NULL,
                                              (struct sockaddr *)&addr,
                                              &length, flags));
    LONGS_EQUAL(sizeof (struct sockaddr_in), length);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "address \"::1\" resolved to a larger sockaddr "
                           "than expected");

    /* Invalid addresses. */
    length = sizeof (addr);
    LONGS_EQUAL(0, xfer_network_resolve_addr ("abc", NULL,
                                              (struct sockaddr *)&addr,
                                              &length, flags));
    RECORD_CHECK_MSG_REGEX("core.weechat", "invalid address \"abc\": error");
    LONGS_EQUAL(0, xfer_network_resolve_addr ("127.0.0.1", "abc",
                                              (struct sockaddr *)&addr,
                                              &length, flags));

    record_stop ();
}

/*
 * Test functions:
 *   xfer_network_create_pipe
 *   xfer_network_write_pipe
 *   xfer_network_child_read_cb
 */

TEST(XferNetworkWithXfer, ChildReadCb)
{
    xfer->type = XFER_TYPE_FILE_RECV_ACTIVE;
    xfer->status = XFER_STATUS_CONNECTING;
    xfer->local_filename = strdup (filename);
    xfer->temp_local_filename = strdup (temp_filename);

    LONGS_EQUAL(1, xfer_network_create_pipe (xfer));
    CHECK(xfer->child_read >= 0);
    CHECK(xfer->child_write >= 0);

    /* Nothing to read: nothing done. */
    fcntl (xfer->child_read, F_SETFL,
           fcntl (xfer->child_read, F_GETFL) | O_NONBLOCK);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);

    /* Incomplete message: ignored. */
    LONGS_EQUAL(5, write (xfer->child_write, "20123", 5));
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);
    LONGS_EQUAL(0, xfer->pos);

    /* Connecting -> active: transfer starts. */
    xfer->pos = 1024;
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE, XFER_NO_ERROR);
    xfer->pos = 0;
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);
    LONGS_EQUAL(1024, xfer->pos);
    CHECK(xfer->last_activity > 0);

    /* Still active, with more data. */
    xfer->pos = 4096;
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE, XFER_NO_ERROR);
    xfer->pos = 0;
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);
    LONGS_EQUAL(4096, xfer->pos);

    /* Connecting. */
    xfer_network_write_pipe (xfer, XFER_STATUS_CONNECTING, XFER_NO_ERROR);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);

    /* Hashing, then hashed OK. */
    xfer_network_write_pipe (xfer, XFER_STATUS_HASHING, XFER_NO_ERROR);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_HASHING, xfer->status);
    xfer_network_write_pipe (xfer, XFER_STATUS_HASHED, XFER_NO_ERROR);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_HASH_STATUS_MATCH, xfer->hash_status);

    /* Hashed with errors. */
    xfer_network_write_pipe (xfer, XFER_STATUS_HASHED,
                             XFER_ERROR_HASH_MISMATCH);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_HASH_STATUS_MISMATCH, xfer->hash_status);
    RECORD_CHECK_MSG_REGEX("core.weechat", "wrong CRC32 for file file.txt");
    xfer_network_write_pipe (xfer, XFER_STATUS_HASHED,
                             XFER_ERROR_HASH_RESUME_ERROR);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_HASH_STATUS_RESUME_ERROR, xfer->hash_status);
    RECORD_CHECK_MSG_REGEX("core.weechat", "CRC32 error while resuming");

    /* Errors (status unchanged). */
    xfer->status = XFER_STATUS_ACTIVE;
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE, XFER_ERROR_READ_LOCAL);
    xfer_network_child_read_cb (xfer, NULL, 0);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to read local file");
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE, XFER_ERROR_SEND_BLOCK);
    xfer_network_child_read_cb (xfer, NULL, 0);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "unable to send block to receiver");
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE, XFER_ERROR_READ_ACK);
    xfer_network_child_read_cb (xfer, NULL, 0);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to read ACK from receiver");
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                             XFER_ERROR_CONNECT_SENDER);
    xfer_network_child_read_cb (xfer, NULL, 0);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to connect to sender");
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE, XFER_ERROR_RECV_BLOCK);
    xfer_network_child_read_cb (xfer, NULL, 0);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "unable to receive block from sender");
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE,
                             XFER_ERROR_WRITE_LOCAL);
    xfer_network_child_read_cb (xfer, NULL, 0);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to write local file");
    xfer_network_write_pipe (xfer, XFER_STATUS_ACTIVE, XFER_ERROR_SEND_ACK);
    xfer_network_child_read_cb (xfer, NULL, 0);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to send ACK to sender");
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);

    /* Done: xfer is closed (and pipe too). */
    xfer->pos = 5000;
    xfer_network_write_pipe (xfer, XFER_STATUS_DONE, XFER_NO_ERROR);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_DONE, xfer->status);
    LONGS_EQUAL(5000, xfer->pos);
    LONGS_EQUAL(-1, xfer->child_read);
    LONGS_EQUAL(-1, xfer->child_write);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "file file.txt received from alice "
                           "\\(127.0.0.1\\): OK");

    /* Failed. */
    xfer->status = XFER_STATUS_ACTIVE;
    LONGS_EQUAL(1, xfer_network_create_pipe (xfer));
    xfer_network_write_pipe (xfer, XFER_STATUS_FAILED,
                             XFER_ERROR_RECV_BLOCK);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_child_read_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
    LONGS_EQUAL(-1, xfer->child_read);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "file file.txt received from alice "
                           "\\(127.0.0.1\\): FAILED");
}

/*
 * Test functions:
 *   xfer_network_send_file_fork
 */

TEST(XferNetworkWithXfer, SendFileFork)
{
    xfer->type = XFER_TYPE_FILE_SEND_PASSIVE;
    xfer->status = XFER_STATUS_ACTIVE;
    xfer->size = 5;

    /* Missing local file: xfer failed, no child process. */
    xfer->local_filename = strdup (filename);
    xfer_network_send_file_fork (xfer);
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
    LONGS_EQUAL(0, xfer->child_pid);
    POINTERS_EQUAL(NULL, xfer->hook_fd);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to read file");
    POINTERS_EQUAL(NULL, record_search_msg_regex ("core.weechat",
                                                  "sending file to"));

    /* Local file OK. */
    create_file (filename, "hello");
    xfer->status = XFER_STATUS_ACTIVE;
    xfer_network_send_file_fork (xfer);
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);
    CHECK(xfer->child_pid > 0);
    CHECK(xfer->child_read >= 0);
    LONGS_EQUAL(-1, xfer->child_write);
    CHECK(xfer->file >= 0);
    CHECK(xfer->hook_fd);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "sending file to alice \\(127.0.0.1, irc.test\\), "
                           "name: file.txt \\(local filename: .*"
                           "test_xfer_send.txt\\), 5 bytes");
}

/*
 * Test functions:
 *   xfer_network_recv_file_fork
 */

TEST(XferNetworkWithXfer, RecvFileFork)
{
    struct stat st;

    xfer->type = XFER_TYPE_FILE_RECV_ACTIVE;
    xfer->status = XFER_STATUS_CONNECTING;

    /* Invalid local file: xfer failed, no child process. */
    xfer->temp_local_filename = strdup ("/nonexistent_dir/file.txt");
    xfer_network_recv_file_fork (xfer);
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
    LONGS_EQUAL(0, xfer->child_pid);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "unable to write file \"/nonexistent_dir/file.txt\"");

    /* Resume of a missing file: xfer failed. */
    free (xfer->temp_local_filename);
    xfer->temp_local_filename = strdup (temp_filename);
    xfer->status = XFER_STATUS_CONNECTING;
    xfer->start_resume = 3;
    xfer_network_recv_file_fork (xfer);
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
    LONGS_EQUAL(0, xfer->child_pid);

    /* Resume of an existing file. */
    create_file (temp_filename, "abc");
    xfer->status = XFER_STATUS_CONNECTING;
    xfer_network_recv_file_fork (xfer);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);
    CHECK(xfer->child_pid > 0);
    CHECK(xfer->file >= 0);
    CHECK(xfer->hook_fd);
    LONGS_EQUAL(0, stat (temp_filename, &st));
    LONGS_EQUAL(3, st.st_size);
    wait_child ();
    close (xfer->file);
    xfer->file = -1;
    close (xfer->child_read);
    xfer->child_read = -1;
    unhook (xfer->hook_fd);
    xfer->hook_fd = NULL;

    /* New file (truncated). */
    xfer->start_resume = 0;
    xfer_network_recv_file_fork (xfer);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);
    CHECK(xfer->child_pid > 0);
    LONGS_EQUAL(0, stat (temp_filename, &st));
    LONGS_EQUAL(0, st.st_size);
}

/*
 * Test functions:
 *   xfer_network_fd_cb
 */

TEST(XferNetworkWithXfer, FdCbFileSendPassive)
{
    xfer->type = XFER_TYPE_FILE_SEND_PASSIVE;
    xfer->status = XFER_STATUS_CONNECTING;
    create_file (filename, "hello");
    xfer->local_filename = strdup (filename);

    listen_and_connect ();
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_fd_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);
    CHECK(xfer->sock >= 0);
    STRCMP_EQUAL("127.0.0.1", xfer->remote_address_str);
    CHECK(xfer->remote_address);
    CHECK(xfer->last_activity > 0);
    CHECK(fcntl (xfer->sock, F_GETFL) & O_NONBLOCK);

    /* Child process to send the file. */
    CHECK(xfer->child_pid > 0);
    CHECK(xfer->hook_fd);
    RECORD_CHECK_MSG_REGEX("core.weechat", "sending file to alice");
}

TEST(XferNetworkWithXfer, FdCbFileRecvPassive)
{
    xfer->type = XFER_TYPE_FILE_RECV_PASSIVE;
    xfer->status = XFER_STATUS_CONNECTING;
    xfer->temp_local_filename = strdup (temp_filename);

    listen_and_connect ();
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_fd_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);
    STRCMP_EQUAL("127.0.0.1", xfer->remote_address_str);

    /* Child process to receive the file. */
    CHECK(xfer->child_pid > 0);
    CHECK(xfer->file >= 0);
    CHECK(xfer->hook_fd);
}

TEST(XferNetworkWithXfer, FdCbChatSend)
{
    xfer->type = XFER_TYPE_CHAT_SEND;
    xfer->status = XFER_STATUS_WAITING;

    listen_and_connect ();
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_fd_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);
    STRCMP_EQUAL("127.0.0.1", xfer->remote_address_str);
    CHECK(xfer->hook_fd);
    CHECK(xfer->buffer);
    STRCMP_EQUAL("xfer.irc_dcc.test.alice",
                 ((struct t_gui_buffer *)xfer->buffer)->full_name);
}

TEST(XferNetworkWithXfer, FdCbErrors)
{
    /* Status not waiting/connecting: nothing done. */
    xfer->type = XFER_TYPE_CHAT_SEND;
    xfer->status = XFER_STATUS_ACTIVE;
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_fd_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);

    /* Connecting, but not a passive file: nothing done. */
    xfer->type = XFER_TYPE_FILE_RECV_ACTIVE;
    xfer->status = XFER_STATUS_CONNECTING;
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_fd_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);

    /* Error on accept (socket not listening). */
    xfer->type = XFER_TYPE_FILE_SEND_PASSIVE;
    xfer->status = XFER_STATUS_CONNECTING;
    xfer->sock = socket (AF_INET, SOCK_STREAM, 0);
    CHECK(xfer->sock >= 0);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_fd_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
    LONGS_EQUAL(-1, xfer->sock);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to create socket for "
                           "sending file: error");

    xfer->type = XFER_TYPE_FILE_RECV_PASSIVE;
    xfer->status = XFER_STATUS_CONNECTING;
    xfer->sock = socket (AF_INET, SOCK_STREAM, 0);
    CHECK(xfer->sock >= 0);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_fd_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
    LONGS_EQUAL(-1, xfer->sock);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to create socket for "
                           "receiving file: error");

    xfer->type = XFER_TYPE_CHAT_SEND;
    xfer->status = XFER_STATUS_WAITING;
    xfer->sock = socket (AF_INET, SOCK_STREAM, 0);
    CHECK(xfer->sock >= 0);
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_fd_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
    LONGS_EQUAL(-1, xfer->sock);
    POINTERS_EQUAL(NULL, xfer->buffer);
    RECORD_CHECK_MSG_REGEX("core.weechat", "unable to create socket for "
                           "chat: error");
}

/*
 * Test functions:
 *   xfer_network_timer_cb
 */

TEST(XferNetworkWithXfer, TimerCb)
{
    xfer->type = XFER_TYPE_CHAT_SEND;

    /* Active: no timeout. */
    xfer->status = XFER_STATUS_ACTIVE;
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_timer_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);

    xfer->status = XFER_STATUS_WAITING;
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_timer_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
    RECORD_CHECK_MSG_REGEX("core.weechat",
                           "timeout for \"file.txt\" with alice");

    xfer->type = XFER_TYPE_FILE_RECV_PASSIVE;
    xfer->status = XFER_STATUS_CONNECTING;
    LONGS_EQUAL(WEECHAT_RC_OK, xfer_network_timer_cb (xfer, NULL, 0));
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
}

/*
 * Test functions:
 *   xfer_network_connect_chat_recv_cb
 */

TEST(XferNetworkWithXfer, ConnectChatRecvCb)
{
    int sock[2];

    xfer->type = XFER_TYPE_CHAT_RECV;
    xfer->status = XFER_STATUS_CONNECTING;

    LONGS_EQUAL(0, socketpair (AF_UNIX, SOCK_STREAM, 0, sock));
    sock_client = sock[0];

    LONGS_EQUAL(WEECHAT_RC_OK,
                xfer_network_connect_chat_recv_cb (xfer, NULL,
                                                   WEECHAT_HOOK_CONNECT_OK,
                                                   0, sock[1], NULL,
                                                   "127.0.0.1"));
    LONGS_EQUAL(XFER_STATUS_ACTIVE, xfer->status);
    LONGS_EQUAL(sock[1], xfer->sock);
    CHECK(fcntl (xfer->sock, F_GETFL) & O_NONBLOCK);
    CHECK(xfer->hook_fd);
    CHECK(xfer->buffer);
    POINTERS_EQUAL(NULL, xfer->hook_connect);
}

TEST(XferNetworkWithXfer, ConnectChatRecvCbErrors)
{
    struct
    {
        int status;
        const char *message;
        const char *message_proxy;
    } errors[] = {
        { WEECHAT_HOOK_CONNECT_ADDRESS_NOT_FOUND,
          "address \"127.0.0.1\" not found",
          "proxy address \"127.0.0.1\" not found" },
        { WEECHAT_HOOK_CONNECT_IP_ADDRESS_NOT_FOUND,
          "IP address not found", "proxy IP address not found" },
        { WEECHAT_HOOK_CONNECT_CONNECTION_REFUSED,
          "connection refused", "proxy connection refused" },
        { WEECHAT_HOOK_CONNECT_PROXY_ERROR,
          "proxy fails to establish connection", NULL },
        { WEECHAT_HOOK_CONNECT_LOCAL_HOSTNAME_ERROR,
          "unable to set local hostname/IP", NULL },
        { WEECHAT_HOOK_CONNECT_MEMORY_ERROR,
          "not enough memory \\(some error\\)", NULL },
        { WEECHAT_HOOK_CONNECT_TIMEOUT, "xfer: timeout", NULL },
        { WEECHAT_HOOK_CONNECT_SOCKET_ERROR,
          "unable to create socket", NULL },
        { 99, "unable to connect: unexpected error \\(99\\)", NULL },
        { 0, NULL, NULL },
    };
    int i;

    xfer->type = XFER_TYPE_CHAT_RECV;

    for (i = 0; errors[i].message; i++)
    {
        xfer->status = XFER_STATUS_CONNECTING;
        LONGS_EQUAL(WEECHAT_RC_OK,
                    xfer_network_connect_chat_recv_cb (xfer, NULL,
                                                       errors[i].status,
                                                       0, -1, "some error",
                                                       NULL));
        LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
        RECORD_CHECK_MSG_REGEX("core.weechat", errors[i].message);
        RECORD_CHECK_MSG_REGEX("core.weechat", "error: some error");
        POINTERS_EQUAL(NULL, xfer->buffer);
    }

    /* With a proxy. */
    xfer->proxy = strdup ("myproxy");
    for (i = 0; errors[i].message; i++)
    {
        if (!errors[i].message_proxy)
            continue;
        xfer->status = XFER_STATUS_CONNECTING;
        LONGS_EQUAL(WEECHAT_RC_OK,
                    xfer_network_connect_chat_recv_cb (xfer, NULL,
                                                       errors[i].status,
                                                       0, -1, NULL, NULL));
        LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
        RECORD_CHECK_MSG_REGEX("core.weechat", errors[i].message_proxy);
    }
}

/*
 * Test functions:
 *   xfer_network_connect
 */

TEST(XferNetworkWithXfer, ConnectChatSend)
{
    xfer->type = XFER_TYPE_CHAT_SEND;

    LONGS_EQUAL(1, xfer_network_connect (xfer));
    LONGS_EQUAL(XFER_STATUS_WAITING, xfer->status);
    CHECK(xfer->sock >= 0);
    CHECK(xfer->hook_fd);
    CHECK(xfer->hook_timer);
}

TEST(XferNetworkWithXfer, ConnectTimeout)
{
    xfer->type = XFER_TYPE_CHAT_SEND;

    /* Timeout of 3000000 seconds: interval does not fit in an integer. */
    config_file_option_set (xfer_config_network_timeout, "3000000", 1);
    LONGS_EQUAL(1, xfer_network_connect (xfer));
    CHECK(xfer->hook_timer);
    CHECK(HOOK_TIMER(xfer->hook_timer, interval) == 3000000000L);
}

TEST(XferNetworkWithXfer, ConnectFileRecvPassive)
{
    hook_signals = hook_signal (NULL, "xfer_send_ready", &signal_cb,
                                &count_signals, NULL);

    xfer->type = XFER_TYPE_FILE_RECV_PASSIVE;
    LONGS_EQUAL(1, xfer_network_connect (xfer));
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);
    CHECK(xfer->sock >= 0);
    CHECK(xfer->hook_fd);
    CHECK(xfer->hook_timer);
    LONGS_EQUAL(1, count_signals);
}

TEST(XferNetworkWithXfer, ConnectFileRecvActive)
{
    /* Connection is made later in child process. */
    xfer->type = XFER_TYPE_FILE_RECV_ACTIVE;
    LONGS_EQUAL(1, xfer_network_connect (xfer));
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);
    LONGS_EQUAL(-1, xfer->sock);
    POINTERS_EQUAL(NULL, xfer->hook_fd);
}

TEST(XferNetworkWithXfer, ConnectErrors)
{
    struct sockaddr_in6 addr6;
    int fd_pipe[2];

    xfer->type = XFER_TYPE_CHAT_SEND;

    /* Invalid address family: socket can not be created. */
    memset (&addr6, 0, sizeof (addr6));
    addr6.sin6_family = 255;
    xfer_set_local_address (xfer, (struct sockaddr *)&addr6, sizeof (addr6),
                            "?");
    LONGS_EQUAL(0, xfer_network_connect (xfer));
    LONGS_EQUAL(-1, xfer->sock);

    /* Not a socket: listen fails. */
    LONGS_EQUAL(0, pipe (fd_pipe));
    close (fd_pipe[1]);
    xfer->sock = fd_pipe[0];
    LONGS_EQUAL(0, xfer_network_connect (xfer));
    POINTERS_EQUAL(NULL, xfer->hook_fd);
}

/*
 * Test functions:
 *   xfer_network_connect_init
 */

TEST(XferNetworkWithXfer, ConnectInit)
{
    int fd_pipe[2];

    /* Error. */
    xfer->type = XFER_TYPE_FILE_RECV_PASSIVE;
    LONGS_EQUAL(0, pipe (fd_pipe));
    close (fd_pipe[1]);
    xfer->sock = fd_pipe[0];
    xfer_network_connect_init (xfer);
    LONGS_EQUAL(XFER_STATUS_FAILED, xfer->status);
    close (xfer->sock);
    xfer->sock = -1;

    /* Active file: child process launched. */
    xfer->type = XFER_TYPE_FILE_RECV_ACTIVE;
    xfer->status = XFER_STATUS_WAITING;
    xfer->temp_local_filename = strdup (temp_filename);
    xfer_network_connect_init (xfer);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);
    CHECK(xfer->child_pid > 0);
    CHECK(xfer->hook_fd);
}

/*
 * Test functions:
 *   xfer_network_accept
 */

TEST(XferNetworkWithXfer, Accept)
{
    hook_signals = hook_signal (NULL, "xfer_resume_ready", &signal_cb,
                                &count_signals, NULL);

    /* Resume. */
    xfer->type = XFER_TYPE_FILE_RECV_ACTIVE;
    xfer->status = XFER_STATUS_WAITING;
    xfer->start_resume = 10;
    xfer_network_accept (xfer);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);
    LONGS_EQUAL(1, count_signals);
    LONGS_EQUAL(0, xfer->child_pid);

    /* No resume: connect and launch child process. */
    xfer->status = XFER_STATUS_WAITING;
    xfer->start_resume = 0;
    xfer->temp_local_filename = strdup (temp_filename);
    xfer_network_accept (xfer);
    LONGS_EQUAL(XFER_STATUS_CONNECTING, xfer->status);
    LONGS_EQUAL(1, count_signals);
    CHECK(xfer->child_pid > 0);
}
