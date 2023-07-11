/*
 * Cheops Next Generation GUI
 * 
 * io.c
 * I/O Managment
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
#include <sys/poll.h>
#include <unistd.h>
#include <stdlib.h>
#include "io.h"
#include "logger.h"

//#define DEBUG_IO

#ifdef DEBUG_IO
	#define DEBUG(a) a
#else
	#define DEBUG(a) 
#endif

/* 
 * Kept for each file descriptor
 */
struct io_rec {
	cheops_io_cb callback;		/* What is to be called */
	void *data; 				/* Data to be passed */
	int *id; 					/* ID number */
};

/* These two arrays are keyed with
   the same index.  it's too bad that
   pollfd doesn't have a callback field
   or something like that.  They grow as
   needed, by GROW_SHRINK_AMOUNT structures
   at once */

#define GROW_SHRINK_SIZE 512

static struct pollfd *fds = NULL;
static struct io_rec *ior = NULL;

/* First available fd */
static unsigned int fdcnt = 0;
/* Maximum available fd */
static unsigned int maxfdcnt = -1;
/* Currently used io callback */
static int current_ioc = -1;
static int remove_current_ioc = 0;

static int io_grow()
{
	/* 
	 * Grow the size of our arrays.  Return 0 on success or
	 * -1 on failure
	 */
	void *tmp;
	DEBUG(c_log(LOG_DEBUG, "io_grow()\n"));
	maxfdcnt += GROW_SHRINK_SIZE;
	tmp = realloc(ior, (maxfdcnt + 1) * sizeof(struct io_rec));
	if (tmp) {
		ior = (struct io_rec *)tmp;
		tmp = realloc(fds, (maxfdcnt + 1) * sizeof(struct pollfd));
		if (tmp) {
			fds = tmp;
		} else {
			/*
			 * Not enough memory for the pollfd.  Not really any need
			 * to shrink back the iorec's as we'll probably want to
			 * grow them again soon when more memory is available, and
			 * then they'll already be the right size
			 */
			maxfdcnt -= GROW_SHRINK_SIZE;
			return -1;
		}
		
	} else {
		/*
		 * Out of memory.  We return to the old size, and return a failure
		 */
		maxfdcnt -= GROW_SHRINK_SIZE;
		return -1;
	}
	return 0;
}

#ifndef  _CHEOPS_IO_ADD  /* Do Not Redefine If Already Included From gui-x.h */
#define _CHEOPS_IO_ADD
int *cheops_io_add(int fd, cheops_io_cb callback, short events, void *data)
{
	/*
	 * Add a new I/O entry for this file descriptor
	 * with the given event mask, to call callback with
	 * data as an argument.  Returns NULL on failure.
	 */
	DEBUG(c_log(LOG_DEBUG, "cheops_io_add()\n"));
	if (fdcnt < maxfdcnt) {
		/* 
		 * We don't have enough space for this entry.  We need to
		 * reallocate maxfdcnt poll fd's and io_rec's, or back out now.
		 */
		if (io_grow())
			return NULL;
	}

	/*
	 * At this point, we've got sufficiently large arrays going
	 * and we can make an entry for it in the pollfd and io_r
	 * structures.
	 */
	fds[fdcnt].fd = fd;
	fds[fdcnt].events = events;
	fds[fdcnt].revents = 0;
	ior[fdcnt].callback = callback;
	ior[fdcnt].data = data;
	ior[fdcnt].id = (int *)malloc(sizeof(int));
	/* Bonk if we couldn't allocate an int */
	if (!ior[fdcnt].id)
		return NULL;
	*ior[fdcnt].id = fdcnt;
	return ior[fdcnt++].id;
}
#endif

int *cheops_io_change(int *id, int fd, cheops_io_cb callback, short events, void *data)
{
	if (*id < fdcnt) {
		if (fd > -1)
			fds[*id].fd = fd;
		if (callback)
			ior[*id].callback = callback;
		if (events)
			fds[*id].events = events;
		if (data)
			ior[*id].data = data;
		return id;
	} else return NULL;
}

static int io_shrink(int which)
{
	/* 
	 * Bring the fields from the very last entry to cover over
	 * the entry we are removing, then decrease the size of the 
	 * arrays by one.
	 */
	fdcnt--;

	/* Free the int */
	free(ior[which].id);
	
	/* If we're not deleting the last one, move the last one to
	   the current position */
	if (which != fdcnt) {
		fds[which] = fds[fdcnt];
		ior[which] = ior[fdcnt];
		*ior[which].id = which;
	}
	/* FIXME: We should free some memory if we have lots of unused
	   io structs */
	return 0;
}

#ifndef  _CHEOPS_IO_REMOVE  /* Do Not Redefine If Already Included From gui-x.h */
#define _CHEOPS_IO_REMOVE
int cheops_io_remove(int *id)
{
	if (current_ioc == *id) 
	{
		DEBUG(c_log(LOG_NOTICE, "Callback for %d tried to remove itself\n", *id));
		remove_current_ioc = 1;
	} 
	else
	{
		if (*id < fdcnt)
		{
			return(io_shrink(*id));
		}
		else 
		{
			DEBUG(c_log(LOG_NOTICE, "Unable to remove unknown id %d\n", *id));
		}
	}

	return -1;
}
#endif

int cheops_io_wait(int howlong)
{
	/*
	 * Make the poll call, and call
	 * the callbacks for anything that needs
	 * to be handled
	 */
	int res;
	int x;
	DEBUG(c_log(LOG_DEBUG, "cheops_io_wait()\n"));
	res = poll(fds, fdcnt, howlong); // if we are using pth this is a pth call
	if (res > 0) {
		/*
		 * At least one event
		 */
		for(x=0;x<fdcnt;x++) 
		{
			if (fds[x].revents) 
			{
				/* There's an event waiting */
				current_ioc = *ior[x].id;
				remove_current_ioc = 0;
				if (!ior[x].callback(ior[x].id, fds[x].fd, fds[x].revents, ior[x].data) || remove_current_ioc) 
				{
					/* Time to delete them since they returned a 0 */
					io_shrink(x);
				}
				current_ioc = -1;
			}
		}
	}
	return(res);
}

void cheops_io_dump()
{
	/*
	 * Print some debugging information via
	 * the logger interface
	 */
	int x;
	c_log(LOG_DEBUG, "Cheops IO Dump: %d entries, %d max entries\n", fdcnt, maxfdcnt);
	c_log(LOG_DEBUG, "================================================\n");
	c_log(LOG_DEBUG, "| ID    FD     Callback    Data        Events  |\n");
	c_log(LOG_DEBUG, "+------+------+-----------+-----------+--------+\n");
	for (x=0;x<fdcnt;x++) {
		c_log(LOG_DEBUG, "| %.4d | %.4d | %p | %p | %.6x |\n", 
				*ior[x].id,
				fds[x].fd,
				ior[x].callback,
				ior[x].data,
				fds[x].events);
	}
	c_log(LOG_DEBUG, "================================================\n");
}

