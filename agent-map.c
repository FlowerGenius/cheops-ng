/*
 * Cheops Next Generation GUI
 * 
 * agent-map.c
 * ICMP TTL code used to map nodes
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
#ifdef FREEBSD
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h> 
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>  
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <fcntl.h> 
#include <sys/ioctl.h>
#include <stdarg.h>   
#include <net/if.h>   
#include <gtk/gtk.h>  
#include <stdio.h>    
#include <sys/sockio.h>
#include <errno.h>
#else 
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <glib.h>
#include <string.h>
#endif
#include "event.h"
#include "io.h"
#include "sched.h"
#include "logger.h"
#include "cheops-agent.h"
#include "ip_utils.h"
#include "cache.h"
#include "agent-map.h"

//#define DEBUG_MAP

#ifdef DEBUG_MAP
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

#ifdef FREEBSD
#define ICMP_TIME_EXCEEDED ICMP_TIMXCEED
#define ICMP_DEST_UNREACH ICMP_UNREACH  
#endif


struct map_request *mrq = NULL;
int map_socket = -1;
int read_map_socket = -1;
int *map_id = NULL;
int timeout_installed;

int handle_map_icmp_request(event_hdr *h, event *e, agent *a);
void map_connect(agent *a, void *np, unsigned int dest, unsigned int addr1, unsigned int addr2);
void start_mapping(unsigned int ip, agent *a, void *np);
int handle_map(char *buf, int len, struct sockaddr_in *sin);
static void mr_probe(struct map_request *mr);
static int map_timeout(void *data);


int handle_map_icmp_request(event_hdr *h, event *e, agent *a)
{
	u32	   ip = ntohl(e->map_icmp_e.ip);
	void  *np = e->map_icmp_e.np;
	
	DEBUG( clog(LOG_NOTICE, "recieved map icmp request for %s\n", inet_ntoa(*(struct in_addr *)&ip)) );	

	start_mapping(ip, a, np);

	return(0);
}


void map_connect(agent *a, void *np, unsigned int dest, unsigned int addr1, unsigned int addr2)
{
	eh->len = htonl(sizeof(event_hdr) + sizeof(map_icmp_r));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(REPLY_MAP_ICMP);
	eh->flags = 0;
	ee->map_icmp_r.dest = htonl(dest);
	ee->map_icmp_r.ip1 = htonl(addr1);
	ee->map_icmp_r.ip2 = htonl(addr2);
	ee->map_icmp_r.np = np;

	if (event_send(a, eh) < 0)
	{
		DEBUG( clog(LOG_WARNING, "Unable to map icmp reply\n") );
	}
}

void init_map_socket(void)
{
	int on = 1;
	
	if( (map_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0 )
	{
		clog(LOG_ERROR, "opening raw send socket");
		exit(1);
	}
	
	setsockopt(map_socket, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));
	
	if( setsockopt(map_socket, IPPROTO_IP, IP_HDRINCL, (char *)&on, sizeof(on)) < 0 )
	{
		clog(LOG_ERROR, "setting option IP_HDRINCL");
		exit(1);
	}
}

static void mr_probe(struct map_request *mr)
{
	struct in_addr ia;
	ia.s_addr = mr->dest;
	mr->retries++;
	mr->sent=time(NULL);
	if(map_socket < 0)
		init_map_socket();
	sendicmp(map_socket, mr->ttl, ia);
}

static int map_timeout(void *data)
{
	time_t current = time(NULL);
	struct map_request *mr, *l;
	struct in_addr ia;
	int cnt=0;
	
	if (!mrq) 
	{
		timeout_installed=0;
		return(FALSE);
	}
	l=NULL;
	mr = mrq;
	while(mr) 
	{
		if (current > mr->sent + MAP_REQUEST_TIMEOUT) 
		{
			cnt++;
			/* Limit how many retransmits we do.  This should stabalize replies a bit and
			   reduce flooding */
			if (cnt > MAP_TIMEOUT_MAX) 
			{
#ifdef DEBUG_MAP
				printf("Too many timeouts!  Waiting for another opportunity\n");
#endif
				return TRUE;
			}
			ia.s_addr=mr->dest;
			if (mr->retries > MAX_MAP_REQUEST_RETRIES) 
			{
#ifdef DEBUG_MAP
				printf("too many retries on host %s...  Skipping\n",inet_ntoa(ia));
#endif
				if (l) 
				{
					l->next = mr->next;
					free(mr);
					mr = l->next;
				} 
				else 
				{
					mrq = mr->next;
					mr = mrq;
				}
			} 
			else 
			{
#ifdef DEBUG_MAP
				printf("retransmit %d to host %s\n", mr->retries + 1, inet_ntoa(ia));
#endif
				mr_probe(mr);
			}
		} 
		else 
		{
			l = mr;
			mr = mr->next;
		}
			
	}
	return TRUE;
}

void start_mapping(unsigned int ip, agent *a, void *np)
{
	struct map_request *mr;
	struct in_addr lip;

	lip = getlocalip(ip);
#ifdef DEBUG_MAP	
	printf("local ip %s dest ip %s\n", inet_ntoa(*(struct in_addr *)&lip), inet_ntoa(*(struct in_addr *)&ip));
#endif

	if(lip.s_addr == ip)
	{
#ifdef DEBUG_MAP
		printf("trying to map own ip %s\n", inet_ntoa(*(struct in_addr *)&ip));
#endif		
		return;
	}
	
	/* check to see if there is already a request */
	mr = mrq;
	while(mr) 
	{
		if ((mr->dest == ip) && (np == mr->np)) 
		{
#ifdef DEBUG_MAP
			printf("Warning: duplicate map request\n");
#endif
			return;
		}
		mr=mr->next;
	}
	mr = malloc(sizeof(struct map_request));
	mr->agent = a;
	mr->np = np;
	mr->dest = ip;
	mr->last = lip.s_addr;
	mr->ttl = 1;
	mr->retries = 0;
	mr->sent=0;
	mr->next = mrq;
	mrq = mr;
	if (!timeout_installed)
		timeout_installed = cheops_sched_add(100, map_timeout, NULL);

#ifdef DEBUG_MAP
	{
		struct in_addr ia;
		ia.s_addr = ip;
		printf("mapping %s\n",inet_ntoa(ia));
	}
#endif
	mr_probe(mr);  /* ping this mapping request */
}

int handle_map(char *buf, int len, struct sockaddr_in *sin)
{
	struct ip *ip = (struct ip *)buf;
	struct icmp *icmp = (struct icmp *)(buf + (ip->ip_hl << 2));
	struct udphdr *udp = (struct udphdr *)(buf + (ip->ip_hl << 2));
	struct map_request *mr, *last=NULL;
	char host[256], dest[256];
	unsigned char ttl;
	int ret = 0;
	
#ifdef DEBUG_MAP
	printf("map reply of %d bytes\n",len);
#endif
	
	switch(icmp->icmp_type) 
	{
		case ICMP_TIME_EXCEEDED:
			DEBUG(printf("ICMP_TIME_EXCEDDED\n"));
			/* Get original header, etc */
			buf += ((ip->ip_hl << 2) + ICMP_MINLEN);
			len -= ((ip->ip_hl << 2) + ICMP_MINLEN);
			ip = (struct ip *)buf;
			
			strcpy(host,inet_ntoa(sin->sin_addr));
			strcpy(dest,inet_ntoa(ip->ip_dst));   
			DEBUG(printf("ICMP_TIME_EXCEEDED from %s to %s\n", host, dest));

			icmp = (struct icmp *)(buf + (ip->ip_hl << 2));
			udp = (struct udphdr *)(buf + (ip->ip_hl << 2));
			if (len < (ip->ip_hl << 2) + ICMP_MINLEN)
				break;
#ifdef DEBUG_MAP
			printf("Revised length: %d\n",len);
#endif
			if (ip->ip_p == IPPROTO_ICMP)
				ttl = ntohs(icmp->icmp_seq);
			else
#ifdef FREEBSD
				ttl = ntohs(udp->uh_dport) - EMPTY_PORT;
#else
				ttl = ntohs(udp->dest) - EMPTY_PORT;
#endif				
			strcpy(host,inet_ntoa(sin->sin_addr));
#ifdef DEBUG_MAP
			printf("%s is hop %hu to %s\n", host,ttl, inet_ntoa(ip->ip_dst));
#endif
			mr=mrq;
			last=NULL;
			while(mr) 
			{
				/* See if this is the right place and the right time */
				if ((mr->dest == ip->ip_dst.s_addr) && (ttl = mr->ttl)) 
				{
#ifdef DEBUG_MAP
					printf("Found map record for %s\n",inet_ntoa(ip->ip_dst));
#endif
					if (ttl < MAP_MAX) 
					{
						mr->ttl++;
						mr->retries=0;
						map_connect(mr->agent, mr->np, mr->dest, mr->last, sin->sin_addr.s_addr);
						mr->last = sin->sin_addr.s_addr;
						mr_probe(mr);
						last = mr;
						mr = mr->next;
					} 
					else 
					{
						if (last) 
						{
							last->next = mr->next;
							free(mr);
							mr = last->next;
						} 
						else 
						{
							mrq = mr->next;
							mr = mrq;
						}
					}
				} 
				else 
				{
					last=mr;
					mr=mr->next;
				}
			}
			break;

		case ICMP_DEST_UNREACH:
			DEBUG(printf("ICMP_DEST_UNREACH\n"));
			/* Get original header, etc */
#ifdef DEBUG_MAP
//			if (icmp->icmp_id != htons((getpid() & 0xffff)))
//				break;
#endif
			strcpy(host,inet_ntoa(sin->sin_addr));
			DEBUG(printf("ICMP_DEST_UNREACH from %s\n", host));

			buf += ((ip->ip_hl << 2) + ICMP_MINLEN);
			len -= ((ip->ip_hl << 2) + ICMP_MINLEN);
			ip = (struct ip *)buf;
			
			icmp = (struct icmp *)(buf + (ip->ip_hl << 2));
			udp = (struct udphdr *)(buf + (ip->ip_hl << 2));

#ifdef FREEBSD
			if (ntohs(udp->uh_dport) < EMPTY_PORT) 
#else
			if (ntohs(udp->dest) < EMPTY_PORT) 
#endif
				break;
			/* Fall through */

		case ICMP_ECHOREPLY:
			strcpy(host,inet_ntoa(sin->sin_addr));
			DEBUG(printf("ICMP_ECHO_REPLY from %s\n", host));
			if (len != (ip->ip_hl << 2) + ICMP_MINLEN)
				break;
			if (ip->ip_p == IPPROTO_ICMP)
				ttl = ntohs(icmp->icmp_seq);
			else 
				/* SNMP responses can also generate DEST_UNREACH */
#ifdef FREEBSD
				ttl = ntohs(udp->uh_dport) - EMPTY_PORT;
#else
				ttl = ntohs(udp->dest) - EMPTY_PORT;
#endif
			if (ttl >= MAP_MAX) 
				break;
			strcpy(host,inet_ntoa(sin->sin_addr));
#ifdef DEBUG_MAP
			printf("%s is last hop %u\n", host, ttl);
#endif
			last=NULL;
			mr=mrq;
			while(mr) 
			{
				/* See if this is the right place and the right time */
				if ((mr->dest == sin->sin_addr.s_addr) && (ttl = mr->ttl)) 
				{
#ifdef DEBUG_MAP
					printf("Found map record for %s\n",inet_ntoa(sin->sin_addr));
#endif
					ret = 1;
					map_connect(mr->agent, mr->np, mr->dest, mr->last, sin->sin_addr.s_addr);
					if (last) 
					{
						last->next = mr->next;
						free(mr);
						mr = last->next;
					} 
					else 
					{
						mrq = mr->next;
						mr = mrq;
					}
				} 
				else 
				{
					last=mr;
					mr=mr->next;
				}
			}
			break;
			
		default:
			DEBUG(printf("unknown ICMP type %d\n", icmp->icmp_type));
			break;
	}

	return(ret);
}

