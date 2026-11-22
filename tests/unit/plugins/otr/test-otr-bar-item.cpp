/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test OTR bar item */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/core-hashtable.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/plugins/irc/irc-server.h"
#include "src/plugins/otr/otr.h"
#include "src/plugins/otr/otr-bar-item.h"
#include "src/plugins/otr/otr-irc.h"
}

TEST_GROUP(OtrBarItem)
{
    /*
     * Add a fake private key for an account in libotr (only the account and
     * protocol are used by function otrl_privkey_find).
     */
    OtrlPrivKey *add_fake_privkey (const char *account)
    {
        OtrlPrivKey *privkey;

        privkey = (OtrlPrivKey *)calloc (1, sizeof (*privkey));
        privkey->accountname = strdup (account);
        privkey->protocol = strdup (OTR_PROTOCOL);
        privkey->next = otr_us->privkey_root;
        if (privkey->next)
            privkey->next->tous = &(privkey->next);
        privkey->tous = &(otr_us->privkey_root);
        otr_us->privkey_root = privkey;
        return privkey;
    }

    /*
     * Get value of a buffer local variable.
     */
    const char *localvar (struct t_gui_buffer *buffer, const char *name)
    {
        return (const char *)hashtable_get (buffer->local_variables, name);
    }

    /*
     * Get bar item content without colors.
     */
    char *bar_item_no_color (struct t_gui_buffer *buffer)
    {
        char *str, *str_no_color;

        str = otr_bar_item_cb (NULL, NULL, NULL, NULL, buffer, NULL);
        if (!str)
            return NULL;
        str_no_color = gui_color_decode (str, NULL);
        free (str);
        return str_no_color;
    }
};

/*
 * Test functions:
 *   otr_bar_item_cb
 */

TEST(OtrBarItem, Cb)
{
    struct t_irc_server *ptr_server;
    struct t_gui_buffer *ptr_buffer;
    ConnContext *ptr_context;
    OtrlPrivKey *ptr_privkey;
    char *str, *str_no_color;

    run_cmd_quiet ("/mute /server add srv_otr_bar fake:127.0.0.1");
    run_cmd_quiet ("/connect srv_otr_bar");
    ptr_server = irc_server_search ("srv_otr_bar");
    CHECK(ptr_server);
    irc_server_set_nick (ptr_server, "alice");
    ptr_server->is_connected = 1;
    run_cmd_quiet ("/query -server srv_otr_bar bob");
    ptr_buffer = otr_irc_buffer ("srv_otr_bar", "bob");
    CHECK(ptr_buffer);

    /* Not a private buffer: empty. */
    str = otr_bar_item_cb (NULL, NULL, NULL, NULL, gui_buffers, NULL);
    STRCMP_EQUAL("", str);
    free (str);

    /* No private key for the account and no session: empty. */
    POINTERS_EQUAL(NULL, otrl_privkey_find (otr_us, "alice@srv_otr_bar",
                                            OTR_PROTOCOL));
    str = otr_bar_item_cb (NULL, NULL, NULL, NULL, ptr_buffer, NULL);
    STRCMP_EQUAL("", str);
    free (str);
    STRCMP_EQUAL("false", localvar (ptr_buffer, "otr_encrypted"));
    STRCMP_EQUAL("false", localvar (ptr_buffer, "otr_authenticated"));

    /* Private key for the account, no session: unencrypted state. */
    ptr_privkey = add_fake_privkey ("alice@srv_otr_bar");
    CHECK(otrl_privkey_find (otr_us, "alice@srv_otr_bar", OTR_PROTOCOL));
    str_no_color = bar_item_no_color (ptr_buffer);
    STRCMP_EQUAL("OTR:!SEC", str_no_color);
    free (str_no_color);
    otrl_privkey_forget (ptr_privkey);
    POINTERS_EQUAL(NULL, otrl_privkey_find (otr_us, "alice@srv_otr_bar",
                                            OTR_PROTOCOL));

    /* OTR session: state displayed. */
    ptr_context = otrl_context_find (otr_us, "bob", "alice@srv_otr_bar",
                                     OTR_PROTOCOL, OTRL_INSTAG_MASTER, 1,
                                     NULL, NULL, NULL);
    CHECK(ptr_context);
    ptr_context->msgstate = OTRL_MSGSTATE_ENCRYPTED;
    str_no_color = bar_item_no_color (ptr_buffer);
    CHECK(str_no_color);
    STRNCMP_EQUAL("OTR:SEC,!AUTH,", str_no_color, 14);
    free (str_no_color);
    STRCMP_EQUAL("true", localvar (ptr_buffer, "otr_encrypted"));
    ptr_context->msgstate = OTRL_MSGSTATE_PLAINTEXT;

    run_cmd_quiet ("/mute /disconnect srv_otr_bar");
    run_cmd_quiet ("/mute /server del srv_otr_bar");
}
