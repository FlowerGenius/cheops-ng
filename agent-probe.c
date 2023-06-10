/*
 * Cheops Next Generation GUI
 * 
 * agent-probe.c
 * port version detection code
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
#include <string.h>

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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdarg.h>   
#include <net/if.h>   
#include <gtk/gtk.h>   
#include <sys/sockio.h>
#include <errno.h>
#else
#include <stdlib.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include "event.h"
#include "io.h"
#include "sched.h"
#include "cheops-agent.h"
#include "probe.h"
#include "logger.h"


//#define DEBUG_AGENT_PROBE

#ifdef DEBUG_AGENT_PROBE
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

typedef struct _probe_user_data {
	void *np;
	agent *a;
} probe_user_data;

void my_probe_callback(unsigned int ip, unsigned short port, char *version, void *user_data)
{
	int len;
	char *string = ((char *)ee) + sizeof(probe_r);
	probe_user_data *ud = user_data;


	if(version)
	{
		DEBUG(printf("Version recieved:\nport %d\n%s\n", port, version));

		len = strlen(version) + 1;
		
		eh->len = htonl(sizeof(event_hdr) + sizeof(probe_r) + len);
		eh->hlen = htons(sizeof(event_hdr));
		eh->type = htons(REPLY_PROBE);
		eh->flags = 0;
		ee->probe_r.ip = htonl(ip);
		ee->probe_r.port = htons(port);
		ee->probe_r.np = ud->np;
		strcpy(string, version);

		DEBUG(printf("port %d again\n", ntohs(ee->probe_r.port)));
		if(event_send(ud->a, eh) < 0)
		{
			DEBUG( clog(LOG_WARNING, "Unable to send discover reply\n") );
		}
	}
	else
	{
		DEBUG(printf("NULL Version recieved:\nport %d\n", port));
	
	}
}

void my_probe_timeout(int fd, unsigned int ip, unsigned short port, void *user_data)
{
	probe_user_data *ud = user_data;

	DEBUG(printf("%s(): timeout %s port %d\n", __FUNCTION__, inet_ntoa(*(struct in_addr *)&ip), port) );	
	
	if(ud)
		free(ud);
}

int handle_probe_request(event_hdr *h, event *e, agent *a)
{
	int ip = ntohl(e->probe_e.ip);
	unsigned short port = ntohs(e->probe_e.port);
	int timeout = ntohl(e->probe_e.timeout_ms);
	void *np = e->probe_e.np;             // we dont have to worry about this cause we are giving it back
	probe_user_data *ud = malloc(sizeof(*ud));
	
	if(ud)
	{
		ud->a = a;
		ud->np = np;
		
		DEBUG(printf("%s(): probeing %s port %d\n", __FUNCTION__, inet_ntoa(*(struct in_addr *)&ip), port) );	
		get_version(ip, port, timeout, my_probe_callback, my_probe_timeout, ud);
	}
	else
	{
		printf("eek we ran out of memory\n");
		exit(1);
	}
	return(0);
}


