/*
 * SPDX-FileCopyrightText: 2006 Emmanuel Bouthenot <kolter@openics.org>
 * SPDX-FileCopyrightText: 2006-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
extern void spell_speller_remove_unused (void);
extern struct t_spell_speller_buffer *spell_speller_buffer_new (struct t_gui_buffer *buffer);
extern int spell_speller_init (void);
extern void spell_speller_end (void);

#endif /* WEECHAT_PLUGIN_SPELL_SPELLER_H */
