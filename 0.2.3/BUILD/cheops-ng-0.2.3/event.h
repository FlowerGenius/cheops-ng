/*
 * Cheops Next Generation GUI
 * 
 * event.h
 * General event management definitions
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

#ifndef _EVENT_H
#define _EVENT_H

#include "cheops-types.h"

typedef struct event_header {
	u32 len;		/* Length of event (including header) */
	u16 hlen;		/* Length of header and options (like proxy) */
	u16 type;		/* Type of event (see below) */
	u32 flags;		/* Event specific flags */
} event_hdr;

/* Max number of events */
#define MAX_EVENT 256

/* Max event size */
#define MAX_EVENT_SIZE 65536
#define BIT(n) (1<<n)
/*
 * Flags for use in all event headers
 */
#define FLAG_FORCE	BIT(0)		/* Do not use cached response */
#define FLAG_PROXY	BIT(1)		/* Use specified proxy for request or
		                           specify that reply is coming from
		                           given proxy */
#define FLAG_IGNORE	BIT(2)		/* Ignore message if unknown */

/* Event may be followed by additional information like a proxy
   host, and then one of the following bits of information. */

typedef struct event_proxy {
#define HDRTYPE_PROXY	        1
	u8	hdrtype;	/* Type of header option */
	u8	family; 	/* Family (e.g. AF_INET) */
	u16	len;		/* Length of address */
	/* Address data */
} proxy;

/* ============  Standard Event Requests ================= */

/* 
 * Discover a given network, given a start and end,
 * IP, both in network byte order.
 */ 
typedef struct discover_ipv4_event {
#define EVENT_DISCOVER_IPV4 	2
	u32	start;		/* Start address */
	u32	end;		/* End address */
	u32	is_name;
	u32	len;
	void *np;
} discover_ipv4_e;

/*
 * OS detection request
 */
typedef struct osscan_event {
#define EVENT_OS_SCAN           3

#define OS_SCAN_OPTION_UDP_SCAN           (1<<0)

// do not change these, like reorder them!!!!!
#define OS_SCAN_OPTION_TCP_CONNECT_SCAN   (1<<1)
#define OS_SCAN_OPTION_TCP_SYN_SCAN       (1<<2)
#define OS_SCAN_OPTION_STEALTH_FIN        (1<<3)
#define OS_SCAN_OPTION_STEALTH_XMAS       (1<<4)
#define OS_SCAN_OPTION_STEALTH_NULL       (1<<5)
#define OS_SCAN_OPTION_SCAN_MASK          (OS_SCAN_OPTION_TCP_CONNECT_SCAN|OS_SCAN_OPTION_TCP_SYN_SCAN|OS_SCAN_OPTION_STEALTH_FIN|OS_SCAN_OPTION_STEALTH_XMAS|OS_SCAN_OPTION_STEALTH_NULL)

#define OS_SCAN_OPTION_DONT_PING          (1<<6)
#define OS_SCAN_OPTION_USE_PORT_RANGE     (1<<7)
#define OS_SCAN_OPTION_FASTSCAN           (1<<8)
#define OS_SCAN_OPTION_OSSCAN             (1<<9)
#define OS_SCAN_OPTION_RPC_SCAN           (1<<10)
#define OS_SCAN_OPTION_IDENTD_SCAN        (1<<11)

#define OS_SCAN_OPTION_TIMIMG_PARANOID    (1<<12)
#define OS_SCAN_OPTION_TIMIMG_SNEAKY      (1<<13)
#define OS_SCAN_OPTION_TIMIMG_POLITE      (1<<14)
#define OS_SCAN_OPTION_TIMIMG_NORMAL      (1<<15)
#define OS_SCAN_OPTION_TIMIMG_AGGRESSIVE  (1<<16)
#define OS_SCAN_OPTION_TIMIMG_INSANE      (1<<17)
#define OS_SCAN_OPTION_TIMIMG_MASK      (OS_SCAN_OPTION_TIMIMG_PARANOID|OS_SCAN_OPTION_TIMIMG_SNEAKY|OS_SCAN_OPTION_TIMIMG_POLITE|OS_SCAN_OPTION_TIMIMG_NORMAL|OS_SCAN_OPTION_TIMIMG_AGGRESSIVE|OS_SCAN_OPTION_TIMIMG_INSANE)

// end of the do not changes :)

                                           
                                           
                                           
                                           

// only fastscan or ports
	u32     options;
	u32     ip;
	void	*np;
	// the start of the NULL termicates string of ports starts here
} os_scan_e;

/*
 * Set the settings that the agent depends on
 */
typedef struct set_settings {
#define EVENT_SET_SETTINGS      4
	u32	flags;
	u32	discover_retries;	/* number discover of retries */
} set_settings_e; 

/* flags for the set_settings event */
#define SET_DISCOVER_RETRIES	0x0001
#define MAX_ALLOWABLE_DISCOVER_RETRIES 5

/*
 * DNS Query event
 */
typedef struct dns_query_event {
#define EVENT_DNS_QUERY         5
	u32     length;
	u32     num_options;
	char    name[256];
	//The first option starts after this
} dns_query_e;

/*
 * ICMP MAP event
 */
typedef struct map_icmp {
#define EVENT_MAP_ICMP          6
	u32     ip;
	void   *np;
} map_icmp_e;

/*
 * PORT probe event
 */
typedef struct _probe_e {
#define EVENT_PROBE             7
	u32     ip;
	u16     port;
	u32     timeout_ms;
	void   *np;
} probe_e;

/*
 * Authenticate request event
 */
typedef struct _auth_request_e {
#define EVENT_AUTH_REQUEST      8
	char username[35];
	char password[35];
} auth_request_e;



/* ============  Standard Event Replies ================= */

/*
 * Report an error string and code.
 * the code # is standard, and the 
 * string is some meaningful message.
 *
 * See cerror.h for more.
 */
typedef struct error_reply {
#define REPLY_ERROR             101
	u16 error;
	u16 len;
} error_r;

/*
 * Notification that a given IP address has
 * been discovered
 */
typedef struct discover_ipv4_reply {
#define REPLY_DISCOVER_IPV4 	102
	u32 ipaddr;
	void *np;
} discover_ipv4_r;

/*
 * OS detection reply
 */
typedef struct osscan_reply {
#define REPLY_OS_SCAN           103
	u32     num_options;
	u32     ip;
	void	*np;
	//The first option starts after this
}os_scan_r;

/*
 * DNS Query reply
 */
typedef struct dns_query_reply {
#define REPLY_DNS_QUERY         105
	u32     length;
	u32     num_options;
	u32     ip;
	char    name[256];
	//The first option starts after this
} dns_query_r;

/*
 * ICMP MAP reply
 */
typedef struct map_icmp_reply {
#define REPLY_MAP_ICMP         106
	u32     dest;
	u32     ip1;
	u32     ip2;
	void   *np;
} map_icmp_r;

/*
 * PORT probe reply
 */
typedef struct probe_r {
#define REPLY_PROBE            107
	u32     ip;
	u32     port;
	void   *np;
	// the version string is a null terminated string after the np pointer
} probe_r;

/*
 * Authenticate request reply
 */
typedef struct _auth_request_r {
#define REPLY_AUTH_REQUEST      108
	int authenticated;
} auth_request_r;



typedef union {
	os_scan_e os_scan_e;
	os_scan_r os_scan_r;
	discover_ipv4_e discover_ipv4_e;
	discover_ipv4_r discover_ipv4_r;
	set_settings_e set_settings_e;
	dns_query_e dns_query_e;
	dns_query_r dns_query_r;
	map_icmp_e map_icmp_e;
	map_icmp_r map_icmp_r;
	probe_r probe_r;
	probe_e probe_e;
	error_r error_r;
	auth_request_e auth_request_e;
	auth_request_r auth_request_r;
		
} event;


/* An agent is someone who communicates to us over a fd of some
   sort.   */

#define AGENT_TYPE_LOCAL 1
#define AGENT_TYPE_IPV4 2

struct agent_struct;

typedef struct agent_struct agent;

#define AGENT_RECV_BUFFER_LENGTH 65536

struct ssl_st;

struct agent_struct {
	agent *next;
	int    type;	/* Kind of agent */
	int   *id;		/* I/O ID */
	int    s;		/* Socket */
	char   buffer[AGENT_RECV_BUFFER_LENGTH];
	char  *write_ptr;  /* pointer to write into the buffer */
	char  *read_ptr;   /* pointer to read from (start of forming event) */
	int    expected_length; /* length we expect */
	void  *more; /* Anything else */
	unsigned int flags;
	struct ssl_st  *ssl;
};

#define AGENT_FLAGS_LOGGED_IN            0x00000001
#define AGENT_FLAGS_AUTHENTICATE_CLIENTS 0x00000002
#define AGENT_FLAGS_USING_SSL            0x00000004

/*
 * A Cheops event handler takes it, a header, an event, and the source 
 * agent, and handles it.
 */

typedef int (*cheops_event_handler)(event_hdr *h, event *e, agent *a);
#define CHEOPS_EVENT_HANDLER(a) ((cheops_event_handler)(a))

/*
 * Request an agent to a specific remote host.
 */
extern agent *event_request_agent(int agent_type, void *addr, int usessl);



/*
 * Build a local listening agent.  Note that agent_type can be more
 * than one agent type, OR'd together.
 */
extern int event_create_agent(int agent_type);

/*
 * Send an event to an agent
 */
extern int event_send(agent *a, event_hdr *h);

/*
 * Destroy agent
 */
extern void event_destroy_agent(agent *a);

/*
 * Register a handler for an event type
 */

extern int event_register_handler(short type, cheops_event_handler h, int replace);

void agent_authenticate_clients(agent *a, int enable);
int agent_authenticating_clients(agent *a);
void authenticate_clients(int enable);
int authenticating_clients(void);
void agent_use_ssl(agent *a, int enable);
void use_ssl(int enable);
int using_ssl(void);
int agent_using_ssl(agent *a);
void event_cleanup(void);
#endif

