/*
 * Copyright (c) 2003-2008 by FlashCode <flashcode@flashtux.org>
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

/* weechat-trigger.h: Trigger plugin support for WeeChat */

#ifndef __WEECHAT_TRIGGER_H
#define __WEECHAT_TRIGGER_H 1

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

#endif /* trigger.h */
