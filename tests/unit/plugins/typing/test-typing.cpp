/*
 * test-typing.cpp - test typing functions
 *
 * Copyright (C) 2021-2023 Sébastien Helleu <flashcode@flashtux.org>
 *
 * This file is part of WeeChat, the extensible chat client.
 *
 * WeeChat is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * WeeChat is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "CppUTest/TestHarness.h"

#include "tests/tests.h"

extern "C"
{
#include "src/plugins/typing/typing.h"
}

TEST_GROUP(Typing)
{
};

/*
 * Tests functions:
 *   typing_send_signal
 */

TEST(Typing, SendSignal)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_buffer_closing_signal_cb
 */

TEST(Typing, BufferClosingSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_input_text_changed_signal_cb
 */

TEST(Typing, InputTextChangedSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_input_text_for_buffer_modifier_cb
 */

TEST(Typing, InputTextForBufferModifierCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_status_self_status_map_cb
 */

TEST(Typing, StatusSelfStatusMapCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_status_nicks_status_map_cb
 */

TEST(Typing, StatusNicksStatusMapCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_status_nicks_hash_map_cb
 */

TEST(Typing, StatusNicksHashMapCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_timer_cb
 */

TEST(Typing, TimerCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_typing_set_nick_signal_cb
 */

TEST(Typing, TypingSetNickSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_typing_reset_buffer_signal_cb
 */

TEST(Typing, TypingResetBufferSignalCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   typing_setup_hooks
 */

TEST(Typing, SetupHooks)
{
    /* TODO: write tests */
}
