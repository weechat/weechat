/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* weechat-trigger.c: Trigger plugin for WeeChat */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../weechat-plugin.h"
#include "weechat-trigger.h"
#include "weechat-trigger-libirc.h"
#include "weechat-trigger-libc.h"


WEECHAT_PLUGIN_NAME("trigger");
WEECHAT_PLUGIN_DESCRIPTION("Trigger plugin for WeeChat");
WEECHAT_PLUGIN_AUTHOR("FlashCode <flashcode@flashtux.org>");
WEECHAT_PLUGIN_VERSION("0.1");
WEECHAT_PLUGIN_LICENSE("GPL");

t_weechat_trigger *weechat_trigger_list = NULL;
t_weechat_trigger *weechat_trigger_last = NULL;

t_weechat_trigger *weechat_trigger_alloc (char *pattern, char *domain, char *commands,
					  char *channels, char *servers,
					  char *action, char *cmd)
{
    t_weechat_trigger *new;
    
    new = (t_weechat_trigger *)malloc (sizeof (t_weechat_trigger));
    if (new)
    {
	new->pattern = strdup (pattern);
	new->domain = strdup (domain);
	
	new->commands = c_explode_string (commands, ",", 0, NULL);
	new->channels = c_explode_string (channels, ",", 0, NULL);
	new->servers = c_explode_string (servers, ",", 0, NULL);
	new->action = strdup (action);
	
	if (cmd)
	    new->cmds = c_split_multi_command (cmd, ';');
	else
	    new->cmds = NULL;

	new->prev_trigger = NULL;
	new->next_trigger = NULL;
    }
    return new;
}

void weechat_trigger_free (t_weechat_trigger *trig)
{
    if (trig)
    {
	if (trig->pattern)
	    free (trig->pattern);
	if (trig->domain)
	    free (trig->domain);
	if (trig->commands)
	    c_free_exploded_string (trig->commands);	
	if (trig->channels)
	    c_free_exploded_string (trig->channels);
	if (trig->servers)
	    c_free_exploded_string (trig->servers);
	if (trig->action)
	    free (trig->action);
	if (trig->cmds)
	    c_free_multi_command (trig->cmds);
	free (trig);
    }
}

int
weechat_trigger_action_exists (char *action)
{
    if (strcasecmp(action, "ignore") == 0
	|| strcasecmp(action, "display") == 0
	|| strcasecmp(action, "highlight") == 0
	|| strcasecmp(action, "run") == 0)
	return 1;
    
    return 0;
}

int
weechat_trigger_domain_exists (char *action)
{
    if (strcasecmp(action, "*") == 0
	|| strcasecmp(action, "user") == 0
	|| strcasecmp(action, "nick") == 0
	|| strcasecmp(action, "userhost") == 0
	|| strcasecmp(action, "msg") == 0)
	return 1;
    
    return 0;
}

/*
 * weechat_trigger_add : create and add a new trigger in triggers list
 *       return :
 *            0 : adding was successfull
 *            1 : memory error
 *            2 : bad arguments
 */

int
weechat_trigger_add (int argc, char **argv)
{
    int ret = 2;
    char *rcmd;
    t_weechat_trigger *trig = NULL;
    
    if (argc == 6)
    {
	if (weechat_trigger_action_exists (argv[5])
	    && strcasecmp (argv[5], "run") != 0)
	{
	    trig = weechat_trigger_alloc (
		argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], NULL);
	    if (!trig)
		ret = 1;
	}
    }
    else if (argc > 6)
    {
	if (strcasecmp (argv[5], "run") == 0)
	{
	    rcmd = c_join_string (&argv[6], " ");
	    if (rcmd)
	    {
		trig = weechat_trigger_alloc (
		    argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], rcmd);
		c_free_joined_string (rcmd);
		if (!trig)
		    ret = 1;
	    }
	}
    }
    
    if (trig)
    {
	if (weechat_trigger_last)
	{
	    weechat_trigger_last->next_trigger = trig;
	    trig->prev_trigger = weechat_trigger_last;
	    weechat_trigger_last = trig;
	}
	else
	{
	    weechat_trigger_list = trig;
	    weechat_trigger_last = trig;
	}
	ret = 0;
    }
    
    return ret;
}

/*
 *
 */

int
weechat_trigger_move (int from, int to)
{
    t_weechat_trigger *l, *f, *t;
    int ret, i;

    if (from == to)
	return 0;

    ret = 1;
    f = NULL;
    t = NULL;
    l = weechat_trigger_list;
    i = 0;
    while (l)
    {
	i++;
	if (i == from)
	    f = l;
	if (i == to)
	    t = l;
	l = l->next_trigger;
    }
    
    if (f && t)
    {
	if (weechat_trigger_list == f)
	    weechat_trigger_list = f->next_trigger;
	
	if (weechat_trigger_last == f)
	    weechat_trigger_last = f->prev_trigger;
	
	if (f->prev_trigger)
	    f->prev_trigger->next_trigger = f->next_trigger;
	if (f->next_trigger)
	    f->next_trigger->prev_trigger = f->prev_trigger;
	
	if (to > from && !t->next_trigger)
	{
	    t->next_trigger = f;
	    f->prev_trigger = t;
	    f->next_trigger = NULL;
	    
	    weechat_trigger_last = f;
	}
	else
	{
	    if (to > from && t->next_trigger)
		t = t->next_trigger;

	    if (t->prev_trigger)
		t->prev_trigger->next_trigger = f;	
	    f->next_trigger = t;
	    f->prev_trigger = t->prev_trigger;
	    t->prev_trigger = f;
	
	    if (t == weechat_trigger_list)
		weechat_trigger_list = f;
	}
	
	ret = 0;
    }
    
    return ret;
}

/*
 * weechat_trigger_move : remove a trigger trigger in triggers list
 *                        by its number
 *          return : 0 = success ; 1 = failed
 */

int
weechat_trigger_remove (int n)
{
    t_weechat_trigger *l, *p;
    int ret;
    
    ret = 1;
    l = weechat_trigger_list;
    p = NULL;
    while (l && n >= 0)
    {
	n--;
	if (n == 0) {
	    p = l;
	    break;
	}
	l = l->next_trigger;
    }
    
    if (p)
    {
	if (p->prev_trigger)
	    p->prev_trigger->next_trigger = p->next_trigger;
	
	if (p->next_trigger)
	    p->next_trigger->prev_trigger = p->prev_trigger;
	
	if (p == weechat_trigger_list)
	    weechat_trigger_list = p->next_trigger;
	
	if (p == weechat_trigger_last)
	    weechat_trigger_last = p->prev_trigger;
	
	weechat_trigger_free (p);
	ret = 0;
    }
    
    return ret;
}

void
weechat_trigger_display (t_weechat_plugin *plugin)
{
    t_weechat_trigger *l;
    int i;
    char *commands, *channels, *servers, *cmds;

    if (weechat_trigger_list)
    {
	plugin->print_server (plugin, "Trigger list:");
	l = weechat_trigger_list;
	i = 1;
	while (l)
	{
	    commands = c_join_string (l->commands, ",");
	    channels = c_join_string (l->channels, ",");
	    servers = c_join_string (l->servers, ",");
	    cmds = c_join_string (l->cmds, ";");
	    
	    if (strcasecmp (l->action, "run") == 0)
		plugin->print_server (plugin,
				      "[%d] pattern '%s/%s' for irc command(s) '%s'"
				      " for channel(s) '%s' on server(s) '%s'"
				      " do '%s' command(s) '%s'",
				      i, l->pattern, l->domain, commands, channels,
				      servers, l->action, cmds);
	    else
		plugin->print_server (plugin,
				      "[%d] pattern '%s/%s' for irc command(s) '%s'"
				      " for channel(s) '%s' on server(s) '%s'"
				      " do '%s'",
				      i, l->pattern, l->domain, commands, channels,
				      servers, l->action);
	    
	    if (commands) c_free_joined_string (commands);
	    if (channels) c_free_joined_string (channels);
	    if (servers) c_free_joined_string (servers);
	    if (cmds) c_free_joined_string (cmds);
	    l = l->next_trigger;
	    i++;
	}
    }
    else
	plugin->print_server (plugin, "Trigger list: no trigger defined.");
}

/*
 * weechat_trigger_cmd: /trigger command
 */

int
weechat_trigger_cmd (t_weechat_plugin *plugin,
                     int cmd_argc, char **cmd_argv,
                     char *handler_args, void *handler_pointer)
{
    int argc, ret, n1, n2;
    char **argv;

    /* make C compiler happy */
    (void) cmd_argc;
    (void) handler_args;
    (void) handler_pointer;
    
    ret = 0;
    
    if (cmd_argv[2])
	argv = c_explode_string (cmd_argv[2], " ", 0, &argc);
    else
    {
        argv = NULL;
        argc = 0;
    }
    
    if (argv)
    {
	if (strcasecmp (argv[0], "add") == 0)
	{
	    if (argc >= 7)
	    {
		switch (weechat_trigger_add (argc-1, &argv[1]))
		{
		case 0:
		    plugin->print_server (plugin, "Trigger: trigger successfully created");
		    break;
		case 1:
		    plugin->print_server (plugin, "Trigger error: memory error while creating trigger");
		    break;
		case 2:
		    plugin->print_server (plugin, "Trigger error: 'action' or 'domain' option seems misused");
		    break;
		}
	    }
	    else
		plugin->print_server (plugin, "Trigger error: missing arguments");
	}
	else if (strcasecmp (argv[0], "move") == 0)
	{
	    if (argc == 3)
	    {
		n1 = c_to_number (argv[1]);
		n2 = c_to_number (argv[2]);
		if (n1 > 0 && n2 > 0)
		{
		    if (weechat_trigger_move (n1, n2) == 0)
			plugin->print_server (plugin, "Trigger: trigger sucessfully moved from psotion %d to %d", n1, n2);
		    else
			plugin->print_server (plugin, "Trigger error: fail to move from %d to %d, number(s) seems invalid", n1, n2);
		}
		else
		    plugin->print_server (plugin, "Trigger error: fail to move from %d to %d, number(s) seems invalid", n1, n2);
	    }
	    else
		plugin->print_server (plugin, "Trigger error: fail to move, missing or too much arguments");
	    
	}
	else if (strcasecmp (argv[0], "remove") == 0)
	{
	    if (argc == 2)
	    {
		n1 = c_to_number (argv[1]);
		if (n1 > 0)
		{
		    if (weechat_trigger_remove (n1) == 0)
			plugin->print_server (plugin, "Trigger: trigger number %d sucessfully removed", n1);
		    else
			plugin->print_server (plugin, "Trigger error: fail to remove trigger number %d, number out of bound", n1);
		}
		else
		    plugin->print_server (plugin, "Trigger error: fail to remove trigger number %d, number seems invalid", n1);
	    }
	    else
		plugin->print_server (plugin, "Trigger error: fail to remove trigger, missing or too much arguments");
	}
	else
	    weechat_trigger_display (plugin);
	
	c_free_exploded_string (argv);
    }
    else
	weechat_trigger_display (plugin);
        
    return PLUGIN_RC_OK;
}

/*
 * weechat_trigger_match: find if a trigger match an irc message
 *         return : 0 if it does match
 *                  1 if it doesn't match
 *                  2 if irc message can't be parsed
 */
irc_msg *
weechat_trigger_match(t_weechat_trigger *trigger, char *msg, char *server, int *ret)
{
    irc_msg *imsg;
    int match_c, match_h, match_s;
    char **pp;
    
    imsg = irc_parse_msg (msg);
    *ret = 1;
    match_c = 0;
    match_h = 0;
    match_s = 0;
    
    if (imsg)
    {
	/*
	p->print_server(p, 
			"uh=[%s] n=[%s] u=[%s] h=[%s] com=[%s] chan=[%s] srv=[%s] msg=[%s]",
			imsg->userhost, imsg->nick, imsg->user, imsg->host,
			imsg->command, imsg->channel, server, imsg->message);
	*/
	
	
	if (
	    (imsg->user
	     && ((strcasecmp(trigger->domain, "*") == 0)
		 || (strcasecmp(trigger->domain, "user") == 0))
	     && c_imatch_string (imsg->user, trigger->pattern))
	    ||
	    (imsg->nick 
	     && ((strcasecmp(trigger->domain, "*") == 0)
		 || strcasecmp(trigger->domain, "nick") == 0)
	     && c_imatch_string (imsg->nick, trigger->pattern))
	    ||
	    (imsg->userhost
	     && ((strcasecmp(trigger->domain, "*") == 0)
		 || (strcasecmp(trigger->domain, "userhost") == 0))
	     && c_imatch_string (imsg->userhost, trigger->pattern))
	    ||
	    (imsg->message 
	     && ((strcasecmp(trigger->domain, "*") == 0)
		 || (strcasecmp(trigger->domain, "msg") == 0))
	     && c_imatch_string (imsg->message, trigger->pattern))
	    )
	{    
	    if (!imsg->command)
		match_c = 1;
	    else if (trigger->commands)
	    {
		for (pp = trigger->commands; *pp; pp++)
		{
		    if (c_imatch_string (imsg->command, *pp))
		    {
			match_c = 1;
			break;
		    }
		}
	    }
	    
	    if (match_c)
	    {
		if (!imsg->channel)
		    match_h = 1;
		else if (trigger->channels)
		{
		    for(pp = trigger->channels; *pp; pp++)
		    {
			if (c_imatch_string (imsg->channel, *pp))
			{
			    match_h = 1;
			    break;
			}
		    }
		}
	    }
	    
	    if (match_h)
	    {
		if (!server)
		    match_s = 1;
		else if (trigger->servers)
		{
		    for(pp = trigger->servers; *pp; pp++)
		    {
			if (c_imatch_string (server, *pp))
			{
			    match_s = 1;
			    break;
			}
		    }
		}
	    }
	}
	
	*ret = ((match_c == 1) &&  (match_h == 1) && (match_s == 1)) ? 0 : 1;
    }
    else
	*ret = 2;
    
    return imsg;
}

/*
 * weechat_trigger_msg: trigger handler
 */

int
weechat_trigger_msg (t_weechat_plugin *plugin,
                     int argc, char **argv,
                     char *handler_args, void *handler_pointer)
{
    t_weechat_trigger *l;
    int ret, i, j, match;
    char *ccom, *tcom;
    irc_msg *imsg;
    
    /* make C compiler happy */
    (void) argc;
    (void) handler_args;
    (void) handler_pointer;

    ret = PLUGIN_RC_OK;
    l = weechat_trigger_list;
    i = 0;

    while (l)
    {
	i++;
	/* argv[0] = server name */
        imsg = weechat_trigger_match (l, argv[2], argv[0], &match);
	
	if (imsg && match == 2)
	{
	    plugin->print_server (plugin, "Trigger error: error while parsing irc message : [%s]", argv[2]);
	    irc_msg_free (imsg);
	}
	else if (imsg && match == 0)
	{
	    if (strcasecmp (l->action, "display") == 0)
		plugin->print_server (plugin, "Trigger display: matching trigger number %d with message [%s]", i, argv[2]);
	    else if (strcasecmp (l->action, "ignore") == 0)
		ret = PLUGIN_RC_OK_IGNORE_WEECHAT;
	    else if (strcasecmp (l->action, "highlight") == 0)
		ret = PLUGIN_RC_OK_WITH_HIGHLIGHT;
	    else if (l->cmds && strcasecmp (l->action, "run") == 0)
	    {
		for(j = 0; l->cmds[j]; j++)
		{
		    /* formatting commands by replacing with defined values */
		    ccom = strdup (l->cmds[j]);
		    if (ccom)
		    {
			tcom = c_weechat_strreplace (ccom, "%uh", imsg->userhost == NULL ? "" : imsg->userhost);
			if (tcom)
			{
			    if (ccom) free (ccom);
			    ccom = tcom;
			}			

			tcom = c_weechat_strreplace (ccom, "%n", imsg->nick == NULL ? "" : imsg->nick);
			if (tcom)
			{
			    if (ccom) free (ccom);
			    ccom = tcom;
			}			

			tcom = c_weechat_strreplace (ccom, "%u", imsg->user == NULL ? "" : imsg->user);
			if (tcom)
			{
			    if (ccom) free (ccom);
			    ccom = tcom;
			}			

			tcom = c_weechat_strreplace (ccom, "%h", imsg->host == NULL ? "" : imsg->host);
			if (tcom)
			{
			    if (ccom) free (ccom);
			    ccom = tcom;
			}

			tcom = c_weechat_strreplace (ccom, "%c", imsg->command == NULL ? "" : imsg->command);
			if (tcom)
			{
			    if (ccom) free (ccom);
			    ccom = tcom;
			}

			tcom = c_weechat_strreplace (ccom, "%C", imsg->channel == NULL ? "" : imsg->channel);
			if (tcom)
			{
			    if (ccom) free (ccom);
			    ccom = tcom;
			}
			
			tcom = c_weechat_strreplace (ccom, "%S", argv[0] == NULL ? "" : argv[0]);
			if (tcom)
			{
			    if (ccom) free (ccom);
			    ccom = tcom;
			}
			
			tcom = c_weechat_strreplace (ccom, "%d", imsg->data == NULL ? "" : imsg->data);
			if (tcom)
			{
			    if (ccom) free (ccom);
			    ccom = tcom;
			}

			plugin->exec_command (plugin, NULL, NULL, ccom);
			free (ccom);
		    }
		    else
			plugin->exec_command (plugin, NULL, NULL, l->cmds[j]);
		}
	    }
	    irc_msg_free (imsg);
	}
	
	l = l->next_trigger;
    }
    
    return ret;
}

/*
 * weechat_trigger_tmsg: /tmsg command
 */

int
weechat_trigger_tmsg (t_weechat_plugin *plugin,
		      int cmd_argc, char **cmd_argv,
		      char *handler_args, void *handler_pointer)
{
    int argc;
    char **argv, *srv, *chan, *cmd;
    
    /* make C compiler happy */
    (void) cmd_argc;
    (void) handler_args;
    (void) handler_pointer;
    
    if (cmd_argv[2])
	argv = c_explode_string (cmd_argv[2], " ", 0, &argc);
    else
    {
        argv = NULL;
        argc = 0;
    }
    
    if (argv && argc>=3)
    {	
	srv = (strcmp (argv[0], "*") == 0) ? NULL : argv[0];
	chan = (strcmp (argv[1], "*") == 0) ? NULL : argv[1];
	
	cmd = c_join_string (&argv[2], " ");	
	plugin->exec_command (plugin, srv, chan, (cmd == NULL) ? "" : cmd);
	if (cmd)
	    c_free_joined_string (cmd);	
    }
    else
	plugin->print_server (plugin, "Trigger error: wrong argument count for command \"tmsg\"");
    
    if (argv)
	c_free_exploded_string (argv);

    return PLUGIN_RC_OK;
}



/*
 * weechat_trigger_edit: load/save triggers from/in a file
 */
int
weechat_trigger_edit (t_weechat_plugin *plugin, int todo)
{
    t_weechat_trigger *l;
    int len, ret, argc;
    char **argv, *commands, *channels, *servers, *cmds;
    char *weechat_dir, *triggerrc, line[1024], *p, *q;
    FILE *f;
    
    ret = 0;
    
    if ((todo != CONF_LOAD) && (todo != CONF_SAVE))
	return -1;
    
    weechat_dir = plugin->get_info (plugin, "weechat_dir", NULL);
    if (!weechat_dir)
	return -1;
    
    len = strlen (weechat_dir) + strlen(DIR_SEP) + strlen(CONF_FILE) + 1;
    triggerrc = (char *)malloc (len * sizeof(char));
    if (!triggerrc)
	return -1;

    snprintf (triggerrc, len, "%s%s%s", weechat_dir, DIR_SEP, CONF_FILE);
    
    /* saving */
    if (todo == CONF_SAVE)
    {
	f = fopen (triggerrc, "w");
	if (!f) {
	    plugin->print_server (plugin, "Trigger plugin: error, unable to write file '%s'", triggerrc);
	    return -1;
	}
    
	fprintf (f, "#\n");
	fprintf (f, "#WeeChat trigger plugin config file\n");
	fprintf (f, "#  BE CAREFUL - DO NOT EDIT BY HAND\n");
	fprintf (f, "#\n\n");
	
	l = weechat_trigger_list;
	while (l)
	{
	    commands = c_join_string (l->commands, ",");
	    channels = c_join_string (l->channels, ",");
	    servers = c_join_string (l->servers, ",");
	    cmds = c_join_string (l->cmds, ";");
	    
	    if (strcasecmp (l->action, "run") == 0)
		fprintf (f, "%s %s %s %s %s %s %s\n", 
			 l->pattern, l->domain, commands,
			 channels, servers, l->action, cmds);
	    else
		fprintf (f, "%s %s %s %s %s %s\n", 
			 l->pattern, l->domain, commands,
			 channels, servers, l->action);
	    
	    ret++;
	    l = l->next_trigger;
	}
	fclose(f);
    }
    
    /* loading */
    if (todo == CONF_LOAD)
    {
	f = fopen (triggerrc, "r");
	if (!f) {
	    plugin->print_server (plugin, "Trigger plugin: error, unable to read file '%s'", triggerrc);
	    return -1;
	}
	
	while (!feof (f))
	{
	    p = fgets (line, sizeof (line), f);
	    if (p)
	    {
		/* skip spaces ans tabs */
		while(p[0] == ' ' || p[0] == '\t')
		    p++;
		/* skip comments and empty lines */
		if ((p[0] != '#') && (p[0] != '\r') && (p[0] != '\n'))
		{
		    q = strrchr (p, '\n');
		    if (q)
			*q = '\0';
		    argv = c_explode_string (p, " ", 0, &argc);
		    if (argv && argc >= 7)
		    {
			if (weechat_trigger_add (argc, argv) == 0)
			    ret++;
			else
			    plugin->print_server (plugin, "Trigger: failed to load trigger \"%s\"", p);
		    }
		    if (argv)
			c_free_exploded_string (argv);
		}
	    }
	}
	fclose(f);
    }
	
    return ret;
}

/*
 * weechat_plugin_init: init trigger plugin
 */

int
weechat_plugin_init (t_weechat_plugin *plugin)
{
    t_plugin_handler *cmd_handler_trigger, *cmd_handler_tmsg, *msg_handler;
    int n;

    weechat_trigger_list = NULL;
    weechat_trigger_last = NULL;
    
    /* loading saved triggers */
    n = weechat_trigger_edit (plugin, CONF_LOAD);
    switch (n)
    {
    case -1:
	plugin->print_server (plugin, "Trigger plugin starting: error");
	break;
    case 0:
	plugin->print_server (plugin, "Trigger plugin starting: no triggers found");
	break;
    default:
	plugin->print_server (plugin, "Trigger plugin starting: %d triggers found and loaded", n);
    }
    
    /* add trigger command handler */
    cmd_handler_trigger = plugin->cmd_handler_add (plugin, "trigger",
						   "Trigger actions on IRC messages by nicks/hosts, irc messages, commands, channels and servers",
						   " [ list ] | [ add pattern domain [type(s) | command(s)] channel(s) server(s) [action [cmd]] ] | [ move from_num to_num ] | [ remove num ]",
						   "  'list': list triggers\n"
						   "   'add': create a new trigger\n"
						   "        pattern: pattern to match\n"
						   "         domain: domain where the pattern is searched (user, nick, userhost, msg)\n"
						   "        type(s): messages types to trigger (privmsg, ctcp-$type, dcc, join, part, quit, ...).\n"
						   "     command(s): irc commands to trigger.\n"
						   "     channel(s): channels to trigger.\n"
						   "      server(s): servers to trigger.\n"
						   "         action: action to perform if trigger match (ignore, display, highlight, run)\n"
						   "            cmd: irc or WeeChat command(s) to run if action is 'run'\n"
						   "                 possible replacements in command(s) :\n"
						   "                    %uh : userhost mask\n"
						   "                     %n : nickname\n"
						   "                     %u : username\n"
						   "                     %h : hostname\n"
						   "                     %c : irc command\n"
						   "                     %C : channel name\n"
						   "                     %S : server name\n"
						   "                     %d : extra data\n"
						   "  'move': move trigger position in trigger's list\n"
						   "       from_num: current trigger position\n"
						   "         to_num: future trigger position\n"
						   "'remove': remove a trigger\n"
						   "            num: position of the trigger to remove\n"
						   "\n"
						   "Multiples values separated by commas can be set for fields : type(s), command(s), channel(s) and server(s).\n"
						   "It's possible to use wildcards for fields : pattern, type(s), command(s), channel(s) and server(s) options.\n\n",
						   "list|add|move|remove *|%n *|user|nick|userhost|msg *|%I|ctcp-action|ctcp-dcc|ctcp-sed|ctcp-finger|ctcp-version|ctcp-source|ctcp-userinfo|ctcp-clientinfo|ctcp-errmsg|ctcp-ping|ctcp-time *|%c *|%s ignore|display|highlight|run",
					       &weechat_trigger_cmd,
					       NULL, NULL);
    
    /* add messge modifier */
    msg_handler = plugin->msg_handler_add (plugin, "*",
					   &weechat_trigger_msg,
					   NULL, NULL);
    
    
    /* add tmsg command handler */
    cmd_handler_tmsg = plugin->cmd_handler_add (plugin, "tmsg",
						"Send a message to a channel",
						" server receiver text",
						"server: server ('*' = current server)\n"
						"channel: channel ('*' = current channel)\n"
						"text: text to send\n",
						"*|%s *|%c",
						&weechat_trigger_tmsg,
						NULL, NULL);    
    
    return PLUGIN_RC_OK;
}

/*
 * weechat_plugin_end: end trigger plugin
 */

void
weechat_plugin_end (t_weechat_plugin *plugin)
{
    t_weechat_trigger *l;
    int n;
    
    /* make C compiler happy */
    (void) plugin;
    
    n = weechat_trigger_edit (plugin, CONF_SAVE);
    switch (n)
    {
    case -1:
	plugin->print_server (plugin, "Trigger plugin ending: error");
	break;
    case 0:
	plugin->print_server (plugin, "Trigger plugin ending: no triggers to save");
	break;
    default:
	plugin->print_server (plugin, "Trigger plugin ending: saving %d triggers", n);
    }
    
    while (weechat_trigger_list)
    {
	l = weechat_trigger_list;
	weechat_trigger_list = weechat_trigger_list->next_trigger;
	weechat_trigger_free (l);
    }
}
