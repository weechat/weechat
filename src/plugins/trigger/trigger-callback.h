/*
 * Copyright (C) 2014-2015 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_TRIGGER_CALLBACK_H
#define WEECHAT_TRIGGER_CALLBACK_H 1

#define TRIGGER_CALLBACK_CB_INIT(__rc)                          \
    struct t_trigger *trigger;                                  \
    struct t_hashtable *pointers, *extra_vars;                  \
    int trigger_rc;                                             \
    pointers = NULL;                                            \
    extra_vars = NULL;                                          \
    (void) trigger_rc;                                          \
    if (!trigger_enabled)                                       \
        return __rc;                                            \
    trigger = (struct t_trigger *)data;                         \
    if (!trigger || trigger->hook_running)                      \
        return __rc;                                            \
    trigger->hook_count_cb++;                                   \
    trigger->hook_running = 1;                                  \
    trigger_rc = trigger_return_code[                           \
        weechat_config_integer (                                \
            trigger->options[TRIGGER_OPTION_RETURN_CODE])];

#define TRIGGER_CALLBACK_CB_NEW_POINTERS                        \
    pointers = weechat_hashtable_new (                          \
        32,                                                     \
        WEECHAT_HASHTABLE_STRING,                               \
        WEECHAT_HASHTABLE_POINTER,                              \
        NULL,                                                   \
        NULL);                                                  \
    if (!pointers)                                              \
        goto end;

#define TRIGGER_CALLBACK_CB_NEW_EXTRA_VARS                      \
    extra_vars = weechat_hashtable_new (                        \
        32,                                                     \
        WEECHAT_HASHTABLE_STRING,                               \
        WEECHAT_HASHTABLE_STRING,                               \
        NULL,                                                   \
        NULL);                                                  \
    if (!extra_vars)                                            \
        goto end;

#define TRIGGER_CALLBACK_CB_END(__rc)                           \
    if (pointers)                                               \
        weechat_hashtable_free (pointers);                      \
    if (extra_vars)                                             \
        weechat_hashtable_free (extra_vars);                    \
    trigger->hook_running = 0;                                  \
    switch (weechat_config_integer (                            \
                trigger->options[TRIGGER_OPTION_ONCE_ACTION]))  \
    {                                                           \
        case TRIGGER_ONCE_DISABLE:                              \
            weechat_config_option_set (                         \
                trigger->options[TRIGGER_OPTION_ENABLED],       \
                "off", 1);                                      \
            break;                                              \
        case TRIGGER_ONCE_DELETE:                               \
            trigger_free (trigger);                             \
            break;                                              \
    }                                                           \
    return __rc;

extern int trigger_callback_signal_cb (void *data, const char *signal,
                                       const char *type_data, void *signal_data);
extern int trigger_callback_hsignal_cb (void *data, const char *signal,
                                        struct t_hashtable *hashtable);
extern char *trigger_callback_modifier_cb (void *data, const char *modifier,
                                           const char *modifier_data,
                                           const char *string);
extern int trigger_callback_print_cb  (void *data, struct t_gui_buffer *buffer,
                                       time_t date, int tags_count,
                                       const char **tags, int displayed,
                                       int highlight, const char *prefix,
                                       const char *message);
extern int trigger_callback_command_cb  (void *data, struct t_gui_buffer *buffer,
                                         int argc, char **argv, char **argv_eol);
extern int trigger_callback_command_run_cb  (void *data,
                                             struct t_gui_buffer *buffer,
                                             const char *command);
extern int trigger_callback_timer_cb  (void *data, int remaining_calls);
extern int trigger_callback_config_cb  (void *data, const char *option,
                                        const char *value);
extern struct t_hashtable *trigger_callback_focus_cb (void *data,
                                                      struct t_hashtable *info);
extern void trigger_callback_init ();
extern void trigger_callback_end ();

#endif /* WEECHAT_TRIGGER_CALLBACK_H */
