/*
 * Cheops Next Generation GUI
 * 
 * gui-io.c
 * I/O Managment (for the gui)
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
#include <gtk/gtk.h>
#include "gui-io.h"
#include "logger.h"

struct gtk_cheops_list {
	struct gtk_cheops_list *next;
	int fd;
	cheops_io_cb callback;
	short events;
	void *data;
	void *id;
};

static struct gtk_cheops_list *list = NULL;

static void gtk_cheops_io_std_callback(gpointer data,gint source, GdkInputCondition condition)
{
	struct gtk_cheops_list *p;
	
	for(p=list;p;p=p->next)
		if(p == data)
			if(p->callback)
				(p->callback)(p->id,p->fd,p->events,p->data);
}

int *cheops_io_add(int fd, cheops_io_cb callback, short events, void *data)
{
	struct gtk_cheops_list *l;
	
	if(NULL == (l=malloc(sizeof(struct gtk_cheops_list))))
	{
		clog(LOG_ERROR,"NULL returned from a malloc");
	}
	l->fd = fd;
	l->callback = callback;
	l->events = events;
	l->data = data;
	l->id = (void *)gdk_input_add(fd,events,gtk_cheops_io_std_callback,l);
	l->next = list;
	list = l;
	
	return(l->id);
}

int cheops_io_remove(int *id)
{
	struct gtk_cheops_list *l,*prev=NULL;
	int ret = -1;
		
	for(l=list;l;l=l->next)
	{
		if(id == l->id)
		{
			if(prev == NULL) /* beginning of list */
			{
				list = l->next;
			}
			else
			{
				prev->next = l->next;
			}
			gdk_input_remove((int)id);
			free(l);
			ret=0;
			break;
		}
		prev = l;
	}
	return(ret);
}
