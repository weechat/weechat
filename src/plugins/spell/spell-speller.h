/*
 * Copyright (C) 2006 Emmanuel Bouthenot <kolter@openics.org>
 * Copyright (C) 2006-2024 Sébastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef WEECHAT_PLUGIN_SPELL_SPELLER_H
#define WEECHAT_PLUGIN_SPELL_SPELLER_H

struct t_spell_speller_buffer
{
#ifdef USE_ENCHANT
    EnchantDict **spellers;                /* enchant spellers for buffer   */
#else
    AspellSpeller **spellers;              /* aspell spellers for buffer    */
#endif /* USE_ENCHANT */
    char *modifier_string;                 /* last modifier string          */
    int input_pos;                         /* position of cursor in input   */
    char *modifier_result;                 /* last modifier result          */
};

extern struct t_hashtable *spell_spellers;
extern struct t_hashtable *spell_speller_buffer;

extern int spell_speller_dict_supported (const char *lang);
extern void spell_speller_check_dictionaries (const char *dict_list);
#ifdef USE_ENCHANT
extern EnchantDict *spell_speller_new (const char *lang);
#else
extern AspellSpeller *spell_speller_new (const char *lang);
#endif /* USE_ENCHANT */
extern void spell_speller_remove_unused ();
extern struct t_spell_speller_buffer *spell_speller_buffer_new (struct t_gui_buffer *buffer);
extern int spell_speller_init ();
extern void spell_speller_end ();

#endif /* WEECHAT_PLUGIN_SPELL_SPELLER_H */
