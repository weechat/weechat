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

struct t_aspell_speller_buffer
{
    AspellSpeller **spellers;              /* pointer to spellers for buf.  */
    char *modifier_string;                 /* last modifier string          */
    int input_pos;                         /* position of cursor in input   */
    char *modifier_result;                 /* last modifier result          */
};

extern struct t_hashtable *weechat_aspell_spellers;
extern struct t_hashtable *weechat_aspell_speller_buffer;

extern int weechat_aspell_speller_dict_supported (const char *lang);
extern void weechat_aspell_speller_check_dictionaries (const char *dict_list);
extern AspellSpeller *weechat_aspell_speller_new (const char *lang);
extern void weechat_aspell_speller_remove_unused ();
extern struct t_aspell_speller_buffer *weechat_aspell_speller_buffer_new (struct t_gui_buffer *buffer);
extern int weechat_aspell_speller_init ();
extern void weechat_aspell_speller_end ();

#endif /* __WEECHAT_ASPELL_SPELLER_H */
