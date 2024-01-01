/*
 * Copyright (C) 2014-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_TRIGGER_CALLBACK_H
#define WEECHAT_PLUGIN_TRIGGER_CALLBACK_H

#include <time.h>
#include <sys/time.h>

struct t_trigger_context
{
    unsigned long id;
    struct t_gui_buffer *buffer;
    struct t_hashtable *pointers;
    struct t_hashtable *extra_vars;
    struct t_weelist *vars_updated;
    struct timeval start_exec;
    struct timeval start_check_conditions;
    struct timeval start_regex;
    struct timeval start_run_command;
    struct timeval end_exec;
};

#define TRIGGER_CALLBACK_CB_INIT(__rc)                          \
    struct t_trigger *trigger;                                  \
    struct t_trigger_context ctx;                               \
    int trigger_rc;                                             \
    (void) data;                                                \
    (void) trigger_rc;                                          \
    if (!trigger_enabled)                                       \
        return __rc;                                            \
    trigger = (struct t_trigger *)pointer;                      \
    if (!trigger || trigger->hook_running)                      \
        return __rc;                                            \
    memset (&ctx, 0, sizeof (ctx));                             \
    if (weechat_trigger_plugin->debug >= 1)                     \
        gettimeofday (&(ctx.start_exec), NULL);                 \
    trigger->hook_count_cb++;                                   \
    trigger->hook_running = 1;                                  \
    trigger_rc = trigger_return_code[                           \
        weechat_config_enum (                                   \
            trigger->options[TRIGGER_OPTION_RETURN_CODE])];

#define TRIGGER_CALLBACK_CB_NEW_POINTERS                        \
    ctx.pointers = weechat_hashtable_new (                      \
        32,                                                     \
        WEECHAT_HASHTABLE_STRING,                               \
        WEECHAT_HASHTABLE_POINTER,                              \
        NULL, NULL);                                            \
    if (!ctx.pointers)                                          \
        goto end;

#define TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS                      \
    ctx.extra_vars = weechat_hashtable_new (                    \
        32,                                                     \
        WEECHAT_HASHTABLE_STRING,                               \
        WEECHAT_HASHTABLE_STRING,                               \
        NULL, NULL);                                            \
    if (!ctx.extra_vars)                                        \
        goto end;

#define TRIGGER_CALLBACK_CB_NEW_VARS_UPDATED                    \
    ctx.vars_updated = weechat_list_new ();                     \
    if (!ctx.vars_updated)                                      \
        goto end;

#define TRIGGER_CALLBACK_CB_END(__rc)                           \
    if (ctx.pointers)                                           \
        weechat_hashtable_free (ctx.pointers);                  \
    if (ctx.extra_vars)                                         \
        weechat_hashtable_free (ctx.extra_vars);                \
    if (ctx.vars_updated)                                       \
        weechat_list_free (ctx.vars_updated);                   \
    trigger->hook_running = 0;                                  \
    switch (weechat_config_enum (                               \
                trigger->options[TRIGGER_OPTION_POST_ACTION]))  \
    {                                                           \
        case TRIGGER_POST_ACTION_DISABLE:                       \
            weechat_config_option_set (                         \
                trigger->options[TRIGGER_OPTION_ENABLED],       \
                "off", 1);                                      \
            break;                                              \
        case TRIGGER_POST_ACTION_DELETE:                        \
            trigger_free (trigger);                             \
            break;                                              \
        default:                                                \
            /* do nothing in the other cases */                 \
            break;                                              \
    }                                                           \
    return __rc;

extern unsigned long trigger_context_id;

extern int trigger_callback_signal_cb (const void *pointer, void *data,
                                       const char *signal,
                                       const char *type_data,
                                       void *signal_data);
extern int trigger_callback_hsignal_cb (const void *pointer, void *data,
                                        const char *signal,
                                        struct t_hashtable *hashtable);
extern char *trigger_callback_modifier_cb (const void *pointer, void *data,
                                           const char *modifier,
                                           const char *modifier_data,
                                           const char *string);
extern struct t_hashtable *trigger_callback_line_cb  (const void *pointer, void *data,
                                                      struct t_hashtable *line);
extern int trigger_callback_print_cb  (const void *pointer, void *data,
                                       struct t_gui_buffer *buffer,
                                       time_t date, int date_usec,
                                       int tags_count, const char **tags,
                                       int displayed, int highlight,
                                       const char *prefix, const char *message);
extern int trigger_callback_command_cb  (const void *pointer,
                                         void *data,
                                         struct t_gui_buffer *buffer,
                                         int argc, char **argv,
                                         char **argv_eol);
extern int trigger_callback_command_run_cb  (const void *pointer,
                                             void *data,
                                             struct t_gui_buffer *buffer,
                                             const char *command);
extern int trigger_callback_timer_cb  (const void *pointer, void *data,
                                       int remaining_calls);
extern int trigger_callback_config_cb  (const void *pointer, void *data,
                                        const char *option, const char *value);
extern struct t_hashtable *trigger_callback_focus_cb (const void *pointer,
                                                      void *data,
                                                      struct t_hashtable *info);
extern char *trigger_callback_info_cb (const void *pointer,
                                       void *data,
                                       const char *info_name,
                                       const char *arguments);
extern struct t_hashtable *trigger_callback_info_hashtable_cb (const void *pointer,
                                                               void *data,
                                                               const char *info_name,
                                                               struct t_hashtable *hashtable);
extern void trigger_callback_init ();
extern void trigger_callback_end ();

#endif /* WEECHAT_PLUGIN_TRIGGER_CALLBACK_H */
