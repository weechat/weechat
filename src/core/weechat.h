/*
 * Copyright (C) 2003-2025 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
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

#ifndef WEECHAT_H
#define WEECHAT_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#include <locale.h>

#if ENABLE_NLS == 1 && !defined(_)
    #include <libintl.h>
    #define _(string) gettext(string)
    #define NG_(single,plural,number) ngettext(single,plural,number)
    #ifdef gettext_noop
        #define N_(string) gettext_noop(string)
    #else
        #define N_(string) (string)
    #endif /* gettext_noop */
#endif /* ENABLE_NLS == 1 && !defined(_) */
#if !defined(_)
    #define _(string) (string)
    #define NG_(single,plural,number) ((number == 1) ? single : plural)
    #define N_(string) (string)
    #define gettext(string) (string)
#endif /* !defined(_) */
#define AI(string) (string)


#define WEECHAT_COPYRIGHT_DATE   "(C) 2003-2025"
#define WEECHAT_WEBSITE          "https://weechat.org/"
#define WEECHAT_WEBSITE_DOWNLOAD "https://weechat.org/download/"

/* log file */
#define WEECHAT_LOG_NAME "weechat.log"

/* license */
#define WEECHAT_LICENSE_TEXT \
    "WeeChat is free software; you can redistribute it and/or modify\n" \
    "it under the terms of the GNU General Public License as published by\n" \
    "the Free Software Foundation; either version 3 of the License, or\n" \
    "(at your option) any later version.\n" \
    "\n", \
    \
    "WeeChat is distributed in the hope that it will be useful,\n" \
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
    "GNU General Public License for more details.\n" \
    "\n" \
    "You should have received a copy of the GNU General Public License\n" \
    "along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.\n\n"

/* directory separator, depending on OS */
#ifdef _WIN32
    #define DIR_SEPARATOR       "\\"
    #define DIR_SEPARATOR_CHAR  '\\'
#else
    #define DIR_SEPARATOR       "/"
    #define DIR_SEPARATOR_CHAR  '/'
#endif /* _WIN32 */

/* some systems like GNU/Hurd do not define PATH_MAX */
#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif /* PATH_MAX */

/* internal charset */
#define WEECHAT_INTERNAL_CHARSET "UTF-8"

/* name of environment variable with an extra lib dir */
#define WEECHAT_EXTRA_LIBDIR "WEECHAT_EXTRA_LIBDIR"

struct t_weelist;

/* global variables and functions */
extern int weechat_headless;
extern int weechat_daemon;
extern int weechat_log_stdout;
extern int weechat_debug_core;
extern char *weechat_argv0;
extern int weechat_upgrading;
extern int weechat_first_start;
extern time_t weechat_first_start_time;
extern struct timeval weechat_current_start_timeval;
extern int weechat_upgrade_count;
extern int weechat_quit;
extern volatile sig_atomic_t weechat_quit_signal;
extern volatile sig_atomic_t weechat_reload_signal;
extern char *weechat_home_force;
extern int weechat_home_temp;
extern int weechat_home_delete_on_exit;
extern char *weechat_config_dir;
extern char *weechat_data_dir;
extern char *weechat_state_dir;
extern char *weechat_cache_dir;
extern char *weechat_runtime_dir;
extern int weechat_locale_ok;
extern char *weechat_local_charset;
extern int weechat_plugin_no_dlclose;
extern int weechat_no_gnutls;
extern int weechat_no_gcrypt;
extern struct t_weelist *weechat_startup_commands;
extern int weechat_auto_connect;
extern int weechat_auto_load_scripts;

extern void weechat_term_check (void);
extern void weechat_shutdown (int return_code, int crash);
extern void weechat_init_gettext (void);
extern void weechat_init (int argc, char *argv[], void (*gui_init_cb)(void));
extern void weechat_end (void (*gui_end_cb)(int clean_exit));

#endif /* WEECHAT_H */
