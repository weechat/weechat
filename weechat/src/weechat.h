/*
 * Copyright (c) 2003 by FlashCode <flashcode@flashtux.org>
 *                       Bounga <bounga@altern.org>
 *                       Xahlexx <xahlexx@tuxisland.org>
 * See README for License detail.
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

#include <stdio.h>
#include <libintl.h>

#define _(string) gettext(string)
#define N_(string) (string)

#define WEECHAT_NAME    "WeeChat"
#define WEECHAT_VERSION "0.0.2-pre2"

#define WEECHAT_NAME_AND_VERSION WEECHAT_NAME " " WEECHAT_VERSION
#define WEECHAT_COPYRIGHT WEECHAT_NAME " (c) 2003 by Wee Team"
#define WEECHAT_WEBSITE "http://weechat.flashtux.org"

#define WEECHAT_ERROR   _(WEECHAT_NAME " Error:")
#define WEECHAT_WARNING _(WEECHAT_NAME " Warning:")

/* debug mode, 0=normal use, 1=some debug msg, 2=full debug (developers only) */
#define DEBUG           0

/* log file */

#define WEECHAT_LOG_NAME "weechat.log"

/* license */

#define WEE_LICENSE \
    WEECHAT_NAME_AND_VERSION " (c) Copyright 2003, compiled on " __DATE__ __TIME__ \
    "Developed by FlashCode <flashcode@flashtux.org>\n" \
    "             Bounga <bounga@altern.org>\n" \
    "             Xahlexx <xahlexx@tuxisland.org>\n\n" \
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
    
#define WEE_USAGE \
    WEECHAT_NAME_AND_VERSION " (c) Copyright 2003, compiled on " __DATE__ __TIME__ \
    "Developed by FlashCode <flashcode@flashtux.org>\n" \
    "             Bounga <bounga@altern.org>\n" \
    "             Xahlexx <xahlexx@tuxisland.org>\n\n" \
    "                                    Bounga <bounga@altern.org>\n" \
    "                                    Xahlexx <xahlexx@tuxisland.org>\n\n" \
    "  -h, --help          this help screen\n", \
    "  -l, --license       display WeeChat license\n" \
    "  -v, --version       display WeeChat version\n\n"

/*    "  -d, --display       choose X display\n" \*/


/*#define DEFAULT_DISPLAY         ":0" */


/*extern char *display_name; */
int quit_weechat;

extern int quit_weechat;

extern void log_printf (char *, ...);
extern void wee_shutdown ();

#endif /* weechat.h */
