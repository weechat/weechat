/*
 * SPDX-FileCopyrightText: 2006-2007 Emmanuel Bouthenot <kolter@openics.org>
 * SPDX-FileCopyrightText: 2006-2026 Sébastien Helleu <flashcode@flashtux.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef WEECHAT_PLUGIN_LUA_API_H
#define WEECHAT_PLUGIN_LUA_API_H

extern struct luaL_Reg weechat_lua_api_funcs[];

extern int weechat_lua_api_buffer_input_data_cb (const void *pointer,
                                                 void *data,
                                                 struct t_gui_buffer *buffer,
                                                 const char *input_data);
extern int weechat_lua_api_buffer_close_cb (const void *pointer,
                                            void *data,
                                            struct t_gui_buffer *buffer);

#endif /* WEECHAT_PLUGIN_LUA_API_H */
