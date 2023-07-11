/*
 * Cheops Next Generation
 * 
 * Brent Priddy <toopriddy@mailcity.com>
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
 */


/* oops dont let this conflict with the real sched.h */
#ifndef _SCHED_H___
#define _SCHED_H___

#ifndef  _GUI_SCHED_H  /* Do Not Redefine If Already Included From gui-x.h */

/*
 * The max number of schedule structs to keep around
 * for use.  Undefine to disable schedule structure
 * caching. (Only disable this on very low memory 
 * machines)
 */
 
#define SCHED_MAX_CACHE 128

/* 
 * A cheops scheduler callback takes a pointer with callback data and
 * returns a 0 if it should not be run again, or non-zero if it should be
 * rescheduled to run again
 */
typedef int (*cheops_sched_cb)(void *data);
#define CHEOPS_SCHED_CB(a) ((cheops_sched_cb)(a))

/* 
 * Schedule an event to take place at some point in the future.  callback 
 * will be called with data as the argument, when milliseconds into the
 * future (approximately)
 */
extern int cheops_sched_add(int when, cheops_sched_cb callback, void *data);

/*
 * Remove this event from being run.  A procedure should not remove its
 * own event, but return 0 instead.
 */
extern int cheops_sched_del(int id);
#endif

/*
 * Determine the number of seconds until the next outstanding event
 * should take place, and return the number of milliseconds until
 * it needs to be run.  This value is perfect for passing to the poll
 * call.  Returns "-1" if there is nothing there are no scheduled events
 * (and thus the poll should not timeout)
 */
extern int cheops_sched_wait();

/*
 * Run the queue, executing all callbacks which need to be performed
 * at this time.  Returns the number of events processed.
 */
extern int cheops_sched_runq();

/*
 * Debugging: Dump the contents of the scheduler to stderr
 */
extern void cheops_sched_dump();

#endif /* _SCHED_H___ */

