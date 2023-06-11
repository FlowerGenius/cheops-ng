/*
 * Cheops Next Generation GUI
 * 
 * sched.c
 * Scheduler Routines
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
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "sched.h"
#include "logger.h"
#ifdef USE_PTHREAD
	#include <pthread.h>
#endif

#ifdef USE_PTHREAD
	#undef USE_PTHREAD
	#define USE_PTHREAD(a) a
#else
	#define USE_PTHREAD(a)
#endif

/* Determine if a is sooner than b */
#define SOONER(a,b) (((b).tv_sec > (a).tv_sec) || \
					 (((b).tv_sec == (a).tv_sec) && ((b).tv_usec > (a).tv_usec)))

#ifdef DEBUG_SCHEDULER
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

/* if we are using pthread muticies make one here for the sched stuff */
USE_PTHREAD(pthread_mutex_t sched_mutex = PTHREAD_MUTEX_INITIALIZER);

/* Number of events processed */
static int eventcnt=1;

/* Number of outstanding schedule events */
static int schedcnt=0;

/* Schedule entry and main queue */
static struct sched {
	struct sched *next;				/* Next event in the list */
	int id; 						/* ID number of event */
	struct timeval when;			/* Absolute time event should take place */
	int resched;					/* When to reschedule */
	void *data; 					/* Data */
	cheops_sched_cb callback;		/* Callback */
} *schedq = NULL;

#ifdef SCHED_MAX_CACHE
/* Cache of unused schedule structures and how many */
static struct sched *schedc = NULL;
static int schedccnt=0;

/* mutex for the cache stuff */
USE_PTHREAD(pthread_mutex_t sched_cache_mutex = PTHREAD_MUTEX_INITIALIZER);
#endif

static struct sched *sched_alloc()
{
	/*
	 * We keep a small cache of schedule entries
	 * to minimize the number of necessary malloc()'s
	 */
	struct sched *tmp;
#ifdef SCHED_MAX_CACHE
	if (schedc) {
USE_PTHREAD(pthread_mutex_lock(&sched_cache_mutex));
		tmp = schedc;
		schedc = schedc->next;
		schedccnt--;
USE_PTHREAD(pthread_mutex_unlock(&sched_cache_mutex));
	} else
#endif
		tmp = malloc(sizeof(struct sched));
	return tmp;
}

static void sched_release(struct sched *tmp)
{
	/*
	 * Add to the cache, or just free() if we
	 * already have too many cache entries
	 */

#ifdef SCHED_MAX_CACHE	 
	if (schedccnt < SCHED_MAX_CACHE) {
USE_PTHREAD(pthread_mutex_lock(&sched_cache_mutex));
		tmp->next = schedc;
		schedc = tmp;
		schedccnt++;
USE_PTHREAD(pthread_mutex_unlock(&sched_cache_mutex));
	} else
#endif
		free(tmp);
}

int cheops_sched_wait()
{
	/*
	 * Return the number of milliseconds 
	 * until the next scheduled event
	 */
	struct timeval tv;
	int ms;
	DEBUG(c_log(LOG_DEBUG, "cheops_sched_wait()\n"));
	if (!schedq)
		return -1;
	if (gettimeofday(&tv, NULL) < 0) {
		/* This should never happen */
		return 0;
	};
USE_PTHREAD(pthread_mutex_lock(&sched_mutex));
	ms = (schedq->when.tv_sec - tv.tv_sec) * 1000;
	ms += (schedq->when.tv_usec - tv.tv_usec) / 1000;
USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));
	if (ms < 0)
		ms = 0;
	return ms;
	
}


static void schedule(struct sched *s)
{
	/*
	 * Take a sched structure and put it in the
	 * queue, such that the soonest event is
	 * first in the list. 
	 */
	 
	struct sched *last=NULL;
	struct sched *current=schedq;
	while(current) {
		if (SOONER(s->when, current->when))
			break;
		last = current;
		current = current->next;
	}
	/* Insert this event into the schedule */
	s->next = current;
	if (last) 
		last->next = s;
	else
		schedq = s;
	schedcnt++;
}

static inline int sched_settime(struct timeval *tv, int when)
{
	if (gettimeofday(tv, NULL) < 0) {
			/* This shouldn't ever happen, but let's be sure */
			c_log(LOG_NOTICE, "gettimeofday() failed!\n");
			return -1;
	}
	tv->tv_sec += when/1000;
	tv->tv_usec += (when % 1000) * 1000;
	if (tv->tv_usec > 1000000) {
		tv->tv_sec++;
		tv->tv_usec-= 1000000;
	}
	return 0;
}

int cheops_sched_add(int when, cheops_sched_cb callback, void *data)
{
	/*
	 * Schedule callback(data) to happen when ms into the future
	 */
	struct sched *tmp;
	DEBUG(c_log(LOG_DEBUG, "cheops_sched_add()\n"));
	if (!when) {
		c_log(LOG_NOTICE, "Scheduled event in 0 ms?");
		return -1;
	}
	if ((tmp = sched_alloc())) {
		tmp->id = eventcnt++;
		tmp->callback = callback;
		tmp->data = data;
		tmp->resched = when;
		
USE_PTHREAD(pthread_mutex_lock(&sched_mutex));
		if (sched_settime(&tmp->when, when)) {
			sched_release(tmp);
USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));
			return -1;
		} else
			schedule(tmp);
USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));
	} else 
		return -1;
	return tmp->id;
}

int cheops_sched_del(int id)
{
	/*
	 * Delete the schedule entry with number
	 * "id".  It's nearly impossible that there
	 * would be two or more in the list with that
	 * id.
	 */
	struct sched *last=NULL, *s;
	DEBUG(c_log(LOG_DEBUG, "cheops_sched_del()\n"));
USE_PTHREAD(pthread_mutex_lock(&sched_mutex));
	s = schedq;
	while(s) {
		if (s->id == id) {
			if (last)
				last->next = s->next;
			else
				schedq = s->next;
			schedcnt--;
USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));
			return 0;
		}
		last = s;
		s = s->next;
	}
USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));
	
	c_log(LOG_NOTICE, "Attempted to delete non-existant schedule entry %d!\n", id);
	return -1;
}

void cheops_sched_dump()
{
	/*
	 * Dump the contents of the scheduler to
	 * stderr
	 */
	struct sched *q;
	struct timeval tv;
	time_t s, ms;
	gettimeofday(&tv, NULL);
	c_log(LOG_DEBUG, "Cheops Schedule Dump (%d in Q, %d Total, %d Cache)\n", 
							 schedcnt, eventcnt - 1, schedccnt);
	c_log(LOG_DEBUG, "=================================================\n");
	c_log(LOG_DEBUG, "|ID    Callback    Data        Time  (sec:ms)   |\n");
	c_log(LOG_DEBUG, "+-----+-----------+-----------+-----------------+\n");
USE_PTHREAD(pthread_mutex_lock(&sched_mutex));
	q = schedq;
	while(q) {
		s =  q->when.tv_sec - tv.tv_sec;
		ms = q->when.tv_usec - tv.tv_usec;
		if (ms < 0) {
			ms += 1000000;
			s--;
		}
		c_log(LOG_DEBUG, "|%.4d | %p | %p | %.6ld : %.6ld |\n", 
				q->id,
				q->callback,
				q->data,
				s,
				ms);
		q=q->next;
	}
USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));
	c_log(LOG_DEBUG, "=================================================\n");
	
}

int cheops_sched_runq()
{
	/*
	 * Launch all events which need to be run at this time.
	 */
	struct sched *current;
	struct timeval tv;
	int x=0;
	DEBUG(c_log(LOG_DEBUG, "cheops_sched_runq()\n"));
		
	for(;;) {
		if (!schedq)
			break;
		if (gettimeofday(&tv, NULL)) {
			/* This should never happen */
			c_log(LOG_NOTICE, "gettimeofday() failed!\n");
			return 0;
		}
		/* We only care about millisecond accuracy anyway, so this will
		   help us get more than one event at one time if they are very
		   close together. */
		tv.tv_usec += 1000;

USE_PTHREAD(pthread_mutex_lock(&sched_mutex));
		if (SOONER(schedq->when, tv)) {
			current = schedq;
			schedq = schedq->next;
			schedcnt--;
USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));

			/*
			 * At this point, the schedule queue is still intact.  We
			 * have removed the first event and the rest is still there,
			 * so it's permissible for the callback to add new events, but
			 * trying to delete itself won't work because it isn't in
			 * the schedule queue.  If that's what it wants to do, it 
			 * should return 0.
			 */
			if (current->callback(current->data)) 
			{
	USE_PTHREAD(pthread_mutex_lock(&sched_mutex));
			 	/*
				 * If they return non-zero, we should schedule them to be
				 * run again.
				 */
				if (sched_settime(&current->when, current->resched)) {
					sched_release(current);
				} else {
					schedule(current);
				}
	USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));
			} else {
				/* No longer needed, so release it */
	USE_PTHREAD(pthread_mutex_lock(&sched_mutex));
				sched_release(current);
	USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));
			}
			x++;
		} else {
USE_PTHREAD(pthread_mutex_unlock(&sched_mutex));
			break;
		}
	}
	return x;
}

