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

int
weechat_trigger_match (t_weechat_trigger *ptr_ign, 
		      char *pattern, char *commands,
		      char *channels, char *servers)
{
    if (strcasecmp (ptr_ign->pattern, pattern) == 0
	&& strcasecmp (ptr_ign->commands, commands) == 0
	&& strcasecmp (ptr_ign->channels, channels) == 0
	&& strcasecmp (ptr_ign->servers, servers) == 0)
	return 1;
    
    return 0;
}

t_weechat_trigger *
weechat_trigger_search (char *pattern, char *commands,
		       char *channels, char *servers)
{
    t_weechat_trigger *l;
    
    l = weechat_trigger_list;
    while (l)
    {
	if (weechat_trigger_match (l, pattern, commands, channels, servers))
	    return l;
	l = l->next_trigger;
    }
    
    return NULL;
}
int
weechat_trigger_add (t_weechat_plugin *plugin,
		    char *pattern, char *commands,
		    char *channels, char *servers)
{
    t_weechat_trigger *new;

    if (!pattern)
	return 0;
    if (weechat_trigger_search (pattern, commands, channels, servers))
    {
	return 2;
    }
    
    new = weechat_trigger_alloc (pattern, commands, channels, servers);
    if (new) 
    {
	if (weechat_trigger_last)
	{
	    weechat_trigger_last->next_trigger = new;
	    new->prev_trigger = weechat_trigger_last;
	    weechat_trigger_last = new;
	}
	else
	{
	    weechat_trigger_list = new;
	    weechat_trigger_last = new;
	}
	
	return 1;
    }
    else
	plugin->print_server (plugin, "Unable to add trigger, memory allocation failed.");
    
    return 0;
}

int
weechat_trigger_list_del (char *pattern, char *commands,
			char *channels, char *servers)
{
    char *error;
    int number;
    t_weechat_trigger *p, *l;
        
    if (!pattern || !weechat_trigger_list)
	return 0;
    
    error = NULL;
    number = strtol (pattern, &error, 10);
    p = NULL;
    
    if (error && !error[0] && number > 0)
    {
	l = weechat_trigger_list;
	while (l && number >= 0)
	{
	    number--;
	    if (number == 0)
		p = l;
	    l = l->next_trigger;
	}
    }
    else
	p = weechat_trigger_search (pattern, commands, channels, servers);
    
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
	return 1;
    }
    
    return 0;
}

    /*
    server = plugin->get_info (plugin, "server", NULL);
    if (!server)
	server = strdup ("*");

    if (argc >= 4)
	ret = weechat_trigger_add (plugin, argv[0], argv[1], argv[2], argv[3]);
    else if (argc == 3)
	ret = weechat_trigger_add (plugin, argv[0], argv[1], argv[2], server);
    else if (argc == 2)
	ret = weechat_trigger_add (plugin, argv[0], argv[1], "*", server);
    else if (argc == 1)
	ret = weechat_trigger_add (plugin, argv[0], "*", "*", server);
    else
    {
	ret = weechat_trigger_list (plugin);
	ret = 1;
    }
    
    if (server)
	free (server);
    
    if (argc > 0 && ret == 1)
	plugin->print_server (plugin, "Trigger: new trigger: on %s/%s: trigger %s from %s",
			      weechat_trigger_last->servers, weechat_trigger_last->channels,
			      weechat_trigger_last->commands, weechat_trigger_last->pattern);
    else if (ret == 2)
	plugin->print_server (plugin, "Trigger: trigger already exists.");
    else if (ret == 0)
	plugin->print_server (plugin, "Trigger: error: unable to add trigger.");
    */
