/*
 * Cheops Next Generation GUI
 * 
 * agent-map.h
 * An agent shell, for testing and communicating directly with agents
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

#ifndef AGENT_MAP_H
#define AGENT_MAP_H

#include "glib.h"

struct map_request_node {
	unsigned int ip;
};	

struct map_request {
	/* A request by the user to map an object */
	struct net_page *np;
	unsigned int dest;
	unsigned int last;
	char ttl;
	int retries;
	time_t sent;
	GList *nodes;
	agent *agent;
	
	struct map_request *next;
};


/* Don't trace deeper than  this, to prevent infinite loops */
#define MAP_MAX	 	60

/* Don't retransmit more than this # every 100 ms */
#define MAP_TIMEOUT_MAX 5

#define MAP_REQUEST_TIMEOUT 10

#define MAX_MAP_REQUEST_RETRIES 3

#define EMPTY_PORT 33434

int handle_map_icmp_request(event_hdr *h, event *e, agent *a);


#endif /* AGENT_MAP_H */
