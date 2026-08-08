/*
 * SPDX-FileCopyrightText: 2006 Emmanuel Bouthenot <kolter@openics.org>
 * SPDX-FileCopyrightText: 2006-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_SPELL_H
#define WEECHAT_PLUGIN_SPELL_H

#ifdef USE_ENCHANT
#include <enchant.h>
#else
#include <aspell.h>
#endif /* USE_ENCHANT */

#define weechat_plugin weechat_spell_plugin
#define SPELL_PLUGIN_NAME "spell"
#define SPELL_PLUGIN_PRIORITY 12000

struct t_spell_code
{
    char *code;
    char *name;
};

#ifdef USE_ENCHANT
extern EnchantBroker *spell_enchant_broker;
#endif /* USE_ENCHANT */

extern struct t_weechat_plugin *weechat_spell_plugin;
extern int spell_enabled;
extern struct t_spell_code spell_langs[];
extern struct t_spell_code spell_countries[];

extern char *spell_build_option_name (struct t_gui_buffer *buffer);
extern const char *spell_get_dict_with_buffer_name (const char *name);
extern const char *spell_get_dict (struct t_gui_buffer *buffer);

#endif /* WEECHAT_PLUGIN_SPELL_H */
