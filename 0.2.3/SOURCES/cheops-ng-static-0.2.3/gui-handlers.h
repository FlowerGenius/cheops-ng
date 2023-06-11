/*
 * Cheops Next Generation GUI
 * 
 * gui-handlers.h
 * Header file for the gui's agent handelers
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
#ifndef _CHEOPS_GUI_HANDLERS_H
#define _CHEOPS_GUI_HANDLERS_H

#include "event.h"
struct _page_object;
struct _net_page;
struct _os_stats;

void register_gui_handlers();
int do_discover(agent *a, char *host, void *np, int flags);
int do_os_scan(agent *a, int ip, void *np, int flags);
int do_map_icmp(agent *a, u32 ip, void *np);
void reverse_lookup_cb(char *ip, char *hostname, void *data);
int do_probe(agent *a, u32 ip, u16 port, u32 timeout_ms, void *np);
struct _page_object *add_discovered_node(agent *a, struct _net_page *np, unsigned int ip, char *name, struct _os_stats *os_data, int mapit);
void do_discover_range_after_last_dns(char *ip, char *hostname, void *data);
int do_discover_range(agent *a, char *first, char *last, void *np, int flags);

#endif /* _CHEOPS_GUI_HANDLERS_H */

