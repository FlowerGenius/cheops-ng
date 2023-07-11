/*
 * Cheops Next Generation GUI
 * 
 * cheops-sh-guts.c
 * Guts of command parsing for the cheops shell.
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
#include <unistd.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <shadow.h>   
#include <pwd.h>
#include <sys/types.h>
#include "cheops.h"
#include "io.h"
#include "logger.h"
#include "event.h"
#include "cheops-sh.h"
#include "ip_utils.h"
#include "misc.h"
#include "agent-osscan.h"

#ifndef DEBUG_CHEOPS_SH
	#define DEBUG(a)
#else
	#define DEBUG(a)
#endif

extern char *crypt(const char *key, const char *salt);

static void usage(char *s);

/* Buffer for creating all events */
static char ebuf[65536];
static event_hdr *eh = (event_hdr *)ebuf;
static event *ee = (event *)(ebuf + sizeof(event_hdr));
static int flags = 0;

static struct command {
	char *word;
	int (*command)(agent *a, int argc, char *argv[]);
	char *help;
	char *usage;
} commands[];

static struct handler {
	int id;
	cheops_event_handler cb;
	char *desc;
} shell_handlers[];

struct ip_list {
	u32	ip;
	struct ip_list *next;
};

static struct ip_list *IPList = NULL;

int delete_ip_list();


static int do_quit(agent *a, int argc, char *argv[])
{
	printf("Bye!\n");
	if (getenv("HOME")) {	
		char filename[80];
		snprintf(filename, sizeof(filename), "%s/.cheops/history", getenv("HOME"));
		write_history(filename);
	}
	exit(0);
}

static int do_discover(agent *a, int argc, char *argv[])
{
	int x;
	int skip = argc;
	char *c;
	u32 start;
	u32 mask;
	u32 end;
	int res;
	int done = -1;
	
	if (argc < 2) 
	{
		usage(argv[0]);
		return(0);
	}
	
	for(x=1; x<argc; x++)
	{
		if(!strcmp("force",argv[x]))
		{
			flags |= FLAG_FORCE;
			skip = x;
		}
	}
	
	for (x=1; x<argc; x++) {
		if(x == skip)
			continue;
		if ((c = strchr(argv[x], '/'))) {
			/* Network/netmask */
			*c='\0';
			c++;
			if (get_host(argv[x], &start)) 
			{
				printf("Invalid host: %s\n", argv[x]);
				return(0);
			}
			else {
				/* Check the netmask, first to see if it's of the form 255.255.255.0 */
				res = inet_aton(c, (struct in_addr *)&mask);
				mask = ntohl(mask);
				if (!res || (mask < 0x80000000) || !allones(mask)) {
					if ((atoi(c) > 32) || (atoi(c) < 1)) {
						/* We don't allow a zero netmask, that would
						   be the entire internet! */
						printf("Invalid netmask: %s\n", c);
						
						continue;
					}
					mask = 0xffffffff;
					mask = mask >> (32-atoi(c));
					mask = mask << (32-atoi(c));
				}
				start = ntohl(start);
				start &= mask;
				end = start | ~mask;
				printf("Discovering from %s to ", ip2str(htonl(start)));
				printf("%s (mask = ", ip2str(htonl(end)));
				printf("%s)...\n", ip2str(htonl(mask)));
			}
		} else {
			/* Host */
			if (get_host(argv[x], &start)) {
				printf("Invalid host: %s\n", argv[x]);
				continue;
			}
			printf("Discovering %s (%s)\n", argv[x], ip2str(start));
			start = ntohl(start);
			end = start;
		}
		eh->len = htonl(sizeof(event_hdr) + sizeof(discover_ipv4_e));
		eh->hlen = htons(sizeof(event_hdr));
		eh->type = htons(EVENT_DISCOVER_IPV4);
		eh->flags = flags;
		ee->discover_ipv4_e.start = start;
		ee->discover_ipv4_e.end = end;
		if (event_send(a, eh) < 0) 
			c_log(LOG_WARNING, "Unable to send discover event\n");
		else
			done = 0;
	}
	return done;	
}

static int do_help(agent *a, int argc, char *argv[])
{
	struct command *c;
	int x;
	if (argc < 2) {
		printf("             -- Cheops command listing --\n\n");
		c = commands;
		while(c->word) {
			printf("  %12s -- %s\n", c->word, c->help);
			c++;
		}
		printf("\nUse help <command> for help with a specific command\n\n");
	} else
		for(x=1; x<argc; x++) {
			c = commands; 
			while(c->word) {
				if (!strcasecmp(c->word, argv[x])) {
					printf("Command:      %s\n", c->word);
					printf("Description:  %s\n", c->help);
					printf("Usage:        %s\n", c->usage);
					printf("\n");
					break;
				}
				c++;
			}
			if (!c->word) {
				printf("I don't know anything about '%s'\n", argv[x]);
			}
		}
	return 0;
		
}

static int do_os_scan_command(agent *a, int argc, char *argv[])
{
	if (argc < 2) 
	{
		usage(argv[0]);
		return(0);
	}
	
	eh->len = htonl(sizeof(event_hdr) + sizeof(discover_ipv4_e) + strlen(argv[1]) + 1);
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(EVENT_OS_SCAN);
	eh->flags = flags;
	if (event_send(a, eh) < 0) 
		c_log(LOG_WARNING, "Unable to send OS-scan event\n");
	return(0);
}


static int do_summary(agent *a, int argc, char *argv[])
{
	struct ip_list *pipl; /* pipl is in NETWORK order */
	
	if(IPList == NULL)
	{
		printf("There is nothing in the cache.\n");
	}
	else
	{
		for(pipl = IPList; pipl; pipl = pipl->next)
		{
			printf("Discovered: %s\n",ip2str(pipl->ip));
		} 
	}
	return(0);
}

int do_force(agent *a, int argc, char *argv[])
{
	int	i;
	int	len=0;
	int	cnt;
	char	*args;
	char	*ptr;
	char	*toks[MAX_TOKENS];

/* eliminate the force arg */	
	for(i=0;i<argc;i++)
	{
		len+=strlen(argv[i]) + 1;
	}
	args=malloc(sizeof(char)*len);

	ptr=args;
	for(i=1; i<argc; i++)
	{
		sprintf(ptr,"%s ",argv[i]);
		ptr+=strlen(argv[i]) + 1;
	}
	
 	cnt = parse(toks, MAX_TOKENS, args);

	flags |= FLAG_FORCE;// set the force flag
	execute_command(a,cnt,toks);
	flags &= ~FLAG_FORCE; // unset the force flag

	free(args);	
	return(0);
}

int do_set_discover_retries(agent *a, int argc, char *argv[])
{
	int retries;
	
	if(argc < 2)
	{
		usage(argv[0]);
		return(0);
	}
	
	retries = atoi(argv[1]);
	if(retries > MAX_ALLOWABLE_DISCOVER_RETRIES || retries < 0)
	{
		usage(argv[0]);
		return(0);
	}
	
	eh->len = htonl(sizeof(event_hdr) + sizeof(set_settings_e));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(EVENT_SET_SETTINGS);
	eh->flags = flags;
	ee->set_settings_e.discover_retries = retries;
	ee->set_settings_e.flags = SET_DISCOVER_RETRIES;
	if (event_send(a, eh) < 0) 
		c_log(LOG_WARNING, "Unable to send set discover retries event\n");

	return(0);
}

int do_auth_request_command(agent *a, int argc, char *argv[])
{
	eh->len = htonl(sizeof(event_hdr) + sizeof(auth_request_e));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(EVENT_AUTH_REQUEST);
	eh->flags = 0;

	if (event_send(a, eh) < 0) 
		c_log(LOG_WARNING, "Unable to send login event\n");

	return(0);
}



/******************************       COMMANDS        ************************ */

struct command commands[] =  {
	{ "help", do_help, "List of commands, and usage information", "help [ command... ]" },
	{ "force", do_force, "Prepend this to a command and it will set the force flag for that command", "force [command]" },
	{ "discover", do_discover, "Find hosts on a network", "discover [ network/netmask ] | [ host ] [force]" }, 
	{ "setdiscoverretries", do_set_discover_retries, "Set the number of discover retries [0-5]", "discover [ number of retries ]" }, 
	{ "quit", do_quit, "Exit from the Cheops shell", "quit" },
	{ "summary", do_summary, "Get a Summary of the IP addresses discovered", "summary [ network/netmask ] " },
	{ "delcache", delete_ip_list, "Delete the Discover cache", "delcache" },
	{ "os", do_os_scan_command, "Scan the host for it's OS", "os [host]" },
	{ "login", do_auth_request_command, "login into the agent", "login" },

	{ NULL },
};

static int handle_error(event_hdr *h, event *e, agent *a)
{
	printf("\nAgent reports: Error[%d] - %s", ntohs(e->error_r.error), ((char *)e) + sizeof(error_r));
	rl_forced_update_display();
	return 0;
}

int add_to_discovered_list(u32 ip)
{
	struct ip_list *pipl;
	
	for(pipl = IPList; pipl; pipl = pipl->next)
	{
		if(pipl->ip == ip)
		{
			DEBUG( c_log(LOG_NOTICE,"add_to_discovered_list(): %s is already in the list",ip2str(htonl(ip))) );
			return(0);
		}
	}
	pipl = NULL;
	
	pipl = malloc(sizeof(struct ip_list));
		
	pipl->ip = ip;
	pipl->next = IPList;
	IPList = pipl;
	
	return(0);
}

int delete_ip_list()
{
	struct ip_list *pipl, *next;
	
	pipl=IPList;
	
	while(1)
	{
		if(pipl==NULL)
			break;
		next = pipl->next;
		free(pipl);
		pipl = next;
	}
	IPList = NULL;
	return(0);
}

int handle_discover_reply(event_hdr *h, event *e, agent *a)
{
	printf("\n==Discovered: %s==\n",ip2str(e->discover_ipv4_r.ipaddr));
	add_to_discovered_list(e->discover_ipv4_r.ipaddr);
	rl_forced_update_display();
	return(0);
}

int handle_os_scan_reply(event_hdr *h, event *e, agent *a)
{
	char *ptr = (void *)e + sizeof(os_scan_r);
	os_scan_option *opt;
	os_scan_option_os *os;
	os_scan_option_port *port;
	int i,j = ntohl(e->os_scan_r.num_options);

	printf("sizeof %d  len=%d\n",sizeof(os_scan_option_port), ntohl(h->len));

	for(i = 0; i < j; i++)
	{
		opt = (void *)ptr;
		switch(ntohl(opt->type))
		{
			case OS_SCAN_OPTION_OS:
				os = (void *)opt;
				printf("\n==Scaned: %s is %s==\n",ip2str(e->os_scan_r.ip), (char *)&os->string );
				ptr += ntohl(opt->length);
				break;
			
			case OS_SCAN_OPTION_PORT:
				port = (void *)opt;
				printf("==port %d==\n",ntohl(port->port_number));
				ptr += ntohl(opt->length);
				break;
					
			default:
				printf("\nWhat the h@#$ is this? handle_os_scan_reply\n");
				ptr += ntohl(opt->length);
				break;
		}
	}
	rl_forced_update_display();
	return(0);
}

int send_auth_request(agent *a, char *username, char *password)
{
	printf("authenticating %s %s\n", username, password);

	eh->len = htonl(sizeof(event_hdr) + sizeof(auth_request_e));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(EVENT_AUTH_REQUEST);
	eh->flags = 0;
	strcpy(ee->auth_request_e.username, username);
	strcpy(ee->auth_request_e.password, password);
	
	if (event_send(a, eh) < 0) 
		c_log(LOG_WARNING, "Unable to send login event\n");
	
	return(0);
}

int authenticate_me(agent *a)
{
	char username[80];
	char password[80];
	
	printf("enter your username: ");
	scanf("%s", username);
	printf("\nenter your password: ");
	scanf("%s", password);
	printf("\nchecking...\n");
	send_auth_request(a, username, password);
	
	return(0);
}

int handle_auth_request_reply(event_hdr *h, event *e, agent *a)
{
	int authenticated = ntohl(e->auth_request_r.authenticated);

	if(authenticated)
		printf("Authenticated\n");
	else
	{
		printf("Authentication Failed!\n");
		authenticate_me(a);
	}
	
	return(0);
}


/* *****************************       HANDLERS        ************************* */

struct handler shell_handlers[] = {
	{ REPLY_ERROR,          handle_error,               "Error Reply" },
	{ REPLY_DISCOVER_IPV4,  handle_discover_reply,      "Discover Reply Handler" },
	{ REPLY_OS_SCAN,        handle_os_scan_reply,       "OS-scan Reply Handler" },
	{ REPLY_AUTH_REQUEST,   handle_auth_request_reply,  "Authenticate Reply Handler" },
	
	{ -1 },
};

static void usage(char *cmd)
{
	struct command *c = commands;	
	while(c->word) {
		if (!strcasecmp(cmd, c->word)) {
			printf("Usage: %s\n", c->usage);
			return;
		}
		c++;
	}
	printf("I don't know how to use %s\n", cmd);
}


int execute_command(agent *a, int argc, char *argv[]) {
	struct command *c = commands;
	while(c->word) {
		if (!strcasecmp(argv[0], c->word)) 
			return c->command(a, argc, argv);
		c++;
	}
	printf("Unknown command: %s\n", argv[0]);
	return -1;
}

char *command_generator(char *text, int state)
{
	static int index;
	struct command *c;
	if (!state) 
		index = 0;
	c = commands + index;
	while(c->word && strncasecmp(c->word, text, strlen(text))) { c++; index++; };
	index++;
	return c->word ? strdup(c->word) : NULL;
};

void register_shell_handlers()
{
	struct handler *h = shell_handlers;
	while(h->id > -1) {
		/* Try to register each of our shell items, but don't replace an existing
		   handler */
		if (event_register_handler(h->id, h->cb, 0)) 
			c_log(LOG_WARNING, "Unable to register handler for '%s'\n", h->desc);
		h++;
	}
}

