/*
 * core-signal.c - signal functions
 *
 * Copyright (C) 2021-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "weechat.h"
#include "core-signal.h"
#include "core-config.h"
#include "core-debug.h"
#include "core-eval.h"
#include "core-hook.h"
#include "core-input.h"
#include "core-log.h"
#include "core-string.h"
#include "../gui/gui-buffer.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


struct t_signal signal_list[] =
{ { SIGHUP, "hup" },
  { SIGINT, "int" },
  { SIGQUIT, "quit" },
  { SIGKILL, "kill" },
  { SIGTERM, "term" },
  { SIGUSR1, "usr1" },
  { SIGUSR2, "usr2" },
  { 0, NULL },
};

volatile sig_atomic_t signal_sighup_count = 0;
volatile sig_atomic_t signal_sigquit_count = 0;
volatile sig_atomic_t signal_sigterm_count = 0;
volatile sig_atomic_t signal_sigusr1_count = 0;
volatile sig_atomic_t signal_sigusr2_count = 0;


/*
 * Callback for system signal SIGHUP.
 */

void
signal_sighup_cb ()
{
    signal_sighup_count++;
}

/*
 * Callback for system signal SIGQUIT.
 */

void
signal_sigquit_cb ()
{
    signal_sigquit_count++;
}

/*
 * Callback for system signal SIGTERM.
 */

void
signal_sigterm_cb ()
{
    signal_sigterm_count++;
}

/*
 * Callback for system signal SIGUSR1.
 */

void
signal_sigusr1_cb ()
{
    signal_sigusr1_count++;
}

/*
 * Callback for system signal SIGUSR2.
 */

void
signal_sigusr2_cb ()
{
    signal_sigusr2_count++;
}

/*
 * Gets a signal index with a signal number; only some commonly used signal
 * names are supported here (see declaration of signal_list[]).
 *
 * Returns the index of signal in structure string_signal, -1 if not found.
 */

int
signal_search_number (int signal_number)
{
    int i;

    for (i = 0; signal_list[i].name; i++)
    {
        if (signal_list[i].signal == signal_number)
            return i;
    }

    /* signal not found */
    return -1;
}

/*
 * Gets a signal number with a name; only some commonly used signal names are
 * supported here (see declaration of signal_list[]).
 *
 * Returns the signal number, -1 if not found.
 */

int
signal_search_name (const char *name)
{
    int i;

    if (!name)
        return -1;

    for (i = 0; signal_list[i].name; i++)
    {
        if (string_strcasecmp (signal_list[i].name, name) == 0)
            return signal_list[i].signal;
    }

    /* signal not found */
    return -1;
}

/*
 * Catches a system signal.
 */

void
signal_catch (int signum, void (*handler)(int))
{
    struct sigaction act;

    sigemptyset (&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = handler;
    sigaction (signum, &act, NULL);
}

/*
 * Sends a WeeChat signal on a system signal received.
 *
 * Returns:
 *   WEECHAT_RC_OK: the WeeChat handler must be executed
 *   WEECHAT_RC_OK_EAT: signal eaten, the WeeChat handler must NOT be executed
 */

int
signal_send_to_weechat (int signal_index)
{
    int rc;
    char str_signal[32];

    if (signal_index < 0)
        return WEECHAT_RC_OK;

    snprintf (str_signal, sizeof (str_signal),
              "signal_sig%s", signal_list[signal_index].name);

    rc = hook_signal_send (str_signal, WEECHAT_HOOK_SIGNAL_STRING, NULL);

    return (rc == WEECHAT_RC_OK_EAT) ? WEECHAT_RC_OK_EAT : WEECHAT_RC_OK;
}

/*
 * Evaluates and executes the command bound to a signal.
 */

void
signal_exec_command (int signal_index, const char *command)
{
    char str_signal[32], *signal_upper, **commands, **ptr_command;
    char *command_eval;

    if (!command || !command[0])
        return;

    snprintf (str_signal, sizeof (str_signal),
              "sig%s", signal_list[signal_index].name);

    commands = string_split_command (command, ';');
    if (commands)
    {
        for (ptr_command = commands; *ptr_command; ptr_command++)
        {
            command_eval = eval_expression (*ptr_command, NULL, NULL, NULL);
            if (command_eval)
            {
                signal_upper = string_toupper (str_signal);
                log_printf ("Signal %s received, executing command: \"%s\"",
                            (signal_upper) ? signal_upper : str_signal,
                            command_eval);
                free (signal_upper);
                (void) input_data (gui_buffer_search_main (),
                                   command_eval, NULL, 0, 0);
                free (command_eval);
            }
        }
        string_free_split_command (commands);
    }
}

/*
 * Handles a specific signal received:
 */

void
signal_handle_number (int signal_number, int count, const char *command)
{
    int i, signal_index, rc;

    if (count == 0)
        return;

    signal_index = signal_search_number (signal_number);
    if (signal_index < 0)
        return;

    for (i = 0; i < count; i++)
    {
        rc = signal_send_to_weechat (signal_index);
        if (rc == WEECHAT_RC_OK_EAT)
            continue;
        signal_exec_command (signal_index, command);
    }
}

/*
 * Handles signals received: sends WeeChat signal and executes the configured
 * command (is signal not eaten).
 */

void
signal_handle ()
{
    /* SIGUSR1 */
    signal_handle_number (SIGUSR1, signal_sigusr1_count,
                          CONFIG_STRING(config_signal_sigusr1));
    signal_sigusr1_count = 0;

    /* SIGUSR2 */
    signal_handle_number (SIGUSR2, signal_sigusr2_count,
                          CONFIG_STRING(config_signal_sigusr2));
    signal_sigusr2_count = 0;

    /* SIGHUP */
    signal_handle_number (SIGHUP, signal_sighup_count,
                          CONFIG_STRING(config_signal_sighup));
    signal_sighup_count = 0;

    /* SIGQUIT */
    signal_handle_number (SIGQUIT, signal_sigquit_count,
                          CONFIG_STRING(config_signal_sigquit));
    signal_sigquit_count = 0;

    /* SIGTERM */
    signal_handle_number (SIGTERM, signal_sigterm_count,
                          CONFIG_STRING(config_signal_sigterm));
    signal_sigterm_count = 0;
}

/*
 * Suspends WeeChat process.
 */

void
signal_suspend ()
{
    kill (getpid (), SIGTSTP);
    gui_window_ask_refresh (2);
}

/*
 * Initializes signal.
 */

void
signal_init ()
{
    /* ignore some signals */
    signal_catch (SIGINT, SIG_IGN);
    signal_catch (SIGPIPE, SIG_IGN);

    /* catch signals that can be customized */
    signal_catch (SIGHUP, &signal_sighup_cb);
    signal_catch (SIGQUIT, &signal_sigquit_cb);
    signal_catch (SIGTERM, &signal_sigterm_cb);
    signal_catch (SIGUSR1, &signal_sigusr1_cb);
    signal_catch (SIGUSR2, &signal_sigusr2_cb);

    /* in case of crash (oh no!) */
    signal_catch (SIGSEGV, &debug_sigsegv_cb);
}
