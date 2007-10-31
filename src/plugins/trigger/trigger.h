/*
 * Copyright (c) 2003-2006 by FlashCode <flashcode@flashtux.org>
 * See README for License detail, AUTHORS for developers list.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* weechat-trigger.h: Trigger plugin support for WeeChat */

#ifndef WEECHAT_TRIGGER__H
#define WEECHAT_TRIGGER__H 1

#define _PLUGIN_NAME    "trigger"
#define _PLUGIN_VERSION "0.1"
#define _PLUGIN_DESC    "Trigger plugin for WeeChat"

char plugin_name[]        = _PLUGIN_NAME;
char plugin_version[]     = _PLUGIN_VERSION;
char plugin_description[] = _PLUGIN_DESC;

typedef struct t_weechat_trigger
{
    char *pattern;
    char *domain;
    char **commands;
    char **channels;
    char **servers;
    char *action;
    char **cmds;
    
    struct t_weechat_trigger *prev_trigger;
    struct t_weechat_trigger *next_trigger;
} t_weechat_trigger;

#define CONF_FILE "trigger.rc"
#define CONF_SAVE 1
#define CONF_LOAD 2
#define DIR_SEP "/"

#endif /* WEECHAT_TRIGGER__H */
