/*
 * Cheops Next Generation GUI
 * 
 * ip_utils.c
 * Utilities for handeling IP addresses.
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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <net/if.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include "cheops.h"
#include "io.h"
#include "logger.h"
#include "event.h"
#include "cheops-sh.h"
#include "ip_utils.h"
#include "config.h"

#ifdef FREEBSD 
#include <sys/sockio.h>
#include <errno.h>
#include <sys/sysctl.h>
#include <net/route.h> 
#include <net/if_var.h>
#include <kvm.h>
#endif
 
#ifdef LINUX
#include <linux/sockios.h>  /* GLIBC don't have sockios.h? */
#endif
  

typedef struct {
	char ifname[17];
	struct in_addr addr;
} interfacerec;
            
typedef struct {
	struct in_addr addr;
	unsigned long naddr;    /* netmask */
	interfacerec *iface;
} routerec;

static short numinterfaces = 0, numroutes = 0;
static interfacerec *interfaces = NULL;
static routerec *routes = NULL;
                            
                            

int get_host(char *hostname, u32 *ip) 
{
	struct hostent *hp;
	hp = gethostbyname(hostname);
	if (hp) 
		memcpy(ip, hp->h_addr, sizeof(u32));
	else
		return -1;
	return 0;
}

char *ip2str(u32 ip)
{
	return inet_ntoa(*(struct in_addr *)&ip);
}

int str2ip(char *s)
{
	struct in_addr ip;
	
	inet_aton(s,&ip);
	
	return( ip.s_addr);
}

int allones(u32 mask)
{
	/* Check to be sure that this only has only a string of ones,
	   return non-zero on success */
	while(mask & 0x80000000) {
		mask = mask << 1;
	}
	return !mask;
}

unsigned short inet_checksum (void *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;
        
	while (nleft > 1)
	{
		sum += *w++;
		nleft -= 2;
	}
	/* mop up an odd byte, if necessary */
	if (nleft == 1)
	{
		*(u_char *) (&answer) = *(u_char *) w;
		sum += answer;
	}
                                     	                            
	sum = (sum >> 16) + (sum & 0xffff);   /* add hi 16 to low 16 */
	sum += (sum >> 16);		/* add carry */
	answer = ~sum;		/* truncate to 16 bits */
	return (answer);
}

#ifdef FREEBSD

/* 
 * XXX FIXME
 * code largely borrowed from traceroute
 * this could use some cleaning up
 */
#define IPLEN 		38
#define IPHDRSZ		20	/* actually sizeof(struct ip) */
#define UDPHDRSZ	8	/* actually sizeof(struct udphdr) */
#define TIMLEN sizeof(struct timeval)
#define PROBELEN (IPHDRSZ + UDPHDRSZ + TIMLEN + 2)

#ifdef IP_MAXPACKET
#define MAXPACKET IP_MAXPACKET	/* max ip packet size */
#else
#define MAXPACKET 65535
#endif

u_char opacket[MAXPACKET];	/* last output (udp) packet */
struct udphdr udppkt;		/* temporary storage */
struct ip ippkt;		/* temporary storage */

/*
 * IP header parameters.
 */

u_short ident;			/* packet ident */
u_short port = 32768+666;	/* -p  starting udp destination port */
int tos = 0;			/* -t  type of service */
int dontfrag = 0;		/* -f  prevent fragmentation if set to IP_DF */


int send_udp_probe(	int sock, struct sockaddr_in *me, struct sockaddr_in *to,
			int seq, int ttl, int iplen);

int send_udp_probe(	int sock, struct sockaddr_in *me, struct sockaddr_in *to,
			int seq, int ttl, int iplen)
{
	u_char *pkt = opacket;
	struct ip *ip = &ippkt;		/* temporary ip header */
	struct udphdr *udp = &udppkt;	/* temporary udp header */
	struct timeval tv;
	int iphdrlen = IPHDRSZ; 	/* size of ip header minus options */
	int udplen = iplen - iphdrlen;	/* size of udp packet */
	register int n;
	struct sockaddr *toaddr_sa   = (struct sockaddr *)to;


/*
 * On traditional platforms, the kernel inserts the IP options.
 * On 4.4BSD-based platforms, we must construct them ourselves.
 */
#ifdef BSD44
	iphdrlen += optlen;		/* size of ip header plus options */
	iplen += optlen;		/* size of ip packet plus options */
#endif /*BSD44*/

/*
 * Fill in ip header. NOTE: certain fields are in machine byte order.
 * On traditional platforms, the kernel stores ip_v and ip_hl itself,
 * and properly computes ip_hl depending on the IP options.
 * The kernel always generates ip_id and ip_sum.
 */
	ip->ip_v   = (u_char)  IPVERSION;	/* version */
	ip->ip_hl  = (u_char)  (iphdrlen >> 2);	/* header length */
	ip->ip_tos = (u_char)  tos;		/* type of service */
	ip->ip_len = (short)   iplen;		/* total size */
	ip->ip_id  = (u_short) 0;		/* identification */
	ip->ip_off = (short)   dontfrag;	/* fragment offset */
	ip->ip_ttl = (u_char)  ttl;		/* time-to-live */
	ip->ip_p   = (u_char)  IPPROTO_UDP;	/* protocol */
	ip->ip_sum = (u_short) 0;		/* checksum */
	ip->ip_src = me->sin_addr;		/* source address */
	ip->ip_dst = to->sin_addr;		/* destination address */

/*
 * Some platforms require the entire header to be in network byte order.
 */
#ifdef RAW_IP_NET_ORDER
	ip->ip_len = htons((u_short)ip->ip_len);
	ip->ip_off = htons((u_short)ip->ip_off);
#endif /*RAW_IP_NET_ORDER*/

/*
 * Fill in udp header. There is no udp checksum.
 */
	udp->uh_sport = htons(ident);		/* source port */
	udp->uh_dport = htons(port+seq);	/* destination port */
	udp->uh_ulen  = htons((u_short)udplen);	/* udp size */
	udp->uh_sum   = 0;			/* checksum */

/*
 * Construct the output packet.
 */
	bcopy((char *)ip, (char *)pkt, IPHDRSZ);
	pkt += IPHDRSZ;

#ifdef BSD44
	bcopy((char *)optbuf, (char *)pkt, optlen);
	pkt += optlen;
#endif /*BSD44*/

	bcopy((char *)udp, (char *)pkt, UDPHDRSZ);
	pkt += UDPHDRSZ;

/*
 * Fill in remainder of output packet (actually unused).
 */
	(void) gettimeofday(&tv, (struct timezone *)NULL);
	bcopy((char *)&tv, (char *)pkt, TIMLEN);
	pkt += TIMLEN;

	pkt[0] = (u_char)seq;
	pkt[1] = (u_char)ttl;

	/* send raw ip packet */
	ip = (struct ip *)opacket;
	n = sendto(sock, (char *)ip, iplen, 0, toaddr_sa, sizeof(struct sockaddr));

/*
 * Check for errors. Report packet too large for fragmentation.
 */
	if (n < 0 || n != iplen)
	{
		printf("\n");
		if (n < 0)
			perror("sendto");
		else
			printf("sendto: truncated packet\n");

		/* failure */
		return(-1);
	}

	/* successfully sent */
	return(n);
}

void sendicmp (int soc, int ttl, struct in_addr to)
{
	struct sockaddr_in dest, src;
  
	src.sin_addr = getlocalip(to.s_addr);

	src.sin_family = AF_INET;
	src.sin_port = 0;
	dest.sin_addr = to;
	dest.sin_family = AF_INET;

	send_udp_probe(soc, &src, &dest, ttl, ttl, IPLEN);
}

#else



void sendicmp (int soc, int ttl, struct in_addr to)
{
	unsigned char pkt[8192];
	struct ip *ip = (struct ip *)pkt;
	struct icmp *icmp = (struct icmp *)&pkt[sizeof(struct ip)];
	int len;
	struct sockaddr_in dest, src;
  
	src.sin_addr = getlocalip(to.s_addr);

	src.sin_family = AF_INET;
	src.sin_port = 0;
	dest.sin_addr = to;
	dest.sin_family = AF_INET;

/*-- IP HDR --*/
	ip->ip_hl = 5;
	ip->ip_v = 4;
	ip->ip_tos = 0;
	ip->ip_id = htons(getpid());
	ip->ip_off = 0;
	ip->ip_ttl = ttl;
	ip->ip_src = src.sin_addr;
	ip->ip_dst = to;
#if CHEOPS_IP_SUM
	ip->ip_sum = 0;
#else 
#if CHEOPS_IP_CSUM
	ip->ip_csum = 0;
#else
	#error There is no ip_sum or ip_csum defined in the struct ip
#endif
#endif
    len = ICMP_MINLEN;
    dest.sin_port = 0;
    ip->ip_p = IPPROTO_ICMP;

/*-- ICMP HDR --*/
  	icmp->icmp_type = ICMP_ECHO;
  	icmp->icmp_code = 0;
  	icmp->icmp_id = htons(getpid());
  	icmp->icmp_seq = htons(ttl);

  	icmp->icmp_cksum = 0;
 	icmp->icmp_cksum = inet_checksum (icmp, len);

	ip->ip_len = htons(len + sizeof(struct ip));
#if CHEOPS_IP_SUM
	ip->ip_sum = inet_checksum ((void *) ip, sizeof (struct ip)+len);
#else
#if CHEOPS_IP_CSUM
	ip->ip_csum = inet_checksum ((void *) ip, sizeof (struct ip)+len);
#else
	#error There is no ip_sum or ip_csum defined in the struct ip
#endif
#endif
	if (ip->ip_sum == 0)
		ip->ip_sum=0xffff;

	sendto (soc, (void *) pkt, sizeof(struct ip) + len, 0, (struct sockaddr *)&dest, sizeof (dest));
}
#endif

struct in_addr getlocalip (unsigned int dest)
{
	static struct in_addr ina;
	int i;
	
	init_route_tables();
	
	for (i = 0; i < numroutes; i++)
	{
		if ((dest & routes[i].naddr) == (unsigned long) routes[i].addr.s_addr)
		{
			return (routes[i].iface->addr);
		}
	}
	ina.s_addr = 0;
	return ina;
}


#ifdef FREEBSD

/*
 ***************************************************************
 * Start code borrowed from FreeBSD's /usr/src/sys/usr.bin/netstat.
 ***************************************************************
 */
 
/*
 * XXX FIXME
 * not the cleanest, but should be portable across BSD/Sun platforms
 * it extracts the kernel routing table via kvm_read()
 */

/*
 * XXX FIXME
 * should go into header
 */

void routepr(u_long rtree);
static void p_tree(struct radix_node *rn);
static void p_rtentry(struct rtentry *rt);
char *p_sockaddr(struct sockaddr *sa, struct sockaddr *mask, int flags);

static u_long forgemask(u_long a);

int kread(u_long addr, char *buf, int size);
static struct sockaddr *kgetsa(struct sockaddr *dst);

typedef union {
	long	dummy;		/* Helps align structure. */
	struct	sockaddr u_sa;
	u_short	u_data[128];
} sa_u;

static sa_u pt_u;

int	do_rtent = 0;
struct	rtentry rtentry;
struct	radix_node rnode;
struct	radix_mask rmask;
struct	radix_node_head *rt_tables[AF_MAX+1];
static kvm_t *kvmd;
char *nlistf = NULL, *memf = NULL;
int	af;		/* address family */

static struct nlist nl[] = {
#define	N_IFNET		0
	{ "_ifnet" },
#define	N_IMP		1
	{ "_imp_softc" },
#define	N_RTSTAT	2
	{ "_rtstat" },
#define	N_UNIXSW	3
	{ "_localsw" },
#define N_IDP		4
	{ "_nspcb"},
#define N_IDPSTAT	5
	{ "_idpstat"},
#define N_SPPSTAT	6
	{ "_spp_istat"},
#define N_NSERR		7
	{ "_ns_errstat"},
#define	N_CLNPSTAT	8
	{ "_clnp_stat"},
#define	IN_NOTUSED	9
	{ "_tp_inpcb" },
#define	ISO_TP		10
	{ "_tp_refinfo" },
#define	N_TPSTAT	11
	{ "_tp_stat" },
#define	N_ESISSTAT	12
	{ "_esis_stat"},
#define N_NIMP		13
	{ "_nimp"},
#define N_RTREE		14
	{ "_rt_tables"},
#define N_CLTP		15
	{ "_cltb"},
#define N_CLTPSTAT	16
	{ "_cltpstat"},
#define	N_NFILE		17
	{ "_nfile" },
#define	N_FILE		18
	{ "_file" },
#define N_MRTSTAT	19
	{ "_mrtstat" },
#define N_MFCTABLE	20
	{ "_mfctable" },
#define N_VIFTABLE	21
	{ "_viftable" },
#define N_IPX		22
	{ "_ipxpcb"},
#define N_IPXSTAT	23
	{ "_ipxstat"},
#define N_SPXSTAT	24
	{ "_spx_istat"},
#define N_DDPSTAT	25
	{ "_ddpstat"},
#define N_DDPCB		26
	{ "_ddpcb"},
#define N_NGSOCKS	27
	{ "_ngsocklist"},
#define N_IP6STAT	28
	{ "_ip6stat" },
#define N_ICMP6STAT	29
	{ "_icmp6stat" },
#define N_IPSECSTAT	30
	{ "_ipsecstat" },
#define N_IPSEC6STAT	31
	{ "_ipsec6stat" },
#define N_PIM6STAT	32
	{ "_pim6stat" },
#define N_MRT6PROTO	33
	{ "_ip6_mrtproto" },
#define N_MRT6STAT	34
	{ "_mrt6stat" },
#define N_MF6CTABLE	35
	{ "_mf6ctable" },
#define N_MIF6TABLE	36
	{ "_mif6table" },
#define N_PFKEYSTAT	37
	{ "_pfkeystat" },
	{ "" },
};

/* column widths; each followed by one space */
#if 0
#ifndef INET6
#define	WID_DST(af) 	18	/* width of destination column */
#define	WID_GW(af)	18	/* width of gateway column */
#else
#define	WID_DST(af) \
	((af) == AF_INET6 ? (lflag ? 39 : (nflag ? 33: 18)) : 18)
#define	WID_GW(af) \
	((af) == AF_INET6 ? (lflag ? 31 : (nflag ? 29 : 18)) : 18)
#endif /*INET6*/
#endif

#define kget(p, d) (kread((u_long)(p), (char *)&(d), sizeof (d)))

/*
 * Read kernel memory, return 0 on success.
 */
int kread(u_long addr, char *buf, int size)
{
char errbuf[100];

	if (kvmd == 0) {
		/*
		 * XXX.
		 */
		kvmd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf);
		if (kvmd != NULL) {
			if (kvm_nlist(kvmd, nl) < 0) {
				perror("kvm_nlist\n");
				exit(-1);
			}

			if (nl[0].n_type == 0) {
				perror("no namelist\n");
				exit(-1);
			}
		} else {
			perror("kvm not available");
			exit(-1);
		}
	}
	if (!buf)
		return (0);
	if (kvm_read(kvmd, addr, buf, size) != size) {
		snprintf(errbuf, 99, "%s", kvm_geterr(kvmd));
		perror(errbuf);
		return(-1);
	}
	return (0);
}

static struct sockaddr *kgetsa(struct sockaddr *dst)
{
	kget(dst, pt_u.u_sa);
	if (pt_u.u_sa.sa_len > sizeof (pt_u.u_sa))
		kread((u_long)dst, (char *)pt_u.u_data, pt_u.u_sa.sa_len);
	return (&pt_u.u_sa);
}

static u_long forgemask(u_long a)
{
	u_long m;

	if (IN_CLASSA(a))
		m = IN_CLASSA_NET;
	else if (IN_CLASSB(a))
		m = IN_CLASSB_NET;
	else
		m = IN_CLASSC_NET;
	return (m);
}

char *p_sockaddr(struct sockaddr *sa, struct sockaddr *mask, int flags)
{
	static char workbuf[128];
	register char *cp = workbuf;
	
	bzero(workbuf, sizeof(workbuf));

	switch(sa->sa_family) {
	case AF_INET:
	    {
		register struct sockaddr_in *sin = (struct sockaddr_in *)sa;

		if (flags & RTF_HOST) 
		{
			if (~(sin->sin_addr.s_addr | htonl(forgemask(ntohl(sin->sin_addr.s_addr))))) 
			{
				sprintf(cp, "\tRoute:\t%08X\tMask:\t%08X", 
					sin->sin_addr.s_addr, htonl(forgemask(ntohl(sin->sin_addr.s_addr))));
			} 
			else 
			{
				sprintf(cp, " catch net" );
			}
		}
		else if (mask) 
		{
			sprintf(cp, "\tNet:\t%08X\tMask:\t%08X", sin->sin_addr.s_addr, ((struct sockaddr_in *)mask)->sin_addr.s_addr);
		} 
		else 
		{
			sprintf(cp, "\tNet:\t%08X\tMask:\t%08X", sin->sin_addr.s_addr, (long int)0);
		}
		break;
	    }

	default:
	    {
	    	sprintf(cp, " catch if");
	    }
	}

	return cp;
}

static void p_rtentry(struct rtentry *rt)
{
	static struct ifnet ifnet, *lastif;
	static char istr[240];
	static char name[16];
	static char iface[9];
	struct sockaddr *sa;
	struct sockaddr_in *sin = NULL;
	struct sockaddr_in *sim = NULL;
	static routerec *oldroutes = NULL;
	sa_u addr, mask;
	char ret = 0;
	int found, i1;
	/*
	 * Don't use protocol-cloned routes
	 */
	if (rt->rt_parent)
		return;

	if (rt->rt_ifp) {
		if (rt->rt_ifp != lastif) {
			kget(rt->rt_ifp, ifnet);
//			kread((u_long)ifnet.if_xname, name, 16);
//			lastif = rt->rt_ifp;
//			snprintf(iface, sizeof(iface),"%s%d", name, ifnet.if_dunit);
			snprintf(iface, sizeof(iface),"%s", ifnet.if_xname);
		}
	}

	bzero(&addr, sizeof(addr));
	if ((sa = kgetsa(rt_key(rt))))
		bcopy(sa, &addr, sa->sa_len);
	bzero(&mask, sizeof(mask));
	if (rt_mask(rt) && (sa = kgetsa(rt_mask(rt))))
		bcopy(sa, &mask, sa->sa_len);

	switch(addr.u_sa.sa_family) {
	case AF_INET:
	    {
		sin = (struct sockaddr_in *)&addr.u_sa;

		if (rt->rt_flags & RTF_HOST) 
		{
			if (~(sin->sin_addr.s_addr | htonl(forgemask(ntohl(sin->sin_addr.s_addr))))) 
			{
				sim = (struct sockaddr_in *)&mask.u_sa;
				sim->sin_addr.s_addr = htonl(forgemask(ntohl(sin->sin_addr.s_addr)));
				sprintf(istr, "\tRoute:\t%08X\tMask:\t%08X", 
					sin->sin_addr.s_addr, 
					sim->sin_addr.s_addr);
			} 
			else 
			{
				sprintf(istr, " catch net" );
				ret = 1;
			}
		}
		else
		{
			sim = (struct sockaddr_in *)&mask.u_sa;
			sprintf(istr, "\tNet:\t%08X\tMask:\t%08X", 
				sin->sin_addr.s_addr, 
				sim->sin_addr.s_addr);
		} 
		break;
	    }

	default:
	    {
	    	sprintf(istr, " catch if");
		ret = 1;
	    }
	}

	printf("%s%s%s", iface, istr, "\n");
	if (ret)
		return;

// start putting in routing array

	numroutes++; // global
	oldroutes = routes;
	routes = (routerec *) malloc (numroutes * sizeof (routerec));
	if (oldroutes)
	{
		bcopy(oldroutes, routes, ((numroutes -1 )*sizeof(routerec))); 
		free(oldroutes);
	}
	oldroutes = NULL;


	found = 0;
	for (i1 = 0; i1 < numinterfaces; i1++)
	{
		if (strcmp (interfaces[i1].ifname, iface) == 0)
		{
			routes[numroutes-1].iface = &interfaces[i1];
			found = 1;
		}

	}

	if (!found)
	{
		printf ("Couldn't find interface %s\n", iface);
		exit (EXIT_FAILURE);
	}
	
	routes[numroutes-1].addr.s_addr = sin->sin_addr.s_addr;
	routes[numroutes-1].naddr = sim->sin_addr.s_addr;


}


static void p_tree(struct radix_node *rn)
{

again:
	kget(rn, rnode);
	if (rnode.rn_bit < 0) {
		kget(rn, rtentry);
		p_rtentry(&rtentry);
		if ((rn = rnode.rn_dupedkey))
			goto again;
	} else {
		rn = rnode.rn_right;
		p_tree(rnode.rn_left);
		p_tree(rn);
	}
}


/*
 * Print routing tables.
 */
void routepr(u_long rtree)
{
	struct radix_node_head *rnh, head;
	int i;

	kget(rtree, rt_tables);
	for (i = 0; i <= AF_MAX; i++) {
		if ((rnh = rt_tables[i]) == 0)
			continue;
		kget(rnh, head);
		if (af == AF_UNSPEC || af == i) {
			p_tree(head.rnh_treetop);
		}
	}
}

/*
 ***************************************************************
 * End code borrowed from FreeBSD /usr/src/sys/usr.bin/netstat.
 ***************************************************************
 */
#endif



void init_route_tables(void)
{
	int ifsock, i, i1, found;
	struct ifconf ifc;
	struct ifreq *ifr;
	char buf[1024], iface[16],/* ip[16], mask[16],*/ *ptr;
	FILE *f;

#ifdef FREEBSD
	long next, count = 0;
	struct ifreq ifrflags, *last;
	struct in_addr addr, naddr;
	static interfacerec *oldinterfaces = NULL;
	typedef struct {
	char ifname[17];
		unsigned long naddr;
	} naddrrec;
#endif

	/* Create a channel to the NET kernel. */
	if ((ifsock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror ("socket");
		exit (EXIT_FAILURE);
	}

	ifc.ifc_len = sizeof (buf);
	ifc.ifc_buf = buf;
	bzero(ifc.ifc_buf, 1023);
	
	if (ioctl (ifsock, SIOCGIFCONF, &ifc) < 0)
	{
		perror ("opening interface socket");
		close (ifsock);
		exit (EXIT_FAILURE);
	}


#ifdef FREEBSD  

	if (interfaces) /* avoid leakage */
	{
		free(interfaces);
		interfaces = NULL;
		numinterfaces = 0;
	}
	
	if (routes) /* avoid leakage */
	{
		free(routes);
		routes = NULL;
		numroutes = 0;
	}
	
      /*
       * the records might be of different length and may contain more then
       * one entry per interface. We loop over them and try to figure them out
       */
	ifr = (struct ifreq *) ifc.ifc_req;
  	last = (struct ifreq *) ((char *) ifr + ifc.ifc_len);

	while (ifr < last)
    	{

      /*
       * save the current value to advance, the ioctl()'s below will
       * override this
       */
		next = (long)ifr->ifr_addr.sa_len + IFNAMSIZ;
		count++;

		if (!strlen(ifr->ifr_name))
		{
			printf("bogus interface\n");
			goto next;
		}

       /*
       * Skip addresses that begin with "dummy", or that include
       * a ":" (the latter are Solaris virtuals). (could weed out lo* too)
       */
     		if (strncmp (ifr->ifr_name, "dummy", 5) == 0 || strchr (ifr->ifr_name, ':') != NULL)
			goto next;

      /*
       * If we already have this interface name on the list,
       * don't add it (SIOCGIFCONF returns, at least on
       * BSD-flavored systems, one entry per interface *address*;
       * if an interface has multiple addresses, we get multiple
       * entries for it).
       */
		for (i = 0; i < numinterfaces; i++)
			if (interfaces && strncmp (interfaces[i].ifname, ifr->ifr_name, IFNAMSIZ) == 0)
				goto next;
      /*
       * Get the interface flags.
       */
		memset (&ifrflags, 0, sizeof ifrflags);
		strncpy (ifrflags.ifr_name, ifr->ifr_name, sizeof ifrflags.ifr_name);
		if (ioctl (ifsock, SIOCGIFFLAGS, (char *) &ifrflags) < 0)
		{
	  		if (errno == ENXIO)
			    goto next;
		  	printf ("SIOCGIFFLAGS error getting flags for interface %s: %s",
		   		ifr->ifr_name, strerror (errno));
			goto next;
		}

      /*
       * Skip interfaces that aren't up.
       */
      		if (!(ifrflags.ifr_flags & IFF_UP))
			goto next;

      /*
       * Get and save the IF address.
       */

		if (ioctl (ifsock, SIOCGIFADDR, ifr) < 0)
		{ 
			printf ("Couldn't get address for %s\n", ifr->ifr_name);
			goto next;
		}
		memcpy(&addr, &((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr, sizeof (struct in_addr));
				
      /*
       * Get and save the netmask.
       */
/*		if (ioctl (ifsock, SIOCGIFNETMASK, ifr) < 0)
		{ 
			printf ("Couldn't get net address for %s\n", ifr->ifr_name);
			goto next;
		}
		memcpy(&naddr, &((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr, sizeof (struct in_addr));
*/
      /*
       * enlarge our table (FIXME: should be done different)
       */
		numinterfaces++;
		oldinterfaces = interfaces;
		interfaces = (interfacerec *) malloc (numinterfaces * sizeof (interfacerec));
		if (oldinterfaces)
		{
			bcopy(oldinterfaces, interfaces, ((numinterfaces -1 )*sizeof(interfacerec))); 
			free(oldinterfaces);
		}
		oldinterfaces = NULL;

      /*
       * copy everything into our interfaces array
       */
		strncpy (interfaces[numinterfaces-1].ifname, ifr->ifr_name, IFNAMSIZ);
		memcpy  (&interfaces[numinterfaces-1].addr, &addr, sizeof (struct in_addr));
/*		memcpy  (&interfaces[numinterfaces-1].naddr, &naddr, sizeof (struct in_addr));
 */
      /*
       * advance by the value saved above
       */
    next:
	      	ifr = (struct ifreq *) ((char *) ifr + next);
	}

      /*
       * DEBUG : dump the table
       */

#ifdef DEBUG
	for (i = 0; i < numinterfaces; i++)
	{
		strncpy(ip, inet_ntoa(interfaces[i].addr), sizeof(ip));
/*		strncpy(mask, inet_ntoa(interfaces[i].naddr), sizeof(mask));
		printf ("interface %s at %s, netmask : %s\n", 
			 interfaces[i].ifname, ip , mask  );
*/
		printf ("interface %s at %s\n", 
			 interfaces[i].ifname, ip );
	}
#endif /* DEBUG */

#else /* the linux way... */



	numinterfaces = (ifc.ifc_len / sizeof (struct ifreq));
	interfaces = (interfacerec *) malloc (numinterfaces * sizeof (interfacerec));

	ifr = ifc.ifc_req;
	for (i = 0; i < numinterfaces; i++, ifr++)
	{
		strcpy (interfaces[i].ifname, ifr->ifr_name);
		memcpy (&interfaces[i].addr, &((struct sockaddr_in *) &ifr->ifr_addr)->sin_addr, sizeof (struct in_addr));
		if (ioctl (ifsock, SIOCGIFADDR, ifr) < 0)
			printf ("Couldn't get address for %s\n", ifr->ifr_name);
	}
#endif /* FREEBSD */

	close (ifsock);

#ifdef FREEBSD
	return routepr(nl[N_RTREE].n_value);
#else

	if ((f = fopen ("/proc/net/route", "r")) == NULL)
	{
		perror ("opening /proc/net/route");
		exit (EXIT_FAILURE);
	}

	numroutes = 0;
	fgets (buf, sizeof (buf), f);         /* strip out description line */
	while (!feof (f))
	{
		fgets (buf, sizeof (buf), f);
		numroutes++;
	}
	numroutes--;

	routes = (routerec *) malloc (numroutes * sizeof (routerec));

	rewind (f);
	
	fgets (buf, sizeof (buf), f); 
	for (i = 0; i < numroutes; i++)
	{
		if (fgets (buf, sizeof (buf), f) == NULL)
		{
		/* Important, since an interface might have been removed since our counting,
				causing us to parse bogus data */
			fputs ("Error reading /proc/net/route: iface count mismatch\n", stderr);
			fclose (f);
			exit (EXIT_FAILURE);
		}
		if ( strlen (buf) == sizeof(buf)-1 )
		{
		/* skip long lines */
			fputs ("Long (corrupt) line encountered, skipping.\n", stderr);
			while ((fgets (buf, sizeof (buf), f)))
				if (buf [strlen (buf) - 1] == '\n')
					break;
			continue; /* continue with next regular line (or fail if EOF */
		}
		ptr = strtok (buf, "\t ");
		if (!ptr)
			continue;

		if (strlen (ptr) >= sizeof (iface))
			continue; /* would overflow if fed with bogus data in a chroot()ed environment */
		else
			strcpy (iface, ptr);
		ptr = strtok (NULL, "\t ");       /* hack avoiding fscanf */
		routes[i].addr.s_addr=(unsigned long)strtoul(ptr,NULL,16);
		for (i1 = 0; i1 < 6; i1++)
		{
			ptr = strtok (NULL, "\t ");   /* ignore Gateway Flags RefCnt Use Metric */
		}
		if (!ptr) 
		{
			fputs ("Error parsing /proc/net/route\n", stderr);
			continue;
		}
		routes[i].naddr=(unsigned long)strtoul(ptr,NULL,16);   /* Netmask */

		found = 0;
		for (i1 = 0; i1 < numinterfaces; i1++)
		{
			if (strcmp (interfaces[i1].ifname, iface) == 0)
			{
				routes[i].iface = &interfaces[i1];
				found = 1;
			}

		}

		if (!found)
		{
			printf ("Couldn't find interface %s\n", iface);
			exit (EXIT_FAILURE);
		}
	}
	fclose (f);
#endif /* FREEBSD */	
}

