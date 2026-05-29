/*
 * SPDX-FileCopyrightText: 2011-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

/* Terminal functions for Curses GUI */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#ifndef WEECHAT_HEADLESS
#ifdef HAVE_NCURSESW_CURSES_H
#ifdef __sun
#include <ncurses/term.h>
#else
#include <ncursesw/term.h>
#endif /* __sun */
#else
#include <term.h>
#endif /* HAVE_NCURSESW_CURSES_H */
#endif /* WEECHAT_HEADLESS */

#include "../../core/weechat.h"


/*
 * Set "eat_newline_glitch" variable.
 *
 * With value 0, this is used to not auto insert newline char at end of lines
 * displayed, so that long words like URLs are not cut when they are selected
 * with mouse.
 */

void
gui_term_set_eat_newline_glitch (int value)
{
#if !defined(WEECHAT_HEADLESS) && defined(HAVE_EAT_NEWLINE_GLITCH)
    eat_newline_glitch = value;
#else
    /* make C compiler happy */
    (void) value;
#endif
}

/*
 * Auto-detects the terminal background as "light" or "dark".
 *
 * Best-effort, in order:
 *
 *   1. Environment variable COLORFGBG (set by rxvt, urxvt, Konsole,
 *      ...): "fg;bg" or "fg;default;bg"; the last ';'-separated
 *      component is the bg ANSI color index. 0-6 + 8 are dark,
 *      7 + 9-15 are light.
 *
 *   2. OSC 11 escape query on /dev/tty: write "\033]11;?\033\\",
 *      wait up to 100 ms for a reply of the form
 *      "...rgb:RRRR/GGGG/BBBB..." (each component is 1-4 hex digits
 *      depending on the terminal). Classify the answer by Rec. 601
 *      luminance computed on the high nibble of each component
 *      (resolution is plenty for a light/dark binary decision and
 *      avoids width-normalization headaches).
 *
 * /dev/tty is used rather than stdin/stdout so a pipe redirection
 * does not defeat detection. The select() timeout caps how long a
 * silent terminal can stall startup. Headless mode and any I/O error
 * short-circuit to the safe default 0 (dark), which is also returned
 * when both probes are inconclusive.
 *
 * MUST be called before curses init: it briefly puts the tty in raw
 * mode and reads/writes escape sequences directly.
 *
 * Returns 1 if a light background is detected, otherwise 0 (0 is the
 * preferred value when detection is unsure).
 */

int
gui_term_theme_is_light (void)
{
    const char *colorfgbg, *p;
    char *endptr;
    long bg;
    int fd, n, i, len;
    struct termios old_attr, new_attr;
    fd_set rfds;
    struct timeval tv;
    char buf[256], *q, *qend;
    unsigned long comp_high[3], luminance;

    if (weechat_headless)
        return 0;

    /* 1. COLORFGBG */
    colorfgbg = getenv ("COLORFGBG");
    if (colorfgbg && colorfgbg[0])
    {
        p = strrchr (colorfgbg, ';');
        if (p)
        {
            bg = strtol (p + 1, &endptr, 10);
            if (endptr != p + 1 && *endptr == '\0')
            {
                if (bg == 7 || (bg >= 9 && bg <= 15))
                    return 1;
                if ((bg >= 0 && bg <= 6) || bg == 8)
                    return 0;
            }
        }
    }

    /* 2. OSC 11 query on the controlling terminal */
    fd = open ("/dev/tty", O_RDWR | O_NOCTTY);
    if (fd < 0)
        return 0;

    if (tcgetattr (fd, &old_attr) != 0)
    {
        close (fd);
        return 0;
    }
    new_attr = old_attr;
    new_attr.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
    new_attr.c_cc[VMIN] = 0;
    new_attr.c_cc[VTIME] = 0;
    if (tcsetattr (fd, TCSANOW, &new_attr) != 0)
    {
        close (fd);
        return 0;
    }

    n = -1;
    if (write (fd, "\033]11;?\033\\", 8) == 8)
    {
        FD_ZERO (&rfds);
        FD_SET (fd, &rfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  /* 100 ms cap on terminals that won't reply */
        if (select (fd + 1, &rfds, NULL, NULL, &tv) > 0)
            n = read (fd, buf, sizeof (buf) - 1);
    }

    tcsetattr (fd, TCSANOW, &old_attr);
    close (fd);

    if (n <= 0)
        return 0;
    buf[n] = '\0';

    q = strstr (buf, "rgb:");
    if (!q)
        return 0;
    q += 4;

    for (i = 0; i < 3; i++)
    {
        qend = q;
        while ((*qend >= '0' && *qend <= '9')
               || (*qend >= 'a' && *qend <= 'f')
               || (*qend >= 'A' && *qend <= 'F'))
            qend++;
        len = (int)(qend - q);
        if (len < 1 || len > 4)
            return 0;
        /* high nibble of this component: width-independent brightness */
        comp_high[i] = strtoul (q, NULL, 16) >> ((len - 1) * 4);
        q = qend;
        if (i < 2)
        {
            if (*q != '/')
                return 0;
            q++;
        }
    }

    /* Rec. 601 with coefficients summing to 1000; max = 15 * 1000 */
    luminance = (299UL * comp_high[0]
                 + 587UL * comp_high[1]
                 + 114UL * comp_high[2]);
    return (luminance > 7500) ? 1 : 0;
}
