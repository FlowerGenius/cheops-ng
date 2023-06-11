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

#ifndef _IP_UTILS_H
#define _IP_UTILS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int get_host(char *hostname, u32 *ip);
extern char *ip2str(u32 ip);
extern int str2ip(char *s);
extern int allones(u32 mask);
extern unsigned short inet_checksum (void *addr, int len);
extern struct in_addr getlocalip (unsigned int dest);
extern void init_route_tables(void);
extern void sendicmp (int soc, int ttl, struct in_addr to);

/*
 * Macros for telling what class the ip address is
 * the ip address MUST be in network byte order
 */
#define IP_CLASSA(x)     ( (((unsigned char *)&x)[0] & 0x80) == 0x00 )
#define IP_CLASSB(x)     ( (((unsigned char *)&x)[0] & 0xc0) == 0x80 )
#define IP_CLASSC(x)     ( (((unsigned char *)&x)[0] & 0xe0) == 0xc0 )

/*
 * Mecro's for seeing if the ip address is a directed network broadcast
 * address, the ip address given MUST be in network byte order
 */
#define IP_CLASSA_BROADCAST(the_ip)  (				\
	(							\
		(((unsigned char *)&the_ip)[3] == 0xff) &&	\
		(((unsigned char *)&the_ip)[2] == 0xff) &&	\
		(((unsigned char *)&the_ip)[1] == 0xff)		\
	) || (							\
		(((unsigned char *)&the_ip)[3] == 0) &&		\
		(((unsigned char *)&the_ip)[2] == 0) &&		\
		(((unsigned char *)&the_ip)[1] == 0)		\
	)							\
)

#define IP_CLASSB_BROADCAST(the_ip)  (				\
	(							\
		(((unsigned char *)&the_ip)[3] == 0xff) &&	\
		(((unsigned char *)&the_ip)[2] == 0xff)		\
	) || (							\
		(((unsigned char *)&the_ip)[3] == 0) &&		\
		(((unsigned char *)&the_ip)[2] == 0)		\
	)							\
)

#define IP_CLASSC_BROADCAST(the_ip)  (				\
	(							\
		(((unsigned char *)&the_ip)[3] == 0xff)		\
	) || (							\
		(((unsigned char *)&the_ip)[3] == 0)		\
	)							\
)


#endif /* _IP_UTILS_H */


