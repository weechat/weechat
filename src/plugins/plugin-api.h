/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_PLUGIN_API_H
#define WEECHAT_PLUGIN_PLUGIN_API_H

#include <time.h>

/* strings */
extern void plugin_api_charset_set (struct t_weechat_plugin *plugin,
                                    const char *charset);
extern const char *plugin_api_gettext (const char *string);
extern const char *plugin_api_ngettext (const char *single, const char *plural,
                                        int count);

/* crypto */
extern int plugin_api_crypto_hash (const void *data, int data_size,
                                   const char *hash_algo,
                                   void *hash, int *hash_size);
extern int plugin_api_crypto_hash_file (const char *filename,
                                        const char *hash_algo,
                                        void *hash, int *hash_size);
extern int plugin_api_crypto_hash_pbkdf2 (const void *data, int data_size,
                                          const char *hash_algo,
                                          const void *salt, int salt_size,
                                          int iterations,
                                          void *hash, int *hash_size);
extern int plugin_api_crypto_hmac (const void *key, int key_size,
                                   const void *message, int message_size,
                                   const char *hash_algo,
                                   void *hash, int *hash_size);

/* config */
extern void plugin_api_config_file_option_free (struct t_config_option *option);
extern struct t_config_option *plugin_api_config_get (const char *option_name);
extern const char *plugin_api_config_get_plugin (struct t_weechat_plugin *plugin,
                                           const char *option_name);
extern int plugin_api_config_is_set_plugin (struct t_weechat_plugin *plugin,
                                            const char *option_name);
extern int plugin_api_config_set_plugin (struct t_weechat_plugin *plugin,
                                         const char *option_name,
                                         const char *value);
extern void plugin_api_config_set_desc_plugin (struct t_weechat_plugin *plugin,
                                               const char *option_name,
                                               const char *description);
extern int plugin_api_config_unset_plugin (struct t_weechat_plugin *plugin,
                                           const char *option_name);

/* display */
extern const char *plugin_api_prefix (const char *prefix);
extern const char *plugin_api_color (const char *color_name);

/* command */
extern int plugin_api_command_options (struct t_weechat_plugin *plugin,
                                       struct t_gui_buffer *buffer,
                                       const char *command,
                                       struct t_hashtable *options);
extern int plugin_api_command (struct t_weechat_plugin *plugin,
                               struct t_gui_buffer *buffer,
                               const char *command);

/* infolist */
extern int plugin_api_infolist_next (struct t_infolist *infolist);
extern int plugin_api_infolist_prev (struct t_infolist *infolist);
extern void plugin_api_infolist_reset_item_cursor (struct t_infolist *infolist);
extern const char *plugin_api_infolist_fields (struct t_infolist *infolist);
extern int plugin_api_infolist_integer (struct t_infolist *infolist,
                                        const char *var);
extern const char *plugin_api_infolist_string (struct t_infolist *infolist,
                                               const char *var);
extern void *plugin_api_infolist_pointer (struct t_infolist *infolist,
                                          const char *var);
extern void *plugin_api_infolist_buffer (struct t_infolist *infolist,
                                         const char *var, int *size);
extern time_t plugin_api_infolist_time (struct t_infolist *infolist,
                                        const char *var);
extern void plugin_api_infolist_free (struct t_infolist *infolist);

extern void plugin_api_init ();

#endif /* WEECHAT_PLUGIN_PLUGIN_API_H */
