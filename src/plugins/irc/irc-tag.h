/*
 * Copyright (C) 2021-2025 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_IRC_TAG_H
#define WEECHAT_PLUGIN_IRC_TAG_H

extern char *irc_tag_escape_value (const char *string);
extern char *irc_tag_unescape_value (const char *string);
extern char *irc_tag_modifier_cb (const void *pointer,
                                  void *data,
                                  const char *modifier,
                                  const char *modifier_data,
                                  const char *string);
extern int irc_tag_parse (const char *tags,
                          struct t_hashtable *hashtable,
                          const char *prefix_key);
extern char *irc_tag_add_tags_to_message (const char *message,
                                          struct t_hashtable *tags);

#endif /* WEECHAT_PLUGIN_IRC_TAG_H */
