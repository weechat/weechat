/*
 * core-sys.c - system actions
 *
 * Copyright (C) 2023-2024 Sébastien Helleu <flashcode@flashtux.org>
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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <sys/wait.h>

#include "weechat.h"
#include "core-config.h"
#include "core-log.h"
#include "core-string.h"
#include "core-sys.h"
#include "core-util.h"
#include "../gui/gui-chat.h"
#include "../gui/gui-window.h"
#include "../plugins/plugin.h"


#ifdef HAVE_SYS_RESOURCE_H
struct t_rlimit_resource rlimit_resource[] =
{
#ifdef RLIMIT_AS
    { "as", RLIMIT_AS },
#endif
#ifdef RLIMIT_CORE
    { "core", RLIMIT_CORE },
#endif
#ifdef RLIMIT_CPU
    { "cpu", RLIMIT_CPU },
#endif
#ifdef RLIMIT_DATA
    { "data", RLIMIT_DATA },
#endif
#ifdef RLIMIT_FSIZE
    { "fsize", RLIMIT_FSIZE },
#endif
#ifdef RLIMIT_LOCKS
    { "locks", RLIMIT_LOCKS },
#endif
#ifdef RLIMIT_MEMLOCK
    { "memlock", RLIMIT_MEMLOCK },
#endif
#ifdef RLIMIT_MSGQUEUE
    { "msgqueue", RLIMIT_MSGQUEUE },
#endif
#ifdef RLIMIT_NICE
    { "nice", RLIMIT_NICE },
#endif
#ifdef RLIMIT_NOFILE
    { "nofile", RLIMIT_NOFILE },
#endif
#ifdef RLIMIT_NPROC
    { "nproc", RLIMIT_NPROC },
#endif
#ifdef RLIMIT_RSS
    { "rss", RLIMIT_RSS },
#endif
#ifdef RLIMIT_RTPRIO
    { "rtprio", RLIMIT_RTPRIO },
#endif
#ifdef RLIMIT_RTTIME
    { "rttime", RLIMIT_RTTIME },
#endif
#ifdef RLIMIT_SIGPENDING
    { "sigpending", RLIMIT_SIGPENDING },
#endif
#ifdef RLIMIT_STACK
    { "stack", RLIMIT_STACK },
#endif
    { NULL, 0 },
};
#endif /* HAVE_SYS_RESOURCE_H */


/*
 * Sets resource limit.
 */

#ifdef HAVE_SYS_RESOURCE_H
void
sys_setrlimit_resource (const char *resource_name, long long limit)
{
    int i;
    struct rlimit rlim;
    char str_limit[64];

    if (!resource_name)
        return;

    if (limit == -1)
        snprintf (str_limit, sizeof (str_limit), "unlimited");
    else
        snprintf (str_limit, sizeof (str_limit), "%lld", limit);

    for (i = 0; rlimit_resource[i].name; i++)
    {
        if (strcmp (rlimit_resource[i].name, resource_name) == 0)
        {
            if (limit < -1)
            {
                gui_chat_printf (NULL,
                                 _("%sInvalid limit for resource \"%s\": %s "
                                   "(must be >= -1)"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 resource_name, str_limit);
                return;
            }
            rlim.rlim_cur = (limit >= 0) ? (rlim_t)limit : RLIM_INFINITY;
            rlim.rlim_max = rlim.rlim_cur;
            if (setrlimit (rlimit_resource[i].resource, &rlim) == 0)
            {
                log_printf (_("Limit for resource \"%s\" has been set to %s"),
                            resource_name, str_limit);
                if (gui_init_ok)
                {
                    gui_chat_printf (NULL,
                                     _("Limit for resource \"%s\" has been set "
                                       "to %s"),
                                     resource_name, str_limit);
                }
            }
            else
            {
                gui_chat_printf (NULL,
                                 _("%sUnable to set resource limit \"%s\" to "
                                   "%s: error %d %s"),
                                 gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                 resource_name,
                                 str_limit,
                                 errno,
                                 strerror (errno));
            }
            return;
        }
    }

    gui_chat_printf (NULL,
                     _("%sUnknown resource limit \"%s\" (see /help "
                       "weechat.startup.sys_rlimit)"),
                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                     resource_name);
}
#endif /* HAVE_SYS_RESOURCE_H */

/*
 * Sets resource limits using value of option "weechat.startup.sys_rlimit".
 */

void
sys_setrlimit ()
{
#ifdef HAVE_SYS_RESOURCE_H
    char **items, *pos, *error;
    int num_items, i;
    long long number;

    items = string_split (CONFIG_STRING(config_startup_sys_rlimit), ",", NULL,
                          WEECHAT_STRING_SPLIT_STRIP_LEFT
                          | WEECHAT_STRING_SPLIT_STRIP_RIGHT
                          | WEECHAT_STRING_SPLIT_COLLAPSE_SEPS,
                          0, &num_items);
    if (items)
    {
        for (i = 0; i < num_items; i++)
        {
            pos = strchr (items[i], ':');
            if (pos)
            {
                pos[0] = '\0';
                error = NULL;
                number = strtoll (pos + 1, &error, 10);
                if (error && !error[0])
                {
                    sys_setrlimit_resource (items[i], number);
                }
                else
                {
                    gui_chat_printf (NULL,
                                     _("%sInvalid limit for resource \"%s\": "
                                       "%s (must be >= -1)"),
                                     gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                                     items[i], pos + 1);
                }
            }
        }
        string_free_split (items);
    }
#endif /* HAVE_SYS_RESOURCE_H */
}

/*
 * Displays resource limits.
 */

void
sys_display_rlimit ()
{
#ifdef HAVE_SYS_RESOURCE_H
    struct rlimit rlim;
    char str_cur[128], str_max[128];
    int i;

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, _("Resource limits (see \"man getrlimit\" for help):"));
    for (i = 0; rlimit_resource[i].name; i++)
    {
        if (getrlimit (rlimit_resource[i].resource, &rlim) == 0)
        {
            if (rlim.rlim_cur == RLIM_INFINITY)
            {
                snprintf (str_cur, sizeof (str_cur), "unlimited");
            }
            else
            {
                snprintf (str_cur, sizeof (str_cur),
                          "%lld", (long long)rlim.rlim_cur);
            }
            if (rlim.rlim_max == RLIM_INFINITY)
            {
                snprintf (str_max, sizeof (str_max), "unlimited");
            }
            else
            {
                snprintf (str_max, sizeof (str_max),
                          "%lld", (long long)rlim.rlim_max);
            }
            gui_chat_printf (NULL,
                             "  %-10s: %s (max: %s)",
                             rlimit_resource[i].name, str_cur, str_max);
        }
        else
        {
            gui_chat_printf (NULL,
                             _("%sUnable to get resource limit \"%s\": "
                               "error %d %s"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             rlimit_resource[i].name,
                             errno,
                             strerror (errno));
        }
    }
#else /* HAVE_SYS_RESOURCE_H */
    gui_chat_printf (NULL,
                     _("System function \"%s\" is not available"),
                     "getrlimit");
#endif /* HAVE_SYS_RESOURCE_H */
}

/*
 * Displays resource usage.
 */

void
sys_display_rusage ()
{
#ifdef HAVE_SYS_RESOURCE_H
    struct rusage usage;
    char *str_time;
    long long microseconds;

    gui_chat_printf (NULL, "");
    gui_chat_printf (NULL, _("Resource usage (see \"man getrusage\" for help):"));
    getrusage (RUSAGE_SELF, &usage);
    /* ru_utime: user CPU time used */
    microseconds = ((long long)usage.ru_utime.tv_sec * 1000000)
        + (long long)usage.ru_utime.tv_usec;
    str_time = util_get_microseconds_string (microseconds);
    if (str_time)
    {
        gui_chat_printf (NULL, "  ru_utime   : %s", str_time);
        free (str_time);
    }
    /* ru_stime: system CPU time used */
    microseconds = ((long long)usage.ru_stime.tv_sec * 1000000)
        + (long long)usage.ru_stime.tv_usec;
    str_time = util_get_microseconds_string (microseconds);
    if (str_time)
    {
        gui_chat_printf (NULL, "  ru_stime   : %s", str_time);
        free (str_time);
    }
    /* ru_maxrss: maximum resident set size */
    gui_chat_printf (NULL, "  ru_maxrss  : %ld", usage.ru_maxrss);
    /* ru_ixrss: integral shared memory size */
    gui_chat_printf (NULL, "  ru_ixrss   : %ld", usage.ru_ixrss);
    /* ru_idrss: integral unshared data size */
    gui_chat_printf (NULL, "  ru_idrss   : %ld", usage.ru_idrss);
    /* ru_isrss: integral unshared stack size */
    gui_chat_printf (NULL, "  ru_isrss   : %ld", usage.ru_isrss);
    /* ru_minflt: page reclaims (soft page faults) */
    gui_chat_printf (NULL, "  ru_minflt  : %ld", usage.ru_minflt);
    /* ru_majflt: page faults (hard page faults) */
    gui_chat_printf (NULL, "  ru_majflt  : %ld", usage.ru_majflt);
    /* ru_nswap: swaps */
    gui_chat_printf (NULL, "  ru_nswap   : %ld", usage.ru_nswap);
    /* ru_inblock: block input operations */
    gui_chat_printf (NULL, "  ru_inblock : %ld", usage.ru_inblock);
    /* ru_oublock: block output operations */
    gui_chat_printf (NULL, "  ru_oublock : %ld", usage.ru_oublock);
    /* ru_msgsnd: IPC messages sent */
    gui_chat_printf (NULL, "  ru_msgsnd  : %ld", usage.ru_msgsnd);
    /* ru_msgrcv: IPC messages received */
    gui_chat_printf (NULL, "  ru_msgrcv  : %ld", usage.ru_msgrcv);
    /* ru_nsignals: signals received */
    gui_chat_printf (NULL, "  ru_nsignals: %ld", usage.ru_nsignals);
    /* ru_nvcsw: voluntary context switches */
    gui_chat_printf (NULL, "  ru_nvcsw   : %ld", usage.ru_nvcsw);
    /* ru_nivcsw: involuntary context switches */
    gui_chat_printf (NULL, "  ru_nivcsw  : %ld", usage.ru_nivcsw);
#else /* HAVE_SYS_RESOURCE_H */
    gui_chat_printf (NULL,
                     _("System function \"%s\" is not available"),
                     "getrusage");
#endif /* HAVE_SYS_RESOURCE_H */
}

/*
 * Calls waitpid() to acknowledge the end of forked processes, thus preventing
 * them to become zombies.
 */

void
sys_waitpid (int number_processes)
{
    int i;

    if (number_processes < 1)
        return;

    /* acknowledge the end of up to 42 forked processes */
    i = 0;
    while ((i < number_processes) && (waitpid (-1, NULL, WNOHANG) > 0))
    {
        i++;
    }
}
