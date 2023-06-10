/*
 * Cheops Next Generation GUI
 * 
 * logger.c
 * Logging routines
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

#include <stdarg.h>
#include <stdio.h>
#include "logger.h"

static char *levels[] = {
	"DEBUG",
	"NOTICE",
	"WARNING",
	"ERROR"
};

extern void clog(int level, const char *file, int line, const char *function, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stdout, "%s: File %s, Line %d (%s): ", levels[level], file, line, function);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

