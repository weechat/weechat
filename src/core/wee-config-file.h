/*
 * Copyright (c) 2003-2007 by FlashCode <flashcode@flashtux.org>
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


#ifndef __WEECHAT_CONFIG_FILE_H
#define __WEECHAT_CONFIG_FILE_H 1

#include "wee-config-option.h"

typedef int (t_config_func_read_option) (struct t_config_option *, char *, char *);
typedef int (t_config_func_write_options) (FILE *, char *, struct t_config_option *);

extern int config_file_read_option (struct t_config_option *, char *, char *);
extern int config_file_read (char **, struct t_config_option **,
                             t_config_func_read_option **,
                             t_config_func_read_option *,
                             t_config_func_write_options **,
                             char *);

extern int config_file_write_options (FILE *, char *, struct t_config_option *);
extern int config_file_write_options_default_values (FILE *, char *,
                                                     struct t_config_option *);
extern int config_file_write_default (char **, struct t_config_option **,
                                      t_config_func_write_options **, char *);
extern int config_file_write (char **, struct t_config_option **,
                              t_config_func_write_options **, char *);

#endif /* wee-config-file.h */
