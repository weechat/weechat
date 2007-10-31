/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* weechat-trigger-libirc.c: Tiny libirc for trigger plugin */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "weechat-trigger-libc.h"
#include "weechat-trigger-libirc.h"

typedef struct irc_msg_type
{
    char *command;
    void (*parser) (irc_msg *, char *);
} irc_msg_type;

void irc_parse_common (irc_msg *, char *);
void irc_parse_join (irc_msg *, char *);
void irc_parse_kick (irc_msg *, char *);
void irc_parse_mode (irc_msg *, char *);
void irc_parse_nick (irc_msg *, char *);
void irc_parse_privmsg (irc_msg *, char *);
void irc_parse_quit (irc_msg *, char *);
void irc_parse_wallops (irc_msg *, char *);

irc_msg_type irc_msg_types[] = 
{
    /* kill and error commands to add */
    { "invite", irc_parse_common },
    { "join", irc_parse_join },
    { "kick", irc_parse_kick },
    { "mode", irc_parse_mode },
    { "nick", irc_parse_nick },
    { "notice", irc_parse_common },
    { "part", irc_parse_common },
    { "ping", irc_parse_common },
    { "pong", irc_parse_common },
    { "privmsg", irc_parse_privmsg },
    { "quit", irc_parse_quit },
    { "topic", irc_parse_common },
    { "wallops", irc_parse_wallops },
    {NULL, NULL }
};

char *irc_ctcp_types[] =
{
    "action", "dcc", "sed", "finger", "version", "source",
    "userinfo", "clientinfo","errmsg", "ping", "time",
    NULL
};

void irc_parse_userhost (irc_msg *m)
{
    char *excl, *ar;
    
    if (m && m->userhost)
    {
	excl = strchr (m->userhost, '!');
	if (excl) {
	    m->nick = c_strndup (m->userhost, excl-m->userhost);
	    ar = strchr (excl, '@');
	    if (ar)
	    {
		m->user = c_strndup (excl+1, ar-excl-1);
		ar++;
		m->host = strdup (ar);
	    }
	}
	else
	    m->host = strdup (m->userhost);
    }
}

void irc_parse_common (irc_msg *m, char *p)
{
    char *spc;
    
    p++;
    if (p && *p)
    {
	spc = strchr (p, ' ');
	if (spc)
	{
	    m->channel = c_strndup (p, spc-p);
	    spc++;
	    if (spc && *spc && spc[0] == ':')
		m->message = strdup (spc+1);
	}
    }
}


void irc_parse_snotice (irc_msg *m, char *p)
{
    char *c;

    if (p)
    {
	c = strchr (p, ':');
	c++;
	if (c && *c)
	{
	    m->message = strdup (c);
	    m->command = strdup ("notice");
	}
    }
}

void irc_parse_privmsg (irc_msg *m, char *p)
{
    char ctcp_buf[32];
    char *c, *d;
    char **it;

    irc_parse_common (m, p);
    
    c = m->message;
    if (c)
    {
	if (*c == '\x01')
	{
	    c++;
	    d = strchr(c, '\x01');
	    if (d)
	    {
		for (it = irc_ctcp_types; *it; it++)
		{
		    if (strncasecmp (c, *it, strlen(*it)) == 0)
		    {
			snprintf (ctcp_buf, sizeof(ctcp_buf), "ctcp-%s", *it);
			free (m->command);
			m->command = strdup (ctcp_buf);
			c += strlen(*it);
			d = c_strndup (c+1, d-c+1);
			free (m->message);
			m->message = d;
			break;
		    }
		}
	    }
	}
    }
}

void irc_parse_join (irc_msg *m, char *p)
{
    p++;
    if (p && *p && p[0] == ':')
    {
	m->channel = strdup (p+1);
    }
}

void irc_parse_nick (irc_msg *m, char *p)
{
    p++;
    if (p && *p && p[0] == ':')
    {
	m->message = strdup (p+1);
    }
}

void irc_parse_mode (irc_msg *m, char *p)
{
    char *spc;
    
    p++;
    if (p && *p)
    {
	spc = strchr (p, ' ');
	if (spc)
	{
	    m->channel = c_strndup (p, spc-p);
	    spc++;
	    if (spc && *spc)
		m->message = strdup (spc);
	}
    }
}

void irc_parse_quit (irc_msg *m, char *p)
{
    p++;
    if (p && *p && p[0] == ':')
    {
	m->message = strdup (p+1);
    }
}

void irc_parse_kick (irc_msg *m, char *p)
{
    char *spc1, *spc2;
    
    p++;
    if (p && *p)
    {
	spc1 = strchr (p, ' ');
	if (spc1)
	{
	    m->channel = c_strndup (p, spc1-p);
	    spc1++;
	    if (spc1 && *spc1)
	    {
		spc2 = strchr(spc1, ' ');
		if (spc2)
		{
		    m->data = c_strndup (spc1, spc2-spc1);
		    spc2++;
		    if (spc2 && *spc2 && spc2[0] == ':')
			m->message = strdup (spc2+1);
		}
	    }
	}
    }
}

void irc_parse_wallops (irc_msg *m, char *p)
{
    p++;
    if (p && *p && p[0] == ':')
    {
	m->message = strdup (p+1);
    }
}

void irc_parse_numeric (irc_msg *m, char *p)
{
    char *spc1, *spc2;
    
    p++;
    if (p && *p)
    {
	spc1 = strchr (p, ' ');
	if (spc1)
	{
	    spc2 = strchr (spc1, ' ');
	    spc2++;
	    if (spc2 && *spc2 && spc2[0] == ':')
		m->message = strdup (spc2+1);
	}
    }
}



irc_msg *irc_parse_msg (char *msg)
{
    irc_msg *m;
    char *spc1, *spc2;
    irc_msg_type *it;
   
    m = (irc_msg *) malloc(sizeof(irc_msg));
    
    if (m)
    {
	m->userhost = NULL;
	m->nick = NULL;
	m->user = NULL;
	m->host = NULL;
	m->command = NULL;
	m->channel = NULL;
	m->message = NULL;
	m->data = NULL;
	
	if (msg && msg[0] == ':')
	{
	    spc1 = strchr (msg, ' ');
	    if (spc1)
	    { 
		spc1++;
		spc2 = strchr (spc1, ' ');
		if (spc2)
		{
		    m->command = c_strndup (spc1, spc2-spc1);
		    c_ascii_tolower (m->command);
		    
		    m->userhost = c_strndup (msg+1, spc1-msg-2);
		    irc_parse_userhost (m);
		    
		    if (c_is_number (m->command))
			irc_parse_numeric (m, spc2);
		    else
			for (it = irc_msg_types; it->command; it++)
			{
			    if (strcmp (m->command, it->command) == 0)
			    {
				it->parser (m, spc2);
				break;
			    }
			}
		}
	    }
	}

	if (msg && strncasecmp("NOTICE ", msg, 7) == 0)
	    irc_parse_snotice (m, msg);
    }    
    return m;
}

void irc_msg_free (irc_msg *m)
{
    if (m)
    {
	if (m->userhost) free (m->userhost);
	if (m->nick) free (m->nick);
	if (m->user) free (m->user);
	if (m->host) free (m->host);
	if (m->command) free (m->command);
	if (m->channel) free (m->channel);
	if (m->message) free (m->message);
	if (m->data) free (m->data);
	free (m);
    }
}
