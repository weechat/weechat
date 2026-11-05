/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test xfer DCC file transfer functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <gcrypt.h>
#include "src/core/core-string.h"
#include "src/plugins/xfer/xfer.h"
#include "src/plugins/xfer/xfer-dcc.h"

extern int xfer_dcc_recv_file_send_ack (struct t_xfer *xfer);
extern int xfer_dcc_resume_hash (struct t_xfer *xfer);
}

/* Maximum time for the peer thread (in milliseconds). */
#define PEER_TIMEOUT 5000

/*
 * Peer of the DCC transfer, running in a thread: it receives a file and sends
 * ACKs, or sends a file (and reads ACKs).
 */

struct t_test_dcc_peer
{
    int sock;                          /* socket used by the peer           */
    int sock_listen;                   /* listening socket (or -1)          */
    const char *data;                  /* data to send (sender)             */
    size_t size;                       /* size of data to send/receive      */
    size_t chunk;                      /* size of chunks sent (sender)      */
    char received[8192];               /* data received (receiver)          */
    size_t received_size;              /* size of data received             */
    size_t ack_offset;                 /* offset added to ACKs (receiver)   */
    int acks;                          /* number of ACKs sent/received      */
    uint32_t last_ack;                 /* last ACK received (sender)        */
};

/* Return current time in milliseconds. */

long long
test_dcc_now_ms ()
{
    struct timespec ts;

    clock_gettime (CLOCK_MONOTONIC, &ts);
    return ((long long)ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

/* Wait until the socket is readable (with a timeout); return 1 if readable. */

int
test_dcc_wait_readable (int sock, long long deadline)
{
    struct pollfd poll_fd;
    long long timeout;

    timeout = deadline - test_dcc_now_ms ();
    if (timeout <= 0)
        return 0;
    poll_fd.fd = sock;
    poll_fd.events = POLLIN;
    poll_fd.revents = 0;
    return (poll (&poll_fd, 1, (int)timeout) > 0) ? 1 : 0;
}

/* Thread: receive a file and send an ACK after each block received. */

void *
test_dcc_peer_receiver (void *arg)
{
    struct t_test_dcc_peer *peer;
    long long deadline;
    uint32_t ack;
    ssize_t num_read;

    peer = (struct t_test_dcc_peer *)arg;
    deadline = test_dcc_now_ms () + PEER_TIMEOUT;

    while ((peer->received_size < peer->size)
           && test_dcc_wait_readable (peer->sock, deadline))
    {
        num_read = recv (peer->sock, peer->received + peer->received_size,
                         sizeof (peer->received) - peer->received_size, 0);
        if (num_read <= 0)
            break;
        peer->received_size += num_read;
        ack = htonl ((uint32_t)(peer->ack_offset + peer->received_size));
        if (send (peer->sock, &ack, 4, MSG_NOSIGNAL) == 4)
            peer->acks++;
    }

    /* End of data sent to the sender of file (no more ACKs). */
    shutdown (peer->sock, SHUT_WR);

    return NULL;
}

/*
 * Thread: accept a connection (if listening socket), send a file by chunks,
 * then read ACKs until the end of connection.
 */

void *
test_dcc_peer_sender (void *arg)
{
    struct t_test_dcc_peer *peer;
    long long deadline;
    size_t pos, length;
    uint32_t ack;
    ssize_t num_read;

    peer = (struct t_test_dcc_peer *)arg;
    deadline = test_dcc_now_ms () + PEER_TIMEOUT;

    if (peer->sock_listen >= 0)
    {
        if (!test_dcc_wait_readable (peer->sock_listen, deadline))
            return NULL;
        peer->sock = accept (peer->sock_listen, NULL, NULL);
        if (peer->sock < 0)
            return NULL;
    }

    pos = 0;
    while (pos < peer->size)
    {
        length = peer->size - pos;
        if (length > peer->chunk)
            length = peer->chunk;
        if (send (peer->sock, peer->data + pos, length,
                  MSG_NOSIGNAL) != (ssize_t)length)
        {
            break;
        }
        pos += length;
        usleep (20000);
    }
    shutdown (peer->sock, SHUT_WR);

    while ((peer->last_ack < peer->size)
           && test_dcc_wait_readable (peer->sock, deadline))
    {
        num_read = recv (peer->sock, &ack, 4, 0);
        if (num_read != 4)
            break;
        peer->acks++;
        peer->last_ack = ntohl (ack);
    }

    return NULL;
}

TEST_GROUP(XferDcc)
{
    struct t_xfer xfer;
    int sock[2], fd_pipe[2];
    char *filename;
    void (*old_sigpipe)(int);
    struct t_test_dcc_peer peer;
    pthread_t thread;
    int thread_started;

    void setup ()
    {
        memset (&xfer, 0, sizeof (xfer));
        memset (&peer, 0, sizeof (peer));
        xfer.sock = -1;
        xfer.file = -1;
        xfer.child_read = -1;
        xfer.child_write = -1;
        xfer.fast_send = 1;
        xfer.send_ack = 1;
        xfer.blocksize = 65536;

        sock[0] = -1;
        sock[1] = -1;
        new_socketpair ();

        LONGS_EQUAL(0, pipe (fd_pipe));
        fcntl (fd_pipe[0], F_SETFL, fcntl (fd_pipe[0], F_GETFL) | O_NONBLOCK);
        xfer.child_write = fd_pipe[1];

        filename = string_eval_path_home (
            "${weechat_data_dir}/test_xfer_dcc.txt", NULL, NULL, NULL);
        unlink (filename);

        peer.sock_listen = -1;
        thread_started = 0;

        /* Sending data to a closed socket must not kill the tests. */
        old_sigpipe = signal (SIGPIPE, SIG_IGN);
    }

    void teardown ()
    {
        if (thread_started)
            pthread_join (thread, NULL);

        signal (SIGPIPE, old_sigpipe);

        if (xfer.hash_handle)
        {
            gcry_md_close (*xfer.hash_handle);
            free (xfer.hash_handle);
        }
        free (xfer.hash_target);
        free (xfer.temp_local_filename);

        if (xfer.file >= 0)
            close (xfer.file);
        if ((xfer.sock >= 0) && (xfer.sock != sock[0]))
            close (xfer.sock);
        if (sock[0] >= 0)
            close (sock[0]);
        if (peer.sock >= 0)
            close (peer.sock);
        if (peer.sock_listen >= 0)
            close (peer.sock_listen);
        close (fd_pipe[0]);
        close (fd_pipe[1]);

        unlink (filename);
        free (filename);
    }

    /*
     * Create a socket pair: one for xfer (non-blocking, like a real DCC
     * socket), the other one for the peer.
     */
    void new_socketpair ()
    {
        LONGS_EQUAL(0, socketpair (AF_UNIX, SOCK_STREAM, 0, sock));
        fcntl (sock[0], F_SETFL, fcntl (sock[0], F_GETFL) | O_NONBLOCK);
        xfer.sock = sock[0];
        peer.sock = sock[1];
    }

    /* Start the peer thread. */
    void start_peer (void *(*function)(void *))
    {
        LONGS_EQUAL(0, pthread_create (&thread, NULL, function, &peer));
        thread_started = 1;
    }

    /* Wait for end of peer thread. */
    void wait_peer ()
    {
        if (thread_started)
        {
            pthread_join (thread, NULL);
            thread_started = 0;
        }
    }

    /* Create a file with content and open it for reading. */
    void open_file_read (const char *content)
    {
        FILE *file;

        file = fopen (filename, "w");
        CHECK(file);
        fputs (content, file);
        fclose (file);
        xfer.file = open (filename, O_RDONLY);
        CHECK(xfer.file >= 0);
    }

    /* Return content of file (must be freed after use). */
    char *file_content ()
    {
        FILE *file;
        char buffer[8192];
        size_t size;

        file = fopen (filename, "r");
        if (!file)
            return NULL;
        size = fread (buffer, 1, sizeof (buffer) - 1, file);
        buffer[size] = '\0';
        fclose (file);
        return strdup (buffer);
    }

    /*
     * Return messages sent by child to parent via the pipe, as "SE" pairs
     * (S = status, E = error) separated by spaces, for example: "20 30"
     * (must be freed after use).
     */
    char *pipe_messages ()
    {
        char **messages, buffer[1 + 1 + 32 + 1], str_msg[4];

        messages = string_dyn_alloc (64);
        while (read (fd_pipe[0], buffer, sizeof (buffer)) == sizeof (buffer))
        {
            snprintf (str_msg, sizeof (str_msg), "%s%c%c",
                      (*messages)[0] ? " " : "", buffer[0], buffer[1]);
            string_dyn_concat (messages, str_msg, -1);
        }
        return string_dyn_free (messages, 0);
    }

    /* Check if the last message sent via the pipe is the given one. */
    int last_message_is (const char *message)
    {
        char *str;
        int rc;

        str = pipe_messages ();
        rc = (str && (strlen (str) >= 2)
              && (strcmp (str + strlen (str) - 2, message) == 0)) ? 1 : 0;
        free (str);
        return rc;
    }

    /* Return the last ACK sent by the receiver on the peer socket (or -1). */
    long long read_last_ack ()
    {
        uint32_t ack;
        long long last_ack;

        last_ack = -1;
        while (recv (peer.sock, &ack, 4, MSG_DONTWAIT) == 4)
        {
            last_ack = ntohl (ack);
        }
        return last_ack;
    }

    /* Initialize CRC32 hash, with target. */
    void init_hash (const char *target)
    {
        xfer.hash_handle = (gcry_md_hd_t *)malloc (sizeof (gcry_md_hd_t));
        CHECK(xfer.hash_handle);
        LONGS_EQUAL(0, gcry_md_open (xfer.hash_handle, GCRY_MD_CRC32, 0));
        xfer.hash_target = strdup (target);
    }
};

/*
 * Test functions:
 *   xfer_dcc_send_file_child
 */

TEST(XferDcc, SendFileChildEmpty)
{
    char *str, c;

    /* Empty file: done immediately, nothing sent. */
    xfer.size = 0;
    xfer_dcc_send_file_child (&xfer);
    WEE_TEST_STR("30", pipe_messages ());
    LONGS_EQUAL(-1, recv (peer.sock, &c, 1, MSG_DONTWAIT));
}

TEST(XferDcc, SendFileChild)
{
    char *str;

    open_file_read ("hello world, this is a DCC test");
    xfer.size = 31;
    peer.size = 31;

    /* Receiver sends an ACK after each block. */
    start_peer (&test_dcc_peer_receiver);
    xfer_dcc_send_file_child (&xfer);
    wait_peer ();

    LONGS_EQUAL(31, xfer.pos);
    LONGS_EQUAL(31, xfer.ack);
    LONGS_EQUAL(31, peer.received_size);
    MEMCMP_EQUAL("hello world, this is a DCC test", peer.received, 31);
    WEE_TEST_STR("20 30", pipe_messages ());
}

TEST(XferDcc, SendFileChildBlocks)
{
    char *str;

    /* Small blocks, wait for ACK before sending next block. */
    open_file_read ("hello world, this is a DCC test");
    xfer.size = 31;
    xfer.blocksize = 8;
    xfer.fast_send = 0;
    peer.size = 31;

    start_peer (&test_dcc_peer_receiver);
    xfer_dcc_send_file_child (&xfer);
    wait_peer ();

    LONGS_EQUAL(31, xfer.pos);
    LONGS_EQUAL(31, xfer.ack);
    LONGS_EQUAL(31, peer.received_size);
    MEMCMP_EQUAL("hello world, this is a DCC test", peer.received, 31);
    LONGS_EQUAL(4, peer.acks);
    str = pipe_messages ();
    CHECK(strncmp (str, "20", 2) == 0);
    CHECK(strcmp (str + strlen (str) - 2, "30") == 0);
    free (str);
}

TEST(XferDcc, SendFileChildResume)
{
    char *str;

    /* Resume: send only the end of file. */
    open_file_read ("hello world");
    xfer.size = 11;
    xfer.pos = 6;
    xfer.ack = 6;
    peer.size = 5;
    peer.ack_offset = 6;

    start_peer (&test_dcc_peer_receiver);
    xfer_dcc_send_file_child (&xfer);
    wait_peer ();

    LONGS_EQUAL(11, xfer.pos);
    LONGS_EQUAL(5, peer.received_size);
    MEMCMP_EQUAL("world", peer.received, 5);
    LONGS_EQUAL(11, xfer.ack);
    WEE_TEST_STR("20 30", pipe_messages ());
}

TEST(XferDcc, SendFileChildErrors)
{
    char *str;

    /* Receiver closed the connection without ACK. */
    open_file_read ("hello");
    xfer.size = 5;
    shutdown (peer.sock, SHUT_WR);
    xfer_dcc_send_file_child (&xfer);
    LONGS_EQUAL(5, xfer.pos);
    WEE_TEST_STR("20 42", pipe_messages ());

    /* Unable to read local file (file shorter than expected). */
    xfer.pos = 0;
    xfer.ack = 0;
    xfer.size = 10;
    close (sock[0]);
    close (sock[1]);
    new_socketpair ();
    xfer_dcc_send_file_child (&xfer);
    LONGS_EQUAL(5, xfer.pos);
    CHECK(last_message_is ("41"));

    /* Unable to send block (invalid socket). */
    xfer.pos = 0;
    xfer.ack = 0;
    xfer.size = 5;
    xfer.sock = 9999;
    xfer_dcc_send_file_child (&xfer);
    LONGS_EQUAL(0, xfer.pos);
    WEE_TEST_STR("42", pipe_messages ());
    xfer.sock = sock[0];
}

/*
 * Test functions:
 *   xfer_dcc_recv_file_send_ack
 */

TEST(XferDcc, RecvFileSendAck)
{
    char buffer[4096];
    int count;

    /* ACK sent. */
    xfer.pos = 0x01020304;
    LONGS_EQUAL(2, xfer_dcc_recv_file_send_ack (&xfer));
    LONGS_EQUAL(0x01020304, read_last_ack ());

    /* Socket error. */
    xfer.sock = 9999;
    LONGS_EQUAL(0, xfer_dcc_recv_file_send_ack (&xfer));
    xfer.sock = sock[0];

    /* Socket buffer full: ACK not sent, but not an error. */
    fcntl (xfer.sock, F_SETFL, fcntl (xfer.sock, F_GETFL) | O_NONBLOCK);
    memset (buffer, 'x', sizeof (buffer));
    for (count = 0; count < 100000; count++)
    {
        if (send (xfer.sock, buffer, 1, 0) < 0)
            break;
    }
    CHECK(count < 100000);
    LONGS_EQUAL(1, xfer_dcc_recv_file_send_ack (&xfer));
}

/*
 * Test functions:
 *   xfer_dcc_resume_hash
 */

TEST(XferDcc, ResumeHash)
{
    FILE *file;
    unsigned char *bin_hash;

    init_hash ("8587D865");

    /* Missing file. */
    xfer.temp_local_filename = strdup (filename);
    xfer.start_resume = 3;
    LONGS_EQUAL(0, xfer_dcc_resume_hash (&xfer));

    /* Hash of the start of file. */
    file = fopen (filename, "w");
    CHECK(file);
    fputs ("abcxyz", file);
    fclose (file);
    LONGS_EQUAL(1, xfer_dcc_resume_hash (&xfer));
    gcry_md_write (*xfer.hash_handle, "de", 2);
    gcry_md_final (*xfer.hash_handle);
    bin_hash = gcry_md_read (*xfer.hash_handle, 0);
    CHECK(bin_hash);
    /* CRC32 of "abcde". */
    BYTES_EQUAL(0x85, bin_hash[0]);
    BYTES_EQUAL(0x87, bin_hash[1]);
    BYTES_EQUAL(0xD8, bin_hash[2]);
    BYTES_EQUAL(0x65, bin_hash[3]);
}

/*
 * Test functions:
 *   xfer_dcc_recv_file_child
 */

TEST(XferDcc, RecvFileChild)
{
    char *str;

    xfer.type = XFER_TYPE_FILE_RECV_PASSIVE;
    xfer.file = open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    CHECK(xfer.file >= 0);
    xfer.size = 31;

    /* Sender sends the file by chunks. */
    peer.data = "hello world, this is a DCC test";
    peer.size = 31;
    peer.chunk = 10;
    start_peer (&test_dcc_peer_sender);
    xfer_dcc_recv_file_child (&xfer);
    wait_peer ();

    LONGS_EQUAL(31, xfer.pos);
    WEE_TEST_STR("hello world, this is a DCC test", file_content ());
    CHECK(peer.acks >= 1);
    LONGS_EQUAL(31, peer.last_ack);
    str = pipe_messages ();
    CHECK(strncmp (str, "20", 2) == 0);
    CHECK(strcmp (str + strlen (str) - 2, "30") == 0);
    free (str);
}

TEST(XferDcc, RecvFileChildHash)
{
    char *str;

    xfer.type = XFER_TYPE_FILE_RECV_PASSIVE;
    xfer.file = open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    CHECK(xfer.file >= 0);
    xfer.size = 5;
    xfer.send_ack = 0;

    /* CRC32 of "hello" (case-insensitive comparison). */
    init_hash ("3610a686");

    send (peer.sock, "hello", 5, 0);
    shutdown (peer.sock, SHUT_WR);
    xfer_dcc_recv_file_child (&xfer);

    LONGS_EQUAL(5, xfer.pos);
    LONGS_EQUAL(5, read_last_ack ());
    WEE_TEST_STR("20 70 30", pipe_messages ());
}

TEST(XferDcc, RecvFileChildHashMismatch)
{
    char *str;

    xfer.type = XFER_TYPE_FILE_RECV_PASSIVE;
    xfer.file = open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    CHECK(xfer.file >= 0);
    xfer.size = 5;

    init_hash ("12345678");

    send (peer.sock, "hello", 5, 0);
    shutdown (peer.sock, SHUT_WR);
    xfer_dcc_recv_file_child (&xfer);

    LONGS_EQUAL(5, xfer.pos);
    WEE_TEST_STR("20 78 30", pipe_messages ());
}

TEST(XferDcc, RecvFileChildResume)
{
    FILE *file;
    char *str;

    /* Partial file on disk: "abc", then "de" is received. */
    file = fopen (filename, "w");
    CHECK(file);
    fputs ("abc", file);
    fclose (file);

    xfer.type = XFER_TYPE_FILE_RECV_PASSIVE;
    xfer.temp_local_filename = strdup (filename);
    xfer.file = open (filename, O_APPEND | O_WRONLY);
    CHECK(xfer.file >= 0);
    xfer.start_resume = 3;
    xfer.pos = 3;
    xfer.size = 5;

    /* CRC32 of "abcde". */
    init_hash ("8587D865");

    send (peer.sock, "de", 2, 0);
    shutdown (peer.sock, SHUT_WR);
    xfer_dcc_recv_file_child (&xfer);

    LONGS_EQUAL(5, xfer.pos);
    WEE_TEST_STR("abcde", file_content ());
    WEE_TEST_STR("60 10 20 70 30", pipe_messages ());
}

TEST(XferDcc, RecvFileChildResumeHashError)
{
    char *str;

    /* Partial file missing: hash is disabled, file is received anyway. */
    xfer.type = XFER_TYPE_FILE_RECV_PASSIVE;
    xfer.temp_local_filename = strdup ("/nonexistent_dir/file.txt");
    xfer.file = open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    CHECK(xfer.file >= 0);
    xfer.start_resume = 3;
    xfer.pos = 3;
    xfer.size = 5;

    init_hash ("8587D865");

    send (peer.sock, "de", 2, 0);
    shutdown (peer.sock, SHUT_WR);
    xfer_dcc_recv_file_child (&xfer);

    POINTERS_EQUAL(NULL, xfer.hash_handle);
    LONGS_EQUAL(5, xfer.pos);
    WEE_TEST_STR("de", file_content ());
    WEE_TEST_STR("60 69 10 20 30", pipe_messages ());
}

TEST(XferDcc, RecvFileChildActive)
{
    struct sockaddr_in addr;
    socklen_t length;
    char *str;

    /* Connect to sender, listening on loopback. */
    peer.sock_listen = socket (AF_INET, SOCK_STREAM, 0);
    CHECK(peer.sock_listen >= 0);
    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    LONGS_EQUAL(0, bind (peer.sock_listen, (struct sockaddr *)&addr,
                         sizeof (addr)));
    LONGS_EQUAL(0, listen (peer.sock_listen, 1));
    length = sizeof (addr);
    LONGS_EQUAL(0, getsockname (peer.sock_listen, (struct sockaddr *)&addr,
                                &length));
    close (peer.sock);
    peer.sock = -1;

    xfer.type = XFER_TYPE_FILE_RECV_ACTIVE;
    xfer.remote_address = (struct sockaddr *)&addr;
    xfer.remote_address_length = length;
    xfer.file = open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    CHECK(xfer.file >= 0);
    xfer.size = 5;

    peer.data = "hello";
    peer.size = 5;
    peer.chunk = 5;
    start_peer (&test_dcc_peer_sender);
    xfer_dcc_recv_file_child (&xfer);
    wait_peer ();

    CHECK(xfer.sock >= 0);
    CHECK(xfer.sock != sock[0]);
    LONGS_EQUAL(5, xfer.pos);
    WEE_TEST_STR("hello", file_content ());
    LONGS_EQUAL(5, peer.last_ack);
    WEE_TEST_STR("20 30", pipe_messages ());
    xfer.remote_address = NULL;
}

TEST(XferDcc, RecvFileChildErrors)
{
    struct sockaddr_in addr;
    socklen_t length;
    int sock_closed;
    char *str;

    xfer.type = XFER_TYPE_FILE_RECV_PASSIVE;

    /* Connection closed by sender before end of file. */
    xfer.file = open (filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    CHECK(xfer.file >= 0);
    xfer.size = 10;
    send (peer.sock, "hello", 5, 0);
    shutdown (peer.sock, SHUT_WR);
    xfer_dcc_recv_file_child (&xfer);
    LONGS_EQUAL(5, xfer.pos);
    CHECK(last_message_is ("45"));

    /* Unable to write local file. */
    close (xfer.file);
    xfer.file = open (filename, O_RDONLY);
    CHECK(xfer.file >= 0);
    xfer.pos = 0;
    close (sock[0]);
    close (sock[1]);
    new_socketpair ();
    send (peer.sock, "hello", 5, 0);
    shutdown (peer.sock, SHUT_WR);
    xfer_dcc_recv_file_child (&xfer);
    LONGS_EQUAL(0, xfer.pos);
    WEE_TEST_STR("20 46", pipe_messages ());

    /* Unable to connect to sender (port closed). */
    sock_closed = socket (AF_INET, SOCK_STREAM, 0);
    CHECK(sock_closed >= 0);
    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    LONGS_EQUAL(0, bind (sock_closed, (struct sockaddr *)&addr,
                         sizeof (addr)));
    length = sizeof (addr);
    LONGS_EQUAL(0, getsockname (sock_closed, (struct sockaddr *)&addr,
                                &length));
    xfer.type = XFER_TYPE_FILE_RECV_ACTIVE;
    xfer.remote_address = (struct sockaddr *)&addr;
    xfer.remote_address_length = length;
    xfer_dcc_recv_file_child (&xfer);
    LONGS_EQUAL(-1, xfer.sock);
    WEE_TEST_STR("44", pipe_messages ());
    xfer.remote_address = NULL;
    xfer.sock = sock[0];
    close (sock_closed);
}
