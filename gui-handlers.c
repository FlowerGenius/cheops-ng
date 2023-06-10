/*
 * Cheops Next Generation GUI
 * 
 * gui-handlers.c
 * Agent Handelers for the GUI
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
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <adns.h>
#include "cheops.h"
#include "gui-io.h"
#include "logger.h"
#include "event.h"
#include "ip_utils.h"
#include "cheops-gui.h"
#include "gui-viewspace.h"
#include "cheops-osscan.h"
#include "gui-canvas.h"
#include "gui-utils.h"
#include "gui-settings.h"
#include "gui-dns.h"
#include "probe.h"

//#define DEBUG_GUI_HANDLERS

#ifdef DEBUG_GUI_HANDLERS
	#define DEBUG(a) a
#else
	#define DEBUG(a) 
#endif

typedef struct _ip_cache {
	struct _ip_cache *next;
	u32 ip;
	char *name;
} ip_cache;

ip_cache *IP_cache = NULL;

typedef struct _handler {
	int id;
	cheops_event_handler cb;
	char *desc;
} handler;

typedef struct _discover_dns {
	char *start;
	char *end;
	u32 start_ip;
	u32 end_ip;
	int flags;
	net_page *np;
	agent *a;
	ip_cache *cache;
	int timeout_id;
} discover_dns;

#define DISCOVER_DNS_FLAGS_NETWORK  1
#define DISCOVER_DNS_FLAGS_USED_DNS 2


void do_host_discover_after_dns(char *ip,char *hostname, void *data);
void do_network_discover_after_dns(char *ip, char *hostname, void *data);
int do_probe(agent *a, u32 ip, u16 port, u32 timeout_ms, void *np);

u32 probe_timeout_ms = 3000;

ip_cache *add_ip_cache(u32 ip, char *name)
{
	ip_cache *p = malloc(sizeof(ip_cache));
	
	if(p)
	{
		p->ip = ip;
		p->name = makestring(name);
		p->next = IP_cache;
		IP_cache = p;
	}
	return(p);
}	

char *get_ip_cache(u32 ip)
{
	ip_cache *p, *prev = NULL;
	char *name;
	
	for(p = IP_cache; p; p = p->next)
		if(ip == p->ip)
			break;
		else
			prev = p;
	
	if(p)
	{
		name = p->name;
		
		if(prev)
			prev->next = p->next;
		else
			IP_cache = p->next;
			
		free(p);
		return(name);
	}
	return(NULL);
}
	

void dns_timeout(char *ip, char *hostname, void *data)
{
	discover_dns *dd = data;
	char buf[256];
			
	sprintf(buf,"I could not get an IP address for '%s'. Do you want me to try it again?",dd->start);
	if(make_gnome_yes_no_dialog("DNS lookup error",buf,TRUE, NULL) == 0 )
	{
		//retry the dns
		if(dd->flags & DISCOVER_DNS_FLAGS_NETWORK)
			dns_name_lookup( dd->start, do_network_discover_after_dns, dns_timeout, (void *)dd);
		else
			dns_name_lookup( dd->start, do_host_discover_after_dns, dns_timeout,  (void *)dd);
	}
	else
	{
		if(dd->start)
			free(dd->start);
		if(dd->end)
			free(dd->end);
		free(dd);
	}
}

static int handle_error(event_hdr *h, event *e, agent *a)
{
	clog(LOG_ERROR,"\nAgent reports: Error[%d] - %s", ntohs(e->error_r.error), ((char *)e) + sizeof(error_r));
	return(0);
}

int send_discover_request(discover_dns *dd)
{
	int done = -1;
	
	eh->len = htonl(sizeof(event_hdr) + sizeof(discover_ipv4_e));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(EVENT_DISCOVER_IPV4);
	eh->flags = 0; // huh need to think of something else dd->flags;
	ee->discover_ipv4_e.start = dd->start_ip; // already htonl
	ee->discover_ipv4_e.end = dd->end_ip;     // already htonl
	ee->discover_ipv4_e.is_name = FALSE;
	ee->discover_ipv4_e.np = dd->np;

	// cache the name so that we can retrieve it when we get a reply
	if(dd->start)
		dd->cache = add_ip_cache(ntohl(dd->start_ip), dd->start);
	
	if (event_send(dd->a, eh) < 0) 
		clog(LOG_WARNING, "Unable to send discover event\n");
	else
		done = 0;

	return(done);	
}

void do_host_discover_after_dns(char *ip, char *hostname, void *data)
{
	discover_dns *dd = data;
	struct in_addr addr;
	
	if(inet_aton(ip, &addr))
	{
		DEBUG( printf("%s(): Discovering host %s (%s)\n", __FUNCTION__, hostname, ip) );
		dd->start_ip = htonl(addr.s_addr);
		dd->end_ip = dd->start_ip;
		
		send_discover_request(dd);
	}
	else
	{
		DEBUG( printf("%s(): Invalid Host %s (%s)\n", __FUNCTION__, hostname, ip) );
	}	
	if(dd->start)
		free(dd->start);
	if(dd->end)
		free(dd->end);
	
	free(dd);
}

void do_network_discover_after_dns(char *ip, char *hostname, void *data)
{
	int res;
	u32 mask;
	discover_dns *dd = data;
	struct in_addr addr;
		
	if(inet_aton(ip,&addr))
	{
		/* Check the netmask, first to see if it's of the form 255.255.255.0 */
		res = inet_aton(dd->end, (struct in_addr *)&mask);
		mask = ntohl(mask);
		if (!res || (mask < 0x80000000) || !allones(mask))
		{
			if ((atoi(dd->end) > 32) || (atoi(dd->end) < 1))
			{
				/* We don't allow a zero netmask, that would
				   be the entire internet! */
				printf("Invalid netmask: %s\n", dd->end);
				return;
			}
			mask = 0xffffffff;
			mask = mask >> (32-atoi(dd->end));
			mask = mask << (32-atoi(dd->end));
		}

		dd->start_ip = htonl(addr.s_addr);
		dd->start_ip &= mask;
		dd->end_ip = dd->start_ip | ~mask;
		DEBUG( printf("Discovering from %s to ", ip2str(htonl(dd->start_ip))) );
		DEBUG( printf("%s (mask = ", ip2str(htonl(dd->end_ip)) ));
		DEBUG( printf("%s)...\n", ip2str(htonl(mask)) ));
		
		send_discover_request(dd);
	}
	else
	{
		DEBUG( printf("%s(): Invalid Address %s (%s)\n", __FUNCTION__, hostname, ip) );
	}	
	
	if(dd->start)
		free(dd->start);
	if(dd->end)
		free(dd->end);
	
	free(dd);
}

int do_discover(agent *a, char *host, void *np, int flags)
{
	char *c;
	discover_dns *dd = malloc(sizeof(discover_dns));
	
	dd->start_ip = 0;
	dd->end_ip = 0;
	dd->start = NULL;
	dd->end = NULL;
	dd->flags = 0;
	dd->a = a;
	dd->np = np;
	
	if ((c = strchr(host, '/')))
	{
		/* Network/netmask */
		*c='\0';
		c++;
		
		dd->end = makestring(c);
		dd->flags |= DISCOVER_DNS_FLAGS_NETWORK;
		
		if( 0 == inet_aton(host, (struct in_addr *)&(dd->start_ip)) )
		{
			//is a hostname
			//this is a network address ie: 255.255.255.0
			dd->flags |= DISCOVER_DNS_FLAGS_USED_DNS;

			dd->start = makestring(host);
			dns_name_lookup( dd->start, do_network_discover_after_dns,dns_timeout, (void *)dd);
		}
		else
		{
			//is a ip address
			//this is a network address ie: 255.255.255.0
			dd->start = makestring(host);	
			do_network_discover_after_dns( host ,NULL, dd);
		}
	}
	else
	{
		/* Host */
		if( 0 == inet_aton(host, (struct in_addr *)&dd->start_ip) )
		{
			//hostname
			dd->flags |= DISCOVER_DNS_FLAGS_USED_DNS;
			dd->start = makestring(host);

			dns_name_lookup( dd->start, do_host_discover_after_dns,dns_timeout, (void *)dd);
		}
		else
		{
			//ip address
			do_host_discover_after_dns(host,NULL, dd);
		}
	}
	return(0);
}

void do_discover_range_after_last_dns(char *ip, char *hostname, void *data)
{
	discover_dns *dd = data;
	struct in_addr addr;
		
	if(inet_aton(ip,&addr))
	{
		dd->end_ip = htonl(addr.s_addr);
		
		if( ((unsigned long)ntohl(dd->start_ip)) > ((unsigned long)ntohl(dd->end_ip)) )
		{
			DEBUG(printf("swap ip range numbers\n"));
			addr.s_addr = dd->start_ip;
			dd->start_ip = dd->end_ip;
			dd->end_ip = addr.s_addr;
			
			ip = dd->start;
			dd->start = dd->end;
			dd->end = ip;
		} 
		
		DEBUG(
			printf("Discovering range from %s to ", ip2str(htonl(dd->start_ip)) );
			printf("%s\n", ip2str(htonl(dd->end_ip)) ); 
		);
		
		send_discover_request(dd);
	}
	else
	{
		DEBUG( printf("%s(): Invalid Address %s (%s)\n", __FUNCTION__, hostname, ip) );
	}	
	
	if(dd->start)
		free(dd->start);
	if(dd->end)
		free(dd->end);
	
	free(dd);
}

void do_discover_range_after_first_dns(char *ip, char *hostname, void *data)
{
	discover_dns *dd = data;
	struct in_addr addr;
		
	if(inet_aton(ip,&addr))
	{
		// we were able to lookup the address and it is a good one
		// now try to do reverse dns on the last ip address
		
		dd->start_ip = htonl(addr.s_addr);
		
		if( 0 == inet_aton(dd->end, (struct in_addr *)&(dd->end_ip)) )
		{
			//is a hostname

			dd->flags |= DISCOVER_DNS_FLAGS_USED_DNS;

			dns_name_lookup( dd->end, 
			                 do_discover_range_after_last_dns,
			                 dns_timeout, 
			                 (void *)dd);
		}
		else
		{
			//is a ip address

			do_discover_range_after_last_dns( dd->end ,NULL, dd);
		}
	}
	else
	{
		DEBUG( printf("%s(): Invalid Address %s (%s)\n", __FUNCTION__, hostname, ip) );

		if(dd->start)
			free(dd->start);
		if(dd->end)
			free(dd->end);
		
		free(dd);
	}	
}

int do_discover_range(agent *a, char *first, char *last, void *np, int flags)
{
	discover_dns *dd = malloc(sizeof(discover_dns));
	
	dd->start_ip = 0;
	dd->end_ip = 0;
	dd->start = NULL;
	dd->end = NULL;
	dd->flags = 0;
	dd->a = a;
	dd->np = np;
	
	dd->start = makestring(first);
	dd->end = makestring(last);
	
	if( 0 == inet_aton(dd->start, (struct in_addr *)&(dd->start_ip)) )
	{
		//is a hostname
		dd->flags |= DISCOVER_DNS_FLAGS_USED_DNS;

		dns_name_lookup( dd->start, 
		                 do_discover_range_after_first_dns,
		                 dns_timeout, 
		                 (void *)dd);
	}
	else
	{
		//is a ip address

		do_discover_range_after_first_dns( dd->start ,NULL, dd);
	}

	return(0);
}


void reverse_lookup_cb(char *ip, char *hostname, void *data)
{
	page_object *po = data;
	
	if( 0 == strcmp(inet_ntoa(*(struct in_addr *)&po->ip), ip) )
	{
		
		if(po->name)
			free(po->name);
		
		po->name = makestring(hostname);	
	
		page_object_display(po);
	}
}


page_object *add_discovered_node(agent *a, 
                                 net_page *np, 
                                 unsigned int ip, 
                                 char *name, 
                                 os_stats *os_data,
                                 int mapit)
{
	page_object *po = NULL;
	int osscan = TRUE;

	if( is_valid_net_page(np) )
	{
		po = add_host_entry_to_net_page(np, ip, name );

		DEBUG( printf("%s()\n", __FUNCTION__) );
		if(po)
		{
			if(os_data && os_data->os && !options_rescan_at_startup)
				osscan = FALSE;
				
			if(os_data)
			{ 
				if(po->os_data)
					page_object_free_os_data(po);
				po->os_data = os_data;
			}
			
			if(osscan && options_operating_system_check)
			{
				DEBUG( printf("%s(): doing os scan '%s'\n", __FUNCTION__, name));	
		
				po->flags &= PAGE_OBJECT_OS_DETECT_SENT;
				do_os_scan(a, ip, np, 0);
			}
			
			if(options_map && mapit)
			{
				page_object_map(np, po);
			}
			
			if(!name && options_reverse_dns)
			{
				DEBUG( printf("%s(): doing reverse lookup '%s'\n", __FUNCTION__, name));	
		
				dns_reverse_lookup( inet_ntoa(*(struct in_addr *)&ip), reverse_lookup_cb, NULL, po);
			}
		}
	}
	else
	{
		DEBUG( printf("%s(): invalid np = %p \n", __FUNCTION__, np));	 
	}
	return(po);
}

int handle_discover_reply(event_hdr *h, event *e, agent *a)
{
	net_page *np = (net_page *)e->discover_ipv4_r.np;
	int ip = ntohl(e->discover_ipv4_r.ipaddr);
	char *name = get_ip_cache(ip);
	
	DEBUG( printf("%s(): got reply %s '%s'\n", __FUNCTION__, ip2str(ip), name));	

	DEBUG( printf("%s()\n", __FUNCTION__) );
	if(is_valid_net_page(np))
	{
		add_discovered_node(a, np, ip, name, NULL, TRUE);
	}
	else
	{
		DEBUG( clog(LOG_NOTICE, "the net page does not exist anymore\n") );
	}
	
	if(name)
		free(name);
	
	return(1);
}

/******************************************************************************/
/*                              OS detection                                  */
/******************************************************************************/

int do_os_scan(agent *a, int ip, void *np, int flags)
{
	DEBUG(printf("%s(): osscan for %s\n", __FUNCTION__, ip2str(ip)) );
	
	eh->len = htonl(sizeof(event_hdr) + sizeof(os_scan_e) + strlen(options_os_scan_ports));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(EVENT_OS_SCAN);
	eh->flags = flags;
	ee->os_scan_e.ip = htonl(ip);
	ee->os_scan_e.np = np;
	ee->os_scan_e.options = htonl(options_os_scan_flags);	
	strcpy(END_OF_OPTION(&(ee->os_scan_e)), options_os_scan_ports);
	
	if (event_send(a, eh) < 0) 
		clog(LOG_WARNING, "Unable to send OS-scan event\n");
	return(0);
}

int handle_os_scan_reply(event_hdr *h, event *e, agent *a)
{
	char *ptr = (void *)e + sizeof(os_scan_r);
	os_scan_option *opt;
	os_scan_option_os *os;
	os_scan_option_port *port;
	os_scan_option_uptime *os_uptime;
	os_stats	*os_data;
	os_port_entry	*os_port;
	int i,j = ntohl(e->os_scan_r.num_options);
	page_object *po;
	int ip = e->os_scan_r.ip;
	net_page *np = e->os_scan_r.np;
	
	DEBUG( printf("%s()\n", __FUNCTION__) );
	DEBUG( printf("%s(): %d options\n", __FUNCTION__, j) );
	po = page_object_get_by_ip( np, ip);
	
	if(po == NULL) // we could not find the page object so dont do anything to it
	{
		DEBUG( printf("could not find host for ip=%s\n", inet_ntoa(*((struct in_addr *)&ip))) );
		return(0);
	}
	page_object_free_os_data(po);	
		
	if( NULL == (os_data = malloc(sizeof(*os_data))) )
	{
		printf("erg no memory\n");
		exit(1);
	}
	memset(os_data, 0, sizeof(*os_data));
	
	for(i = 0; i < j; i++)
	{
		opt = (void *)ptr;
		switch(ntohl(opt->type))
		{
			case OS_SCAN_OPTION_OS:
				os = (void *)opt;
				os_data->os = makestring(END_OF_OPTION(os));
				DEBUG( printf("OS_SCAN_OPTION_OS: %s\n", os_data->os) );
				break;
			
			case OS_SCAN_OPTION_UPTIME:
				os_uptime = (void *)opt;
				os_data->uptime_seconds = ntohl(os_uptime->seconds);
				if(strlen(END_OF_OPTION(os_uptime)) != 0)
					os_data->uptime_last_boot = makestring(END_OF_OPTION(os_uptime));
				DEBUG( printf("OS_SCAN_OPTION_OS: %d %s\n", os_data->uptime_seconds, os_data->uptime_last_boot) );
				break;
			
			case OS_SCAN_OPTION_PORT:
				port = (void *)opt;
				os_port = malloc(sizeof(os_port_entry));
				os_port->port = ntohl(port->port_number);
				os_port->protocol = ntohl(port->protocol);
				os_port->version = NULL;
				os_port->proto = ntohl(port->proto);
				os_port->state = ntohl(port->state);
				os_port->rpcnum = ntohl(port->rpcnum);
				os_port->rpclowver = ntohl(port->rpclowver);
				os_port->rpchighver = ntohl(port->rpchighver);
				os_port->name = strdup(END_OF_OPTION(port));
				os_port->owner = strdup(END_OF_OPTION(port) + strlen(os_port->name) + 1);
				add_service(os_port->port, os_port->protocol, os_port->name);
				
				if(options_probe_ports)
					do_probe(a, ip, os_port->port, probe_timeout_ms, np);
				
				os_port->next = os_data->ports;
				os_data->ports = os_port;
				DEBUG( printf("OS_SCAN_OPTION_PORT: proto=%d port=%d %s\n", os_port->protocol, 
				                                                            os_port->port, 
				                                                            os_port->name) );
				DEBUG( printf("OS_SCAN_OPTION_PORT: rpcnum=%d ver=%d.%d owner=%s\n", os_port->rpcnum, 
				                                                                     os_port->rpchighver, 
				                                                                     os_port->rpclowver, 
				                                                                     os_port->owner) );
				break;
					
			default:
				DEBUG( printf("unknown option\n") );
				clog(LOG_ERROR,"\nWhat the h@#$ is this? handle_os_scan_reply\n");
				break;
		}
		ptr += ntohl(opt->length);
	}
	po->os_data = os_data;

	// redisplay the object
	page_object_display(po);

	return(0);
}


/******************************************************************************/
/*                              DNS Request                                   */
/******************************************************************************/

int do_dns_request_scan(agent *a, char *host, int flags)
{
	eh->len = htonl(sizeof(event_hdr) + sizeof(discover_ipv4_e));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(EVENT_DNS_QUERY);
	eh->flags = flags;
	strcpy(ee->dns_query_e.name, host);
	
	DEBUG( printf("%s()\n", __FUNCTION__) );
	if (event_send(a, eh) < 0) 
		clog(LOG_WARNING, "Unable to send DNS Query event\n");
	return(0);
}


int handle_dns_query_reply(event_hdr *h, event *e, agent *a)
{

	clog(LOG_NOTICE, "i got a dns reply");		

	return(0);
}


/******************************************************************************/
/*                              ICMP Mapping                                  */
/******************************************************************************/
int do_map_icmp(agent *a, u32 ip, void *np)
{
	eh->len = htonl(sizeof(event_hdr) + sizeof(map_icmp_e));
	eh->hlen = htons(sizeof(event_hdr));  
	eh->type = htons(EVENT_MAP_ICMP);
	eh->flags = 0;
	ee->map_icmp_e.ip = htonl(ip);
	ee->map_icmp_e.np = np;
	
	DEBUG( printf("%s()\n", __FUNCTION__) );
	if (event_send(a, eh) < 0) 
		clog(LOG_WARNING, "Unable to send MAP ICMP event\n");
	return(0);
}

int handle_map_icmp_reply(event_hdr *h, event *e, agent *a)
{
	int dest = ntohl(e->map_icmp_r.dest);
	int ip1 = ntohl(e->map_icmp_r.ip1);
	int ip2 = ntohl(e->map_icmp_r.ip2);
	net_page *np = e->map_icmp_r.np;
	page_object *po1;
	page_object *po2;
	
	DEBUG(
		clog(LOG_NOTICE, "i got a map icmp reply for %s\n", inet_ntoa(*(struct in_addr *)&dest) );		
		clog(LOG_NOTICE, "  %s\tto %s\n", inet_ntoa( *(struct in_addr *)&ip1)
	                                , inet_ntoa( *(struct in_addr *)&ip2) );
		clog(LOG_NOTICE, "  np = %p\n", np);
	)

	if(ip1 != ip2)
	{
#if 0
		po1 = add_host_entry_to_net_page(np, ip1, NULL );
		po2 = add_host_entry_to_net_page(np, ip2, NULL );
#else
		if( !is_valid_net_page(np) )
			printf("invalid net_page\n");
		if( !page_object_get_by_ip(np, ip1) )
		{
			add_discovered_node(a, np, ip1, NULL, NULL, FALSE);
			DEBUG( printf("%s(): adding node %s\n", __FUNCTION__, inet_ntoa( *(struct in_addr *)&ip1)) );
		}
		if( !page_object_get_by_ip(np, ip2) )
		{
			add_discovered_node(a, np, ip2, NULL, NULL, FALSE);
			DEBUG( printf("%s(): adding node %s\n", __FUNCTION__, inet_ntoa( *(struct in_addr *)&ip2)) );
		}
			
		if( !(po1 = page_object_get_by_ip(np, ip1)) )
		{
			clog(LOG_NOTICE, "oops why did the po1 not get added?\n");
		}
		else
		{
			if( !(po2 = page_object_get_by_ip(np, ip2)) )
			{
				clog(LOG_NOTICE, "oops why did the po2 not get added?\n");
			}
			else
			{
				/* map the nodes togeather */
				net_page_map_page_objects(np, po1, po2);
			}
		}
#endif
	}

	return(0);
}

/******************************************************************************/
/*                              PORT Probing                                  */
/******************************************************************************/
int do_probe(agent *a, u32 ip, u16 port, u32 timeout_ms, void *np)
{
	eh->len = htonl(sizeof(event_hdr) + sizeof(probe_e));
	eh->hlen = htons(sizeof(event_hdr));  
	eh->type = htons(EVENT_PROBE);
	eh->flags = 0;
	ee->probe_e.ip = htonl(ip);
	ee->probe_e.port = htons(port);
	ee->probe_e.timeout_ms = htonl(timeout_ms);
	ee->probe_e.np = np;
	
	DEBUG( printf("%s()\n", __FUNCTION__) );
	if (event_send(a, eh) < 0) 
		clog(LOG_WARNING, "Unable to send PROBE event\n");
	return(0);
}

int handle_probe_reply(event_hdr *h, event *e, agent *a)
{
	int ip = ntohl(e->probe_r.ip);
	u16 port = ntohs(e->probe_r.port);
	net_page *np = e->probe_r.np;
	char *version = ((char *)&e->probe_r.np + sizeof(e->probe_r.np));
	page_object *po;
	os_port_entry *os_port, *prev, *new_os;
	int found = FALSE;
	struct servent *service;
	
	DEBUG(
		clog(LOG_NOTICE, "i got a probe reply for %s\n", inet_ntoa(*(struct in_addr *)&ip) );		
		clog(LOG_NOTICE, "port %d\n", port);
		clog(LOG_NOTICE, "np = %p\n", np);
		clog(LOG_NOTICE, "version = '%s'\n", version);
	)

	po = page_object_get_by_ip(np, ip);
	if(po)
	{
		if(po->os_data)
		{
			for(os_port = po->os_data->ports; os_port; os_port = os_port->next)
			{
				if(os_port->port == port)
				{
					if(os_port->version)
						free(os_port->version);
					os_port->version = strdup(version);
					found = TRUE;
				}
			}
			
		}
		else
		{
			os_stats *os_data = malloc(sizeof(*os_data));
			memset(os_data, 0, sizeof(*os_data));
			os_data->os = strdup("unknown");
			
			po->os_data = os_data;
			DEBUG(printf("no os_data\n"));
		}
		
		if(!found)
		{
			prev = NULL;
			for(os_port = po->os_data->ports; os_port; os_port = os_port->next)
			{
				if(os_port->port > port)
					break;
				prev = os_port;
			}

			service = getservbyport(htons(port), "tcp");
			if(service)
			{
				new_os = malloc(sizeof(*os_port));
				memset(new_os, 0, sizeof(*new_os));
				new_os->port = port;
				new_os->protocol = 6; // tcp
				new_os->version = strdup(version);
				new_os->name = strdup(service ? service->s_name : "NONE");
			
				if(prev)
				{
					new_os = prev->next;
					prev->next = new_os;
				}
				else
				{
					new_os->next = po->os_data->ports;
					po->os_data->ports = new_os;
				}
			}
			else
			{
				DEBUG(
					printf("getservbyport() returned NULL\n");
				);
			}
		}
	}
	else
	{
		DEBUG(printf("could not find the host\n"));
	}
	return(0);
}

int send_auth_request(agent *a, char *username, char *password)
{
	printf("authenticating %s %s\n", username, password);

	eh->len = htonl(sizeof(event_hdr) + sizeof(auth_request_e));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(EVENT_AUTH_REQUEST);
	eh->flags = 0;
	strcpy(ee->auth_request_e.username, username);
	strcpy(ee->auth_request_e.password, password);
	
	if (event_send(a, eh) < 0) 
		clog(LOG_WARNING, "Unable to send login event\n");
	
	return(0);
}

int handle_auth_reply(event_hdr *h, event *e, agent *a)
{
	int authenticated = ntohl(e->auth_request_r.authenticated);

	if(authenticated)
		a->flags |= AGENT_FLAGS_LOGGED_IN;
	else
	{
		GtkWidget *username;
		GtkWidget *password;
		GtkWidget *dialog;
		GtkWidget *hbox;
		GtkWidget *label;
		
		a->flags &= ~(AGENT_FLAGS_LOGGED_IN);
		
		dialog = gnome_dialog_new("Please log in",
		                          GNOME_STOCK_BUTTON_OK,
		                          GNOME_STOCK_BUTTON_CANCEL,
		                          NULL);
		gnome_dialog_close_hides(GNOME_DIALOG(dialog), FALSE);
		gtk_signal_connect(GTK_OBJECT(dialog), "destroy", gtk_object_destroy, NULL);		
		gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
		
		hbox = gtk_hbox_new(FALSE, 5);

		label = gtk_label_new("Username:");
		username = gnome_entry_new("login_username");
		
		gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 5);
		gtk_box_pack_start( GTK_BOX(hbox), username, FALSE, FALSE, 5);
		gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

		gtk_widget_show(label);	
		gtk_widget_show(username);
		gtk_widget_show(hbox);

		hbox = gtk_hbox_new(FALSE, 5);

		label = gtk_label_new("Password:");
		password = gtk_entry_new();
		gtk_entry_set_visibility(GTK_ENTRY(password), FALSE);
		
		gtk_box_pack_start( GTK_BOX(hbox), label, FALSE, FALSE, 5);
		gtk_box_pack_start( GTK_BOX(hbox), password, FALSE, FALSE, 5);
		gtk_box_pack_start( GTK_BOX( GNOME_DIALOG(dialog)->vbox ), hbox, TRUE, TRUE, 0);

		gtk_widget_show(label);	
		gtk_widget_show(password);
		gtk_widget_show(hbox);

		gtk_widget_grab_focus(GTK_COMBO(username)->entry);

		gtk_signal_connect(GTK_OBJECT(GTK_COMBO(username)->entry), "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), password);
		gtk_signal_connect(GTK_OBJECT(GTK_ENTRY(password)),  
		                   "activate", 
		                   GTK_SIGNAL_FUNC(click_ok_on_gnome_dialog), 
		                   dialog);
		switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
		{
			case 0:
				send_auth_request(a, 
				                  gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(username)))),
				                  gtk_entry_get_text(GTK_ENTRY(password)) );
				gnome_dialog_close(GNOME_DIALOG(dialog));
				break;
			case 1:	
				gnome_dialog_close(GNOME_DIALOG(dialog));
				break;
			case 2:	
				gnome_dialog_close(GNOME_DIALOG(dialog));
				break;
			default:
				break;
		}
		
	}
	return(0);
}

/* *****************************       HANDLERS        ************************* */

static handler gui_handlers[] = {
	{ REPLY_ERROR,         handle_error,            "Error Reply" },
	{ REPLY_DISCOVER_IPV4, handle_discover_reply,   "Discover Reply Handler" },
	{ REPLY_OS_SCAN,       handle_os_scan_reply,    "OS Scan Reply Handler" },
	{ REPLY_DNS_QUERY,     handle_dns_query_reply,  "DNS Query Reply Handler" },
	{ REPLY_MAP_ICMP,      handle_map_icmp_reply,   "MAP ICMP Reply Handler" },
	{ REPLY_PROBE,         handle_probe_reply,      "PORT PROBE Handler" },
	{ REPLY_AUTH_REQUEST,  handle_auth_reply,       "authentication Handler" },
	{ -1 },
};

void register_gui_handlers()
{
	handler *h = gui_handlers;
	while(h->id > -1) {
		/* Try to register each of our shell items, but don't replace an existing
		   handler */
		if (event_register_handler(h->id, h->cb, 0)) 
			clog(LOG_WARNING, "Unable to register handler for '%s'\n", h->desc);
		h++;
	}
}

