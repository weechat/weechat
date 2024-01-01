/*
 * Copyright (C) 2003-2024 Sébastien Helleu <flashcode@flashtux.org>
 * Copyright (C) 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
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

#ifndef WEECHAT_PLUGIN_RUBY_API_H
#define WEECHAT_PLUGIN_RUBY_API_H

/* required for Ruby < 2.3 */
#ifndef RB_FIXNUM_P
#define RB_FIXNUM_P(f) (((int)(SIGNED_VALUE)(f))&RUBY_FIXNUM_FLAG)
#endif

/* required for Ruby < 2.4 */
#ifndef RB_INTEGER_TYPE_P
#define RB_INTEGER_TYPE_P(obj) (RB_FIXNUM_P(obj) || RB_TYPE_P(obj, T_BIGNUM))
#endif

#define CHECK_INTEGER(obj)                      \
    if (!RB_INTEGER_TYPE_P(obj))                \
    {                                           \
        Check_Type(obj, T_BIGNUM);              \
    }

extern int weechat_ruby_api_buffer_input_data_cb (const void *pointer,
                                                  void *data,
                                                  struct t_gui_buffer *buffer,
                                                  const char *input_data);
extern int weechat_ruby_api_buffer_close_cb (const void *pointer,
                                             void *data,
                                             struct t_gui_buffer *buffer);
extern void weechat_ruby_api_init (VALUE ruby_mWeechat);

#endif /* WEECHAT_PLUGIN_RUBY_API_H */
