/*
 * gui-gtk-main.c - main loop for Gtk GUI
 *
 * Copyright (C) 2003-2013 Sebastien Helleu <flashcode@flashtux.org>
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
 * along with WeeChat.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "../../core/weechat.h"
#include "../../core/wee-config.h"
#include "../../core/wee-utf8.h"
#include "../../core/wee-version.h"
#include "../../plugins/plugin.h"
#include "../gui-bar.h"
#include "../gui-bar-item.h"
#include "../gui-bar-window.h"
#include "../gui-chat.h"
#include "../gui-main.h"
#include "../gui-buffer.h"
#include "../gui-filter.h"
#include "../gui-history.h"
#include "../gui-input.h"
#include "../gui-layout.h"
#include "../gui-window.h"
#include "gui-gtk.h"


GtkWidget *gui_gtk_main_window;
GtkWidget *gui_gtk_vbox1;
GtkWidget *gui_gtk_entry_topic;
GtkWidget *gui_gtk_notebook1;
GtkWidget *gui_gtk_vbox2;
GtkWidget *gui_gtk_hbox1;
GtkWidget *gui_gtk_hpaned1;
GtkWidget *gui_gtk_scrolledwindow_chat;
GtkWidget *gui_gtk_scrolledwindow_nick;
GtkWidget *gui_gtk_entry_input;
GtkWidget *gui_gtk_label1;


/*
 * Pre-initializes GUI (called before gui_init).
 */

void
gui_main_pre_init (int *argc, char **argv[])
{
    /* pre-init colors */
    gui_color_pre_init ();

    /* init some variables for chat area */
    gui_chat_init ();

    /* Initialise Gtk */
    gtk_init (argc, argv);
}

/*
 * Initializes GUI.
 */

void
gui_main_init ()
{
    struct t_gui_buffer *ptr_buffer;
    struct t_gui_bar *ptr_bar;
    struct t_gui_bar_window *ptr_bar_win;
    GdkColor color_fg, color_bg;

    gui_color_init ();

    /* build prefixes according to configuration */
    gui_chat_prefix_build ();

    /* init clipboard buffer */
    gui_input_clipboard = NULL;

    /* create Gtk widgets */

    gdk_color_parse ("white", &color_fg);
    gdk_color_parse ("black", &color_bg);

    gui_gtk_main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (gui_gtk_main_window),
                          version_get_name_version ());

    g_signal_connect (G_OBJECT (gui_gtk_main_window), "destroy", gtk_main_quit, NULL);

    gui_gtk_vbox1 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (gui_gtk_vbox1);
    gtk_container_add (GTK_CONTAINER (gui_gtk_main_window), gui_gtk_vbox1);

    gui_gtk_entry_topic = gtk_entry_new ();
    gtk_widget_show (gui_gtk_entry_topic);
    gtk_box_pack_start (GTK_BOX (gui_gtk_vbox1), gui_gtk_entry_topic, FALSE, FALSE, 0);
    gtk_widget_modify_text (gui_gtk_entry_topic, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (gui_gtk_entry_topic, GTK_STATE_NORMAL, &color_bg);

    gui_gtk_notebook1 = gtk_notebook_new ();
    gtk_widget_show (gui_gtk_notebook1);
    gtk_box_pack_start (GTK_BOX (gui_gtk_vbox1), gui_gtk_notebook1, TRUE, TRUE, 0);
    gtk_notebook_set_tab_pos (GTK_NOTEBOOK (gui_gtk_notebook1), GTK_POS_BOTTOM);

    gui_gtk_vbox2 = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (gui_gtk_vbox2);
    gtk_container_add (GTK_CONTAINER (gui_gtk_notebook1), gui_gtk_vbox2);

    gui_gtk_hbox1 = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (gui_gtk_hbox1);
    gtk_box_pack_start (GTK_BOX (gui_gtk_vbox2), gui_gtk_hbox1, TRUE, TRUE, 0);

    gui_gtk_hpaned1 = gtk_hpaned_new ();
    gtk_widget_show (gui_gtk_hpaned1);
    gtk_box_pack_start (GTK_BOX (gui_gtk_hbox1), gui_gtk_hpaned1, TRUE, TRUE, 0);
    gtk_paned_set_position (GTK_PANED (gui_gtk_hpaned1), 0);

    gui_gtk_scrolledwindow_chat = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (gui_gtk_scrolledwindow_chat);
    gtk_paned_pack1 (GTK_PANED (gui_gtk_hpaned1), gui_gtk_scrolledwindow_chat,
                     FALSE, TRUE);
    /*gtk_box_pack_start (GTK_PANED (hpaned1), scrolledwindow_chat, TRUE, TRUE, 0);*/
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (gui_gtk_scrolledwindow_chat),
                                    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_text (gui_gtk_scrolledwindow_chat, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (gui_gtk_scrolledwindow_chat, GTK_STATE_NORMAL, &color_bg);

    gui_gtk_scrolledwindow_nick = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (gui_gtk_scrolledwindow_nick);
    gtk_paned_pack2 (GTK_PANED (gui_gtk_hpaned1), gui_gtk_scrolledwindow_nick,
                     FALSE, TRUE);
    /*gtk_box_pack_start (GTK_PANED (hpaned1), scrolledwindow_nick, TRUE, TRUE, 0);*/
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (gui_gtk_scrolledwindow_nick),
                                    GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_widget_modify_text (gui_gtk_scrolledwindow_nick, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (gui_gtk_scrolledwindow_nick, GTK_STATE_NORMAL, &color_bg);

    gui_gtk_entry_input = gtk_entry_new ();
    gtk_widget_show (gui_gtk_entry_input);
    gtk_box_pack_start (GTK_BOX (gui_gtk_vbox2), gui_gtk_entry_input, FALSE,
                        FALSE, 0);
    gtk_widget_modify_text (gui_gtk_entry_input, GTK_STATE_NORMAL, &color_fg);
    gtk_widget_modify_base (gui_gtk_entry_input, GTK_STATE_NORMAL, &color_bg);

    gui_gtk_label1 = gtk_label_new (_("server"));
    gtk_widget_show (gui_gtk_label1);
    gtk_notebook_set_tab_label (GTK_NOTEBOOK (gui_gtk_notebook1),
                                gtk_notebook_get_nth_page (GTK_NOTEBOOK (gui_gtk_notebook1), 0),
                                gui_gtk_label1);
    gtk_label_set_justify (GTK_LABEL (gui_gtk_label1), GTK_JUSTIFY_LEFT);

    gtk_widget_show_all (gui_gtk_main_window);

    gui_init_ok = 0;

    /* create core buffer */
    ptr_buffer = gui_buffer_new (NULL, "weechat", NULL, NULL, NULL, NULL);
    if (ptr_buffer)
    {
        gui_init_ok = 1;

        /* set title for core buffer */
        gui_buffer_set_title (ptr_buffer,
                              "WeeChat " WEECHAT_COPYRIGHT_DATE
                              " - " WEECHAT_WEBSITE);

        /* create main window (using full space) */
        if (gui_window_new (NULL, ptr_buffer, 0, 0, 0, 0, 100, 100))
        {
            gui_current_window = gui_windows;

            if (CONFIG_BOOLEAN(config_look_set_title))
                gui_window_set_title (version_get_name_version ());
        }

        /*
         * create bar windows for root bars (they were read from config,
         * but no window was created (GUI was not initialized)
         */
        for (ptr_bar = gui_bars; ptr_bar; ptr_bar = ptr_bar->next_bar)
        {
            if ((CONFIG_INTEGER(ptr_bar->options[GUI_BAR_OPTION_TYPE]) == GUI_BAR_TYPE_ROOT)
                && (!ptr_bar->bar_window))
            {
                gui_bar_window_new (ptr_bar, NULL);
            }
        }
        for (ptr_bar_win = gui_windows->bar_windows;
             ptr_bar_win; ptr_bar_win = ptr_bar_win->next_bar_window)
        {
            gui_bar_window_calculate_pos_size (ptr_bar_win, gui_windows);
            gui_bar_window_create_win (ptr_bar_win);
        }
    }
}

/*
 * Main loop for WeeChat with Gtk GUI.
 */

void
gui_main_loop ()
{
    /* TODO: write this function for Gtk */
    gtk_main ();
}

/*
 * Ends GUI.
 *
 * Argument "clean_exit" is 0 when WeeChat is crashing (we don't clean objects
 * because WeeChat can crash again during this cleanup...).
 */

void
gui_main_end (int clean_exit)
{
    if (clean_exit)
    {
        /* remove bar items and bars */
        gui_bar_item_end ();
        gui_bar_free_all ();

        /* remove filters */
        gui_filter_free_all ();

        /* free clipboard buffer */
        if (gui_input_clipboard)
            free(gui_input_clipboard);

        /* delete layout saved */
        gui_layout_window_remove_all (&gui_layout_windows);
        gui_layout_buffer_remove_all (&gui_layout_buffers, &last_gui_layout_buffer);

        /* delete all windows */
        while (gui_windows)
        {
            gui_window_free (gui_windows);
            /* TODO: destroy Gtk widgets */
        }
        gui_window_tree_free (&gui_windows_tree);

        /* delete all buffers */
        while (gui_buffers)
        {
            gui_buffer_close (gui_buffers);
        }

        /* delete global history */
        gui_history_global_free ();

        /* reset title */
        if (CONFIG_BOOLEAN(config_look_set_title))
            gui_window_set_title (NULL);

        /* end color */
        gui_color_end ();

        /* free some variables used for chat area */
        gui_chat_end ();
    }
}
