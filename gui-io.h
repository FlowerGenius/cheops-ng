/*
 * Cheops Next Generation GUI
 * 
 * gui-io.h
 * I/O Managment shim between the cheops io and gtk's io
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

#ifndef _GUI_IO_H
#define _GUI_IO_H

#include <sys/poll.h>		/* For POLL* constants */

#define CHEOPS_IO_IN 	GDK_INPUT_READ		/* Input ready */
#define CHEOPS_IO_OUT 	GDK_INPUT_WRITE 	/* Output ready */
#define CHEOPS_IO_PRI	CHEOPS_IO_IN 		/* Priority input ready */

/* Implicitly polled for */
#define CHEOPS_IO_ERR	0	 	/* Error condition (errno or getsockopt) */
#define CHEOPS_IO_HUP	0	 	/* Hangup */
#define CHEOPS_IO_NVAL	0		/* Invalid fd */

/*
 * A Cheops IO callback takes its id, a file descriptor, list of events, and
 * callback data as arguments and returns 0 if it should not be
 * run again, or non-zero if it should be run again.
 */

typedef int (*cheops_io_cb)(int *id, int fd, short events, void *cbdata);
#define CHEOPS_IO_CB(a) ((cheops_io_cb)(a))

/* 
 * Watch for any of revents activites on fd, calling callback with data as 
 * callback data.  Returns a pointer to ID of the IO event, or NULL on failure.
 */
extern int *cheops_io_add(int fd, cheops_io_cb callback, short events, void *data);

/* 
 * Remove an I/O id from consideration  Returns 0 on success or -1 on failure.
 */
extern int cheops_io_remove(int *id);

#endif /* _GUI_IO_H */

