/*
 * Cheops Next Generation GUI
 * 
 * cheops-main.c
 * Main Functions
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
#include <errno.h>
#include <string.h>
#include <sys/poll.h>	/* Remove me */
#include "cheops.h"
#include "logger.h"
#include "sched.h"
#include "event.h"
#include "io.h"
#include "misc.h"
#include "logger.h"
 
 
int cheops_init()
{
	/*
	 * Call all sub initialization
	 * routines
	 */
	make_home_dir();
	return 0;
}
int cheops_main()
{
	/*
	 * This is the main loop which looks for events and
	 * handles the scheduler, etc
	 */
	int waittime;
	
	for(;;) {
		waittime = cheops_sched_wait();
		cheops_io_wait(waittime);
		cheops_sched_runq();
	}
	return 0;
}


