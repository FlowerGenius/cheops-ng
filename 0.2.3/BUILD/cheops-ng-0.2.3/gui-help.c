/*
 * Cheops Next Generation GUI
 * 
 * gui-help.c
 * stuff to display the about and liscense
 *
 * Copyright(C) 1999 Brent Priddy <toopriddy@mailcity.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 USA
 */

#include <gnome.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "cheops-gui.h"

GtkWidget *about;
GtkWidget *gpl;

extern char *gpl_text;

void destroy_about(GtkWidget *w, GtkWidget *l)
{
	if ((l != about) && (l != gpl))
		l = w;
	if (l)
		gtk_widget_destroy(l);
	if (l == about)
		about = NULL;
	if (l == gpl)
		gpl = NULL;
}

void do_license()
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *sb;
	GtkWidget *text;
	GtkWidget *close;
	GdkFont *font;
	
	font = gdk_font_load("fixed");
	
	if (gpl) {
		gtk_widget_show(gpl);
		gdk_window_raise(gpl->window);
		return;
	}
	gpl = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize(gpl);

	text = gtk_text_new(NULL, NULL);
	hbox = gtk_hbox_new(FALSE, 0);
	hbox2 = gtk_hbox_new(FALSE, 0);
	sb = gtk_vscrollbar_new(GTK_TEXT(text)->vadj);
	
	close = gtk_button_new_with_label("  Close  ");

	gtk_widget_set_usize(text, 480, 200);
	
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), text, TRUE, TRUE, 5);
	gtk_box_pack_start(GTK_BOX(hbox), sb, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 5);
	gtk_box_pack_end(GTK_BOX(hbox2), close, FALSE, FALSE, 5);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 5);
	gtk_widget_show(vbox);
	gtk_widget_show(close);
	gtk_widget_show(text);
	gtk_widget_show(sb);
	gtk_widget_show(hbox2);
	gtk_widget_show(hbox);
	gtk_signal_connect(GTK_OBJECT(close), "clicked", GTK_SIGNAL_FUNC(destroy_about), gpl);
	gtk_signal_connect(GTK_OBJECT(gpl), "delete_event", GTK_SIGNAL_FUNC(destroy_about), gpl);
	
	gtk_container_add(GTK_CONTAINER(gpl), vbox);
	gtk_widget_grab_focus(close);
	
	gtk_window_set_title(GTK_WINDOW(gpl), "License: GNU General Public License");
	gtk_widget_realize(gpl);
	gtk_widget_realize(vbox);
	gtk_widget_realize(hbox);
	gtk_widget_realize(text);
	gtk_text_insert(GTK_TEXT(text), font, NULL, NULL, gpl_text, strlen(gpl_text));
	
	gtk_widget_show(gpl);
}



void do_about(GtkWidget *w, gpointer data)
{
	GtkWidget *about;
	const char *authors[] = { "Brent Priddy <toopriddy@mailcity.com>", NULL };
	
	about = gnome_about_new( _(CHEOPS_TITLE), CHEOPS_VERSION, 
				_("Copyright Brent Priddy (C) 1999"),
				authors,
				"Cheops-ng is a network swiss-army knife",
				"/usr/local/share/pixmaps/cheops-ng.xpm");
	
	gtk_widget_show(about);
}

