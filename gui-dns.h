/*
 * Cheops Next Generation GUI
 * 
 * gui-dns.h
 * shim between gNet and cheops
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

#ifndef GUI_DNS_H
#define GUI_DNS_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <adns.h>

typedef void (*dns_timeout_cb)(char *ip, char *hostname, void *data);
typedef void (*dns_answer_cb)(char *ip, char *hostname, void *data);

typedef struct _dns_query {
	struct _dns_query 	*next;
	char 			*hostname;
	char 			*ip;
	struct sockaddr_in	saddr;
	dns_timeout_cb 	timeout_cb;
	dns_answer_cb 		answer_cb;
	unsigned char 		flags;
	void 			*data;
	adns_query 		qu;
} dns_query;

#define DNS_QUERY_FLAGS_NAME		1
#define DNS_QUERY_FLAGS_REVERSE	2

/*
 * Should be in network format
 */
void dns_name_lookup( char *hostname, dns_answer_cb answer_cb, dns_timeout_cb timeout_cb, void *data);
void dns_reverse_lookup( char *ip, dns_answer_cb answer_cb, dns_timeout_cb timeout_cb, void *data);


#endif /* GUI_DNS_H */

