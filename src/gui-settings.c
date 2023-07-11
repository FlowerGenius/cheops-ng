/*
 * Cheops Next Generation GUI
 * 
 * gui-settings.c
 * Settings window stuff
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
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef FREEBSD
#include <sys/types.h>
#include <netinet/in_systm.h>
#endif
#include <netinet/in.h>
#include <string.h>
#include "cheops-gui.h"
#include "event.h"
#include "logger.h"
#include "gui-config.h"
#include "gui-settings.h"
#include "gui-canvas.h"
#include "gui-service.h"
#include "gui-pixmap.h"
#include "gui-utils.h"
#include "gui-plugins.h"
#include "script.h"

GtkWidget *services_clist;
int services_row_number = -1;

GtkWidget *scripts_clist;
int scripts_row_number = -1;

GtkWidget *os_pixmaps_clist;
int os_pixmaps_row_number = -1;

GtkWidget *probe_ports_clist;
int probe_ports_row_number = -1;

GtkWidget *plugins_clist;
int plugins_row_number = -1;

static GtkWidget *os_scan_ports;
static GtkWidget *os_scan_ports_label;
static GtkWidget *os_scan_udp_scan;
static GtkWidget *os_scan_tcp_scan;
static GtkWidget *os_scan_timing;
static GtkWidget *os_scan_dont_ping;
static GtkWidget *os_scan_rpc_scan;
static GtkWidget *os_scan_identd_scan;
static GtkWidget *os_scan_fastscan;
static GtkWidget *os_scan_osscan;         // this is the os scan? y/n
static GtkWidget *os_scan_scan_specified_ports;
static GtkWidget *rescan_at_startup;
static GtkWidget *discover_retries;
static GtkWidget *operating_system_check; // really this is portscan? y/n
static GtkWidget *reverse_dns;
static GtkWidget *map;
static GtkWidget *confirm_delete;
static GtkWidget *save_changes_on_exit;
static GtkWidget *use_ip_for_label;
static GtkWidget *only_display_hostname;
static GtkWidget *move_stuff_live;
static GtkWidget *probe_ports;
static GtkWidget *use_ip_for_merged_ports;

int options_rescan_at_startup = 0;
int options_use_ip_for_label = 0;
int options_discover_retries = 2;
int options_operating_system_check = 1;
int options_reverse_dns = 1;
int options_map = 1;
int options_confirm_delete = 1;
int options_save_changes_on_exit = 1;
int tooltips_timeout = 1;
int options_move_stuff_live = 1;
int options_probe_ports = 1;
int options_use_ip_for_merged_ports = 0;
int options_only_display_hostname = 0;

unsigned int options_os_scan_flags = 0;
char options_os_scan_ports[OPTIONS_OS_SCAN_PORTS_SIZE];
char options_os_scan_tcp_scan[OPTIONS_OS_SCAN_TCP_SCAN_SIZE] = TCP_SCAN_DEFAULT;
char options_os_scan_timing[OPTIONS_OS_SCAN_TIMING_SIZE] = TIMING_SCAN_DEFAULT;
int options_os_scan_udp_scan = 0;
int options_os_scan_fastscan = 1;
int options_os_scan_osscan = 1;
int options_os_scan_rpc_scan = 1;
int options_os_scan_identd_scan = 1;
int options_os_scan_dont_ping = 1;
int options_os_scan_scan_specified_ports = 0;

char FLAG_CHARACTER_DIRECTIONS[] = 
"These are the flags that you may use to make the service's command\n"
"unique to the specific host that you are monitoring\n\n"
" %i   - this is the IP address of the object\n"
" %p   - this is the specific port of this service\n"
" %u   - this will activate a user prompt to enter a username\n"
" %P   - this will activate a user prompt to enter a password";

char SCRIPT_FLAG_CHARACTER_DIRECTIONS[] = 
"These are the flags that you may use to make the script's command\n"
"unique to the specific host that you are monitoring\n\n"
" %i   - this is the IP address of the object\n"
" %u   - this will activate a user prompt to enter a username\n"
" %P   - this will activate a user prompt to enter a password";



void apply_the_setting(int flags);



// a stupid thing that i should not have to define, if we have any problems with the
// row height with the os_pixmaps here is the problem
#define ROW_ELEMENT(clist, row) (((row) == (clist)->rows - 1) ? \
                                  (clist)->row_list_end : \
                                  g_list_nth ((clist)->row_list, (row)))
 



#define SET_CHECK_OPTION(widget, value) {                                       \
	if(value)                                                               \
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),1);      \
	else                                                                    \
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),0);      \
}

#define APPLY_CHECK_OPTION(widget, value, flag) {				\
	value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));	\
	apply_the_setting(flag);						\
}


void apply_the_setting(int flags)
{
	net_page *np;
	switch(flags)
	{
		case OPTIONS_DISCOVER_RETRIES:
			eh->len = htonl(sizeof(event_hdr) + sizeof(set_settings_e));
			eh->hlen = htons(sizeof(event_hdr));
			eh->type = htons(EVENT_SET_SETTINGS);
			eh->flags = 0;
			ee->set_settings_e.discover_retries = options_discover_retries;
			ee->set_settings_e.flags = flags;
			for(np = main_window->net_pages; np; np = np->next)
				if (event_send(np->agent, eh) < 0) 
					c_log(LOG_WARNING, "Unable to send set settings event\n");
			break;
			
		case OPTIONS_USE_IP_FOR_LABEL:
			{
				net_page *np;
				page_object *po;
				
				for(np = main_window->net_pages; np; np = np->next)
					for(po = np->page_objects; po; po = po->next)
						page_object_display(po);	
			}
			break;
			
		case OPTIONS_OPERATING_SYSTEM_CHECK:
			break;
		case OPTIONS_REVERSE_DNS:
			break;
		case OPTIONS_MAP:
			break;
		case OPTIONS_TOOLTIPS_TIMEOUT:
			break;
		case OPTIONS_CONFIRM_DELETE:
			break;
		case OPTIONS_SAVE_CHANGES_ON_EXIT:
			break;
		case OPTIONS_MOVE_STUFF_LIVE:
			break;
		case OPTIONS_PROBE_PORTS:
			break;
		case OPTIONS_RESCAN_AT_STARTUP:
			break;
		case OPTIONS_OS_SCAN_PORTS:
			break;
		case OPTIONS_USE_IP_FOR_MERGED_PORTS:
			break;
		case OPTIONS_ONLY_DISPLAY_HOSTNAME:
			break;
		case OPTIONS_OS_SCAN_UDP_SCAN:
			if(options_os_scan_udp_scan)
				options_os_scan_flags |= OS_SCAN_OPTION_UDP_SCAN;
			else
				options_os_scan_flags &= ~OS_SCAN_OPTION_UDP_SCAN;
			break;
		case OPTIONS_OS_SCAN_DONT_PING:
			if(options_os_scan_dont_ping)
				options_os_scan_flags |= OS_SCAN_OPTION_DONT_PING;
			else
				options_os_scan_flags &= ~OS_SCAN_OPTION_DONT_PING;
			break;
		case OPTIONS_OS_SCAN_TCP_SCAN:
			options_os_scan_flags &= ~(OS_SCAN_OPTION_SCAN_MASK);
			if(0 == strcmp(options_os_scan_tcp_scan, TCP_SCAN_0))
				options_os_scan_flags |= OS_SCAN_OPTION_TCP_CONNECT_SCAN;
			else if(0 == strcmp(options_os_scan_tcp_scan, TCP_SCAN_1))
				options_os_scan_flags |= OS_SCAN_OPTION_TCP_SYN_SCAN;
			else if(0 == strcmp(options_os_scan_tcp_scan, TCP_SCAN_2))
				options_os_scan_flags |= OS_SCAN_OPTION_STEALTH_FIN;
			else if(0 == strcmp(options_os_scan_tcp_scan, TCP_SCAN_3))
				options_os_scan_flags |= OS_SCAN_OPTION_STEALTH_XMAS;
			else if(0 == strcmp(options_os_scan_tcp_scan, TCP_SCAN_4))
				options_os_scan_flags |= OS_SCAN_OPTION_STEALTH_NULL;
			else
			{
				printf("%s(): you should not see this message!\n", __FUNCTION__);
				options_os_scan_flags |= OS_SCAN_OPTION_TCP_CONNECT_SCAN;
			}
			
			break;
		case OPTIONS_OS_SCAN_TIMING:
			options_os_scan_flags &= ~(OS_SCAN_OPTION_TIMIMG_MASK);
			if(0 == strcmp(options_os_scan_timing,      TIMING_SCAN_NORMAL))
				options_os_scan_flags |= OS_SCAN_OPTION_TIMIMG_NORMAL;
			else if(0 == strcmp(options_os_scan_timing, TIMING_SCAN_PARANOID))
				options_os_scan_flags |= OS_SCAN_OPTION_TIMIMG_PARANOID;
			else if(0 == strcmp(options_os_scan_timing, TIMING_SCAN_SNEAKY))
				options_os_scan_flags |= OS_SCAN_OPTION_TIMIMG_SNEAKY;
			else if(0 == strcmp(options_os_scan_timing, TIMING_SCAN_POLITE))
				options_os_scan_flags |= OS_SCAN_OPTION_TIMIMG_POLITE;
			else if(0 == strcmp(options_os_scan_timing, TIMING_SCAN_AGGRESSIVE))
				options_os_scan_flags |= OS_SCAN_OPTION_TIMIMG_AGGRESSIVE;
			else if(0 == strcmp(options_os_scan_timing, TIMING_SCAN_INSANE))
				options_os_scan_flags |= OS_SCAN_OPTION_TIMIMG_INSANE;
			else
			{
				printf("%s(TIMING): you should not see this message!\n", __FUNCTION__);
				options_os_scan_flags |= OS_SCAN_OPTION_TIMIMG_NORMAL;
			}
			
			break;
		case OPTIONS_OS_SCAN_SCAN_SPECIFIED_PORTS:
			if(options_os_scan_scan_specified_ports)
				options_os_scan_flags |= OS_SCAN_OPTION_USE_PORT_RANGE;
			else
				options_os_scan_flags &= ~OS_SCAN_OPTION_USE_PORT_RANGE;
			break;
		case OPTIONS_OS_SCAN_FASTSCAN:
			if(options_os_scan_fastscan)
				options_os_scan_flags |= OS_SCAN_OPTION_FASTSCAN;
			else
				options_os_scan_flags &= ~OS_SCAN_OPTION_FASTSCAN;
			break;
		case OPTIONS_OS_SCAN_OSSCAN:
			if(options_os_scan_osscan)
				options_os_scan_flags |= OS_SCAN_OPTION_OSSCAN;
			else
				options_os_scan_flags &= ~OS_SCAN_OPTION_OSSCAN;
			break;
		case OPTIONS_OS_SCAN_RPC_SCAN:
			if(options_os_scan_osscan)
				options_os_scan_flags |= OS_SCAN_OPTION_RPC_SCAN;
			else
				options_os_scan_flags &= ~OS_SCAN_OPTION_RPC_SCAN;
			break;
		case OPTIONS_OS_SCAN_IDENTD_SCAN:
			if(options_os_scan_osscan)
				options_os_scan_flags |= OS_SCAN_OPTION_IDENTD_SCAN;
			else
				options_os_scan_flags &= ~OS_SCAN_OPTION_IDENTD_SCAN;
			break;
		default:
			break;		
	}
}

void apply_all_settings(void)
{
	int i;
	
	for(i = 0; i < OPTIONS_LAST_OPTION; i++)
		apply_the_setting(i);
}

void apply_settings(GtkWidget *w, gpointer data)
{
	int retries;
	char *temp;
	
	APPLY_CHECK_OPTION(only_display_hostname, options_only_display_hostname, OPTIONS_ONLY_DISPLAY_HOSTNAME);
	APPLY_CHECK_OPTION(os_scan_identd_scan, options_os_scan_identd_scan, OPTIONS_OS_SCAN_IDENTD_SCAN);
	APPLY_CHECK_OPTION(os_scan_rpc_scan, options_os_scan_rpc_scan, OPTIONS_OS_SCAN_RPC_SCAN);
	APPLY_CHECK_OPTION(os_scan_osscan, options_os_scan_osscan, OPTIONS_OS_SCAN_OSSCAN);
	APPLY_CHECK_OPTION(os_scan_fastscan, options_os_scan_fastscan, OPTIONS_OS_SCAN_FASTSCAN);
	APPLY_CHECK_OPTION(os_scan_dont_ping, options_os_scan_dont_ping, OPTIONS_OS_SCAN_DONT_PING);
	APPLY_CHECK_OPTION(os_scan_udp_scan, options_os_scan_udp_scan, OPTIONS_OS_SCAN_UDP_SCAN);
	APPLY_CHECK_OPTION(os_scan_scan_specified_ports, options_os_scan_scan_specified_ports, OPTIONS_OS_SCAN_SCAN_SPECIFIED_PORTS);
	APPLY_CHECK_OPTION(rescan_at_startup, options_rescan_at_startup, OPTIONS_RESCAN_AT_STARTUP);
	APPLY_CHECK_OPTION(operating_system_check, options_operating_system_check, OPTIONS_OPERATING_SYSTEM_CHECK);
	APPLY_CHECK_OPTION(reverse_dns, options_reverse_dns, OPTIONS_REVERSE_DNS);
	APPLY_CHECK_OPTION(map, options_map, OPTIONS_MAP);
	APPLY_CHECK_OPTION(confirm_delete, options_confirm_delete, OPTIONS_CONFIRM_DELETE);
	APPLY_CHECK_OPTION(save_changes_on_exit, options_save_changes_on_exit, OPTIONS_SAVE_CHANGES_ON_EXIT);
	APPLY_CHECK_OPTION(use_ip_for_label, options_use_ip_for_label, OPTIONS_USE_IP_FOR_LABEL);
	APPLY_CHECK_OPTION(move_stuff_live, options_move_stuff_live, OPTIONS_MOVE_STUFF_LIVE);
	APPLY_CHECK_OPTION(probe_ports, options_probe_ports, OPTIONS_PROBE_PORTS);
	APPLY_CHECK_OPTION(use_ip_for_merged_ports, options_use_ip_for_merged_ports, OPTIONS_USE_IP_FOR_MERGED_PORTS);
	
	retries = atoi(gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(discover_retries)->entry)));
	if(retries > MAX_ALLOWABLE_DISCOVER_RETRIES || retries < 0)
	{
		printf("you entered a dumb entry \n");
		return;
	}
	options_discover_retries = retries;
	

	temp = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(os_scan_ports))));
	strncpy(options_os_scan_ports, temp, sizeof(options_os_scan_ports));

	temp = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(os_scan_tcp_scan)->entry));
	if(0 == strcmp(temp, TCP_SCAN_0))
		;
	else if(0 == strcmp(temp, TCP_SCAN_1))
		;
	else if(0 == strcmp(temp, TCP_SCAN_2))
		;
	else if(0 == strcmp(temp, TCP_SCAN_3))
		;
	else if(0 == strcmp(temp, TCP_SCAN_4))
		;
	else
	{
		printf("%s(): you should not see this message!\n", __FUNCTION__);
		temp = TCP_SCAN_DEFAULT;
	}

	strncpy(options_os_scan_tcp_scan, temp, sizeof(options_os_scan_tcp_scan));	

	temp = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(os_scan_timing)->entry));
	if(0 == strcmp(temp, TIMING_SCAN_NORMAL))
		;
	else if(0 == strcmp(temp, TIMING_SCAN_PARANOID))
		;
	else if(0 == strcmp(temp, TIMING_SCAN_SNEAKY))
		;
	else if(0 == strcmp(temp, TIMING_SCAN_POLITE))
		;
	else if(0 == strcmp(temp, TIMING_SCAN_AGGRESSIVE))
		;
	else if(0 == strcmp(temp, TIMING_SCAN_INSANE))
		;
	else
	{
		printf("%s(): you should not see this message!\n", __FUNCTION__);
		temp = TIMING_SCAN_DEFAULT;
	}

	strncpy(options_os_scan_timing, temp, sizeof(options_os_scan_timing));	

	apply_all_settings();
}

/*************************************************************************
 *             services
 *************************************************************************/
void services_select_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	services_row_number = row;
}
void services_unselect_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	services_row_number = -1;
}

service_list_t *services_make_dialog(service_list_t *service, char *name, int port, int protocol, char *str, GtkWidget *parent)
{
	GtkWidget *dialog;
	GtkWidget *name_w;
	GtkWidget *string_w;
	GtkWidget *port_w;
	GtkWidget *protocol_w;
	GtkWidget *table;
	GtkWidget *label;
	char *ptrs[4];
	char buf[256];
	service_list_t *srv = NULL;
	GList *comboList = NULL;
	
	dialog = gnome_dialog_new("Add a Service Entry",
				  GNOME_STOCK_BUTTON_OK,
				  GNOME_STOCK_BUTTON_CANCEL,NULL);

	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(parent));

	table = gtk_table_new( FALSE, 2, 4);		
	
	name_w = gnome_entry_new("Service Name");
	string_w = gnome_entry_new("Command to execute");
	port_w = gnome_entry_new("Service Port");

	protocol_w = gtk_combo_new();
	comboList = g_list_append(comboList, "TCP");
	comboList = g_list_append(comboList, "UDP");
	gtk_combo_set_popdown_strings(GTK_COMBO(protocol_w),comboList);
	g_list_free(comboList);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(protocol_w)->entry), FALSE);

	label = gtk_label_new("Service Name");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	label = gtk_label_new("Port Number");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	label = gtk_label_new("Protocol");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	label = gtk_label_new("Comand to Execute");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);

	gtk_table_attach_defaults(GTK_TABLE(table), name_w,     1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), port_w,     1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), protocol_w, 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), string_w,   1, 2, 3, 4);

	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), table, FALSE, FALSE, 5);

	label = gtk_label_new(FLAG_CHARACTER_DIRECTIONS);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	if(name == NULL)
		name = "";
	if(str == NULL)
		str = "";
	if(protocol != 17 && protocol != 6)
		protocol = 6;
	if(port >= 0)
		sprintf(buf, "%d", port); 
	else
		buf[0] = '\0';

			
	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(name_w))), name );
	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(port_w))), buf );
	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(string_w))), str );
	gtk_combo_set_value_in_list(GTK_COMBO(protocol_w), (protocol==6 ?0:1), FALSE);

	gtk_widget_grab_focus(GTK_COMBO(name_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(name_w)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), GTK_COMBO(port_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(port_w)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), GTK_COMBO(protocol_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(protocol_w)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), GTK_COMBO(string_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(string_w)->entry), "activate", click_ok_on_gnome_dialog, dialog);

	gtk_widget_show( name_w);
	gtk_widget_show( port_w);
	gtk_widget_show( protocol_w);
	gtk_widget_show( string_w);
	gtk_widget_show( table);
	
	switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
	{
		case -1:
			break;
		case 0:
			ptrs[0] = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(name_w))));
			ptrs[1] = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(port_w))));
			ptrs[2] = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(protocol_w)->entry));
			protocol = (strcmp(ptrs[2],"TCP")==0 ? 6: 17);
			ptrs[3] = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(string_w))));

			if( (ptrs[0] && strlen(ptrs[0])) && (ptrs[1] && strlen(ptrs[1])) && (ptrs[3] && strlen(ptrs[3])))
			{
				if(service)
					srv = service_list_change( service, ptrs[0], atoi(ptrs[1]), protocol, ptrs[3]);
				else
					srv = service_list_add( ptrs[0], atoi(ptrs[1]), protocol, ptrs[3]);
			}
			gnome_dialog_close(GNOME_DIALOG(dialog));
			service = srv;
			break;
			
		case 1:
			gnome_dialog_close(GNOME_DIALOG(dialog));
			break;
	}
	return(service);
}
void services_do_add(GtkWidget *w, GtkWidget *parent)
{
	service_list_t *srv;
	char *ptrs[4];
	char buf[256];
	
	srv = services_make_dialog(NULL, NULL,-1, 6, NULL, parent);
	if(srv)
	{
		sprintf(buf, "%d", srv->port);
		ptrs[0] = srv->name;
		ptrs[1] = buf;
		ptrs[2] = (srv->protocol==6 ? "TCP":"UDP");
		ptrs[3] = srv->command;
		
		gtk_clist_freeze(GTK_CLIST(services_clist));
		
		gtk_clist_prepend(GTK_CLIST(services_clist), ptrs);
		gtk_clist_set_row_data(GTK_CLIST(services_clist), 0, srv);
		
		gtk_clist_thaw(GTK_CLIST(services_clist));
	}
}

void services_do_edit(GtkWidget *w, GtkWidget *parent)
{
	int num = services_row_number;
	char buf[256];
	service_list_t *service;
	char *ptrs[4];

	if(services_row_number < 0)
		return;

	service = gtk_clist_get_row_data(GTK_CLIST(services_clist), services_row_number);
	
	service = services_make_dialog(service, service->name, service->port, service->protocol, service->command, parent);
	
	gtk_clist_freeze(GTK_CLIST(services_clist));
	gtk_clist_remove(GTK_CLIST(services_clist), services_row_number);
	if(service)
	{
		sprintf(buf, "%d", service->port);
		ptrs[0] = service->name;
		ptrs[1] = buf;
		ptrs[2] = (service->protocol==6 ? "TCP":"UDP");
		ptrs[3] = service->command;
		
		gtk_clist_insert(GTK_CLIST(services_clist), num, ptrs);
		gtk_clist_set_row_data(GTK_CLIST(services_clist), num, service);
	}		
	gtk_clist_thaw(GTK_CLIST(services_clist));
}

void services_do_delete(GtkWidget *w, gpointer p)
{
	service_list_t *service;
	
	if(services_row_number < 0)
		return;
		
	service = gtk_clist_get_row_data(GTK_CLIST(services_clist), services_row_number);
	service_list_remove_ptr(service);

	gtk_clist_freeze(GTK_CLIST(services_clist));
	gtk_clist_remove(GTK_CLIST(services_clist), services_row_number);
	gtk_clist_thaw(GTK_CLIST(services_clist));

	services_row_number = -1;
}

/*************************************************************************
 *             scripts
 *************************************************************************/
void scripts_select_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	scripts_row_number = row;
}
void scripts_unselect_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	scripts_row_number = -1;
}

script_t *scripts_make_dialog(script_t *script, char *name, char *str, unsigned int flags, GtkWidget *parent)
{
	GtkWidget *dialog;
	GtkWidget *name_w;
	GtkWidget *string_w;
	GtkWidget *shared_w;
	GtkWidget *table;
	GtkWidget *label;
	char *ptrs[3];
	script_t *srv = NULL;
	GList *comboList = NULL;
		
	dialog = gnome_dialog_new("Add a Service Entry",
				  GNOME_STOCK_BUTTON_OK,
				  GNOME_STOCK_BUTTON_CANCEL,NULL);

	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(parent));

	table = gtk_table_new( FALSE, 2, 4);		
	
	shared_w = gtk_combo_new();
	comboList = g_list_append(comboList, "yes");
	comboList = g_list_append(comboList, "no");
	gtk_combo_set_popdown_strings(GTK_COMBO(shared_w),comboList);
	g_list_free(comboList);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(shared_w)->entry), FALSE);

	name_w = gnome_entry_new("Script Name");
	string_w = gnome_entry_new("Command to execute");

	label = gtk_label_new("Service Name");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	label = gtk_label_new("Viewable from Host right click menu");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	label = gtk_label_new("Comand to Execute");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);

	gtk_table_attach_defaults(GTK_TABLE(table), name_w,     1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), shared_w,   1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), string_w,   1, 2, 2, 3);

	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), table, FALSE, FALSE, 5);

	label = gtk_label_new(SCRIPT_FLAG_CHARACTER_DIRECTIONS);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	if(name == NULL)
		name = "";
	if(str == NULL)
		str = "";
			
	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(name_w))), name );
	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(string_w))), str );
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(shared_w)->entry), (flags & SCRIPT_FLAGS_VIEWABLE) ? "yes":"no" );

	gtk_widget_grab_focus(GTK_COMBO(name_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(name_w)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), GTK_COMBO(shared_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(name_w)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), string_w);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(string_w)->entry), "activate", click_ok_on_gnome_dialog, dialog);

	gtk_widget_show(name_w);
	gtk_widget_show(string_w);
	gtk_widget_show(shared_w);
	gtk_widget_show(table);
	
	switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
	{
		case -1:
			break;
		case 0:
			ptrs[0] = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(name_w))));
			ptrs[1] = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(string_w))));
			ptrs[2] = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(shared_w)->entry));
			flags = (strcmp(ptrs[2],"yes")==0 ? SCRIPT_FLAGS_VIEWABLE : 0);

			if( (ptrs[0] && strlen(ptrs[0])) && (ptrs[1] && strlen(ptrs[1])) && (ptrs[2] && strlen(ptrs[2])))
			{
				if(script)
				{
					srv = script_change(script, ptrs[0], ptrs[1], flags);
				}
				else
				{
					srv = script_add(ptrs[0], ptrs[1], flags);
				}
			}
			gnome_dialog_close(GNOME_DIALOG(dialog));
			script = srv;
			break;
			
		case 1:
			gnome_dialog_close(GNOME_DIALOG(dialog));
			break;
	}
	return(script);
}

void scripts_do_add(GtkWidget *w, GtkWidget *parent)
{
	script_t *srv;
	char *ptrs[3];
	
	srv = scripts_make_dialog(NULL, NULL, NULL, SCRIPT_FLAGS_VIEWABLE, parent);
	
	if(srv)
	{
		ptrs[0] = srv->name;
		ptrs[1] = srv->script;
		ptrs[2] = ((srv->flags&SCRIPT_FLAGS_VIEWABLE) ? "yes":"no");
		
		gtk_clist_freeze(GTK_CLIST(scripts_clist));
		
		gtk_clist_prepend(GTK_CLIST(scripts_clist), ptrs);
		gtk_clist_set_row_data(GTK_CLIST(scripts_clist), 0, srv);
		
		gtk_clist_thaw(GTK_CLIST(scripts_clist));
	}
}

void scripts_do_edit(GtkWidget *w, GtkWidget *parent)
{
	int num = scripts_row_number;
	script_t *script;
	char *ptrs[3];

	if(scripts_row_number < 0)
		return;

	script = gtk_clist_get_row_data(GTK_CLIST(scripts_clist), scripts_row_number);
	
	script = scripts_make_dialog(script, script->name, script->script, script->flags, parent);
	
	gtk_clist_freeze(GTK_CLIST(scripts_clist));
	gtk_clist_remove(GTK_CLIST(scripts_clist), scripts_row_number);
	if(script)
	{
		ptrs[0] = script->name;
		ptrs[1] = script->script;
		ptrs[2] = ((script->flags & SCRIPT_FLAGS_VIEWABLE) ? "yes":"no");
		
		gtk_clist_insert(GTK_CLIST(scripts_clist), num, ptrs);
		gtk_clist_set_row_data(GTK_CLIST(scripts_clist), num, script);
	}		
	gtk_clist_thaw(GTK_CLIST(scripts_clist));
}

void scripts_do_delete(GtkWidget *w, gpointer p)
{
	script_t *script;
	
	if(scripts_row_number < 0)
		return;
		
	script = gtk_clist_get_row_data(GTK_CLIST(scripts_clist), scripts_row_number);
	script_remove_ptr(script);

	gtk_clist_freeze(GTK_CLIST(scripts_clist));
	gtk_clist_remove(GTK_CLIST(scripts_clist), scripts_row_number);
	gtk_clist_thaw(GTK_CLIST(scripts_clist));

	scripts_row_number = -1;
}

/*************************************************************************
 *             probe ports
 *************************************************************************/
void probe_ports_select_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	probe_ports_row_number = row;
}
void probe_ports_unselect_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	probe_ports_row_number = -1;
}
#ifdef USING_PROBE_PORTS

service_list_t *probe_ports_make_dialog(service_list_t *service, char *name, int port, int protocol, char *str, GtkWidget *parent)
{
	GtkWidget *dialog;
	GtkWidget *name_w;
	GtkWidget *string_w;
	GtkWidget *port_w;
	GtkWidget *protocol_w;
	GtkWidget *table;
	GtkWidget *label;
	char *ptrs[4];
	char buf[256];
	service_list_t *srv = NULL;
	GList *comboList = NULL;
	
	dialog = gnome_dialog_new("Add a Service Entry",
				  GNOME_STOCK_BUTTON_OK,
				  GNOME_STOCK_BUTTON_CANCEL,NULL);

	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(parent));
	table = gtk_table_new( FALSE, 2, 4);		
	
	name_w = gnome_entry_new("Service Name");
	string_w = gnome_entry_new("Command to execute");
	port_w = gnome_entry_new("Service Port");

	protocol_w = gtk_combo_new();
	comboList = g_list_append(comboList, "TCP");
	comboList = g_list_append(comboList, "UDP");
	gtk_combo_set_popdown_strings(GTK_COMBO(protocol_w),comboList);
	g_list_free(comboList);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(protocol_w)->entry), FALSE);

	
	label = gtk_label_new("Service Name");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	label = gtk_label_new("Port Number");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	label = gtk_label_new("Protocol");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	label = gtk_label_new("Comand to Execute");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 3, 4);

	gtk_table_attach_defaults(GTK_TABLE(table), name_w,     1, 2, 0, 1);
	gtk_table_attach_defaults(GTK_TABLE(table), port_w,     1, 2, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), protocol_w, 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(table), string_w,   1, 2, 3, 4);

	gtk_widget_grab_focus(GTK_COMBO(name_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(name_w)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), GTK_COMBO(port_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(port_w)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), GTK_COMBO(protocol_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(protocol_w)->entry),  "activate", GTK_SIGNAL_FUNC(gtk_widget_focus_me), GTK_COMBO(string_w)->entry);
	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(string_w)->entry), "activate", click_ok_on_gnome_dialog, dialog);

	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), table, FALSE, FALSE, 5);

	label = gtk_label_new(FLAG_CHARACTER_DIRECTIONS);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), label, FALSE, FALSE, 5);
	gtk_widget_show(label);

	if(name == NULL)
		name = "";
	if(str == NULL)
		str = "";
	if(protocol != 17 && protocol != 6)
		protocol = 6;
	if(port >= 0)
		sprintf(buf, "%d", port); 
	else
		buf[0] = '\0';

			
	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(name_w))), name );
	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(port_w))), buf );
	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(string_w))), str );
	gtk_combo_set_value_in_list(GTK_COMBO(protocol_w), (protocol==6 ?0:1), FALSE);

	gtk_widget_show( name_w);
	gtk_widget_show( port_w);
	gtk_widget_show( protocol_w);
	gtk_widget_show( string_w);
	gtk_widget_show( table);
	
	switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
	{
		case -1:
			break;
		case 0:
			ptrs[0] = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(name_w))));
			ptrs[1] = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(port_w))));
			ptrs[2] = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(protocol_w)->entry));
			protocol = (strcmp(ptrs[2],"TCP")==0 ? 6: 17);
			ptrs[3] = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(string_w))));

			if( (ptrs[0] && strlen(ptrs[0])) && (ptrs[1] && strlen(ptrs[1])) && (ptrs[3] && strlen(ptrs[3])))
			{
				if(service)
					srv = service_list_change( service, ptrs[0], atoi(ptrs[1]), protocol, ptrs[3]);
				else
					srv = service_list_add( ptrs[0], atoi(ptrs[1]), protocol, ptrs[3]);
			}
			gnome_dialog_close(GNOME_DIALOG(dialog));
			service = srv;
			break;
			
		case 1:
			gnome_dialog_close(GNOME_DIALOG(dialog));
			break;
	}
	return(service);

}

void probe_ports_do_add(GtkWidget *w, GtkWidget *parent)
{
	char *ptrs[4];
	char buf[256];
	service_list_t *srv = probe_ports_make_dialog(NULL, NULL,-1, 6, NULL, parent);
	if(srv)
	{
		sprintf(buf, "%d", srv->port);
		ptrs[0] = srv->name;
		ptrs[1] = buf;
		ptrs[2] = (srv->protocol==6 ? "TCP":"UDP");
		ptrs[3] = srv->command;
		
		gtk_clist_freeze(GTK_CLIST(probe_ports_clist));
		
		gtk_clist_prepend(GTK_CLIST(probe_ports_clist), ptrs);
		gtk_clist_set_row_data(GTK_CLIST(probe_ports_clist), 0, srv);
		
		gtk_clist_thaw(GTK_CLIST(probe_ports_clist));
	}
}

void probe_ports_do_configure(GtkWidget *w, GtkWidget *parent)
{
	int num = probe_ports_row_number;
	char buf[256];
	service_list_t *service;
	char *ptrs[4];

	if(probe_ports_row_number < 0)
		return;

	service = gtk_clist_get_row_data(GTK_CLIST(probe_ports_clist), probe_ports_row_number);
	
	service = probe_ports_make_dialog(service, service->name, service->port, service->protocol, service->command, parent);
	
	gtk_clist_freeze(GTK_CLIST(probe_ports_clist));
	gtk_clist_remove(GTK_CLIST(probe_ports_clist), probe_ports_row_number);
	if(service)
	{
		sprintf(buf, "%d", service->port);
		ptrs[0] = service->name;
		ptrs[1] = buf;
		ptrs[2] = (service->protocol==6 ? "TCP":"UDP");
		ptrs[3] = service->command;
		
		gtk_clist_insert(GTK_CLIST(probe_ports_clist), num, ptrs);
		gtk_clist_set_row_data(GTK_CLIST(probe_ports_clist), num, service);
	}		
	gtk_clist_thaw(GTK_CLIST(probe_ports_clist));
}

void probe_ports_do_delete(GtkWidget *w, gpointer p)
{
	service_list_t *service;
	
	if(probe_ports_row_number < 0)
		return;
		
	service = gtk_clist_get_row_data(GTK_CLIST(probe_ports_clist), probe_ports_row_number);
	service_list_remove_ptr(service);

	gtk_clist_freeze(GTK_CLIST(probe_ports_clist));
	gtk_clist_remove(GTK_CLIST(probe_ports_clist), probe_ports_row_number);
	gtk_clist_thaw(GTK_CLIST(probe_ports_clist));

	probe_ports_row_number = -1;
}
#endif /* USING_PROBE_PORTS */

/*************************************************************************
 *             os pixmap functions  
 *************************************************************************/
void os_pixmaps_select_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	os_pixmaps_row_number = row;
}
void os_pixmaps_unselect_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	os_pixmaps_row_number = -1;
}

os_pixmap_list_t *os_pixmaps_make_dialog(os_pixmap_list_t *osp, char *pix, char *str, GtkWidget *parent)
{
	GtkWidget *dialog;
	GtkWidget *string;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *icon;
	char *ptrs[2];
	os_pixmap_list_t *os_pixmap;
	char buf[256];
	char buf2[256];
	
	dialog = gnome_dialog_new("Add an OS Pixmap Entry",
				  GNOME_STOCK_BUTTON_OK,
				  GNOME_STOCK_BUTTON_CANCEL,NULL);

	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(parent));

	table = gtk_table_new( FALSE, 2, 2);		
	
	string = gnome_entry_new("OS String Match");
	
	label = gtk_label_new("Pixmap:");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	label = gtk_label_new("OS String Name (partial os ok):");
	gtk_widget_show(label);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	gtk_table_attach_defaults(GTK_TABLE(table), string, 1, 2, 1, 2);

	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), table, FALSE, FALSE, 5);

	sprintf(buf, "Pick the icon for '%s'", (str == NULL ? "Unknown" : str));
	icon = gnome_icon_entry_new("pixmap entry", buf);

	if(pix != NULL)
		strcpy(buf, get_my_image_path(pix));	
	else
		strcpy(buf, get_my_image_path("unknown.xpm"));	
	
	if(buf[0] == '.' && buf[1] == '/')
	{
	     	buf[0] = '\0';
	     	getcwd(buf2, sizeof(buf));
	     	strcat(buf2, "/");
		strcat(buf2, buf + 2);
	
		gnome_icon_entry_set_icon(GNOME_ICON_ENTRY(icon), buf2);
	}
	else
	{
		gnome_icon_entry_set_icon(GNOME_ICON_ENTRY(icon), buf);
	}		
	
	gtk_table_attach_defaults(GTK_TABLE(table), icon, 1, 2, 0, 1);
	
	if(str == NULL)
		str = "";
	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(string))), str );

	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(string)->entry), "activate", click_ok_on_gnome_dialog, dialog);

	gtk_widget_show( string);
	gtk_widget_show( table);
	gtk_widget_show(icon);
	

	gtk_widget_show(icon);	

	switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
	{
		case -1:
			break;
		case 0:
			ptrs[0] = gnome_icon_entry_get_filename(GNOME_ICON_ENTRY(icon));
			ptrs[1] = gtk_entry_get_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(string))));

			if( (ptrs[0] && strlen(ptrs[0])) && (ptrs[1] && strlen(ptrs[1])))
			{
				if(osp)
					os_pixmap = os_pixmap_list_change( osp, ptrs[1], ptrs[0]);
				else
					os_pixmap = os_pixmap_list_add( ptrs[1], ptrs[0]);
			}
			else
				os_pixmap = NULL;
			
			gnome_dialog_close(GNOME_DIALOG(dialog));
			
			osp = os_pixmap;
			break;

		case 1:
			gnome_dialog_close(GNOME_DIALOG(dialog));
			break;
	}
	return(osp);
}

void os_pixmaps_do_add(GtkWidget *w, GtkWidget *parent)
{
	os_pixmap_list_t *os_pixmap;
	char *ptrs[2];
	
	os_pixmap = os_pixmaps_make_dialog(NULL, NULL, NULL, parent);
	if(os_pixmap)
	{
		ptrs[0] = os_pixmap->pixmap_name;
		ptrs[1] = os_pixmap->string;
		
		gtk_clist_freeze(GTK_CLIST(os_pixmaps_clist));

		gtk_clist_prepend(GTK_CLIST(os_pixmaps_clist), ptrs);
 		gtk_clist_set_row_data(GTK_CLIST(os_pixmaps_clist), 0, os_pixmap);
 		{
 			GdkBitmap *mask;
 			GdkPixmap *pixmap_data;
 			
 			get_my_pixmap_and_mask(os_pixmap->pixmap_name, &pixmap_data, &mask);
 	
 			if(pixmap_data && mask)
 				gtk_clist_set_pixmap(GTK_CLIST(os_pixmaps_clist),0, 0, pixmap_data, mask); 
 		}			
 		gtk_clist_columns_autosize(GTK_CLIST(os_pixmaps_clist));
		gtk_clist_thaw(GTK_CLIST(os_pixmaps_clist));

	}
}

void os_pixmaps_do_edit(GtkWidget *w, GtkWidget *parent)
{
	int num = os_pixmaps_row_number;
	os_pixmap_list_t *os_pixmap;
	char *ptrs[2];

	if(os_pixmaps_row_number < 0)
		return;

	os_pixmap = gtk_clist_get_row_data(GTK_CLIST(os_pixmaps_clist), os_pixmaps_row_number);
	
	os_pixmap = os_pixmaps_make_dialog(os_pixmap, os_pixmap->pixmap_name,os_pixmap->string, parent);

	gtk_clist_freeze(GTK_CLIST(os_pixmaps_clist));
	gtk_clist_remove(GTK_CLIST(os_pixmaps_clist), os_pixmaps_row_number);

	if(os_pixmap)
	{
		ptrs[0] = os_pixmap->pixmap_name;
		ptrs[1] = os_pixmap->string;
		
		gtk_clist_insert(GTK_CLIST(os_pixmaps_clist), num, ptrs);
 		gtk_clist_set_row_data(GTK_CLIST(os_pixmaps_clist), num, os_pixmap);
 		{
 			GdkBitmap *mask;
 			GdkPixmap *pixmap_data;
 			
 			get_my_pixmap_and_mask(os_pixmap->pixmap_name, &pixmap_data, &mask);
 	
 			if(pixmap_data && mask)
 				gtk_clist_set_pixmap(GTK_CLIST(os_pixmaps_clist),num, 0, pixmap_data, mask); 
 		}
	}
	gtk_clist_columns_autosize(GTK_CLIST(os_pixmaps_clist));
	gtk_clist_thaw(GTK_CLIST(os_pixmaps_clist));
}

void os_pixmaps_do_delete(GtkWidget *w, gpointer p)
{
	os_pixmap_list_t *os_pixmap;

	if(os_pixmaps_row_number > -1)
	{
		os_pixmap = gtk_clist_get_row_data(GTK_CLIST(os_pixmaps_clist), os_pixmaps_row_number);

		os_pixmap_list_remove_ptr(os_pixmap);

		gtk_clist_freeze(GTK_CLIST(os_pixmaps_clist));
		gtk_clist_remove(GTK_CLIST(os_pixmaps_clist), os_pixmaps_row_number);
 		gtk_clist_columns_autosize(GTK_CLIST(os_pixmaps_clist));
		gtk_clist_thaw(GTK_CLIST(os_pixmaps_clist));

		os_pixmaps_row_number = -1;
	}
}


void plugins_select_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	plugins_row_number = row;
}
void plugins_unselect_row(GtkWidget *clist, gint row, gint column, GdkEventButton *event, gpointer data)
{
	plugins_row_number = -1;
}


void plugins_do_add(GtkWidget *w, GtkWidget *parent)
{
}

void plugins_do_configure(GtkWidget *w, GtkWidget *parent)
{
}

void plugins_do_delete(GtkWidget *w, gpointer ptr)
{
	plugin_list_t *p;

	if(plugins_row_number > -1)
	{
		p = gtk_clist_get_row_data(GTK_CLIST(plugins_clist), plugins_row_number);

//		plugins_list_remove_ptr(p);

		gtk_clist_freeze(GTK_CLIST(plugins_clist));
		gtk_clist_remove(GTK_CLIST(plugins_clist), plugins_row_number);
 		gtk_clist_columns_autosize(GTK_CLIST(plugins_clist));
		gtk_clist_thaw(GTK_CLIST(plugins_clist));
		plugins_row_number = -1;
	}
}

void fastscan_toggle(GtkToggleButton *w, void *blah)
{
	int value;
	
	value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(os_scan_fastscan));	\
	if(value)
	{
		gtk_widget_set_sensitive(os_scan_ports, 0);
		gtk_widget_set_sensitive(os_scan_ports_label, 0);
		gtk_widget_set_sensitive(os_scan_scan_specified_ports, 0);
	}
	else
	{
		gtk_widget_set_sensitive(os_scan_ports, 1);
		gtk_widget_set_sensitive(os_scan_ports_label, 1);
		gtk_widget_set_sensitive(os_scan_scan_specified_ports, 1);
	}
}

void settings_script_add_clist(script_t *s, int *i)
{
	char *ptrs[3];
	
	ptrs[0] = s->name;
	ptrs[1] = s->script;
	ptrs[2] = (s->flags & SCRIPT_FLAGS_VIEWABLE) ? "yes" : "no";

	gtk_clist_append(GTK_CLIST(scripts_clist),ptrs);
	gtk_clist_set_row_data(GTK_CLIST(scripts_clist), *i, (void *)s);
	(*i)++;
}

void do_settings(GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;
	GtkWidget *notebook;
	GtkWidget *table,*table2,*table3;
	GtkWidget *scrolled_window;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *frame;
	GtkWidget *vbox;
	GList *comboList = NULL;
	char comboListBuffer[MAX_DISCOVER_RETRIES*3];
	char *char_ptr;
	int keepgoing, i;
	char *services_clist_titles[] = { "Service", "Port", "Protocol", "Command to execute" };
	char *scripts_clist_titles[] = { "Name", "Script", "Viewable" };
	GtkWidget *services_delete;
	GtkWidget *services_edit;
	GtkWidget *scripts_delete;
	GtkWidget *scripts_edit;
	char *os_pixmaps_clist_titles[] = {"File Name", "Familiar string for OS" };
    GtkWidget *os_pixmaps_delete;
    GtkWidget *os_pixmaps_edit;
#ifdef USING_PROBE_PORTS
	char *plugins_clist_titles[] = {"Plugin Name", "Description"};
	char *probe_ports_clist_titles[] = {"Service Name", "Port", "Send data", "Recieve Parser", "Recieve Parser Argument"};
#endif	
	
	dialog = gnome_dialog_new("Settings for cheops-ng and the agent",
				  GNOME_STOCK_BUTTON_APPLY,
				  GNOME_STOCK_BUTTON_OK,
				  GNOME_STOCK_BUTTON_CANCEL,NULL);
	gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(main_window->window));
		
	table = gtk_table_new(7, 2, TRUE);

	notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
	gtk_table_attach_defaults(GTK_TABLE(table),notebook,0,2,0,7);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(dialog)->vbox), table, FALSE, FALSE, 0);
	
/*
 * Agent Settings Tab
 */

	label = gtk_label_new("Agent");
	gtk_widget_show(label);
	table2 = gtk_vbox_new(FALSE, 5);
	gtk_widget_show(table2);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table2,label);

	if( options_discover_retries > MAX_DISCOVER_RETRIES || ((int)options_discover_retries) < 0)
		options_discover_retries = 2;

	label = gtk_label_new("Discover Retries:");
	discover_retries = gtk_combo_new();
	char_ptr = comboListBuffer;
	for(i = 0; i < MAX_DISCOVER_RETRIES; i++)
	{
		sprintf(char_ptr, "%d", i);
		comboList = g_list_append(comboList, char_ptr);
		char_ptr += strlen(char_ptr) + 1;
	}
	gtk_combo_set_popdown_strings(GTK_COMBO(discover_retries),comboList);
	g_list_free(comboList);
	gtk_combo_set_use_arrows_always(GTK_COMBO(discover_retries),1);
	sprintf(comboListBuffer, "%d", options_discover_retries);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(discover_retries)->entry), comboListBuffer);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(discover_retries)->entry), FALSE);
	gtk_widget_show(discover_retries);
	gtk_widget_show(label);

	table3 = gtk_table_new(1,2,TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table3),label,0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table3),discover_retries,1,2,0,1);
	gtk_widget_show(table3);
	
	frame = gtk_frame_new("Discover Settings");
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), table3, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table2), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);
	gtk_widget_show(vbox);

	os_scan_osscan = gtk_check_button_new_with_label("Enable the OS detection option");
	SET_CHECK_OPTION(os_scan_osscan, options_os_scan_osscan);

	os_scan_dont_ping = gtk_check_button_new_with_label("Don't ping the host");
	SET_CHECK_OPTION(os_scan_dont_ping, options_os_scan_dont_ping);

	os_scan_udp_scan = gtk_check_button_new_with_label("Enable UDP port scanning");
	SET_CHECK_OPTION(os_scan_udp_scan, options_os_scan_udp_scan);

	os_scan_rpc_scan = gtk_check_button_new_with_label("Enable RPC scanning");
	SET_CHECK_OPTION(os_scan_rpc_scan, options_os_scan_rpc_scan);

	os_scan_identd_scan = gtk_check_button_new_with_label("Enable identd scanning");
	SET_CHECK_OPTION(os_scan_identd_scan, options_os_scan_identd_scan);

	os_scan_fastscan = gtk_check_button_new_with_label("Enable Fastscan");
	SET_CHECK_OPTION(os_scan_fastscan, options_os_scan_fastscan);
	gtk_signal_connect(GTK_OBJECT(os_scan_fastscan), "toggled", (GtkSignalFunc)fastscan_toggle, NULL);

	os_scan_scan_specified_ports = gtk_check_button_new_with_label("Specify the ports to scan");
	SET_CHECK_OPTION(os_scan_scan_specified_ports, options_os_scan_scan_specified_ports);

	frame = gtk_frame_new("Nmap settings");
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), os_scan_osscan, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), os_scan_udp_scan, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), os_scan_rpc_scan, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), os_scan_identd_scan, FALSE, FALSE, 0);


	label = gtk_label_new("Select TCP Scan Type:");
	os_scan_tcp_scan = gtk_combo_new();
	comboList = NULL;
	comboList = g_list_append(comboList, TCP_SCAN_0);
	comboList = g_list_append(comboList, TCP_SCAN_1);
	comboList = g_list_append(comboList, TCP_SCAN_2);
	comboList = g_list_append(comboList, TCP_SCAN_3);
	comboList = g_list_append(comboList, TCP_SCAN_4);
	gtk_combo_set_popdown_strings(GTK_COMBO(os_scan_tcp_scan),comboList);
	g_list_free(comboList);
	gtk_combo_set_use_arrows_always(GTK_COMBO(os_scan_tcp_scan),1);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(os_scan_tcp_scan)->entry), options_os_scan_tcp_scan);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(os_scan_tcp_scan)->entry), FALSE);
	table3 = gtk_table_new(1,2,TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table3),label,0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table3),os_scan_tcp_scan,1,2,0,1);
	gtk_widget_show(table3);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), table3, FALSE, FALSE, 0);

	label = gtk_label_new("Select Scan Timing:");
	os_scan_timing = gtk_combo_new();
	comboList = NULL;
	comboList = g_list_append(comboList, TIMING_SCAN_NORMAL);
	comboList = g_list_append(comboList, TIMING_SCAN_PARANOID);
	comboList = g_list_append(comboList, TIMING_SCAN_SNEAKY);
	comboList = g_list_append(comboList, TIMING_SCAN_POLITE);
	comboList = g_list_append(comboList, TIMING_SCAN_AGGRESSIVE);
	comboList = g_list_append(comboList, TIMING_SCAN_INSANE);
	gtk_combo_set_popdown_strings(GTK_COMBO(os_scan_timing),comboList);
	g_list_free(comboList);
	gtk_combo_set_use_arrows_always(GTK_COMBO(os_scan_timing),1);
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(os_scan_timing)->entry), options_os_scan_timing);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(os_scan_timing)->entry), FALSE);
	table3 = gtk_table_new(1,2,TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table3),label,0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table3),os_scan_timing,1,2,0,1);
	gtk_widget_show(table3);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), table3, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), os_scan_dont_ping, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), os_scan_fastscan, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), os_scan_scan_specified_ports, FALSE, FALSE, 0);

	os_scan_ports = gnome_entry_new("os_scan_ports");

	gtk_entry_set_text(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(os_scan_ports))), options_os_scan_ports);
	gtk_entry_set_max_length(GTK_ENTRY(gnome_entry_gtk_entry(GNOME_ENTRY(os_scan_ports))), OPTIONS_OS_SCAN_PORTS_SIZE);
	os_scan_ports_label = gtk_label_new("Ports:");
	table3 = gtk_table_new(1,2,TRUE);
	gtk_table_attach_defaults(GTK_TABLE(table3),os_scan_ports_label,0,1,0,1);
	gtk_table_attach_defaults(GTK_TABLE(table3),os_scan_ports,1,2,0,1);
	gtk_widget_show(table3);
	gtk_widget_show(os_scan_ports_label);
	gtk_box_pack_start(GTK_BOX(vbox), table3, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(table2), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);
	gtk_widget_show(vbox);
	gtk_widget_show(os_scan_osscan);
	gtk_widget_show(os_scan_udp_scan);
	gtk_widget_show(os_scan_rpc_scan);
	gtk_widget_show(os_scan_identd_scan);
	gtk_widget_show(os_scan_tcp_scan);
	gtk_widget_show(os_scan_timing);
	gtk_widget_show(os_scan_scan_specified_ports);
	gtk_widget_show(os_scan_ports);
	gtk_widget_show(os_scan_ports_label);
	gtk_widget_show(os_scan_fastscan);
	gtk_widget_show(os_scan_dont_ping);
	
	fastscan_toggle(GTK_TOGGLE_BUTTON(os_scan_fastscan), NULL);

/*
 * Features Settings Tab
 */
	label = gtk_label_new("Features");
	table2 = gtk_vbox_new(FALSE, 5);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table2,label);

	operating_system_check = gtk_check_button_new_with_label("Determine the operating system of new hosts");
	SET_CHECK_OPTION(operating_system_check, options_operating_system_check);
	
	probe_ports = gtk_check_button_new_with_label("Determine the version of services running on hosts");
	SET_CHECK_OPTION(probe_ports, options_probe_ports);

	reverse_dns = gtk_check_button_new_with_label("Look up hosts I add with reverse DNS");
	SET_CHECK_OPTION(reverse_dns, options_reverse_dns);
	
	map = gtk_check_button_new_with_label("Automatically map the network");
	SET_CHECK_OPTION(map, options_map);

	rescan_at_startup = gtk_check_button_new_with_label("Re-OS Scan/Port Scan/ICMP Map hosts from map file");
	SET_CHECK_OPTION(rescan_at_startup, options_rescan_at_startup);

	move_stuff_live = gtk_check_button_new_with_label("Update the viewspace while moving stuff (turn off for slow puters)");
	SET_CHECK_OPTION(move_stuff_live, options_move_stuff_live);

	use_ip_for_label = gtk_check_button_new_with_label("Use the IP address for the label");
	SET_CHECK_OPTION(use_ip_for_label, options_use_ip_for_label);
	
	only_display_hostname = gtk_check_button_new_with_label("Only display hostname, instead of the FQDN");
	SET_CHECK_OPTION(only_display_hostname, options_only_display_hostname);
	
	confirm_delete = gtk_check_button_new_with_label("Confirm Deleting of hosts");
	SET_CHECK_OPTION(confirm_delete, options_confirm_delete);

	save_changes_on_exit = gtk_check_button_new_with_label("Save changes on exit");
	SET_CHECK_OPTION(save_changes_on_exit, options_save_changes_on_exit);

	use_ip_for_merged_ports = gtk_check_button_new_with_label("Use IP address for merged host's menues");
	SET_CHECK_OPTION(use_ip_for_merged_ports, options_use_ip_for_merged_ports);

	frame = gtk_frame_new("Tool Options");
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), operating_system_check, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), reverse_dns, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), map, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), probe_ports, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), rescan_at_startup, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table2), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);
	gtk_widget_show(vbox);
		
	frame = gtk_frame_new("Look and Feel");
	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), confirm_delete, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), use_ip_for_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), only_display_hostname, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), save_changes_on_exit, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), move_stuff_live, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), use_ip_for_merged_ports, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(table2), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);
	gtk_widget_show(vbox);

	gtk_widget_show(frame);
	gtk_widget_show(save_changes_on_exit);
	gtk_widget_show(confirm_delete);
	gtk_widget_show(reverse_dns);
	gtk_widget_show(map);
	gtk_widget_show(probe_ports);
	gtk_widget_show(move_stuff_live);
	gtk_widget_show(use_ip_for_label);
	gtk_widget_show(operating_system_check);
	gtk_widget_show(use_ip_for_merged_ports);
	gtk_widget_show(only_display_hostname);
	gtk_widget_show(rescan_at_startup);
	gtk_widget_show(table2);

/*
 * Services Settings Tab
 */
	label = gtk_label_new("Services");
	table2 = gtk_table_new(9, 3, TRUE);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table2,label);
	
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);


	services_clist = gtk_clist_new_with_titles(4,services_clist_titles);
	gtk_clist_set_selection_mode(GTK_CLIST(services_clist),GTK_SELECTION_SINGLE);
	gtk_clist_set_shadow_type(GTK_CLIST(services_clist), GTK_SHADOW_ETCHED_OUT);
	gtk_clist_set_column_width(GTK_CLIST(services_clist), 0, 100);
	gtk_clist_set_column_width(GTK_CLIST(services_clist), 1, 35);
	gtk_clist_set_column_width(GTK_CLIST(services_clist), 2, 50);
	gtk_clist_column_titles_passive(GTK_CLIST(services_clist));
	gtk_signal_connect(GTK_OBJECT(services_clist), "select_row",GTK_SIGNAL_FUNC(services_select_row),NULL);
	gtk_signal_connect(GTK_OBJECT(services_clist), "unselect_row",GTK_SIGNAL_FUNC(services_unselect_row),NULL);

	gtk_container_add(GTK_CONTAINER(scrolled_window), services_clist);

	gtk_table_attach_defaults(GTK_TABLE(table2), scrolled_window, 0, 3, 0, 8);
	
	{
		service_list_t *p;
		char *ptrs[4];
		char name[1024];
		char port[10];
		char protocol[4];
		char command[1024];
		int i = 0;

		ptrs[0] = name;
		ptrs[1] = port;
		ptrs[2] = protocol;
		ptrs[3] = command;
				
	    for(p = service_list; p; p = p->next)
       	{
			if(p->name == NULL)
				sprintf(name, "Port %d", p->port);
			else
				sprintf(name, "%s", p->name);
		
			sprintf(port, "%d", p->port);
			strcpy(protocol, (p->protocol == 6 ? "TCP" : "UDP"));
			strcpy(command, p->command);

			gtk_clist_append(GTK_CLIST(services_clist),ptrs);
			gtk_clist_set_row_data(GTK_CLIST(services_clist), i, (void *)p);
			i++;
       	}
	}	
	
	button = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_ADD), "Add");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(services_do_add), dialog);

	services_edit = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_PROPERTIES), "Edit");
	gtk_signal_connect(GTK_OBJECT(services_edit), "clicked", GTK_SIGNAL_FUNC(services_do_edit), dialog);

	services_delete = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_REMOVE), "Delete");
	gtk_signal_connect(GTK_OBJECT(services_delete), "clicked", GTK_SIGNAL_FUNC(services_do_delete), NULL);
	 

	gtk_table_attach(GTK_TABLE(table2), button,          0, 1, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	gtk_table_attach(GTK_TABLE(table2), services_edit,   1, 2, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	gtk_table_attach(GTK_TABLE(table2), services_delete, 2, 3, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	
	gtk_widget_show(services_clist);
	gtk_widget_show(button);
	gtk_widget_show(services_edit);
	gtk_widget_show(services_delete);
	gtk_widget_show(scrolled_window);
	gtk_widget_show(table2);


/*
 * Script Settings Tab
 */
	label = gtk_label_new("Scripts");
	table2 = gtk_table_new(9, 3, TRUE);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table2,label);
	
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);


	scripts_clist = gtk_clist_new_with_titles(3, scripts_clist_titles);
	gtk_clist_set_selection_mode(GTK_CLIST(scripts_clist),GTK_SELECTION_SINGLE);
	gtk_clist_set_shadow_type(GTK_CLIST(scripts_clist), GTK_SHADOW_ETCHED_OUT);
	gtk_clist_set_column_width(GTK_CLIST(scripts_clist), 0, 80);
	gtk_clist_set_column_width(GTK_CLIST(scripts_clist), 1, 230);
	gtk_clist_column_titles_passive(GTK_CLIST(scripts_clist));
	gtk_signal_connect(GTK_OBJECT(scripts_clist), "select_row",GTK_SIGNAL_FUNC(scripts_select_row),NULL);
	gtk_signal_connect(GTK_OBJECT(scripts_clist), "unselect_row",GTK_SIGNAL_FUNC(scripts_unselect_row),NULL);

	gtk_container_add(GTK_CONTAINER(scrolled_window), scripts_clist);

	gtk_table_attach_defaults(GTK_TABLE(table2), scrolled_window, 0, 3, 0, 8);

	i = 0;
	script_foreach((GFunc)settings_script_add_clist, &i);
	button = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_ADD), "Add");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(scripts_do_add), dialog);

	scripts_edit = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_PROPERTIES), "Edit");
	gtk_signal_connect(GTK_OBJECT(scripts_edit), "clicked", GTK_SIGNAL_FUNC(scripts_do_edit), dialog);

	scripts_delete = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_REMOVE), "Delete");
	gtk_signal_connect(GTK_OBJECT(scripts_delete), "clicked", GTK_SIGNAL_FUNC(scripts_do_delete), NULL);
	 

	gtk_table_attach(GTK_TABLE(table2), button,          0, 1, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	gtk_table_attach(GTK_TABLE(table2), scripts_edit,   1, 2, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	gtk_table_attach(GTK_TABLE(table2), scripts_delete, 2, 3, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	
	gtk_widget_show(scripts_clist);
	gtk_widget_show(button);
	gtk_widget_show(scripts_edit);
	gtk_widget_show(scripts_delete);
	gtk_widget_show(scrolled_window);
	gtk_widget_show(table2);

//	gtk_clist_optimal_column_width(GTK_CLIST(scripts_clist), 0);
//	gtk_clist_optimal_column_width(GTK_CLIST(scripts_clist), 1);
//	gtk_clist_optimal_column_width(GTK_CLIST(scripts_clist), 2);
	

/*
 * Service Version Detection Tab
 */
#ifdef USING_PROBE_PORTS
	label = gtk_label_new("Service Version Detection");
	table2 = gtk_table_new(6, 3, TRUE);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table2,label);
	
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);


	probe_ports_clist = gtk_clist_new_with_titles(5,probe_ports_clist_titles);
	gtk_clist_set_selection_mode(GTK_CLIST(probe_ports_clist),GTK_SELECTION_SINGLE);
	gtk_clist_set_shadow_type(GTK_CLIST(probe_ports_clist), GTK_SHADOW_ETCHED_OUT);
	gtk_clist_set_column_width(GTK_CLIST(probe_ports_clist), 0, 100);
	gtk_clist_set_column_width(GTK_CLIST(probe_ports_clist), 1, 30);
	gtk_clist_set_column_width(GTK_CLIST(probe_ports_clist), 2, 50);
	gtk_clist_column_titles_passive(GTK_CLIST(probe_ports_clist));
	gtk_signal_connect(GTK_OBJECT(probe_ports_clist), "select_row",GTK_SIGNAL_FUNC(probe_ports_select_row),NULL);
	gtk_signal_connect(GTK_OBJECT(probe_ports_clist), "unselect_row",GTK_SIGNAL_FUNC(probe_ports_unselect_row),NULL);

	gtk_container_add(GTK_CONTAINER(scrolled_window), probe_ports_clist);

	gtk_table_attach_defaults(GTK_TABLE(table2), scrolled_window, 0, 3, 0, 5);
	
	button = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_ADD), "Add");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(probe_ports_do_add), dialog);
	gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 5, 6, GTK_FILL, GTK_FILL, 5, 5);
	gtk_widget_show(button);

	button = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_PROPERTIES), "Configure");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(probe_ports_do_configure), dialog);
	gtk_table_attach(GTK_TABLE(table2), button, 1, 2, 5, 6, GTK_FILL, GTK_FILL, 5, 5);
	gtk_widget_show(button);

	button = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_REMOVE), "Remove");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(probe_ports_do_delete), NULL);
	gtk_table_attach(GTK_TABLE(table2), button, 2, 3, 5, 6, GTK_FILL, GTK_FILL, 5, 5);
	gtk_widget_show(button);
	
	
	gtk_widget_show(scrolled_window);
	gtk_widget_show(probe_ports_clist);
	gtk_widget_show(table2);
#endif

/*
 * OS Pixmaps Tab
 */
	label = gtk_label_new("OS Pixmaps");
	table2 = gtk_table_new(9, 3, TRUE);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table2,label);
	
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);


	os_pixmaps_clist = gtk_clist_new_with_titles(2, os_pixmaps_clist_titles);
	gtk_clist_set_selection_mode(GTK_CLIST(os_pixmaps_clist), GTK_SELECTION_SINGLE);
	gtk_clist_set_shadow_type(GTK_CLIST(os_pixmaps_clist), GTK_SHADOW_ETCHED_OUT);
	gtk_clist_set_column_width(GTK_CLIST(os_pixmaps_clist), 0, 150);
	gtk_clist_column_titles_passive(GTK_CLIST(os_pixmaps_clist));

	gtk_signal_connect(GTK_OBJECT(os_pixmaps_clist), "select_row",GTK_SIGNAL_FUNC(os_pixmaps_select_row),NULL);
	gtk_signal_connect(GTK_OBJECT(os_pixmaps_clist), "unselect_row",GTK_SIGNAL_FUNC(os_pixmaps_unselect_row), NULL);

	gtk_container_add(GTK_CONTAINER(scrolled_window), os_pixmaps_clist);

	gtk_table_attach_defaults(GTK_TABLE(table2), scrolled_window, 0, 3, 0, 8);
	
	{
		os_pixmap_list_t *p;
		char *ptrs[2];
		int num = 0;	
		int height = 0, h = 0;
				
	       	for(p = os_pixmap_list; p; p = p->next)
       		{
    	 		ptrs[0] = p->pixmap_name;
    	 		ptrs[1] = p->string;
           			
    			gtk_clist_append(GTK_CLIST(os_pixmaps_clist),ptrs);
    			gtk_clist_set_row_data(GTK_CLIST(os_pixmaps_clist), num, (void *)p);
    			
    			{
    				GdkBitmap *mask;
    				GdkPixmap *pixmap_data;
    				
    				get_my_pixmap_and_mask(p->pixmap_name, &pixmap_data, &mask);
    	
    				if(pixmap_data && mask)
    				{
    					GtkCListRow *clist_row;
    					gtk_clist_set_pixmap(GTK_CLIST(os_pixmaps_clist),num, 0, pixmap_data, mask); 
    					
    					clist_row = ROW_ELEMENT (GTK_CLIST(os_pixmaps_clist), num)->data;
    					
    					if( clist_row->cell[0].type == GTK_CELL_PIXMAP)
    					{
    						h = GTK_CELL_PIXMAP(clist_row->cell[0])->vertical;
    					
    						if(h > height)
    							height = h;
    					}
    				}
    			}
       			num++;
       		}
		gtk_clist_set_row_height(GTK_CLIST(os_pixmaps_clist), 50);
	}	
	gtk_clist_columns_autosize(GTK_CLIST(os_pixmaps_clist));
	
	
	button = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_ADD), "Add");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(os_pixmaps_do_add), dialog);

	os_pixmaps_edit = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_PROPERTIES), "Edit");
	gtk_signal_connect(GTK_OBJECT(os_pixmaps_edit), "clicked", GTK_SIGNAL_FUNC(os_pixmaps_do_edit), dialog);

	os_pixmaps_delete = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_REMOVE), "Delete");
	gtk_signal_connect(GTK_OBJECT(os_pixmaps_delete), "clicked", GTK_SIGNAL_FUNC(os_pixmaps_do_delete), NULL);
	 

	gtk_table_attach(GTK_TABLE(table2), button,            0, 1, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	gtk_table_attach(GTK_TABLE(table2), os_pixmaps_edit,   1, 2, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	gtk_table_attach(GTK_TABLE(table2), os_pixmaps_delete, 2, 3, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	
	gtk_widget_show(os_pixmaps_clist);
	gtk_widget_show(button);
	gtk_widget_show(os_pixmaps_edit);
	gtk_widget_show(os_pixmaps_delete);
	gtk_widget_show(scrolled_window);
	gtk_widget_show(table2);

#ifdef USING_PLUGINS
/*
 * Plugins Settings Tab
 */
	label = gtk_label_new("Plugins");
	table2 = gtk_table_new(6, 3, TRUE);

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),table2,label);
	
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);


	plugins_clist = gtk_clist_new_with_titles(2,plugins_clist_titles);
	gtk_clist_set_selection_mode(GTK_CLIST(plugins_clist),GTK_SELECTION_SINGLE);
	gtk_clist_set_shadow_type(GTK_CLIST(plugins_clist), GTK_SHADOW_ETCHED_OUT);
	gtk_clist_set_column_width(GTK_CLIST(plugins_clist), 0, 100);
	gtk_clist_set_column_width(GTK_CLIST(plugins_clist), 1, 30);
	gtk_clist_set_column_width(GTK_CLIST(plugins_clist), 2, 50);
	gtk_clist_column_titles_passive(GTK_CLIST(plugins_clist));
	gtk_signal_connect(GTK_OBJECT(plugins_clist), "select_row",GTK_SIGNAL_FUNC(plugins_select_row),NULL);
	gtk_signal_connect(GTK_OBJECT(plugins_clist), "unselect_row",GTK_SIGNAL_FUNC(plugins_unselect_row),NULL);

	gtk_container_add(GTK_CONTAINER(scrolled_window), plugins_clist);

	gtk_table_attach_defaults(GTK_TABLE(table2), scrolled_window, 0, 3, 0, 8);
	
	button = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_ADD), "Add");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(plugins_do_add), dialog);
	gtk_table_attach(GTK_TABLE(table2), button, 0, 1, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	gtk_widget_show(button);

	button = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_PROPERTIES), "Configure");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(plugins_do_configure), dialog);
	gtk_table_attach(GTK_TABLE(table2), button, 1, 2, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	gtk_widget_show(button);

	button = gnome_pixmap_button( gnome_stock_pixmap_widget(dialog, GNOME_STOCK_PIXMAP_REMOVE), "Remove");
	gtk_signal_connect(GTK_OBJECT(button), "clicked", GTK_SIGNAL_FUNC(plugins_do_delete), NULL);
	gtk_table_attach(GTK_TABLE(table2), button, 2, 3, 8, 9, GTK_FILL, GTK_FILL, 5, 5);
	gtk_widget_show(button);
	
	
	gtk_widget_show(scrolled_window);
	gtk_widget_show(plugins_clist);
	gtk_widget_show(table2);
#endif
/* this is the end of the notebook */

	gtk_widget_show(notebook);
	gtk_widget_show(table);

	keepgoing = TRUE;
	while(keepgoing)
	{
		switch( gnome_dialog_run(GNOME_DIALOG(dialog)) )
		{
			case -1:
				keepgoing = FALSE;
				break;
			case 0:
				apply_settings(NULL, NULL);
				keepgoing = TRUE;
				break;
			case 1:
				apply_settings(NULL, NULL);
				gnome_dialog_close(GNOME_DIALOG(dialog));
				keepgoing = FALSE;
				break;
			case 2:
				gnome_dialog_close(GNOME_DIALOG(dialog));
				keepgoing = FALSE;
				break;
		}
	}
}

