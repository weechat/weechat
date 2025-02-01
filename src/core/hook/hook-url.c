/*
 * hook-url.c - WeeChat URL hook
 *
 * Copyright (C) 2023-2025 Sébastien Helleu <flashcode@flashtux.org>
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
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "../weechat.h"
#include "../core-hashtable.h"
#include "../core-hook.h"
#include "../core-infolist.h"
#include "../core-log.h"
#include "../core-string.h"
#include "../core-url.h"
#include "../../gui/gui-chat.h"
#include "../../plugins/plugin.h"


/*
 * Returns description of hook.
 *
 * Note: result must be freed after use.
 */

char *
hook_url_get_description (struct t_hook *hook)
{
    char str_desc[1024];

    snprintf (str_desc, sizeof (str_desc),
              "URL: \"%s\", thread id: %d",
              HOOK_URL(hook, url),
              0);

    return strdup (str_desc);
}

/*
 * Displays keys and values of a hashtable.
 */

void
hook_url_hashtable_map_cb (void *data, struct t_hashtable *hashtable,
                           const void *key, const void *value)
{
    /* make C compiler happy */
    (void) data;
    (void) hashtable;

    gui_chat_printf (NULL, "    %s: \"%s\"",
                     (const char *)key,
                     (const char *)value);
}

/*
 * Runs callback of url hook.
 */

void
hook_url_run_callback (struct t_hook *hook)
{
    if (url_debug)
    {
        gui_chat_printf (NULL, "Running hook_url callback for URL \"%s\":",
                         HOOK_URL(hook, url));
        gui_chat_printf (NULL, "  options:");
        hashtable_map (HOOK_URL(hook, options), &hook_url_hashtable_map_cb, NULL);
        gui_chat_printf (NULL, "  output:");
        hashtable_map (HOOK_URL(hook, output), &hook_url_hashtable_map_cb, NULL);
    }

    (void) (HOOK_URL(hook, callback))
        (hook->callback_pointer,
         hook->callback_data,
         HOOK_URL(hook, url),
         HOOK_URL(hook, options),
         HOOK_URL(hook, output));
}

/*
 * Thread cleanup function: mark thread as not running anymore.
 */

void
hook_url_thread_cleanup (void *hook_pointer)
{
    struct t_hook *hook;

    hook = (struct t_hook *)hook_pointer;

    HOOK_URL(hook, thread_running) = 0;
}

/*
 * URL transfer (in a separate thread).
 */

void *
hook_url_transfer_thread (void *hook_pointer)
{
    struct t_hook *hook;
    int url_rc;
    char str_error_code[12];

    hook = (struct t_hook *)hook_pointer;

    pthread_cleanup_push (&hook_url_thread_cleanup, hook);

    url_rc = weeurl_download (HOOK_URL(hook, url),
                              HOOK_URL(hook, options),
                              HOOK_URL(hook, output));

    if (url_rc != 0)
    {
        snprintf (str_error_code, sizeof (str_error_code), "%d", url_rc);
        hashtable_set (HOOK_URL(hook, output), "error_code", str_error_code);
    }

    pthread_cleanup_pop (1);

    pthread_exit (NULL);
}

/*
 * Checks if thread is still alive.
 */

int
hook_url_timer_cb (const void *pointer, void *data, int remaining_calls)
{
    struct t_hook *hook;
    const char *ptr_error;
    char str_error[1024], str_error_code[12];

    /* make C compiler happy */
    (void) data;
    (void) remaining_calls;

    hook = (struct t_hook *)pointer;

    if (hook->deleted)
        return WEECHAT_RC_OK;

    if (!HOOK_URL(hook, thread_running))
    {
        hook_url_run_callback (hook);
        ptr_error = hashtable_get (HOOK_URL(hook, output), "error");
        if ((weechat_debug_core >= 1) && ptr_error && ptr_error[0])
        {
            gui_chat_printf (
                NULL,
                _("%sURL transfer error: %s (URL: \"%s\")"),
                gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                ptr_error,
                HOOK_URL(hook, url));
        }
        unhook (hook);
        return WEECHAT_RC_OK;
    }

    if (remaining_calls == 0)
    {
        if (!hashtable_has_key (HOOK_URL(hook, output), "error_code"))
        {
            snprintf (str_error, sizeof (str_error),
                      "transfer timeout reached (%.3fs)",
                      ((float)HOOK_URL(hook, timeout)) / 1000);
            snprintf (str_error_code, sizeof (str_error_code), "6");
            hashtable_set (HOOK_URL(hook, output), "error", str_error);
            hashtable_set (HOOK_URL(hook, output), "error_code", str_error_code);
        }
        hook_url_run_callback (hook);
        if (weechat_debug_core >= 1)
        {
            gui_chat_printf (
                NULL,
                _("End of URL transfer '%s', timeout reached (%.3fs)"),
                HOOK_URL(hook, url),
                ((float)HOOK_URL(hook, timeout)) / 1000);
        }
        pthread_cancel (HOOK_URL(hook, thread_id));
        usleep (1000);
        unhook (hook);
    }

    return WEECHAT_RC_OK;
}

/*
 * Starts transfer for an URL hook.
 */

void
hook_url_transfer (struct t_hook *hook)
{
    int rc, timeout, max_calls;
    long interval;
    char str_error[1024], str_error_code[12], str_error_code_pthread[12];

    HOOK_URL(hook, thread_running) = 1;

    /* create thread */
    rc = pthread_create (&(HOOK_URL(hook, thread_id)), NULL,
                         &hook_url_transfer_thread, hook);
    if (rc != 0)
    {
        snprintf (str_error, sizeof (str_error),
                  "error calling pthread_create (%d)", rc);
        snprintf (str_error_code, sizeof (str_error_code), "5");
        snprintf (str_error_code_pthread, sizeof (str_error_code_pthread),
                  "%d", rc);
        hashtable_set (HOOK_URL(hook, output), "error", str_error);
        hashtable_set (HOOK_URL(hook, output), "error_code", str_error_code);
        hashtable_set (HOOK_URL(hook, output), "error_code_pthread",
                       str_error_code_pthread);
        hook_url_run_callback (hook);

        if (weechat_debug_core >= 1)
        {
            gui_chat_printf (NULL,
                             _("%sError running thread in hook_url: %s (URL: \"%s\")"),
                             gui_chat_prefix[GUI_CHAT_PREFIX_ERROR],
                             strerror (rc),
                             HOOK_URL(hook, url));
        }
        unhook (hook);
        return;
    }

    /* main thread */
    HOOK_URL(hook, thread_created) = 1;
    timeout = HOOK_URL(hook, timeout);
    interval = 100;
    max_calls = 0;
    if (timeout > 0)
    {
        if (timeout <= 100)
        {
            interval = timeout;
            max_calls = 1;
        }
        else
        {
            interval = 100;
            max_calls = timeout / 100;
            if (timeout % 100 == 0)
                max_calls++;
        }
    }
    HOOK_URL(hook, hook_timer) = hook_timer (hook->plugin,
                                             interval, 0, max_calls,
                                             &hook_url_timer_cb,
                                             hook,
                                             NULL);
}

/*
 * Hooks a URL.
 *
 * Returns pointer to new hook, NULL if error.
 */

struct t_hook *
hook_url (struct t_weechat_plugin *plugin,
          const char *url,
          struct t_hashtable *options,
          int timeout,
          t_hook_callback_url *callback,
          const void *callback_pointer,
          void *callback_data)
{
    struct t_hook *new_hook;
    struct t_hook_url *new_hook_url;

    new_hook = NULL;
    new_hook_url = NULL;

    if (!url || !url[0] || !callback)
        goto error;

    new_hook = malloc (sizeof (*new_hook));
    if (!new_hook)
        goto error;

    new_hook_url = malloc (sizeof (*new_hook_url));
    if (!new_hook_url)
        goto error;

    hook_init_data (new_hook, plugin, HOOK_TYPE_URL, HOOK_PRIORITY_DEFAULT,
                    callback_pointer, callback_data);

    new_hook->hook_data = new_hook_url;
    new_hook_url->callback = callback;
    new_hook_url->url = strdup (url);
    new_hook_url->options = (options) ? hashtable_dup (options) : NULL;
    new_hook_url->timeout = timeout;
    new_hook_url->thread_id = 0;
    new_hook_url->thread_created = 0;
    new_hook_url->thread_running = 0;
    new_hook_url->hook_timer = NULL;
    new_hook_url->output = hashtable_new (32,
                                          WEECHAT_HASHTABLE_STRING,
                                          WEECHAT_HASHTABLE_STRING,
                                          NULL, NULL);
    hook_add_to_list (new_hook);

    if (weechat_debug_core >= 1)
    {
        gui_chat_printf (NULL,
                         "debug: hook_url: url=\"%s\", "
                         "options=\"%s\", timeout=%d",
                         new_hook_url->url,
                         hashtable_get_string (new_hook_url->options,
                                               "keys_values"),
                         new_hook_url->timeout);
    }

    hook_url_transfer (new_hook);

    return new_hook;

error:
    free (new_hook);
    free (new_hook_url);
    return NULL;
}

/*
 * Frees data in a url hook.
 */

void
hook_url_free_data (struct t_hook *hook)
{
    void *retval;

    if (!hook || !hook->hook_data)
        return;

    if (HOOK_URL(hook, url))
    {
        free (HOOK_URL(hook, url));
        HOOK_URL(hook, url) = NULL;
    }
    if (HOOK_URL(hook, options))
    {
        hashtable_free (HOOK_URL(hook, options));
        HOOK_URL(hook, options) = NULL;
    }
    if (HOOK_URL(hook, hook_timer))
    {
        unhook (HOOK_URL(hook, hook_timer));
        HOOK_URL(hook, hook_timer) = NULL;
    }
    if (HOOK_URL(hook, thread_running))
    {
        pthread_cancel (HOOK_URL(hook, thread_id));
        HOOK_URL(hook, thread_running) = 0;
    }
    if (HOOK_URL(hook, thread_created))
        pthread_join (HOOK_URL(hook, thread_id), &retval);
    if (HOOK_URL(hook, output))
    {
        hashtable_free (HOOK_URL(hook, output));
        HOOK_URL(hook, output) = NULL;
    }

    free (hook->hook_data);
    hook->hook_data = NULL;
}

/*
 * Adds url hook data in the infolist item.
 *
 * Returns:
 *   1: OK
 *   0: error
 */

int
hook_url_add_to_infolist (struct t_infolist_item *item,
                          struct t_hook *hook)
{
    if (!item || !hook || !hook->hook_data)
        return 0;

    if (!infolist_new_var_pointer (item, "callback", HOOK_URL(hook, callback)))
        return 0;
    if (!infolist_new_var_string (item, "url", HOOK_URL(hook, url)))
        return 0;
    if (!infolist_new_var_string (item, "options", hashtable_get_string (HOOK_URL(hook, options), "keys_values")))
        return 0;
    if (!infolist_new_var_integer (item, "timeout", (int)(HOOK_URL(hook, timeout))))
        return 0;
    if (!infolist_new_var_integer (item, "thread_created", (int)(HOOK_URL(hook, thread_created))))
        return 0;
    if (!infolist_new_var_integer (item, "thread_running", (int)(HOOK_URL(hook, thread_running))))
        return 0;
    if (!infolist_new_var_pointer (item, "hook_timer", HOOK_URL(hook, hook_timer)))
        return 0;
    if (!infolist_new_var_string (item, "output", hashtable_get_string (HOOK_URL(hook, output), "keys_values")))
        return 0;

    return 1;
}

/*
 * Prints url hook data in WeeChat log file (usually for crash dump).
 */

void
hook_url_print_log (struct t_hook *hook)
{
    if (!hook || !hook->hook_data)
        return;

    log_printf ("  url data:");
    log_printf ("    callback. . . . . . . : %p", HOOK_URL(hook, callback));
    log_printf ("    url . . . . . . . . . : '%s'", HOOK_URL(hook, url));
    log_printf ("    options . . . . . . . : %p (hashtable: '%s')",
                HOOK_URL(hook, options),
                hashtable_get_string (HOOK_URL(hook, options),
                                      "keys_values"));
    log_printf ("    timeout . . . . . . . : %ld", HOOK_URL(hook, timeout));
    log_printf ("    thread_created. . . . : %d", (int)HOOK_URL(hook, thread_created));
    log_printf ("    thread_running. . . . : %d", (int)HOOK_URL(hook, thread_running));
    log_printf ("    hook_timer. . . . . . : %p", HOOK_URL(hook, hook_timer));
    log_printf ("    output. . . . . . . . : %p (hashtable: '%s')",
                HOOK_URL(hook, output),
                hashtable_get_string (HOOK_URL(hook, output),
                                      "keys_values"));
}
