/*
 * Copyright (C) 2003-2012 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_H
#define __WEECHAT_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/time.h>

#include <locale.h>

#if defined(ENABLE_NLS) && !defined(_)
    #ifdef HAVE_LIBINTL_H
        #include <libintl.h>
    #else
        #include "../../intl/libintl.h"
    #endif
    #define _(string) gettext(string)
    #define NG_(single,plural,number) ngettext(single,plural,number)
    #ifdef gettext_noop
        #define N_(string) gettext_noop(string)
    #else
        #define N_(string) (string)
    #endif
#endif
#if !defined(_)
    #define _(string) (string)
    #define NG_(single,plural,number) (plural)
    #define N_(string) (string)
#endif


#define WEECHAT_COPYRIGHT_DATE   "(C) 2003-2012"
#define WEECHAT_WEBSITE          "http://www.weechat.org/"
#define WEECHAT_WEBSITE_DOWNLOAD "http://www.weechat.org/download"

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
    "along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.\n\n"

/* directory separator, depending on OS */

#ifdef _WIN32
    #define DIR_SEPARATOR       "\\"
    #define DIR_SEPARATOR_CHAR  '\\'
#else
    #define DIR_SEPARATOR       "/"
    #define DIR_SEPARATOR_CHAR  '/'
#endif

/* some systems (like GNU/Hurd) doesn't define PATH_MAX */

#ifndef PATH_MAX
    #define PATH_MAX 4096
#endif

/* internal charset */

#define WEECHAT_INTERNAL_CHARSET "UTF-8"

/* global variables and functions */

extern int weechat_debug_core;
extern char *weechat_argv0;
extern int weechat_upgrading;
extern time_t weechat_first_start_time;
extern struct timeval weechat_current_start_timeval;
extern int weechat_upgrade_count;
extern int weechat_quit;
extern char *weechat_home;
extern char *weechat_local_charset;
extern int weechat_plugin_no_dlclose;
extern int weechat_no_gnutls;
extern int weechat_no_gcrypt;
extern char *weechat_startup_commands;

extern void weechat_shutdown (int return_code, int crash);

#endif /* __WEECHAT_H */
