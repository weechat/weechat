/*
 * test-trigger-config.cpp - test trigger configuration functions
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
#include <stdio.h>
#include "src/core/wee-hook.h"
#include "src/plugins/trigger/trigger.h"
}

#define WEE_CHECK_MODIFIER(__result, __modifier, __string)              \
    WEE_TEST_STR(                                                       \
        __result,                                                       \
        hook_modifier_exec (NULL, __modifier, NULL, __string));

#define WEE_CHECK_MODIFIER_INPUT(__result, __string)                    \
    WEE_CHECK_MODIFIER(__result, "input_text_display", __string)

#define WEE_CHECK_MODIFIER_MSG_AUTH(__result, __string)                 \
    WEE_CHECK_MODIFIER(__result, "irc_message_auth", __string)


TEST_GROUP(TriggerConfig)
{
};

/*
 * Tests default trigger "beep".
 */

TEST(TriggerConfig, DefaultTriggerBeep)
{
    /* TODO: write tests */
}

/*
 * Tests default trigger "cmd_pass".
 */

TEST(TriggerConfig, DefaultTriggerCmdPass)
{
    char *str;

    /*
     * /msg nickserv id <nick> <password>
     * /msg nickserv id <password>
     */
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv id",
                             "/msg nickserv id");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv id ************",
                             "/msg nickserv id alice secret");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv id ******",
                             "/msg nickserv id secret");

    /*
     * /m nickserv id <nick> <password>
     * /m nickserv id <password>
     */
    WEE_CHECK_MODIFIER_INPUT("/m nickserv id",
                             "/m nickserv id");
    WEE_CHECK_MODIFIER_INPUT("/m nickserv id ************",
                             "/m nickserv id alice secret");
    WEE_CHECK_MODIFIER_INPUT("/m nickserv id ******",
                             "/m nickserv id secret");

    /*
     * /quote nickserv id <nick> <password>
     * /quote nickserv id <password>
     */
    WEE_CHECK_MODIFIER_INPUT("/quote nickserv id",
                             "/quote nickserv id");
    WEE_CHECK_MODIFIER_INPUT("/quote nickserv id ************",
                             "/quote nickserv id alice secret");
    WEE_CHECK_MODIFIER_INPUT("/quote nickserv id ******",
                             "/quote nickserv id secret");

    /*
     * /msg -server xxx nickserv id <nick> <password>
     * /msg -server xxx nickserv id <password>
     */
    WEE_CHECK_MODIFIER_INPUT("/msg -server libera nickserv id",
                             "/msg -server libera nickserv id");
    WEE_CHECK_MODIFIER_INPUT("/msg -server libera nickserv id ************",
                             "/msg -server libera nickserv id alice secret");
    WEE_CHECK_MODIFIER_INPUT("/msg -server libera nickserv id ******",
                             "/msg -server libera nickserv id secret");

    /*
     * /m -server xxx nickserv id <nick> <password>
     * /m -server xxx nickserv id <password>
     */
    WEE_CHECK_MODIFIER_INPUT("/m -server libera nickserv id",
                             "/m -server libera nickserv id");
    WEE_CHECK_MODIFIER_INPUT("/m -server libera nickserv id ************",
                             "/m -server libera nickserv id alice secret");
    WEE_CHECK_MODIFIER_INPUT("/m -server libera nickserv id ******",
                             "/m -server libera nickserv id secret");

    /*
     * /quote -server xxx nickserv id <nick> <password>
     * /quote -server xxx nickserv id <password>
     */
    WEE_CHECK_MODIFIER_INPUT("/quote -server libera nickserv id",
                             "/quote -server libera nickserv id");
    WEE_CHECK_MODIFIER_INPUT("/quote -server libera nickserv id ************",
                             "/quote -server libera nickserv id alice secret");
    WEE_CHECK_MODIFIER_INPUT("/quote -server libera nickserv id ******",
                             "/quote -server libera nickserv id secret");

    /*
     * /msg nickserv identify <nick> <password>
     * /msg nickserv identify <password>
     */
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv identify",
                             "/msg nickserv identify");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv identify ************",
                             "/msg nickserv identify alice secret");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv identify ******",
                             "/msg nickserv identify secret");

    /* /msg nickserv set password <password> */
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv set password",
                             "/msg nickserv set password");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv set password ******",
                             "/msg nickserv set password secret");

    /* /msg nickserv ghost <nick> <password> */
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv ghost alice",
                             "/msg nickserv ghost alice");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv ghost alice ******",
                             "/msg nickserv ghost alice secret");

    /* /msg nickserv release <nick> <password> */
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv release alice",
                             "/msg nickserv release alice");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv release alice ******",
                             "/msg nickserv release alice secret");

    /* /msg nickserv regain <nick> <password> */
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv regain alice",
                             "/msg nickserv regain alice");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv regain alice ******",
                             "/msg nickserv regain alice secret");

    /* /msg nickserv recover <nick> <password> */
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv recover alice",
                             "/msg nickserv recover alice");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv recover alice ******",
                             "/msg nickserv recover alice secret");

    /* /msg nickserv setpass <nick> <key> <password> */
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv setpass alice",
                             "/msg nickserv setpass alice");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv setpass alice **********",
                             "/msg nickserv setpass alice key secret");

    /* /oper <nick> <password> */
    WEE_CHECK_MODIFIER_INPUT("/oper alice",
                             "/oper alice");
    WEE_CHECK_MODIFIER_INPUT("/oper alice ******",
                             "/oper alice secret");

    /* /quote pass <password> */
    WEE_CHECK_MODIFIER_INPUT("/quote pass",
                             "/quote pass");
    WEE_CHECK_MODIFIER_INPUT("/quote pass ******",
                             "/quote pass secret");

    /* /secure passphrase <pasphrase> */
    WEE_CHECK_MODIFIER_INPUT("/secure passphrase",
                             "/secure passphrase");
    WEE_CHECK_MODIFIER_INPUT("/secure passphrase **********************",
                             "/secure passphrase this is the passphrase");

    /* /secure decrypt <pasphrase> */
    WEE_CHECK_MODIFIER_INPUT("/secure decrypt",
                             "/secure decrypt");
    WEE_CHECK_MODIFIER_INPUT("/secure decrypt **********************",
                             "/secure decrypt this is the passphrase");

    /* /secure set <name> <value> */
    WEE_CHECK_MODIFIER_INPUT("/secure set name",
                             "/secure set name");
    WEE_CHECK_MODIFIER_INPUT("/secure set name ******",
                             "/secure set name secret");

    /* modifier "history_add" */
    WEE_CHECK_MODIFIER("/msg nickserv identify ************",
                       "history_add",
                       "/msg nickserv identify alice secret");

    /* modifier "irc_command_auth" */
    WEE_CHECK_MODIFIER("/msg nickserv identify ************",
                       "irc_command_auth",
                       "/msg nickserv identify alice secret");
}

/*
 * Tests default trigger "cmd_pass_register".
 */

TEST(TriggerConfig, DefaultTriggerCmdPassRegister)
{
    char *str;

    /* /msg nickserv register <password> <email> */
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv register",
                             "/msg nickserv register");
    WEE_CHECK_MODIFIER_INPUT("/msg nickserv register ****** test@example.com",
                             "/msg nickserv register secret test@example.com");

    /* /m nickserv register <password> <email> */
    WEE_CHECK_MODIFIER_INPUT("/m nickserv register",
                             "/m nickserv register");
    WEE_CHECK_MODIFIER_INPUT("/m nickserv register ****** test@example.com",
                             "/m nickserv register secret test@example.com");

    /* /quote nickserv register <password> <email> */
    WEE_CHECK_MODIFIER_INPUT("/quote nickserv register",
                             "/quote nickserv register");
    WEE_CHECK_MODIFIER_INPUT("/quote nickserv register ****** test@example.com",
                             "/quote nickserv register secret test@example.com");

    /* /msg -server ccc nickserv register <password> <email> */
    WEE_CHECK_MODIFIER_INPUT("/quote -server libera nickserv register",
                             "/quote -server libera nickserv register");
    WEE_CHECK_MODIFIER_INPUT("/quote -server libera nickserv register ****** test@example.com",
                             "/quote -server libera nickserv register secret test@example.com");

    /* /m -server ccc nickserv register <password> <email> */
    WEE_CHECK_MODIFIER_INPUT("/m -server libera nickserv register",
                             "/m -server libera nickserv register");
    WEE_CHECK_MODIFIER_INPUT("/m -server libera nickserv register ****** test@example.com",
                             "/m -server libera nickserv register secret test@example.com");

    /* /quote -server ccc nickserv register <password> <email> */
    WEE_CHECK_MODIFIER_INPUT("/quote -server libera nickserv register",
                             "/quote -server libera nickserv register");
    WEE_CHECK_MODIFIER_INPUT("/quote -server libera nickserv register ****** test@example.com",
                             "/quote -server libera nickserv register secret test@example.com");

    /* modifier "history_add" */
    WEE_CHECK_MODIFIER("/msg nickserv register ****** test@example.com",
                       "history_add",
                       "/msg nickserv register secret test@example.com");

    /* modifier "irc_command_auth" */
    WEE_CHECK_MODIFIER("/msg nickserv register ****** test@example.com",
                       "irc_command_auth",
                       "/msg nickserv register secret test@example.com");

}

/*
 * Tests default trigger "msg_auth".
 */

TEST(TriggerConfig, DefaultTriggerMsgAuth)
{
    char *str;

    /* id <password> */
    WEE_CHECK_MODIFIER_MSG_AUTH("id", "id");
    WEE_CHECK_MODIFIER_MSG_AUTH("id ******", "id secret");

    /* identify <password> */
    WEE_CHECK_MODIFIER_MSG_AUTH("identify", "identify");
    WEE_CHECK_MODIFIER_MSG_AUTH("identify ******", "identify secret");

    /* set password <password> */
    WEE_CHECK_MODIFIER_MSG_AUTH("set password", "set password");
    WEE_CHECK_MODIFIER_MSG_AUTH("set password ******", "set password secret");

    /* register <password> */
    WEE_CHECK_MODIFIER_MSG_AUTH("register", "register");
    WEE_CHECK_MODIFIER_MSG_AUTH("register ******", "register secret");

    /* ghost <nick> <password> */
    WEE_CHECK_MODIFIER_MSG_AUTH("ghost alice", "ghost alice");
    WEE_CHECK_MODIFIER_MSG_AUTH("ghost alice ******", "ghost alice secret");

    /* release <nick> <password> */
    WEE_CHECK_MODIFIER_MSG_AUTH("release alice", "release alice");
    WEE_CHECK_MODIFIER_MSG_AUTH("release alice ******", "release alice secret");

    /* regain <nick> <password> */
    WEE_CHECK_MODIFIER_MSG_AUTH("regain alice", "regain alice");
    WEE_CHECK_MODIFIER_MSG_AUTH("regain alice ******", "regain alice secret");

    /* recover <nick> <password> */
    WEE_CHECK_MODIFIER_MSG_AUTH("recover alice", "recover alice");
    WEE_CHECK_MODIFIER_MSG_AUTH("recover alice ******", "recover alice secret");
}

/*
 * Tests default trigger "server_pass".
 */

TEST(TriggerConfig, DefaultTriggerServerPass)
{
    char *str;

    /*
     * /server add <name> -password=xxx
     * /server add <name> -sasl_password=xxx
     */
    WEE_CHECK_MODIFIER_INPUT(
        "/server add libera irc.libera.chat",
        "/server add libera irc.libera.chat");
    WEE_CHECK_MODIFIER_INPUT(
        "/server add libera irc.libera.chat -password=******",
        "/server add libera irc.libera.chat -password=secret");
    WEE_CHECK_MODIFIER_INPUT(
        "/server add libera irc.libera.chat -sasl_password=******",
        "/server add libera irc.libera.chat -sasl_password=secret");

    /*
     * /connect <address> -password=xxx
     * /connect <address> -sasl_password=xxx
     */
    WEE_CHECK_MODIFIER_INPUT(
        "/connect irc.libera.chat",
        "/connect irc.libera.chat");
    WEE_CHECK_MODIFIER_INPUT(
        "/connect irc.libera.chat -password=******",
        "/connect irc.libera.chat -password=secret");
    WEE_CHECK_MODIFIER_INPUT(
        "/connect irc.libera.chat -sasl_password=******",
        "/connect irc.libera.chat -sasl_password=secret");
}

/*
 * Tests functions:
 *   trigger_config_change_enabled
 */

TEST(TriggerConfig, ChangeEnabled)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_change_trigger_enabled
 */

TEST(TriggerConfig, ChangeTriggerEnabled)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_change_trigger_hook
 */

TEST(TriggerConfig, ChangeTriggerHook)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_change_trigger_arguments
 */

TEST(TriggerConfig, ChangeTriggerArguments)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_change_trigger_regex
 */

TEST(TriggerConfig, ChangeTriggerRegex)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_change_trigger_command
 */

TEST(TriggerConfig, ChangeTriggerCommand)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_create_trigger_option
 */

TEST(TriggerConfig, ChangeTriggerOption)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_create_option_temp
 */

TEST(TriggerConfig, CreateOptionTemp)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_use_temp_triggers
 */

TEST(TriggerConfig, UseTempTriggers)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_trigger_read_cb
 */

TEST(TriggerConfig, TriggerReadCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_trigger_write_default_cb
 */

TEST(TriggerConfig, TriggerWriteDefaultCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_reload_cb
 */

TEST(TriggerConfig, ReloadCb)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_init
 */

TEST(TriggerConfig, Init)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_read
 */

TEST(TriggerConfig, Read)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_write
 */

TEST(TriggerConfig, Write)
{
    /* TODO: write tests */
}

/*
 * Tests functions:
 *   trigger_config_free
 */

TEST(TriggerConfig, Free)
{
    /* TODO: write tests */
}
