#ifndef AGENT_NMAP_XML_H
#define AGENT_NMAP_XML_H

#include "cheops-osscan.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <glib.h>
#include "event.h"

typedef struct _nmap_host_port_t {
	unsigned int protocol; // PORT_PROTOCOL_*
	unsigned int portid;
	unsigned int state; // PORT_STATE_*
	xmlChar *service;
	xmlChar *owner;
	unsigned int proto;
	unsigned int rpcnum;
	unsigned int rpclowver;
	unsigned int rpchighver;
} nmap_host_port_t; 
 
typedef struct _nmap_host_t {
	xmlChar *addr;
	GList *ports;
	xmlChar *uptime_seconds;
	xmlChar *uptime_last_boot;
} nmap_host_t;

typedef struct _nmap_os_t {
	xmlChar *name;
	xmlChar *accuracy;
} nmap_os_t;

typedef struct _nmap_scan_t {
	xmlChar *scanner;
	xmlChar *args;
	xmlChar *version;
	xmlChar *xmloutputversion;
	nmap_host_t host;
	nmap_os_t os;
} nmap_scan_t;


void g_free_not_null(void *it);

void nmap_scan_free(nmap_scan_t *n);

nmap_scan_t *nmap_scan(char *ip, unsigned long options, char *ports);
// the options are in event.h

void nmap_scan_print(nmap_scan_t *cur);

#endif /* AGENT_NMAP_XML_H */
