/*
 * Cheops Next Generation GUI
 * 
 * cache.c
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
 
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include "cache.h"
#include "logger.h"

struct cache *Cache = NULL;

#ifdef DEBUG_CACHE
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

void add_to_cache(void *data, cache_destructor_cb callback)
{
	struct cache *p,*old;
	static int count=0;
	struct timezone tz;
	int newone=0;
		
	if(count >= MAX_PING_CACHE_ENTRIES)
	{
		DEBUG( c_log(LOG_NOTICE, "Cache is at MAX, removing an entry\n") );
		for(p=Cache,old=Cache;p;p = p->next)
		{
			if((old->tv.tv_sec*1000 + old->tv.tv_usec/1000) < (p->tv.tv_sec*1000 + p->tv.tv_usec/1000))
				old = p;
		}
		if(callback)
			callback(data);
		p = old;
	}
	else
	{
		if((p = malloc(sizeof(struct cache)))==NULL)
		{
			DEBUG( c_log(LOG_ERROR,"malloc returned NULL\n") );
			exit(1);
		}
		count++;
		newone=1;
	}
	
	p->data = data;
	gettimeofday(&(p->tv),&tz);

	if(newone)
	{
		p->next = Cache;
		Cache = p;
	}
	
}

int in_cache(void *data)
{
	struct cache *p;
	
	for(p=Cache;p;p=p->next)
		if(p->data == data)
		{
			return(1);
		}
	return(0);
}

