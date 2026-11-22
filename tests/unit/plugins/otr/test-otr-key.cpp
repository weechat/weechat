/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR functions to generate private keys */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-line.h"
#include "src/plugins/weechat-plugin.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-key.h"

extern struct t_otr_key_generation *otr_key_generation_search (
    const char *account, const char *protocol);
extern void otr_key_generation_add_peer (
    struct t_otr_key_generation *generation, const char *peer);
extern int otr_key_save (const char *account, const char *protocol,
                         const char *key);
extern int otr_key_process_cb (const void *pointer, void *data,
                               const char *command, int return_code,
                               const char *out, const char *err);
}

/* DSA private key (generated once, generating a key is too slow for tests). */
#define OTR_TEST_KEY                                                    \
    "(private-key " \
    " (dsa " \
    "  (p #00C3D3F1E48C98020AC9CD5F95F707280AED289CE20E453C9E384DB4B2" \
    "C9D26B9BC8A87EC7C784DD7C1C718D1F91F4DF1EAA9CE50025403E4B997A92DC" \
    "C2CE365E88CE263449A8D781035B66C0CBAA5197F4BD39CE1835A65CADFCA7ED" \
    "28CD575A460B877706DB634C539698DF44D1E17963C03AF82629A263F46DCC4A" \
    "1202B16F#) " \
    "  (q #0090954D22E22DC4CCAF29D6F9402674070D545051#) " \
    "  (g #45F8272D7F54EF411D767F119DD077FEC977CCB08B36475D50773261D1" \
    "5CBBA3BD0B91F0AF6D8BBD113ADD82AAD89AFE5AF20704F5EB5823B498265BCD" \
    "E9204D6DB659B66282612770368D1C2808ACCBCA835CEA3B2626E0CCFEE30E0B" \
    "0509FE6657ED012EEDD41A1BA409176441A8454FF05A5328DE38CE9E96DFF868" \
    "9E8D26#) " \
    "  (y #54C79693DFC850EE12B85786690129BCADC10D14B578FA2C755B7FCD7F" \
    "5DA7EEFFB986C2E6E060E1B63976B0CDE13A56B1C55BEA361777A2C4BF0DC12F" \
    "2896B774582B82FB7A25D78C9158A257A3A361D15027EBB1CD55D078D82FF8D7" \
    "2A91D90C8DC6C28CA0DF004D2EFDBA14CB3C1D1DC0C537E1395A694959BEF9B4" \
    "4479B5#) " \
    "  (x #71908CD2C7412C4A8C42C0DB97A3F8A99004EC41#) " \
    "  ) " \
    " ) "

TEST_GROUP(OtrKey)
{
    /*
     * Check if a line containing a string has been displayed in core buffer
     * after a given line.
     */
    int core_line_displayed (struct t_gui_line *after_line, const char *string)
    {
        struct t_gui_line *ptr_line;

        for (ptr_line = gui_buffers->own_lines->last_line;
             ptr_line && (ptr_line != after_line);
             ptr_line = ptr_line->prev_line)
        {
            if (ptr_line->data->message
                && strstr (ptr_line->data->message, string))
            {
                return 1;
            }
        }
        return 0;
    }

    /*
     * Create a key generation (without process).
     */
    struct t_otr_key_generation *new_generation (const char *account)
    {
        struct t_otr_key_generation *generation;

        generation = (struct t_otr_key_generation *)calloc (
            1, sizeof (*generation));
        generation->account = strdup (account);
        generation->protocol = strdup (OTR_PROTOCOL);
        generation->output = weechat_string_dyn_alloc (64);
        generation->error = weechat_string_dyn_alloc (64);
        generation->peers = weechat_string_dyn_alloc (64);
        generation->prev_generation = last_otr_key_generation;
        if (last_otr_key_generation)
            last_otr_key_generation->next_generation = generation;
        else
            otr_key_generations = generation;
        last_otr_key_generation = generation;
        return generation;
    }
};

/*
 * Test functions:
 *   otr_key_generation_search
 *   otr_key_generation_add_peer
 *   otr_key_generation_free
 *   otr_key_generation_free_all
 */

TEST(OtrKey, Generation)
{
    struct t_otr_key_generation *generation;

    POINTERS_EQUAL(NULL, otr_key_generation_search (NULL, NULL));
    POINTERS_EQUAL(NULL, otr_key_generation_search ("alice@srv_key",
                                                    OTR_PROTOCOL));

    generation = new_generation ("alice@srv_key");
    POINTERS_EQUAL(generation,
                   otr_key_generation_search ("alice@srv_key", OTR_PROTOCOL));
    POINTERS_EQUAL(NULL, otr_key_generation_search ("alice@srv_key", "xxx"));

    /* Peers waiting for the key (no duplicate). */
    otr_key_generation_add_peer (generation, NULL);
    otr_key_generation_add_peer (generation, "");
    STRCMP_EQUAL("", *(generation->peers));
    otr_key_generation_add_peer (generation, "bob");
    otr_key_generation_add_peer (generation, "carol");
    otr_key_generation_add_peer (generation, "bob");
    STRCMP_EQUAL("bob\ncarol", *(generation->peers));

    /* Key already being generated: peer added, no new generation. */
    otr_key_generate ("alice@srv_key", OTR_PROTOCOL, "dave");
    POINTERS_EQUAL(generation, otr_key_generations);
    POINTERS_EQUAL(generation, last_otr_key_generation);
    STRCMP_EQUAL("bob\ncarol\ndave", *(generation->peers));

    otr_key_generation_free_all ();
    POINTERS_EQUAL(NULL, otr_key_generations);
    POINTERS_EQUAL(NULL, last_otr_key_generation);
}

/*
 * Test functions:
 *   otr_key_save
 *   otr_key_write_account
 */

TEST(OtrKey, Save)
{
    OtrlPrivKey *ptr_privkey;

    LONGS_EQUAL(0, otr_key_save (NULL, OTR_PROTOCOL, OTR_TEST_KEY));
    LONGS_EQUAL(0, otr_key_save ("alice@srv_key_save", NULL, OTR_TEST_KEY));
    LONGS_EQUAL(0, otr_key_save ("alice@srv_key_save", OTR_PROTOCOL, NULL));
    LONGS_EQUAL(0, otr_key_save ("alice@srv_key_save", OTR_PROTOCOL, ""));
    LONGS_EQUAL(0, otr_key_save ("alice@srv_key_save", OTR_PROTOCOL,
                                 "invalid key"));
    LONGS_EQUAL(0, otr_key_save ("alice@srv_key_save", OTR_PROTOCOL,
                                 "(public-key (dsa (y #01#)))"));
    POINTERS_EQUAL(NULL, otrl_privkey_find (otr_us, "alice@srv_key_save",
                                            OTR_PROTOCOL));

    /* Key saved and loaded in libotr. */
    LONGS_EQUAL(1, otr_key_save ("alice@srv_key_save", OTR_PROTOCOL,
                                 OTR_TEST_KEY));
    CHECK(otrl_privkey_find (otr_us, "alice@srv_key_save", OTR_PROTOCOL));

    /* Another account: both keys are kept. */
    LONGS_EQUAL(1, otr_key_save ("bob@srv_key_save", OTR_PROTOCOL,
                                 OTR_TEST_KEY));
    CHECK(otrl_privkey_find (otr_us, "alice@srv_key_save", OTR_PROTOCOL));
    CHECK(otrl_privkey_find (otr_us, "bob@srv_key_save", OTR_PROTOCOL));

    /* Same account again: key replaced (only one key for the account). */
    LONGS_EQUAL(1, otr_key_save ("alice@srv_key_save", OTR_PROTOCOL,
                                 OTR_TEST_KEY));
    CHECK(otrl_privkey_find (otr_us, "alice@srv_key_save", OTR_PROTOCOL));
    for (ptr_privkey = otr_us->privkey_root; ptr_privkey;
         ptr_privkey = ptr_privkey->next)
    {
        if (strcmp (ptr_privkey->accountname, "alice@srv_key_save") == 0)
            break;
    }
    CHECK(ptr_privkey);
    for (ptr_privkey = ptr_privkey->next; ptr_privkey;
         ptr_privkey = ptr_privkey->next)
    {
        CHECK(strcmp (ptr_privkey->accountname, "alice@srv_key_save") != 0);
    }
}

/*
 * Test functions:
 *   otr_key_save (permissions of file, symbolic link)
 */

TEST(OtrKey, SaveFile)
{
    char *tmp_filename, *target_filename;
    struct stat st;
    FILE *file;

    tmp_filename = NULL;
    target_filename = NULL;
    weechat_asprintf (&tmp_filename, "%s.tmp", otr_keyfile);
    weechat_asprintf (&target_filename, "%s.target", otr_keyfile);
    CHECK(tmp_filename);
    CHECK(target_filename);

    /* Existing temporary file readable by all: file created with mode 0600. */
    file = fopen (tmp_filename, "w");
    CHECK(file);
    fclose (file);
    chmod (tmp_filename, 0644);
    LONGS_EQUAL(1, otr_key_save ("alice@srv_key_file", OTR_PROTOCOL,
                                 OTR_TEST_KEY));
    LONGS_EQUAL(0, stat (otr_keyfile, &st));
    LONGS_EQUAL(0600, st.st_mode & 0777);
    LONGS_EQUAL(-1, access (tmp_filename, F_OK));

    /* File of private keys is a symbolic link: the link is kept. */
    LONGS_EQUAL(0, rename (otr_keyfile, target_filename));
    LONGS_EQUAL(0, symlink (target_filename, otr_keyfile));
    LONGS_EQUAL(1, otr_key_save ("bob@srv_key_file", OTR_PROTOCOL,
                                 OTR_TEST_KEY));
    LONGS_EQUAL(0, lstat (otr_keyfile, &st));
    CHECK(S_ISLNK(st.st_mode));
    LONGS_EQUAL(0, stat (target_filename, &st));
    LONGS_EQUAL(0600, st.st_mode & 0777);
    CHECK(otrl_privkey_find (otr_us, "alice@srv_key_file", OTR_PROTOCOL));
    CHECK(otrl_privkey_find (otr_us, "bob@srv_key_file", OTR_PROTOCOL));
    LONGS_EQUAL(0, unlink (otr_keyfile));
    LONGS_EQUAL(0, rename (target_filename, otr_keyfile));

    free (tmp_filename);
    free (target_filename);
}

/*
 * Test functions:
 *   otr_key_process_cb
 *   otr_key_send_queries
 */

TEST(OtrKey, ProcessCb)
{
    struct t_otr_key_generation *generation;
    struct t_gui_line *ptr_last_line;

    /* Key received in two parts, then saved; OTR query sent to the peer. */
    generation = new_generation ("alice@srv_key_cb");
    otr_key_generation_add_peer (generation, "bob");
    ptr_last_line = gui_buffers->own_lines->last_line;
    LONGS_EQUAL(WEECHAT_RC_OK,
                otr_key_process_cb (generation, NULL, "func:otr_key_generate",
                                    WEECHAT_HOOK_PROCESS_RUNNING,
                                    "(private-key ", NULL));
    POINTERS_EQUAL(generation, otr_key_generations);
    LONGS_EQUAL(WEECHAT_RC_OK,
                otr_key_process_cb (generation, NULL, "func:otr_key_generate",
                                    0, OTR_TEST_KEY + 13, NULL));
    POINTERS_EQUAL(NULL, otr_key_generations);
    CHECK(otrl_privkey_find (otr_us, "alice@srv_key_cb", OTR_PROTOCOL));
    LONGS_EQUAL(1, core_line_displayed (ptr_last_line, "OTR key generated."));
    LONGS_EQUAL(1, core_line_displayed (ptr_last_line, "Sending OTR query"));

    /* Error in child process: key not saved. */
    generation = new_generation ("alice@srv_key_cb_error");
    ptr_last_line = gui_buffers->own_lines->last_line;
    LONGS_EQUAL(WEECHAT_RC_OK,
                otr_key_process_cb (generation, NULL, "func:otr_key_generate",
                                    1, NULL, "generation error"));
    POINTERS_EQUAL(NULL, otr_key_generations);
    POINTERS_EQUAL(NULL, otrl_privkey_find (otr_us, "alice@srv_key_cb_error",
                                            OTR_PROTOCOL));
    LONGS_EQUAL(1, core_line_displayed (
                    ptr_last_line, "key generation failed: generation error"));
}
