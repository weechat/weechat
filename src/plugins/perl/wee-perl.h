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


#ifndef __WEECHAT_PERL_H
#define __WEECHAT_PERL_H 1

typedef struct t_perl_script t_perl_script;

struct t_perl_script
{
    char *name;                     /* name of script                       */
    char *version;                  /* version of script                    */
    char *shutdown_func;            /* function when script ends            */
    char *description;              /* description of script                */
    t_perl_script *prev_script;     /* link to previous Perl script         */
    t_perl_script *next_script;     /* link to next Perl script             */
};

extern void wee_perl_init ();
extern t_perl_script *wee_perl_search (char *);
extern int wee_perl_exec (char *, char *);
extern int wee_perl_load (char *);
extern void wee_perl_unload (t_perl_script *);
extern void wee_perl_unload_all ();
extern void wee_perl_end ();

#endif /* wee-perl.h */
