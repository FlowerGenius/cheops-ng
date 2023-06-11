/*
 * Cheops Next Generation GUI
 * 
 * gui-dns.c
 * shim between adns and cheops
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <adns.h>

#include "event.h"
#include "logger.h"
#include "cheops-gui.h"
#include "gui-sched.h"
#include "gui-utils.h"
#include "gui-handlers.h"
#include "ip_utils.h"
#include "gui-dns.h"

unsigned char adns_initalized = 0;
int dns_timer_int = -1;

static adns_state ads;
static dns_query *dns_query_list;


#if 0
void dns_loop()
{
	adns_answer *answer = NULL;
	dns_query *d, *prev, *q;
	static unsigned char in_adns_loop = 0;

	if(!in_adns_loop)
	{
		in_adns_loop = 1;
		while(we_are_still_running)
		{
			adns_processany(ads);

			while(gtk_events_pending())
				gtk_main_iteration();
			
			answer = NULL;
			// run through all of the queries
			for(d = dns_query_list; d; d = d->next)
			{
				adns_check(ads, &d->qu, &answer, (void *)&q);

				while(gtk_events_pending())
					gtk_main_iteration();
			
				if(answer)
					break;
			}
			
			
			if(answer)
			{
				// get it off of the queue
				prev = NULL;
				for(d = dns_query_list; d; d = d->next)
				{
					if(q == d)
					{
						if(prev)
							prev->next = d->next;
						else
							dns_query_list = d->next;
						break;
					}
					prev = d;
				}
				if(d == NULL)
				{
					printf("\n I did not find the q\n");
				}	
				if(answer->status == adns_s_ok)
				{
					if(q->flags & DNS_QUERY_FLAGS_NAME)
						q->ip = makestring(inet_ntoa( (struct in_addr) *answer->rrs.inaddr));
					else
						q->hostname = makestring( (char *)(answer->rrs.str) + 4 );
					if(q->answer_cb)
						q->answer_cb(q->ip, q->hostname, q->data);
					if(q->ip)
						free(q->ip);
					if(q->hostname)
						free(q->hostname);
					
					free(q);	
					
				}
				else
				{
					if(q->timeout_cb)
						q->timeout_cb(q->ip, q->hostname, q->data);
					if(q->ip)
						free(q->ip);
					if(q->hostname)
						free(q->hostname);
					
					free(q);	
				}
			}
			// there are no more entries in the list so get out
			if(!dns_query_list)
				break;
		}		
		in_adns_loop = 0;
	}

}
#else

static int dns_timer(void *data)
{
	adns_answer *answer = NULL;
	dns_query *d, *prev, *q;

	adns_processany(ads);

	// run through all of the queries
	for(d = dns_query_list; d; d = d->next)
	{
		adns_check(ads, &d->qu, &answer, (void *)&q);

		while(gtk_events_pending())
			gtk_main_iteration();
	
		if(answer)
			break;
	}
	
	if(answer)
	{
		// get it off of the queue
		prev = NULL;
		for(d = dns_query_list; d; d = d->next)
		{
			if(q == d)
			{
				if(prev)
					prev->next = d->next;
				else
					dns_query_list = d->next;
				break;
			}
			prev = d;
		}

		if(d == NULL)
		{
			printf("\n I did not find the q\n");
		}	

		if(answer->status == adns_s_ok)
		{
			if(q->flags & DNS_QUERY_FLAGS_NAME)
				q->ip = makestring(inet_ntoa( (struct in_addr) *answer->rrs.inaddr));
			else
				q->hostname = makestring( (char *)(answer->rrs.str) + 4 );
			if(q->answer_cb)
				q->answer_cb(q->ip, q->hostname, q->data);
			if(q->ip)
				free(q->ip);
			if(q->hostname)
				free(q->hostname);
			
			free(q);	
		}
		else
		{
			if(q->timeout_cb)
				q->timeout_cb(q->ip, q->hostname, q->data);
			if(q->ip)
				free(q->ip);
			if(q->hostname)
				free(q->hostname);
			
			free(q);	
		}
	}
	
	// there are no more entries in the list so kill the timer
	if(!dns_query_list)
	{
		dns_timer_int = -1;
		return(0);
	}
	return(1);
}

void dns_loop()
{
	if(dns_timer_int < 0)
		dns_timer_int = cheops_sched_add(100, dns_timer, NULL);
}
#endif

void dns_name_lookup( char *hostname, dns_answer_cb answer_cb, dns_timeout_cb timeout_cb, void *data)
{
	dns_query *q;
	
	if(!adns_initalized)
	{
		adns_initalized++;
		adns_init (&ads,/*adns_if_debug |*/ adns_if_noautosys, 0);
	}

	while(gtk_events_pending())
		gtk_main_iteration();

	q = malloc(sizeof(dns_query));
	q->hostname = makestring(hostname);
	q->ip = NULL;
	q->answer_cb = answer_cb;
	q->timeout_cb = timeout_cb;
	q->data = data;
	q->flags = DNS_QUERY_FLAGS_NAME;
	q->next = dns_query_list;
	dns_query_list = q;

	adns_submit (ads, q->hostname, adns_r_a, 0, q, &(q->qu));
	
	while(gtk_events_pending())
		gtk_main_iteration();

	dns_loop();
}

void dns_reverse_lookup( char *ip, dns_answer_cb answer_cb, dns_timeout_cb timeout_cb, void *data)
{
	dns_query *q;
		
	if(!adns_initalized)
	{
		adns_initalized++;
		adns_init (&ads,/*adns_if_debug |*/ adns_if_noautosys, 0);
	}

	while(gtk_events_pending())
		gtk_main_iteration();
	
	q = malloc(sizeof(dns_query));
	q->hostname = NULL;
	q->ip = makestring(ip);
	if(!inet_aton(q->ip, &q->saddr.sin_addr))
		return; //invalid ip address 
	q->saddr.sin_family = AF_INET;

	q->answer_cb = answer_cb;
	q->timeout_cb = timeout_cb;
	q->data = data;
	q->flags = DNS_QUERY_FLAGS_REVERSE;
	q->next = dns_query_list;
	dns_query_list = q;
	
	adns_submit_reverse(ads, (struct sockaddr *)&q->saddr, adns_r_ptr, 0, q, &(q->qu));

	while(gtk_events_pending())
		gtk_main_iteration();

	dns_loop();
	
}
