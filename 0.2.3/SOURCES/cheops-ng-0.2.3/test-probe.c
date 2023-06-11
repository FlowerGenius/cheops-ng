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
#include "probe.h"

int *id;
int tid;
int sock;

void print_callback(unsigned short port, char *version, void *user_data)
{
	printf("Version recieved:\n port %d\n%s\n", port, version);
}

int do_it(void *arg)
{
	int i;
	int addr = inet_addr("127.0.0.1");
	
	for(i = 0; i < 1025; i++)
	{
		get_version(addr, i, 1000, print_callback, NULL);
	}
	
	get_version(addr, 5901, 1000, print_callback, NULL);
	return(1);
}

int main(int argc, char *argv[])
{
	init_probes();

	do_it(NULL);
		
	cheops_sched_add(5000, do_it, NULL);
	

	cheops_main();
	return 0;
}

