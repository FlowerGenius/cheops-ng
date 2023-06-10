/*
 * Cheops Next Generation GUI
 * 
 * event.c
 * General event management routines
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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>

#ifdef FREEBSD
	#include <netinet/in_systm.h>
#endif

#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#include "logger.h"

#if defined(HAS_SSL) && defined(USING_SSL)
	#include <openssl/ssl.h>
	#include <openssl/err.h>
	#include <openssl/rand.h>
#endif

#ifdef COMPILING_GUI
	#include <gtk/gtk.h>
	#include "gui-io.h"
#else
	#include "io.h"
#endif

#include "misc.h"
#include "event.h"
#include "cerror.h"
#include "ip_utils.h"

#ifndef FREEBSD
#include <shadow.h>
#endif
#include <pwd.h>

char *crypt(const char *key, const char *salt);


/* Maximum length of local/unix socket */
#define MAX_SUN_LEN 108

/* Number of connections to let wait */
#define AGENT_BACKLOG 10

//#define DEBUG_EVENT

#ifdef DEBUG_EVENT
	#define DEBUG(a) a
#else
	#define DEBUG(a) 
#endif

#ifdef FREEBSD
	#define PF_FILE PF_LOCAL
#endif

#define MY_CERT "/etc/ssl/cheops-agent.crt"
#define MY_PRIVATE_KEY MY_CERT

/* How many buckets for the event hash? */
static cheops_event_handler handlers[MAX_EVENT];

/* We accept sockets here */
static agent *our_local_agent = NULL;
static agent *our_local_ipv4_agent = NULL;

static agent *agents=NULL;

int authenticate_connecting_clients = 0;

#if defined(HAS_SSL) && defined(USING_SSL)
	int initalized_ssl = 0;
	int connect_with_ssl = 0;
	SSL_CTX *ssl_ctx;
#endif

static char unix_connect_string[] 	= "handle_unix_connect";
static char local_string[] 		= "local_agent";
static char ipv4_connect_string[] 	= "handle_ipv4_connect";
static char ipv4_string[] 		= "ipv4_agent";

int handle_auth_request(event_hdr *h, event *e, agent *a);
int send_auth_request_reply(agent *a, int authenticated);

void agent_authenticate_clients(agent *a, int enable)
{
	if(enable)
	{
		if(a)
			a->flags |= AGENT_FLAGS_AUTHENTICATE_CLIENTS;
	}
	else
	{
		if(a)
			a->flags &= ~AGENT_FLAGS_AUTHENTICATE_CLIENTS;
	}
	
}
int agent_authenticating_clients(agent *a)
{
	if(a && a->flags & AGENT_FLAGS_AUTHENTICATE_CLIENTS)
		return(1);
	
	return(0);  
}
                
                
void authenticate_clients(int enable)
{
	authenticate_connecting_clients = (enable ? 1 : 0);
}

int authenticating_clients(void)
{
	return(authenticate_connecting_clients);
}

#if defined(HAS_SSL) && defined(USING_SSL)
void agent_use_ssl(agent *a, int enable)
{
	if(enable)
	{
		if(a)
			a->flags |= AGENT_FLAGS_USING_SSL;
	}
	else
	{
		if(a)
			a->flags &= ~AGENT_FLAGS_USING_SSL;
	}
}

void use_ssl(int enable)
{
	connect_with_ssl = (enable ? 1 : 0);
}

int using_ssl(void)
{
	return(connect_with_ssl);
}

int agent_using_ssl(agent *a)
{
	if(a && a->flags & AGENT_FLAGS_USING_SSL)
		return(1);
	
	return(0);	
}
#endif /* HAS_SSL */

static agent *new_agent()
{
	agent *a = (agent *)malloc(sizeof(agent));
	if(a)
	{
		a->write_ptr = a->buffer;
		a->expected_length = -1;
		a->flags = 0;
	}
	return(a);
}

static char *get_local_name()
{
	/*
	 * Determine what the name of our local socket
	 * should be.  Returns NULL upon failure.
	 */
	 
	static char buf[256];
	char *c;
	c = getenv("HOME");
	if (!c) {
		clog(LOG_ERROR, "No HOME environment variable.\n");
		return NULL;
	}
	snprintf(buf, sizeof(buf), "%s/.cheops/comsocket",c);
	if (strlen(buf) > MAX_SUN_LEN) {
		clog(LOG_ERROR, "Socket name '%s' too long (must be <= %d).\n", buf, MAX_SUN_LEN);
		return NULL;
	}
	return buf;
}

static int agent_read(agent *a, int fd, char *buffer, int length)
{
	int ret;

#if defined(HAS_SSL) && defined(USING_SSL)
	if(a->ssl)
	{
		while(1)
		{
			fprintf(stderr, "agent_read(): SSL_read()\n");
			ret = SSL_read(a->ssl, buffer, length);
			if(ret != 0)
			{
				if(ret < 0)
				{
					switch(SSL_get_error(a->ssl, ret))
					{
						case SSL_ERROR_WANT_READ:
							fprintf(stderr, "agent_read(): SSL_ERROR_WANT_READ\n");
							continue;
							break;
						case SSL_ERROR_WANT_WRITE:
							fprintf(stderr, "agent_read(): SSL_ERROR_WANT_WRITE\n");
							SSL_write(a->ssl, NULL, 0);
							break;
					}
				}
			}
			break;
		}
	}
	else
#endif
	{
		ret = read(fd, buffer, length);
	}
	
	return(ret);
}

static int handle_agent(int *id, int fd, short events, void *data)
{
	/*
	 * Read and handle the event.
	 */
	static event_hdr *h;
	event *e;
	int res;
	agent *a;

	DEBUG(printf("handle_agent(%p, %d, %04X, %p)\n", id, fd, events, data));
	
	a = (agent *)data;
	if (!a) {
		/* Something bad has happened, try not to repeat it */
		clog(LOG_WARNING, "NULL data passed!\n");
		return 0;
	}
	if (a->s != fd) {
		/* The agent and the descriptor we got this on don't match */
		clog(LOG_WARNING, 
			"File descriptor mismatch (%d != %d)\n", a->s, fd);
		return 0;
	}
	/*
	 * Check for errors first 
	 */
	if (events & (CHEOPS_IO_ERR | CHEOPS_IO_HUP | CHEOPS_IO_NVAL)) {
		clog(LOG_DEBUG, 
		     "Agent destroyed %s %s %s %s\n",
		     (events & CHEOPS_IO_ERR ? "CHEOPS_IO_ERR" : ""),
		     (events & CHEOPS_IO_HUP ? "CHEOPS_IO_HUP" : ""),
		     (events & CHEOPS_IO_NVAL ? "CHEOPS_IO_NVAL" : ""),
		     (events & CHEOPS_IO_IN ? "CHEOPS_IO_IN" : ""));
		a->id = 0;
		event_destroy_agent(a);
		return 0;
	}
	DEBUG(clog(LOG_DEBUG, "Agent data on %d (%d)\n", fd, events));

	h = (event_hdr *)a->buffer;
	res = AGENT_RECV_BUFFER_LENGTH - (a->write_ptr - a->buffer);

	if(res == 0)
	{
		printf("hey you need to increase the buffer size need %d\n", a->expected_length);
		exit(1);
	}

	// -1 == new message comming in
	// 0  == new message in progress, but dont have length yet
	// #  == this is the length of the message that we are to recieve
	if(a->expected_length == -1)
	{
//		res = read(fd, &h->len, res);
		res = agent_read(a, fd, (char *)&h->len, res);
		if(res < 0)
		{
			DEBUG(clog(LOG_WARNING,
				"Closing connection on read error\n"));
			event_destroy_agent(a);
			return 0;
		}
		if(res == 0)
		{
			DEBUG(clog(LOG_WARNING,
				"Closing connection on NULL read (socket closed)\n"));
			event_destroy_agent(a);
			return 0;
		}
		if(res < sizeof(h->len))
		{
			a->expected_length = 0;
		}
		else
		{
			if (ntohl(h->len) > sizeof(a->buffer)) {
				clog(LOG_WARNING,
				        "event too large: %d bytes (max = %d)\n", ntohl(h->len), sizeof(a->buffer));
				return 0;
			}
			a->expected_length = ntohl(h->len);
		}
		a->write_ptr += res;
	}
	else
	{
//		res = read(fd, a->write_ptr, res);
		res = agent_read(a, fd, (char *)a->write_ptr, res);
		if(res < 0)
		{
			DEBUG(clog(LOG_WARNING,
				"Closing connection on read error\n"));
			event_destroy_agent(a);
			return 0;
		}
		if(res == 0)
		{
			DEBUG(clog(LOG_WARNING,
				"Closing connection on NULL read (socket closed)\n"));
			event_destroy_agent(a);
			return 0;
		}
again:
		if(res < sizeof(h->len) - (a->write_ptr - a->buffer))
		{
			a->expected_length = 0;
		}
		else
		{
			if (ntohl(h->len) > sizeof(a->buffer)) {
				clog(LOG_WARNING,
				        "event too large: %d bytes (max = %d)\n", ntohl(h->len), sizeof(a->buffer));
				return 0;
			}
			a->expected_length = ntohl(h->len);
		}

		a->write_ptr += res;
	}
	
	if( (a->write_ptr - a->buffer) >= a->expected_length &&
	    a->expected_length > 0)
	{
		// we have a message sitting in our buffer, now do it
		if (ntohs(h->type) >= MAX_EVENT){
			clog(LOG_WARNING,
				"Message type %d is too large\n", h->type);
		} 
		else if (ntohs(h->hlen) > ntohl(h->len)) 
		{
			clog(LOG_WARNING,
				"Message header longer than message itself?\n", ntohs(h->type));
		} 
		else 
		{
			if((a->flags & AGENT_FLAGS_LOGGED_IN) || !authenticating_clients())
			{
				e = (event *)(a->buffer + ntohs(h->hlen));
				DEBUG(clog(LOG_DEBUG, "Event received of type %d\n", ntohs(h->type)));
				if (handlers[ntohs(h->type)]) 
				{
					handlers[ntohs(h->type)](h, e, a);
				}
				else 
				{
					clog(LOG_DEBUG, "No handler for event type %d\n", ntohs(h->type));
					if (!(htonl(h->flags) & FLAG_IGNORE)) 
					{
						cheops_error(a, ERROR_UNIMPL, "Unsupported message type %d\n", ntohs(h->type));
					}
				}
			}
			else
			{
				/*
				 * if the user is not logged in then the only thing that we will
				 * reply with is event error
				 */
				e = (event *)(a->buffer + ntohs(h->hlen));
				printf("not logged in!\n");
				if( ntohs(h->type) == EVENT_AUTH_REQUEST)
				{
					handlers[EVENT_AUTH_REQUEST](h,e,a);
				}
				else
				{
					clog(LOG_DEBUG, "No handler for event type %d when not logged in\n", ntohs(h->type));
					if (!(htonl(h->flags) & FLAG_IGNORE)) 
					{
						cheops_error(a, ERROR_UNIMPL, "Unsupported message type %d\n", ntohs(h->type));
					}
				}
			}
		}

		// move the message over to the head of the 
		res = a->write_ptr - &a->buffer[a->expected_length];
		
		memcpy(a->buffer, 
		       &a->buffer[a->expected_length],
		       res);

		a->write_ptr = a->buffer; // i am going to move this in "again:"
		a->expected_length = 0;
		
		goto again;
	}
	
	/* Keep running */
	return 1;
}


static int handle_unix_connect(int *id, int fd, short events, void *data)
{
	/*
	 * Read and handle the event.
	 */
	int res;
	agent *a;
	struct sockaddr_un sun;
	int sunlen;
	
	a = (agent *)data;
	if (!a) {
		/* Something bad has happened, try not to repeat it */
		clog(LOG_WARNING, "NULL data passed!\n");
		return 0;
	}
	if (a->s != fd) {
		/* The agent and the descriptor we got this on don't match */
		clog(LOG_WARNING, 
			"File descriptor mismatch (%d != %d)\n", a->s, fd);
		return 0;
	}
	DEBUG(clog(LOG_DEBUG, "Agent data on %d\n", fd));
	sunlen = sizeof(sun);
	res = accept(fd, (void *)&sun, &sunlen);
	if (res < 0) {
		clog(LOG_WARNING,
			"accept failed: %s\n", strerror(errno));
	} else {
		a = new_agent();
		if (a) {
			a->type = AGENT_TYPE_LOCAL;
			a->s = res;
			a->id = cheops_io_add(res, handle_agent, CHEOPS_IO_IN, a);
			a->next = agents;
			a->more = &unix_connect_string[0];
			agents = a;
		}
	}
	
	/* Keep running */
	return 1;
}

static agent *local_agent()
{
	/* Connect to a local agent, or start one if one hasn't been
	   started yet.  Local connections are done with UNIX space
	   sockets, although they could be done with other things
	   as well. */
	int s;
	char *c;
	agent *a = NULL;
	struct sockaddr_un sun;
	if (our_local_agent)
		return our_local_agent;
	if (!(c = get_local_name())) {
		clog(LOG_ERROR, "Unable to get local name\n");
		return NULL;
	}
	s = socket(PF_FILE, SOCK_STREAM, 0);
	if (s < 0) 
		clog(LOG_ERROR, "Unable to create UNIX socket: %s\n",strerror(errno));
	else {
		memset(&sun, 0, sizeof(sun));
		sun.sun_family = AF_UNIX;
		memcpy(&sun.sun_path, c, strlen(c));
		
		if (connect(s, (void *)&sun, SUN_LEN(&sun)) < 0) {
			/* FIXME: We should fork() and exec cheops-agent to start it
			   if we cannot connect */
			DEBUG(
				clog(LOG_ERROR, "Unable to connect to local agent (%s)\n", strerror(errno));
			);
			return NULL;
		}
		a = new_agent();
		if (a) {
			a->type = AGENT_TYPE_LOCAL;
			a->id = cheops_io_add(s, handle_agent, CHEOPS_IO_IN | CHEOPS_IO_ERR,a);
			a->s = s;
			a->more = &local_string[0];
			a->next = agents;
			agents = a;
		}
	}
	return a;
}

static int make_local_socket(char *c)
{
	/*
	 * Make a local socket, and return the int.  
	 * returns -1 on failure, and a fd for
	 * for success.
	 */
	 
	struct sockaddr_un sun;
	int s;
	int flags;
	int retry=0;
	int on = 1;
	make_home_dir();
	memset(&sun, 0, sizeof(sun));
	sun.sun_family = PF_FILE;
	strncpy(sun.sun_path, c, sizeof(sun.sun_path));
	s = socket(PF_UNIX, SOCK_STREAM, 0);
	if (s < 0)
		return -1;
tryagain:
	if (bind(s, (struct sockaddr *)&sun, SUN_LEN(&sun)) < 0) {
		/* FIXME:  We should try to see if there is a socket in
		   file space that is preventing us from doing so.  If so,
		   we should test it to see if it's for real or dead, and if
		   dead, remove it and try again. */
		if (!retry) {
			unlink(c);
			retry++;
			goto tryagain;
		} else {
			close(s);
			return -1;
		}
	}

	/* Try to set non-blocking mode */
	if ((flags = fcntl(s, F_GETFL)) < 0)
		clog(LOG_WARNING, "fcntl(F_GETFL) failed(%s)\n", strerror(errno));
	else
		if (fcntl(s, F_SETFL, (flags | O_NONBLOCK)) < 0)
			clog(LOG_WARNING, "fcntl(F_SETFL) failed(%s)\n", strerror(errno));

	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&on, sizeof(on));

	if (listen(s, AGENT_BACKLOG) < 0) {
		clog(LOG_ERROR, "listen() failed (%s)\n", strerror(errno));
		close(s);
		s = -1;
	}
	return s;
}

static int handle_ipv4_connect(int *id, int fd, short events, void *data)
{
	/*
	 * Read and handle the event.
	 */
	int res;
	agent *a;
	struct sockaddr_in sin;
	int sinlen;
	
	a = (agent *)data;
	if (!a) {
		/* Something bad has happened, try not to repeat it */
		clog(LOG_WARNING, "NULL data passed!\n");
		return 0;
	}
	if (a->s != fd) {
		/* The agent and the descriptor we got this on don't match */
		clog(LOG_WARNING, 
			"File descriptor mismatch (%d != %d)\n", a->s, fd);
		return 0;
	}
	DEBUG(clog(LOG_DEBUG, "Agent data on %d\n", fd));
	sinlen = sizeof(sin);
	res = accept(fd, (void *)&sin, &sinlen);
	if (res < 0) {
		clog(LOG_WARNING,
			"accept failed: %s\n", strerror(errno));
	} else {
		a = new_agent();
		if (a) {
			a->type = AGENT_TYPE_IPV4;
			a->s = res;
			a->id = cheops_io_add(res, handle_agent, CHEOPS_IO_IN, a);
			a->next = agents;
			a->more = &ipv4_connect_string[0];
			a->flags = 0;
			
#if defined(HAS_SSL) && defined(USING_SSL)
			if( using_ssl() ) {
				int ret;
				
				if(!initalized_ssl) {
					RAND_seed("/dev/random", 50);	
					SSL_load_error_strings();
					SSL_library_init();

					if( (ssl_ctx = SSL_CTX_new(SSLv2_server_method())) == NULL) {
						clog(LOG_ERROR, "Unable to create the ssl ctx\n");
					}
					if (SSL_CTX_use_certificate_file(ssl_ctx, MY_CERT, SSL_FILETYPE_PEM) <= 0) {
						ERR_print_errors_fp(stderr);
						exit(3);
					}
					if (SSL_CTX_use_PrivateKey_file(ssl_ctx, MY_PRIVATE_KEY, SSL_FILETYPE_PEM) <= 0) {
						ERR_print_errors_fp(stderr);
						exit(4);
					}
					if (!SSL_CTX_check_private_key(ssl_ctx)) {
						fprintf(stderr,"Private key does not match the certificate public key\n");
						exit(5);   
					}
				}
				if((a->ssl = SSL_new(ssl_ctx)) == NULL) {
					clog(LOG_ERROR, "Unable to create the ssl\n");
				}
				if(!SSL_set_fd(a->ssl, a->s)) {
					clog(LOG_ERROR, "Unable to set the SSL_fd\n");
				}
				
				if((ret = SSL_accept(a->ssl)))
				{
					clog(LOG_ERROR, "they are in\n");
				}
				else
				{
					clog(LOG_ERROR, "they are not in\n");
				}
			}
			else
#endif /* HAS_SSL */
			{
				a->ssl = NULL;
			}

			agents = a;

			if(authenticating_clients())
			{
				DEBUG(printf("Sending auth request reply with !authenticated\n"));
				send_auth_request_reply(a, 0);
			}
		}
	}
	
	/* Keep running */
	return 1;
}

static agent *ipv4_agent(char *ip_addr, int usessl, int is_client)
{
	/* We asynchronously request a ipv4 agent here.
	   To do so, we allocate some random socket and
	   send/listen on it. This is used for the shell */

	int s;
	agent *a = NULL;
	int optval=1;
	
	struct sockaddr_in sin;
	if(our_local_ipv4_agent && !is_client)
		return(our_local_ipv4_agent);

	s = socket(PF_INET, SOCK_STREAM, 0);
	if (s < 0) 
		clog(LOG_ERROR, "Unable to create UDP socket: %s\n",strerror(errno));
	else 
	{
		sin.sin_family = AF_INET;
		sin.sin_port = htons(2300);
		sin.sin_addr.s_addr = inet_addr(ip_addr);

		setsockopt(s, SOL_SOCKET, TCP_NODELAY, &optval,sizeof(optval));
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));	
		
		if (connect(s, (void *)&sin, sizeof(sin)) < 0)
		{
			/* FIXME: We should fork() and exec cheops-agent to start it
			   if we cannot connect */
			clog(LOG_ERROR, "Unable to connect to ipv4 agent '%s' (%s)\n", ip_addr, strerror(errno));
			return NULL;
		}

		a = new_agent();
		if (a)
		{
			a->type = AGENT_TYPE_IPV4;
			a->s = s;
			a->more = &ipv4_string[0];

#if defined(HAS_SSL) && defined(USING_SSL)
			if( usessl ) 
			{
				if(!initalized_ssl) 
				{
					RAND_seed("/dev/random", 50);	
					SSL_load_error_strings();
					SSL_library_init();

					if( (ssl_ctx = SSL_CTX_new(SSLv2_client_method())) == NULL) 
					{
						clog(LOG_ERROR, "Unable to create the ssl ctx\n");
					}
				}
				if((a->ssl = SSL_new(ssl_ctx)) == NULL) 
				{
					clog(LOG_ERROR, "Unable to create the ssl\n");
				}
				if(!SSL_set_fd(a->ssl, a->s)) 
				{
					clog(LOG_ERROR, "Unable to set the SSL_fd\n");
				}
				
				a->id = cheops_io_add(a->s, handle_agent, CHEOPS_IO_IN, a);
			}
			else
#endif /* HAS_SSL */
			{
				a->ssl = NULL;
				a->id = cheops_io_add(s, handle_agent, CHEOPS_IO_IN | CHEOPS_IO_ERR,a);
			}

			a->next = agents;
			agents = a;
		}
	}
	if(!is_client)
		our_local_ipv4_agent = a;
#if defined(HAS_SSL) && defined(USING_SSL)	
	agent_use_ssl(a, usessl);
#endif
	return a;
}

static int make_ipv4_socket(struct sockaddr_in *sin)
{
	/*
	 * Make a ipv4 socket, and return the int.  
	 * returns -1 on failure, and a fd for
	 * for success. This is used by the agent
	 */
	 
	int s;
	int flags;
	int optval = 1;
		
	if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
	{
		clog(LOG_ERROR, "socket failed(%s)\n", strerror(errno));
		return -1;
	}
	if(bind(s, (struct sockaddr *)sin, sizeof(*sin)) < 0) {
		clog(LOG_ERROR, "bind failed(%s)\n", strerror(errno));
		close(s);
		return -1;
	}

	if(setsockopt(s, SOL_SOCKET, TCP_NODELAY, &optval,sizeof(optval)) < 0)
	{
		clog(LOG_ERROR, "Unable to set TCP_NODELAY");
	}		
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&optval, sizeof(optval));
	
	if((flags = fcntl(s, F_GETFL)) < 0)
		clog(LOG_ERROR, "fcntl(F_GETFL) failed(%s)\n", strerror(errno));
	else
		if(fcntl(s, F_SETFL, (flags | O_NONBLOCK)) < 0)
			clog(LOG_ERROR, "fcntl(F_SETFL) failed(%s)\n", strerror(errno));

	if(listen(s, AGENT_BACKLOG) < 0) {
		clog(LOG_ERROR, "listen() failed (%s)\n", strerror(errno));
		close(s);
		s = -1;
	}
	
	return s;
}


void event_destroy_agent(agent *a)
{
	/* 
	 * Unlink the agent 
	 */

	char *c;
	agent *l,*d;
	d=NULL;
	l=agents;

	while(l) {
		if (l == a) {
			if (d)
				d->next = l->next;
			else
				agents=agents->next;
			break;
		}
		d=l;
		l=l->next;
	}
	
	if (!l) {
		/* If they weren't in our agent list, don't touch it!  It might
		   be some old, expired pointer, and as long as we don't 
		   dereference it, everything will be fine.  But log a warning
		   just in case */
		clog(LOG_WARNING, "Asked to remove non-existant agent (%p)!\n", a);
		return;
	}

	if( shutdown(a->s, SHUT_WR) < 0)
	{
		DEBUG(
			clog(LOG_WARNING, "shutdown(s,SHUT_WR) is not happy (%s)\n",strerror(errno));
		);
	}
	close(a->s);
	if (a->id)
		cheops_io_remove(a->id);

	/* 
	 * Special treatment for UNIX sockets 
	 */		
	if (a == our_local_agent) {
		if ((c = get_local_name())) {
			unlink(c);
		}
		our_local_agent=NULL;
	}
	free(a);
}

int event_create_agent(int agent_type)
{
	/*
	 * Create an event listener for each type
	 * present in agent_type.  Return -1 upon
	 * failure, and 0 upon success
	 */

	char *c;
	int res=-1;
	agent *a;
	char hostname[80];
	struct hostent *hp;
	struct sockaddr_in sin;
		
	if (agent_type & AGENT_TYPE_LOCAL) {
		if (!our_local_agent) {
			if ((c = get_local_name())) {
				res = make_local_socket(c);
				if (res > -1) {
					a = new_agent();
					if (a) {
						a->type = AGENT_TYPE_LOCAL;
						a->s = res;
						a->id = cheops_io_add(res, handle_unix_connect, CHEOPS_IO_IN, a);
						a->next = agents;
						a->more = NULL;
						agents = a;
						our_local_agent = a;
						res=0;
					} else {
						res=-1;
					}
				} else
					clog(LOG_ERROR, "Unable to create socket (%s).\n", strerror(errno));
			}
		}
	}
	
	if (agent_type & AGENT_TYPE_IPV4) 
	{
		sin.sin_family = AF_INET;
		sin.sin_port = htons(2300);
		sin.sin_addr.s_addr = INADDR_ANY;

		res = make_ipv4_socket(&sin);
		if (res > -1)
		{
			a = new_agent();

			if (a) 
			{
				a->type = AGENT_TYPE_IPV4;
				a->s = res;
				a->next = agents;
				a->more = NULL;
				a->ssl = NULL;
				a->id = cheops_io_add(res, handle_ipv4_connect, CHEOPS_IO_IN, a);
				agents = a;
				res = 0;
			} 
			else 
			{
				res = -1;
			}
		}
	}
	return res;
}

static inline agent *chk_agent(agent *a) 
{
	/*
 	 * Check to be sure this agent really exists
	 */
	agent *ta = agents;
	while(ta) 
	{
		if (ta == a)
			break;
		ta = ta->next;
	}
	return ta;
}

int agent_write(agent *a, char *buffer, int length)
{
	int ret = -1;
	
	switch(a->type)
	{
		case AGENT_TYPE_LOCAL:
			ret = write(a->s, buffer, length);
			break;
		case AGENT_TYPE_IPV4:
#if defined(HAS_SSL) && defined(USING_SSL)
			if(a->ssl)
			{
				while(1)
				{
					fprintf(stderr, "agent_write(): SSL_write()\n");
					ret = SSL_write(a->ssl, buffer, length);
					if(ret != 0)
					{
						if(ret < 0)
						{
							switch(SSL_get_error(a->ssl, ret))
							{
								case SSL_ERROR_WANT_READ:
									fprintf(stderr, "agent_write(): SSL_ERROR_WANT_READ\n");
									SSL_read(a->ssl, NULL, 0);
									break;
								case SSL_ERROR_WANT_WRITE:
									fprintf(stderr, "agent_write(): SSL_ERROR_WANT_WRITE\n");
									continue;
									break;
							}
						}
					}
					break;
				}
			}
			else
#endif
			{
				ret = send(a->s, buffer, length,0);
			}
			break;
		default:
			ret=-1;
			break;
	}
	if(ret != length)
	{
		fprintf(stderr, "agent_write(): did not write %d but %s\n", length, ret);
	}
	return(ret);
}

int event_send(agent *a, event_hdr *h)
{
	/*
	 * Send an event after being sure it's
	 * valid.
	 */
	int res=-1;
	static pthread_mutex_t mutex;
	static int initalized = 0;
	
	if(!initalized)
	{
		pthread_mutex_init(&mutex, NULL);
		initalized = 1;
	}
	
	pthread_mutex_lock(&mutex);
	
	a = chk_agent(a);
	if (a) {
		res = agent_write(a, (char *)h, ntohl(h->len));
		
		if (res < 0) {
			clog(LOG_WARNING, "write failed: %s\n", strerror(errno));
		} else if (res != ntohl(h->len)) {
			clog(LOG_WARNING, "only wrote %d of %d bytes\n", res, ntohl(h->len));
		}
	} else
		clog(LOG_DEBUG, "agent not valid\n");
	
	pthread_mutex_unlock(&mutex);
		
	return res;
}

agent *event_request_agent(int agent_type, void *addr, int usessl)
{
	/* 
	 * Requests that an agent connection be established 
	 * using the specified type and address (if applicable).
	 * The interpretation of addr is specific to the agent
	 * type.
	 * 
	 * If successful, it is added to the agent queue, and 
	 * the default agent is set to this if is_default is
	 * set.
	 *
	 * The return value differs depending on the kind of agent
	 * requested.  AGENT_TYPE_LOCAL agents should immediately
	 * connect, whereas AGENT_TYPE_IPV4, for example, will be
	 * done with callbacks.
	 *
	 * Return 0 upon success and -1 upon failure
	 */
	 
	agent *a=NULL;

	switch(agent_type) {
	case AGENT_TYPE_LOCAL:
	 	/* Communicate using a local method like
		   a UNIX socket or whatever.  Address is
		   not important */
		a = local_agent();
		break;
	case AGENT_TYPE_IPV4:
		/* Communicate by requesting an agent
		   connection over IPV4 */
		a = ipv4_agent((char *)addr, usessl, 1);
		break;
	default:
		clog(LOG_ERROR, "unknown agent requested: %d\n", agent_type);
	}
	return a;
}

extern int event_register_handler(short type, cheops_event_handler h, int replace)
{
	static int initalized = 0;
	
	if(!initalized){
		initalized = 1;
		event_register_handler(EVENT_AUTH_REQUEST, handle_auth_request, 1);
	}
	if (type < MAX_EVENT) {
		if (replace || !handlers[type]) 
			handlers[type] = h;
		return 0;
	} else {
		clog(LOG_ERROR, "Attempt to register handler for event type %d, greater than max %d\n", type, MAX_EVENT);
	}
	return -1;
}


int send_auth_request_reply(agent *a, int authenticated)
{
	static char ebuf[MAX_EVENT_SIZE];
	static event_hdr *eh = (event_hdr *)ebuf;
	static event *ee = (event *)(ebuf + sizeof(event_hdr));
	
	eh->len = htonl(sizeof(event_hdr) + sizeof(auth_request_r));
	eh->hlen = htons(sizeof(event_hdr));
	eh->type = htons(REPLY_AUTH_REQUEST);
	eh->flags = 0;
	ee->auth_request_r.authenticated = htonl(authenticated); // yes is stupid but oh well

	DEBUG(printf("%s() %d\n", __FUNCTION__, authenticated));

	if (event_send(a, eh) < 0)
	{
		DEBUG( clog(LOG_WARNING, "Unable to send discover reply\n") );
		return(0);
	}
	return(1);
	
}

int handle_auth_request(event_hdr *h, event *e, agent *a)
{
#ifndef FREEBSD
	char *user = e->auth_request_e.username;
	char *pass = e->auth_request_e.password;
	char *enc;
	struct passwd *pw;
	struct spwd   *spw;
	int authenticated = 0;
	
	DEBUG(
		printf("%s()\n", __FUNCTION__);
		printf("user '%s'\n", e->auth_request_e.username);
		printf("pass '%s'\n", e->auth_request_e.password);
	);

	if( (pw = getpwnam(user)) == NULL)
	{
		send_auth_request_reply(a, 0);
		return(1);
	}

	if( (spw = getspnam((char *)pw->pw_name)) == NULL)
	{
		send_auth_request_reply(a, 0);
		return(1);
	}	
	enc = crypt(pass, (char *)spw->sp_pwdp);
	DEBUG(
		printf("crypt = %s\n"
		       "pwdb  = %s\n",
		       enc,
		       (char *)spw->sp_pwdp);
	)
	if(0 == strcmp((char *)spw->sp_pwdp, enc))
	{
		a->flags |= AGENT_FLAGS_LOGGED_IN;
		authenticated = 1;
	}
	else
	{
		authenticated = 0;
	}
	send_auth_request_reply(a, authenticated);
#endif
	return(1);
}

void event_cleanup(void)
{
	while(agents) 
	{
		event_destroy_agent(agents);
	}
}



