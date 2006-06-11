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

/* weechat-aspell.c: Aspell plugin support for WeeChat */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <ctype.h>

#include "../weechat-plugin.h"
#include "weechat-aspell.h"

speller_t *plugin_speller = NULL;
config_t *plugin_config = NULL;
options_t plugin_options;

t_weechat_plugin *plugin = NULL;

/*
 * new_speller : create a new speller cell
 */
speller_t *new_speller (void)
{
    speller_t *s;
    
    s = (speller_t *) malloc (sizeof (speller_t));    
    if (!s)
    {
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [ERROR] : unable to alloc memory.", _PLUGIN_NAME);
	return NULL;
    }
    
    s->speller= NULL;
    s->lang = NULL;
    s->refs = 0;
    s->prev_speller = NULL;
    s->next_speller = NULL;
    
    return s;
}

/*
 * free_speller : free a speller cell 
 */
void free_speller (speller_t *s)
{
    if (s)
    {
	if (s->speller) 
	{
	    aspell_speller_save_all_word_lists (s->speller);
	    delete_aspell_speller (s->speller);
	}
	if (s->lang)
	    free(s->lang);
	free(s);
    }
}

/*
 * speller_list_search : search a speller cell
 */
speller_t *speller_list_search (char *lang)
{
    speller_t *p, *r = NULL;
    
    for(p = plugin_speller; p; p = p->next_speller)
    {
	if (strcmp(p->lang, lang) == 0)
	{
	    r = p;
	    break;
	}
    }
    
    return r;
}

/*
 * speller_list_add : create and add a new speller instance
 */
int speller_list_add (char *lang)
{
    speller_t *s;
    AspellConfig *config;
    AspellCanHaveError *ret;

    /* create a speller instance for the newly created cell */
    config = new_aspell_config();
    aspell_config_replace (config, "lang", lang);   
    
    ret = new_aspell_speller (config);

    if (aspell_error (ret) != 0)
    {
	plugin->print (plugin, NULL, NULL,
		       "[%s] [ERROR] : %s", 
		       _PLUGIN_NAME, aspell_error_message (ret));
	delete_aspell_config (config);
	delete_aspell_can_have_error (ret);
	return 0;
    }

    /* create and add a new speller cell */
    s = new_speller();
    if (!s)
	return 0;
    
    s->next_speller = plugin_speller;
    if (plugin_speller)
	plugin_speller->prev_speller = s;
    plugin_speller = s;
    
    s->lang = strdup (lang);
    s->refs = 1;
    s->speller = to_aspell_speller (ret);
    
    delete_aspell_config (config);
    
    return 1;
}

/*
 * speller_list_remove : remove a speller instance
 */
int speller_list_remove(char *lang)
{
    speller_t *p;
    int r = 0;

    if (!plugin_speller || !lang)
	return 0;
    
    if (!plugin_speller->prev_speller 
	&& !plugin_speller->next_speller)
    {
	free_speller (plugin_speller);
	plugin_speller = NULL;
	return 1;
    }

    for(p = plugin_speller; p; p = p->next_speller)
    {
	if (strcmp(p->lang, lang) == 0)
	{
	    if (p->prev_speller)
		p->prev_speller->next_speller = p->next_speller;
	    else
		plugin_speller = p->next_speller;

	    if (p->next_speller)
		p->next_speller->prev_speller = p->prev_speller;
	    
	    free_speller (p);
	    r = 1;
	    break;
	}
    }
    
    return r;
}

/*
 * new_config : create a new config cell
 */
config_t *new_config (void)
{
    config_t *c;
    
    c = (config_t *) malloc (sizeof (config_t));    
    if (!c)
    {
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [ERROR] : unable to alloc memory.", _PLUGIN_NAME);
	return NULL;
    }

    c->server = NULL;
    c->channel = NULL;
    c->speller = NULL;
    c->prev_config = NULL;
    c->next_config = NULL;
    
    return c;
}

/*
 * free_config : free a config cell
 */
void free_config (config_t *c)
{
    if (c) 
    {
	if (c->server)
	    free(c->server);
	if (c->channel)
	    free(c->channel);
	
	free(c);
    }
}

/*
 * config_list_search : search a config cell
 */
config_t *config_list_search (char *server, char *channel)
{
    config_t *p, *r = NULL;

    if (!server || !channel)
	return NULL;
    
    for(p = plugin_config; p; p = p->next_config)
    {
	if (strcmp(p->server, server) == 0 
	    && strcmp(p->channel, channel) == 0)
	{
	    r = p;
	    break;
	}
    }
    
    return r;
}

/*
 * config_list_add : create and add a new config
 */
int config_list_add (char *server, char *channel)
{
    config_t *c;
    
    c = new_config();
    if (!c)
	return 0;

    c->channel = strdup (channel);
    c->server = strdup (server);
    
    c->next_config = plugin_config;
    if (plugin_config)
	plugin_config->prev_config = c;
    plugin_config = c;
    
    return 1;
}

/*
 * config_list_remove : remove a speller config
 */
int config_list_remove(char *server, char *channel)
{
    config_t *p;
    int r = 0;

    if (!plugin_config || !server || !channel)
	return 0;

    if (!plugin_config->prev_config 
	&& !plugin_config) 
    {
	free_config (plugin_config);
	plugin_config = NULL;
	return 1;
    }
    
    for(p = plugin_config; p; p = p->next_config)
    {	
	if (strcmp(p->server, server) == 0
	    && strcmp(p->channel, channel) == 0)
	{
	    if (p->prev_config)
		p->prev_config->next_config = p->next_config;
	    else
		plugin_config = p->next_config;

	    if (p->next_config)
		p->next_config->prev_config = p->prev_config;
	    free_config (p);
	    r = 1;
	    break;
	}
    }

    return r;
}

/*
 * iso_to_lang : convert an aspell iso lang code
 *               in its english full name
 *
 */
char *iso_to_lang (char *code)
{
    iso_langs_t *p;
    char *l;
    
    l = NULL;
    
    for(p = langs_avail; p->code; ++p)
    {
	if (strcmp(p->code, code) == 0)
	{
	    l = strdup(p->name);
	    break;
	}
    }

    if (!l)
	l = strdup ("Unknown");

    return l;
}


/*
 * iso_to_country : convert an aspell iso country
 *                  code in its english full name
 */
char *iso_to_country (char *code)
{
    iso_countries_t *p;
    char *c;

    c = NULL;
    
    for(p = countries_avail; p->code; ++p)
    {
	if (strcmp(p->code, code) == 0)
	{
	    c = strdup(p->name);
	    break;
	}
    }

    if (!c)
	c = strdup ("Unknown");

    return c;
}

/*
 * speller_exists : return 1 if an aspell dict exists 
 *                  for a lang, 0 otherwise
 */
int speller_exists (char *lang) 
{
    struct AspellConfig *config;
    AspellDictInfoList *l;
    AspellDictInfoEnumeration *el;
    const AspellDictInfo *di;
    int r;
    
    r = 0;
    config = new_aspell_config();
    l = get_aspell_dict_info_list (config);
    el = aspell_dict_info_list_elements (l);
    
    di = NULL;    
    while (( di = aspell_dict_info_enumeration_next (el)))
    {
	if (strcmp(di->name, lang) == 0)
	{
	    r = 1;
	    break;
	}
    }

    delete_aspell_dict_info_enumeration (el);    
    delete_aspell_config (config);

    return r;
}

/*
 * speller_list_dicts : list all aspell dict installed on system and display them
 */
void speller_list_dicts (void) 
{
    char *country, *lang, *p;
    char buffer[192];
    struct AspellConfig *config;
    AspellDictInfoList *l;
    AspellDictInfoEnumeration *el;
    const AspellDictInfo *di;    
    
    config = new_aspell_config();
    l = get_aspell_dict_info_list (config);
    el = aspell_dict_info_list_elements (l);    
    di = NULL;
    
    plugin->print (plugin, NULL, NULL, "[%s] *** dictionnaries list :", _PLUGIN_NAME);

    while (( di = aspell_dict_info_enumeration_next (el)))
    {
	country = NULL;
	p = strchr (di->code, '_');
		
	if (p)
	{
	    *p = '\0';
	    lang = iso_to_lang ((char*) di->code);
	    *p = '_';
	    country = iso_to_country (p+1); 
	}
	else
	    lang = iso_to_lang ((char*) di->code);

	if (strlen (di->jargon) == 0)
	{
	    if (p)
		snprintf (buffer, sizeof(buffer), "%-22s %s (%s)", di->name, lang, country);
	    else
		snprintf (buffer, sizeof(buffer), "%-22s %s", di->name, lang);
	}
	else
	{
	    if (p)
		snprintf (buffer, sizeof(buffer), "%-22s %s (%s - %s)", di->name, lang, country, di->jargon);
	    else
		snprintf (buffer, sizeof(buffer), "%-22s %s (%s)", di->name, lang, di->jargon);
	}
	
	plugin->print (plugin, NULL, NULL, "[%s]  - %s", _PLUGIN_NAME, buffer);
	
	if (lang)
	    free (lang);
	if (country)
	    free (country);
    }

    delete_aspell_dict_info_enumeration (el);    
    delete_aspell_config (config);
}

/*
 * config_show : display plugin settings
 */
void config_show (void) 
{
    config_t *p;
    
    if (!plugin_config)
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [SHOW] *** No buffers with spellchecking enable",
		       _PLUGIN_NAME);
    else
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [SHOW] *** Spellchecking is active on the following buffers :",
		       _PLUGIN_NAME);
	
    for(p = plugin_config; p; p = p->next_config)
	plugin->print (plugin, NULL, NULL,
		       "[%s] [SHOW]    -> %s@%s with lang '%s'",
		       _PLUGIN_NAME, p->channel, p->server, p->speller->lang);

    plugin->print (plugin, NULL, NULL,
		   "[%s] [SHOW] *** plugin options :", _PLUGIN_NAME);
    plugin->print (plugin, NULL, NULL,
		   "[%s] [SHOW]     -> word-size = %d", 
		   _PLUGIN_NAME, plugin_options.word_size);
    plugin->print (plugin, NULL, NULL,
		   "[%s] [SHOW]     ->     color = %s",
		   _PLUGIN_NAME, plugin_options.color_name);
    plugin->print (plugin, NULL, NULL,
		   plugin_options.check_sync == 1
		   ? "[%s] [SHOW]     -> realtime spellchecking is enable"
		   : "[%s] [SHOW]     -> asynchronous spellchecking is enable"
		   , 
		   _PLUGIN_NAME);
}

/*
 * config_addword : adding a word in personnal dictionaries
 */
int config_addword(char *word)
{
    char *server, *channel;
    config_t *c;    
    int ret;
    
    ret = 0;
    channel = plugin->get_info (plugin, "channel", NULL);
    server = plugin->get_info (plugin, "server", NULL); 
    
    if (!server || !channel)
	return 0;

    c = config_list_search (server, channel);
    if (c) {
	if (aspell_speller_add_to_personal (
		c->speller->speller, (const char *) word, strlen(word)) == 1)
	    ret = 1;
    }
    
    if (ret)
	plugin->print (plugin, NULL, NULL,
		       "[%s] [ADD-WORD] word '%s' successfully added in your personnal dictionnary",
		       _PLUGIN_NAME, word);
    else
	plugin->print (plugin, NULL, NULL,
		       "[%s] [ADD-WORD] an error occured while adding word '%s' in your personnal dict",
		       _PLUGIN_NAME, word);
    
    if (server)
	free (server);
    if (channel)
	free (channel);
	
    return ret;
}

/*
 * config_dump : display debug infos
 */
void config_dump (void) 
{
    config_t *p;
    speller_t *s;
    
    if (!plugin_config)
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [DEBUG] [CONFIG] no config",
		       _PLUGIN_NAME);

    for(p = plugin_config; p; p = p->next_config)
	plugin->print (plugin, NULL, NULL,
		       "[%s] [DEBUG] [CONFIG] @%p server='%s' channel='%s' @speller=%p lang='%s' @p=%p @n=%p",
		       _PLUGIN_NAME, p, p->server, p->channel, p->speller, p->speller->lang, p->prev_config, p->next_config);

    if (!plugin_speller)
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [DEBUG] [SPELLER] no speller",
		       _PLUGIN_NAME);
    
    for(s = plugin_speller; s; s = s->next_speller)
	plugin->print (plugin, NULL, NULL,
		       "[%s] [DEBUG] [SPELLER] @%p lang='%s' refs=%d @engine=%p @p=%p @n=%p",
		       _PLUGIN_NAME, s, s->lang, s->refs, s->speller, s->prev_speller, s->next_speller);
}

/*
 * config_enable_for : internal subroutine
 */
void config_enable_for (char *server, char *channel, char *lang)
{
    config_t *c;
    speller_t *s;
    
    if (!speller_exists (lang))
    {
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [WARN] '%s' dictionary doesn't seems to be available on your system", 
		       _PLUGIN_NAME, lang);
       	return;
    }
    
    c = config_list_search (server, channel);
    if (c)
    {
	c->speller->refs--;
	if (c->speller->refs == 0)
	    speller_list_remove (c->speller->lang);
	config_list_remove (server, channel);
    }

    if (!config_list_add (server, channel))
    {
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [ERROR] enabling spell checking on %s@%s failed", 
		       _PLUGIN_NAME, channel, server);
	return;
    }

    s = speller_list_search (lang);
    
    if (!s) {
	speller_list_add (lang);
	s = plugin_speller;
    }
    else
	s->refs++;

    plugin_config->speller = s;
    
}

/*
 * config_enable : enabling given lang spell checking on current server/channel
 */
void config_enable (char *lang) 
{
    char *channel, *server;
    
    channel = plugin->get_info (plugin, "channel", NULL);
    server = plugin->get_info (plugin, "server", NULL);
        
    if (!server || !channel)
    {
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [WARN] you are not in a channel", 
		       _PLUGIN_NAME);
	return;
    }
    
    config_enable_for (server, channel, lang);

    plugin->print (plugin, NULL, NULL, 
		   "[%s] spell checking '%s' is now active on %s@%s",
		   _PLUGIN_NAME, lang, channel, server);
    
    if (channel)
	free (channel);
    if (server)
	free (server);
}

/*
 * config_disable : disabling spell checking on current server/channel
 */
void config_disable (void) 
{
    config_t *c;
    char *channel, *server;
    
    channel = plugin->get_info (plugin, "channel", NULL);
    server = plugin->get_info (plugin, "server", NULL);

    if (!server || !channel)
    {
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [WARN] you are not in a channel", 
		       _PLUGIN_NAME, NULL, NULL);
	return;
    }
    
    c = config_list_search (server, channel);
    if (!c)
    {
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [WARN] spell checking is not active on %s@%s", 
		       _PLUGIN_NAME, channel, server);
	if (channel)
	    free (channel);
	if (server)
	    free (server);
	return;
    }
    
    c->speller->refs--;
    if (c->speller->refs == 0)
	speller_list_remove (c->speller->lang);
    
    config_list_remove (server, channel);
    
    plugin->print (plugin, NULL, NULL, 
		   "[%s] spell checking is now inactive on %s@%s",
		   _PLUGIN_NAME, channel, server);
    
    if (channel)
	free (channel);
    if (server)
	free (server);
}

/*
 * config_set : setting options values
 */
int config_set(char *option, char *value)
{
    int c;
    
    if (strcmp (option, "word-size") == 0)
    {
	plugin_options.word_size = atoi ((value == NULL) ? "" : value);
	plugin->print (plugin, NULL, NULL, 
		       "[%s] [SET] setting %s = %d", 
		       _PLUGIN_NAME, option, plugin_options.word_size);
    }
    else if (strcmp (option, "toggle-check-mode") == 0)
    {
	plugin_options.check_sync = plugin_options.check_sync == 1 ? 0 : 1;
	plugin->print (plugin, NULL, NULL,
		       plugin_options.check_sync == 1
		       ? "[%s] [SET] spellchecking is now set in realtime mode"
		       : "[%s] [SET] spellchecking is now set in asynchronous mode",
                       _PLUGIN_NAME, option);
    }
    else if (strcmp (option, "color") == 0)
    {
	c = plugin->get_irc_color (plugin, (value == NULL) ? "" : value);
	if (c == -1)
	    plugin->print (plugin, NULL, NULL, 
			   "[%s] [SET] setting %s = %s failed : color '%s' is unknown", 
			   _PLUGIN_NAME, option, 
			   (value == NULL) ? "" : value,
			   (value == NULL) ? "" : value);
	else
	{
	    plugin_options.color = c;
	    if (plugin_options.color_name)
		free (plugin_options.color_name);
	    plugin_options.color_name = strdup (value);
	    plugin->print (plugin, NULL, NULL, 
			   "[%s] [SET] setting %s = %s",
			   _PLUGIN_NAME, option, plugin_options.color_name);
	}
    }
    else
	return 0;
    
    return 1;
}

/*
 * config_save : saving plugin config
 */
int config_save (void) 
{
    config_t *p, *q;
    char *servers, *channels, *option;
    int n;
    
    servers = NULL;

    plugin->set_plugin_config (plugin, "servers", ""); 
    
    for(p = plugin_config; p; p = p->next_config)
    {
	servers = plugin->get_plugin_config (plugin, "servers");
	
	if (!servers)
	    plugin->set_plugin_config (plugin, "servers", p->server);
	else if (strlen (servers) == 0) 
	{
	    plugin->set_plugin_config (plugin, "servers", p->server);
	    free (servers);
	}
	else 
	{
	    if (!strstr (servers, p->server))
	    {
		n = strlen (servers) + strlen (p->server) + 2;
		servers = (char *) realloc (servers, n * sizeof (char));
		strcat (servers, " ");
		strcat (servers, p->server);
		plugin->set_plugin_config (plugin, "servers", servers);
	    }
	    free (servers);
	}
	
	channels = NULL;
	for(q = plugin_config; q; q = q->next_config)
	{
	    if (strcmp (p->server, q->server) == 0)
	    {
		if (!channels)
		    channels = strdup (q->channel);
		else
		{
		    if (!strstr (channels, q->channel))
		    {
			n = strlen (channels) + strlen (q->channel) + 2;
			channels = (char *) realloc (channels, n * sizeof (char));
			strcat (channels, " ");
			strcat (channels, q->channel);
		    }
		}
		
		n = 7 + strlen (p->server) + strlen (q->channel);
		option = (char *) malloc ( n * sizeof (char));
		snprintf (option, n, "lang_%s_%s", p->server, q->channel);
		plugin->set_plugin_config (plugin, option, q->speller->lang);
		free (option);
	    }
	}
	
	if (channels)
	{
	    n = 10 + strlen (p->server);
	    option = (char *) malloc ( n * sizeof (char));
	    snprintf (option, n, "channels_%s", p->server);
	    plugin->set_plugin_config (plugin, option, channels);
	    free (option);
	    free (channels);
	}
    }
    
    plugin->print (plugin, NULL, NULL, "[%s] [SAVE] configuration saved", _PLUGIN_NAME);
    return 1;
}

/*
 * config_load : loading plugin config
 */
int config_load(void)
{
    char *servers, *channels, *lang;
    char *option_s,  *option_l;
    char **servers_list, **channels_list;
    int i, j, s, c, n;
    
    servers = plugin->get_plugin_config (plugin, "servers");
    if (!servers)
	return 0;
    
    servers_list = plugin->explode_string (plugin, servers, " ", 0, &s);
    
    if (servers_list)
    {
	for (i=0; i<s; i++)
	{
	    n = 10 + strlen (servers_list[i]);
	    option_s = (char *) malloc (n * sizeof (char));
	    snprintf (option_s, n, "channels_%s", servers_list[i]);
	    
	    channels = plugin->get_plugin_config (plugin, option_s);
	    if (channels)
	    {
		channels_list = plugin->explode_string (plugin, channels, " ", 0, &c);
		if (channels_list)
		{
		    for (j=0; j<c; j++)
		    {
			n = 7 + strlen (servers_list[i]) + strlen (channels_list[j]);
			option_l = (char *) malloc (n * sizeof (char));
			snprintf (option_l, n, "lang_%s_%s", servers_list[i], channels_list[j]);
			
			lang = plugin->get_plugin_config (plugin, option_l);
			if (lang)
			{
			    config_enable_for (servers_list[i], channels_list[j], lang);
			    free (lang);
			}
			free (option_l);
		    }
		    plugin->free_exploded_string (plugin, channels_list);
		}
		free (channels);
	    }
	    free (option_s);
	}

	plugin->free_exploded_string (plugin, servers_list);
    }

    plugin->print (plugin, NULL, NULL, "[%s] [LOAD] configuration loaded", _PLUGIN_NAME);
    return 1;
}

/*
 * options_save : saving plugin options
 */
int options_save(void)
{
    char buf[8];
    
    snprintf (buf, sizeof(buf), "%d", plugin_options.word_size);
    plugin->set_plugin_config (plugin, "word-size", buf);
    
    snprintf (buf, sizeof(buf), "%d", plugin_options.check_sync);
    plugin->set_plugin_config (plugin, "check-sync", buf);
    
    plugin->set_plugin_config (plugin, "color", plugin_options.color_name);

    plugin->print (plugin, NULL, NULL, "[%s] [SAVE] options saved", _PLUGIN_NAME);
    return 1;
}

/*
 * options_load : loading plugin options
 */
int options_load(void)
{
    char *buffer;
    int n;
    
    buffer = plugin->get_plugin_config (plugin, "word-size");
    if (buffer)
    {
	plugin_options.word_size = atoi (buffer);
	free (buffer);
    }
    else
	plugin_options.word_size = _PLUGIN_OPTION_WORD_SIZE;
    
    buffer = plugin->get_plugin_config (plugin, "check-sync");
    if (buffer)
    {	
	plugin_options.check_sync = atoi (buffer);
	if (plugin_options.check_sync != 0 && plugin_options.check_sync != 1)
	    plugin_options.check_sync = _PLUGIN_OPTION_CHECK_SYNC;
	free (buffer);
    }
    else
	plugin_options.check_sync = _PLUGIN_OPTION_CHECK_SYNC;
    
    
    buffer = plugin->get_plugin_config (plugin, "color");    
    if (buffer)
    {	
	n = plugin->get_irc_color (plugin, buffer);
	if (n == -1)
	{
	    plugin_options.color = plugin->get_irc_color (plugin, _PLUGIN_OPTION_COLOR);
	    plugin_options.color_name = strdup (_PLUGIN_OPTION_COLOR);
	}
	else 
	{
	    plugin_options.color = n;
	    plugin_options.color_name = strdup (buffer);
	}
	free (buffer);
    }
    else {
	plugin_options.color = plugin->get_irc_color (plugin, _PLUGIN_OPTION_COLOR);
	plugin_options.color_name = strdup (_PLUGIN_OPTION_COLOR);
    }
    
    plugin->print (plugin, NULL, NULL, "[%s] [LOAD] options loaded", _PLUGIN_NAME);
    return 1;
}

/*
 * speller_command : manage "/aspell" uses
 */
int speller_command (t_weechat_plugin *p, 
		      int argc, char **argv, 
		      char *handler_args, 
		      void *handler_pointer)
{
    char helpcmd[32];
    char **args;
    int c, r;
    
    /* make gcc happy */
    (void) p;
    (void) handler_args;
    (void) handler_pointer;

    snprintf(helpcmd, sizeof(helpcmd), "/help %s", plugin_command);
    r = 0;

    if ((argc == 3) && argv[1] && argv[2])
    {
	args = plugin->explode_string (plugin, argv[2], " ", 0, &c);
	if (args)
	{
	    if (c >= 1)
	    {
		if (strcmp (args[0], "dictlist") == 0)
		{ 
		    speller_list_dicts (); r = 1; 
		}
		else if (strcmp (args[0], "show") == 0)
		{ 
		    config_show (); r = 1; 
		}
		else if (strcmp (args[0], "save") == 0)
		{ 
		    config_save (); options_save(); r = 1; 
		}
		else if (strcmp (args[0], "dump") == 0)
		{ 
		    config_dump (); r = 1; 
		}
		else if (strcmp (args[0], "enable") == 0) 
		{
		    if (c >= 2) { config_enable (args[1]); r = 1; }
		}
		else if (strcmp (args[0], "disable") == 0)
		    config_disable ();
		else if (strcmp (args[0], "set") == 0) 
		{
		    if (c >= 2) { r = config_set (args[1], args[2]); }
		}
		else if (strcmp (args[0], "add-word") == 0) 
		{
		    if (c >= 2) { config_addword (args[1]); r = 1; }
		}

	    }
	    plugin->free_exploded_string (plugin, args);
	}
    }

    if (r == 0)
	plugin->exec_command (plugin, NULL, NULL, helpcmd);
    
    return PLUGIN_RC_OK;
}

/*
 * keyb_check : handler to check spelling on input line
 */
int keyb_check (t_weechat_plugin *p, int argc, char **argv,
		char *handler_args, void *handler_pointer)
{
    char *server, *channel;
    config_t *c;
    char *input, *ptr_input, *pos_space;
    int count;
    
    /* make gcc happy */
    (void) p;
    (void) handler_args;
    (void) handler_pointer;

    channel = plugin->get_info (plugin, "channel", NULL);
    server = plugin->get_info (plugin, "server", NULL);

    if (!server || !channel)
	return PLUGIN_RC_OK;

    c = config_list_search (server, channel);
    if (!c)
	return PLUGIN_RC_OK;

    if (plugin_options.check_sync == 0 && argv[0] && argv[0][0])
    {
	/* FIXME : using isalpha(), can make problem with UTF-8 encodings */
	if (argv[0][0] == '*' && isalpha (argv[0][1]))
	    return PLUGIN_RC_OK;
    }

    if (!(argc == 3 && argv[1] && argv[2]))
	return PLUGIN_RC_OK;
    
    if (strcmp (argv[1], argv[2]) == 0)
	return PLUGIN_RC_OK;
    
    input = plugin->get_info (plugin, "input", NULL);
    if (!input)
	return PLUGIN_RC_OK;

    if (!input[0])
	return PLUGIN_RC_OK;
    
    if (input[0] == '/')
	return PLUGIN_RC_OK;
    
    count = 0;
    ptr_input = input;
    plugin->input_color (plugin, 0, 0, 0);
    while (ptr_input && ptr_input[0])
    {
	pos_space = strchr (ptr_input, ' ');
	if (pos_space)
	    pos_space[0] = '\0';
	
	if ( (int) strlen (ptr_input) >= plugin_options.word_size)
	{
	    if (aspell_speller_check (c->speller->speller, ptr_input, -1) != 1)
	    {
		if (count == 0)
		    plugin->input_color (plugin, 0, 0, 0);
		plugin->input_color (plugin, plugin_options.color,
				     ptr_input - input, strlen (ptr_input));
		count++;
	    }	
	}
	
	if (pos_space)
	{
	    pos_space[0] = ' ';
	    ptr_input = pos_space + 1;
	    while (ptr_input[0] == ' ')
		ptr_input++;
	}
	else
	    ptr_input = NULL;	
    }
    plugin->input_color (plugin, -1, 0, 0);
    
    free (input);
    
    return PLUGIN_RC_OK;
}

/* 
 * weechat_plugin_init : init function, called when plugin is loaded
 */
int weechat_plugin_init (t_weechat_plugin *p)
{
    char help[1024];
    plugin_speller = NULL; 
    plugin_config = NULL; 
    plugin = p;
    
    snprintf (help, sizeof(help),	      
	      "    show : show plugin configuration\n"
	      "dictlist : show installed dictionnaries\n"
	      "    save : save plugin configuration\n"			     
	      "  enable : enable aspell on current channel/server\n"
	      " disable : disable aspell on current channel/server\n"
	      "add-word : add a word in your personnal aspell dict\n"
	      "    dump : show plugins internals (usefull for debug)\n"
	      "     set : setting options\n"
	      "           OPTION := { word-size SIZE | toogle-check-mode  | color COLOR }\n"
	      "           word-size : minimum size for a word to be checked (default : %d)\n" 	      
	      "   toggle-check-mode : switch between a realtime or an asynchronous checking\n"
	      "               color : color of the mispelled words\n"
	      "\n"
	      " *NB : input line beginning with a '/' is not checked\n",
	      _PLUGIN_OPTION_WORD_SIZE);
    
    plugin->cmd_handler_add (plugin, "aspell",
                             "Aspell Plugin configuration",
			     "{ show | save | dictlist | set [OPTION [VALUE]] | add-word WORD | enable LANG | disable | dump }",
			     help,
                             "show|dictlist|save|enable|disable|set|add-word|dump word-size|toggle-check-mode|color",
                             &speller_command, NULL, NULL);
    
    plugin->keyboard_handler_add (plugin, &keyb_check, NULL, NULL);
    
    options_load ();
    config_load ();

    return PLUGIN_RC_OK;
}

/* 
 * weechat_plugin_end : end function, called when plugin is unloaded
 */
void weechat_plugin_end (t_weechat_plugin *p)
{
    speller_t *s, *t;
    config_t *c, *d;
    
    /* make gcc happy */
    (void) p;
    
    options_save ();
    config_save ();

    /* freeing memory */
    
    /* options */
    if (plugin_options.color_name)
	free (plugin_options.color_name);

    /* spellers */
    s = plugin_speller;
    while ( s != NULL) 
    {
	t = s;
	s = s->next_speller;
	free_speller (t);
    }
    
    /* config */
    c = plugin_config;
    while ( c != NULL) 
    {
	d = c;
	c = c->next_config;
	free_config (c);
    }
}
