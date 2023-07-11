/*
 * Cheops Next Generation GUI
 * 
 * gui-sched.c
 * Scheduler Routines (front end for gtk)
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

#include <gtk/gtk.h>
#include "gui-sched.h"

#ifndef  _CHEOPS_SCHED_ADD  /* Do Not Redefine If Already Included From gui-x.h */
#define _CHEOPS_SCHED_ADD
/* 
 * Schedule an event to take place at some point in the future.  callback 
 * will be called with data as the argument, when milliseconds into the
 * future (approximately)
 */
int cheops_sched_add(int when, cheops_sched_cb callback, void *data)
{ 
	return(gtk_timeout_add(when,callback,data));
}
#endif

#ifndef  _CHEOPS_SCHED_DEL  /* Do Not Redefine If Already Included From gui-x.h */
#define _CHEOPS_SCHED_DEL
/*
 * Remove this event from being run.  A procedure should not remove its
 * own event, but return 0 instead.
 */
int cheops_sched_del(int id)
{ 
	gtk_timeout_remove(id);
	return(1);
}
#endif

