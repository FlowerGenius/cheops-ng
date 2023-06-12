/*
 * Cheops Next Generation GUI
 * 
 * probe.c
 * IP service detection routines
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
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "cheops.h"
#include "probe.h"
#include "sched.h"

#ifdef COMPILING_GUI
	#include "gui-io.h"
#else
	#include "io.h"
#endif

//#define DEBUG_PROBE

#ifdef DEBUG_PROBE
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

#ifdef USE_PTHREAD
	#include <pthread.h>
	#undef USE_PTHREAD
	#define USE_PTHREAD(a) a
#else
	#undef USE_PTHREAD
	#define USE_PTHREAD(a)
#endif

USE_PTHREAD(pthread_mutex_t probe_mutex);

/* Probing routines, may be useful for monitoring as well */

GList *probes = NULL;


static struct probe probesa[] = {
	{ 21,   NULL,      NULL,                    strip_version, (void *)1 }, //FTP//
	{ 22,   NULL,      NULL,                    strip_version, (void *)0 }, //SSH//
//	{ 23,   NULL,      NULL,                    strip_newline, (void *)3 }, //Telnet//
	{ 25,   NULL,      NULL,                    strip_version, (void *)1 }, //E-mail//
	{ 80,   send_text, "GET / HTTP/1.0\n\n",    after_text,    "Server: "}, //Web//
	{ 110,  NULL,      NULL,                    strip_version, (void *)2 }, //POP3//
	{ 119,  NULL,      NULL,                    strip_version, (void *)1 }, //News//
	{ 143,  NULL,      NULL,                    strip_version, (void *)2 }, //IMAP//
	{ 444,  NULL,      NULL,                    strip_version, (void *)0 }, //SNPP//
	{ 5900, NULL,      NULL,                    strip_version, (void *)0 }, //VNC(0)//
	{ 5901, NULL,      NULL,                    strip_version, (void *)0 }, //VNC(1)//
	{ 5902, NULL,      NULL,                    strip_version, (void *)0 }, //VNC(2)//
	{ 5903, NULL,      NULL,                    strip_version, (void *)0 }, //VNC(3)//
	{ 5904, NULL,      NULL,                    strip_version, (void *)0 }, //VNC(4)//
	{ 5905, NULL,      NULL,                    strip_version, (void *)0 }, //VNC(5)//
	{ 5906, NULL,      NULL,                    strip_version, (void *)0 }, //VNC(6)//
	{ 5907, NULL,      NULL,                    strip_version, (void *)0 }, //VNC(7)//  
};

static int probe_cnt = sizeof(probesa) / sizeof(struct probe);

void get_probe_each(gpointer data, gpointer user_data)
{
	uintptr_t *args = (uintptr_t *)user_data;
	uintptr_t port = args[0];
	struct probe *p = (struct probe *)data;
	struct probe **pp = (struct probe **)args[1];
	
//	DEBUG(printf("%s()\n", __FUNCTION__));
	if(*pp == NULL)
	{
		if( p->port == (u16)port)
			*pp = p;
	}
}

struct probe *get_probe(unsigned short port)
{
	uintptr_t args[2];
	struct probe *p = NULL;
	
	DEBUG(printf("%s()\n", __FUNCTION__));
	args[0] = port;
	args[1] = (uintptr_t)&p;
	
	g_list_foreach(probes, get_probe_each, args);
	
	return(p);		
}

void remove_probe(struct probe *p)
{
	DEBUG(printf("%s()\n", __FUNCTION__));
	if(p)
	{
USE_PTHREAD(pthread_mutex_lock(&probe_mutex));
		probes = g_list_remove(probes, p);
USE_PTHREAD(pthread_mutex_unlock(&probe_mutex));
		free(p);
	}
}

void register_probe(unsigned short port, probe_send_cb send, void *send_arg, probe_recieve_cb recieve, void *recieve_arg)
{
	struct probe *p;
	
	DEBUG(printf("%s()\n", __FUNCTION__));
	if(port > 0)
	{
		if( (p = get_probe(port)) )
			remove_probe(p);
			
		if( (p = malloc(sizeof(*p))) == NULL)
		{
			printf("EEK out of mem\n");
			exit(1);
		}
		p->port = port;
		p->send = send;
		p->send_arg = send_arg;
		p->recieve = recieve;
		p->recieve_arg = recieve_arg;
		
USE_PTHREAD(pthread_mutex_lock(&probe_mutex));
		probes = g_list_append(probes, p);
USE_PTHREAD(pthread_mutex_unlock(&probe_mutex));
	}
}

void init_probes(void)
{
	int x;
	DEBUG(printf("%s()\n", __FUNCTION__));

USE_PTHREAD(pthread_mutex_init(&probe_mutex, NULL));

	for (x = 0; x < probe_cnt; x++) 
		register_probe(probesa[x].port, probesa[x].send, probesa[x].send_arg, probesa[x].recieve, probesa[x].recieve_arg);
}

char *after_text(int fd, void *arg, void *user_data)
{
	char *c, *d;
	char *after = arg;
	
	DEBUG(printf("%s()\n", __FUNCTION__));
	c = get_text(fd, arg, user_data);
	
	if(c) 
	{
		c = strstr(c, after);
		if(c)
		{
			d = strchr(c, '\n');
			if(d)
				*d = '\0';
			return(c + strlen(after));
		}
	}
	
	return(NULL);
}

void send_text(int fd, void *arg, void *user_data)
{
	DEBUG(printf("%s()\n", __FUNCTION__));
	if(arg)
		send(fd, arg, strlen(arg), 0);
}

char *get_text(int fd, void *arg, void *user_data)
{
/* yes i know there is a potential problem with recursion in this function
 * but the solution is to not have the probe running in > 1 threads :)
 * (i am talking about the static char buf[1024])
 */
	static char buf[2048];
	int res;
	DEBUG(printf("%s()\n", __FUNCTION__));

	res = read(fd, buf, sizeof(buf));
	
	if (res >= 0) 
	{
		buf[res]='\0';
		DEBUG(fprintf(stderr, "%s(): %s\n", __FUNCTION__, buf));
	} 
	else 
	{
		fprintf(stderr, "Read bad read! socket %d\n", fd);
		fprintf(stderr, "(%s)\n", strerror(errno));
		return(NULL);
	}	
	
	return(buf);
}

char *strip_version(int fd, void *arg, void *user_data)
{
	char *c, *d;
	long count = (long)arg;
	DEBUG(printf("%s()\n", __FUNCTION__));
	
	c = get_text(fd, arg, user_data);

	while(count--)
	{
		if (c) 
		{
			if (!strcasecmp(c, "No Answer") ||
			    !strcasecmp(c, "Inoperable"))
			    	return c;
			d = strchr(c, ' ');
			if (d) {
				c = d + 1;
			}
		}
	}

	return c;
}


char *strip_newline(int fd, void *arg, void *user_data)
{
	char *c, *d;
	long count = (long)arg;
	DEBUG(printf("%s()\n", __FUNCTION__));
	
	c = get_text(fd, arg, user_data);

	while(count--)
	{
		if (c) 
		{
			if (!strcasecmp(c, "No Answer") ||
			    !strcasecmp(c, "Inoperable"))
			    	return c;
			d = strchr(c, '\n');
			if (d) {
				*d = ' ';
			}
		}
	}

	return c;
}

int make_nonblock_stream_socket(int addr, unsigned short port)
{
	int flags;
	struct sockaddr_in sin;
	int sock;
	int res;
	int on = 1;
	
	DEBUG(printf("%s()\n", __FUNCTION__));
		
	sin.sin_addr.s_addr = addr;
	sin.sin_port = htons(port);
	sin.sin_family = AF_INET;
	
	if( (sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("problems opening the socket %s\n", strerror(errno));
		return(-1);
	}
	if ((flags = fcntl(sock, F_GETFL)) < 0)
	{
		printf("fcntl(F_GETFL) failed(%s)\n", strerror(errno));
		return(-1);
	}
	if (fcntl(sock, F_SETFL, (flags | O_NONBLOCK)) < 0)
	{
		printf("fcntl(F_SETFL) failed(%s)\n", strerror(errno));
		return(-1);
	}
	
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));
	
	if( (res = connect(sock, (struct sockaddr *)&sin, sizeof(sin))) < 0)
	{
		if(errno == EINPROGRESS)
			DEBUG(printf("have to wait for the socket to connect\n"));
		else
		{
			printf("connect error %s\n", strerror(errno));
			return(-1);
		}
	}
	
	DEBUG(printf("Created socket for port %d fd %d\n", port, sock));
	return(sock);
}

static int recieve_cb(int *id, int fd, short events, void *data)
{
	struct probe_data *pd = data;
	char *string;

	DEBUG(printf("%s() port %d\n", __FUNCTION__, pd->p->port));
	
	if(pd && pd->p && pd->p->recieve && pd->cb)
	{
		if(pd->flags & PROBE_DATA_FLAGS_SEND)
		{
			string = pd->p->recieve(fd, pd->p->recieve_arg, pd->user_data);
			
			pd->cb(pd->ip, pd->p->port, string, pd->user_data);
		}
		else
		{
			DEBUG(printf("%s(): no send before recieve\n", __FUNCTION__));
			return(1); // keep it going
		}
	}
	
	if(pd->tid >= 0)
		cheops_sched_del(pd->tid); 
	if(pd->soid != NULL)
		cheops_io_remove(pd->soid);
	
	if(pd->socket >= 0)
		close(pd->socket);	
	pd->socket = -1;

	pd->siid = NULL;
	
	free(pd);
	
	return(0);
}

static int send_cb(int *id, int fd, short events, void *data)
{
	struct probe_data *pd = data;
	DEBUG(printf("%s() port %d\n", __FUNCTION__, pd->p->port));
	
	if(pd && pd->p && pd->p->send)
		pd->p->send(fd, pd->p->send_arg, pd->user_data);
	
	pd->flags |= PROBE_DATA_FLAGS_SEND;
	pd->soid = NULL;
	return(0);
}

int timeout_cb(void *data)
{
	// the connection must have timeouted
	// or the server did not report anything back
	struct probe_data *pd = data;
	DEBUG(printf("%s() port %d\n soid=%p siid=%p\n", __FUNCTION__, pd->p->port, pd->soid, pd->siid) );
	
	if(pd->soid)
		cheops_io_remove(pd->soid);
	if(pd->siid)
		cheops_io_remove(pd->siid);

	if(pd->p && pd->timeout_cb)
		pd->timeout_cb(pd->socket, pd->ip, pd->p->port, pd->user_data);

	if(pd->socket >= 0)
		close(pd->socket);	
	pd->socket = -1;
	
	pd->tid = -1;
	
	free(pd);
	
	return(0);
}

void get_version(unsigned int     addr, 
                 unsigned short   port, 
                 unsigned int     timeout_ms, 
                 probe_callback   cb,
                 probe_timeout_callback timeout, 
                 void            *user_data)
{
	struct probe *p = get_probe(port);
	struct probe_data *pd = malloc(sizeof(*pd));
	
	DEBUG(printf("%s()\n", __FUNCTION__));
	if(p)
	{
		if(pd)
		{
			pd->socket = make_nonblock_stream_socket(addr, port);
			if( pd->socket < 0)
			{
				free(pd);
				return;
			}
			
			pd->p = p;
			pd->cb = cb;
			pd->timeout_cb = timeout;
			pd->user_data = user_data;
			pd->flags = 0;
			pd->ip = addr;
			pd->tid = cheops_sched_add(timeout_ms, timeout_cb, pd);
			pd->soid = cheops_io_add(pd->socket, CHEOPS_IO_CB(send_cb), POLLOUT, pd);
			pd->siid = cheops_io_add(pd->socket, CHEOPS_IO_CB(recieve_cb), POLLIN, pd);
		}
	}
	else
		if(pd)
			free(pd);
}

