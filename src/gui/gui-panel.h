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


#ifndef __WEECHAT_GUI_PANEL_H
#define __WEECHAT_GUI_PANEL_H 1

#define GUI_PANEL_TOP     1
#define GUI_PANEL_BOTTOM  2
#define GUI_PANEL_LEFT    4
#define GUI_PANEL_RIGHT   8

#define GUI_PANEL_GLOBAL  1
#define GUI_PANEL_WINDOWS 2


/* panel structure */

typedef struct t_gui_panel t_gui_panel;

struct t_gui_panel
{
    int number;                     /* panel number                         */
    char *name;                     /* panel name                           */
    int position;                   /* position (top, bottom, left, right)  */
    void *panel_window;             /* pointer to panel window, NULL if     */
                                    /* displayed on each window (in this    */
                                    /* case, pointers are in windows)       */
    int separator;                  /* 1 if separator (line) displayed      */
    int size;                       /* panel size                           */
    t_gui_panel *prev_panel;        /* link to previous panel               */
    t_gui_panel *next_panel;        /* link to next panel                   */
};

/* panel variables */

extern t_gui_panel *gui_panels;
extern t_gui_panel *last_gui_panel;

#endif /* gui-panel.h */
