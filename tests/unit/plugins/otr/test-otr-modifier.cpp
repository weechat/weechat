/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR modifier functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-context.h"
#include "src/plugins/otr/otr-modifier.h"

extern int otr_modifier_is_base64_part (const char *line);
extern int otr_modifier_assemble (struct t_otr_peer_data *peer_data,
                                  const char *line, char **message);
extern int otr_modifier_parse_out_privmsg (const char *message, char **target,
                                           const char **text);
extern int otr_modifier_all_targets_channels (const char *server,
                                              const char *targets);
}

TEST_GROUP(OtrModifier)
{
};

/*
 * Test functions:
 *   otr_modifier_is_base64_part
 */

TEST(OtrModifier, IsBase64Part)
{
    LONGS_EQUAL(0, otr_modifier_is_base64_part (""));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("."));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("a.b"));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("abc.."));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("hello world"));
    LONGS_EQUAL(0, otr_modifier_is_base64_part ("abc,"));
    LONGS_EQUAL(1, otr_modifier_is_base64_part ("abc"));
    LONGS_EQUAL(1, otr_modifier_is_base64_part ("abc."));
    LONGS_EQUAL(1, otr_modifier_is_base64_part ("AZaz09+/=="));
}

/*
 * Test functions:
 *   otr_modifier_assemble
 */

TEST(OtrModifier, Assemble)
{
    struct t_otr_peer_data peer_data;
    char *message, *line;

    memset (&peer_data, 0, sizeof (peer_data));

    /* Lines returned as-is. */
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "hello", &message));
    STRCMP_EQUAL("hello", message);
    free (message);
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "?OTRv23?", &message));
    STRCMP_EQUAL("?OTRv23?", message);
    free (message);
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data,
                                          "?OTR|1|2,00001,00002,abc,",
                                          &message));
    STRCMP_EQUAL("?OTR|1|2,00001,00002,abc,", message);
    free (message);
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "?OTR:AAMDabc.",
                                          &message));
    STRCMP_EQUAL("?OTR:AAMDabc.", message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);

    /* Plain text starting with "?OTR" must not hide next lines. */
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "?OTR is that you",
                                          &message));
    STRCMP_EQUAL("?OTR is that you", message);
    free (message);
    message = NULL;
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "hello", &message));
    STRCMP_EQUAL("hello", message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);

    /* OTR message split over 3 lines. */
    message = NULL;
    LONGS_EQUAL(0, otr_modifier_assemble (&peer_data, "?OTR:AAMDabc",
                                          &message));
    POINTERS_EQUAL(NULL, message);
    STRCMP_EQUAL("?OTR:AAMDabc", peer_data.assembly);
    LONGS_EQUAL(0, otr_modifier_assemble (&peer_data, "def", &message));
    POINTERS_EQUAL(NULL, message);
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "ghi.", &message));
    STRCMP_EQUAL("?OTR:AAMDabcdefghi.", message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);
    LONGS_EQUAL(0, peer_data.assembly_length);

    /* Incomplete OTR message dropped by a line which is not base64. */
    message = NULL;
    LONGS_EQUAL(0, otr_modifier_assemble (&peer_data, "?OTR:AAMDabc",
                                          &message));
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, "hello world",
                                          &message));
    STRCMP_EQUAL("hello world", message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);

    /* Incomplete OTR message dropped when the max size is reached. */
    line = (char *)malloc (OTR_MAX_ASSEMBLY);
    CHECK(line);
    memset (line, 'A', OTR_MAX_ASSEMBLY - 1);
    line[OTR_MAX_ASSEMBLY - 1] = '\0';
    message = NULL;
    LONGS_EQUAL(0, otr_modifier_assemble (&peer_data, "?OTR:AAMDabc",
                                          &message));
    LONGS_EQUAL(1, otr_modifier_assemble (&peer_data, line, &message));
    STRCMP_EQUAL(line, message);
    free (message);
    POINTERS_EQUAL(NULL, peer_data.assembly);
    free (line);
}

/*
 * Test functions:
 *   otr_modifier_parse_out_privmsg
 */

TEST(OtrModifier, ParseOutPrivmsg)
{
    char *target;
    const char *text;

    target = NULL;
    text = NULL;
    LONGS_EQUAL(0, otr_modifier_parse_out_privmsg ("", &target, &text));
    LONGS_EQUAL(0, otr_modifier_parse_out_privmsg ("NOTICE bob :hello",
                                                   &target, &text));
    LONGS_EQUAL(0, otr_modifier_parse_out_privmsg ("PRIVMSG bob",
                                                   &target, &text));
    POINTERS_EQUAL(NULL, target);

    LONGS_EQUAL(1, otr_modifier_parse_out_privmsg ("PRIVMSG bob :hello world",
                                                   &target, &text));
    STRCMP_EQUAL("bob", target);
    STRCMP_EQUAL("hello world", text);
    free (target);

    LONGS_EQUAL(1, otr_modifier_parse_out_privmsg ("  privmsg  bob hello",
                                                   &target, &text));
    STRCMP_EQUAL("bob", target);
    STRCMP_EQUAL("hello", text);
    free (target);
}

/*
 * Test functions:
 *   otr_modifier_all_targets_channels
 */

TEST(OtrModifier, AllTargetsChannels)
{
    LONGS_EQUAL(0, otr_modifier_all_targets_channels ("test", "bob"));
    LONGS_EQUAL(0, otr_modifier_all_targets_channels ("test", "#chan,bob"));
    LONGS_EQUAL(1, otr_modifier_all_targets_channels ("test", "#chan"));
    LONGS_EQUAL(1, otr_modifier_all_targets_channels ("test", "#a,#b"));
    LONGS_EQUAL(1, otr_modifier_all_targets_channels ("test", ",#a,,#b,"));
}
