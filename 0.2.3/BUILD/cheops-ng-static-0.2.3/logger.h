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

#ifndef _LOGGER_H
#define _LOGGER_H

#define DEBUG_M(a) { \
	a; \
}

extern void c_log(int level, const char *file, int line, const char *function, const char *fmt, ...);

#define _A_ __FILE__, __LINE__, __PRETTY_FUNCTION__

#define LOG_DEBUG	0, _A_
#define LOG_NOTICE  1, _A_
#define LOG_WARNING 2, _A_
#define LOG_ERROR	3, _A_

#endif

