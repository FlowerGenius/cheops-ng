/*
 * Cheops Next Generation GUI
 * 
 * gui-service.c
 * Functions for the service callback stuff (spawning a program to handle
 * the service ie: ftp telnet...)
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

#include <stdlib.h>
#include <gnome.h>
#include <pthread.h>
#include "cheops-gui.h"
#include "gui-service.h"
#include "logger.h"
#include "gui-utils.h"
#include "ip_utils.h"
#include "cheops-osscan.h"

service_list_t *service_list;

typedef struct _thread_list_t {
	pthread_t              thread;
	char                  *command;
	struct _thread_list_t *next;
} thread_list_t;

thread_list_t *thread_list = NULL;
pthread_mutex_t mutex;
char mutex_initalized = FALSE;

service_list_t *service_list_add(char *name, int port, int protocol, char *string)
{
	service_list_t *p;
	
	/*
	 * this will convert the old way of saving off the protocol number
	 * to the new enum
	 */
	if(protocol == 6)
		protocol = PORT_PROTOCOL_TCP;
	else if(protocol == 17)
		protocol = PORT_PROTOCOL_UDP;
		
	for(p = service_list; p; p = p->next)
	{
		if( port == p->port && protocol == p->protocol )
		{
			if(p->command)
				free(p->command);
			if(p->name)
				free(p->name);
			
			p->name = makestring(name);
			p->command = makestring(string);
			return(p);
		}
	}
	// if it was not there then just add one
	
	p = malloc(sizeof(service_list_t));
	if(!p)
	{
		clog(LOG_ERROR," we ran out of memory?");
		exit(1);
	}
	p->name = makestring(name);
	p->command = makestring(string);
	p->port = port;
	p->protocol = protocol;
	p->next = service_list;
	service_list = p;
	
	return(p);
}

service_list_t *service_list_change(service_list_t *service, char *name, int port, int protocol, char *string)
{
	service_list_t *p, *ret;
	
	for(p = service_list; p; p = p->next)
	{
		if( service == p )
		{
			if(p->command)
				free(p->command);
			if(p->name)
				free(p->name);
			
			p->name = makestring(name);
			p->command = makestring(string);
			p->port = port;
			p->protocol = protocol;
			
			ret = p;
		     	for(p = service_list; p; p = p->next)
		     	{
		     		if( p != ret &&
		     		    p->port == port &&
		     		    p->protocol == protocol )
		     		{
		     			service_list_remove_ptr(ret);
		     			return(NULL);
		     		}
			}
			return(ret);
		}
	}
	// uh something bad happened?!?
	return(NULL);
}

void service_list_default(void)
{
	service_list_t *p, *next;
	char **c;
	char *default_list[] = {
		"ftp",    (char *)21 ,  (char *)PORT_PROTOCOL_TCP, "xterm -e ftp %i",
		"ssh",    (char *)22 ,  (char *)PORT_PROTOCOL_TCP, "xterm -e ssh %i -l %u",
		"telnet", (char *)23 ,  (char *)PORT_PROTOCOL_TCP, "xterm -e telnet %i",
		"http",   (char *)80 ,  (char *)PORT_PROTOCOL_TCP, "netscape %i",
		"javaVNC:0",  (char *)5800, (char *)PORT_PROTOCOL_TCP, "netscape %i:%p",
		"VNC:0",  (char *)5900, (char *)PORT_PROTOCOL_TCP, "xterm -e vncviewer %i:%p",
		NULL
	};
	
	if(service_list)
	{
		for(p = service_list; p;)
		{
			service_list = p->next;
					
			free(p->command);
			next = p->next;
			free(p);
			p = next;
		}
	}
	
	for(c = &default_list[0]; c[0];c+=4)
		service_list_add( *(c), (int)*(c+1), (int)*(c+2), *(c+3));
}	

void service_list_remove(int port, int protocol)
{
	service_list_t *p, *prev = NULL;
	
	for(p = service_list; p; p = p->next)
	{
		if( port == p->port && protocol && p->protocol )
		{
			if(prev)
				prev->next = p->next;
			else
				service_list = p->next;
				
			
			if(p->command)
				free(p->command);
			if(p->name)
				free(p->name);
			free(p);
			break;
		}	
		prev = p;
	}
}

void service_list_remove_ptr(service_list_t *t)
{
	service_list_t *p, *prev = NULL;
	
	for(p = service_list; p; p = p->next)
	{
		if( p == t )
		{
			if(prev)
				prev->next = p->next;
			else
				service_list = p->next;
				
			
			if(p->command)
				free(p->command);
			if(p->name)
				free(p->name);
			free(p);
			break;
		}	
		prev = p;
	}
}

char *service_list_get(int port, int protocol)
{
	service_list_t *p;

	if(service_list == NULL)
		service_list_default(); //initalize it
	
	for(p = service_list; p; p = p->next)
	{
		if(port == p->port && protocol == p->protocol)
			return(p->command);
	}
	return(NULL);
}

void *run_service(void *arg)
{
	pthread_t pth;
	thread_list_t *threadp = arg;
	thread_list_t *p, *prev;
	
	if(threadp->command)
		system(threadp->command);

	pth = pthread_self();
	
	if(pth != threadp->thread)
		printf("there is a thread problem\n");

	pthread_mutex_lock(&mutex);
	prev = NULL;
	for(p = thread_list; p; p = p->next)
	{
		if(p->thread == pth)
		{
			if(prev)
				prev->next = p->next;
			else
				thread_list = p->next;
			if(p->command)
				free(p->command);
			free(p);
			break;
		}
		prev = p;
	}
	pthread_mutex_unlock(&mutex);
			
	return(NULL);
}

char *make_username_dialog(void)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *label;
	GtkWidget *hbox;
	char *text;
	
	dialog = gnome_dialog_new("Enter a username",
	                          GNOME_STOCK_BUTTON_OK,
	                          GNOME_STOCK_BUTTON_CANCEL,
	                          NULL);
	
	entry = gnome_entry_new("username_entry");
	label = gtk_label_new("User Name:");
	hbox = gtk_hbox_new(FALSE, 5);
	
	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
	
	gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_box_pack_start( GTK_BOX(hbox), entry, FALSE, FALSE, 5);
	gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

	gtk_widget_grab_focus(GTK_COMBO(entry)->entry);

	gnome_dialog_editable_enters(GNOME_DIALOG(dialog), GTK_EDITABLE(GTK_COMBO(entry)->entry));
	
	gtk_widget_show( hbox );
	gtk_widget_show( label );
	gtk_widget_show( entry );

	switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
	{
		case 0:
			text = strdup(gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(entry)))));
			gnome_dialog_close(GNOME_DIALOG(dialog));
			break;
		case 1:	
			gnome_dialog_close(GNOME_DIALOG(dialog));
			text = NULL;
			break;
		default:
			text = NULL;
			break;
	}
	
	return(text);
}

char *make_password_dialog(void)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *label;
	GtkWidget *hbox;
	char *text;
	
	dialog = gnome_dialog_new("Enter a password",
					  GNOME_STOCK_BUTTON_OK,
					  GNOME_STOCK_BUTTON_CANCEL,
					  NULL);
	
	entry = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
	
	label = gtk_label_new("Password:");
	hbox = gtk_hbox_new(FALSE, 5);
	
	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
	
	gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 5);
	gtk_box_pack_start( GTK_BOX(hbox), entry, FALSE, FALSE, 5);
	gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

	gtk_widget_grab_focus(entry);

	gnome_dialog_editable_enters(GNOME_DIALOG(dialog), GTK_EDITABLE(entry));
	
	gtk_widget_show( hbox );
	gtk_widget_show( label );
	gtk_widget_show( entry );

	switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
	{
		case 0:
			text = strdup( gtk_entry_get_text(GTK_ENTRY(entry)) );
			gnome_dialog_close(GNOME_DIALOG(dialog));
			break;
		case 1:	
			gnome_dialog_close(GNOME_DIALOG(dialog));
			text = NULL;
			break;
		default:
			text = NULL;
			break;
	}
	
	return(text);
}


int service_callback(page_object *po, int port, int protocol)
{
	int ddone = 0;
	char *string;
		
	while(!ddone)
	{	
		if( (string = service_list_get(port, protocol)) )
		{
			ddone = 1;

			run_command_callback(po, port, protocol, string);

#if 0			
			b = string;
			buf[0] = '\0';
			
			while(!done)
			{
				if( (c = strstr(b, "%")) )
				{
					switch(*(c + 1))
					{
						case 'i':
							if(c - b)
			   				{
			   					len = strlen(buf);
			   					strncpy(buf + len, b, c - b);
			   					buf[len + c - b] = '\0';
			   				}

			   				strcat(buf, ip2str(ip));
			   				b = c+2;
			   				break;

						case 'p':
							if(c - b)
			   				{
			   					len = strlen(buf);
			   					strncpy(buf + len, b, c - b);
			   					buf[len + c - b] = '\0';
			   				}

			   				sprintf(buf + strlen(buf),"%d", port);
			   				b = c+2;
			   				break;

						case 'u':
							if(c - b)
			   				{
			   					len = strlen(buf);
			   					strncpy(buf + len, b, c - b);
			   					buf[len + c - b] = '\0';
			   				}

			   				str = make_username_dialog();
			   				if(str)
			   				{
			   					strcat(buf, str);
			   					free(str);
			   				}
			   				b = c+2;
			   				break;

						case 'P':
							if(c - b)
			   				{
			   					len = strlen(buf);
			   					strncpy(buf + len, b, c - b);
			   					buf[len + c - b] = '\0';
			   				}

			   				str = make_password_dialog();
			   				if(str)
			   				{
			   					strcat(buf, str);
			   					free(str);
			   				}
			   				b = c+2;
			   				break;

			   			default:
							if(c - b)
			   				{
			   					len = strlen(buf);
			   					strncpy(buf + len, b, c - b + 1);
			   					buf[len + c - b + 1] = '\0';
			   				}

			   				b = c + 1;
			   				break;
					}
				}
				else
				{
					strcat(buf, b);
					done = 1;
				}
			}

			if(!mutex_initalized)
				pthread_mutex_init( &mutex, NULL);

			pthread_mutex_lock(&mutex);
			threadp = malloc(sizeof(thread_list_t));
			threadp->next = thread_list;
			thread_list = threadp;
			threadp->command = malloc(strlen(buf) + 1);
			strcpy(threadp->command, buf);
			pthread_mutex_unlock(&mutex);

			pthread_create(&threadp->thread, NULL, run_service, threadp);
#endif
		}
		else
		{
			
			if(NULL == services_make_dialog(NULL, get_service(port, protocol), port, protocol, "", main_window->window))
				ddone = 1;
			else
				ddone = 0;
		}
	}
	return(1);
}

int run_command_callback(page_object *po, int port, int protocol, char *string)
{
	char *b, *c;
	int done = 0;
	thread_list_t *threadp;
	char buf[1024];
	char *str;
	int len;
	

	b = string;
	buf[0] = '\0';
	
	while(!done)
	{
		if( (c = strstr(b, "%")) )
		{
			switch(*(c + 1))
			{
				case 'i':
					if(c - b)
	   				{
	   					len = strlen(buf);
	   					strncpy(buf + len, b, c - b);
	   					buf[len + c - b] = '\0';
	   				}

	   				strcat(buf, ip2str(po->ip));
	   				b = c+2;
	   				break;

				case 'p':
					if(c - b)
	   				{
	   					len = strlen(buf);
	   					strncpy(buf + len, b, c - b);
	   					buf[len + c - b] = '\0';
	   				}

	   				sprintf(buf + strlen(buf),"%d", port);
	   				b = c+2;
	   				break;

				case 'u':
					if(c - b)
	   				{
	   					len = strlen(buf);
	   					strncpy(buf + len, b, c - b);
	   					buf[len + c - b] = '\0';
	   				}

	   				str = make_username_dialog();
	   				if(str)
	   				{
	   					strcat(buf, str);
	   					free(str);
	   				}
	   				b = c+2;
	   				break;

				case 'P':
					if(c - b)
	   				{
	   					len = strlen(buf);
	   					strncpy(buf + len, b, c - b);
	   					buf[len + c - b] = '\0';
	   				}

	   				str = make_password_dialog();
	   				if(str)
	   				{
	   					strcat(buf, str);
	   					free(str);
	   				}
	   				b = c+2;
	   				break;

	   			default:
					if(c - b)
	   				{
	   					len = strlen(buf);
	   					strncpy(buf + len, b, c - b + 1);
	   					buf[len + c - b + 1] = '\0';
	   				}

	   				b = c + 1;
	   				break;
			}
		}
		else
		{
			strcat(buf, b);
			done = 1;
		}
	}

	if(!mutex_initalized)
		pthread_mutex_init( &mutex, NULL);

	pthread_mutex_lock(&mutex);
	threadp = malloc(sizeof(thread_list_t));
	threadp->next = thread_list;
	thread_list = threadp;
	threadp->command = malloc(strlen(buf) + 1);
	strcpy(threadp->command, buf);
	pthread_mutex_unlock(&mutex);

	pthread_create(&threadp->thread, NULL, run_service, threadp);

	return(1);
}
