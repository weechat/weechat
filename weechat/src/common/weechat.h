/*
 * Copyright (c) 2004 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef __WEECHAT_H
#define __WEECHAT_H 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#if defined(ENABLE_NLS) && !defined(_)
    #include <locale.h>
    #include <libintl.h>
    #define _(x) gettext(x)
    #ifdef gettext_noop
        #define N_(string) gettext_noop (string)
    #else
        #define N_(string) (string)
    #endif
#endif
#if !defined(_)
    #define _(x) (x)
    #define N_(string) (string)
#endif


#define WEECHAT_COPYRIGHT PACKAGE_NAME " (c) 2004 by Wee Team"
#define WEECHAT_WEBSITE "http://weechat.flashtux.org"

#define WEECHAT_ERROR   _(PACKAGE_NAME " Error:")
#define WEECHAT_WARNING _(PACKAGE_NAME " Warning:")

/* log file */

#define WEECHAT_LOG_NAME "weechat.log"

/* license */

#define WEE_LICENSE \
    PACKAGE_STRING " (c) Copyright 2004, compiled on " __DATE__ " " __TIME__ \
    "\nDeveloped by FlashCode <flashcode@flashtux.org>\n" \
    "             Bounga <bounga@altern.org>\n" \
    "             Xahlexx <xahlexx@weeland.org>\n" \
    "Website: " WEECHAT_WEBSITE "\n\n" \
    "This program is free software; you can redistribute it and/or modify\n" \
    "it under the terms of the GNU General Public License as published by\n" \
    "the Free Software Foundation; either version 2 of the License, or\n" \
    "(at your option) any later version.\n" \
    "\n", \
    \
    "This program is distributed in the hope that it will be useful,\n" \
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n" \
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n" \
    "GNU General Public License for more details.\n" \
    "\n" \
    "You should have received a copy of the GNU General Public License\n" \
    "along with this program; if not, write to the Free Software\n" \
    "Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA\n\n"
    
#define WEE_USAGE1 \
    PACKAGE_STRING " (c) Copyright 2004, compiled on " __DATE__ " " __TIME__ \
    "\nDeveloped by FlashCode, Bounga and Xahlexx - " WEECHAT_WEBSITE "\n\n" \
    "Usage: %s [options ...]\n" \
    "   or: %s [irc://[nickname[:password]@]irc.example.org[:port][/channel] ...]\n\n"

#define WEE_USAGE2 \
    "  -c, --config        config file help (list of options)\n" \
    "  -h, --help          this help screen\n" \
    "  -l, --license       display WeeChat license\n" \
    "  -v, --version       display WeeChat version\n\n"

/* directory separator, depending on OS */

#ifdef _WIN32
    #define DIR_SEPARATOR       "\\"
    #define DIR_SEPARATOR_CHAR  '\\'
#else
    #define DIR_SEPARATOR       "/"
    #define DIR_SEPARATOR_CHAR  '/'
#endif

/* global variables and functions */

extern int quit_weechat;
extern char *weechat_home;

extern void wee_log_printf (char *, ...);
extern void wee_shutdown ();

#endif /* weechat.h */
