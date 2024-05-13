/*
 * test-irc-command.cpp - test IRC commands
 *
 * Copyright (C) 2024 Sébastien Helleu <flashcode@flashtux.org>
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

extern "C"
{
#include <string.h>
#include "src/core/core-string.h"
#include "src/plugins/irc/irc-command.h"

extern char **irc_command_mode_masks_convert_ranges (char **argv, int arg_start);
}

TEST_GROUP(IrcCommand)
{
};

/*
 * Tests functions:
 *   irc_command_mode_nicks
 */

TEST(IrcCommand, ModeNicks)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_mode_masks_convert_ranges
 */

TEST(IrcCommand, ModeMasksConvertRanges)
{
    char **args, **masks;

    POINTERS_EQUAL(NULL, irc_command_mode_masks_convert_ranges (NULL, 0));

    args = string_split ("test", " ", NULL, 0, 0, NULL);
    POINTERS_EQUAL(NULL, irc_command_mode_masks_convert_ranges (args, -1));
    masks = irc_command_mode_masks_convert_ranges (args, 0);
    CHECK(masks);
    STRCMP_EQUAL("test", masks[0]);
    POINTERS_EQUAL(NULL, masks[1]);
    string_free_split (masks);
    string_free_split (args);

    args = string_split ("3 test 6-9 5-1 -2 64-", " ", NULL, 0, 0, NULL);
    masks = irc_command_mode_masks_convert_ranges (args, 0);
    CHECK(masks);
    STRCMP_EQUAL("3", masks[0]);
    STRCMP_EQUAL("test", masks[1]);
    STRCMP_EQUAL("6", masks[2]);
    STRCMP_EQUAL("7", masks[3]);
    STRCMP_EQUAL("8", masks[4]);
    STRCMP_EQUAL("9", masks[5]);
    STRCMP_EQUAL("5-1", masks[6]);
    STRCMP_EQUAL("-2", masks[7]);
    STRCMP_EQUAL("64-", masks[8]);
    POINTERS_EQUAL(NULL, masks[9]);
    string_free_split (masks);
    string_free_split (args);

    args = string_split ("4-10", " ", NULL, 0, 0, NULL);
    masks = irc_command_mode_masks_convert_ranges (args, 0);
    CHECK(masks);
    STRCMP_EQUAL("4", masks[0]);
    STRCMP_EQUAL("5", masks[1]);
    STRCMP_EQUAL("6", masks[2]);
    STRCMP_EQUAL("7", masks[3]);
    STRCMP_EQUAL("8", masks[4]);
    STRCMP_EQUAL("9", masks[5]);
    STRCMP_EQUAL("10", masks[6]);
    POINTERS_EQUAL(NULL, masks[7]);
    string_free_split (masks);
    string_free_split (args);
}

/*
 * Tests functions:
 *   irc_command_mode_masks
 */

TEST(IrcCommand, ModeMasks)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_me_channel_message
 */

TEST(IrcCommand, MeChannelMessage)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_me_channel
 */

TEST(IrcCommand, MeChannel)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_me_all_channels
 */

TEST(IrcCommand, MeAllChannels)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_action
 */

TEST(IrcCommand, Action)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_admin
 */

TEST(IrcCommand, Admin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_exec_buffers
 */

TEST(IrcCommand, ExecBuffers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_exec_all_channels
 */

TEST(IrcCommand, ExecAllChannels)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_allchan
 */

TEST(IrcCommand, Allchan)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_allpv
 */

TEST(IrcCommand, Allpv)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_exec_all_servers
 */

TEST(IrcCommand, ExecAllServers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_allserv
 */

TEST(IrcCommand, Allserv)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_auth
 */

TEST(IrcCommand, Auth)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_autojoin
 */

TEST(IrcCommand, Autojoin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_display_away
 */

TEST(IrcCommand, DisplayAway)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_away_server
 */

TEST(IrcCommand, AwayServer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_away
 */

TEST(IrcCommand, Away)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_run_away
 */

TEST(IrcCommand, RunAway)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_send_ban
 */

TEST(IrcCommand, SendBan)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_ban
 */

TEST(IrcCommand, Ban)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_cap
 */

TEST(IrcCommand, Cap)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_connect_one_server
 */

TEST(IrcCommand, ConnectOneServer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_connect
 */

TEST(IrcCommand, Connect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_ctcp
 */

TEST(IrcCommand, Ctcp)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_cycle
 */

TEST(IrcCommand, Cycle)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_dcc
 */

TEST(IrcCommand, Dcc)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_dehalfop
 */

TEST(IrcCommand, Dehalfop)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_deop
 */

TEST(IrcCommand, Deop)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_devoice
 */

TEST(IrcCommand, Devoice)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_die
 */

TEST(IrcCommand, Die)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_quit_server
 */

TEST(IrcCommand, QuitServer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_disconnect_one_server
 */

TEST(IrcCommand, DisconnectOneServer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_disconnect
 */

TEST(IrcCommand, Disconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_halfop
 */

TEST(IrcCommand, Halfop)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_ignore_display
 */

TEST(IrcCommand, IgnoreDisplay)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_ignore
 */

TEST(IrcCommand, Ignore)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_info
 */

TEST(IrcCommand, Info)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_invite
 */

TEST(IrcCommand, Invite)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_ison
 */

TEST(IrcCommand, Ison)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_join_server
 */

TEST(IrcCommand, JoinServer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_join
 */

TEST(IrcCommand, Join)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_kick_channel
 */

TEST(IrcCommand, KickChannel)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_kick
 */

TEST(IrcCommand, Kick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_kickban
 */

TEST(IrcCommand, Kickban)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_kill
 */

TEST(IrcCommand, Kill)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_knock
 */

TEST(IrcCommand, Knock)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_links
 */

TEST(IrcCommand, Links)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_list_get_int_arg
 */

TEST(IrcCommand, ListGetIntArg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_list
 */

TEST(IrcCommand, List)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_lusers
 */

TEST(IrcCommand, Lusers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_map
 */

TEST(IrcCommand, Map)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_me
 */

TEST(IrcCommand, Me)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_mode_server
 */

TEST(IrcCommand, ModeServer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_mode
 */

TEST(IrcCommand, Mode)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_motd
 */

TEST(IrcCommand, Motd)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_msg
 */

TEST(IrcCommand, Msg)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_names
 */

TEST(IrcCommand, Names)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_nick_server
 */

TEST(IrcCommand, NickServer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_nick
 */

TEST(IrcCommand, Nick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_notice
 */

TEST(IrcCommand, Notice)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_notify
 */

TEST(IrcCommand, Notify)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_op
 */

TEST(IrcCommand, Op)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_oper
 */

TEST(IrcCommand, Oper)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_part_channel
 */

TEST(IrcCommand, PartChannel)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_part
 */

TEST(IrcCommand, Part)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_ping
 */

TEST(IrcCommand, Ping)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_pong
 */

TEST(IrcCommand, Pong)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_query
 */

TEST(IrcCommand, Query)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_quiet
 */

TEST(IrcCommand, Quiet)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_quote
 */

TEST(IrcCommand, Quote)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_reconnect_one_server
 */

TEST(IrcCommand, ReconnectOneServer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_reconnect
 */

TEST(IrcCommand, Reconnect)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_rehash
 */

TEST(IrcCommand, Rehash)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_remove
 */

TEST(IrcCommand, Remove)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_restart
 */

TEST(IrcCommand, Restart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_rules
 */

TEST(IrcCommand, Rules)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_sajoin
 */

TEST(IrcCommand, Sajoin)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_samode
 */

TEST(IrcCommand, Samode)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_sanick
 */

TEST(IrcCommand, Sanick)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_sapart
 */

TEST(IrcCommand, Sapart)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_saquit
 */

TEST(IrcCommand, Saquit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_display_server
 */

TEST(IrcCommand, DisplayServer)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_server
 */

TEST(IrcCommand, Server)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_service
 */

TEST(IrcCommand, Service)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_servlist
 */

TEST(IrcCommand, Servlist)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_squery
 */

TEST(IrcCommand, Squery)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_setname
 */

TEST(IrcCommand, Setname)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_squit
 */

TEST(IrcCommand, Squit)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_stats
 */

TEST(IrcCommand, Stats)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_summon
 */

TEST(IrcCommand, Summon)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_time
 */

TEST(IrcCommand, Time)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_topic
 */

TEST(IrcCommand, Topic)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_trace
 */

TEST(IrcCommand, Trace)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_unban
 */

TEST(IrcCommand, Unban)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_unquiet
 */

TEST(IrcCommand, Unquiet)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_userhost
 */

TEST(IrcCommand, Userhost)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_users
 */

TEST(IrcCommand, Users)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_version
 */

TEST(IrcCommand, Version)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_voice
 */

TEST(IrcCommand, Voice)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_wallchops
 */

TEST(IrcCommand, Wallchops)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_wallops
 */

TEST(IrcCommand, Wallops)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_who
 */

TEST(IrcCommand, Who)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_whois
 */

TEST(IrcCommand, Whois)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_whowas
 */

TEST(IrcCommand, Whowas)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   irc_command_init
 */

TEST(IrcCommand, Init)
{
    /* TODO: write tests */
}
