/*
 * Cheops Next Generation GUI
 * 
 * cheops-osscan.c
 * osscan service names stuff
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
 
#include "cheops-types.h"
#include "gui-utils.h"

typedef struct _service_cache {
	struct	_service_cache *next;
	u16		port;
	u8		protocol;
	char		*name;
} service_cache;

service_cache *services = NULL;

void add_service(u16 port, u8 protocol, char *name)
{
	service_cache *p;
	for(p = services; p; p = p->next)
	{
		if(p->port == port && p->protocol == protocol)
			return;
	}
	p = (service_cache *)malloc(sizeof(service_cache));
	p->port = port;
	p->protocol = protocol;
	p->name = makestring(name);
	p->next = services;
	services = p;
}

void remove_service(u16 port, u8 protocol)
{
	service_cache *p, *prev = NULL;
	for(p = services; p; p = p->next)
	{
		if(p->port == port && p->protocol == protocol)
		{
			if(p->name)
				free(p->name);
				
			if(prev)
				prev->next = p->next;
			else
				services = p->next;
				
			free(p);	
		}
	}
}	

void remove_all_services(void)
{
	service_cache *p;
	for(p = services; p; p = services)
	{
		services = p->next;
		if(p->name)
			free(p->name);
		free(p);
	}
}	

char *get_service(u16 port, u8 protocol)
{
	service_cache *p;
	for(p = services; p; p = p->next)
		if(p->port == port && p->protocol == protocol)
			return(p->name);
	return(NULL);
}
