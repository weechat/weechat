/*
 * Copyright (c) 2003-2009 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef __WEECHAT_PYTHON_API_H
#define __WEECHAT_PYTHON_API_H 1

extern PyMethodDef weechat_python_funcs[];

extern int weechat_python_api_buffer_input_data_cb (void *data,
                                                    struct t_gui_buffer *buffer,
                                                    const char *input_data);
extern int weechat_python_api_buffer_close_cb (void *data,
                                               struct t_gui_buffer *buffer);

#endif /* weechat-python.h */
