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
#include "cheops.h"
#include "io.h"
#include "sched.h"

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

int hello2(void *blah)
{
	char *b = blah;
	fprintf(stderr, "%s:(repeat)!\n", b);
	return 1;
}

int main(int argc, char *argv[])
{
	int *id;
	id = cheops_io_add(0, CHEOPS_IO_CB(hello), POLLIN, "Hello World!");
	cheops_sched_add(1000, hello2, "blah");
	cheops_io_dump();
	fprintf(stdout,"Scheduled event %d\n", *id);
	cheops_main();
	return 0;
}

