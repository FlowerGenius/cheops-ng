/*
 * Cheops Next Generation GUI
 * 
 * gui-utils.c
 * GUI Utilities
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
#include "cheops-gui.h"
#include "gui-utils.h"
GtkWidget *current_dialog;


void click_ok_on_gnome_dialog(GtkWidget *w, GtkWidget *dialog)
{
	gtk_signal_emit_by_name(GTK_OBJECT(dialog), "clicked", 0);
}

void kill_widget_argument(GtkWidget *w, GtkWidget *fs)
{
	gtk_widget_destroy(fs);
}

void gtk_widget_focus_me(GtkWidget *w, GtkWidget *it)
{
	if(it)
	{
		gtk_widget_grab_focus(GTK_WIDGET(it));
	}
}

void close_dialog(GtkWidget *w, GtkWidget *dialog)
{
	gtk_grab_remove(current_dialog);
	gtk_widget_destroy(current_dialog);
	current_dialog = NULL;
}

void closing_dialog(GtkWidget *w, GtkWidget *dialog)
{
	gtk_grab_remove(dialog);
	current_dialog = NULL;
}

/*
 * make a yes no dialog
 *
 * if you replace the no_callback then you MUST do a gtk_grab_remove on current_dialog !!!!
 */
void make_yes_no_dialog(char *title,
			char *message,
			char *yes_label,
			char *no_label,
			void *yes_callback,
			void *yes_data,
			void *no_callback,
			void *no_data)
{
	GtkWidget *button;
	GtkWidget *label;
	GtkWidget *dialog;
	
	dialog = gtk_dialog_new();
	
	current_dialog = dialog;
/*
 * Set the window attributes
 */
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy", GTK_SIGNAL_FUNC(closing_dialog), dialog);
	gtk_container_border_width(GTK_CONTAINER(dialog), 5);
	
	if(title)
		gtk_window_set_title(GTK_WINDOW(dialog), title);
	else
		gtk_window_set_title(GTK_WINDOW(dialog), "Dialog...");
	
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

/*
 * Set up the message
 */
 	if(message)
 		label = gtk_label_new(message);
 	else
 		label = gtk_label_new("Are you sure?");

	gtk_misc_set_padding(GTK_MISC(label), 10, 10);
	
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label, TRUE, TRUE, 0);
	
	gtk_widget_show(label);


/*
 * Set up the "yes" button
 */
	if(yes_label)
		button = gtk_button_new_with_label(yes_label);
	else
		button = gtk_button_new_with_label("Yes");
	
	if(yes_callback)
		gtk_signal_connect(GTK_OBJECT(button),"clicked", yes_callback, yes_data);
	else 
		gtk_signal_connect(GTK_OBJECT(button),"clicked", close_dialog, NULL);
	
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 5);
	
	gtk_widget_show(button);
/*
 * Set up the "no" button
 */
	if(no_label)
		button = gtk_button_new_with_label(no_label);
	else
		button = gtk_button_new_with_label("No");
	
	if(no_callback)
		gtk_signal_connect(GTK_OBJECT(button),"clicked", no_callback, yes_data);
	else 
		gtk_signal_connect(GTK_OBJECT(button),"clicked", close_dialog, NULL);
	
	GTK_WIDGET_SET_FLAGS(button, GTK_CAN_DEFAULT);
	
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button, TRUE, TRUE, 5);
	
	gtk_widget_grab_default(button);

	gtk_widget_show(button);
	
	gtk_widget_show(dialog);

	gtk_grab_add(dialog);
}

void set_kill_me_too_close(GtkWidget *w, gpointer p)
{
	kill_me_too = NULL;
}

void toggle_dialog_toggle(GtkWidget *w, int *value)
{
	if(value)
		*value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)) != 0 ? 0 : 1;
}

/* 
 * This makes a gnome dialog box with a title message and default  button
 *
 * return: 0 if yes was pressed and 1 if no was pressed
 */
int make_gnome_yes_no_dialog(char *title, char *message, int no_default, int *ask_again)
{
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *toggle;
		
	if(no_default > 1 || no_default < 0)
		no_default = 1;
		
	dialog = gnome_dialog_new(title,GNOME_STOCK_BUTTON_YES,GNOME_STOCK_BUTTON_NO,NULL);
	gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
	gnome_dialog_set_default(GNOME_DIALOG(dialog), no_default);
	kill_me_too = dialog; /* so when the main_window is killed this dialog can be also */
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy", set_kill_me_too_close, NULL);		

	label = gtk_label_new(message);

	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
	gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), label, TRUE, TRUE, 0);
	
	if(ask_again)
	{
		toggle = gtk_check_button_new_with_label("Do not ask me again");
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), 0);
		gtk_signal_connect(GTK_OBJECT(toggle), "toggled", toggle_dialog_toggle, ask_again);
		gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), toggle, TRUE, TRUE, 0);
		gtk_widget_show(toggle);
	}
			
	gtk_widget_show( label);

	return( gnome_dialog_run_and_close(GNOME_DIALOG(dialog)) );
}

/* 
 * This makes a gnome dialog box with a title message and default  button
 */
int make_gnome_ok_dialog(char *title, char *message)
{
	GtkWidget *dialog;
	GtkWidget *label;
	
	dialog = gnome_dialog_new(title,GNOME_STOCK_BUTTON_OK,NULL);
	gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
	gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);
	kill_me_too = dialog; /* so when the main_window is killed this dialog can be also */
	gtk_signal_connect(GTK_OBJECT(dialog), "destroy", set_kill_me_too_close, NULL);		
	
	label = gtk_label_new(message);

	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
	gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), label, TRUE, TRUE, 0);
			
	gtk_widget_show( label);

	return( gnome_dialog_run_and_close(GNOME_DIALOG(dialog)) );
}

#ifndef DEBUGGING_MEMORY			
char *makestring(char *s)
{
	char *c;
	if( NULL == (c = malloc(strlen(s) + 1)) )
	{
		g_print("could not malloc\n");
	}
	strcpy(c,s);
	return(c);
}
#else
	char *makestring_temp;
#endif	

