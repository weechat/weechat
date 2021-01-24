/*
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_PERL_API_H
#define WEECHAT_PLUGIN_PERL_API_H

extern int weechat_perl_api_buffer_input_data_cb (const void *pointer,
                                                  void *data,
                                                  struct t_gui_buffer *buffer,
                                                  const char *input_data);
extern int weechat_perl_api_buffer_close_cb (const void *pointer,
                                             void *data,
                                             struct t_gui_buffer *buffer);
extern void weechat_perl_api_init (pTHX);

#endif /* WEECHAT_PLUGIN_PERL_API_H */
