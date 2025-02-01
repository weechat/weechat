/*
 * test-relay-remote-event.cpp - test event functions for relay remote
 *
 * Copyright (C) 2024-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <cjson/cJSON.h>
#include "src/core/core-config-file.h"
#include "src/plugins/relay/relay.h"
#include "src/plugins/relay/api/remote/relay-remote-event.h"
}

TEST_GROUP(RelayRemoteEvent)
{
};

/*
 * Tests functions:
 *   relay_remote_event_search_buffer
 */

TEST(RelayRemoteEvent, SearchBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_get_buffer_id
 */

TEST(RelayRemoteEvent, GetBufferId)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_build_string_tags
 */

TEST(RelayRemoteEvent, BuildStringTags)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_line_add
 */

TEST(RelayRemoteEvent, LineAdd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_search_line_by_id
 */

TEST(RelayRemoteEvent, SearchLineById)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_line_update
 */

TEST(RelayRemoteEvent, LineUpdate)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_line
 */

TEST(RelayRemoteEvent, CbLine)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_handle_nick
 */

TEST(RelayRemoteEvent, HandleNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_handle_nick_group
 */

TEST(RelayRemoteEvent, HandleNickGroup)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_nick_group
 */

TEST(RelayRemoteEvent, CbNickGroup)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_nick
 */

TEST(RelayRemoteEvent, CbNick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_apply_props
 */

TEST(RelayRemoteEvent, ApplyProps)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_buffer_input_cb
 */

TEST(RelayRemoteEvent, BufferInputCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_check_local_var
 */

TEST(RelayRemoteEvent, CheckLocalVar)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_remove_localvar_cb
 */

TEST(RelayRemoteEvent, RemoveLocalVarCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_initial_sync_buffers
 */

TEST(RelayRemoteEvent, InitialSyncBuffers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_buffer
 */

TEST(RelayRemoteEvent, CbBuffer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_buffer_cleared
 */

TEST(RelayRemoteEvent, CbBufferCleared)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_buffer_closed
 */

TEST(RelayRemoteEvent, CbBufferClosed)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_vinput
 */

TEST(RelayRemoteEvent, CbInput)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_clear_buffers
 */

TEST(RelayRemoteEvent, ClearBuffers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_sync_with_remote
 */

TEST(RelayRemoteEvent, SyncWithRemote)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_version
 */

TEST(RelayRemoteEvent, CbVersion)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_upgrade
 */

TEST(RelayRemoteEvent, CbUpgrade)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_cb_quit
 */

TEST(RelayRemoteEvent, CbQuit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   relay_remote_event_recv
 */

TEST(RelayRemoteEvent, Recv)
{
    /* TODO: write tests */
}
