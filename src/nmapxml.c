/*
 * Cheops Next Generation GUI
 * 
 * nmapxml.c
 * code to parse nmap's XML output
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
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <glib.h>

#define DEBUG(x) printf x

/*
 * a Description for a Job
 */
typedef struct _nmap_host_port_t {
	xmlChar *protocol;
	int portid;
	xmlChar *state;
	xmlChar *service;
} nmap_host_port_t; 
 
typedef struct _nmap_host_t {
	xmlChar *addr;
	GList *ports;
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

		g_free_not_null(n->os.name);
		g_free_not_null(n->os.accuracy);
	}
}

static void parseNmapScanPortService(nmap_host_port_t *port, xmlNode *node)
{
	xmlAttr *prop = node->properties;

	DEBUG(("%s()\n", __FUNCTION__));
	while(prop)
	{
		if(prop->type == XML_ATTRIBUTE_NODE)
		{
			if(0 == xmlStrcmp(prop->name, (const xmlChar *) "name"))
			{
				if(prop->val && prop->val->type == XML_TEXT_NODE)
				{
					if(port->service)
						g_free(port->service);
					port->service = g_strdup(prop->val->content);
				}
			}
		}
		prop = prop->next;
	}
}

static void parseNmapScanPortState(nmap_host_port_t *port, xmlNode *node)
{
	xmlAttr *prop = node->properties;

	DEBUG(("%s()\n", __FUNCTION__));
	while(prop)
	{
		if(prop->type == XML_ATTRIBUTE_NODE)
		{
			if(0 == xmlStrcmp(prop->name, (const xmlChar *) "state"))
			{
				if(prop->val && prop->val->type == XML_TEXT_NODE)
				{
					if(port->state)
						g_free(port->state);
					port->state = g_strdup(prop->val->content);
				}
			}
		}
		prop = prop->next;
	}
}

static void parseNmapScanPort(nmap_scan_t *nms, xmlNode *node)
{
	xmlAttr *prop = node->properties;
	nmap_host_port_t *port = malloc(sizeof(*port));
	
	DEBUG(("%s()\n", __FUNCTION__));
	if(port == NULL)
	{
		perror("out of memory");
		exit(-1);
	}
	memset(port, 0, sizeof(*port));
	
	while(prop)
	{
		if(prop->type == XML_ATTRIBUTE_NODE)
		{
			if(0 == xmlStrcmp(prop->name, (const xmlChar *) "protocol"))
			{
				if(prop->val && prop->val->type == XML_TEXT_NODE)
				{
					if(port->protocol)
						g_free(port->protocol);
					port->protocol = g_strdup(prop->val->content);
				}
			}
			else if(0 == xmlStrcmp(prop->name, (const xmlChar *) "portid"))
			{
				if(prop->val && prop->val->type == XML_TEXT_NODE)
					port->portid = atoi(prop->val->content);
			}
		}
		prop = prop->next;
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
		}
		node = node->next;
	}
	
	nms->host.ports = g_list_prepend(nms->host.ports, port);
}

static void parseNmapScanPorts(nmap_scan_t *nms, xmlNode *node)
{
	DEBUG(("%s()\n", __FUNCTION__));
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
	xmlAttr *prop = node->properties;

	DEBUG(("%s()\n", __FUNCTION__));
	while(prop)
	{
		if(prop->type == XML_ATTRIBUTE_NODE)
		{
			if(0 == xmlStrcmp(prop->name, (const xmlChar *) "name"))
			{
				if(prop->val && prop->val->type == XML_TEXT_NODE)
				{
					if(nms->os.name)
						g_free(nms->os.name);
					nms->os.name = g_strdup(prop->val->content);
				}
			}
		}
		prop = prop->next;
	}
}

static void parseNmapScanOS(nmap_scan_t *nms, xmlNode *node)
{
	DEBUG(("%s()\n", __FUNCTION__));
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
	xmlAttr *prop = node->properties;

	DEBUG(("%s()\n", __FUNCTION__));
	while(prop)
	{
		if(prop->type == XML_ATTRIBUTE_NODE)
		{
			if(0 == xmlStrcmp(prop->name, (const xmlChar *) "addr"))
			{
				if(prop->val && prop->val->type == XML_TEXT_NODE)
				{
					if(nms->host.addr)
						g_free(nms->host.addr);
					nms->host.addr = g_strdup(prop->val->content);
				}
			}
		}
		prop = prop->next;
	}
}

static void parseNmapScanHost(nmap_scan_t *nms, xmlNode *node)
{
	DEBUG(("%s()\n", __FUNCTION__));
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
		}
		node = node->next;
	}
}

static nmap_scan_t *parseNmapScan(xmlDoc *doc)
{
	nmap_scan_t *ret;
	nmap_scan_t *curscan;
	xmlNode *cur;
	xmlAttr *prop;
	
	DEBUG(("%s()\n", __FUNCTION__));
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


	prop = cur->properties;
	while(prop)
	{
		if(prop->type == XML_ATTRIBUTE_NODE)
		{
			if(0 == xmlStrcmp(prop->name, (const xmlChar *) "xmloutputversion"))
			{
				
			}
			else if(0 == xmlStrcmp(prop->name, (const xmlChar *) "version"))
			{
				if(prop->val && prop->val->type == XML_TEXT_NODE)
					ret->version = g_strdup(prop->val->content);
			}
			else if(0 == xmlStrcmp(prop->name, (const xmlChar *) "args"))
			{
				if(prop->val && prop->val->type == XML_TEXT_NODE)
					ret->args = g_strdup(prop->val->content);
			}
			else if(0 == xmlStrcmp(prop->name, (const xmlChar *) "scanner"))
			{
				if(prop->val && prop->val->type == XML_TEXT_NODE)
					ret->scanner = g_strdup(prop->val->content);
			}
		}
		prop = prop->next;
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
		printf("host port protocol = %s\n", port->protocol);
		printf("host port portid = %d\n", port->portid);
		printf("host port state = %s\n", port->state);
		printf("host port service = %s\n", port->service);
	}
}

static void handleNmapScan(nmap_scan_t * cur)
{
	int i;

	printf("scanner = %s\n", cur->scanner);
	printf("args = %s\n", cur->args);
	printf("version = %s\n", cur->version);

	printf("host addr = %s\n", cur->host.addr);
	g_list_foreach(cur->host.ports, (GFunc)printPorts, NULL);
	
	printf("host os name = %s\n", cur->os.name);
}

xmlDoc *runNmapScan(char *ip)
{
	char buffer[40000];
	char cmd[200];
	FILE *pfp;
	int i;
	
	sprintf(cmd, "nmap %s -O -oX -", ip);
	pfp = popen(cmd, "r");
	if(pfp == NULL)
	{
		perror("I could not run nmap");
		exit(-1);
	}
	
	i = 0;
	while(!feof(pfp) && i < sizeof(buffer))
	{
		buffer[i] = fgetc(pfp);
		if(feof(pfp))
			buffer[i] = '\0';
		i++;
	}
	if(i == sizeof(buffer))
	{
		perror("we ran out of buffer space for nmap's output");
		exit(-1);
	}
	pclose(pfp);
	
	return(xmlParseDoc((xmlChar *)buffer));
}

int main(int argc, char **argv)
{
	int i;
	nmap_scan_t *cur;
	xmlDoc *doc;
	
	/* COMPAT: Do not genrate nodes for formatting spaces */
	LIBXML_TEST_VERSION xmlKeepBlanksDefault(0);

	for (i = 1; i < argc; i++) {
#if 0
		/*
		 * build an XML tree from a the file;
		 */
		doc = xmlParseFile(argv[i]);
#else
		doc = runNmapScan(argv[i]);
#endif
		if (doc == NULL)
		{
			fprintf(stderr, "Error parsing file '%s'\n", argv[i]);
			continue;
		}

		cur = parseNmapScan(doc);
		if (cur)
			handleNmapScan(cur);
		else
			fprintf(stderr, "Error parsing file '%s'\n", argv[i]);
		
		nmap_scan_free(cur);

	}

	xmlCleanupParser();

	return (0);
}
