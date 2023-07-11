/*
 * Cheops Next Generation GUI
 * 
 * agent-nmapxml.c
 * code to parse nmap XML output
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
#include "agent-nmapxml.h"

#define DEBUG_NMAPXML

#ifdef DEBUG_NMAPXML
	#define DEBUG(x) x
#else
	#define DEBUG(a)
#endif

void g_free_not_null(void *it)
{
	if(it)
		g_free(it);
}

void nmapPortFreeEntry(nmap_host_port_t *p, void *arg)
{
	g_free_not_null(p);
}

void nmap_scan_free(nmap_scan_t *n)
{
	if(n)
	{
		g_free_not_null(n->scanner);
		g_free_not_null(n->args);
		g_free_not_null(n->version);
		g_free_not_null(n->xmloutputversion);

		g_free_not_null(n->host.addr);
		g_list_foreach(n->host.ports, (GFunc)nmapPortFreeEntry, NULL);
		g_list_free(n->host.ports);
		g_free_not_null(n->host.uptime_seconds);
		g_free_not_null(n->host.uptime_last_boot);

		g_free_not_null(n->os.name);
		g_free_not_null(n->os.accuracy);
	}
}

static void parseNmapScanPortService(nmap_host_port_t *port, xmlNode *node)
{
	xmlChar *value;
	DEBUG(printf("%s()\n", __FUNCTION__));

	if( (value = xmlGetProp(node, "name")) )
	{
		g_free_not_null(port->service);
		port->service = g_strdup(value);
	}
	if( (value = xmlGetProp(node, "proto")) )
	{
		if(0 == xmlStrcmp(value, "rpc"))
			port->proto = PORT_PROTO_RPC;
		else
			port->proto = PORT_PROTO_NONE;
	}
	else
		port->proto = PORT_PROTO_NONE;
	
	if( (value = xmlGetProp(node, "rpcnum")) )
	{
		port->rpcnum = atoi(value);
	}
	if( (value = xmlGetProp(node, "lowver")) )
	{
		port->rpclowver = atoi(value);
	}
	if( (value = xmlGetProp(node, "highver")) )
	{
		port->rpchighver = atoi(value);
	}
}

static void parseNmapScanPortState(nmap_host_port_t *port, xmlNode *node)
{
	xmlChar *value;
	DEBUG(printf("%s()\n", __FUNCTION__));

	if( (value = xmlGetProp(node, "state")) )
	{
		if(0 == xmlStrcmp(value, "open"))
			port->state = PORT_STATE_OPEN;
		else if(0 == xmlStrcmp(value, "closed"))
			port->state = PORT_STATE_CLOSED;
		else if(0 == xmlStrcmp(value, "filtered"))
			port->state = PORT_STATE_FILTERED;
		else if(0 == xmlStrcmp(value, "UNfilitered"))
			port->state = PORT_STATE_UNFILTERED;
		else if(0 == xmlStrcmp(value, "unknown"))
			port->state = PORT_STATE_UNKNOWN;
		else
			port->state = PORT_STATE_UNKNOWN;
	}
	else
		port->state = PORT_STATE_UNKNOWN;
	
}

static void parseNmapScanPortOwner(nmap_host_port_t *port, xmlNode *node)
{
	xmlChar *value;
	
	DEBUG(printf("%s()\n", __FUNCTION__));
	if( (value = xmlGetProp(node, "name")) )
	{
		g_free_not_null(port->owner);
		port->owner = g_strdup(value);
	}
}

static void parseNmapScanPort(nmap_scan_t *nms, xmlNode *node)
{
	xmlChar *value;
	nmap_host_port_t *port = malloc(sizeof(*port));
	
	DEBUG(printf("%s()\n", __FUNCTION__));
	if(port == NULL)
	{
		perror("out of memory");
		exit(-1);
	}
	memset(port, 0, sizeof(*port));
	
	if( (value = xmlGetProp(node, "protocol")) )
	{
		if(0 == xmlStrcmp(value, "tcp"))
			port->protocol = PORT_PROTOCOL_TCP;
		else if(0 == xmlStrcmp(value, "udp"))
			port->protocol = PORT_PROTOCOL_UDP;
		else if(0 == xmlStrcmp(value, "ip"))
			port->protocol = PORT_PROTOCOL_IP;
	}
	if( (value = xmlGetProp(node, "portid")) )
	{
		port->portid = atoi(value);
	}
	
	/*
	 * Now, walk the tree.
	 */
	node = node->xmlChildrenNode;
	while (node) 
	{
		if(!xmlIsBlankNode(node))
		{
			if(0 == xmlStrcmp(node->name, (xmlChar *)"service"))
				parseNmapScanPortService(port, node);
			else if(0 == xmlStrcmp(node->name, (xmlChar *)"state"))
				parseNmapScanPortState(port, node);
			else if(0 == xmlStrcmp(node->name, (xmlChar *)"owner"))
				parseNmapScanPortOwner(port, node);
		}
		node = node->next;
	}
	
	nms->host.ports = g_list_prepend(nms->host.ports, port);
}

static void parseNmapScanPorts(nmap_scan_t *nms, xmlNode *node)
{
	DEBUG(printf("%s()\n", __FUNCTION__));
	/*
	 * Now, walk the tree.
	 */
	node = node->xmlChildrenNode;
	while (node) 
	{
		if(!xmlIsBlankNode(node))
		{
			if(0 == xmlStrcmp(node->name, (xmlChar *)"port"))
				parseNmapScanPort(nms, node);
		}
		node = node->next;
	}
}

static void parseNmapScanOSosmatch(nmap_scan_t *nms, xmlNode *node)
{
	xmlChar *value;

	DEBUG(printf("%s()\n", __FUNCTION__));
	if( (value = xmlGetProp(node, "name")) )
	{
		g_free_not_null(nms->os.name);
		nms->os.name = g_strdup(value);
	}
}

static void parseNmapScanOS(nmap_scan_t *nms, xmlNode *node)
{
	DEBUG(printf("%s()\n", __FUNCTION__));
	/*
	 * Now, walk the tree.
	 */
	node = node->xmlChildrenNode;
	while (node) 
	{
		if(!xmlIsBlankNode(node))
		{
			if(0 == xmlStrcmp(node->name, (xmlChar *)"osmatch"))
				parseNmapScanOSosmatch(nms, node);
		}
		node = node->next;
	}
}

static void parseNmapScanAddress(nmap_scan_t *nms, xmlNode *node)
{
	xmlChar *type;
	xmlChar *value;

	DEBUG(printf("%s()\n", __FUNCTION__));
	if( (value = xmlGetProp(node, "addr")) )
	{
              if( (type = xmlGetProp(node, "addrtype")) && 0 == strcmp(type, "ipv4"))
              {
                      g_free_not_null(nms->host.addr);
                      nms->host.addr = g_strdup(value);
              }
	}
}

static void parseNmapScanUptime(nmap_scan_t *nms, xmlNode *node)
{
	xmlChar *value;

	DEBUG(printf("%s()\n", __FUNCTION__));
	if( (value = xmlGetProp(node, "seconds")) )
	{
		g_free_not_null(nms->host.uptime_seconds);
		nms->host.uptime_seconds = g_strdup(value);
	}
	if( (value = xmlGetProp(node, "lastboot")) )
	{
		g_free_not_null(nms->host.uptime_last_boot);
		nms->host.uptime_last_boot = g_strdup(value);
	}
}

static void parseNmapScanHost(nmap_scan_t *nms, xmlNode *node)
{
	DEBUG(printf("%s()\n", __FUNCTION__));
	/*
	 * Now, walk the tree.
	 */
	node = node->xmlChildrenNode;
	while (node) 
	{
		if(!xmlIsBlankNode(node))
		{
			if(0 == xmlStrcmp(node->name, (xmlChar *)"ports"))
				parseNmapScanPorts(nms, node);
			else if(0 == xmlStrcmp(node->name, (xmlChar *)"os"))
				parseNmapScanOS(nms, node);
			else if(0 == xmlStrcmp(node->name, (xmlChar *)"address"))
				parseNmapScanAddress(nms, node);
			else if(0 == xmlStrcmp(node->name, (xmlChar *)"uptime"))
				parseNmapScanUptime(nms, node);
		}
		node = node->next;
	}
}

static nmap_scan_t *parseNmapScan(xmlDoc *doc)
{
	nmap_scan_t *ret;
	xmlNode *cur;
	xmlChar *value;
	
	DEBUG(printf("%s()\n", __FUNCTION__));
	cur = xmlDocGetRootElement(doc);
	if (cur == NULL) {
		fprintf(stderr, "empty document\n");
		xmlFreeDoc(doc);
		return (NULL);
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "nmaprun")) {
		fprintf(stderr, "document of the wrong type, root node != nmaprun but '%s'", cur->name);
		xmlFreeDoc(doc);
		return (NULL);
	}

	/*
	 * Allocate the structure to be returned.
	 */
	ret = (nmap_scan_t *)malloc(sizeof(*ret));
	if (ret == NULL) {
		fprintf(stderr, "out of memory\n");
		xmlFreeDoc(doc);
		return (NULL);
	}
	memset(ret, 0, sizeof(*ret));

	if( (value = xmlGetProp(cur, "version")) )
	{
		g_free_not_null(ret->version);
		ret->version = g_strdup(value);
	}
	if( (value = xmlGetProp(cur, "args")) )
	{
		g_free_not_null(ret->args);
		ret->args = g_strdup(value);
	}
	if( (value = xmlGetProp(cur, "scanner")) )
	{
		g_free_not_null(ret->scanner);
		ret->scanner = g_strdup(value);
	}

	/*
	 * Now, walk the tree.
	 */
	/* First level we expect just Jobs */
	cur = cur->xmlChildrenNode;
	while (cur) 
	{
		if(!xmlIsBlankNode(cur))
		{
			if(0 == xmlStrcmp(cur->name, (xmlChar *)"host"))
				parseNmapScanHost(ret, cur);
		}
		cur = cur->next;
	}
	
	xmlFreeDoc(doc);

	return (ret);
}

void printPorts(nmap_host_port_t *port, void *arg)
{
	if(port)
	{
		printf("host port protocol = %s\n", (port->protocol == PORT_PROTOCOL_TCP ? "tcp" : 
		                                     (port->protocol == PORT_PROTOCOL_UDP ? "udp": 
		                                      (port->protocol == PORT_PROTOCOL_IP ? "ip": "unknown"))));
		printf("host port portid = %d\n", port->portid);
		printf("host port state = %s\n", (port->state == PORT_STATE_OPEN ? "open" :
		                                  (port->state == PORT_STATE_CLOSED ? "closed" :
		                                   (port->state == PORT_STATE_FILTERED ? "filtered" :
		                                    (port->state == PORT_STATE_UNFILTERED ? "UNfiltered" : "unknown")))));
		printf("host port service = %s\n", port->service);
		printf("host port owner = %s\n", port->owner);
		printf("host port proto = %d\n", port->proto);
		printf("host port rpcnum = %d\n", port->rpcnum);
		printf("host port rpclowver = %d\n", port->rpclowver);
		printf("host port rpchighver = %d\n", port->rpchighver);
	}
}

void nmap_scan_print(nmap_scan_t *cur)
{
	printf("scanner = %s\n", cur->scanner);
	printf("args = %s\n", cur->args);
	printf("version = %s\n", cur->version);

	printf("host addr = %s\n", cur->host.addr);
	g_list_foreach(cur->host.ports, (GFunc)printPorts, NULL);
	
	printf("host os name = %s\n", cur->os.name);
}

xmlDoc *runNmapScan(char *ip, unsigned long options, char *port_range)
{
	char buffer[80000];
	char cmd[500];
	FILE *pfp;
	int i;
	
	sprintf(cmd, "nmap %s -oX - -n", ip);

	switch(options & OS_SCAN_OPTION_SCAN_MASK)
	{
		case OS_SCAN_OPTION_TCP_CONNECT_SCAN:
			strcat(cmd, " -sT");
			break;
			
		case OS_SCAN_OPTION_TCP_SYN_SCAN:
			strcat(cmd, " -sS");
			break;
			
		case OS_SCAN_OPTION_STEALTH_FIN:
			strcat(cmd, " -sF");
			break;
			
		case OS_SCAN_OPTION_STEALTH_XMAS:
			strcat(cmd, " -sX");
			break;
			
		case OS_SCAN_OPTION_STEALTH_NULL:
			strcat(cmd, " -sN");
			break;
		
		default:
			break;
	}
	
	switch(options & OS_SCAN_OPTION_TIMIMG_MASK)
	{
		case OS_SCAN_OPTION_TIMIMG_PARANOID:
			strcat(cmd, " -T Paranoid");
			break;
		
		case OS_SCAN_OPTION_TIMIMG_SNEAKY:
			strcat(cmd, " -T Sneaky");
			break;
		
		case OS_SCAN_OPTION_TIMIMG_POLITE:
			strcat(cmd, " -T Polite");
			break;
		
		case OS_SCAN_OPTION_TIMIMG_NORMAL:
			strcat(cmd, " -T Normal");
			break;
		
		case OS_SCAN_OPTION_TIMIMG_AGGRESSIVE:
			strcat(cmd, " -T Aggressive");
			break;
		
		case OS_SCAN_OPTION_TIMIMG_INSANE:
			strcat(cmd, " -T Insane");
			break;
		
	}
	if(options & OS_SCAN_OPTION_OSSCAN)
		strcat(cmd, " -O");

	if(options & OS_SCAN_OPTION_UDP_SCAN)
		strcat(cmd, " -sU");

	if(options & OS_SCAN_OPTION_RPC_SCAN)
		strcat(cmd, " -sR");
	
	if(options & OS_SCAN_OPTION_IDENTD_SCAN)
		strcat(cmd, " -I");
	
	if(options & OS_SCAN_OPTION_FASTSCAN)
		strcat(cmd, " -F");
	
	if(options & OS_SCAN_OPTION_DONT_PING)
		strcat(cmd, " -P0");
	
	if(options & OS_SCAN_OPTION_USE_PORT_RANGE)
	{
		strcat(cmd, " -p ");
		strcat(cmd, port_range);
	}
	DEBUG(printf("%s(): scanning %s using '%s'\n", __FUNCTION__, ip, cmd));
	pfp = popen(cmd, "r");
	if(pfp == NULL)
	{
		perror("I could not run nmap");
		return(NULL);
	}
	
	while(!feof(pfp))
	{
		if('<' == (buffer[0] = fgetc(pfp)))
		{
			break;
		}
	}
	i = 1;
	while(!feof(pfp) && i < sizeof(buffer))
	{
		buffer[i] = fgetc(pfp);
		if(feof(pfp))
			buffer[i] = '\0';
		i++;
	}
	pclose(pfp);
	if(i == sizeof(buffer))
	{
		fprintf(stderr, "hmm... it seems like nmap had a rather large output...\n");
		fprintf(stderr, "we ran out of buffer space for nmap's output when scanning %s\n", ip);
		buffer[sizeof(buffer) - 1] = '\0';
		fprintf(stderr, "dump of buffer=\n%s", buffer);
		return(NULL);
	}
	
	return(xmlParseDoc((xmlChar *)buffer));
}

nmap_scan_t *nmap_scan(char *ip, unsigned long options, char *ports)
{
	nmap_scan_t *ret = NULL;
	xmlDoc *doc;
	
	/* COMPAT: Do not genrate nodes for formatting spaces */
	LIBXML_TEST_VERSION xmlKeepBlanksDefault(0);

	doc = runNmapScan(ip, options, ports);
	if(doc == NULL)
	{
		fprintf(stderr, "cheops-agent: I could not run nmap... is it in your path?\n");
		return(NULL);
	}
	
	ret = parseNmapScan(doc);

	xmlCleanupParser();

	return(ret);
}
