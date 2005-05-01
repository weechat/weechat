/*
 * Copyright (c) 2003-2005 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_PYTHON_H
#define __WEECHAT_PYTHON_H 1

#include "../plugins.h"

extern void wee_python_init ();
extern t_plugin_script *wee_python_search (char *);
extern int wee_python_exec (char *, char *, char *);
extern int wee_python_load (char *);
extern void wee_python_unload (t_plugin_script *);
extern void wee_python_unload_all ();
extern void wee_python_end ();

#endif /* wee-python.h */
