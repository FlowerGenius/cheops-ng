/*
 * Cheops Next Generation GUI
 * 
 * agent-osscan.c
 * Operating system discovery code
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
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <glib.h>
#include <sched.h>
#include <semaphore.h>
#include "event.h"
#include "io.h"
#include "sched.h"
#include "cheops-sh.h"
#include "logger.h"
#include "cheops-agent.h"
#include "agent-osscan.h"
#include "ip_utils.h"
#include "cache.h"
#include "agent-nmapxml.h"

#define DEBUG_OS_SCAN

#ifdef DEBUG_OS_SCAN
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

pthread_mutex_t os_list_mutex;
pthread_mutex_t os_read_mutex;
pthread_mutex_t os_write_mutex;
pthread_t os_detect_thread;
sem_t os_detect_go_sem;

static char ebuf_os[MAX_EVENT_SIZE];
static event_hdr *eh_os = (event_hdr *)ebuf_os;
static event *ee_os = (event *)(ebuf_os + sizeof(event_hdr));

typedef struct _os_detect_queue {
	char *name;
	void *np;
	void *agent;
	unsigned int flags;
	char *ports;
} os_detect_queue;

GList *os_detect_q = NULL;
int os_detect_q_outstanding = 0;

void *dequeue_osscan(void *arg);
int do_osscan (char *name, void *np, void *agent);
void enqueue_osscan(char *name, void *np, void *agent, char *ports, unsigned int flags);
int osscan_timer(void *data);
void agent_osscan_send_event(agent *a, event_hdr *e);

event_hdr *event_to_send = NULL;
agent *agent_to_send = NULL;

char *scan_ports;
unsigned int scan_flags;

int handle_osscan_request(event_hdr *h, event *e, agent *a)
{
	char	*name = ip2str(ntohl(e->os_scan_e.ip));

	DEBUG(printf("%s(): enqueueing scan\n", __FUNCTION__));	
	enqueue_osscan(name,
	               e->os_scan_e.np ,
	               a,
	               END_OF_OPTION(&(e->os_scan_e)),
	               ntohl(e->os_scan_e.options));
	return(0);
}

void init_osscan(void)
{
	DEBUG(printf("%s(): Initalizing\n", __FUNCTION__));
	pthread_mutex_init(&os_list_mutex, NULL);
	pthread_mutex_init(&os_read_mutex, NULL);
	pthread_mutex_init(&os_write_mutex, NULL);
	
	pthread_mutex_lock(&os_read_mutex);
	
	sem_init(&os_detect_go_sem, 0, 0);
	
	cheops_sched_add(800, osscan_timer, NULL);
		
	pthread_create(&os_detect_thread, NULL, dequeue_osscan, NULL);
}

void enqueue_osscan(char *name, void *np, void *agent, char *ports, unsigned int flags)
{
	os_detect_queue *q;

	DEBUG(printf("%s(): enqueueing %s\n", __FUNCTION__, name));

	if(name && np && agent)
	{
		q = malloc(sizeof(os_detect_queue));
		if(!q)
			return;
		
		q->name = strdup(name);
		q->np = np;
		q->agent = agent;
		q->flags = flags;
		q->ports = strdup(ports);
		
		pthread_mutex_lock(&os_list_mutex);
		os_detect_q_outstanding++;

		DEBUG(printf("%s(): enqueueing %s aquired list mutex\n", __FUNCTION__, name));
		DEBUG(printf("%s(): %d outstanding entries\n", __FUNCTION__, os_detect_q_outstanding));
		os_detect_q = g_list_append(os_detect_q, q);
		
		pthread_mutex_unlock(&os_list_mutex);

		sem_post(&os_detect_go_sem);
	}
}

void send_osscan_reply(nmap_scan_t *n, void *np, agent *a)
{
	int i, length, len;
	char *opt;
	os_scan_option_os *os_opt;
	os_scan_option_port *os_port;
	os_scan_option_uptime *os_uptime;
	GList *gl;
	nmap_host_port_t *port;

	pthread_mutex_lock(&os_write_mutex);

	length = sizeof(event_hdr) + sizeof(os_scan_r);

	eh_os->hlen = htons (sizeof (event_hdr));
	eh_os->type = htons (REPLY_OS_SCAN);
	eh_os->flags = 0;
	ee_os->os_scan_r.ip = inet_addr(n->host.addr);
	ee_os->os_scan_r.np = np;

	DEBUG(printf("%s(): for ip address '%s'\n", __FUNCTION__, n->host.addr));
	
	opt = (void *) ee_os + sizeof (os_scan_r);
	i = 0;
	if(n->os.name)
	{
		os_opt = (void *) opt;
		os_opt->type = htonl (OS_SCAN_OPTION_OS);
		strcpy (END_OF_OPTION(os_opt), (char *) n->os.name);

		len = sizeof(*os_opt) + strlen(n->os.name) + 1;
		length += len;

		os_opt->length = htonl(len);

		/* advance to the next option */
		opt += len;
		i++;
		DEBUG(printf("%s(): sending os type '%s'\n", __FUNCTION__, n->os.name));
	}
	

	gl = n->host.ports;
	while (gl != NULL)
	{
		port = gl->data;
		
		len = sizeof (*os_port);
		os_port = (void *) opt;
		os_port->type = htonl (OS_SCAN_OPTION_PORT);
		
		os_port->protocol = htonl(port->protocol);
		os_port->state = htonl(port->state);
		os_port->port_number = htonl(port->portid);
		os_port->proto = htonl(port->proto);
		os_port->rpcnum = htonl(port->rpcnum);
		os_port->rpclowver = htonl(port->rpclowver);
		os_port->rpchighver = htonl(port->rpchighver);
		
		if(port->service)
		{
			strcpy( END_OF_OPTION(os_port), port->service);
			len += strlen(port->service) + 1;
		}
		else
		{
			*END_OF_OPTION(os_port) = '\0';
			len += 1;
		}
		
		if(port->owner)
		{
			strcpy( ((char *)os_port) + len, port->owner);
			len += strlen(port->owner) + 1;
		}
		else
		{
			*(((char *)os_port) + len) = '\0';
			len += 1;
		}

		os_port->length = htonl (len);

		gl = gl->next;

		/* advance to the next option */
		length += len;
		opt += len;
		i++;
	}

	if(n->host.uptime_seconds)
	{
		os_uptime = (void *)opt;
		len = sizeof (*os_uptime);
		os_uptime = (void *)opt;
		os_uptime->type = htonl(OS_SCAN_OPTION_UPTIME);
		
		os_uptime->seconds = htonl(atoi(n->host.uptime_seconds));
		if(n->host.uptime_last_boot)
		{
			strcpy(END_OF_OPTION(os_uptime), n->host.uptime_last_boot);
			len += strlen(n->host.uptime_last_boot) + 1;
		}
		else
		{
			*END_OF_OPTION(os_uptime) = '\0';
			len += 1;
		}
		
		os_uptime->length = htonl (len);

		length += len;
		opt += len;
		i++;
	}	
	DEBUG(printf("%s(): sending %d options\n", __FUNCTION__, i));

	ee_os->os_scan_r.num_options = htonl (i);
	eh_os->len = htonl (length);

	pthread_mutex_unlock(&os_write_mutex);

	agent_osscan_send_event(a, eh_os);
}

void *dequeue_osscan(void *arg)
{
	nmap_scan_t *n;
	os_detect_queue *q = NULL;
	GList *gl;
	
	while(1)
	{
		DEBUG(printf("%s(): dequeue going to sleep\n", __FUNCTION__));
		sem_wait(&os_detect_go_sem);
		DEBUG(printf("%s(): dequeue woken up\n", __FUNCTION__));

		pthread_mutex_lock(&os_list_mutex);
		DEBUG(printf("%s(): dequeue aquired mutex \n", __FUNCTION__));
		if( (gl = g_list_first(os_detect_q)) )
		{
			if( (q = gl->data) )
			{
				os_detect_q_outstanding--;
				DEBUG(printf("%s(): %d outstanding entries\n", __FUNCTION__, os_detect_q_outstanding));
				os_detect_q = g_list_remove(os_detect_q, q);
			}
			else
			{
				DEBUG(printf("%s(): dequeue'ed a NULL data pointer\n", __FUNCTION__));
			}
		}
		else
		{
			DEBUG(printf("%s(): dequeue did not dequeue anything\n", __FUNCTION__));
		}
		DEBUG(printf("%s(): dequeue released mutex \n", __FUNCTION__));
		pthread_mutex_unlock(&os_list_mutex);
		
		if(gl == NULL || q == NULL)
		{
			continue;
		}
		else
		{
			DEBUG(printf("%s(): starting os scan on %s\n", __FUNCTION__, q->name));
				
			n = nmap_scan(q->name, q->flags, q->ports);
			if(n)
			{
				send_osscan_reply(n, q->np, q->agent);
				DEBUG(nmap_scan_print(n));
				nmap_scan_free(n);
			}
			else
			{
				DEBUG(fprintf(stderr, "host not there\n"));
			}
			
			DEBUG(printf("%s(): finished os scan on %s\n", __FUNCTION__, q->name));

			if(q->name)
				free(q->name);
			if(q->ports)
				free(q->ports);
			if(q)
				free(q);
		}
	}
	printf("%s(): I returned?!?!?\b\n", __FUNCTION__);
	return(NULL);
}

void agent_osscan_send_event(agent *a, event_hdr *e)
{
	event_to_send = e;
	agent_to_send = a;
	pthread_mutex_unlock(&os_read_mutex);
}

int osscan_timer(void *data)
{
	if( EBUSY != pthread_mutex_trylock(&os_read_mutex) )
	{
		pthread_mutex_lock(&os_write_mutex);
		if (event_send (agent_to_send, event_to_send) < 0)
		{
			clog (LOG_WARNING, "Unable to send osscan reply\n");
		}
		pthread_mutex_unlock(&os_write_mutex);
	}
	return(1); // keep it running
}
