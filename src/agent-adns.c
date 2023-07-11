/*
 * Cheops Next Generation GUI
 * 
 * agent-adns.c
 * DNS Query code
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
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "event.h"
#include "io.h"
#include "sched.h"
#include "cheops-sh.h"
#include "logger.h"
#include "cheops-agent.h"
#include "agent-adns.h"
#include "ip_utils.h"
#include "cache.h"
#include "adns-1.0/src/adns.h"

#ifndef DEBUG_DISCOVER
	#define DEBUG
#else
	#define DEBUG(a)
#endif


#define DNS_QUE_FLAG_ALREADY_EXECUTED 1

typedef struct _dns_que {
	time_t started;
	int	flags;
	int	id;      //cheops-io id
	char *name;
	adns_query query;
	struct pollfd pollfds;
	int npollfds;
	struct _dns_que *next;
} dns_que;

dns_que *request_q = NULL;
int adns_initalized = 0;
adns_state ads;

struct pollfd *pollfds = NULL;
int npollfds = 0;


int dns_query_callback(int *id, int fd, short events, void *cbdata);
int dns_query_timeout(dns_que *request);


int handle_dns_query(event_hdr *h, event *e, agent *a)
{
	dns_que *request;
	int result;
	char *domain;
#if 0
	
	if(!adns_initalized)
	{
		result = adns_init(&ads,adns_if_noautosys,0);
		if(result)
		{
			//send dns problem
		}
	
	}
	
	request = malloc(sizeof(dns_que));
	
	request->id = 0;
	request->name = NULL;
	request->flags = 0;
	request->next = NULL;
	
	result = adns_submit(ads, domain, adns_r_a, 0, request, &request->query);
	if(result)
	{
		//send dns problem
	}
	
	printf("\ni got a dns query\n");


	result = adns_check (ads, &qu, &ans, &mcr);
	if (result != EWOULDBLOCK)
	{
		//send some error of no can do or it would be blocking
	}
	for (;;)
	{
		timeout = -1;
		result = adns_beforepoll (ads, pollfds, &npollfds, &timeout, 0);
		if (result != ERANGE)
			break;
		pollfds = realloc (pollfds, sizeof (*pollfds) * npollfds);
		if (!pollfds)
			exit(1);
	}
	if (result)
	{
		//send failure 
	}
		
	request->pollfds = pollfds;
	request->npollfds = npollfds;
	
	for(i = 0; i < npollfds; i++)
	{
		cheops_io_add(pollfds[i].fd, CHEOPS_IO_CB(dns_query_callback), pollfds[i].events, request);
	}
	cheops_sched_add(timeout, CHEOPS_SCHED_CB(dns_query_timeout), request);
	
	
#endif	
}

int dns_query_timeout(dns_que *request)
{
}

int dns_query_callback(int *id, int fd, short events, void *cbdata)
{
	int wasfound = 0;

#if 0
	if(wasfound)
	{
		eh->len = htonl(sizeof(event_hdr) + sizeof(discover_ipv4_r));
		eh->hlen = htons(sizeof(event_hdr));
		eh->type = htons(REPLY_DISCOVER_IPV4);
		eh->flags = pp->flags;
		ee->discover_ipv4_r.ipaddr = ip;
		
		if (event_send(pp->a, eh) < 0)
		{
			DEBUG( c_log(LOG_WARNING, "Unable to send discover reply\n") );
			return(0);
		}
	}
	return(1);

#endif

}


