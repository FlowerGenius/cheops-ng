/*
 * Cheops Next Generation GUI
 * 
 * cerror.c
 * Error handling routines
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

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#ifdef FREEBSD
#include <sys/types.h>
#include <netinet/in_systm.h>
#endif
#include <netinet/in.h>
#include "logger.h"
#include "event.h"
#include "cerror.h"
#include "string.h"

void cheops_error(agent *a, u16 error, char *fmt, ...)
{
	static char buf[65536];
	static event_hdr *h = (event_hdr *)buf;
	static error_r *er = (error_r *)(buf + sizeof(event_hdr));
	static char *c = buf + sizeof(event_hdr) + sizeof(error_r);
	va_list ap;

		
	va_start(ap, fmt);
	vsnprintf(c, 
			  sizeof(buf) - sizeof(event_hdr) - sizeof(error_r), 
			  		fmt, ap);
	er->len = htons(strlen(c));
	er->error = htons(error);
	h->len = htonl(strlen(c) + sizeof(event_hdr) + sizeof(error_r));
	h->hlen = htons(sizeof(event_hdr));
	h->type = htons(REPLY_ERROR);
	h->flags = htonl(FLAG_IGNORE);
	if (event_send(a, h) < 0) 
		c_log(LOG_DEBUG, "Unable to send error '%s'\n", c);
	va_end(ap);
}

