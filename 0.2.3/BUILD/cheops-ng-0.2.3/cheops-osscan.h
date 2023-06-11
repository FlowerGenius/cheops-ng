/*
 * Cheops Next Generation GUI
 * 
 * cheops-osscan.h
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

#ifndef _CHEOPS_OSSCAN_H
#define _CHEOPS_OSSCAN_H

#include "cheops-types.h"

typedef struct _os_scan_option {
	u32	type;
	u32	length;
} os_scan_option;

typedef struct _os_scan_option_os {
#define OS_SCAN_OPTION_OS 1
	u32	type;
	u32	length;
	// the '\0' terminated string is after this
} os_scan_option_os;

typedef struct _os_scan_option_port {
#define OS_SCAN_OPTION_PORT 2
	u32	type;
	u32	length;
	u32	protocol;
	u32	state;
	u32	port_number;
	u32 proto;
	u32 rpcnum;
	u32 rpclowver;
	u32 rpchighver;
	// the '\0' terminated string of the service name is after this
	// the '\0' terminated string of the owner is after this
} os_scan_option_port;

typedef struct _os_scan_option_uptime {
#define OS_SCAN_OPTION_UPTIME 3
	u32	type;
	u32	length;
	u32	seconds;
	// the '\0' terminated string of the last boot is after this
} os_scan_option_uptime;


enum {
	HOST_STATE_UP = 1,
	HOST_STATE_DOWN,
	HOST_STATE_UNKNOWN,
	HOST_STATE_SKIPPED,
};

enum {
	PORT_STATE_OPEN = 1,
	PORT_STATE_CLOSED,
	PORT_STATE_FILTERED,
	PORT_STATE_UNFILTERED,
	PORT_STATE_UNKNOWN
};

enum {
	PORT_PROTOCOL_TCP = 1,
	PORT_PROTOCOL_UDP,
	PORT_PROTOCOL_IP
};

enum {
	PORT_PROTO_NONE = 0,
	PORT_PROTO_RPC = 1
};

void add_service(u16 port, u8 protocol, char *name);

void remove_service(u16 port, u8 protocol);

void remove_all_services(void);

char *get_service(u16 port, u8 protocol);


#define END_OF_OPTION(a) ((char *)(((char *)a) + sizeof(*a)))

#endif /* _CHEOPS_AGENT_OSSCAN_H */


