/*
 * Cheops Next Generation GUI
 * 
 * cheops-agent.c
 * Cheops Agent Main File
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
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "cheops.h"
#include "logger.h"
#include "event.h"
#include "agent-discover.h"
#include "agent-settings.h"
#include "agent-osscan.h"
#include "agent-map.h"
#include "agent-auth.h"
#include "agent-probe.h"
#include "probe.h"

#ifdef DEBUG_CHEOPS_AGENT
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

void init_osscan(void);

typedef struct handler_t {
	int id;
	cheops_event_handler cb;
	char *desc;
} handler;


/* Buffer for creating all events */
char ebuf[MAX_EVENT_SIZE];
event_hdr *eh = (event_hdr *)ebuf;
event *ee = (event *)(ebuf + sizeof(event_hdr));
int dopasswords = FALSE;

#if defined(HAS_SSL) && defined(USING_SSL)
	int dossl = FALSE;
#endif

static int handle_error(event_hdr *h, event *e, agent *a)
{
	DEBUG(clog(LOG_ERROR,"\nClient reports: Error[%d] - %s", ntohs(e->error_r.error), ((char *)e) + sizeof(error_r)));
	return(0);
}


static handler agent_handlers[] = {
	{ REPLY_ERROR,         handle_error,            "Error Reply" },	
	{ EVENT_OS_SCAN,       handle_osscan_request,   "Os-Scan Request Handler" },
	{ EVENT_DISCOVER_IPV4, handle_discover_request, "Discover Request Handler" },
	{ EVENT_SET_SETTINGS,  handle_set_settings,     "Set Agent Settings Handler" },
//	{ EVENT_DNS_QUERY,     handle_dns_query,        "DNS Query Handler" },
	{ EVENT_MAP_ICMP,      handle_map_icmp_request, "MAP ICMP Handler" },
	{ EVENT_PROBE,         handle_probe_request,    "PORT PROBE Handler" },
	{ -1 },
};

void usage()
{
	fprintf(stderr, "Usage: cheops-agent [options]\n"
	                "  options:\n"
	                "          -n: Listen using network method (default)\n"
	                "          -l: Listen using local method\n"
	                "          -a: Listen using all methods\n"
#ifdef NOTDEF
	                "          -f: Fork to background\n"
#endif
#ifndef FREEBSD
	                "          -p: Authenticate clients with passwords\n"
#endif
#if defined(HAS_SSL) && defined(USING_SSL)
	                "          -s: Use SSL for transferring information (USE THIS WITH -p!!!)\n"
#else
	                "              WARNING: without openssl your passwords will be\n"
	                "                       sent in cleartext\n"
#endif
           );         
	exit(1);
}

void register_agent_handlers()
{
	handler *h = agent_handlers;
	while(h->id > -1) {
		/* Try to register each of our shell items, but don't replace an existing
		   handler */
		if (event_register_handler(h->id, h->cb, 0)) 
			clog(LOG_WARNING, "Unable to register handler for '%s'\n", h->desc);
		h++;
	}
}

void *do_it(void *type)
{
	if( event_create_agent((int)type) )
	{
		clog(LOG_ERROR, "unable to listen.\n");
		exit(1);
	}
	authenticate_clients(dopasswords);
#if defined(HAS_SSL) && defined(USING_SSL)
		use_ssl(dossl);
#endif
	register_agent_handlers();
	cheops_main();
	return NULL;
}


static void myexit(int sig)
{
	event_cleanup();
    exit(0);
}

void do_tasks(int type)
{
	do_it((void *)type);
}

int main(int argc, char *argv[])
{
	char c;
	int local=0;
	int ipv4=0;
	int all=0;
	int dofork=0;
	pid_t pid;
	int type;
	
	if(getuid())
	{
		printf("You must be root to run %s\n\n",argv[0]);
		myexit(0);
	}
	
	while((c = getopt(argc, 
	                  argv, 
	                  "a"
	                  "n"
	                  "l"
#ifdef NOTDEF
	                  "f"
#endif	                  
	                  "p"
#if defined(HAS_SSL) && defined(USING_SSL)
	                  "s"
#endif
	                  )) 
	      >= 0
	     ) 
	{
		switch(c) {
		case 'a':
			all++;
			break;
		case 'n':
			ipv4++;
			break;
		case 'l':
			local++;
			break;
#ifdef NOTDEF
		case 'f':
			dofork++;
			break;
#endif /* NOTDEF */			
#ifndef FREEBSD
		case 'p':
			dopasswords++;
			break;
#endif
#if defined(HAS_SSL) && defined(USING_SSL)			
		case 's':
			dossl++;
			break;
#endif /* HAS_SSL */			
		default:
			usage();
		}
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT,  myexit);
	signal(SIGTERM, myexit);

	/* Check for garbage */
	if (optind != argc)
		usage();
	/* Default is to listen on ipv4 */
	if (!all && !local & !ipv4) 
		ipv4++; //all++;

	type = (all || local) ? AGENT_TYPE_LOCAL : 0; 
	type |= (all || ipv4) ? AGENT_TYPE_IPV4 : 0 ;

	init_osscan();
	init_probes();

#if defined(HAS_SSL) && defined(USING_SSL) && !defined(FREEBSD)
	if(dopasswords && !dossl)
	{
		fprintf(stderr, "WARNING!!! using -p without -s will reveal all passwords\n"
		                "           to anyone sniffing the network!\n");
	}
#endif /* HAS_SSL */

	if(system("nmap &> /dev/null") == 127 << 8)
	{
		fprintf(stderr, "WARNING!!! It seems like you do not have nmap installed\n"
		                "           or it is not in your path, os detection will\n"
		                "           not work without nmap\n");
	}
		
	if (dofork) {

		switch((pid = fork())) {
		case 0:
			/* Child */
			/* Main thread */
			do_tasks(type);
			break;
		case -1:
			/* Error */
			clog(LOG_ERROR, "fork() failed: %s\n", strerror(errno));
			exit(1);
			break;
		default:
			/* Parent */
			DEBUG(clog(LOG_NOTICE, "cheops-agent launched, pid %d\n", pid));
		}
	}
	else
		do_tasks(type);

	myexit(0);
	return(1);
}


