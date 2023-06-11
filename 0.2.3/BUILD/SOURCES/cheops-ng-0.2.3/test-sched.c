/*
 * Cheops Next Generation GUI
 * 
 * test-sched.c
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

#include "cheops.h"
#include "sched.h"
#include <stdio.h>

int hello(char *blah)
{
	fprintf(stderr, "%s:(once only)!\n", blah);
	return 0;
}

int hello2(char *blah)
{
	fprintf(stderr, "%s:(repeat)!\n", blah);
	return 1;
}

int main(int argc, char *argv[])
{
	int id;
	id = cheops_sched_add(1000, CHEOPS_SCHED_CB(hello2), "Hello World!");
	id = cheops_sched_add(5000, CHEOPS_SCHED_CB(hello), "Hello World!");
	id = cheops_sched_add(2000, CHEOPS_SCHED_CB(hello), "Hello World!");
	fprintf(stdout,"Scheduled event %d\n", id);
	cheops_sched_dump();
	cheops_main();
	return 0;
}

