/*
 * Copyright (C) 2003-2021 Sébastien Helleu <flashcode@flashtux.org>
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

#ifndef WEECHAT_PLUGIN_PYTHON_API_H
#define WEECHAT_PLUGIN_PYTHON_API_H

extern PyMethodDef weechat_python_funcs[];

extern int weechat_python_api_buffer_input_data_cb (const void *pointer,
                                                    void *data,
                                                    struct t_gui_buffer *buffer,
                                                    const char *input_data);
extern int weechat_python_api_buffer_close_cb (const void *pointer,
                                               void *data,
                                               struct t_gui_buffer *buffer);

#endif /* WEECHAT_PLUGIN_PYTHON_API_H */
