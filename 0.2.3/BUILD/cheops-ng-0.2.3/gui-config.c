/*
 * Cheops Next Generation GUI
 * 
 * gui-config.c
 * Config file reader and parser
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
#include <gnome.h>
#include "event.h"
#include "logger.h"
#include "gui-sched.h"
#include "cheops-gui.h"
#include "gui-viewspace.h"
#include "gui-handlers.h"
#include "gui-settings.h"
#include "gui-utils.h"
#include "gui-canvas.h"
#include "gui-config.h"
#include "gui-service.h"
#include "ip_utils.h"
#include "gui-pixmap.h"
#include "gui-handlers.h"
#include "cheops-osscan.h"
#include "script.h"

//#define DEBUG_CONFIG

#ifdef DEBUG_CONFIG
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

#define CONFIG_BUFFER_SIZE 1024
		
void add_name_to_buf(char *base, char *buf, char *add)
{
	sprintf(buf, "%s%s",base,add);
}

char *config_convert_notes(char *notes)
{
	char *string;
	int len = strlen(notes);
	int i, index = 0;
	
	if( ( string = malloc(len + 1) ) == NULL)
	{
		printf("\n bad error\n");
		exit(1);
	}
	
	for(i = 0; i <= len; i++)
	{
		if((string[i] = notes[i + index]) == '%' && 
				notes[i + index + 1] == '0' && 
				notes[i + index + 2] == 'a' 
		)
		{
			string[i] = '\n';
			index +=2;
		}
	}
	free(notes);
	return(string);
}	

char *config_get_agent_ip_address(void)
{
	int def;
	char *ip;
	char buf[CONFIG_BUFFER_SIZE];
	char buf2[CONFIG_BUFFER_SIZE];
	FILE *fp;
	
	sprintf(buf2,"%s/%s",getenv("HOME"), CHEOPS_DEFAULT_FILENAME_NOT_HOME);
	
	if( (fp = fopen(buf2, "rt")) == NULL)
	{
		return(NULL);
	}
	fclose(fp);
	
	sprintf(buf,"=%s=/viewspace0/agent_ip=NO_IP",buf2);
	
	ip = gnome_config_get_string_with_default(buf,&def);
	if(def)
	{
		free(ip);
		ip = NULL;
	}
	
	return(ip);	
}

int *config_get_agent_usessl(void)
{
	int def;
	int *usessl;
	char buf[CONFIG_BUFFER_SIZE];
	char buf2[CONFIG_BUFFER_SIZE];
	FILE *fp;
	
	sprintf(buf2,"%s/%s",getenv("HOME"), CHEOPS_DEFAULT_FILENAME_NOT_HOME);
	
	if( (fp = fopen(buf2, "rt")) == NULL)
	{
		return(NULL);
	}
	fclose(fp);
	
	sprintf(buf,"=%s=/viewspace0/agent_usessl=0",buf2);
	
	usessl = malloc(sizeof(int));
	*usessl = gnome_config_get_int_with_default(buf,&def);
	if(def)
	{
		free(usessl);
		usessl = NULL;
	}
	
	return(usessl);	
}

void read_config_file(GtkWidget *w, GtkWidget *fs)
{
	char buf[CONFIG_BUFFER_SIZE], buf2[CONFIG_BUFFER_SIZE], base[CONFIG_BUFFER_SIZE], *filename;
	net_page *np;
	page_object *po;
	int i=0, j=0, def=0;
	char *temp_string;
	char *name;
	char *po_name;
	char *po_ip;
	char *os_name;
	char *fp_string;
	char *fp_pixmap;
	char *service_name;
	int port_count;
	int service_port;
	int service_protocol;
	char *service_command;
	os_port_entry *current = NULL;
	char *agent_ip;	
	double ppu;
	
// default the service list and pixmaps list so the use without a config file can use these
	os_pixmap_list_default();
	service_list_default();
	script_default();
	
	if(fs != NULL)
	{
		filename = makestring(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));

		if(main_window->filename)
			free(main_window->filename);
			
		main_window->filename = filename;
	
		gtk_widget_destroy(fs);
	}
	else
	{
		filename = main_window->filename;
	}

// this is the case when we have started 'aclean and have no config file
	if(filename == NULL)
		return;
	
	/* lets write somethings in it */
	sprintf(buf,"=%s=/global/options_discover_retries=2",filename);
	options_discover_retries = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_DISCOVER_RETRIES);

	sprintf(buf,"=%s=/global/options_rescan_at_startup=0",filename);
	options_rescan_at_startup = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_RESCAN_AT_STARTUP);

	sprintf(buf,"=%s=/global/options_operating_system_check=%d",filename, options_operating_system_check);
	options_operating_system_check = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_OPERATING_SYSTEM_CHECK);

	sprintf(buf,"=%s=/global/options_confirm_delete=%d",filename, options_confirm_delete);
	options_confirm_delete = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_CONFIRM_DELETE);

	sprintf(buf,"=%s=/global/options_save_changes_on_exit=%d",filename, options_save_changes_on_exit);
	options_save_changes_on_exit = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_SAVE_CHANGES_ON_EXIT);

	sprintf(buf,"=%s=/global/options_reverse_dns=%d",filename, options_reverse_dns);
	options_reverse_dns = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_REVERSE_DNS);

	sprintf(buf,"=%s=/global/options_use_ip_for_label=%d",filename, options_use_ip_for_label);
	options_use_ip_for_label = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_USE_IP_FOR_LABEL);

	sprintf(buf,"=%s=/global/options_only_display_hostname=%d",filename, options_only_display_hostname);
	options_only_display_hostname = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_ONLY_DISPLAY_HOSTNAME);

	sprintf(buf,"=%s=/global/options_move_stuff_live=%d",filename, options_move_stuff_live);
	options_move_stuff_live = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_MOVE_STUFF_LIVE);

	sprintf(buf,"=%s=/global/options_probe_ports=%d",filename, options_probe_ports);
	options_probe_ports = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_PROBE_PORTS);

	sprintf(buf,"=%s=/global/options_map=%d",filename, options_map);
	options_map = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_MAP);

	sprintf(buf,"=%s=/global/options_os_scan_ports=%s",filename, options_os_scan_ports);
	temp_string = gnome_config_get_string_with_default(buf,&def);
	if(def)
		options_os_scan_ports[0] = '\0';
	else
		strncpy(options_os_scan_ports, temp_string, sizeof(options_os_scan_ports));
	free(temp_string);
	apply_the_setting(OPTIONS_OS_SCAN_PORTS);

	sprintf(buf,"=%s=/global/options_os_scan_tcp_scan=%s",filename, options_os_scan_tcp_scan);
	temp_string = gnome_config_get_string_with_default(buf,&def);
	if(def)
		strncpy(options_os_scan_tcp_scan, TCP_SCAN_DEFAULT, sizeof(options_os_scan_tcp_scan));
	else
		strncpy(options_os_scan_tcp_scan, temp_string, sizeof(options_os_scan_tcp_scan));
	free(temp_string);
	apply_the_setting(OPTIONS_OS_SCAN_TCP_SCAN);

	sprintf(buf,"=%s=/global/options_os_scan_timing=%s",filename, options_os_scan_timing);
	temp_string = gnome_config_get_string_with_default(buf,&def);
	if(def)
		strncpy(options_os_scan_timing, TIMING_SCAN_DEFAULT, sizeof(options_os_scan_timing));
	else
		strncpy(options_os_scan_timing, temp_string, sizeof(options_os_scan_timing));
	free(temp_string);
	apply_the_setting(OPTIONS_OS_SCAN_TIMING);

	sprintf(buf,"=%s=/global/options_os_scan_udp_scan=%d",filename, options_os_scan_udp_scan);
	options_os_scan_udp_scan = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_OS_SCAN_UDP_SCAN);

	sprintf(buf,"=%s=/global/options_os_scan_rpc_scan=%d",filename, options_os_scan_rpc_scan);
	options_os_scan_rpc_scan = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_OS_SCAN_RPC_SCAN);

	sprintf(buf,"=%s=/global/options_os_scan_identd_scan=%d",filename, options_os_scan_identd_scan);
	options_os_scan_identd_scan = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_OS_SCAN_IDENTD_SCAN);

	sprintf(buf,"=%s=/global/options_os_scan_fastscan=%d",filename, options_os_scan_fastscan);
	options_os_scan_fastscan = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_OS_SCAN_FASTSCAN);

	sprintf(buf,"=%s=/global/options_os_scan_osscan=%d",filename, options_os_scan_osscan);
	options_os_scan_osscan = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_OS_SCAN_OSSCAN);

	sprintf(buf,"=%s=/global/options_os_scan_dont_ping=%d",filename, options_os_scan_dont_ping);
	options_os_scan_dont_ping = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_OS_SCAN_DONT_PING);

	sprintf(buf,"=%s=/global/options_os_scan_scan_specified_ports=%d",filename, options_os_scan_scan_specified_ports);
	options_os_scan_scan_specified_ports = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_OS_SCAN_SCAN_SPECIFIED_PORTS);

	sprintf(buf,"=%s=/global/options_use_ip_for_merged_ports=%d",filename, options_use_ip_for_merged_ports);
	options_use_ip_for_merged_ports = gnome_config_get_int_with_default(buf,&def);
	apply_the_setting(OPTIONS_USE_IP_FOR_MERGED_PORTS);

	sprintf(buf,"=%s=/global/width=%d",filename, CHEOPS_MIN_WIDTH);
	i = gnome_config_get_int_with_default(buf,&def);

	sprintf(buf,"=%s=/global/height=%d",filename, CHEOPS_MIN_HEIGHT);
	j = gnome_config_get_int_with_default(buf,&def);

	gtk_window_set_default_size(GTK_WINDOW(main_window->window), i, j);

/*
 * These are all of the strings to match OS names with
 */
 	for(i = 0;;)
 	{
		sprintf(buf,"=%s=/global/fingerprint_string%d=NO_STRING",filename,i);
		fp_string = gnome_config_get_string_with_default(buf,&def);
		if(def)
		{
			free(fp_string);
			break;
		}
			
		sprintf(buf,"=%s=/global/fingerprint_pixmap%d=NO_STRING",filename,i);
		fp_pixmap = gnome_config_get_string_with_default(buf,&def);
		i++;
		
		if(def)
		{
			free(fp_string);
			free(fp_pixmap);
			break;
		}
			
		os_pixmap_list_add(fp_string, fp_pixmap);
		free(fp_string);
		free(fp_pixmap);
	}

/*
 * These are all of the strings for the type of services and their command
 */
 	for(i = 0;;)
 	{
		sprintf(buf,"=%s=/global/service%d_name=NO_STRING",filename,i);
		service_name = gnome_config_get_string_with_default(buf,&def);
		if(def)
		{
			free(service_name);
			break;
		}
			
		sprintf(buf,"=%s=/global/service%d_port=0",filename,i);
		service_port = gnome_config_get_int_with_default(buf,&def);
		
		if(def)
		{
			free(service_name);
			break;
		}
			
		sprintf(buf,"=%s=/global/service%d_protocol=0",filename,i);
		service_protocol = gnome_config_get_int_with_default(buf,&def);
		
		if(def)
		{
			free(service_name);
			break;
		}
			
		sprintf(buf,"=%s=/global/service%d_command=NO_STRING",filename,i);
		service_command = gnome_config_get_string_with_default(buf,&def);
		
		if(def)
		{
			free(service_name);
			free(service_command);
			break;
		}
			
		i++;
		service_list_add(service_name, service_port, service_protocol, service_command);
		free(service_name);
		free(service_command);
	}

/*
 * These are all of the strings for the scripts and their commands
 */
 	for(i = 0;;)
 	{
		char *script_name;
		char *script_command;
		unsigned int script_flags;
		
		sprintf(buf,"=%s=/global/script%d_name=NO_STRING",filename,i);
		script_name = gnome_config_get_string_with_default(buf,&def);
		if(def)
		{
			free(script_name);
			break;
		}
			
		sprintf(buf,"=%s=/global/script%d_command=NO_STRING",filename,i);
		script_command = gnome_config_get_string_with_default(buf,&def);
		
		if(def)
		{
			free(script_name);
			free(script_command);
			break;
		}

		sprintf(buf,"=%s=/global/script%d_flags=0",filename,i);
		script_flags = gnome_config_get_int_with_default(buf,&def);
		
		i++;
		script_add(script_name, script_command, script_flags);
		free(script_name);
		free(script_command);
	}


//	sprintf(buf,"=%s=/global/tooltips_timeout=1",filename);
//	tooltips_timeout = gnome_config_get_int_with_default(buf,&def);
	
	for(i=0;;i++)
	{
		int usessl = 0;
		
		sprintf(base,"=%s=/viewspace%d",filename,i);
		
		sprintf(buf,"%s%s",base, "/name=NOTHING");
		name = gnome_config_get_string_with_default(buf, &def);
		if(def) /* if it uses the default then there are no more viewspaces in the config file */
		{
			free(name);
			break;
		}

		sprintf(buf,"%s%s",base, "/agent_ip=NOTHING");
		agent_ip = gnome_config_get_string_with_default(buf, &def);
		if(def) 
		{
			free(agent_ip);
			agent_ip = NULL;
		}
		
		sprintf(buf,"%s%s",base, "/usessl=NOTHING");
		usessl = gnome_config_get_int_with_default(buf, &def);
		if(def) 
		{
			usessl = 0;
		}
		np = make_viewspace(name, agent_ip, usessl);
		if(name)
			free(name);
		if(agent_ip)
			free(agent_ip);
		
		sprintf(buf,"%s%s",base, "/pixels_per_unit=NOTHING");
		ppu = (double)gnome_config_get_float_with_default(buf, &def);
		if(!def) 
			net_page_set_pixels_per_unit(np, ppu);

		while(gtk_events_pending())		/* have to let the notebook add the page */
			gtk_main_iteration();
                                                       			
				
		/* add the reference to the page_objects in the netpage sections */
		strcat(base,"/page_object_");

		for(j=0;;j++)
		{
			char *notes;
			char *icon_file_name;
			os_stats *os_data;
			float x;
			float y;
			int flags = 0;
			
			
			sprintf(buf, "%s%d",base,j);

			add_name_to_buf(buf, buf2, "_ip=NOTHING");
			po_ip = gnome_config_get_string_with_default(buf2,&def);
			if(def || (po_ip && po_ip[0] == '\0'))
			{
				free(po_ip);
				break;
			}

			add_name_to_buf(buf, buf2, "_name=NOTHING");
			po_name = gnome_config_get_string_with_default(buf2,&def);
			if(def || (po_name && po_name[0] == '\0'))
			{
				free(po_name);
				po_name = NULL;
			}
				
			add_name_to_buf(buf, buf2, "_notes=NOTHING");
			notes = gnome_config_get_string_with_default(buf2,&def);
			
			if(def)
			{
				free(notes);
				notes = NULL;
			}
			else
				notes = config_convert_notes(notes);
			
			add_name_to_buf(buf, buf2, "_x=0.0");
			x = gnome_config_get_float_with_default(buf2,&def);
			
			add_name_to_buf(buf, buf2, "_y=0.0");
			y = gnome_config_get_float_with_default(buf2,&def);

// There is no need to save the flags right now			
//			add_name_to_buf(buf, buf2, "_flags=0");
//			flags = gnome_config_get_int_with_default(buf2,&def);
			
			flags |= PAGE_OBJECT_CONFIG_NEW;

			add_name_to_buf(buf, buf2, "_icon_file_name=BLAH");
			icon_file_name = gnome_config_get_string_with_default(buf2,&def);
			if(def)
			{
				free(icon_file_name);
				icon_file_name = NULL;
			}
			
			add_name_to_buf(buf, buf2, "_os_name=BLAH");
			os_name = gnome_config_get_string_with_default(buf2,&def);
			if(def)
			{
				free(os_name);
				os_data = NULL;
				os_name = NULL;
			}
			else
			{
				os_data = malloc(sizeof(*os_data));
				memset(os_data, 0, sizeof(*os_data));
				os_data->ports = NULL;
				os_data->os = os_name;
			}
			current = NULL;			
			for(port_count = 0;;port_count++)
			{
				char port_buffer[1024];
				int port;
				int protocol;
				int state;
				int rpcnum;
				int rpclowver;
				int rpchighver;
				int proto;
				char *owner;
				char *name;
				char *version;
				os_port_entry *osp;
				
				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_port=0", port_count);
				add_name_to_buf(buf, buf2, port_buffer);
				port = gnome_config_get_int_with_default(buf2,&def);
				
				if(def)
					break;

				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_protocol=0", port_count);
				add_name_to_buf(buf, buf2, port_buffer);
				protocol = gnome_config_get_int_with_default(buf2,&def);
				
				if(def)
					break;

				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_state=%d", port_count, PORT_STATE_OPEN);
				add_name_to_buf(buf, buf2, port_buffer);
				state = gnome_config_get_int_with_default(buf2,&def);

				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_proto=0", port_count);
				add_name_to_buf(buf, buf2, port_buffer);
				proto = gnome_config_get_int_with_default(buf2,&def);
				
				if(def)
					state = PORT_PROTO_NONE;

				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_rpcnum=0", port_count);
				add_name_to_buf(buf, buf2, port_buffer);
				rpcnum = gnome_config_get_int_with_default(buf2,&def);

				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_rpclowver=0", port_count);
				add_name_to_buf(buf, buf2, port_buffer);
				rpclowver = gnome_config_get_int_with_default(buf2,&def);

				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_rpchighver=0", port_count);
				add_name_to_buf(buf, buf2, port_buffer);
				rpchighver = gnome_config_get_int_with_default(buf2,&def);

				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_name=BLAH", port_count);
				add_name_to_buf(buf, buf2, port_buffer);
				name = gnome_config_get_string_with_default(buf2,&def);
				
				if(def)
				{
					free(name);
					name = NULL;
				}

				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_version=BLAH", port_count);
				add_name_to_buf(buf, buf2, port_buffer);
				version = gnome_config_get_string_with_default(buf2,&def);
				
				if(def)
				{
					free(version);
					version = NULL;
				}
				
				snprintf(port_buffer, sizeof(port_buffer), "_port_%d_owner=BLAH", port_count);
				add_name_to_buf(buf, buf2, port_buffer);
				owner = gnome_config_get_string_with_default(buf2,&def);
				
				if(def)
				{
					free(owner);
					owner = NULL;
				}

				osp = malloc(sizeof(*osp));
				memset(osp, 0, sizeof(*osp));
				osp->name = name;
				osp->version = version;
				osp->port = port;
				osp->protocol = protocol;
				osp->next = NULL;
				osp->state = state;
				osp->proto = proto;
				osp->rpcnum = rpcnum;
				osp->rpclowver = rpclowver;
				osp->rpchighver = rpchighver;
				osp->owner = owner;
				
				if(os_data == NULL)
				{
					os_data = malloc(sizeof(*os_data));
					memset(os_data, 0, sizeof(*os_data));
					os_data->os = NULL;
					os_data->ports = NULL;
				}

				// snazzy way to reverse the order
				if(current == NULL)
				{
					os_data->ports = osp;
					current = osp;
				}
				else
				{
					current->next = osp;
					current = osp;
				}
			}

			po = add_discovered_node(np->agent, np, str2ip(po_ip), po_name, os_data, FALSE); 
			free(po_ip);
			
			if(!po)
				break;

			po->x = x;
			po->y = y;
			if(po->icon_file_name)
				free(po->icon_file_name);
			po->icon_file_name = icon_file_name;
			if(po->notes)
				free(po->notes);
			po->notes = notes;
			po->flags = flags;			

// make sure the canvas updates everything
			page_object_display(po);
			page_object_display(po);
		}

		// set all of the merged hosts
		sprintf(base,"=%s=/viewspace%d",filename,i);
		strcat(base,"/merge_");
		for(j = 0;; j++)
		{
			page_object *primary, *secondary;
			char *string1;
			char *string2;
			struct in_addr addr;
			int jj;
						
			sprintf(buf, "%s%d",base,j);
			
			add_name_to_buf(buf, buf2, "_primary=NOTHING");
			string1 = gnome_config_get_string_with_default(buf2,&def);
			
			if(def || 
			   !inet_aton(string1, &addr) || 
			   ((primary = page_object_get_by_ip(np, addr.s_addr)) == NULL))
			{
				free(string1);
				break;
			}
			
			for(jj = 0;; jj++)
			{
				sprintf(buf, "%s%d_secondary_%d=NOTHING", base, j, jj);
				string2 = gnome_config_get_string_with_default(buf, &def);
				
				if(def || 
				   !inet_aton(string2, &addr) ||
				   ((secondary = page_object_get_by_ip(np, addr.s_addr)) == NULL))
				{
					free(string2);
					break;
				}
				
				page_object_merge(primary, secondary);
				free(string2);
			}
			
			free(string1);
		}

		// we have to read the page_object_links after we do the merged hosts
		sprintf(base,"=%s=/viewspace%d",filename,i);
		strcat(base,"/page_object_link_");

		for(j = 0;; j++)
		{
			page_object *po1, *po2;
			char *string1;
			char *string2;
			struct in_addr addr;
			
			sprintf(buf, "%s%d",base,j);
			
			add_name_to_buf(buf, buf2, "_ip1=NOTHING");
			string1 = gnome_config_get_string_with_default(buf2,&def);
			
			if(def)
			{
				free(string1);
				break;
			}

			add_name_to_buf(buf, buf2, "_ip2=NOTHING");
			string2 = gnome_config_get_string_with_default(buf2,&def);

			if(def)
			{
				free(string1);
				free(string2);
				break;
			}

			if(inet_aton(string1, &addr) && 
			   (po1 = page_object_get_by_ip(np, addr.s_addr)) )
			{
				if(inet_aton(string2, &addr) && 
				   (po2 = page_object_get_by_ip(np, addr.s_addr)) )
				{
					net_page_map_page_objects(np, po1, po2);
				}
			}
			free(string1);
			free(string2);
		}
	}
}

char *page_object_convert_notes(page_object *po)
{
	char *string;
	int len = strlen(po->notes);
	int i,count, index;
	
	for(i=0,count=0;i<len;i++)
	{
		if(po->notes[i] == '\n')
			count++;
	}
	
	if( (string = malloc(strlen(po->notes) + 1 + count*3)) == NULL)
	{
		printf("\n bad error\n");
		exit(1);
	}
	
	if(count == 0)
	{
		strcpy(string, po->notes);
		return(string);
	}
	
	index = 0;
	for(i = 0; i <= len; i++)
	{
		if((string[i + index] = po->notes[i]) == '\n')
		{
			string[i+ index] = '%';
			string[i+ index + 1] = '0';
			string[i+ index + 2] = 'a';
			index +=2;
		}
	}
	return(string);
}	

void write_config_file(GtkWidget *w, GtkWidget *fs)
{
	char buf[CONFIG_BUFFER_SIZE], buf2[CONFIG_BUFFER_SIZE], base[CONFIG_BUFFER_SIZE], *filename;
	net_page *np;
	page_object *po;
	int i=0,j=0;
	os_pixmap_list_t *pos;
	service_list_t *srv;
	GList *gl;
	
	if(fs != NULL)
	{
		filename = makestring(gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs)));

		if(main_window->filename)
			free(main_window->filename);
			
		main_window->filename = filename;
	
		gtk_widget_destroy(fs);
	}
	else
	{
		filename = main_window->filename;
	}
	
	/* clear out the destination file if it is there */
	sprintf(base,"=%s=",filename);
	/* 
	 * for some reason the clean_file does not get rid of all data
	 * so i have to do this stupid loop
	 */
	i = 0;
	while(1)
	{
		sprintf(buf, "=%s=/viewspace%d", filename, i);
		if(gnome_config_has_section(buf))
			gnome_config_clean_section(buf);
		else
			break;
		i++;
	}
	gnome_config_clean_file(base);
	gnome_config_sync();
	
	sprintf(buf,"=%s=/global/options_rescan_at_startup",filename);
	gnome_config_set_int(buf,options_rescan_at_startup);

	sprintf(buf,"=%s=/global/options_discover_retries",filename);
	gnome_config_set_int(buf,options_discover_retries);

	sprintf(buf,"=%s=/global/options_operating_system_check",filename);
	gnome_config_set_int(buf,options_operating_system_check);

	sprintf(buf,"=%s=/global/options_confirm_delete",filename);
	gnome_config_set_int(buf,options_confirm_delete);

	sprintf(buf,"=%s=/global/options_reverse_dns",filename);
	gnome_config_set_int(buf,options_reverse_dns);

	sprintf(buf,"=%s=/global/options_save_changes_on_exit",filename);
	gnome_config_set_int(buf,options_save_changes_on_exit);

	sprintf(buf,"=%s=/global/options_use_ip_for_label",filename);
	gnome_config_set_int(buf,options_use_ip_for_label);

	sprintf(buf,"=%s=/global/options_only_display_hostname",filename);
	gnome_config_set_int(buf,options_only_display_hostname);

	sprintf(buf,"=%s=/global/options_move_stuff_live",filename);
	gnome_config_set_int(buf,options_move_stuff_live);

	sprintf(buf,"=%s=/global/options_probe_ports",filename);
	gnome_config_set_int(buf,options_probe_ports);

	sprintf(buf,"=%s=/global/options_map",filename);
	gnome_config_set_int(buf,options_map);

	sprintf(buf,"=%s=/global/options_os_scan_ports",filename);
	gnome_config_set_string(buf,options_os_scan_ports);

	sprintf(buf,"=%s=/global/options_os_scan_tcp_scan",filename);
	gnome_config_set_string(buf,options_os_scan_tcp_scan);

	sprintf(buf,"=%s=/global/options_os_scan_timing",filename);
	gnome_config_set_string(buf,options_os_scan_timing);

	sprintf(buf,"=%s=/global/options_os_scan_fastscan",filename);
	gnome_config_set_int(buf,options_os_scan_fastscan);

	sprintf(buf,"=%s=/global/options_os_scan_udp_scan",filename);
	gnome_config_set_int(buf,options_os_scan_udp_scan);

	sprintf(buf,"=%s=/global/options_os_scan_identd_scan",filename);
	gnome_config_set_int(buf,options_os_scan_identd_scan);

	sprintf(buf,"=%s=/global/options_os_scan_rpc_scan",filename);
	gnome_config_set_int(buf,options_os_scan_rpc_scan);

	sprintf(buf,"=%s=/global/options_os_scan_osscan",filename);
	gnome_config_set_int(buf,options_os_scan_osscan);

	sprintf(buf,"=%s=/global/options_os_scan_dont_ping",filename);
	gnome_config_set_int(buf,options_os_scan_dont_ping);

	sprintf(buf,"=%s=/global/options_os_scan_scan_specified_ports",filename);
	gnome_config_set_int(buf,options_os_scan_scan_specified_ports);

	sprintf(buf,"=%s=/global/options_use_ip_for_merged_ports",filename);
	gnome_config_set_int(buf,options_use_ip_for_merged_ports);

//	sprintf(buf,"=%s=/global/tooltips_timeout",filename);
//	gnome_config_set_int(buf,options_tooltips_timeout);

	sprintf(buf,"=%s=/global/width",filename);
	gnome_config_set_int(buf,main_window->window->allocation.width);

	sprintf(buf,"=%s=/global/height",filename);
	gnome_config_set_int(buf,main_window->window->allocation.height);

/*
 * These are all of the strings to match OS names with
 */
 	for(i = 0, pos = os_pixmap_list; pos; pos = pos->next, i++)
 	{
		sprintf(buf,"=%s=/global/fingerprint_string%d",filename,i);
		gnome_config_set_string(buf,pos->string);
			
		sprintf(buf,"=%s=/global/fingerprint_pixmap%d",filename,i);
		gnome_config_set_string(buf,pos->pixmap_name);
	}
/*
 * These are all of the services and their commands
 */
 	for(i = 0, srv = service_list; srv; srv = srv->next, i++)
 	{
		sprintf(buf,"=%s=/global/service%d_name",filename,i);
		gnome_config_set_string(buf,srv->name);

		sprintf(buf,"=%s=/global/service%d_port",filename,i);
		gnome_config_set_int(buf,srv->port);
			
		sprintf(buf,"=%s=/global/service%d_protocol",filename,i);
		gnome_config_set_int(buf,srv->protocol);

		sprintf(buf,"=%s=/global/service%d_command",filename,i);
		gnome_config_set_string(buf,srv->command);
	}
	
/*
 * These are all of the scripts and their commands
 */
 	for(i = 0, gl = script_get_list(); gl; gl = g_list_next(gl), i++)
 	{
 		script_t *script = (script_t *)gl->data;
 		
		sprintf(buf,"=%s=/global/script%d_name",filename,i);
		gnome_config_set_string(buf,script->name);

		sprintf(buf,"=%s=/global/script%d_command",filename,i);
		gnome_config_set_string(buf,script->script);

		sprintf(buf,"=%s=/global/script%d_flags",filename,i);
		gnome_config_set_int(buf,script->flags);
	}

	for(i=0, np = main_window->net_pages; np; np = np->next, i++)
	{
		sprintf(base,"=%s=/viewspace%d",filename,i);
		
		sprintf(buf,"%s%s",base, "/name");
		gnome_config_set_string(buf,np->name);
		
		sprintf(buf,"%s%s",base, "/agent_ip");
		gnome_config_set_string(buf,np->agent_ip);
		
		sprintf(buf,"%s%s",base, "/usessl");
		gnome_config_set_int(buf,(np->agent->flags & AGENT_FLAGS_USING_SSL?1:0));
		
		/* add the reference to the page_objects in the netpage sections */
		strcat(base,"/page_object_");

		for(j = 0, po = np->page_objects; po; po = po->next, j++)
		{
			char *string;
			
			sprintf(buf, "%s%d",base,j);

			add_name_to_buf(buf, buf2, "_ip");
			gnome_config_set_string(buf2,ip2str(po->ip));

			add_name_to_buf(buf, buf2, "_name");
			gnome_config_set_string(buf2,po->name);

			add_name_to_buf(buf, buf2, "_x");
			gnome_config_set_float(buf2,po->x);
			
			add_name_to_buf(buf, buf2, "_y");
			gnome_config_set_float(buf2,po->y);

			if(po->icon_file_name)
			{
				add_name_to_buf(buf, buf2, "_icon_file_name");
				gnome_config_set_string(buf2,po->icon_file_name);
			}
			
			if(po->os_data)
			{
				if(po->os_data->os)
				{
					os_port_entry *osp;
					char port_buffer[1024];
					int port_count;
					
					add_name_to_buf(buf, buf2, "_os_name");
					gnome_config_set_string(buf2,po->os_data->os);
					
					for(osp = po->os_data->ports, port_count = 0; osp; osp = osp->next, port_count++)
					{
						snprintf(port_buffer, sizeof(port_buffer), "_port_%d_port", port_count);
						add_name_to_buf(buf, buf2, port_buffer);
						gnome_config_set_int(buf2,osp->port);

						snprintf(port_buffer, sizeof(port_buffer), "_port_%d_protocol", port_count);
						add_name_to_buf(buf, buf2, port_buffer);
						gnome_config_set_int(buf2,osp->protocol);

						snprintf(port_buffer, sizeof(port_buffer), "_port_%d_state", port_count);
						add_name_to_buf(buf, buf2, port_buffer);
						gnome_config_set_int(buf2,osp->state);

						snprintf(port_buffer, sizeof(port_buffer), "_port_%d_proto", port_count);
						add_name_to_buf(buf, buf2, port_buffer);
						gnome_config_set_int(buf2,osp->proto);

						snprintf(port_buffer, sizeof(port_buffer), "_port_%d_rpcnum", port_count);
						add_name_to_buf(buf, buf2, port_buffer);
						gnome_config_set_int(buf2,osp->rpcnum);

						snprintf(port_buffer, sizeof(port_buffer), "_port_%d_rpclowver", port_count);
						add_name_to_buf(buf, buf2, port_buffer);
						gnome_config_set_int(buf2,osp->rpclowver);

						snprintf(port_buffer, sizeof(port_buffer), "_port_%d_rpchighver", port_count);
						add_name_to_buf(buf, buf2, port_buffer);
						gnome_config_set_int(buf2,osp->rpchighver);

						if(osp->name)
						{
							snprintf(port_buffer, sizeof(port_buffer), "_port_%d_name", port_count);
							add_name_to_buf(buf, buf2, port_buffer);
							gnome_config_set_string(buf2,osp->name);
						}

						if(osp->version)
						{
							snprintf(port_buffer, sizeof(port_buffer), "_port_%d_version", port_count);
							add_name_to_buf(buf, buf2, port_buffer);
							gnome_config_set_string(buf2,osp->version);
						}

						if(osp->owner)
						{
							snprintf(port_buffer, sizeof(port_buffer), "_port_%d_owner", port_count);
							add_name_to_buf(buf, buf2, port_buffer);
							gnome_config_set_string(buf2,osp->owner);
						}
					}
				}
			}

// There is no need to get the flags right now			
//			add_name_to_buf(buf, buf2, "_flags");
//			gnome_config_set_int(buf2,po->flags);
			
			if(po->notes)
			{
				string = page_object_convert_notes(po);
				add_name_to_buf(buf, buf2, "_notes");
				gnome_config_set_string(buf2,string);
				free(string);
			}
		}

		/*
		 * now to fool with the ICMP Mapping
		 */
		sprintf(base,"=%s=/viewspace%d",filename,i);
		
		sprintf(buf,"%s%s",base, "/name");
		gnome_config_set_string(buf,np->name);

		/*
		 * now to fool with the merged hosts
		 */
		sprintf(base,"=%s=/viewspace%d",filename,i);
		strcat(base,"/merge_");

		for(j = 0, po = np->page_objects; po; po = po->next)
		{
			page_object *pri;
			int counter = 0;
			int didsomething = FALSE;
			
			if(po->merge && po == po->merge_primary)
			{
				sprintf(buf, "%s%d",base,j);
				add_name_to_buf(buf, buf2, "_primary");
				
				for(pri = po; pri; pri = pri->merge)
				{
					sprintf(buf, "%s%d_secondary_%d",base,j,counter);
					gnome_config_set_string(buf,ip2str(pri->ip));
					didsomething = TRUE;
				}
				
				if(didsomething)
				{
					//only add something if we have at least a primary and a secondary
					gnome_config_set_string(buf2,ip2str(po->ip));
					j++;
				}
				
			}
		}
		
		/* add the reference to the page_objects in the netpage sections */
		sprintf(base,"=%s=/viewspace%d",filename,i);
		strcat(base,"/page_object_link_");

		for(j = 0, po = np->page_objects; po; po = po->next)
		{
			page_object_link *pol;
			GList *gl;
			
			for(gl = po->links; gl; gl = gl->next)
			{
				pol = gl->data;

				sprintf(buf, "%s%d",base,j);

				add_name_to_buf(buf, buf2, "_ip1");
				gnome_config_set_string(buf2,ip2str(pol->po1->ip));

				add_name_to_buf(buf, buf2, "_ip2");
				gnome_config_set_string(buf2,ip2str(pol->po2->ip));

				j++;
			}
		}
	}
	
	gnome_config_sync();	
}


