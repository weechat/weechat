/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test secured data buffer functions */

#include "CppUTest/TestHarness.h"

extern "C"
{
#include <stdlib.h>
#include <string.h>
#include "src/core/core-hashtable.h"
#include "src/core/core-secure.h"
#include "src/plugins/plugin.h"
#include "src/core/core-secure-buffer.h"
#include "src/core/core-string.h"
#include "src/gui/gui-buffer.h"
#include "src/gui/gui-color.h"
#include "src/gui/gui-line.h"

extern int secure_buffer_input_cb (const void *pointer, void *data,
                                   struct t_gui_buffer *buffer,
                                   const char *input_data);
}

TEST_GROUP(CoreSecureBuffer)
{
    char *old_passphrase = NULL;

    void setup ()
    {
        old_passphrase = (secure_passphrase) ?
            strdup (secure_passphrase) : NULL;
        free (secure_passphrase);
        secure_passphrase = NULL;
        hashtable_remove_all (secure_hashtable_data);
        hashtable_remove_all (secure_hashtable_data_encrypted);
    }

    void teardown ()
    {
        if (secure_buffer)
            gui_buffer_close (secure_buffer);
        hashtable_remove_all (secure_hashtable_data);
        hashtable_remove_all (secure_hashtable_data_encrypted);
        free (secure_passphrase);
        secure_passphrase = old_passphrase;
        old_passphrase = NULL;
        secure_buffer_display_values = 0;
    }

    /*
     * Return content of secured data buffer (lines without colors separated
     * by "\n").
     *
     * Note: result must be freed after use.
     */

    char *get_content ()
    {
        struct t_gui_line *ptr_line;
        char **content, *message;

        content = string_dyn_alloc (256);
        for (ptr_line = secure_buffer->own_lines->first_line; ptr_line;
             ptr_line = ptr_line->next_line)
        {
            message = gui_color_decode (ptr_line->data->message, NULL);
            string_dyn_concat (content, (message) ? message : "", -1);
            string_dyn_concat (content, "\n", -1);
            free (message);
        }
        return string_dyn_free (content, 0);
    }
};

/*
 * Test functions:
 *   secure_buffer_open
 *   secure_buffer_display
 */

TEST(CoreSecureBuffer, OpenNoData)
{
    char *content;

    /* Display without buffer: nothing is done. */
    POINTERS_EQUAL(NULL, secure_buffer);
    secure_buffer_display ();

    secure_buffer_open ();
    CHECK(secure_buffer);
    POINTERS_EQUAL(secure_buffer,
                   gui_buffer_search (NULL, SECURE_BUFFER_NAME));
    LONGS_EQUAL(GUI_BUFFER_TYPE_FREE, secure_buffer->type);
    STRCMP_EQUAL(SECURE_BUFFER_NAME, secure_buffer->short_name);
    STRCMP_EQUAL("1", (const char *)hashtable_get (
                     secure_buffer->local_variables, "no_log"));
    STRCMP_EQUAL("WeeChat secured data (sec.conf) | Keys: [alt-v] Toggle "
                 "values", secure_buffer->title);

    content = get_content ();
    STRCMP_EQUAL("Hash algo: sha256  Cipher: aes256  Salt: on\n"
                 "\n"
                 "Passphrase is not set\n"
                 "\n"
                 "No secured data set\n",
                 content);
    free (content);

    /* Open again: same buffer. */
    secure_buffer_open ();
    POINTERS_EQUAL(secure_buffer,
                   gui_buffer_search (NULL, SECURE_BUFFER_NAME));
}

/*
 * Test functions:
 *   secure_buffer_display
 *   secure_buffer_display_data
 */

TEST(CoreSecureBuffer, DisplayData)
{
    char *content;

    secure_passphrase = strdup ("test");
    hashtable_set (secure_hashtable_data, "name2", "value2");
    hashtable_set (secure_hashtable_data, "name1", "value1");
    hashtable_set (secure_hashtable_data_encrypted, "name3", "0123");

    secure_buffer_open ();
    CHECK(secure_buffer);

    /* Values are hidden by default, data is sorted. */
    content = get_content ();
    STRCMP_EQUAL("Hash algo: sha256  Cipher: aes256  Salt: on\n"
                 "\n"
                 "Passphrase is set\n"
                 "\n"
                 "Secured data:\n"
                 "\n"
                 "  name1\n"
                 "  name2\n"
                 "\n"
                 "Secured data STILL ENCRYPTED: (use /secure decrypt, see "
                 "/help secure)\n"
                 "\n"
                 "  name3\n",
                 content);
    free (content);

    /* Display values (not for encrypted data). */
    secure_buffer_display_values = 1;
    secure_buffer_display ();
    content = get_content ();
    CHECK(strstr (content, "  name1 = \"value1\"\n  name2 = \"value2\"\n"));
    CHECK(strstr (content, "\n  name3\n"));
    POINTERS_EQUAL(NULL, strstr (content, "0123"));
    free (content);
}

/*
 * Test functions:
 *   secure_buffer_input_cb
 *   secure_buffer_close_cb
 */

TEST(CoreSecureBuffer, InputClose)
{
    secure_buffer_open ();
    CHECK(secure_buffer);

    /* Any input except "q" is ignored. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                secure_buffer_input_cb (NULL, NULL, secure_buffer, "test"));
    CHECK(secure_buffer);

    /* "q" closes the buffer. */
    LONGS_EQUAL(WEECHAT_RC_OK,
                secure_buffer_input_cb (NULL, NULL, secure_buffer, "q"));
    POINTERS_EQUAL(NULL, secure_buffer);
    POINTERS_EQUAL(NULL, gui_buffer_search (NULL, SECURE_BUFFER_NAME));
}

/*
 * Test functions:
 *   secure_buffer_assign
 */

TEST(CoreSecureBuffer, Assign)
{
    struct t_gui_buffer *buffer;

    /* No buffer: nothing is assigned. */
    secure_buffer_assign ();
    POINTERS_EQUAL(NULL, secure_buffer);

    /* Buffer exists (for example after /upgrade). */
    buffer = gui_buffer_new (NULL, SECURE_BUFFER_NAME,
                             NULL, NULL, NULL, NULL, NULL, NULL);
    CHECK(buffer);
    POINTERS_EQUAL(NULL, secure_buffer);
    secure_buffer_assign ();
    POINTERS_EQUAL(buffer, secure_buffer);
    POINTERS_EQUAL(&secure_buffer_input_cb, buffer->input_callback);
    CHECK(buffer->close_callback);

    /* Already assigned: nothing changes. */
    secure_buffer_assign ();
    POINTERS_EQUAL(buffer, secure_buffer);

    gui_buffer_close (buffer);
    POINTERS_EQUAL(NULL, secure_buffer);
}
