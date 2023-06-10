/*
 * Cheops Next Generation GUI
 * 
 * gui-pixmap.c
 * Functions to handle pixmap stuff for the gui
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
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include "event.h"
#include "logger.h"
#include "script.h"

static GList *script_list = NULL;

void script_default(void)
{
	char **c;
	char *default_list[] = {
		"traceroute", "xterm -hold -e /usr/sbin/traceroute %i", (char *)(SCRIPT_FLAGS_VIEWABLE),
		"ping",       "xterm -hold -e ping %i", (char *)(SCRIPT_FLAGS_VIEWABLE),
		NULL
	};
	
	g_list_foreach(script_list, (GFunc)script_remove_ptr, NULL);
	g_list_free(script_list);
	script_list = NULL;
	
	for(c = &default_list[0]; c[0]; c += 3)
		script_add(*c, *(c + 1), (unsigned int)*(c + 2));
}	

script_t *script_add(char *name, char *script, unsigned int flags)
{
	script_t *s;
	
	s = script_get(name);
	script_remove_ptr(s);
	
	s = malloc(sizeof(script_t));
	if(!s)
	{
		clog(LOG_ERROR," we ran out of memory?");
		exit(1);
	}
	memset(s, 0, sizeof(*s));
	s->name = g_strdup(name);
	s->script = g_strdup(script);
	s->flags = flags;
	
	script_list = g_list_append(script_list, s);
	
	return(s);
}

script_t *script_change(script_t *scr, char *name, char *script, unsigned int flags)
{
	script_t *s;
	GList *gl;
	
	gl = g_list_find(script_list, scr);
	if(gl)
	{
		s = (script_t *)gl->data;
	
		script_remove_ptr(s);
	
		return(script_add(name, script, flags));
	}
		
	return(NULL);
}

void script_remove_ptr(script_t *s)
{
	GList *gl;
	
	gl = g_list_find(script_list, s);
	if(gl)
	{
		g_free(s->name);
		g_free(s->script);
		script_list = g_list_remove(script_list, s);
	}
}

void script_remove(char *name)
{
	script_t *s;
	
	if((s = script_get(name)))
		script_remove_ptr(s);
}

script_t *script_get(char *name)
{
	GList *gl;
	for(gl = script_list; gl; gl = g_list_next(gl))
	{
		if(gl->data && 0 == strcmp(((script_t *)gl->data)->name, name) )
			return((script_t *)gl->data);
	}
	
	return(NULL);
}

void script_foreach(GFunc func, void *user_data)
{
	g_list_foreach(script_list, func, user_data);
}

GList *script_get_list(void)
{
	return(script_list);
}
