/*
 * Cheops Next Generation GUI
 * 
 * gui-viewspace.c
 * All functions that consern the viewspace
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
#include "logger.h"
#include "gui-sched.h"
#include "cheops-gui.h"
#include "gui-viewspace.h"
#include "gui-canvas.h"
#include "gui-config.h"
#include "gui-utils.h"
#include "gui-handlers.h"
#include "ip_utils.h"

GtkNotebookPage *page_ptr;

static int num_pages = 0;

int is_valid_net_page(net_page *np)
{
	net_page *n;
	
	for(n = main_window->net_pages; n; n = n->next)
	{
		if(np == n)
			return(TRUE);
	}
		
	return(FALSE);
}

net_page *get_current_net_page(void)
{
	GtkWidget *page = NULL;
	net_page *np;
	page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(main_window->notebook),
			(gint)gtk_notebook_get_current_page( GTK_NOTEBOOK(main_window->notebook) ) );
	if(page)
		if( (np = (void *)gtk_object_get_user_data(GTK_OBJECT(page))) != NULL )
			return((net_page *)np);
	return(NULL);
}

void net_page_remove(net_page *np)
{
	gint page;
	net_page *n, *prev = NULL;
	page_object *po, *next = NULL;

	if(main_window->notebook != NULL && np != NULL)
	{
		page = gtk_notebook_current_page(GTK_NOTEBOOK(main_window->notebook));
		gtk_notebook_remove_page(GTK_NOTEBOOK(main_window->notebook),page);
		gtk_widget_draw(GTK_WIDGET(main_window->notebook),NULL);
		for(n = main_window->net_pages; n; n = n->next)
		{
			if(n == np)
			{
				for(po = np->page_objects; po; po = next)
				{
					next = po->next;
					page_object_delete(np, po, 1); // force delete
				}
				
				free(np->name);
				if(prev == NULL)
					main_window->net_pages = n->next;
				else
					prev->next = n->next;
				free(n);
				num_pages--;
				return;
			}
			prev = n;
		}
	}
	else
	{
		fprintf(stderr,"remove_viewspace: Null notebook\n");
	}
}

void remove_viewspace(GtkWidget *w, gpointer p)
{
	net_page *np = get_current_net_page();

	net_page_remove(np);	
}

void net_page_remove_all(void)
{
	net_page *np;
	
	for(np = main_window->net_pages; np; np = main_window->net_pages)
	{
		net_page_remove(np);
	}
}

int set_the_page(void *ptr)
{
	if( (num_pages - 1) >= 0 )
		gtk_notebook_set_page(GTK_NOTEBOOK(main_window->notebook),num_pages - 1);
	return(0);
}

void add_viewspace(GtkWidget *w, char *name)
{
	if (strlen(name))
	{
		make_viewspace(name, NULL, 0);
	}

	while(gtk_events_pending())
		gtk_main_iteration();

	cheops_sched_add(30,set_the_page,NULL);
}

int start_ipv4_agent(net_page *np, char *hostname, int usessl)
{
	if ((np->agent = event_request_agent(AGENT_TYPE_IPV4, hostname, usessl)))
	{
		printf("Connected to ipv4 agent.\n\n");
		if(np->agent_ip)
			free(np->agent_ip);
		np->agent_ip = makestring(hostname);
		return(1);
	} 
	else 
	{
		clog(LOG_ERROR, "Unable to connect to local agent\n");
		return(0);
	}
}

void connect_to_ipv4_agent(net_page *np, char *host, int usessl)
{
	if (strlen(host))
		np->connected = start_ipv4_agent(np, host, usessl);

	if(!np->connected)
	{
		char *message = "I could not connect to the Specified IP agent at: ";
		char *buffer = malloc(strlen(message) + strlen(host) + 1);
		sprintf(buffer, "%s%s", message, host);
		make_gnome_ok_dialog("Could not connect to the cheops-agent", buffer);
		free(buffer);
	}
}

int net_page_rename(net_page *np, char *name)
{
	if(np && is_valid_net_page(np))
	{
		if(np->label && name)
		{
			if(np->name)
				free(np->name);
			np->name = strdup(name);
			gtk_label_set_text(GTK_LABEL(np->label), name);
			return(1);
		}
	}
	return(0);
}

net_page *make_viewspace(char *c, char *agent_ip, int usessl)
{
	net_page *np, *npp, *prev;
	char *buf;
#if defined(HAS_SSL) && defined(USING_SSL)
	GtkWidget *checkbox;
#endif
	GtkWidget *dialog;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *entry1;


	for(np = main_window->net_pages; np; np = np->next)
		if(!strcmp(np->name, c) && (!agent_ip || strcmp(agent_ip, np->agent_ip)) )
			return(np);
	
	

	if((np = malloc(sizeof(net_page))) == NULL)
	{
		exit(0);
	}
	memset(np, 0, sizeof(*np));
	if((buf = strdup(c)) == NULL)
	{
		exit(0);
	}

/* add to end to make config file restore look like what you have before saving (adding to
 * the beginning of the list made the restore put in the tabs in reverse order) */
	prev = main_window->net_pages;
	for(npp = main_window->net_pages; npp; npp = npp->next)
		prev = npp;
	if(prev == NULL)
		main_window->net_pages = np;
	else
		prev->next = np;
	np->next = NULL;
	
	np->page_objects = NULL;
	np->plink = NULL;
	np->label = gtk_label_new(buf);
	np->button_state = 0;
	np->select_rectangle=NULL;
	np->pixels_per_unit = 1.0;
	np->name = makestring(c);
			
	gtk_widget_push_visual(gdk_imlib_get_visual());
	gtk_widget_push_colormap(gdk_imlib_get_colormap());
	np->canvas = gnome_canvas_new();
	gtk_widget_pop_visual();
	gtk_widget_pop_colormap();
	
	gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(np->canvas), 1.0);
	gtk_widget_set_usize(np->canvas, CANVAS_FIXED_WIDTH, CANVAS_FIXED_HEIGHT);
	gnome_canvas_set_scroll_region(GNOME_CANVAS(np->canvas), 0.0, 0.0, (double)CANVAS_FIXED_WIDTH, (double)CANVAS_FIXED_HEIGHT);
	gtk_signal_connect(GTK_OBJECT(np->canvas), "event", GTK_SIGNAL_FUNC(net_page_event_dispatch), np);
	
	np->scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(np->scrolled_window),
	                               GTK_POLICY_AUTOMATIC,
	                               GTK_POLICY_AUTOMATIC);

	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(np->scrolled_window), np->canvas);
	
	np->hbox = gtk_hbox_new(FALSE, 5);
	gtk_object_set_user_data(GTK_OBJECT(np->hbox),np);
	gtk_box_pack_start(GTK_BOX(np->hbox), np->scrolled_window, TRUE, TRUE, 0);

	gtk_notebook_append_page(GTK_NOTEBOOK(main_window->notebook),np->hbox,np->label);

	gtk_widget_show(np->label);
	gtk_widget_show(np->scrolled_window);
	gtk_widget_show(np->canvas);
	gtk_widget_show(np->hbox);

	num_pages++;

	while(gtk_events_pending())
		gtk_main_iteration();

	if(agent_ip == NULL || *agent_ip == '\0')
	{
		/*
		 * Ok well it looks like we dont have the first netpage and we are
		 * trying to make a page, and connect to a agent, so lets try the
		 * most probable agents...
		 */
		if((np->agent = event_request_agent(AGENT_TYPE_LOCAL, NULL, usessl)))
		{	
			printf("Connected to local agent.\n\n");
			register_gui_handlers();
			np->connected = 1;
		}
	}
	else
	{
		np->connected = start_ipv4_agent(np, agent_ip, usessl);
	}
	
	while(!np->connected)
	{
		/* do something to give them a prompt to loginto a netaork agent */
		dialog = gnome_dialog_new("Connect to Agent",
						  GNOME_STOCK_BUTTON_OK,
						  GNOME_STOCK_BUTTON_CANCEL,
						  NULL);
		gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
		
		entry1 = gnome_entry_new("");
		label = gtk_label_new("Agent hostname:");
		hbox = gtk_hbox_new(FALSE, 5);
		gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
		
		gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 5);
		gtk_box_pack_start( GTK_BOX(hbox), entry1, FALSE, FALSE, 5);
		gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

#if defined(HAS_SSL) && defined(USING_SSL)
		checkbox = gtk_check_button_new_with_label("Use SSL");
		gtk_signal_connect(GTK_OBJECT(checkbox), "toggled", toggle_dialog_toggle, &usessl);
		gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), checkbox, TRUE, TRUE, 0);
		gtk_widget_show( checkbox );
#endif

		gtk_widget_grab_focus(GTK_COMBO(entry1)->entry);

		gtk_signal_connect(GTK_OBJECT(GTK_COMBO(entry1)->entry),  "activate", GTK_SIGNAL_FUNC(click_ok_on_gnome_dialog), dialog);
			
		gtk_widget_show( hbox );
		gtk_widget_show( label );
		gtk_widget_show( entry1 );

		switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
		{
			case 0:
				connect_to_ipv4_agent(np, gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(entry1)))), usessl);
				gnome_dialog_close(GNOME_DIALOG(dialog));
				break;
			case 1:	
				gnome_dialog_close(GNOME_DIALOG(dialog));
				exit(0);
			default:
				break;
		}
	}
		
	return(np);
}

void add_network(GtkWidget *w, char *net, char *mask)
{
	char *c;

	if(mask == NULL)
	{
		c = makestring( net );
	}
	else
	{
		c = malloc(strlen(net) + strlen(mask) + 1 + 1);
		strcpy(c, net);
		strcat(c,"/");
		strcat(c,mask);
	}
	
	if (strlen(c))
	{
		net_page *np = get_current_net_page();
		do_discover(np->agent, c, (void *)np,0);	
	}
	free(c);
}

void add_network_range(char *first, char *last)
{
	if(first && last &&strlen(first) && strlen(last))
	{
		net_page *np = get_current_net_page();
		do_discover_range(np->agent, first, last, (void *)np,0);	
	}
}

page_object *add_host_entry_to_net_page(net_page *np, int ip, char *name)
{
	page_object *po;
	char *c;
	net_page *pnp;
	
	for(pnp = main_window->net_pages; pnp; pnp = pnp->next)
		if(pnp == np)	
			break;

	if(pnp == NULL)
		return(NULL);

	/* see if it is already there */
	for(po = np->page_objects; po; po = po->next)
	{
		if(po->ip == ip)
		{
			if(name)
			{
				if(po->name)
					free(po->name);
				po->name = makestring(name);
			}
			return(po);
		}
	}

	po = page_object_new(GNOME_CANVAS(np->canvas));

	po->ip = ip;	
	if(name)
		c = name;
	else
		c = ip2str(ip);
		
	if((po->name = malloc(strlen(c)+1)) == NULL)
	{
		exit(0);
	}
	strcpy(po->name,c);
		
	po->next = np->page_objects;
	np->page_objects = po;

	page_object_display(po);
	return(po);
}

page_object *add_host_entry(int ip, char *name)
{
	net_page *np;
	
	while( (np = get_current_net_page()) == NULL )
	{
		do_page(NULL, 0); // add a viewspace and go back
	}	
	return( add_host_entry_to_net_page(np, ip, name) );
}


