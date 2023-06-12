/*
 * Cheops Next Generation GUI
 * 
 * cheops-gui.c
 * An agent GUI, for testing and communicating directly with agents
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

#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <gnome.h>
#include "event.h"
#include "cheops-gui.h"
#include "gui-viewspace.h"
#include "gui-settings.h"
#include "gui-canvas.h"
#include "gui-help.h"
#include "gui-utils.h"
#include "logger.h"
#include "gui-handlers.h"
#include "gui-config.h"
#include "gui-sched.h"
#include "pixmaps/unknown.xpm"


#include <gtk/gtk.h>
#include <gdk/gdkx.h>


//#define DEBUG_CHEOPS_GUI

#ifdef DEBUG_CHEOPS_GUI
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

/* duh... */
cheops_window *main_window = NULL;

/* should be the current notebook page */
net_page *current_page = NULL;

/* global preferences */
gui_preferences preferences;

/* this is for modal gnome_dialogs, so we can kill them if the upper level application is killed */
GtkWidget *kill_me_too = NULL;

int we_are_still_running = 1;

/* Buffer for creating all events, and event stuff, dont you love event driven programs :) */
char ebuf[65536];
event_hdr *eh = (event_hdr *)ebuf;
event *ee = (event *)(ebuf + sizeof(event_hdr));

static void do_save(GtkWidget *widget, int data);
void open_file_dialog(GtkWidget *w, gpointer p);
void make_save_dialog(char *title, char *filename, void *ok_clicked_function, void *cancel_clicked_function, char *message);
static void do_write_config_file_and_exit(GtkWidget *w, GtkWidget *fs);


/*
 * This is either called from opening a file while we have one alredy open or, we are quitting
 * and we should save the config
 */
void save_changes(int quit)
{
	int ask_again = TRUE;
	
	if(options_save_changes_on_exit && main_window->filename)
	{
		write_config_file(NULL,NULL);
		if(!quit)
		{
			open_file_dialog(NULL, NULL);

			return;
		}
	}
	else
	{
		if(main_window->filename == NULL)
		{
			switch(make_gnome_yes_no_dialog("Save Session?","Would you like to save your session?", FALSE, &ask_again))
			{
				case 0:
					if(!ask_again)
						options_save_changes_on_exit = TRUE;
					
					if(quit)
						make_save_dialog("Save As", NULL, do_write_config_file_and_exit, NULL, NULL);
					else
						make_save_dialog("Save As", NULL, open_file_dialog, NULL, NULL);
						
					return; // dont quit yet, open up a file save dialog
					
				case 1:
					if(!quit)
					{
						open_file_dialog(NULL, NULL);
						return;
					}
					break;
			}
		}
		else
		{
			switch(make_gnome_yes_no_dialog("Save Session?","Would you like to save your session?", FALSE, &ask_again))
			{
				case 0:
					if(!ask_again)
						options_save_changes_on_exit = TRUE;
					
					write_config_file(NULL, NULL);

					break;
				case 1:
					if(!quit)
					{
						open_file_dialog(NULL, NULL);
						return;
					}
					break;
			}
		}
	}
	gtk_main_quit();
}

void do_quit(GtkWidget *w, gpointer data)
{
	int save = 0;
	net_page *np;
	
	if(kill_me_too != NULL)
		gnome_dialog_close(GNOME_DIALOG(kill_me_too));

	we_are_still_running = FALSE;

	for(np = main_window->net_pages; np; np = np->next)
	{
		if(np->connected)
		{
			save = 1;
			break;
		}
	}

	if(save)
	{
		save_changes(TRUE);
		event_cleanup();
	}
	else
	{
		event_cleanup();
		gtk_main_quit();
	}
}

void quit(void)
{
	do_quit(NULL,NULL);
}

void destroy(GtkWidget *w, gpointer p)
{
	gtk_widget_destroy((GtkWidget *)p);
}

void showme(GtkWidget *widget, gpointer data)
{
	printf("event '%s'\n",(char *)data);	
} 

void do_page(GtkWidget *widget, gint data)
{
	GtkWidget *dialog = NULL;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *entry1;
	net_page *np;
	
	switch(data)
	{
		case 0:
			dialog = gnome_dialog_new("Add Viewspace",
							  GNOME_STOCK_BUTTON_OK,
							  GNOME_STOCK_BUTTON_CANCEL,
							  NULL);
			gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
			
			entry1 = gnome_entry_new("new_viewspace");
			label = gtk_label_new("New page name:");
			hbox = gtk_hbox_new(FALSE, 5);
			gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
			
			gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 5);
			gtk_box_pack_start( GTK_BOX(hbox), entry1, FALSE, FALSE, 5);
			gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

			gtk_widget_grab_focus(GTK_COMBO(entry1)->entry);

			gtk_signal_connect(GTK_OBJECT(GTK_COMBO(entry1)->entry),  "activate", GTK_SIGNAL_FUNC(click_ok_on_gnome_dialog), dialog);
			
			gtk_widget_show( hbox );
			gtk_widget_show( label );
			gtk_widget_show( entry1 );

			switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
			{
				case 0:
					add_viewspace(NULL, gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(entry1)))) );
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				case 1:	
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				default:
					break;
			}
			break;
		case 1:
			if( make_gnome_yes_no_dialog("Remove Viewspace","Are you sure you want to delete this viewspace?",TRUE, NULL) == 0)
			{
				remove_viewspace(NULL, dialog);
			}
			break;
		case 2:
			dialog = gnome_dialog_new("Rename Viewspace",
							  GNOME_STOCK_BUTTON_OK,
							  GNOME_STOCK_BUTTON_CANCEL,
							  NULL);
			gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
			
			entry1 = gnome_entry_new("new_viewspace");
			label = gtk_label_new("Viewspace Name:");
			hbox = gtk_hbox_new(FALSE, 5);
			gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
			
			gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 5);
			gtk_box_pack_start( GTK_BOX(hbox), entry1, FALSE, FALSE, 5);
			gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

			np = get_current_net_page();
			if(np && np->label)
			{
				char *text = NULL;
				gtk_label_get(GTK_LABEL(np->label), &text);
				if(text != NULL)
				{
					gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(entry1)->entry), text);
				}
			}
			gtk_widget_grab_focus(GTK_COMBO(entry1)->entry);

			gtk_signal_connect(GTK_OBJECT(GTK_COMBO(entry1)->entry),  "activate", GTK_SIGNAL_FUNC(click_ok_on_gnome_dialog), dialog);
			
			gtk_widget_show( hbox );
			gtk_widget_show( label );
			gtk_widget_show( entry1 );

			switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
			{
				case 0:
					net_page_rename(np, gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(entry1)))) );
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				case 1:	
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}

}

static void do_network(GtkWidget *widget, gint data)
{
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *label2;
	GtkWidget *entry1;
	GtkWidget *entry2;
	char *c1, *c2;
	
	switch(data)
	{
		case 0:
		
			dialog = gnome_dialog_new("Add Host",
							  GNOME_STOCK_BUTTON_OK,
							  GNOME_STOCK_BUTTON_CANCEL,
							  NULL);
			gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
			
			entry1 = gnome_entry_new("add_host_ip");
			label = gtk_label_new("Host / Address:");
			hbox = gtk_hbox_new(FALSE, 5);
			gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
			
			gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 5);
			gtk_box_pack_start( GTK_BOX(hbox), entry1, FALSE, FALSE, 5);
			gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

			gtk_widget_grab_focus(GTK_COMBO(entry1)->entry);

			gtk_signal_connect(GTK_OBJECT(GTK_COMBO(entry1)->entry),  "activate", GTK_SIGNAL_FUNC(click_ok_on_gnome_dialog), dialog);
			
			gtk_widget_show( hbox );
			gtk_widget_show( label );
			gtk_widget_show( entry1 );

			switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
			{
				case 0:
					c1 = strdup(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(entry1)))) );
					
					add_network(NULL, c1, NULL); 
					if(c1)
						free(c1);
					
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				case 1:	
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				default:
					break;
			}
			break;

		case 1:
		
			dialog = gnome_dialog_new("Add Network",
							  GNOME_STOCK_BUTTON_OK,
							  GNOME_STOCK_BUTTON_CANCEL,
							  NULL);
			gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
			
			entry1 = gnome_entry_new("add_host_ip");
			entry2 = gnome_entry_new("add_host_network");
			label = gtk_label_new("Network:");
			label2 = gtk_label_new("Netmask:");

			hbox = gtk_hbox_new(FALSE, 5);
			gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
			
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
			gtk_box_pack_start(GTK_BOX(hbox), entry1, FALSE, FALSE, 5);
			
			gtk_box_pack_start(GTK_BOX(hbox), label2, FALSE, FALSE, 5);
			gtk_box_pack_start(GTK_BOX(hbox), entry2, FALSE, FALSE, 5);

			gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

			gtk_widget_grab_focus(GTK_COMBO(entry1)->entry);

			gtk_widget_show( hbox );
			gtk_widget_show( label );
			gtk_widget_show( entry1 );
			gtk_widget_show( label2 );
			gtk_widget_show( entry2 );

			gtk_signal_connect(GTK_OBJECT(GTK_COMBO(entry1)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), GTK_COMBO(entry2)->entry);
			gtk_signal_connect(GTK_OBJECT(GTK_COMBO(entry2)->entry),  "activate", GTK_SIGNAL_FUNC(click_ok_on_gnome_dialog), dialog);

			switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
			{
				case 0:
					c1 = strdup(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(entry1)))) );
					c2 = strdup(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(entry2)))) );
					
					add_network(NULL, c1, c2); 
					if(c1)
						free(c1);
					if(c2)
						free(c2);
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				case 1:	
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				default:
					break;
			}
			break;
		case 2:
		
			dialog = gnome_dialog_new("Add Network Range",
							  GNOME_STOCK_BUTTON_OK,
							  GNOME_STOCK_BUTTON_CANCEL,
							  NULL);
			gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
			
			entry1 = gnome_entry_new("add_host_ip");
			entry2 = gnome_entry_new("add_host_ip");
			label = gtk_label_new("First IP:");
			label2 = gtk_label_new("Last IP:");

			hbox = gtk_hbox_new(FALSE, 5);
			gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
			
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
			gtk_box_pack_start(GTK_BOX(hbox), entry1, FALSE, FALSE, 5);
			
			gtk_box_pack_start(GTK_BOX(hbox), label2, FALSE, FALSE, 5);
			gtk_box_pack_start(GTK_BOX(hbox), entry2, FALSE, FALSE, 5);

			gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

			gtk_widget_show( hbox );
			gtk_widget_show( label );
			gtk_widget_show( entry1 );
			gtk_widget_show( label2 );
			gtk_widget_show( entry2 );

			gtk_widget_grab_focus(GTK_COMBO(entry1)->entry);

			gtk_signal_connect(GTK_OBJECT(GTK_COMBO(entry1)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), GTK_COMBO(entry2)->entry);
			gtk_signal_connect(GTK_OBJECT(GTK_COMBO(entry2)->entry),  "activate", GTK_SIGNAL_FUNC(click_ok_on_gnome_dialog), dialog);
			
			switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
			{
				case 0:
					c1 = strdup(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(entry1)))) );
					c2 = strdup(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(entry2)))) );
					
					DEBUG(
						c_log(LOG_DEBUG, "Discovering range %s to %s\n", c1, c2);
					);
					
					add_network_range(c1, c2); 
					if(c1)
						free(c1);
					if(c2)
						free(c2);
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				case 1:	
					gnome_dialog_close(GNOME_DIALOG(dialog));
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}

static void do_map(GtkWidget *w, gpointer data)
{
	switch(GPOINTER_TO_INT(data))
	{
		case 0:
			net_page_map_all(get_current_net_page());
			break;

		case 1:
			net_page_unmap_all(get_current_net_page());
			net_page_map_all(get_current_net_page());
			break;
			
		default:
			printf("do_map(): there should not be a default\n");
	}
}

static void do_unmap(GtkWidget *w, gpointer data)
{
	net_page_unmap_all(get_current_net_page());
}

static void do_select_all(GtkWidget *w, gpointer data)
{
	net_page_highlight_all(get_current_net_page(), TRUE);
	net_page_set_plink();
}

/*
 * Callback for the menu delete selection (it creates a confirm dialog)
 */
void do_delete_page_object(GtkWidget *w, gpointer data)
{
	int ask_again = TRUE;
/* check and see if there is nothing selected */
	if((get_current_net_page())->plink == NULL)
		return;

	if(options_confirm_delete)
	{
		if(make_gnome_yes_no_dialog("Delete?!?","Are you Sure you want to delete all Highlighted items?",TRUE, &ask_again) == 0 )
		{
			net_page_delete_page_object(get_current_net_page());
			if(!ask_again)
				options_confirm_delete = FALSE;
		}
	}
	else
		net_page_delete_page_object(get_current_net_page());
	
}

static void do_write_config_file_and_exit(GtkWidget *w, GtkWidget *fs)
{
	write_config_file(w,fs);
	gtk_main_quit();
}


void make_save_dialog(char *title, char *filename, void *ok_clicked_function, void *cancel_clicked_function, char *message)
{
	GtkWidget *fs;
	GtkWidget *label;
	
	if((fs = gtk_file_selection_new(title)) != NULL)
	{
		gtk_window_set_position(GTK_WINDOW(fs), GTK_WIN_POS_CENTER);
		if(filename)
			gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs), filename);
		else
			gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs),CHEOPS_DEFAULT_FILENAME);

		if(ok_clicked_function)
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button), "clicked", ok_clicked_function, fs);
		else
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button), "clicked", write_config_file, fs);
			
		if(cancel_clicked_function)
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button), "clicked", ok_clicked_function, fs);
		else
			gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button), "clicked", kill_widget_argument, fs);

		gtk_signal_connect(GTK_OBJECT(fs), "delete_event", gtk_widget_destroy, fs);
		
		if(message)
		{
			label = gtk_label_new(message);

			gtk_box_pack_end(GTK_BOX(GTK_FILE_SELECTION(fs)->main_vbox), label, FALSE, FALSE, 5);
		}
		
		gtk_widget_show_all(fs);
	}
}

static void do_save(GtkWidget *widget, int data)
{
	if((main_window->filename != NULL) && !data)
	{
		write_config_file(NULL,NULL); /* this will write to the existing filename */
		return;
	}
	else
		make_save_dialog("Save As", main_window->filename, (void *)write_config_file, NULL, NULL);
}

void open_file_and_read_config_file(GtkWidget *w, GtkWidget *fs)
{
	net_page_remove_all();
	read_config_file(w, fs);
}

void open_file_dialog(GtkWidget *w, gpointer p)
{
	GtkWidget *fs = gtk_file_selection_new("Open");
	if(fs)
	{
		gtk_window_set_position(GTK_WINDOW(fs), GTK_WIN_POS_CENTER);
		gtk_file_selection_set_filename(GTK_FILE_SELECTION(fs),CHEOPS_DEFAULT_FILENAME);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button), "clicked", open_file_and_read_config_file, fs);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button), "clicked", kill_widget_argument, fs);
		gtk_signal_connect(GTK_OBJECT(fs), "delete_event", gtk_widget_destroy, fs);
		gtk_widget_show_all(fs);
	}
}

static void do_open(GtkWidget *widget, gpointer data)
{
	save_changes(FALSE); // don't quit	
}

static void do_zoom(GtkWidget *w, gpointer data)
{
	switch(GPOINTER_TO_INT(data))
	{
		case 0:
			net_page_zoom_normal(get_current_net_page());
			break;

		case 1:
			net_page_zoom_in(get_current_net_page());
			break;

		case 2:
			net_page_zoom_out(get_current_net_page());
			break;
			
		default:
			printf("do_map(): there should not be a default\n");
	}
}

int close_app(GtkWidget *widget, gpointer data)
{
   gtk_main_quit();
   return TRUE;
}

int export_type = 0;

// http://cvs.gnome.org/lxr/source/imlib/
// this place had some info about saving
int export_timeout_callback(void *data)
{
	net_page *np = get_current_net_page();
	GdkImlibImage *image;
	GtkAdjustment *vadj, *hadj;
	char *name = data;
	int width, height;
	
	switch(export_type)
	{
		case 0:
			if(np && np->canvas && name && name[0])
			{
				hadj = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(np->scrolled_window));
				vadj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(np->scrolled_window));

				DEBUG(
					printf("H adj\n"
					       "lower          %f\n"
					       "upper          %f\n"
					       "value          %f\n"
					       "step_increment %f\n"
					       "page_increment %f\n"
					       "page_size      %f\n",
					       hadj->lower,
					       hadj->upper,
					       hadj->value,
					       hadj->step_increment,
					       hadj->page_increment,
					       hadj->page_size);
					printf("H adj\n"
					       "lower          %f\n"
					       "upper          %f\n"
					       "value          %f\n"
					       "step_increment %f\n"
					       "page_increment %f\n"
					       "page_size      %f\n",
					       vadj->lower,
					       vadj->upper,
					       vadj->value,
					       vadj->step_increment,
					       vadj->page_increment,
					       vadj->page_size);
				);
				gnome_canvas_update_now(GNOME_CANVAS(np->canvas));
				gnome_canvas_request_redraw(GNOME_CANVAS(np->canvas), 
				                            0, 0,
				                            hadj->page_size, 
				                            vadj->page_size);
				while(gtk_events_pending())
					gtk_main_iteration();                            
				
				image = gdk_imlib_create_image_from_drawable(np->canvas->window, 
				                                             NULL,
				                                             0, 0, 
				                                             hadj->page_size, 
				                                             vadj->page_size);

				if(gdk_imlib_save_image(image, name, NULL))
				{
					printf("file saved '%s'\n", name);
				}
				else
				{
					printf("file not saved '%s'\n", name);
				}
			}
			break;
			
		case 1:
			gdk_window_get_size(main_window->window->window, &width, &height);
			image = gdk_imlib_create_image_from_drawable(main_window->window->window, 
			                                             NULL,
			                                             0, 0, 
			                                             width, 
			                                             height);

			if(gdk_imlib_save_image(image, name, NULL))
			{
				printf("file saved '%s'\n", name);
			}
			else
			{
				printf("file not saved '%s'\n", name);
			}
			break;
	}
	if(name)
		free(name);

	return(0);
}

void export_callback(GtkWidget *w, GtkWidget *fs)
{
	char *name = makestring(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));
	net_page *np = get_current_net_page();
	
	gtk_widget_destroy(fs);
	gnome_canvas_update_now(GNOME_CANVAS(np->canvas));
	while(gtk_events_pending())
		gtk_main_iteration();

	cheops_sched_add(100, export_timeout_callback, name);
}

static void do_export(GtkWidget *w, gpointer data)
{
	char *name;
	net_page *np = get_current_net_page();
	
	export_type = GPOINTER_TO_INT(data);
	if(np->name && strlen(np->name) > 0)
	{
		name = malloc(strlen(np->name) + 1 + 4);
		sprintf(name, "%s.jpg", np->name);
	}
	else
	{
		name = malloc(strlen("map.jpg") + 1);
		strcpy(name, "map.jpg");
	}
	make_save_dialog("Export Viewspace to a graphic file (saved by extension)", 
	                 name, 
	                 (void *)export_callback, 
	                 NULL,
	                 "The graphic type will be selected by the extention\n"
	                 "The applicable types are:"
	                 "gif, png, jpg, xpm, bmp");

	free(name);
}



void make_main_window(cheops_window *w)
{
	static GtkItemFactoryEntry menu_items[] = 
	{
		{ "/_File",                             NULL,         NULL,                  0, "<Branch>" },
		{ "/File/tearoff1",                     NULL,         NULL,                  0, "<Tearoff>" },
		{ "/File/_Open...",                     "<control>O", do_open,               0 },
		{ "/File/_Save",                        "<control>S", do_save,               0 },
		{ "/File/Save _As",                     "",           do_save,               1 },
		{ "/File/sep1",                         NULL,         NULL,                  0, "<Separator>" },
		{ "/File/Export Viewspace...",          "",           do_export,             0 },
		{ "/File/Export Window...",             "",           do_export,             1 },
		{ "/File/sep1",                         NULL,         NULL,                  0, "<Separator>" },
		{ "/File/_Quit",                        "<control>Q", do_quit,               0 },
		{ "/_Edit",                             NULL,         NULL,                  0, "<Branch>" },
		{ "/Edit/tearoff1",                     NULL,         NULL,                  0, "<Tearoff>" },
		{ "/Edit/S_ettings...",                 "<control>E", do_settings,           0 },
		{ "/Edit/sep2",                         NULL,         NULL,                  0, "<Separator>" },
		{ "/Edit/Select _All...",               "<control>A", do_select_all,         0 },
		{ "/Edit/_Delete host...",              "<control>D", do_delete_page_object, 0 },
		{ "/_Viewspace",                        NULL,         NULL,                  0, "<Branch>" },
		{ "/Viewspace/tearoff1",                NULL,         NULL,                  0, "<Tearoff>" },
		{ "/Viewspace/_New...",                 "<control>N", do_page,               0 },
		{ "/Viewspace/Rename",                  NULL,         do_page,               2 },
		{ "/Viewspace/sep0",                    NULL,         NULL,                  0, "<Separator>" },
		{ "/Viewspace/_Remove",                 NULL,         do_page,               1 },
		{ "/Viewspace/sep1",                    NULL,         NULL,                  0, "<Separator>" },
		{ "/Viewspace/Add _Host...",            "<control>H", do_network,            0 },
		{ "/Viewspace/Add N_etwork...",         "<control>N", do_network,            1 },
		{ "/Viewspace/Add Network R_ange...",   "<control>R", do_network,            2 },
		{ "/Viewspace/sep2",                    NULL,         NULL,                  0, "<Separator>" },
		{ "/Viewspace/Map",                     NULL,         NULL,                  0, "<Branch>" },
		{ "/Viewspace/Map/_Map Everything",     "<control>M", do_map,                0 },
		{ "/Viewspace/Map/ReMap Everything",    NULL,         do_map,                1 },
		{ "/Viewspace/Map/UnMap Everything",    "<control>U", do_unmap,              0 },
		{ "/Viewspace/sep2",                    NULL,         NULL,                  0, "<Separator>" },
		{ "/Viewspace/Zoom/",                   NULL,         NULL,                  0 },
		{ "/Viewspace/Zoom/Normal",             NULL,         do_zoom,               0 },
		{ "/Viewspace/Zoom/In",                 NULL,         do_zoom,               1 },
		{ "/Viewspace/Zoom/Out",                NULL,         do_zoom,               2 },
		{ "/_Help",                             NULL,         NULL,                  0, "<LastBranch>" },
		{ "/Help/About...",                     NULL,         do_about,              0 },
		{ "/Help/License...",                   NULL,         do_license,            0 },
	};
	
	int nmenu_items = sizeof(menu_items)/sizeof(menu_items[0]);
	char buf[256];
	FILE *fp;
			
	w->net_pages = NULL;
	w->filename = NULL;
	
	w->window = gnome_app_new(CHEOPS_TITLE, CHEOPS_TITLE);
	gtk_widget_set_usize(w->window, CHEOPS_MIN_WIDTH, CHEOPS_MIN_HEIGHT);
	gtk_signal_connect(GTK_OBJECT(w->window),"delete-event", GTK_SIGNAL_FUNC(do_quit),NULL);
	gtk_window_set_position(GTK_WINDOW(w->window), GTK_WIN_POS_CENTER);
		
	w->accel_group = gtk_accel_group_new();
	
	w->itemf = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<blah>",w->accel_group);
	gtk_item_factory_create_items(w->itemf, nmenu_items, menu_items, NULL);
	w->menu = gtk_item_factory_get_widget(w->itemf, "<blah>");
	
	gtk_accel_group_attach(w->accel_group, GTK_OBJECT(w->window));
	
	w->vbox = gtk_vbox_new(FALSE,0);
	w->notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w->notebook), GTK_POS_TOP);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(w->notebook), TRUE);

	gnome_app_set_contents(GNOME_APP(w->window), w->vbox);
	gtk_box_pack_start(GTK_BOX(w->vbox), w->menu, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(w->vbox), w->notebook, TRUE, TRUE, 0);

	gtk_widget_show(w->notebook);
	gtk_widget_show(w->vbox);
	gtk_widget_show(w->menu);
	gtk_widget_show(w->window);

	init_net_page_stuff();

/* lets open the default ~/.gnome/cheops-ng.map file to read the config */
	sprintf(buf,"%s/%s",getenv("HOME"), CHEOPS_DEFAULT_FILENAME_NOT_HOME);

	if(NULL == (fp = fopen(buf,"r")))
	{
		w->filename = NULL;
		make_viewspace("My Network Stuff", NULL, 0);
	}
	else
	{
		fclose(fp);
		w->filename = makestring(buf);
	}
	read_config_file(NULL,NULL);
	apply_all_settings();
}

int main(int argc, char *argv[])
{
	gnome_init(CHEOPS_TITLE, CHEOPS_VERSION ,argc,argv);

	if (!getenv("HOME"))
	{
		fprintf(stderr, "No home directory!\n");
		return(1);
	}

	register_gui_handlers();

#ifdef USING_MONITORING	
	gui_monitoring_init();
#endif

	if( (main_window = NEW(cheops_window)) == NULL)
	{
		quit();
	}
	make_main_window(main_window);

	gnome_dns_init(2); //use 2 dns servers
	gtk_main();
	return(0);
}

