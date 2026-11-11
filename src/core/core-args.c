/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/* Function for command-line arguments */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "weechat.h"
#include "core-args.h"
#include "core-debug.h"
#include "core-list.h"
#include "core-string.h"
#include "core-utf8.h"
#include "core-version.h"
#include "../gui/gui-color.h"
#include "../plugins/plugin.h"

/* Some command line options */
#define OPTION_DOCGEN     1000
#define OPTION_NO_DLCLOSE 1001
#define OPTION_NO_GNUTLS  1002
#define OPTION_NO_GCRYPT  1003
#define OPTION_DAEMON     1004
#define OPTION_STDOUT     1005

/* Exit code for errors in command line arguments (usage error) */
#define ARGS_EXIT_USAGE_ERROR 2

/* Option that displays something and exits (run by args_run_action) */
int args_action = ARGS_ACTION_NONE;

/* Long options (headless options are removed in the other modes) */
struct option args_long_options[] = {
    /* Standard options */
    { "no-connect",  no_argument,       NULL, 'a'               },
    { "colors",      no_argument,       NULL, 'c'               },
    { "dir",         required_argument, NULL, 'd'               },
    { "temp-dir",    no_argument,       NULL, 't'               },
    { "help",        no_argument,       NULL, 'h'               },
    { "build-info",  no_argument,       NULL, 'i'               },
    { "license",     no_argument,       NULL, 'l'               },
    { "no-plugin",   no_argument,       NULL, 'p'               },
    { "plugins",     required_argument, NULL, 'P'               },
    { "run-command", required_argument, NULL, 'r'               },
    { "no-script",   no_argument,       NULL, 's'               },
    { "upgrade",     no_argument,       NULL, 'u'               },
    { "version",     no_argument,       NULL, 'v'               },
    /* Debug options */
    { "no-dlclose",  no_argument,       NULL, OPTION_NO_DLCLOSE },
    { "no-gnutls",   no_argument,       NULL, OPTION_NO_GNUTLS  },
    { "no-gcrypt",   no_argument,       NULL, OPTION_NO_GCRYPT  },
    /* Options for headless mode only (see args_option_is_headless_only) */
    { "daemon",      no_argument,       NULL, OPTION_DAEMON     },
    { "stdout",      no_argument,       NULL, OPTION_STDOUT     },
    { "doc-gen",     required_argument, NULL, OPTION_DOCGEN     },
    { NULL,          0,                 NULL, 0                 },
};

#define ARGS_NUM_LONG_OPTIONS                                           \
    (int)(sizeof (args_long_options) / sizeof (args_long_options[0]))

char *weechat_args_common[][2] = {
    {
        "-a, --no-connect",
        /* TRANSLATORS: command line option "-a", "--no-connect". */
        N_("disable auto-connect to servers at startup"),
    },
    {
        "-c, --colors",
        /* TRANSLATORS: command line option "-c", "--colors". */
        N_("display default colors in terminal and exit"),
    },
    {
        /* TRANSLATORS: only "<path>" may be translated. */
        N_("-d, --dir <path>"),
        /* TRANSLATORS: command line option "-d", "--dir <path>". */
        N_("force a single WeeChat home directory or 5 different directories "
           "separated by colons (in this order: config, data, state, cache, runtime) "
           "(environment variable WEECHAT_HOME is read if this option is not given)"),
    },
    {
        "-t, --temp-dir",
        /* TRANSLATORS: command line option "-t", "--temp-dir". */
        N_("create a temporary WeeChat home directory and delete it on exit "
           "(incompatible with option \"-d\")"),
    },
    {
        "-h, --help",
        /* TRANSLATORS: command line option "-h", "--help". */
        N_("display this help and exit"),
    },
    {
        "-i, --build-info",
        /* TRANSLATORS: command line option "-i", "--build-info". */
        N_("display build information and exit"),
    },
    {
        "-l, --license",
        /* TRANSLATORS: command line option "-l", "--license". */
        N_("display WeeChat license and exit"),
    },
    {
        "-p, --no-plugin",
        /* TRANSLATORS: command line option "-p", "--no-plugin". */
        N_("don't load any plugin at startup"),
    },
    {
        /* TRANSLATORS: only "<plugins>" may be translated. */
        N_("-P, --plugins <plugins>"),
        /* TRANSLATORS: command line option "-P", "--plugins <plugins>". */
        N_("load only these plugins at startup (see /help weechat.plugin.autoload)"),
    },
    {
        /* TRANSLATORS: only "<cmd>" may be translated (please keep it short). */
        N_("-r, --run-command <cmd>"),
        /* TRANSLATORS: command line option "-r", "--run-command <cmd>". */
        N_("run command(s) after startup; many commands can be separated "
           "by semicolons and are evaluated, this option can be given multiple times"),
    },
    {
        "-s, --no-script",
        /* TRANSLATORS: command line option "-s", "--no-script". */
        N_("don't load any script at startup"),
    },
    {
        "--upgrade",
        /* TRANSLATORS: command line option "--upgrade". */
        N_("upgrade WeeChat using session files (see /help upgrade in WeeChat)"),
    },
    {
        "-v, --version",
        /* TRANSLATORS: command line option "-v", "--version". */
        N_("display WeeChat version and exit"),
    },
    {
        /* TRANSLATORS: command line option: "plugin" and "option" may be translated. */
        N_("plugin:option"),
        /* TRANSLATORS: command line option "plugin:option". */
        N_("option for plugin (see man weechat)"),
    },
    { NULL, NULL },
};
char *weechat_args_headless[][2] = {
    {
        /* TRANSLATORS: only "<path>" may be translated. */
        N_("--doc-gen <path>"),
        /* TRANSLATORS: command line option "--doc-gen <path>". */
        N_("generate files to build documentation and exit"),
    },
    {
        "--daemon",
        /* TRANSLATORS: command line option "--daemon". */
        N_("run WeeChat as a daemon (fork, new process group, file descriptors closed); "
           "by default in headless mode WeeChat is blocking and does not run in background"),
    },
    {
        "--stdout",
        /* TRANSLATORS: command line option "--stdout". */
        N_("display log messages on standard output instead of writing them in log file "
           "(option ignored if option \"--daemon\" is given)"),
    },
    { NULL, NULL },
};
char *weechat_args_debug[][2] = {
    {
        "--no-dlclose",
        /* TRANSLATORS: command line option "--no-dlclose". */
        N_("do not call function dlclose after plugins are unloaded"),
    },
    {
        "--no-gnutls",
        /* TRANSLATORS: command line option "--no-gnutls". */
        N_("disable init/deinit of gnutls"),
    },
    {
        "--no-gcrypt",
        /* TRANSLATORS: command line option "--no-gcrypt". */
        N_("disable init/deinit of gcrypt"),
    },
    { NULL, NULL },
};


/*
 * Display WeeChat copyright on standard output.
 */

void
args_display_copyright (void)
{
    string_fprintf (stdout, "\n");
    string_fprintf (
        stdout,
        /* TRANSLATORS: "%s %s" after "compiled on" is date and time. */
        _("WeeChat %s Copyright %s, compiled on %s %s\n"
          "Developed by %s <%s> "
          "- %s"),
        version_get_version_with_git (),
        WEECHAT_COPYRIGHT_DATE,
        version_get_compilation_date (),
        version_get_compilation_time (),
        WEECHAT_AUTHOR_NAME,
        WEECHAT_AUTHOR_EMAIL,
        WEECHAT_WEBSITE);
    string_fprintf (stdout, "\n");
}

/*
 * Display a list of command line arguments with their description on
 * standard output.
 */

void
args_display_list_args (char *args[][2])
{
    int i, j, length, length_fill, max_length_screen;
    const char *ptr_text;

    max_length_screen = 0;
    for (i = 0; args[i][0]; i++)
    {
        length = utf8_strlen_screen (_(args[i][0]));
        if (length > max_length_screen)
            max_length_screen = length;
    }

    for (i = 0; args[i][0]; i++)
    {
        ptr_text = _(args[i][0]);
        length = utf8_strlen_screen (ptr_text);
        length_fill = max_length_screen - length;
        string_fprintf (stdout, "  %s", ptr_text);
        for (j = 0; j < length_fill; j++)
        {
            string_fprintf (stdout, " ");
        }
        string_fprintf (stdout, "  %s\n", _(args[i][1]));
    }
}

/*
 * Display WeeChat help on standard output.
 */

void
args_display_help (void)
{
    const char *ptr_default;
    char *command;

    ptr_default = (weechat_headless) ? "weechat-headless" : "weechat";

    /* Escape control chars (argv[0] can contain any char). */
    command = (weechat_argv0) ?
        args_escape (weechat_argv0, strlen (weechat_argv0)) : NULL;

    args_display_copyright ();
    string_fprintf (stdout, "\n");
    string_fprintf (stdout,
                    "%s %s %s\n\n",
                    _("Usage:"),
                    (command) ? command : ptr_default,
                    _("[option...] [plugin:option...]"));
    free (command);
    args_display_list_args (weechat_args_common);
    string_fprintf (stdout, "\n");

    /* Extra options in headless mode */
    if (weechat_headless)
    {
        string_fprintf (stdout, "%s\n",
                        _("Extra options in headless mode:"));
        args_display_list_args (weechat_args_headless);
        string_fprintf (stdout, "\n");
    }

    /* Debug options */
    string_fprintf (stdout, "%s\n",
                    _("Debug options (for tools like valgrind, "
                      "DO NOT USE IN PRODUCTION):"));
    args_display_list_args (weechat_args_debug);
    string_fprintf (stdout, "\n");
}

/*
 * Escape a string for display: control chars (C0, DEL and C1) and invalid
 * UTF-8 bytes are replaced by "\xNN" (one for each byte).
 *
 * Note: result must be freed after use.
 */

char *
args_escape (const char *string, int length)
{
    char *result;
    const char *ptr_string, *ptr_end;
    int size, pos;

    if (!string || (length < 0))
        return NULL;

    /* Worst case: each byte is replaced by "\xNN" (4 bytes). */
    result = malloc ((length * 4) + 1);
    if (!result)
        return NULL;

    pos = 0;
    ptr_string = string;
    ptr_end = string + length;
    while (ptr_string < ptr_end)
    {
        size = 1;
        if ((unsigned char)ptr_string[0] >= 0x80)
        {
            size = utf8_char_size (ptr_string);
            if ((size <= 0) || (size > ptr_end - ptr_string)
                || !utf8_is_valid (ptr_string, 1, NULL))
            {
                size = 0;
            }
            else if ((size == 2) && ((unsigned char)ptr_string[0] == 0xC2)
                     && ((unsigned char)ptr_string[1] <= 0x9F))
            {
                /* C1 control char (U+0080 to U+009F). */
                size = 0;
            }
        }
        else if (((unsigned char)ptr_string[0] < 0x20)
                 || ((unsigned char)ptr_string[0] == 0x7F))
        {
            size = 0;
        }
        if (size > 0)
        {
            memcpy (result + pos, ptr_string, size);
            pos += size;
            ptr_string += size;
        }
        else
        {
            snprintf (result + pos, 5, "\\x%02X",
                      (unsigned char)ptr_string[0]);
            pos += 4;
            ptr_string++;
        }
    }
    result[pos] = '\0';

    return result;
}

/*
 * Get the invalid option after getopt_long() returned '?' or ':'.
 *
 * Argument "arg" is the command line argument where the error occurred,
 * "optstring" is the string with short options given to getopt_long().
 *
 * For a group of short options (for example "-aZs"), return the first char
 * which is not a short option, or the short option with a missing argument,
 * for example "-Z" (the whole UTF-8 char is returned, like "-é"); if this char
 * is "-", the whole argument is returned (for example "-a-").
 * For a long option, return it without its value (for example "--foo" for
 * "--foo=value").
 * Otherwise return the whole argument.
 *
 * Control chars and invalid UTF-8 bytes are escaped (see args_escape).
 *
 * Note: result must be freed after use.
 */

char *
args_get_invalid_option (const char *arg, const char *optstring)
{
    const char *ptr_arg, *ptr_opt, *ptr_bad;
    char *option, *result;
    int size, length;

    if (!arg)
        return NULL;

    /* Long option: return it without its value. */
    if ((arg[0] == '-') && (arg[1] == '-'))
    {
        ptr_arg = strchr (arg, '=');
        return args_escape (
            arg, (ptr_arg) ? (int)(ptr_arg - arg) : (int)strlen (arg));
    }

    /* Not an option: return the whole argument. */
    if ((arg[0] != '-') || !arg[1] || !optstring)
        return args_escape (arg, strlen (arg));

    /* Skip flags at beginning of optstring. */
    while ((optstring[0] == '-') || (optstring[0] == '+')
           || (optstring[0] == ':'))
    {
        optstring++;
    }

    /* Search the invalid char in the group of short options. */
    ptr_bad = NULL;
    for (ptr_arg = arg + 1; ptr_arg[0]; ptr_arg++)
    {
        ptr_opt = ((unsigned char)ptr_arg[0] < 0x80) ?
            strchr (optstring, ptr_arg[0]) : NULL;
        if (!ptr_opt || (ptr_arg[0] == ':'))
        {
            /* Not a short option. */
            ptr_bad = ptr_arg;
            break;
        }
        if (ptr_opt[1] == ':')
        {
            /* Option with argument: the rest is the argument. */
            if (!ptr_arg[1])
                ptr_bad = ptr_arg;  /* Missing argument. */
            break;
        }
    }
    /* Not found, or "-": return the whole argument. */
    if (!ptr_bad || (ptr_bad[0] == '-'))
        return args_escape (arg, strlen (arg));

    size = utf8_char_size (ptr_bad);
    length = strlen (ptr_bad);
    if ((size <= 0) || (size > length) || !utf8_is_valid (ptr_bad, 1, NULL))
        size = 1;

    option = args_escape (ptr_bad, size);
    if (!option)
        return NULL;
    length = strlen (option) + 2;
    result = malloc (length);
    if (result)
        snprintf (result, length, "-%s", option);
    free (option);

    return result;
}

/*
 * Check why a long option given on command line (for example "--foo" or
 * "--help=value") was rejected by getopt_long().
 *
 * The rules of getopt_long() are used: an exact name is used first, otherwise
 * the name can be abbreviated if it matches a single long option.
 *
 * If "index_option" is not NULL, it is set with the index of the matching
 * long option (-1 if not found or ambiguous).
 *
 * Return the error, see enum t_args_option_error.
 */

int
args_check_long_option (const char *arg, const struct option *long_options,
                        int *index_option)
{
    const char *ptr_name, *ptr_equal;
    int i, length, index_found, count;

    if (index_option)
        *index_option = -1;

    if (!arg || !long_options || (strncmp (arg, "--", 2) != 0))
        return ARGS_OPTION_ERROR_UNKNOWN;

    ptr_name = arg + 2;
    ptr_equal = strchr (ptr_name, '=');
    length = (ptr_equal) ? ptr_equal - ptr_name : (int)strlen (ptr_name);
    if (length == 0)
        return ARGS_OPTION_ERROR_UNKNOWN;

    /* Search an exact name, otherwise the long options with this prefix. */
    index_found = -1;
    count = 0;
    for (i = 0; long_options[i].name; i++)
    {
        if (strncmp (long_options[i].name, ptr_name, length) != 0)
            continue;
        if (!long_options[i].name[length])
        {
            /* Exact name. */
            index_found = i;
            count = 1;
            break;
        }
        if (count == 0)
            index_found = i;
        count++;
    }

    if (count == 0)
        return ARGS_OPTION_ERROR_UNKNOWN;
    if (count > 1)
        return ARGS_OPTION_ERROR_AMBIGUOUS;

    if (index_option)
        *index_option = index_found;

    if (ptr_equal && (long_options[index_found].has_arg == no_argument))
        return ARGS_OPTION_ERROR_ARGUMENT_NOT_ALLOWED;

    return ARGS_OPTION_ERROR_UNKNOWN;
}

/*
 * Display on standard error the hint to get help on command line options.
 */

void
args_display_usage_hint (void)
{
    const char *ptr_default, *ptr_command;
    char *command;

    ptr_default = (weechat_headless) ? "weechat-headless" : "weechat";

    /* Command displayed in the hint: basename of argv[0]. */
    ptr_command = NULL;
    if (weechat_argv0)
    {
        ptr_command = strrchr (weechat_argv0, '/');
        ptr_command = (ptr_command) ? ptr_command + 1 : weechat_argv0;
    }
    if (!ptr_command || !ptr_command[0])
        ptr_command = ptr_default;

    /* Escape control chars (argv[0] can contain any char). */
    command = args_escape (ptr_command, strlen (ptr_command));

    string_fprintf (stderr,
                    /* TRANSLATORS: "%s" is the command used to run WeeChat. */
                    _("Try \"%s --help\" for more information.\n"),
                    (command) ? command : ptr_default);

    free (command);
}

/*
 * Display on standard error an error about an invalid option (unknown,
 * ambiguous or given with an argument not allowed), followed by the hint to
 * get help.
 *
 * Argument "arg" is the command line argument where the error occurred.
 */

void
args_display_error_invalid_option (const char *arg,
                                   const struct option *long_options)
{
    char *option;
    int index_option;

    switch (args_check_long_option (arg, long_options, &index_option))
    {
        case ARGS_OPTION_ERROR_AMBIGUOUS:
            /* Long option: displayed without its argument. */
            option = args_get_invalid_option (arg, ARGS_SHORT_OPTIONS);
            string_fprintf (stderr,
                            _("Error: ambiguous option \"%s\"\n"),
                            (option) ? option : "?");
            free (option);
            break;
        case ARGS_OPTION_ERROR_ARGUMENT_NOT_ALLOWED:
            string_fprintf (stderr,
                            _("Error: option \"--%s\" does not take an "
                              "argument\n"),
                            long_options[index_option].name);
            break;
        default:
            option = args_get_invalid_option (arg, ARGS_SHORT_OPTIONS);
            string_fprintf (stderr,
                            _("Error: unknown option \"%s\"\n"),
                            (option) ? option : "?");
            free (option);
            break;
    }

    args_display_usage_hint ();
}

/*
 * Exit WeeChat after an error in command line arguments (usage error).
 */

void
args_exit_usage_error (void)
{
    /* Exit code must not be the one of doc generation. */
    weechat_doc_gen = 0;
    weechat_shutdown (ARGS_EXIT_USAGE_ERROR, 0);
}

/*
 * Check if a long option is allowed only in headless mode.
 *
 * Argument "val" is the value of the option in args_long_options.
 *
 * Return:
 *   1: option allowed only in headless mode
 *   0: option allowed in all modes
 */

int
args_option_is_headless_only (int val)
{
    return ((val == OPTION_DAEMON)
            || (val == OPTION_STDOUT)
            || (val == OPTION_DOCGEN)) ? 1 : 0;
}

/*
 * Copy the long options (args_long_options) to "long_options", without the
 * options allowed only in headless mode if "headless" is 0.
 *
 * The array "long_options" must have room for ARGS_NUM_LONG_OPTIONS options
 * (including the final NULL option).
 */

void
args_get_long_options (struct option *long_options, int headless)
{
    int i, count;

    if (!long_options)
        return;

    count = 0;
    for (i = 0; args_long_options[i].name; i++)
    {
        if (!headless && args_option_is_headless_only (args_long_options[i].val))
            continue;
        long_options[count++] = args_long_options[i];
    }
    memset (&long_options[count], 0, sizeof (long_options[count]));
}

/*
 * Parse command line arguments.
 *
 * This function does not require any initialization (it is called before
 * any other init of WeeChat): the options that display something and exit
 * (like "--help") are only recorded in args_action, and stop the parsing;
 * they are run later by function args_run_action.
 *
 * Arguments argc and argv come from main() function.
 */

void
args_parse (int argc, char *argv[])
{
    int opt, index_arg;
    char *option;
    struct option long_options[ARGS_NUM_LONG_OPTIONS];

    weechat_argv0 = (argv[0]) ? strdup (argv[0]) : NULL;
    weechat_upgrading = 0;
    weechat_home_force = NULL;
    weechat_home_temp = 0;
    weechat_home_delete_on_exit = 0;
    weechat_server_cmd_line = 0;
    weechat_force_plugin_autoload = NULL;
    weechat_doc_gen = 0;
    weechat_plugin_no_dlclose = 0;
    weechat_daemon = 0;
    weechat_log_stdout = 0;
    args_action = ARGS_ACTION_NONE;

    /* Options for headless mode are unknown in the other modes. */
    args_get_long_options (long_options, weechat_headless);

    optind = 0;
    opterr = 0;

    /*
     * The optstring starts with "-": the arguments are processed in order
     * (non-option arguments are returned as option 1), so before each call
     * to getopt_long(), optind is the index of the argument being processed.
     */
    while (1)
    {
        index_arg = (optind > 0) ? optind : 1;
        opt = getopt_long (argc, argv, ARGS_SHORT_OPTIONS, long_options, NULL);
        if (opt == -1)
        {
            /*
             * Some implementations of getopt_long() (like on BSD) return -1
             * for a "-" in a group of short options (for example "-a-"):
             * this is an invalid option, not the end of options.
             */
            if ((index_arg < argc) && (argv[index_arg][0] == '-')
                && argv[index_arg][1] && (strcmp (argv[index_arg], "--") != 0))
            {
                args_display_error_invalid_option (argv[index_arg],
                                                   long_options);
                args_exit_usage_error ();
            }
            break;
        }
        switch (opt)
        {
            case 1: /* Non-option argument */
                /* Argument ignored, it will be used by plugins. */
                break;
            case 'a': /* -a / --no-connect */
                /* Option ignored, it will be used by plugins/scripts. */
                break;
            case 'c': /* -c / --colors */
                args_action = ARGS_ACTION_COLORS;
                break;
            case 'd': /* -d / --dir */
                weechat_home_temp = 0;
                free (weechat_home_force);
                weechat_home_force = strdup (optarg);
                break;
            case 't': /* -t / --temp-dir */
                weechat_home_temp = 1;
                free (weechat_home_force);
                weechat_home_force = NULL;
                break;
            case 'h': /* -h / --help */
                args_action = ARGS_ACTION_HELP;
                break;
            case 'i': /* -i / --build-info */
                args_action = ARGS_ACTION_BUILD_INFO;
                break;
            case 'l': /* -l / --license */
                args_action = ARGS_ACTION_LICENSE;
                break;
            case 'p': /* -p / --no-plugin */
                free (weechat_force_plugin_autoload);
                weechat_force_plugin_autoload = strdup ("!*");
                break;
            case 'P': /* -P / --plugins */
                free (weechat_force_plugin_autoload);
                weechat_force_plugin_autoload = strdup (optarg);
                break;
            case 'r': /* -r / --run-command */
                if (!weechat_startup_commands)
                    weechat_startup_commands = weelist_new ();
                weelist_add (weechat_startup_commands, optarg,
                             WEECHAT_LIST_POS_END, NULL);
                break;
            case 's': /* -s / --no-script */
                /* Option ignored, it will be used by the scripting plugins. */
                break;
            case 'u': /* --upgrade */
                weechat_upgrading = 1;
                break;
            case OPTION_DOCGEN: /* --doc-gen (headless mode only) */
                weechat_doc_gen = 1;
                free (weechat_doc_gen_path);
                weechat_doc_gen_path = strdup (optarg);
                break;
            case OPTION_DAEMON: /* --daemon (headless mode only) */
                weechat_daemon = 1;
                break;
            case OPTION_STDOUT: /* --stdout (headless mode only) */
                weechat_log_stdout = 1;
                break;
            case 'v': /* -v / --version */
                args_action = ARGS_ACTION_VERSION;
                break;
            case OPTION_NO_DLCLOSE: /* --no-dlclose */
                /*
                 * Valgrind works better when dlclose() is not done after
                 * plugins are unloaded, it can display stack for plugins,*
                 * otherwise you'll see "???" in stack for functions of
                 * unloaded plugins.
                 * This option disables the call to dlclose(),
                 * it must NOT be used for other purposes!
                 */
                weechat_plugin_no_dlclose = 1;
                break;
            case OPTION_NO_GNUTLS: /* --no-gnutls */
                /*
                 * Electric-fence is not working fine when gnutls loads
                 * certificates and Valgrind reports many memory errors with
                 * gnutls.
                 * This option disables the init/deinit of gnutls,
                 * it must NOT be used for other purposes!
                 */
                weechat_no_gnutls = 1;
                break;
            case OPTION_NO_GCRYPT: /* --no-gcrypt */
                /*
                 * Valgrind reports many memory errors with gcrypt.
                 * This option disables the init/deinit of gcrypt,
                 * it must NOT be used for other purposes!
                 */
                weechat_no_gcrypt = 1;
                break;
            case ':':
                /*
                 * Some implementations of getopt_long() (like on BSD) return
                 * ':' for an argument given to a long option which does not
                 * take an argument (for example "--help=value").
                 */
                if ((strncmp (argv[index_arg], "--", 2) == 0)
                    && strchr (argv[index_arg], '='))
                {
                    args_display_error_invalid_option (argv[index_arg],
                                                       long_options);
                    args_exit_usage_error ();
                }
                option = args_get_invalid_option (argv[index_arg],
                                                  ARGS_SHORT_OPTIONS);
                string_fprintf (stderr,
                                _("Error: missing argument for \"%s\" option\n"),
                                (option) ? option : "?");
                free (option);
                args_display_usage_hint ();
                args_exit_usage_error ();
                break;
            case '?':
                args_display_error_invalid_option (argv[index_arg],
                                                   long_options);
                args_exit_usage_error ();
                break;
            default:
                /* Ignore any other error. */
                break;
        }
        /* Stop at first option that displays something and exits. */
        if (args_action != ARGS_ACTION_NONE)
            break;
    }

    /* Option "--daemon" is ignored if "--doc-gen" is given. */
    if (weechat_doc_gen)
        weechat_daemon = 0;

    /* Option "--stdout" is ignored if "--daemon" is given. */
    if (weechat_daemon)
        weechat_log_stdout = 0;
}

/*
 * Run the option that displays something and exits (like "--help"), if such
 * option was given on command line, then exit WeeChat.
 *
 * Nothing is done if no such option was given.
 */

void
args_run_action (void)
{
    switch (args_action)
    {
        case ARGS_ACTION_NONE:
            return;
        case ARGS_ACTION_COLORS:
            gui_color_display_terminal_colors ();
            break;
        case ARGS_ACTION_HELP:
            args_display_help ();
            break;
        case ARGS_ACTION_BUILD_INFO:
            debug_build_info ();
            break;
        case ARGS_ACTION_LICENSE:
            args_display_copyright ();
            string_fprintf (stdout, "\n");
            string_fprintf (stdout, "%s%s", WEECHAT_LICENSE_TEXT);
            break;
        case ARGS_ACTION_VERSION:
            string_fprintf (stdout, version_get_version ());
            fprintf (stdout, "\n");
            break;
    }
    weechat_shutdown (EXIT_SUCCESS, 0);
}
