/*
 * Cheops Next Generation GUI
 * 
 * test-io.c
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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "cheops.h"
#include "io.h"
#include "sched.h"

int *id;
int tid;
int sock;

int recieve_r(int *id, int fd, short events, void *data)
{
	char *string = "GET / HTTP/1.0\n\n";

	printf("recieve_ready\n\n");

	send(fd, string, strlen(string), 0);
	return(0);
}

int recieve(int *id, int fd, short events, void *data)
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
	
	cheops_sched_del(tid);
	close(sock);
	return 0;
}

int timeout(void *blah)
{
	static int count = 1;
	
	printf("timeout: %d\n", count);
	count++;
	
	cheops_io_remove(id);
	close(sock);
	cheops_io_dump();
	return(0);		
}

int do_it(void *asdf)
{
	int flags;
	struct sockaddr_in sin;
	int res;
	
	inet_aton("127.0.0.1", &sin.sin_addr);
	sin.sin_port = htons(80);
	sin.sin_family = AF_INET;
	
	if( (sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("problems opening the socket %s\n", strerror(errno));
		exit(1);
	}
	if ((flags = fcntl(sock, F_GETFL)) < 0)
	{
		printf("fcntl(F_GETFL) failed(%s)\n", strerror(errno));
		exit(1);
	}
	if (fcntl(sock, F_SETFL, (flags | O_NONBLOCK)) < 0)
	{
		printf("fcntl(F_SETFL) failed(%s)\n", strerror(errno));
		exit(1);
	}
	
	if( (res = connect(sock, (struct sockaddr *)&sin, sizeof(sin))) < 0)
	{
		if(errno == EINPROGRESS)
			printf("have to wait for the socket to connect\n");
		else
		{
			printf("connect error %s\n", strerror(errno));
			exit(1);
		}
	}
	
	id = cheops_io_add(sock, CHEOPS_IO_CB(recieve), POLLIN, NULL);
	cheops_io_add(sock, CHEOPS_IO_CB(recieve_r), POLLOUT, NULL);
	tid = cheops_sched_add(1000, timeout, NULL);

	return(1);
}

int main(int argc, char *argv[])
{
	do_it(NULL);
	
	cheops_sched_add(2000, do_it, NULL);
	
	cheops_io_dump();
	fprintf(stdout,"Scheduled event %d\n", *id);

	cheops_main();
	return 0;
}

