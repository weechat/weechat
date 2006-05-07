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


#ifndef __WEECHAT_GUI_PANEL_H
#define __WEECHAT_GUI_PANEL_H 1

#define GUI_PANEL_TOP     1
#define GUI_PANEL_BOTTOM  2
#define GUI_PANEL_LEFT    3
#define GUI_PANEL_RIGHT   4

#define GUI_PANEL_GLOBAL  0
#define GUI_PANEL_WINDOWS 1


/* panel structure */

typedef struct t_gui_panel t_gui_panel;

struct t_gui_panel
{
    int position;                   /* position (top, bottom, left, right)  */
    char *name;                     /* panel name                           */
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
