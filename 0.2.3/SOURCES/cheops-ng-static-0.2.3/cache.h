/*
 * Cheops Next Generation GUI
 * 
 * cache.h
 * Generic Cache 
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

#ifndef _CACHE_H
#define _CACHE_H


struct cache {
	void	*data;
	struct timeval tv;
	struct cache *next;
};


#define MAX_PING_CACHE_ENTRIES 128

typedef int (*cache_destructor_cb)(void *data);
#define CACHE_DESTRUCTOR_CB(a) ((cache_destructor_cb)(a))

void add_to_cache(void *data, cache_destructor_cb callback);
int in_cache(void *data);

#endif /*_CACHE_H */


