/*
 * Cheops Next Generation GUI
 * 
 * gui-viewspace.h
 * All functions that consern the viewspace
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

#ifndef _GUI_VIEWSPACE_H
#define _GUI_VIEWSPACE_H

#include "cheops-gui.h"

int is_valid_net_page(net_page *np);

void remove_viewspace(GtkWidget *w, gpointer p);

void add_viewspace(GtkWidget *w, char *name);

net_page *make_viewspace(char *c, char *agent_ip, int usessl);

void add_network(GtkWidget *w, char *net, char *mask);

page_object *add_host_entry(int ip, char *name);

page_object *add_host_entry_to_net_page(net_page *np, int ip, char *name);

void set_current_page(GtkWidget *w, net_page *np);

net_page *get_current_net_page(void);

void net_page_remove_all(void);

void add_network_range(char *first, char *last);

int net_page_rename(net_page *np, char *name);


#endif /* _GUI_VIEWSPACE_H */

