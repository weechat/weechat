/*
 * SPDX-FileCopyrightText: 2003-2026 Sébastien Helleu <flashcode@flashtux.org>
 * SPDX-FileCopyrightText: 2005-2007 Emmanuel Bouthenot <kolter@openics.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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
