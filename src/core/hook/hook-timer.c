/*
 * hook-timer.c - WeeChat timer hook
 *
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <string.h>
#include <time.h>

#include "../weechat.h"
#include "../core-hook.h"
#include "../core-infolist.h"
#include "../core-log.h"
#include "../core-util.h"
#include "../../gui/gui-chat.h"


time_t hook_last_system_time = 0;      /* used to detect system clock skew  */


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_timer_get_description (struct t_hook *hook)
{
    char str_desc[512];
    int unit_seconds;
    long interval;

    unit_seconds = (HOOK_TIMER(hook, interval) % 1000 == 0);
    interval = (unit_seconds) ?
        HOOK_TIMER(hook, interval) / 1000 :
        HOOK_TIMER(hook, interval);

    if (HOOK_TIMER(hook, remaining_calls) > 0)
    {
        snprintf (str_desc, sizeof (str_desc),
                  "%ld%s (%d calls remaining)",
                  interval,
                  (unit_seconds) ? "s" : "ms",
                  HOOK_TIMER(hook, remaining_calls));
    }
    else
    {
        snprintf (str_desc, sizeof (str_desc),
                  "%ld%s (no call limit)",
                  interval,
                  (unit_seconds) ? "s" : "ms");
    }

    return strdup (str_desc);
}

/*
 * Initializes a timer hook.
 */

void
hook_timer_init (struct t_hook *hook)
{
    time_t time_now;
    struct tm *local_time, gm_time;
    int local_hour, gm_hour, diff_hour;

    gettimeofday (&HOOK_TIMER(hook, last_exec), NULL);
    time_now = time (NULL);
    local_time = localtime (&time_now);
    local_hour = local_time->tm_hour;
    gmtime_r (&time_now, &gm_time);
    gm_hour = gm_time.tm_hour;
    if ((local_time->tm_year > gm_time.tm_year)
        || (local_time->tm_mon > gm_time.tm_mon)
        || (local_time->tm_mday > gm_time.tm_mday))
    {
        diff_hour = (24 - gm_hour) + local_hour;
    }
    else if ((gm_time.tm_year > local_time->tm_year)
             || (gm_time.tm_mon > local_time->tm_mon)
             || (gm_time.tm_mday > local_time->tm_mday))
    {
        diff_hour = (-1) * ((24 - local_hour) + gm_hour);
    }
    else
        diff_hour = local_hour - gm_hour;

    if ((HOOK_TIMER(hook, interval) >= 1000)
        && (HOOK_TIMER(hook, align_second) > 0))
    {
        /*
         * here we should use 0, but with this value timer is sometimes called
         * before second has changed, so for displaying time, it may display
         * 2 times the same second, that's why we use 10000 micro seconds
         */
        HOOK_TIMER(hook, last_exec).tv_usec = 10000;
        HOOK_TIMER(hook, last_exec).tv_sec =
            HOOK_TIMER(hook, last_exec).tv_sec -
            ((HOOK_TIMER(hook, last_exec).tv_sec + (diff_hour * 3600)) %
             HOOK_TIMER(hook, align_second));
    }

    /* init next call with date of last call */
    HOOK_TIMER(hook, next_exec).tv_sec = HOOK_TIMER(hook, last_exec).tv_sec;
    HOOK_TIMER(hook, next_exec).tv_usec = HOOK_TIMER(hook, last_exec).tv_usec;

    /* add interval to next call date */
    util_timeval_add (&HOOK_TIMER(hook, next_exec),
                      ((long long)HOOK_TIMER(hook, interval)) * 1000);
}

/*
 * Hooks a timer.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_timer (struct t_weechat_plugin *plugin, long interval, int align_second,
            int max_calls, t_hook_callback_timer *callback,
            const void *callback_pointer,
            void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_timer *new_hook_timer;

    if ((interval <= 0) || !callback)
        return NULL;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        return NULL;
    new_hook_timer = malloc (sizeof (*new_hook_timer));
    if (!new_hook_timer)
    {
        free (new_hook);
        return NULL;
    }

    hook_init_data (new_hook, plugin, HOOK_TYPE_TIMER, HOOK_PRIORITY_DEFAULT,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_timer;
    new_hook_timer->callback = callback;
    new_hook_timer->interval = interval;
    new_hook_timer->align_second = align_second;
    new_hook_timer->remaining_calls = max_calls;

    hook_timer_init (new_hook);

    hook_add_to_list (new_hook);

    return new_hook;
}

/*
 * Checks if system clock is older than previous call to this function (that
 * means new time is lower than in past). If yes, adjusts all timers to current
 * time.
 */

void
hook_timer_check_system_clock (void)
{
    time_t now;
    long diff_time;
    struct t_hook *ptr_hook;

    now = time (NULL);

    /*
     * check if difference with previous time is more than 10 seconds:
     * if it is, then consider it's clock skew and reinitialize all timers
     */
    diff_time = now - hook_last_system_time;
    if ((diff_time <= -10) || (diff_time >= 10))
    {
        if (weechat_debug_core >= 1)
        {
            gui_chat_printf (NULL,
                             _("System clock skew detected (%+ld seconds), "
                               "reinitializing all timers"),
                             diff_time);
        }

        /* reinitialize all timers */
        for (ptr_hook = weechat_hooks[HOOK_TYPE_TIMER]; ptr_hook;
             ptr_hook = ptr_hook->next_hook)
        {
            if (!ptr_hook->deleted)
                hook_timer_init (ptr_hook);
        }
    }

    hook_last_system_time = now;
}

/*
 * Returns time until next timeout (in milliseconds).
 */

int
hook_timer_get_time_to_next (void)
{
    struct t_hook *ptr_hook;
    int found, timeout;
    struct timeval tv_now, tv_timeout;
    long diff_usec;

    hook_timer_check_system_clock ();

    found = 0;
    tv_timeout.tv_sec = 0;
    tv_timeout.tv_usec = 0;

    for (ptr_hook = weechat_hooks[HOOK_TYPE_TIMER]; ptr_hook;
         ptr_hook = ptr_hook->next_hook)
    {
        if (!ptr_hook->deleted
            && (!found
                || (util_timeval_cmp (&HOOK_TIMER(ptr_hook, next_exec),
                                      &tv_timeout) < 0)))
        {
            found = 1;
            tv_timeout.tv_sec = HOOK_TIMER(ptr_hook, next_exec).tv_sec;
            tv_timeout.tv_usec = HOOK_TIMER(ptr_hook, next_exec).tv_usec;
        }
    }

    /* no timeout found, return 2 seconds by default */
    if (!found)
    {
        tv_timeout.tv_sec = 2;
        tv_timeout.tv_usec = 0;
        goto end;
    }

    gettimeofday (&tv_now, NULL);

    /* next timeout is past date! */
    if (util_timeval_cmp (&tv_timeout, &tv_now) < 0)
    {
        tv_timeout.tv_sec = 0;
        tv_timeout.tv_usec = 0;
        goto end;
    }

    tv_timeout.tv_sec = tv_timeout.tv_sec - tv_now.tv_sec;
    diff_usec = tv_timeout.tv_usec - tv_now.tv_usec;
    if (diff_usec >= 0)
    {
        tv_timeout.tv_usec = diff_usec;
    }
    else
    {
        tv_timeout.tv_sec--;
        tv_timeout.tv_usec = 1000000 + diff_usec;
    }

    /*
     * to detect clock skew, we ensure there's a call to timers every
     * 2 seconds max
     */
    if (tv_timeout.tv_sec >= 2)
    {
        tv_timeout.tv_sec = 2;
        tv_timeout.tv_usec = 0;
    }

end:
    /* return a number of milliseconds */
    timeout = (tv_timeout.tv_sec * 1000) + (tv_timeout.tv_usec / 1000);
    return (timeout < 1) ? 1 : timeout;
}

/*
 * Executes timer hooks.
 */

void
hook_timer_exec (void)
{
    struct t_hook *ptr_hook, *next_hook;
    struct t_hook_exec_cb hook_exec_cb;
    struct timeval tv_time;

    if (!weechat_hooks[HOOK_TYPE_TIMER])
        return;

    hook_timer_check_system_clock ();

    gettimeofday (&tv_time, NULL);

    hook_exec_start ();

    ptr_hook = weechat_hooks[HOOK_TYPE_TIMER];
    while (ptr_hook)
    {
        next_hook = ptr_hook->next_hook;

        if (!ptr_hook->deleted
            && !ptr_hook->running
            && (util_timeval_cmp (&HOOK_TIMER(ptr_hook, next_exec),
                                  &tv_time) <= 0))
        {
            hook_callback_start (ptr_hook, &hook_exec_cb);
            (void) (HOOK_TIMER(ptr_hook, callback))
                (ptr_hook->callback_pointer,
                 ptr_hook->callback_data,
                 (HOOK_TIMER(ptr_hook, remaining_calls) > 0) ?
                  HOOK_TIMER(ptr_hook, remaining_calls) - 1 : -1);
            hook_callback_end (ptr_hook, &hook_exec_cb);
            if (!ptr_hook->deleted)
            {
                HOOK_TIMER(ptr_hook, last_exec).tv_sec = tv_time.tv_sec;
                HOOK_TIMER(ptr_hook, last_exec).tv_usec = tv_time.tv_usec;

                util_timeval_add (
                    &HOOK_TIMER(ptr_hook, next_exec),
                    ((long long)HOOK_TIMER(ptr_hook, interval)) * 1000);

                if (HOOK_TIMER(ptr_hook, remaining_calls) > 0)
                {
                    HOOK_TIMER(ptr_hook, remaining_calls)--;
                    if (HOOK_TIMER(ptr_hook, remaining_calls) == 0)
                        unhook (ptr_hook);
                }
            }
        }

        ptr_hook = next_hook;
    }

    hook_exec_end ();
}

/*
 * Frees data in a timer hook.
 */

void
hook_timer_free_data (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds timer hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_timer_add_to_infolist (struct t_infolist_item *item,
                            struct t_hook *hook)
{
    char value[64];

    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_TIMER(hook, callback)))
        return 0;
    snprintf (value, sizeof (value), "%ld", HOOK_TIMER(hook, interval));
    if (!infolist_new_var_string (item, "interval", value))
        return 0;
    if (!infolist_new_var_integer (item, "align_second", HOOK_TIMER(hook, align_second)))
        return 0;
    if (!infolist_new_var_integer (item, "remaining_calls", HOOK_TIMER(hook, remaining_calls)))
        return 0;
    if (!infolist_new_var_buffer (item, "last_exec",
                                  &(HOOK_TIMER(hook, last_exec)),
                                  sizeof (HOOK_TIMER(hook, last_exec))))
        return 0;
    if (!infolist_new_var_buffer (item, "next_exec",
                                  &(HOOK_TIMER(hook, next_exec)),
                                  sizeof (HOOK_TIMER(hook, next_exec))))
        return 0;

    return 1;
}

/*
 * Prints timer hook data in WeeChat log file (usually for crash dump).
 */

void
hook_timer_print_log (struct t_hook *hook)
{
    char text_time[128];

    if (!hook || !hook->hook_data)
        return;

    log_printf ("  timer data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_TIMER(hook, callback));
    log_printf ("    interval. . . . . . . : %ld", HOOK_TIMER(hook, interval));
    log_printf ("    align_second. . . . . : %d", HOOK_TIMER(hook, align_second));
    log_printf ("    remaining_calls . . . : %d", HOOK_TIMER(hook, remaining_calls));
    util_strftimeval (text_time, sizeof (text_time),
                      "%Y-%m-%dT%H:%M:%S.%f", &(HOOK_TIMER(hook, last_exec)));
    log_printf ("    last_exec . . . . . . : %s", text_time);
    log_printf ("      tv_sec. . . . . . . : %lld",
                (long long)(HOOK_TIMER(hook, last_exec.tv_sec)));
    log_printf ("      tv_usec. . . .  . . : %ld",
                (long)(HOOK_TIMER(hook, last_exec.tv_usec)));
    util_strftimeval (text_time, sizeof (text_time),
                      "%Y-%m-%dT%H:%M:%S.%f", &(HOOK_TIMER(hook, next_exec)));
    log_printf ("    last_exec . . . . . . : %s", text_time);
    log_printf ("      tv_sec. . . . . . . : %lld",
                (long long)(HOOK_TIMER(hook, next_exec.tv_sec)));
    log_printf ("      tv_usec. . . .  . . : %ld",
                (long)(HOOK_TIMER(hook, next_exec.tv_usec)));
}
