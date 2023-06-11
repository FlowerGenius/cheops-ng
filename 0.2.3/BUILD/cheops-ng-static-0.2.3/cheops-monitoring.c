/*
 * Cheops Next Generation GUI
 * 
 * cheops-monitoring.c
 * initial monitoring support
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
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cheops-monitoring.h"

#define MONITORING_DEBUG

#ifdef MONITORING_DEBUG
	#define DEBUG(a) a
#else
	#define DEBUG(a) 
#endif

static pthread_mutex_t monitoring_list_mutex = PTHREAD_MUTEX_INITIALIZER;
static GList *toplevel_list = NULL;

monitoring_node *monitoring_node_find_toplevel(unsigned int id)
{
	GList *gl;
	monitoring_node *m;

	pthread_mutex_lock(&monitoring_list_mutex);	
	for(gl = toplevel_list; gl; gl = g_list_next(gl))
	{
		m = gl->data;
		if(m && m->id == id)
		{
			pthread_mutex_unlock(&monitoring_list_mutex);	
			return(m);
		}
	}
	pthread_mutex_unlock(&monitoring_list_mutex);	
	return(NULL);
}

monitoring_node *monitoring_node_find(monitoring_node *parent, unsigned int child_id)
{
	GList *gl;
	monitoring_node *m;

	pthread_mutex_lock(&monitoring_list_mutex);	
	for(gl = parent->children; gl; gl = g_list_next(gl))
	{
		m = gl->data;
		if(m && m->id == child_id)
		{
			pthread_mutex_unlock(&monitoring_list_mutex);	
			return(m);
		}
	}
	pthread_mutex_unlock(&monitoring_list_mutex);	
	return(NULL);
}

monitoring_node *monitoring_node_find_by_name_toplevel(char *child_name)
{
	GList *gl;
	monitoring_node *m;

	pthread_mutex_lock(&monitoring_list_mutex);	
	for(gl = toplevel_list; gl; gl = g_list_next(gl))
	{
		m = gl->data;
		if(m && 0 == strcmp(m->name, child_name))
		{
			pthread_mutex_unlock(&monitoring_list_mutex);	
			return(m);
		}
	}
	pthread_mutex_unlock(&monitoring_list_mutex);	
	return(NULL);
}

monitoring_node *monitoring_node_find_by_name(monitoring_node *parent, char *child_name)
{
	GList *gl;
	monitoring_node *m;

	pthread_mutex_lock(&monitoring_list_mutex);	
	for(gl = parent->children; gl; gl = g_list_next(gl))
	{
		m = gl->data;
		if(m && 0 == strcmp(m->name, child_name))
		{
			pthread_mutex_unlock(&monitoring_list_mutex);	
			return(m);
		}
	}
	pthread_mutex_unlock(&monitoring_list_mutex);	
	return(NULL);
}


monitoring_node *monitoring_node_new(char *name, unsigned int id, monitoring_callback cb)
{
	monitoring_node *m;
	
	m = g_malloc(sizeof(*m));
	if(m)
	{
		memset(m, 0, sizeof(*m));
		if(name)
			m->name = g_strdup(name);
		
		m->id = id;
		m->callback = cb;
	}
	
	return(m);
}

/*
 * 0 good, 1 bad
 */
int monitoring_node_add_toplevel(monitoring_node *m)
{
	char buffer[100];
	if(m)
	{
		if(monitoring_node_find_toplevel(m->id))
		{
			DEBUG(printf("%s(): monitoring node id=%d already in list\n", __FUNCTION__, m->id));
			return(1);
		}
		sprintf(buffer, "%d", m->id);
		if(m->path)
			g_free(m->path);
		m->path = g_strdup(buffer);
		
		toplevel_list = g_list_append(toplevel_list, m);
		
		return(0);
	}
	return(1);
}

/*
 * 0 good, 1 bad
 */
int monitoring_node_add(monitoring_node *parent, monitoring_node *child)
{
	char buffer[500];
	if(parent && child)
	{
		if(monitoring_node_find(parent, child->id))
		{
			DEBUG(printf("%s(): monitoring node (%d) already in parent's (%d) list\n", __FUNCTION__, child->id, parent->id));
			return(1);
		}
		sprintf(buffer, "%s.%d", parent->path, child->id);
		if(child->path)
			g_free(child->path);
		child->path = g_strdup(buffer);
		
		parent->children = g_list_append(parent->children, child);
		
		return(0);
	}
	return(1);
}

unsigned int monitoring_session_run(monitoring_session *session)
{
	monitoring_node *m = NULL;
	char *strtok_context = NULL;
	char *it;
	char *temp;
	int value;
	
	if(session)
	{
		session->state = MONITOR_STATE_OK;
		
		/*
		 * lets make another string for strtok_r to trash
		 */
		temp = g_strdup(session->id);
		if(temp == NULL)
		{
			pthread_mutex_lock(&monitoring_list_mutex);	
			if(session->error)
				g_free(session->error);
			session->error = g_strdup("ran out of memory"); // how ironic... 
			session->state = MONITOR_STATE_ERROR;
			pthread_mutex_unlock(&monitoring_list_mutex);	

			return(MONITOR_STATE_ERROR);
		}
		/*
		 * go through all of the dotted decimal strings and do the callbacks
		 * for the monitoring nodes that correspond to the numbers
		 */
		while( (it = strtok_r((m ? NULL : temp), ".", &strtok_context)) )
		{
			printf("working on %s\n", it);
			value = atoi(it);
			if(m)
			{
				/*
				 * ok we have already got the parent node to this one
				 * lets try to find it in the parent node
				 */
				if( (m = monitoring_node_find(m, value)) )
				{
					m->callback(session, m);
				}
				else
				{
					pthread_mutex_lock(&monitoring_list_mutex);	
					if(session->error)
						g_free(session->error);
					session->error = g_strdup("monitoring node not found");
					session->state = MONITOR_STATE_ERROR;					
					pthread_mutex_unlock(&monitoring_list_mutex);	
					
					g_free(temp);
					return(MONITOR_STATE_ERROR);
				}
			}
			else
			{
				/*
				 * lets get the toplevel node out and run it
				 */
				if( (m = monitoring_node_find_toplevel(value)) )
				{
					m->callback(session, m);
				}
				else
				{
					pthread_mutex_lock(&monitoring_list_mutex);	
					if(session->error)
						g_free(session->error);
					session->error = g_strdup("toplevel monitoring node not found");
					session->state = MONITOR_STATE_ERROR;					
					pthread_mutex_unlock(&monitoring_list_mutex);	
					
					g_free(temp);
					return(MONITOR_STATE_ERROR);
				}
			}
		}
		g_free(temp);
		return(session->state);
	}
	return(MONITOR_STATE_ERROR);
}


monitoring_session *monitoring_session_new(char *id)
{
	monitoring_session *m = g_malloc(sizeof(*m));
	if(m)
	{
		memset(m, 0, sizeof(*m));
		m->id = g_strdup(id ? id : "");
	}
	return(m);
}

void monitoring_session_destroy(monitoring_session *mon)
{
	if(mon)
	{
		if(mon->id)
			g_free(mon->id);
		
		g_free(mon);
	}
}

void monitoring_list_foreach_toplevel(void (*func)(monitoring_node *node, void *arg), void *arg)
{
	g_list_foreach(toplevel_list, (GFunc)func, arg);
}

void monitoring_list_foreach(monitoring_node *node, void (*func)(monitoring_node *node, void *arg), void *arg)
{
	g_list_foreach(node->children, (GFunc)func, arg);
}

