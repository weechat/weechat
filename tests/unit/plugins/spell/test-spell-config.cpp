/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Test spell configuration functions */

#include "CppUTest/TestHarness.h"

#include "tests.h"

extern "C"
{
#include "src/core/core-hashtable.h"
#include "src/plugins/spell/spell.h"
#include "src/plugins/spell/spell-speller.h"
}

int spell_spellers_freed = 0;

TEST_GROUP(SpellConfig)
{
    t_hashtable_free_value *callback_free_value_old;

    /*
     * Count spellers removed from hashtable "spell_spellers"
     * (the fake spellers added by tests must not be freed).
     */

    static void
    free_value_cb (struct t_hashtable *hashtable,
                   const void *key, void *value)
    {
        (void) hashtable;
        (void) key;
        (void) value;

        spell_spellers_freed++;
    }

    void setup ()
    {
        /* dictionary "en" is used, so its speller is not "unused" */
        run_cmd_quiet ("/set spell.check.default_dict \"en\"");
        hashtable_remove_all (spell_spellers);
        callback_free_value_old = spell_spellers->callback_free_value;
        spell_spellers->callback_free_value = &free_value_cb;
        spell_spellers_freed = 0;
    }

    void teardown ()
    {
        hashtable_remove_all (spell_spellers);
        spell_spellers->callback_free_value = callback_free_value_old;
        run_cmd_quiet ("/unset spell.check.default_dict");
    }

    void add_fake_speller ()
    {
        hashtable_set (spell_spellers, "en", (void *)0x1);
        LONGS_EQUAL(1, spell_spellers->items_count);
    }
};

/*
 * Test functions:
 *   spell_config_option_change
 *   spell_config_option_delete_option
 *   spell_config_option_create_option
 */

TEST(SpellConfig, OptionCreateChangeDelete)
{
    /* create option: spellers are removed */
    add_fake_speller ();
    run_cmd_quiet ("/set spell.option.ignore-case \"true\"");
    LONGS_EQUAL(0, spell_spellers->items_count);
    LONGS_EQUAL(1, spell_spellers_freed);

    /* change option: spellers are removed */
    add_fake_speller ();
    run_cmd_quiet ("/set spell.option.ignore-case \"false\"");
    LONGS_EQUAL(0, spell_spellers->items_count);
    LONGS_EQUAL(2, spell_spellers_freed);

    /* delete option: spellers are removed */
    add_fake_speller ();
    run_cmd_quiet ("/unset spell.option.ignore-case");
    LONGS_EQUAL(0, spell_spellers->items_count);
    LONGS_EQUAL(3, spell_spellers_freed);
}
