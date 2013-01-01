/*
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WEECHAT_ASPELL_SPELLER_H
#define __WEECHAT_ASPELL_SPELLER_H 1

struct t_aspell_speller
{
    AspellSpeller *speller;                /* aspell speller                */
    char *lang;                            /* language                      */

    struct t_aspell_speller *prev_speller; /* pointer to next speller       */
    struct t_aspell_speller *next_speller; /* pointer to previous speller   */
};

extern struct t_aspell_speller *weechat_aspell_spellers;

extern int weechat_aspell_speller_exists (const char *lang);
extern struct t_aspell_speller *weechat_aspell_speller_search (const char *lang);
extern void weechat_aspell_speller_check_dictionaries (const char *dict_list);
extern struct t_aspell_speller *weechat_aspell_speller_new (const char *lang);
extern void weechat_aspell_speller_free (struct t_aspell_speller *speller);
extern void weechat_aspell_speller_free_all ();

#endif /* __WEECHAT_ASPELL_SPELLER_H */
