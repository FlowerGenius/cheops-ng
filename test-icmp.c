/*
 * Cheops Next Generation GUI
 * 
 * test-icmp.c
 * Test Scheduler Routines
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
#include <glib.h>
#include "cheops-types.h"
#include "cheops.h"
#include "io.h"
#include "sched.h"
#include "logger.h"
#include "ip_utils.h"

//#define DO_DEBUG

#ifdef DO_DEBUG
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

int ping_socket = -1;

int hello(int *id, int fd, short events, void *data)
{
	char buf[256];
	int res;
	fprintf(stderr, "IO %d called with data '%s'\n", *id, (char *)data);
	res = read(fd, buf, sizeof(buf));
	if (res >= 0) {
		buf[res]='\0';
		fprintf(stderr, "Read: %s", buf);
	} else {
		fprintf(stderr, "Read bad read!\n");
	}
	return 1;
}

void send_pkt(void)
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
	static int count = 0;
	
	if(ping_socket < 0)
	{
/* initalize the ping_socket */
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

		id = cheops_io_add(ping_socket,hello,CHEOPS_IO_IN,NULL);
	}
		
/* send the ICMP PING */
	memset(&to, 0, sizeof(struct sockaddr_in));
	to.sin_family = AF_INET;
	inet_aton("127.0.0.1", &(to.sin_addr));
	i = (struct icmp *)buf;
	i->icmp_type = ICMP_ECHO;
	i->icmp_code = 0;
	i->icmp_cksum = htons(0);
	i->icmp_seq = htons(count++);

	memset(&i->icmp_id, 0, sizeof(void *));
	i->icmp_cksum = inet_checksum(i, len);

	sent = sendto(ping_socket, buf, len, 0, (struct sockaddr *)&to, sizeof(to));
	if (sent < 0 || sent != len)
	{
		DEBUG( c_log(LOG_DEBUG,"wrote %s %d chars, ret = %d\n",inet_ntoa(to.sin_addr), len, sent) );
		if (sent < 0)
			c_log(LOG_DEBUG,"sendto error %s for %s\n",strerror(errno), inet_ntoa(to.sin_addr));
	}				

}

int main(int argc, char *argv[])
{
	int *id;
	struct in_addr dest;

	sendicmp(soc, 0, dest);
	cheops_io_dump();

	cheops_main();
	return 0;
}

