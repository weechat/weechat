/*
 * SPDX-FileCopyrightText: 2026 Nils Görs <weechatter@arcor.de>
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Support of IRCv3 echo-message in OTR plugin */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../weechat-plugin.h"
#include "otr.h"
#include "otr-echo.h"
#include "otr-irc.h"
#include "otr-message.h"
#include "otr-util.h"


struct t_otr_echo_pending otr_echo_pending_queue[OTR_ECHO_PENDING_MAX];
int otr_echo_pending_count = 0;


/*
 * Remove a pending message (by index) from the queue.
 */

void
otr_echo_pending_drop (int index)
{
    free (otr_echo_pending_queue[index].server);
    free (otr_echo_pending_queue[index].peer);
    free (otr_echo_pending_queue[index].cipher);
    free (otr_echo_pending_queue[index].plain);
    memmove (&otr_echo_pending_queue[index],
             &otr_echo_pending_queue[index + 1],
             (size_t)(otr_echo_pending_count - index - 1)
             * sizeof (struct t_otr_echo_pending));
    otr_echo_pending_count--;
    memset (&otr_echo_pending_queue[otr_echo_pending_count], 0,
            sizeof (struct t_otr_echo_pending));
}

/*
 * Remove all pending messages.
 */

void
otr_echo_pending_clear_all (void)
{
    while (otr_echo_pending_count > 0)
    {
        otr_echo_pending_drop (0);
    }
}

/*
 * Remove pending messages older than OTR_ECHO_PENDING_TTL seconds.
 */

void
otr_echo_pending_purge (void)
{
    time_t time_now;
    int i;

    time_now = time (NULL);

    i = 0;
    while (i < otr_echo_pending_count)
    {
        if (time_now - otr_echo_pending_queue[i].date > OTR_ECHO_PENDING_TTL)
            otr_echo_pending_drop (i);
        else
            i++;
    }
}

/*
 * Remember a message sent (OTR message and plain text), so that the echo of the
 * message by the server (IRCv3 capability "echo-message") can be displayed as
 * plain text.
 *
 * If the queue is full, the oldest message is removed.
 */

void
otr_echo_pending_add (const char *server, const char *peer, const char *cipher,
                      const char *plain)
{
    struct t_otr_echo_pending *ptr_pending;

    otr_echo_pending_purge ();
    if (otr_echo_pending_count == OTR_ECHO_PENDING_MAX)
        otr_echo_pending_drop (0);

    ptr_pending = &otr_echo_pending_queue[otr_echo_pending_count];
    ptr_pending->server = otr_util_dupstr (server);
    ptr_pending->peer = otr_irc_nick_tolower (server, peer);
    ptr_pending->cipher = otr_util_dupstr (cipher);
    ptr_pending->plain = otr_util_dupstr (plain);
    ptr_pending->date = time (NULL);
    if (ptr_pending->server && ptr_pending->peer && ptr_pending->cipher
        && ptr_pending->plain)
    {
        otr_echo_pending_count++;
    }
    else
    {
        free (ptr_pending->server);
        free (ptr_pending->peer);
        free (ptr_pending->cipher);
        free (ptr_pending->plain);
        memset (ptr_pending, 0, sizeof (*ptr_pending));
    }
}

/*
 * Get the length of a string without trailing spaces and tabs.
 */

size_t
otr_echo_trimmed_len (const char *string)
{
    size_t length;

    length = strlen (string);
    while ((length > 0)
           && ((string[length - 1] == ' ') || (string[length - 1] == '\t')))
    {
        length--;
    }

    return length;
}

/*
 * Search a pending message by server, target and OTR message, and remove it
 * from the queue.
 *
 * Return the plain text, NULL if not found.
 *
 * Note: result must be freed after use.
 */

char *
otr_echo_pending_take (const char *server, const char *target,
                       const char *cipher)
{
    char *plain, *peer;
    size_t cipher_length;
    int i;

    otr_echo_pending_purge ();

    peer = otr_irc_nick_tolower (server, target);
    if (!peer)
        return NULL;

    cipher_length = otr_echo_trimmed_len (cipher);

    plain = NULL;
    for (i = 0; i < otr_echo_pending_count; i++)
    {
        if ((strcmp (otr_echo_pending_queue[i].server, server) == 0)
            && (strcmp (otr_echo_pending_queue[i].peer, peer) == 0)
            && (otr_echo_trimmed_len (
                    otr_echo_pending_queue[i].cipher) == cipher_length)
            && (memcmp (otr_echo_pending_queue[i].cipher, cipher,
                        cipher_length) == 0))
        {
            plain = otr_echo_pending_queue[i].plain;
            otr_echo_pending_queue[i].plain = NULL;
            otr_echo_pending_drop (i);
            break;
        }
    }

    free (peer);

    return plain;
}

/*
 * Replace the echo of an OTR message sent (IRCv3 capability "echo-message") by
 * the plain text that was sent.
 *
 * Return NULL to leave the echoed message unchanged (not an OTR message), an
 * empty string to hide it (OTR protocol message or unknown message), otherwise
 * the new IRC message(s).
 *
 * Note: result must be freed after use.
 */

char *
otr_echo_replace (const char *server, const char *message, const char *host,
                  const char *target, const char *text)
{
    char *plain, *tags, *result;

    if (strncmp (text, "?OTR", 4) != 0)
        return NULL;

    plain = otr_echo_pending_take (server, target, text);
    if (!plain)
        return strdup ("");

    tags = otr_message_get_tags (message);
    result = otr_message_build_in_privmsgs ((tags) ? tags : "", host, target,
                                            plain, "", 1);

    free (tags);
    free (plain);

    return (result) ? result : strdup ("");
}
