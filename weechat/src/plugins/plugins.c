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

/* plugins.c: manages WeeChat plugins (Perl and/or Python and/or Ruby) */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include "plugins.h"

#ifdef PLUGIN_PERL
#include "perl/wee-perl.h"
#endif


/*
 * plugins_init: initialize all plugins
 */

void
plugins_init ()
{
    #ifdef PLUGIN_PERL
    wee_perl_init();
    #endif
}

/*
 * plugins_load: load a plugin
 */

void
plugins_load (int plugin_type, char *filename)
{
    switch (plugin_type)
    {
        case PLUGIN_PERL:
            #ifdef PLUGIN_PERL
            wee_perl_load (filename);
            #endif
            break;
        case PLUGIN_PYTHON:
            /* TODO: load Python script */
            break;
        case PLUGIN_RUBY:
            /* TODO: load Ruby script */
            break;
    }
}

/*
 * plugins_unload: unload a plugin
 */

void
plugins_unload (int plugin_type, char *scriptname)
{
    switch (plugin_type)
    {
        case PLUGIN_PERL:
            #ifdef PLUGIN_PERL
            wee_perl_unload (wee_perl_search (scriptname));
            #endif
            break;
        case PLUGIN_PYTHON:
            /* TODO: load Python script */
            break;
        case PLUGIN_RUBY:
            /* TODO: load Ruby script */
            break;
    }
}

/*
 * plugins_end: shutdown plugin interface
 */

void
plugins_end ()
{
    #ifdef PLUGIN_PERL
    wee_perl_end();
    #endif
}
