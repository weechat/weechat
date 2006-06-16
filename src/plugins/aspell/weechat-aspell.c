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

aspell_speller_t *aspell_plugin_speller = NULL;
aspell_config_t *aspell_plugin_config = NULL;
aspell_options_t aspell_plugin_options;

t_weechat_plugin *weechat_aspell_plugin = NULL;

/*
 * weechat_aspell_new_speller : create a new speller cell
 */
aspell_speller_t *
weechat_aspell_new_speller (void)
{
    aspell_speller_t *s;
    
    s = (aspell_speller_t *) malloc (sizeof (aspell_speller_t));    
    if (!s)
    {
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
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
 * weechat_aspell_free_speller : free a speller cell 
 */
void
weechat_aspell_free_speller (aspell_speller_t *s)
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
 * weechat_aspell_speller_list_search : search a speller cell
 */
aspell_speller_t *
weechat_aspell_speller_list_search (char *lang)
{
    aspell_speller_t *p, *r = NULL;
    
    for(p = aspell_plugin_speller; p; p = p->next_speller)
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
 * weechat_aspell_speller_list_add : create and add a new speller instance
 */
int 
weechat_aspell_speller_list_add (char *lang)
{
    aspell_speller_t *s;
    AspellConfig *config;
    AspellCanHaveError *ret;

    /* create a speller instance for the newly created cell */
    config = new_aspell_config();
    aspell_config_replace (config, "lang", lang);   
    
    ret = new_aspell_speller (config);

    if (aspell_error (ret) != 0)
    {
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		       "[%s] [ERROR] : %s", 
		       _PLUGIN_NAME, aspell_error_message (ret));
	delete_aspell_config (config);
	delete_aspell_can_have_error (ret);
	return 0;
    }

    /* create and add a new speller cell */
    s = weechat_aspell_new_speller();
    if (!s)
	return 0;
    
    s->next_speller = aspell_plugin_speller;
    if (aspell_plugin_speller)
	aspell_plugin_speller->prev_speller = s;
    aspell_plugin_speller = s;
    
    s->lang = strdup (lang);
    s->refs = 1;
    s->speller = to_aspell_speller (ret);
    
    delete_aspell_config (config);
    
    return 1;
}

/*
 * weechat_aspell_speller_list_remove : remove a speller instance
 */
int
weechat_aspell_speller_list_remove(char *lang)
{
    aspell_speller_t *p;
    int r = 0;

    if (!aspell_plugin_speller || !lang)
	return 0;
    
    if (!aspell_plugin_speller->prev_speller 
	&& !aspell_plugin_speller->next_speller)
    {
	weechat_aspell_free_speller (aspell_plugin_speller);
	aspell_plugin_speller = NULL;
	return 1;
    }

    for(p = aspell_plugin_speller; p; p = p->next_speller)
    {
	if (strcmp(p->lang, lang) == 0)
	{
	    if (p->prev_speller)
		p->prev_speller->next_speller = p->next_speller;
	    else
		aspell_plugin_speller = p->next_speller;

	    if (p->next_speller)
		p->next_speller->prev_speller = p->prev_speller;
	    
	    weechat_aspell_free_speller (p);
	    r = 1;
	    break;
	}
    }
    
    return r;
}

/*
 * weechat_aspell_new_config : create a new config cell
 */
aspell_config_t *
weechat_aspell_new_config (void)
{
    aspell_config_t *c;
    
    c = (aspell_config_t *) malloc (sizeof (aspell_config_t));    
    if (!c)
    {
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
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
 * weechat_aspell_free_config : free a config cell
 */
void 
weechat_aspell_free_config (aspell_config_t *c)
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
 * weechat_aspell_config_list_search : search a config cell
 */
aspell_config_t *
weechat_aspell_config_list_search (char *server, char *channel)
{
    aspell_config_t *p, *r = NULL;

    if (!server || !channel)
	return NULL;
    
    for(p = aspell_plugin_config; p; p = p->next_config)
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
 * weechat_aspell_config_list_add : create and add a new config
 */
int 
weechat_aspell_config_list_add (char *server, char *channel)
{
    aspell_config_t *c;
    
    c = weechat_aspell_new_config();
    if (!c)
	return 0;

    c->channel = strdup (channel);
    c->server = strdup (server);
    
    c->next_config = aspell_plugin_config;
    if (aspell_plugin_config)
	aspell_plugin_config->prev_config = c;
    aspell_plugin_config = c;
    
    return 1;
}

/*
 * weechat_aspell_config_list_remove : remove a speller config
 */
int
weechat_aspell_config_list_remove(char *server, char *channel)
{
    aspell_config_t *p;
    int r = 0;

    if (!aspell_plugin_config || !server || !channel)
	return 0;

    if (!aspell_plugin_config->prev_config 
	&& !aspell_plugin_config) 
    {
	weechat_aspell_free_config (aspell_plugin_config);
	aspell_plugin_config = NULL;
	return 1;
    }
    
    for(p = aspell_plugin_config; p; p = p->next_config)
    {	
	if (strcmp(p->server, server) == 0
	    && strcmp(p->channel, channel) == 0)
	{
	    if (p->prev_config)
		p->prev_config->next_config = p->next_config;
	    else
		aspell_plugin_config = p->next_config;

	    if (p->next_config)
		p->next_config->prev_config = p->prev_config;
	    weechat_aspell_free_config (p);
	    r = 1;
	    break;
	}
    }

    return r;
}

/*
 * weechat_aspell_iso_to_lang : 
 *               convert an aspell iso lang code
 *               in its english full name
 *
 */
char *
weechat_aspell_iso_to_lang (char *code)
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
 * weechat_aspell_iso_to_country : 
 *                  convert an aspell iso country
 *                  code in its english full name
 */
char *
weechat_aspell_iso_to_country (char *code)
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
 * weechat_aspell_speller_exists : 
 *                  return 1 if an aspell dict exists 
 *                  for a lang, 0 otherwise
 */
int
weechat_aspell_speller_exists (char *lang) 
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
 * weechat_aspell_speller_list_dicts : 
 *      list all aspell dict installed on system and display them
 */
void
weechat_aspell_speller_list_dicts (void) 
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
    
    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, "[%s] *** dictionnaries list :", _PLUGIN_NAME);

    while (( di = aspell_dict_info_enumeration_next (el)))
    {
	country = NULL;
	p = strchr (di->code, '_');
		
	if (p)
	{
	    *p = '\0';
	    lang = weechat_aspell_iso_to_lang ((char*) di->code);
	    *p = '_';
	    country = weechat_aspell_iso_to_country (p+1); 
	}
	else
	    lang = weechat_aspell_iso_to_lang ((char*) di->code);

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
	
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, "[%s]  - %s", _PLUGIN_NAME, buffer);
	
	if (lang)
	    free (lang);
	if (country)
	    free (country);
    }

    delete_aspell_dict_info_enumeration (el);    
    delete_aspell_config (config);
}

/*
 * weechat_aspell_config_show : display plugin settings
 */
void
weechat_aspell_config_show (void) 
{
    aspell_config_t *p;
    
    if (!aspell_plugin_config)
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		       "[%s] [SHOW] *** No buffers with spellchecking enable",
		       _PLUGIN_NAME);
    else
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		       "[%s] [SHOW] *** Spellchecking is active on the following buffers :",
		       _PLUGIN_NAME);
	
    for(p = aspell_plugin_config; p; p = p->next_config)
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		       "[%s] [SHOW]    -> %s@%s with lang '%s'",
		       _PLUGIN_NAME, p->channel, p->server, p->speller->lang);

    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		   "[%s] [SHOW] *** plugin options :", _PLUGIN_NAME);
    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		   "[%s] [SHOW]     -> word-size = %d", 
		   _PLUGIN_NAME, aspell_plugin_options.word_size);
    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		   "[%s] [SHOW]     ->     color = %s",
		   _PLUGIN_NAME, aspell_plugin_options.color_name);
    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		   aspell_plugin_options.check_sync == 1
		   ? "[%s] [SHOW]     -> realtime spellchecking is enable"
		   : "[%s] [SHOW]     -> asynchronous spellchecking is enable"
		   , 
		   _PLUGIN_NAME);
}

/*
 * weechat_aspell_config_addword : adding a word in personnal dictionaries
 */
int
weechat_aspell_config_addword(char *word)
{
    char *server, *channel;
    aspell_config_t *c;    
    int ret;
    
    ret = 0;
    channel = weechat_aspell_plugin->get_info (weechat_aspell_plugin, "channel", NULL);
    server = weechat_aspell_plugin->get_info (weechat_aspell_plugin, "server", NULL); 
    
    if (!server || !channel)
	return 0;

    c = weechat_aspell_config_list_search (server, channel);
    if (c) {
	if (aspell_speller_add_to_personal (
		c->speller->speller, (const char *) word, strlen(word)) == 1)
	    ret = 1;
    }
    
    if (ret)
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		       "[%s] [ADD-WORD] word '%s' successfully added in your personnal dictionnary",
		       _PLUGIN_NAME, word);
    else
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		       "[%s] [ADD-WORD] an error occured while adding word '%s' in your personnal dict",
		       _PLUGIN_NAME, word);
    
    if (server)
	free (server);
    if (channel)
	free (channel);
	
    return ret;
}

/*
 * weechat_aspell_config_dump : display debug infos
 */
void
weechat_aspell_config_dump (void) 
{
    aspell_config_t *p;
    aspell_speller_t *s;
    
    if (!aspell_plugin_config)
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		       "[%s] [DEBUG] [CONFIG] no config",
		       _PLUGIN_NAME);

    for(p = aspell_plugin_config; p; p = p->next_config)
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		       "[%s] [DEBUG] [CONFIG] @%p server='%s' channel='%s' @speller=%p lang='%s' @p=%p @n=%p",
		       _PLUGIN_NAME, p, p->server, p->channel, p->speller, p->speller->lang, p->prev_config, p->next_config);

    if (!aspell_plugin_speller)
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		       "[%s] [DEBUG] [SPELLER] no speller",
		       _PLUGIN_NAME);
    
    for(s = aspell_plugin_speller; s; s = s->next_speller)
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		       "[%s] [DEBUG] [SPELLER] @%p lang='%s' refs=%d @engine=%p @p=%p @n=%p",
		       _PLUGIN_NAME, s, s->lang, s->refs, s->speller, s->prev_speller, s->next_speller);
}

/*
 * weechat_aspell_config_enable_for : internal subroutine
 */
void
weechat_aspell_config_enable_for (char *server, char *channel, char *lang)
{
    aspell_config_t *c;
    aspell_speller_t *s;
    
    if (!weechat_aspell_speller_exists (lang))
    {
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		       "[%s] [WARN] '%s' dictionary doesn't seems to be available on your system", 
		       _PLUGIN_NAME, lang);
       	return;
    }
    
    c = weechat_aspell_config_list_search (server, channel);
    if (c)
    {
	c->speller->refs--;
	if (c->speller->refs == 0)
	    weechat_aspell_speller_list_remove (c->speller->lang);
	weechat_aspell_config_list_remove (server, channel);
    }

    if (!weechat_aspell_config_list_add (server, channel))
    {
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		       "[%s] [ERROR] enabling spell checking on %s@%s failed", 
		       _PLUGIN_NAME, channel, server);
	return;
    }

    s = weechat_aspell_speller_list_search (lang);
    
    if (!s) {
	weechat_aspell_speller_list_add (lang);
	s = aspell_plugin_speller;
    }
    else
	s->refs++;

    aspell_plugin_config->speller = s;
    
}

/*
 * weechat_aspell_config_enable : 
 *        enabling given lang spell checking on current server/channel
 */
void
weechat_aspell_config_enable (char *lang) 
{
    char *channel, *server;
    
    channel = weechat_aspell_plugin->get_info (weechat_aspell_plugin, "channel", NULL);
    server = weechat_aspell_plugin->get_info (weechat_aspell_plugin, "server", NULL);
        
    if (!server || !channel)
    {
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		       "[%s] [WARN] you are not in a channel", 
		       _PLUGIN_NAME);
	return;
    }
    
    weechat_aspell_config_enable_for (server, channel, lang);

    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		   "[%s] spell checking '%s' is now active on %s@%s",
		   _PLUGIN_NAME, lang, channel, server);
    
    if (channel)
	free (channel);
    if (server)
	free (server);
}

/*
 * weechat_aspell_config_disable : 
 *        disabling spell checking on current server/channel
 */
void
weechat_aspell_config_disable (void) 
{
    aspell_config_t *c;
    char *channel, *server;
    
    channel = weechat_aspell_plugin->get_info (weechat_aspell_plugin, "channel", NULL);
    server = weechat_aspell_plugin->get_info (weechat_aspell_plugin, "server", NULL);

    if (!server || !channel)
    {
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		       "[%s] [WARN] you are not in a channel", 
		       _PLUGIN_NAME, NULL, NULL);
	return;
    }
    
    c = weechat_aspell_config_list_search (server, channel);
    if (!c)
    {
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
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
	weechat_aspell_speller_list_remove (c->speller->lang);
    
    weechat_aspell_config_list_remove (server, channel);
    
    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		   "[%s] spell checking is now inactive on %s@%s",
		   _PLUGIN_NAME, channel, server);
    
    if (channel)
	free (channel);
    if (server)
	free (server);
}

/*
 * weechat_aspell_config_set : setting options values
 */
int
weechat_aspell_config_set(char *option, char *value)
{
    int c;
    
    if (strcmp (option, "word-size") == 0)
    {
	aspell_plugin_options.word_size = atoi ((value == NULL) ? "" : value);
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
		       "[%s] [SET] setting %s = %d", 
		       _PLUGIN_NAME, option, aspell_plugin_options.word_size);
    }
    else if (strcmp (option, "toggle-check-mode") == 0)
    {
	aspell_plugin_options.check_sync = aspell_plugin_options.check_sync == 1 ? 0 : 1;
	weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL,
		       aspell_plugin_options.check_sync == 1
		       ? "[%s] [SET] spellchecking is now set in realtime mode"
		       : "[%s] [SET] spellchecking is now set in asynchronous mode",
                       _PLUGIN_NAME, option);
    }
    else if (strcmp (option, "color") == 0)
    {
	c = weechat_aspell_plugin->get_irc_color (weechat_aspell_plugin, (value == NULL) ? "" : value);
	if (c == -1)
	    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
			   "[%s] [SET] setting %s = %s failed : color '%s' is unknown", 
			   _PLUGIN_NAME, option, 
			   (value == NULL) ? "" : value,
			   (value == NULL) ? "" : value);
	else
	{
	    aspell_plugin_options.color = c;
	    if (aspell_plugin_options.color_name)
		free (aspell_plugin_options.color_name);
	    aspell_plugin_options.color_name = strdup (value);
	    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, 
			   "[%s] [SET] setting %s = %s",
			   _PLUGIN_NAME, option, aspell_plugin_options.color_name);
	}
    }
    else
	return 0;
    
    return 1;
}

/*
 * weechat_aspell_config_save : saving plugin config
 */
int 
weechat_aspell_config_save (void) 
{
    aspell_config_t *p, *q;
    char *servers, *channels, *option;
    int n;
    
    servers = NULL;

    weechat_aspell_plugin->set_plugin_config (weechat_aspell_plugin, "servers", ""); 
    
    for(p = aspell_plugin_config; p; p = p->next_config)
    {
	servers = weechat_aspell_plugin->get_plugin_config (weechat_aspell_plugin, "servers");
	
	if (!servers)
	    weechat_aspell_plugin->set_plugin_config (weechat_aspell_plugin, "servers", p->server);
	else if (strlen (servers) == 0) 
	{
	    weechat_aspell_plugin->set_plugin_config (weechat_aspell_plugin, "servers", p->server);
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
		weechat_aspell_plugin->set_plugin_config (weechat_aspell_plugin, "servers", servers);
	    }
	    free (servers);
	}
	
	channels = NULL;
	for(q = aspell_plugin_config; q; q = q->next_config)
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
		weechat_aspell_plugin->set_plugin_config (weechat_aspell_plugin, option, q->speller->lang);
		free (option);
	    }
	}
	
	if (channels)
	{
	    n = 10 + strlen (p->server);
	    option = (char *) malloc ( n * sizeof (char));
	    snprintf (option, n, "channels_%s", p->server);
	    weechat_aspell_plugin->set_plugin_config (weechat_aspell_plugin, option, channels);
	    free (option);
	    free (channels);
	}
    }
    
    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, "[%s] [SAVE] configuration saved", _PLUGIN_NAME);
    return 1;
}

/*
 * weechat_aspell_config_load : loading plugin config
 */
int 
weechat_aspell_config_load(void)
{
    char *servers, *channels, *lang;
    char *option_s,  *option_l;
    char **servers_list, **channels_list;
    int i, j, s, c, n;
    
    servers = weechat_aspell_plugin->get_plugin_config (weechat_aspell_plugin, "servers");
    if (!servers)
	return 0;
    
    servers_list = weechat_aspell_plugin->explode_string (weechat_aspell_plugin, servers, " ", 0, &s);
    
    if (servers_list)
    {
	for (i=0; i<s; i++)
	{
	    n = 10 + strlen (servers_list[i]);
	    option_s = (char *) malloc (n * sizeof (char));
	    snprintf (option_s, n, "channels_%s", servers_list[i]);
	    
	    channels = weechat_aspell_plugin->get_plugin_config (weechat_aspell_plugin, option_s);
	    if (channels)
	    {
		channels_list = weechat_aspell_plugin->explode_string (weechat_aspell_plugin, channels, " ", 0, &c);
		if (channels_list)
		{
		    for (j=0; j<c; j++)
		    {
			n = 7 + strlen (servers_list[i]) + strlen (channels_list[j]);
			option_l = (char *) malloc (n * sizeof (char));
			snprintf (option_l, n, "lang_%s_%s", servers_list[i], channels_list[j]);
			
			lang = weechat_aspell_plugin->get_plugin_config (weechat_aspell_plugin, option_l);
			if (lang)
			{
			    weechat_aspell_config_enable_for (servers_list[i], channels_list[j], lang);
			    free (lang);
			}
			free (option_l);
		    }
		    weechat_aspell_plugin->free_exploded_string (weechat_aspell_plugin, channels_list);
		}
		free (channels);
	    }
	    free (option_s);
	}

	weechat_aspell_plugin->free_exploded_string (weechat_aspell_plugin, servers_list);
    }

    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, "[%s] [LOAD] configuration loaded", _PLUGIN_NAME);
    return 1;
}

/*
 * weechat_aspell_options_save : saving plugin options
 */
int
weechat_aspell_options_save(void)
{
    char buf[8];
    
    snprintf (buf, sizeof(buf), "%d", aspell_plugin_options.word_size);
    weechat_aspell_plugin->set_plugin_config (weechat_aspell_plugin, "word-size", buf);
    
    snprintf (buf, sizeof(buf), "%d", aspell_plugin_options.check_sync);
    weechat_aspell_plugin->set_plugin_config (weechat_aspell_plugin, "check-sync", buf);
    
    weechat_aspell_plugin->set_plugin_config (weechat_aspell_plugin, "color", aspell_plugin_options.color_name);

    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, "[%s] [SAVE] options saved", _PLUGIN_NAME);
    return 1;
}

/*
 * weechat_aspell_options_load : loading plugin options
 */
int
weechat_aspell_options_load(void)
{
    char *buffer;
    int n;
    
    buffer = weechat_aspell_plugin->get_plugin_config (weechat_aspell_plugin, "word-size");
    if (buffer)
    {
	aspell_plugin_options.word_size = atoi (buffer);
	free (buffer);
    }
    else
	aspell_plugin_options.word_size = _PLUGIN_OPTION_WORD_SIZE;
    
    buffer = weechat_aspell_plugin->get_plugin_config (weechat_aspell_plugin, "check-sync");
    if (buffer)
    {	
	aspell_plugin_options.check_sync = atoi (buffer);
	if (aspell_plugin_options.check_sync != 0 && aspell_plugin_options.check_sync != 1)
	    aspell_plugin_options.check_sync = _PLUGIN_OPTION_CHECK_SYNC;
	free (buffer);
    }
    else
	aspell_plugin_options.check_sync = _PLUGIN_OPTION_CHECK_SYNC;
    
    
    buffer = weechat_aspell_plugin->get_plugin_config (weechat_aspell_plugin, "color");    
    if (buffer)
    {	
	n = weechat_aspell_plugin->get_irc_color (weechat_aspell_plugin, buffer);
	if (n == -1)
	{
	    aspell_plugin_options.color = weechat_aspell_plugin->get_irc_color (weechat_aspell_plugin, _PLUGIN_OPTION_COLOR);
	    aspell_plugin_options.color_name = strdup (_PLUGIN_OPTION_COLOR);
	}
	else 
	{
	    aspell_plugin_options.color = n;
	    aspell_plugin_options.color_name = strdup (buffer);
	}
	free (buffer);
    }
    else {
	aspell_plugin_options.color = weechat_aspell_plugin->get_irc_color (weechat_aspell_plugin, _PLUGIN_OPTION_COLOR);
	aspell_plugin_options.color_name = strdup (_PLUGIN_OPTION_COLOR);
    }
    
    weechat_aspell_plugin->print (weechat_aspell_plugin, NULL, NULL, "[%s] [LOAD] options loaded", _PLUGIN_NAME);
    return 1;
}

/*
 * weechat_aspell_speller_command : manage "/aspell" uses
 */
int
weechat_aspell_speller_command (t_weechat_plugin *p, 
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
	args = weechat_aspell_plugin->explode_string (weechat_aspell_plugin, argv[2], " ", 0, &c);
	if (args)
	{
	    if (c >= 1)
	    {
		if (strcmp (args[0], "dictlist") == 0)
		{ 
		    weechat_aspell_speller_list_dicts (); 
		    r = 1; 
		}
		else if (strcmp (args[0], "show") == 0)
		{ 
		    weechat_aspell_config_show (); 
		    r = 1; 
		}
		else if (strcmp (args[0], "save") == 0)
		{ 
		    weechat_aspell_config_save (); 
		    weechat_aspell_options_save(); 
		    r = 1; 
		}
		else if (strcmp (args[0], "dump") == 0)
		{ 
		    weechat_aspell_config_dump (); 
		    r = 1; 
		}
		else if (strcmp (args[0], "enable") == 0) 
		{
		    if (c >= 2) 
		    { 
			weechat_aspell_config_enable (args[1]); 
			r = 1; 
		    }
		}
		else if (strcmp (args[0], "disable") == 0)
		    weechat_aspell_config_disable ();
		else if (strcmp (args[0], "set") == 0) 
		{
		    if (c >= 2) 
		    { 
			r = weechat_aspell_config_set (args[1], args[2]); 
		    }
		}
		else if (strcmp (args[0], "add-word") == 0) 
		{
		    if (c >= 2) 
		    { 
			weechat_aspell_config_addword (args[1]); r = 1; 
		    }
		}

	    }
	    weechat_aspell_plugin->free_exploded_string (weechat_aspell_plugin, args);
	}
    }

    if (r == 0)
	weechat_aspell_plugin->exec_command (weechat_aspell_plugin, NULL, NULL, helpcmd);
    
    return PLUGIN_RC_OK;
}

/*
 * weechat_aspell_nick_in_server_channel : 
 *    check presence of a nick in a server/channel
 */
int
weechat_aspell_nick_in_channel (char *nick, char *server, char *channel)
{
    t_plugin_nick_info *nick_info, *ptr_nick;
    int ret;
    
    ret = 0;
    if (!nick || !server || ! channel)
	return ret;
    
    nick_info = weechat_aspell_plugin->get_nick_info (
	weechat_aspell_plugin, server, channel);
    if  (!nick_info)
	return ret;

    for(ptr_nick = nick_info; ptr_nick; ptr_nick = ptr_nick->next_nick)
    {
	if (strcmp (nick, ptr_nick->nick) == 0) {
	    ret = 1;
	    break;
	}
    }
    
    weechat_aspell_plugin->free_nick_info (weechat_aspell_plugin, nick_info);

    return ret;
}    

/*
 * weechat_aspell_clean_word : 
 *    strip punct chars at the begining and at the end of a word
 */
char *
weechat_aspell_clean_word (char *word, int *offset)
{
    int len;
    char *buffer, *w, *p;

    if (!word)
	return NULL;
    
    buffer = strdup (word);

    *offset = 0;    
    p = buffer;    
    while (p)
    {
	if (!ispunct(*p))
	    break;
	p++;
	(*offset)++;
    }

    p = buffer + strlen(buffer) - 1;
    while (p >= buffer)
    {
	if (!ispunct(*p))
	    break;
	p--;
    }

    len = p - buffer - *offset + 1;
    if (len <= 0)
    {
	free (buffer);
	return NULL;
    }
    
    w = (char *) malloc ((len+1) * sizeof(char));

    if (w) {
	memcpy (w, buffer + *offset, len);
	w[len] = '\0';
    }    
    
    free (buffer);

    return w;
}

/*
 * weechat_aspell_keyb_check : handler to check spelling on input line
 */
int
weechat_aspell_keyb_check (t_weechat_plugin *p, int argc, char **argv,
			       char *handler_args, void *handler_pointer)
{
    char *server, *channel;
    aspell_config_t *c;
    char *input, *ptr_input, *pos_space, *clword;
    int count, offset;
    
    /* make gcc happy */
    (void) p;
    (void) handler_args;
    (void) handler_pointer;

    channel = weechat_aspell_plugin->get_info (weechat_aspell_plugin, "channel", NULL);
    server = weechat_aspell_plugin->get_info (weechat_aspell_plugin, "server", NULL);

    if (!server || !channel)
	return PLUGIN_RC_OK;

    c = weechat_aspell_config_list_search (server, channel);
    if (!c)
	return PLUGIN_RC_OK;

    if (aspell_plugin_options.check_sync == 0 && argv[0] && argv[0][0])
    {
	if (argv[0][0] == '*' && !ispunct (argv[0][1]) && !isspace (argv[0][1]))
	    return PLUGIN_RC_OK;
    }

    if (!(argc == 3 && argv[1] && argv[2]))
	return PLUGIN_RC_OK;
    
    if (strcmp (argv[1], argv[2]) == 0)
	return PLUGIN_RC_OK;
    
    input = weechat_aspell_plugin->get_info (weechat_aspell_plugin, "input", NULL);
    if (!input)
	return PLUGIN_RC_OK;

    if (!input[0])
	return PLUGIN_RC_OK;
    
    if (input[0] == '/')
	return PLUGIN_RC_OK;
    
    count = 0;
    ptr_input = input;
    weechat_aspell_plugin->input_color (weechat_aspell_plugin, 0, 0, 0);
    while (ptr_input && ptr_input[0])
    {
	pos_space = strchr (ptr_input, ' ');
	if (pos_space)
	    pos_space[0] = '\0';
	
	clword = weechat_aspell_clean_word (ptr_input, &offset);
	if (clword)
	{
	    if ( (int) strlen (clword) >= aspell_plugin_options.word_size)
	    {
		if (!weechat_aspell_nick_in_channel (clword, server, channel))
		{
		    if (aspell_speller_check (c->speller->speller, clword, -1) != 1)
		    {
			if (count == 0)
			    weechat_aspell_plugin->input_color (weechat_aspell_plugin, 0, 0, 0);
			weechat_aspell_plugin->input_color (weechat_aspell_plugin, aspell_plugin_options.color,
							    ptr_input - input + offset, strlen (clword));
			count++;
		    }
		}
	    }
	    free (clword);
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
    weechat_aspell_plugin->input_color (weechat_aspell_plugin, -1, 0, 0);
    
    free (input);
    
    return PLUGIN_RC_OK;
}

/* 
 * weechat_plugin_init : init function, called when plugin is loaded
 */
int weechat_plugin_init (t_weechat_plugin *plugin)
{
    char help[1024];
    aspell_plugin_speller = NULL; 
    aspell_plugin_config = NULL; 
    weechat_aspell_plugin = plugin;
    
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
    
    weechat_aspell_plugin->cmd_handler_add (weechat_aspell_plugin, "aspell",
					    "Aspell Plugin configuration",
					    "{ show | save | dictlist | set [OPTION [VALUE]] | add-word WORD | enable LANG | disable | dump }",
					    help,
					    "show|dictlist|save|enable|disable|set|add-word|dump word-size|toggle-check-mode|color",
					    &weechat_aspell_speller_command, NULL, NULL);
    
    weechat_aspell_plugin->keyboard_handler_add (
	weechat_aspell_plugin, &weechat_aspell_keyb_check, NULL, NULL);
    
    weechat_aspell_options_load ();
    weechat_aspell_config_load ();

    return PLUGIN_RC_OK;
}

/* 
 * weechat_plugin_end : end function, called when plugin is unloaded
 */
void weechat_plugin_end (t_weechat_plugin *p)
{
    aspell_speller_t *s, *t;
    aspell_config_t *c, *d;
    
    /* make gcc happy */
    (void) p;
    
    weechat_aspell_options_save ();
    weechat_aspell_config_save ();

    /* freeing memory */
    
    /* options */
    if (aspell_plugin_options.color_name)
	free (aspell_plugin_options.color_name);

    /* spellers */
    s = aspell_plugin_speller;
    while ( s != NULL) 
    {
	t = s;
	s = s->next_speller;
	weechat_aspell_free_speller (t);
    }
    
    /* config */
    c = aspell_plugin_config;
    while ( c != NULL) 
    {
	d = c;
	c = c->next_config;
	weechat_aspell_free_config (c);
    }
}
