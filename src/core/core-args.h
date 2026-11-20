/*
 * SPDX-FileCopyrightText: 2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_ARGS_H
#define WEECHAT_ARGS_H

/*
 * Short options for getopt_long(): "-" to process the arguments in order
 * (non-option arguments are returned as option 1), ":" to return ':' if an
 * argument is missing.
 */
#define ARGS_SHORT_OPTIONS "-:acd:thilpP:r:sv"

enum t_args_action
{
    ARGS_ACTION_NONE = 0,              /* no action                         */
    ARGS_ACTION_COLORS,                /* -c / --colors                     */
    ARGS_ACTION_HELP,                  /* -h / --help                       */
    ARGS_ACTION_BUILD_INFO,            /* -i / --build-info                 */
    ARGS_ACTION_LICENSE,               /* -l / --license                    */
    ARGS_ACTION_VERSION,               /* -v / --version                    */
};

enum t_args_option_error
{
    ARGS_OPTION_ERROR_UNKNOWN = 0,     /* unknown option                    */
    ARGS_OPTION_ERROR_AMBIGUOUS,       /* abbreviation of many options      */
    ARGS_OPTION_ERROR_ARGUMENT_NOT_ALLOWED, /* argument given (not allowed) */
};

extern int args_action;

struct option;

extern int args_option_is_headless_only (int val);
extern void args_get_long_options (struct option *long_options, int headless);

extern char *args_escape (const char *string, int length);
extern char *args_get_invalid_option (const char *arg, const char *optstring);
extern int args_check_long_option (const char *arg,
                                   const struct option *long_options,
                                   int *index_option);
extern void args_display_usage_hint (void);
extern void args_display_error_invalid_option (const char *arg,
                                               const struct option *long_options);
extern void args_exit (int return_code);
extern void args_add_plugin_arg (const char *arg);
extern void args_display_help (void);
extern void args_parse (int argc, char *argv[]);
extern void args_run_action (void);

#endif /* WEECHAT_ARGS_H */
