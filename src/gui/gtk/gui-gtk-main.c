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

/* gui-gtk-main.c: main loop for Gtk GUI */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "../../common/weechat.h"
#include "../gui.h"
#include "../../common/fifo.h"
#include "../../common/utf8.h"
#include "../../common/weeconfig.h"
#include "gui-gtk.h"

#ifdef PLUGINS
#include "../../plugins/plugins.h"
#endif


GtkWidget *gtk_main_window;
GtkWidget *vbox1;
GtkWidget *entry_topic;
GtkWidget *notebook1;
GtkWidget *vbox2;
GtkWidget *hbox1;
GtkWidget *hpaned1;
GtkWidget *scrolledwindow_chat;
GtkWidget *scrolledwindow_nick;
GtkWidget *entry_input;
GtkWidget *label1;


/*
 * gui_main_loop: main loop for WeeChat with Gtk GUI
 */

void
gui_main_loop ()
{
    /* TODO: write this function for Gtk */
    gtk_main ();
}

/*
 * gui_main_pre_init: pre-initialize GUI (called before gui_init)
 */

void
gui_main_pre_init (int *argc, char **argv[])
{
    /* Initialise Gtk */
    gtk_init (argc, argv);
}

/*
 * gui_main_init: init GUI
 */

void
gui_main_init ()
{
    GdkColor color_fg, color_bg;
    
    gui_color_init ();
    
    gui_infobar = NULL;
    
    gui_ok = 1;
    
    /* init clipboard buffer */
    gui_input_clipboard = NULL;
    
    /* create Gtk widgets */

    gdk_color_parse ("white", &color_fg);
    gdk_color_parse ("black", &color_bg);
    
    gtk_main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (gtk_main_window), PACKAGE_STRING);
    
    g_signal_connect (G_OBJECT (gtk_main_window), "destroy",  gtk_main_quit, NULL);
    
    vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox1);
    gtk_container_add (GTK_CONTAINER (gtk_main_window), vbox1);
    
    entry_topic = gtk_entry_new ();
    gtk_widget_show (entry_topic);
    gtk_box_pack_start (GTK_BOX (vbox1), entry_topic, FALSE, FALSE, 0);
    gtk_widget_modify_text (entry_topic, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (entry_topic, GTK_STATE_NORMAL, &color_bg);
    
    notebook1 = gtk_notebook_new ();
    gtk_widget_show (notebook1);
    gtk_box_pack_start (GTK_BOX (vbox1), notebook1, TRUE, TRUE, 0);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook1), GTK_POS_BOTTOM);
    
    vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox2);
    gtk_container_add (GTK_CONTAINER (notebook1), vbox2);
    
    hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox1);
    gtk_box_pack_start (GTK_BOX (vbox2), hbox1, TRUE, TRUE, 0);
    
    hpaned1 = gtk_hpaned_new ();
    gtk_widget_show (hpaned1);
    gtk_box_pack_start (GTK_BOX (hbox1), hpaned1, TRUE, TRUE, 0);
    gtk_paned_set_position (GTK_PANED (hpaned1), 0);
    
    scrolledwindow_chat = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow_chat);
    gtk_paned_pack1 (GTK_PANED (hpaned1), scrolledwindow_chat, FALSE, TRUE);
    //gtk_box_pack_start (GTK_PANED (hpaned1), scrolledwindow_chat, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_chat), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_text (scrolledwindow_chat, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (scrolledwindow_chat, GTK_STATE_NORMAL, &color_bg);
    
    scrolledwindow_nick = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow_nick);
    gtk_paned_pack2 (GTK_PANED (hpaned1), scrolledwindow_nick, FALSE, TRUE);
    //gtk_box_pack_start (GTK_PANED (hpaned1), scrolledwindow_nick, TRUE, TRUE, 0);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow_nick), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_text (scrolledwindow_nick, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (scrolledwindow_nick, GTK_STATE_NORMAL, &color_bg);
    
    entry_input = gtk_entry_new ();
    gtk_widget_show (entry_input);
    gtk_box_pack_start (GTK_BOX (vbox2), entry_input, FALSE, FALSE, 0);
    gtk_widget_modify_text (entry_input, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (entry_input, GTK_STATE_NORMAL, &color_bg);
    
    label1 = gtk_label_new (_("server"));
    gtk_widget_show (label1);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (notebook1), gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook1), 0), label1);
    gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_LEFT);
    
    gtk_widget_show_all (gtk_main_window);
    
    /* create new window/buffer */
    if (gui_window_new (NULL, 0, 0, 0, 0, 100, 100))
    {
        gui_current_window = gui_windows;
        gui_buffer_new (gui_windows, NULL, NULL, 0, 1);
        
        if (cfg_look_set_title)
            gui_window_set_title ();
        
        gui_init_ok = 1;
    }
}

/*
 * gui_main_end: GUI end
 */

void
gui_main_end ()
{
    t_gui_window *ptr_win;
    
    /* free clipboard buffer */
    if (gui_input_clipboard)
      free(gui_input_clipboard);

    /* delete all panels */
    while (gui_panels)
        gui_panel_free (gui_panels);
    
    /* delete all windows */
    for (ptr_win = gui_windows; ptr_win; ptr_win = ptr_win->next_window)
    {
        /* TODO: destroy Gtk widgets */
    }
    
    /* delete all buffers */
    while (gui_buffers)
        gui_buffer_free (gui_buffers, 0);
    
    /* delete all windows */
    while (gui_windows)
        gui_window_free (gui_windows);
    gui_window_tree_free (&gui_windows_tree);
    
    /* delete global history */
    history_global_free ();
    
    /* delete infobar messages */
    while (gui_infobar)
        gui_infobar_remove ();

    /* reset title */
    if (cfg_look_set_title)
	gui_window_reset_title ();
}
