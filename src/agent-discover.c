/*
 * Cheops Next Generation GUI
 * 
 * agent-discover.c
 * Network discovery code
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
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef FREEBSD
#include <netinet/in_systm.h>
#endif
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <glib.h>
#include <string.h>
#include <errno.h>
#include "event.h"
#include "io.h"
#include "sched.h"
#include "cheops-sh.h"
#include "logger.h"
#include "cheops-agent.h"
#include "agent-discover.h"
#include "ip_utils.h"
#include "cache.h"

//#define DEBUG_DISCOVER

#ifdef DEBUG_DISCOVER
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

#ifdef FREEBSD
#define ICMP_TIME_EXCEEDED ICMP_TIMXCEED
#define ICMP_DEST_UNREACH ICMP_UNREACH  
#endif


#define PING_TIMEOUT_INTERVAL 3000
#define PING_INTERVAL 78 /* this is to have a class C disvover = 20 seconds */

struct ping_q {
	u32	start;
	u32	end;
	u32	current;
	u32	desched;
	u32	retry;
	u32	flags;
	void *np;
	unsigned int	time;
	agent 	*a;
	struct ping_q *next;
};

static struct ping_q *pingQ = NULL;
static int ping_socket = -1;
int ping_retries = 1;

typedef struct _map_link_t {
	u32 address;
	u32 metric;
} map_link_t;

typedef struct _map_t {
	u32         address;
	GList      *map;
	u32         ttl;
} map_t;

int handle_discover_request(event_hdr *h, event *e, agent *a);
int handle_ping_recieve(int *id, int fd, short e, void *data);
int ping_send(struct ping_q *pp);
int handle_map(char *buf, int len, struct sockaddr_in *sin);

int handle_discover_request(event_hdr *h, event *e, agent *a)
{
	int 	ret 	= -1;
	u32	start 	= ntohl(e->discover_ipv4_e.start);
	u32	end 	= ntohl(e->discover_ipv4_e.end);
	char	*errorPtr=NULL;
	char	errorStr[] = "Can't Discover 0.0.0.0\n";
	char	errorStr2[] = "Start is larger than Finish\n";
	struct ping_q *pp=NULL;
	
	DEBUG(c_log(LOG_DEBUG,"i got a discover request %s\n", ip2str(start)) );
	DEBUG(c_log(LOG_DEBUG,"to %s\n",ip2str(end)) );

	if(start==0 || end==0)
	{
		errorPtr=&errorStr[0];
	}

	if( ((unsigned long)ntohl(start)) > ((unsigned long)ntohl(end)) )
	{
		errorPtr=&errorStr2[0];
	}
	
	if(errorPtr)
	{
		eh->len = htonl(sizeof(event_hdr) + sizeof(error_r) + strlen(errorPtr));
		eh->hlen = htons(sizeof(event_hdr));
		eh->type = htons(REPLY_ERROR);
		eh->flags = 0;
		ee->error_r.error = 99;
		ee->error_r.len = strlen(errorPtr);
		strcpy( ((char *)ee + sizeof(error_r)) , errorPtr);

		if (event_send(a, eh) < 0)
			c_log(LOG_WARNING, "Unable to send error reply\n");
		else
			ret = 0;
		return(ret);
	}	
	
	if((pp = malloc(sizeof(struct ping_q)))==NULL)
	{
		c_log(LOG_DEBUG,"malloc returned NULL\n");
		exit(1);
	}
	pp->start = start;
	pp->current = ntohl(start); /* so we can increment it nicely */
	pp->end = end;
	pp->a = a;
	pp->retry = 0;
	pp->flags = h->flags;
	pp->time = 0;
	pp->np = e->discover_ipv4_e.np;
	pp->next = pingQ;
	pingQ = pp;

	pp->desched = cheops_sched_add(1,CHEOPS_SCHED_CB(ping_send),pp);
		
	return(ret);
}

int remove_ping_q(struct ping_q *pp)
{
	struct ping_q *p,*prev=NULL;
	
	for(p=pingQ; p; p = p->next)
	{
		if(p == pp)
		{
			if(pp == pingQ)
				pingQ = pingQ->next;
			else
				prev->next = p->next;
			free(pp);
			return(1);
		}
		prev = p;
	}
	return(0);
}

int in_ping_q(struct ping_q *pp)
{
	struct ping_q *p;
	for(p=pingQ;p;p=p->next)
	{
		if(pp == p)
			return(1);
	}
	return(0);
}

int in_ping_q_range(struct ping_q *q, int ip)
{
	if(in_ping_q(q))
		if((ntohl(ip) <= ntohl(q->end)) && (ntohl(ip) >= ntohl(q->start)))
			return(1);
	return(0);
}

int send_discover_reply(int ip, struct ping_q *pp)
{
	eh->len = htonl(sizeof(event_hdr) + sizeof(discover_ipv4_r));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(REPLY_DISCOVER_IPV4);
	eh->flags = pp->flags;
	ee->discover_ipv4_r.ipaddr = htonl(ip);
	ee->discover_ipv4_r.np = pp->np;

	if (event_send(pp->a, eh) < 0)
	{
		DEBUG( c_log(LOG_WARNING, "Unable to send discover reply\n") );
		return(0);
	}
	return(1);
}

int handle_ping_recieve(int *id, int fd, short e, void *data)
{
	struct ip 	*ip;
	struct icmp 	*i;
	struct ping_q	*pp;	
	struct sockaddr_in sin;
	int 	hlen;
	int	len;
	int	sinlen = sizeof(sin);
	char	buf[1024];

	if ((len = recvfrom(fd, buf, sizeof(buf), 0,(struct sockaddr *)&sin, &sinlen)) < 0) 
	{
		perror("network_socket_cb: recvfrom");
	}
	
	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;

	if(len < hlen + ICMP_MINLEN) {
		c_log(LOG_DEBUG,"handle_packet: too short (%d bytes) from %s\n",len, inet_ntoa(sin.sin_addr));
		return(1);
	}

	if( !handle_map(buf, len, &sin) )
	{
		i = (struct icmp *)(buf + hlen);
		if((i->icmp_type == ICMP_TIME_EXCEEDED) || (i->icmp_type == ICMP_DEST_UNREACH))
			return(1);

		if(i->icmp_type != ICMP_ECHOREPLY)
			return(1);

		bcopy((buf + hlen + 8),&pp,sizeof(void *));

		if(!in_ping_q_range(pp,sin.sin_addr.s_addr))
		{
			/* someone is being a bad little boy and sending us corrupt 
			 * ping packets */
			DEBUG( c_log(LOG_WARNING,"%s sent us a ping out of the discovery range\n",inet_ntoa(sin.sin_addr)) );
			return(1);
		}
	
	   DEBUG( c_log(LOG_WARNING,"Recieved ping reply %s\n",inet_ntoa(sin.sin_addr)) );
	
	   add_to_cache((void *)(unsigned long)sin.sin_addr.s_addr,NULL);
		
	   send_discover_reply(sin.sin_addr.s_addr,pp);
	}
		
	return(1);
}

int ping_send(struct ping_q *pp)	
{
	struct protoent		*proto;
	struct sockaddr_in	to;
	struct icmp 		*i;
	struct timeval		tv;
	struct timezone		tz;
	char 	buf[256];
	unsigned int	time,ms;
	int 	len = 8 + sizeof(void *);
	int	sent;
	int	hold;
	int	*id;
	int	ret=1;
	int sent_something = FALSE;
	static int removed_queue = FALSE;
	int negative_retry = 0;
	
/* we will keep going till we actually send something */	
	do
	{
		/* the global retries expired */

		if(pp->retry > ping_retries )
		{
			removed_queue = TRUE;
			remove_ping_q(pp);
			DEBUG( c_log(LOG_NOTICE,"Removed schedule entry, and pingQ: retries expired\n") );
			return(0);
	   }
		

	   if((pp->flags & FLAG_FORCE) || !in_cache((void *)(long)ntohl(pp->current)))
	   {
			/* Have to send a ping packet */
			
			if(pp->flags & FLAG_FORCE)
			{
				DEBUG( c_log(LOG_NOTICE,"The FORCE FLAG is set\n") );
			}
			else
			{
				DEBUG( c_log(LOG_NOTICE,"The entry %s is not in the cache\n",ip2str(htonl(pp->current))) );
			}
			
		
			if(ping_socket < 0)
			{
				/* initalize the ping_socket */
				
				DEBUG( printf("%s(): Initalizing the ping_socket\n", __FUNCTION__));
				if(!(proto = getprotobyname("icmp")))
				{
					c_log(LOG_DEBUG,"socket error\n");
					exit(1);
				}
				
				if((ping_socket = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0)
				{
					c_log(LOG_DEBUG,"socket error\n");
					exit(1);
				}
				hold = 48 * 1024;
				setsockopt(ping_socket,SOL_SOCKET,SO_RCVBUF,(char *)&hold,sizeof(hold)); 
		
				id = cheops_io_add(ping_socket,handle_ping_recieve,CHEOPS_IO_IN,NULL);
			}
				
			/* send the ICMP PING */
			
			memset(&to, 0, sizeof(struct sockaddr_in));
			to.sin_family = AF_INET;
			to.sin_addr.s_addr = htonl(pp->current);
			i = (struct icmp *)buf;
			i->icmp_type = ICMP_ECHO;
			i->icmp_code = 0;
			i->icmp_cksum = htons(0);
			i->icmp_seq = htons(pp->retry + pp->current);
		
			bcopy(&pp, &i->icmp_id, sizeof(void *));
			bcopy(&pp,&buf[8], sizeof(void *));
			i->icmp_cksum = inet_checksum(i, len);

			sent = 1;
			
			if( IP_CLASSA(to.sin_addr.s_addr) && IP_CLASSA_BROADCAST(to.sin_addr.s_addr) )
				sent = 0;
			else if( IP_CLASSB(to.sin_addr.s_addr) && IP_CLASSB_BROADCAST(to.sin_addr.s_addr) )
				sent = 0;
			else if( IP_CLASSC(to.sin_addr.s_addr) && IP_CLASSC_BROADCAST(to.sin_addr.s_addr) )
				sent = 0;
				
			if(sent)
			{
				sent_something = TRUE;
				sent = sendto(ping_socket, buf, len, 0, (struct sockaddr *)&to, sizeof(to));
				if (sent < 0 || sent != len)
				{
					DEBUG( c_log(LOG_DEBUG,"wrote %s %d chars, ret = %d\n",inet_ntoa(to.sin_addr), len, sent) );
					if (sent < 0)
					{
						if(negative_retry++ < 4)
						{
							sent_something = FALSE;
							continue;
						}
						else
						{
							c_log(LOG_DEBUG,"sendto error %s for %s\n",strerror(errno), inet_ntoa(to.sin_addr));
						}
					}
				}				
				DEBUG( c_log(LOG_DEBUG,"wrote %s %d chars, ret = %d\n",inet_ntoa(to.sin_addr), len, sent) );
			}
		}
		else
		{
			/* The entry is in the cache and can be used in the discover reply */
			
			DEBUG( c_log(LOG_NOTICE,"The entry %s is in the cache\n",ip2str(htonl(pp->current))) );
			if(pp->retry == 0)
			{
				/* 
				 * This is the first try for this ip address and since it is
				 * in the cache we only want to send a dicover reply for this 
				 * first time, all other times that this ip address is "discovered"
				 * due to the retries the discover request will not be sent. If it
				 * is not in the cache and is discovered within the retries then 
				 * there will be sent a discover reply anyway from the recieve function 
				 */

				send_discover_reply(htonl(pp->current),pp);
			}
		}
		
		/* Advance the current ip address */
		if(pp->current == ntohl(pp->end))
		{
			pp->current = ntohl(pp->start);
			pp->retry++;
		}
		else
		{
			pp->current++;
		}
    	
	} while(!sent_something);
	

	if(pp->current == ntohl(pp->end))
	{
		/* 
		 * adjust the schedule for ping_send to be 
		 *     PING_TIMEOUT_INTERVAL - (current_time - first_ping_send_time) 
		 */
	
		DEBUG( c_log(LOG_NOTICE,"Changed schedule entry\n") );
		
		if(pp->retry > ping_retries)
		{
			/* we are at the last entry and should wait for the replies */
			time = PING_TIMEOUT_INTERVAL;
		}
		else
		{
			gettimeofday(&tv,&tz);
			ms = (int)tv.tv_sec*1000 + (int)tv.tv_usec/1000;
			if(pp->time == 0)
			{
				time = PING_TIMEOUT_INTERVAL;
			}
			else
			{
				if(ms - pp->time > PING_TIMEOUT_INTERVAL)
				{
					time = 1; /* it has been a long time since the start ip */
				}
				else
				{
					if(ms - pp->time <= 0)
					{
						time = +ms - pp->time + PING_TIMEOUT_INTERVAL;
					}
					else
					{
						time = -ms + pp->time + PING_TIMEOUT_INTERVAL;
					}
				}
			}
			if(time < 0)
				time = 1; /* check, we can't schedult something with -time */
		}
		cheops_sched_add(time,CHEOPS_SCHED_CB(ping_send),pp);
		ret=0;
	}
	else
	{
		if(pp->current == ntohl(pp->start))
		{
			/* 
			 * Record the first send time, so we can adjust the time from the ->end's send
			 * and the ->start's, of the next retry, send to be >= PING_TIMEOUT_INTERVAL 
			 */	
			gettimeofday(&tv,&tz);
			pp->time = (int)tv.tv_sec*1000 + (int)tv.tv_usec/1000;
			DEBUG( c_log(LOG_NOTICE, "I set the pp->time to %u\n",pp->time) );
			
			cheops_sched_add(PING_INTERVAL,CHEOPS_SCHED_CB(ping_send),pp);
			ret=0;
		}
	}

	return(ret);
}


