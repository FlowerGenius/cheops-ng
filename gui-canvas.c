/*
 * Cheops Next Generation GUI
 * 
 * gui-canvas.c
 * Functions for the gnome canvas and the page_object's that are on the canvas
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <gnome.h>
#include <netdb.h>
#include "cheops-gui.h"
#include "gui-viewspace.h"
#include "gui-canvas.h"
#include "gui-utils.h"
#include "gui-settings.h"
#include "gui-config.h"
#include "gui-service.h"
#include "cheops-osscan.h"
#include "ip_utils.h"
#include "logger.h"
#include "gui-dns.h"
#include "gui-pixmap.h"
#include "gui-handlers.h"
#include "gui-dns.h"
#include "script.h"
#ifdef USING_MONITORING
	#include "gui-monitoring.h"
#endif
//#define DEBUG_CANVAS

#ifdef DEBUG_CANVAS
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

//#define DEBUG_CANVAS_BIGTIME

#ifdef DEBUG_CANVAS_BIGTIME
	#define DEBUG_BIGTIME(a) a
#else
	#define DEBUG_BIGTIME(a)
#endif



typedef struct _poo_info {
	GtkWidget   *menu;
	page_object *po;
	net_page    *np;
	int          count;
} poo_info;

void set_widget_tooltips(GtkWidget *w, char *text);
int page_object_event_dispatch(GtkObject *widget, GdkEvent *event, page_object *po);
struct servent *nmap_getservbyport (int port, const char *proto);


double mouse_down_x, mouse_down_y;

GList *page_object_option_list;

extern int control_key;


#define FOR_EACH_DO(function_name, pointer, struct_name, next_name, argument) {\
	struct_name *FOR_EACH_DO_temp;                                             \
	for(FOR_EACH_DO_temp = pointer; FOR_EACH_DO_temp; FOR_EACH_DO_temp = FOR_EACH_DO_temp->next_name) \
		function_name (FOR_EACH_DO_temp, argument);                              \
}


void deleting_group(GtkWidget *w, page_object *po)
{
	po->group = NULL;
}

void page_object_remove_group(page_object *po)
{
	if(po == NULL)
		return;
		
	if(po->group)
	{
		gtk_object_destroy(GTK_OBJECT(po->group));
		po->group = NULL;
		if(po->im)
		{
			gdk_imlib_kill_image(po->im);
			po->im = NULL;
		}

	}
}

void page_object_create_group(page_object *po)
{
	if(po == NULL || po->canvas == NULL)
	{
		c_log(LOG_ERROR,"page_object was null\n");
		exit(1);
	}
		
	page_object_remove_group(po);
			
	po->group = gnome_canvas_item_new( gnome_canvas_root(po->canvas),
					  GNOME_TYPE_CANVAS_GROUP, NULL );
	po->box =   gnome_canvas_item_new( GNOME_CANVAS_GROUP(po->group),
					  GNOME_TYPE_CANVAS_RECT,
					  "fill_color", "white",
					  "outline_color", "black",
					  NULL );
	po->label = gnome_canvas_item_new( GNOME_CANVAS_GROUP(po->group),
					  GNOME_TYPE_CANVAS_TEXT,
					  "font", "fixed",
					  "fill_color", "black",
					  "anchor", GTK_ANCHOR_N,
					  NULL );
	po->icon  = gnome_canvas_item_new( GNOME_CANVAS_GROUP(po->group),
					  gnome_canvas_image_get_type(),
					  "anchor", GTK_ANCHOR_S,
					  NULL );

	// put a signal on the goup so we dont try to free it twice
	gtk_signal_connect(GTK_OBJECT(po->group), "destroy", deleting_group, po);

	gtk_signal_connect( GTK_OBJECT(po->group), "event", GTK_SIGNAL_FUNC(page_object_event_dispatch), po);
}

/*
 * Create a new page_abject with all fields avaliable for use
 */
page_object *page_object_new(GnomeCanvas *canvas)
{
	page_object *po;
	
	g_return_val_if_fail( canvas, NULL);
	g_return_val_if_fail( po = malloc(sizeof(page_object)), NULL );

	memset(po, 0, sizeof(page_object));

	po->flags = PAGE_OBJECT_CREATED_NEW;
	po->icon_file_name = makestring("unknown.xpm");
	po->canvas = canvas;
//	po->tooltips = gtk_tooltips_new();
//	gtk_tooltips_set_delay(po->tooltips, tooltips_timeout);
	
	page_object_create_group(po);
	
	return(po);
}

#define INITIAL_Y 50.0
#define INITIAL_X 40.0

char *page_object_get_name(page_object *po)
{
	static char *hold = NULL;
	char *name;
	
	if(hold)
		g_free(hold);
	hold = NULL;
	
	if(po)
	{
		if(po->name == NULL)
			name = ip2str(po->ip);
		else
		{
			if(options_use_ip_for_label)
				name = ip2str(po->ip);
			else
			{
				if(options_only_display_hostname)
				{
					hold = g_strdup(po->name);
					if( (name = strchr(hold, '.')) )
					{
						*name = '\0';
					}
					name = hold;
				}
				else
				{
					name = po->name;
				}
			}
		}
	}
	else
		name = "Unknown";
	return(name);
}	

/*
 * Display the page object, give it a pixmap, and put it on the canvas
 */
void page_object_display( page_object *po )
{
	double x1, x2, y1, y2;
	double nextx = 0.0;
	double nexty = INITIAL_Y;
	double height, width;
	double nextydown = 0.0;
	double tempx, tempy, tempwidth, tempheight;
	char *name;

	g_return_if_fail( po );

	// correct for the restore from config (the ip address is used as the name)
	if(po->name)
	{
		struct in_addr address;
		if(inet_aton(po->name, &address))
		{
			if(po->ip == address.s_addr)
			{
				free(po->name);
				po->name = NULL;
			}
		}
	}

	if(po->merge_primary != NULL && po->merge_primary != po)
	{
		if(po->group)
		{
			page_object_highlight_unhighlight(po);
			page_object_remove_group(po);
		}
		return;
	}
	
	page_object_create_group(po);
	
/*
 * add the text and the text box (which is 2 units on each side fatter than the
 * text	is)
 */
	name = page_object_get_name(po);
	gnome_canvas_item_set(GNOME_CANVAS_ITEM(po->label), "text", name, NULL);

	gtk_object_get(GTK_OBJECT(po->label), "x", &tempx,
	                                      "y", &tempy,
	                                      "text_height", &tempheight,
	                                      "text_width", &tempwidth,
	                                      NULL);
	
	gnome_canvas_item_set(GNOME_CANVAS_ITEM(po->box), 
	                      "x1", (tempx - tempwidth/2.0)-2.0, 
	                      "x2", (tempx + tempwidth/2.0)+2.0, 
	                      "y1", (tempy)-2.0, 
	                      "y2", (tempy + tempheight)+2.0, NULL);

/*
 * get rid of the image, cause we are going to redisplay it
 */ 
	if(po->im)
	{
		gdk_imlib_kill_image(po->im);
		po->im = NULL;
	}

	if(po->os_data && po->os_data->os)
	{
		if(po->icon_file_name)
			free(po->icon_file_name);
		
		po->icon_file_name = makestring(os_pixmap_list_get(po->os_data->os));	
	}
	po->im = get_my_imlib_image(po->icon_file_name);

	gnome_canvas_item_set(GNOME_CANVAS_ITEM(po->icon), 
	                      "image", po->im, 
	                      "width", (double)po->im->rgb_width, 
	                      "height", (double)po->im->rgb_height, NULL);

//	gtk_tooltips_set_tip(po->tooltips, GTK_WIDGET(main_window->notebook), "this is a tip", NULL);
		
/*
 * if the page_object is new then place it next to the last created page_object
 */
	if(po->flags & PAGE_OBJECT_CREATED_NEW)
	{
		gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(po->group), 
		                             &x1, &y1, &x2, &y2);
		height = y2 - y1;
		width = x2 - x1;
	
		if(po->next != NULL)
		{
			nextx = po->next->x + width;
			nexty = po->next->y;
			nextydown = height;
		}
		po->x = INITIAL_X + nextx;
		po->y = nexty;
	
		if((po->x + width) >= CANVAS_FIXED_WIDTH)
		{
			po->x = INITIAL_X;
			po->y += INITIAL_Y + nextydown;
		}
		if((po->y + height) >= CANVAS_FIXED_HEIGHT)
		{
			po->x = INITIAL_X;
			po->y = INITIAL_Y;
		}
		po->flags &= ~PAGE_OBJECT_CREATED_NEW;

	}
	gnome_canvas_item_set(GNOME_CANVAS_ITEM(po->group), 
	                      "x", (double)po->x, 
	                      "y", (double)po->y, NULL); 	
	
/*
 * This bit of code will fix the time when the border of the page_object
 * extends off of the side of the screen
 */
	gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(po->group), &x1, &y1, &x2, &y2);
	DEBUG_BIGTIME(printf("bounds:\nx1 = %f\ny1 = %f\nx2 = %f\ny2 = %f\n",x1, y1, x2, y2));

	if(x1 < 0)
		po->x -= x1;
	if(x2 > CANVAS_FIXED_WIDTH)
		po->x -= x2 - CANVAS_FIXED_WIDTH;
	if(y1 < 0)
		po->y -= y1;
	if(y2 > CANVAS_FIXED_HEIGHT)
		po->y -= y2 - CANVAS_FIXED_HEIGHT;

	gnome_canvas_item_set(GNOME_CANVAS_ITEM(po->group), 
	                      "x", po->x, 
	                      "y", po->y, NULL); 	

	page_object_highlight_set(po);

	page_object_links_move(po->links, po);
}

void page_object_free_os_data(page_object *po)
{
	os_port_entry	*os_port, *temp;

	if(po && po->os_data)
	{
		if(po->os_data->os)
			free(po->os_data->os);

		if(po->os_data->uptime_last_boot)
			free(po->os_data->uptime_last_boot);
		
		for(os_port = po->os_data->ports; os_port;)
		{
			temp = os_port->next;
			
			if(os_port->name)
				free(os_port->name);
			if(os_port->version)
				free(os_port->version);
			if(os_port->owner)
				free(os_port->owner);
			free(os_port);
			
			os_port = temp;
		}
		
		free(po->os_data);
		po->os_data = NULL;
	}
}


void page_object_set_merge_primary(net_page *np, page_object *primary, page_object *new_primary)
{
	page_object *p;
	if(is_valid_net_page(np))
	{
		/*
		 * make sure that the page objects exist on the netpage
		 * and make sure that the new_primary is not already the primary
		 */
		if(is_valid_page_object(np, primary) && 
		   is_valid_page_object(np, new_primary) &&
		   new_primary->merge_primary != new_primary)
		{
			for(p = primary; p; p = p->merge)
			{
				if(p == new_primary)
				{
					page_object_unmerge(new_primary);
					page_object_merge(new_primary, primary);
					break;
				}
			}
		}
	}
}

page_object_link *page_object_link_new()
{
	page_object_link *pol = malloc(sizeof(*pol));
	
	if(pol)
	{
		pol->link = NULL;
		pol->po1 = NULL;
		pol->po2 = NULL;
		pol->merge1 = NULL;
		pol->merge2 = NULL;
		pol->pts = gnome_canvas_points_new(2);
	}
	return(pol);
}

void page_object_link_destroy(page_object_link *pol, page_object *po)
{
	if(pol)
	{
		if(!pol->link || 
		   !pol->po1 ||
		   !pol->po2 ||
		   !pol->pts)
		{
			g_error("oops... something bad has happened with this page_object_link");
		}
		else
		{
			/*
			 * take the GList node out of the other page_object (cause we delete the 
			 * po->link list after the g_list_foreach()  )
			 */
			if(pol->merge1)
			{
				if(po != pol->merge1)
				{
					pol->merge1->links = g_list_remove(pol->merge1->links, pol);
				}
			}
			else
			{
				if(po != pol->po1)
				{
					pol->po1->links = g_list_remove(pol->po1->links, pol);
				}
			}

			if(pol->merge2)
			{
				if(po != pol->merge2)
				{
					pol->merge2->links = g_list_remove(pol->merge2->links, pol);
				}
			}
			else
			{
				if(po != pol->po2)
				{
					pol->po2->links = g_list_remove(pol->po2->links, pol);
				}
			}
			
			gnome_canvas_points_free(pol->pts);
			gtk_object_destroy(GTK_OBJECT(pol->link));
			
			free(pol);
		}
	}	
}

/*
 * Function used to see if there is already a connection from one page_object
 * to another page_object (it returns the page_object_link)
 */
page_object_link *page_object_link_get(page_object *po1, page_object *po2)
{
	GList *gl;
	page_object_link *pol = NULL;
	page_object *p1, *p2;
	
	if(po1 && po2)
	{
		// we have to look on the primary's list cause that is where merged
		// links exist
		p1 = (po1->merge_primary ? po1->merge_primary : po1);
		p2 = (po2->merge_primary ? po2->merge_primary : po2);
		for(gl = p1->links; gl; gl = gl->next) 
		{
			pol = gl->data;

			if( (pol->po1 == po1 && pol->po2 == po2) || 
			    (pol->po1 == po2 && pol->po2 == po1))
			    break;
			pol = NULL;
		}
		if(!pol)
		{
			for(gl = p2->links; gl; gl = gl->next) 
			{
				pol = gl->data;
	
				if( (pol->po1 == po1 && pol->po2 == po2) || (pol->po1 == po2 && pol->po2 == po1) )
				    break;
				pol = NULL;
			}
		}
	}
	return(pol);
}

void net_page_set_pixels_per_unit(net_page *np, double ppi)
{
	page_object *po;
	double x, y;
	
	if(is_valid_net_page(np))
	{
		np->pixels_per_unit = ppi;
		gnome_canvas_set_pixels_per_unit(GNOME_CANVAS(np->canvas), np->pixels_per_unit);

		gnome_canvas_world_to_window(GNOME_CANVAS(np->canvas),
		                             CANVAS_FIXED_WIDTH, CANVAS_FIXED_HEIGHT,
		                             &x, &y);
		gnome_canvas_set_scroll_region(GNOME_CANVAS(np->canvas), 0, 0, x, y);

		for(po = np->page_objects; po; po = po->next)
			page_object_display(po);
		
	}
}

void net_page_zoom_normal(net_page *np)
{
	if(is_valid_net_page(np))
	{
		net_page_set_pixels_per_unit(np, 1.0);
	}
}

void net_page_zoom_in(net_page *np)
{
	if(is_valid_net_page(np))
	{
		net_page_set_pixels_per_unit(np, np->pixels_per_unit + 0.1);
	}
}

void net_page_zoom_out(net_page *np)
{
	if(is_valid_net_page(np))
	{
		net_page_set_pixels_per_unit(np, np->pixels_per_unit - 0.1);
	}
}

	
void net_page_map_all(net_page *np)
{
	page_object *po;
	
	for(po = np->page_objects; po; po = po->next)
		do_map_icmp(np->agent, po->ip, np);
}

void page_object_map(net_page *np, page_object *po)
{
	if(is_valid_net_page(np))
		if(is_valid_page_object(np, po))
			do_map_icmp(np->agent, po->ip, np);
}

void net_page_map_plink(GtkWidget *w, gpointer data)
{
	net_page *np = data;
	page_object *po;
	
	if(is_valid_net_page(np))
		for(po = np->plink; po; po = po->plink)
			do_map_icmp(np->agent, po->ip, np);
}

void net_page_osscan_plink(GtkWidget *w, gpointer data)
{
	net_page *np = data;
	page_object *po;

	if(is_valid_net_page(np))
	{
		for(po = np->plink; po; po = po->plink)
		{
			do_os_scan(np->agent, po->ip, np, 0);
		}
	}
}

void net_page_probe_plink(GtkWidget *w, gpointer data)
{
	net_page *np = data;
	page_object *po;
	os_port_entry *port;
	int i;
	
	if(is_valid_net_page(np))
	{
		for(po = np->plink; po; po = po->plink)
		{
			if(po->os_data)
			{
				for(port = po->os_data->ports; port; port = port->next)
				{
					do_probe(np->agent, po->ip, port->port, probe_timeout_ms, np);
				}
			}
			else
			{
				for(i = 0; i < 100; i++)
				{
					do_probe(np->agent, po->ip, i, probe_timeout_ms, np);
				}
			}
		}
	}
}

void net_page_dns_plink(GtkWidget *w, gpointer data)
{
	net_page *np = data;
	page_object *po;
	
	if(is_valid_net_page(np))
		for(po = np->plink; po; po = po->plink)
			dns_reverse_lookup( inet_ntoa(*(struct in_addr *)&(po->ip)), reverse_lookup_cb, NULL, po);
}

void page_object_unmerge(page_object *p)
{
	page_object *po, *next, *prev;
	page_object_link *pol;
	int current;
	
	if(p->merge_primary == p)
	{
		// we are unmerging the primary
		for(po = p->merge; po; po = next)
		{
			next = po->merge;
			
			po->merge = NULL;
			po->merge_primary = NULL;
			
			current = 0;
			while((pol = g_list_nth_data(p->links, current++)) )
			{
				/*
				 * check the po that we are unmerging to see if
				 * it already has its reference to the pol
				 */
				if(!g_list_find(po->links, pol))
				{
					if(pol->po1 == po)
					{
						p->links = g_list_remove(p->links, pol);
						po->links = g_list_append(po->links, pol);
						pol->merge1 = NULL;
						current = 0;
					}
					else
					{
						if(pol->po2 == po)
						{
							p->links = g_list_remove(p->links, pol);
							po->links = g_list_append(po->links, pol);
							pol->merge2 = NULL;
							current = 0;
						}
					}
				}
			}
			page_object_display(po);
			page_object_links_move(po->links, NULL);
		}
		p->merge_primary = NULL;
		p->merge = NULL;
		page_object_display(p);
	}
	else
	{
		// we are unmerging a secndary
		if(p->merge_primary)
		{
			// to make it jive with the code above
			po = p;
			p = p->merge_primary;
			
			current = 0;
			while((pol = g_list_nth_data(p->links, current++)) )
			{
				/*
				 * check the po that we are unmerging to see if
				 * it already has its reference to the pol
				 */
				if(!g_list_find(po->links, pol))
				{
					if(pol->po1 == po)
					{
						p->links = g_list_remove(p->links, pol);
						po->links = g_list_append(po->links, pol);
						pol->merge1 = NULL;
						current = 0;
					}
					else
					{
						if(pol->po2 == po)
						{
							p->links = g_list_remove(p->links, pol);
							po->links = g_list_append(po->links, pol);
							pol->merge2 = NULL;
							current = 0;
						}
					}
				}
			}
			// take the secondary out of the list of merged page_objects
			prev = NULL;
			for(next = p->merge; next; next = next->merge)
			{
				if(next == po)
				{
					if(prev)
						prev->merge = po->merge;
					else
						p->merge = po->merge;
				}
				prev = next;
			}
			if(p->merge == NULL)
				p->merge_primary = NULL;
			
			po->merge_primary = NULL;
			po->merge = NULL;
			
			page_object_display(po);
			page_object_display(p);
			page_object_links_move(po->links, NULL);
		}
		else
		{
			printf("%s(): ERROR!! trying to unmerge a page_object that is not merged!!\n", __FUNCTION__);
		}
		
	}
}

page_object *page_object_get_last_merge(page_object *po)
{
	page_object *p;
	
	if(po == NULL)
		return(NULL);  // should never do this!!!
		
	for(p = po->merge; p && p->merge; p = p->merge)
		;
		
	return(p);
}

void page_object_merge(page_object *primary, page_object *secondary)
{
	page_object *po;
	page_object *last;
	page_object_link *pol;
	GList *gl;
	
	if(primary && secondary)
	{
		primary->merge_primary = primary;
		
		if(secondary->merge_primary)
		{
			//this one is already merged
			if(secondary->merge_primary == secondary)
			{
				//this one is the primary, so move all of the links to the new primary
				for(gl = secondary->links; gl; gl = gl->next)
				{
					pol = gl->data;
					if(pol->po1 == secondary)
					{
						pol->merge1 = primary;
					}
					if(pol->po2 == secondary)
					{
						pol->merge2 = primary;
					}
					if(pol->merge1 == secondary)
					{
						pol->merge1 = primary;
					}
					if(pol->merge2 == secondary)
					{
						pol->merge2 = primary;
					}
				}
				
				// add the new page object links to the primary, and take them away from the secondary
				primary->links = g_list_concat(primary->links, secondary->links);
				secondary->links = NULL;
				
				if( (last=page_object_get_last_merge(primary)) )
				{
					last->merge = secondary;
				}
				else
				{
					// the primary has no merges
					primary->merge = secondary;
				}
				secondary->merge_primary = primary;
				for(po = secondary->merge; po; po = po->merge)
					po->merge_primary = primary;
			}
			else
			{
				//this one is a merge ! the primary, so move the merge_primary's links to 
				//the new primary's
				for(gl = secondary->merge_primary->links; gl; gl = gl->next)
				{
					pol = gl->data;
					
					if(pol->po1 == secondary->merge_primary)
					{
						pol->merge1 = primary;
					}
					else if(pol->po2 == secondary->merge_primary)
					{
						pol->merge2 = primary;
					}
					else if(pol->merge1 == secondary->merge_primary)
					{
						pol->merge1 = primary;
					}
					else if(pol->merge2 == secondary->merge_primary)
					{
						pol->merge2 = primary;
					}
					else
						printf("%s(): uh... we should not be here\n", __FUNCTION__);
				}
				
				// add the new page object links to the primary, and take them away from the secondary->merge_primary
				primary->links = g_list_concat(primary->links, secondary->merge_primary->links);
				secondary->merge_primary->links = NULL;
				
				if( (last=page_object_get_last_merge(primary)) )
				{
					last->merge = secondary->merge_primary;
				}
				else
				{
					// the primary has no merges
					primary->merge = secondary->merge_primary;
				}
				secondary->merge_primary->merge_primary = primary;
				for(po = secondary->merge_primary->merge; po; po = po->merge)
					po->merge_primary = primary;
			}
		}
		else
		{
			// single unmerged secondary
			
			
			secondary->merge = primary->merge;
			primary->merge = secondary;
			secondary->merge_primary = primary;

			// add the new page object links to the primary, and take them away from the secondary->merge_primary
			for(gl = secondary->links; gl; gl = gl->next)
			{
				pol = gl->data;
				if(pol->po1 == secondary)
				{
					pol->merge1 = primary;
				}
				else if(pol->po2 == secondary)
				{
					pol->merge2 = primary;
				}
				else if(pol->merge1 == secondary)
				{
					pol->merge1 = primary;
				}
				else if(pol->merge2 == secondary)
				{
					pol->merge2 = primary;
				}
				else
					printf("%s(): uh... we should not be here\n", __FUNCTION__);
			}
			primary->links = g_list_concat(primary->links, secondary->links);
			secondary->links = NULL;
		}
		
		secondary->flags &= ~(PAGE_OBJECT_HIGHLIGHT);
		page_object_links_move(primary->links, NULL);
		page_object_display(secondary);
		page_object_display(primary);
	}
}

void net_page_merge_plink(GtkWidget *w, gpointer data)
{
	net_page *np = data;
	page_object *primary, *secondary;
	
	if(is_valid_net_page(np))
	{
		primary = np->plink;
		
		if(primary)
		{
			for(secondary = primary->plink; secondary; secondary = secondary->plink)
				page_object_merge(primary, secondary);
		}
	}

	net_page_unset_plink();
	net_page_highlight_all(np, FALSE);
}

void net_page_unmerge_plink(GtkWidget *w, gpointer data)
{
	net_page *np = data;
	page_object *po;
	
	if(is_valid_net_page(np))
	{
		for(po = np->plink; po; po = po->plink)
			page_object_unmerge(po);
	}
}

void net_page_map_page_objects(net_page *np, page_object *po, page_object *po2)
{
	page_object_link *pol;
	double x1, x2, y1, y2;
	
	if(np && po && po2 &&                                                // are our pointers ok?
	   is_valid_page_object(np, po) && is_valid_page_object(np, po2) &&  // are they valid page_objects?
	   po != po2)                                                        // dont add link if they are the same
	{
		// called twice cause it would be in there twice
		if((pol = page_object_link_get(po, po2)) )
			page_object_link_destroy(pol, NULL);
		if((pol = page_object_link_get(po, po2)) )
			page_object_link_destroy(pol, NULL);
			
		pol = page_object_link_new();

		pol->po1 = po;
		pol->po2 = po2;
		
		if(po->merge_primary)
			po->merge_primary->links =  g_list_append(po->merge_primary->links, pol);
		else
			po->links =  g_list_append(po->links, pol);
			
		if(po2->merge_primary)
			po2->merge_primary->links = g_list_append(po2->merge_primary->links, pol);
		else
			po2->links = g_list_append(po2->links, pol);

// we are going to use the primary page_object to draw the lines, while 
// still preserving the REAL links to the old page_objects, they might want
// to unmerge them sometime
		if(po->merge_primary)
		{
			po = po->merge_primary;
			pol->merge1 = po;
		}
		
		if(po2->merge_primary)
		{
			po2 = po2->merge_primary;
			pol->merge2 = po2;
		}

		gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(po->group), &x1, &y1, &x2, &y2);
		pol->pts->coords[0] = x1 + (x2 - x1)/2.0;
		pol->pts->coords[1] = y1 + (y2 - y1)/2.0;

		gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(po2->group), &x1, &y1, &x2, &y2);
		pol->pts->coords[2] = x1 + (x2 - x1)/2.0;
		pol->pts->coords[3] = y1 + (y2 - y1)/2.0;

		pol->link = gnome_canvas_item_new( gnome_canvas_root(GNOME_CANVAS(np->canvas)),
		                                   GNOME_TYPE_CANVAS_LINE,
		                                   "points", pol->pts,
		                                   "fill_color", "black",
		                                   "width_units", 1.0,
		                                   NULL);

		gnome_canvas_item_lower_to_bottom(pol->link);
	}
}

void net_page_unmap_all(net_page *np)
{
	page_object *po;

	if(np)
	{
		for(po = np->page_objects; po; po = po->next)
		{
			g_list_foreach(po->links, (GFunc)page_object_link_destroy, po);
			g_list_free(po->links);
			po->links = NULL;
		}
	}
}
	

void page_object_link_move(page_object_link *pol, page_object *po)
{
	double x1, x2, y1, y2;
	page_object *po1, *po2;
	
	if(pol)
	{
		po1 = (pol->merge1 ? pol->merge1 : pol->po1);
		po2 = (pol->merge2 ? pol->merge2 : pol->po2);
	
		/*
		 * if the link should point from it to itself, then just move both
		 * ends of the link (po == NULL for short)
		 */
		if(po1 == po2)
			po = NULL;
				
		if(po1 == po)
		{
			gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(po1->group), &x1, &y1, &x2, &y2);
			pol->pts->coords[0] = x1 + (x2 - x1)/2.0;
			pol->pts->coords[1] = y1 + (y2 - y1)/2.0;
			DEBUG_BIGTIME(printf("link bounds:\nx1 = %f\ny1 = %f\nx2 = %f\ny2 = %f\n",x1, y1, x2, y2));
		}
		else
		{
			if(po2 == po)
			{
				gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(po2->group), &x1, &y1, &x2, &y2);
				pol->pts->coords[2] = x1 + (x2 - x1)/2.0;
				pol->pts->coords[3] = y1 + (y2 - y1)/2.0;
				DEBUG_BIGTIME(printf("link bounds:\nx1 = %f\ny1 = %f\nx2 = %f\ny2 = %f\n",x1, y1, x2, y2));
			}
			else
			{
				if(po == NULL)
				{
					gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(po1->group), &x1, &y1, &x2, &y2);
					pol->pts->coords[0] = x1 + (x2 - x1)/2.0;
					pol->pts->coords[1] = y1 + (y2 - y1)/2.0;

					gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(po2->group), &x1, &y1, &x2, &y2);
					pol->pts->coords[2] = x1 + (x2 - x1)/2.0;
					pol->pts->coords[3] = y1 + (y2 - y1)/2.0;
				}
				else
					printf("%s(): oops, i got an invalid page_object to move\n", __FUNCTION__);
			}
		}
		gnome_canvas_item_set( GNOME_CANVAS_ITEM(pol->link), "points", pol->pts, NULL);
	}
}

void page_object_links_move(GList *list, page_object *po)
{
	if(list)
		g_list_foreach(list, (GFunc)page_object_link_move,po);
}

void page_object_highlight_highlight(page_object *po)
{
	DEBUG(printf("%s()\n", __FUNCTION__)); 
	if(po->merge_primary == NULL || po->merge_primary == po)
	{
		po->flags |= PAGE_OBJECT_HIGHLIGHT;
		gnome_canvas_item_set(GNOME_CANVAS_ITEM(po->box), "fill_color", "blue", NULL);
		gnome_canvas_item_set(GNOME_CANVAS_ITEM(po->label), "fill_color", "white", NULL);
	}
}

void page_object_highlight_unhighlight(page_object *po)
{
	DEBUG(printf("%s()\n", __FUNCTION__)); 
	if(po->merge_primary == NULL || po->merge_primary == po)
	{
		po->flags &= ~(PAGE_OBJECT_HIGHLIGHT); 
		gnome_canvas_item_set(GNOME_CANVAS_ITEM(po->box), "fill_color", "white", NULL);
		gnome_canvas_item_set(GNOME_CANVAS_ITEM(po->label), "fill_color", "black", NULL);
	}
}	

/*
 * Highlight all or unhighlight all (i know it looks nasty but it is fast)
 */
void net_page_highlight_all(net_page *np, int highlight)
{
	page_object *po;
	
	if(highlight) /* highlight all */
	{
		for(po = np->page_objects;po;po = po->next)
		{
			if(!(po->flags & PAGE_OBJECT_HIGHLIGHT) && 
			    (po->merge_primary == po || po->merge_primary == NULL))
			{
				page_object_highlight_highlight(po);
			}
		}
	}
	else
	{
		for(po = np->page_objects;po;po = po->next)
		{
			if(po->flags & PAGE_OBJECT_HIGHLIGHT && 
			    (po->merge_primary == po || po->merge_primary == NULL))
			{
				page_object_highlight_unhighlight(po);
			}
		}
	}
}

/*
 * Toggle the page_object's highlight state
 */
void page_object_highlight_toggle(page_object *po)
{
	if(po->flags & PAGE_OBJECT_HIGHLIGHT)
		page_object_highlight_unhighlight(po);
	else
		page_object_highlight_highlight(po);
}

/*
 * set the page_object's highlight state
 */
void page_object_highlight_set(page_object *po)
{
	if(po->flags & PAGE_OBJECT_HIGHLIGHT)
		page_object_highlight_highlight(po);
	else
		page_object_highlight_unhighlight(po);
}

/*
 * remove all plink's on the current net_page
 */
void net_page_unset_plink()
{
	net_page *np = get_current_net_page();
	
	np->plink = NULL;
}

/*
 * Set up the plink adding all highlighted items on the current net_page
 */
void net_page_set_plink()
{
	net_page *np = get_current_net_page();
	page_object *po;
	
	net_page_unset_plink();
	
	for(po = np->page_objects; po; po = po->next)
	{
		if(po->flags & PAGE_OBJECT_HIGHLIGHT)
		{
			po->plink = np->plink;
			np->plink = po;
		}
	}
}

/*
 * get a page object on the net_page
 */
page_object *page_object_get(net_page *np, char *name)
{
	page_object *p;
	
	if(is_valid_net_page(np))
	{
		for(p = np->page_objects; p; p = p->next)
		{
			if(!strcmp(name, p->name))
			{
				return(p);
			}
		}
	}
	return(NULL);
}

/*
 * get a page object on the net_page by the ip address
 */
page_object *page_object_get_by_ip(net_page *np, unsigned int ip)
{
	page_object *p;
	
	if(is_valid_net_page(np))
	{
		for(p = np->page_objects; p; p = p->next)
		{
			if(p->ip == ip)
			{
				return(p);
			}
		}
	}
	return(NULL);
}

int is_valid_page_object(net_page *np, page_object *po)
{
	page_object *p;
	
	if(is_valid_net_page(np))
	{
		for(p = np->page_objects; p; p = p->next)
		{
			if(p == po)
			{
				return(TRUE);
			}
		}
	}
	return(FALSE);
}

/*
 * delete a single page_object on the net_page's canvas
 */
void page_object_delete(net_page *np, page_object *po, int force)
{
	page_object *temp, *p, *prev = NULL;

	for(p = np->page_objects; p; p = p->next)
	{
		if((void *)po == (void *)p)
		{
			if(p->merge)
			{
				if(p->merge_primary == p)
				{
					page_object *next;
					/*
					 * we are the primary in the merge, so ask them if they
					 * want to delete all or what?
					 */
					if(force || make_gnome_yes_no_dialog("Delete all Merged Hosts?",
					                             "Do you want to delete all merged hosts?",
					                             TRUE, 
					                             NULL) == 0)
					{
						for(temp = p->merge; temp; temp = next)
						{
							next = temp->merge;
							temp->merge = NULL;
							page_object_delete(np,temp, force);
						}
					}
					else
					{
						for(temp = p->merge; temp; temp = next)
						{
							next = temp->merge;
							page_object_unmerge(temp);
						}
					}
				}
				else
				{
					page_object_unmerge(p);
				}
			}
			break;
		}
		prev = p;
	}

	prev = NULL;
		
	for(p = np->page_objects; p; p = p->next)
	{
		if((void *)po == (void *)p)
		{
			if(p->info_window)
				gtk_widget_destroy(p->info_window);
			if(p->group)
				gtk_object_destroy(GTK_OBJECT(p->group));
			if(p->data)	
				free(p->data);
			if(p->notes)	
				free(p->notes);
			if(p->name)	
				free(p->name);
			if(p->icon_file_name) 	
				free(p->icon_file_name);
			if(p->popup_list)	
				free(p->popup_list);
			
			page_object_free_os_data(po);
			
			if(p->links)
			{
				g_list_foreach(p->links, (GFunc)page_object_link_destroy, p);
				g_list_free(p->links);
			}
			
			if(prev == NULL)
				np->page_objects = p->next;
			else
				prev->next = p->next;
			free(p);

			break;
		}
		prev = p;
	}
}

/*
 * delete all highlighted page_objects on a net_page (np can be NULL == use current page)
 */
void net_page_delete_page_object(net_page *np)
{
	page_object *po;
	page_object *next;
	
	if(np==0)
		np = get_current_net_page();
	
	for(po = np->plink; po; po = next)
	{
		next = po->plink;
		page_object_delete(np, po, 0);
	}
	np->plink = NULL;
}			

/*
 * Save page_object info and remove the info window
 */
void page_object_hide_properties(GtkWidget *w,page_object *po)
{
	char *new = gtk_editable_get_chars(GTK_EDITABLE(po->notes_widget), 0, -1);
	char *os = gtk_editable_get_chars(GTK_EDITABLE(po->os_widget), 0, -1);
	
	if(po->notes == NULL || strcmp(po->notes,new))
	{
		if(po->notes)
			g_free(po->notes);
		po->notes = strdup(new);
	}
	else
	{
		g_free(new);
	}
	if(os)
	{
		if(po->os_data)
		{
			if(po->os_data->os)
				free(po->os_data->os);
			po->os_data->os = strdup(os);
		}
		else
		{
			po->os_data = malloc(sizeof(*(po->os_data)));
			if(po->os_data)
			{
				memset(po->os_data, 0, sizeof(*(po->os_data)));
				po->os_data->os = strdup(os);
			}
		}
	}
	
	gtk_widget_destroy(po->info_window);
	po->info_window = NULL;
	page_object_display(po);
}

/*
 * Display the Info about the page object
 */
void page_object_show_properties(GtkWidget *w, gpointer data)
{
	net_page *np = get_current_net_page();
	page_object *po = np->plink;
	os_port_entry *port;
	GtkWidget *button;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *window;
	GtkWidget *label;
	GtkWidget *clist;
	GtkWidget *scrolled_window;
	GtkWidget *textbox;
	GtkWidget *notebook;
	GtkWidget *main_notebook;
	GtkWidget *edit;
	char buf[256];
	char *clist_titles[] = { "Service", "Port", "Protocol", "State", "Owner", "Service Version" };
	char *bufs[ sizeof(clist_titles)/sizeof(char *) ];
	char port_buf[20];
	int i = 0;
	
	if(po)
	{
		strcpy(buf, "Properties for ");
		strcat(buf, page_object_get_name(po));
		
		window = gtk_window_new(GTK_WINDOW_DIALOG);
		gtk_window_set_title(GTK_WINDOW(window),buf);
//		gtk_widget_set_usize(window,200,200);
		gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
		
		vbox = gtk_vbox_new(FALSE, 5);
		gtk_widget_show(vbox);
		gtk_container_add(GTK_CONTAINER(window),vbox);

		main_notebook = gtk_notebook_new();
		gtk_notebook_set_tab_pos(GTK_NOTEBOOK(main_notebook), GTK_POS_TOP);
		gtk_notebook_set_show_border(GTK_NOTEBOOK(main_notebook), TRUE);
		gtk_box_pack_start(GTK_BOX(vbox), main_notebook, FALSE, FALSE, 5);
		gtk_widget_show(main_notebook);

		button = gtk_button_new_with_label("close");
		gtk_signal_connect(GTK_OBJECT(button), "clicked", page_object_hide_properties, po);
		gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 5);
		gtk_widget_show(button);

				
		label = gtk_label_new("General");
		vbox = gtk_vbox_new(FALSE, 5);
		gtk_widget_show(vbox);
		gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook),vbox, label); 

/* Operating system */
		hbox = gtk_hbox_new(FALSE, 5);
		gtk_widget_show(hbox);
		label = gtk_label_new("Operating System: ");
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
		gtk_widget_show(label);
		label = gtk_label_new(buf);
		textbox = gtk_entry_new();
		po->os_widget = textbox;
		if(po->os_data)
			gtk_entry_set_text(GTK_ENTRY(textbox), po->os_data->os);
		gtk_widget_show(textbox);
		gtk_box_pack_start(GTK_BOX(hbox), textbox, FALSE, FALSE, 5);

		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
		gtk_widget_show(hbox);

		notebook = gtk_notebook_new();
		gtk_box_pack_start(GTK_BOX(vbox), notebook, FALSE, FALSE, 2);
		gtk_widget_show(notebook);
				
		if(po->merge_primary)
		{
			page_object *p;
			
			for(p = po; p; p = p->merge)
			{
				vbox = gtk_vbox_new(FALSE, 5);		
				gtk_widget_show(vbox);
				label = gtk_label_new( ip2str(p->ip) );		
				gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

		/* IP Address */
				hbox = gtk_hbox_new(FALSE, 5);
				gtk_widget_show(hbox);
				label = gtk_label_new("IP Address: ");
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				label = gtk_label_new( ip2str(p->ip) );		
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

		/* DNS Address */
				hbox = gtk_hbox_new(FALSE, 5);
				gtk_widget_show(hbox);
				label = gtk_label_new("DNS Address: ");
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				if(p->name)
					label = gtk_label_new( p->name );
				else
					label = gtk_label_new( "Not Known" );
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

		/* Uptime */
				hbox = gtk_hbox_new(FALSE, 5);
				gtk_widget_show(hbox);
				label = gtk_label_new("Uptime: ");
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				{
					char buffer[100];
					if(p->os_data)
					{
						sprintf(buffer, "%d seconds", p->os_data->uptime_seconds);
						label = gtk_label_new( buffer );
					}
					else
						label = gtk_label_new( "Not Known" );
				}
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

		/* Last Boot Time */
				hbox = gtk_hbox_new(FALSE, 5);
				gtk_widget_show(hbox);
				label = gtk_label_new("Last Boot Time: ");
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				{
					char buffer[100];
					if(p->os_data && p->os_data->uptime_last_boot)
					{
						sprintf(buffer, "%s", p->os_data->uptime_last_boot);
						label = gtk_label_new( buffer );
					}
					else
						label = gtk_label_new( "Not Known" );
				}
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

		/* Services running */
				clist = gtk_clist_new_with_titles(sizeof(clist_titles)/sizeof(char*), clist_titles);
				gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
				gtk_clist_set_shadow_type(GTK_CLIST(clist), GTK_SHADOW_ETCHED_OUT); 
			
				if(p->os_data)
				{
					for(i = 0, port = p->os_data->ports; port; port = port->next)
					{
						char *rpcstr = NULL;
						
						bufs[0] = port->name;
						
						sprintf(port_buf, "%d", port->port);
						bufs[1] = port_buf;
						
						bufs[2] = ( port->protocol == 6 ? "TCP":"UDP");
						switch(port->state)
						{
							case PORT_STATE_OPEN:
								bufs[3] = "open";
								break;
							
							case PORT_STATE_CLOSED:
								bufs[3] = "closed";
								break;
							
							case PORT_STATE_FILTERED:
								bufs[3] = "filtered";
								break;
							
							case PORT_STATE_UNFILTERED:
								bufs[3] = "UNfiltered";
								break;
							
							case PORT_STATE_UNKNOWN:
							default:
								bufs[3] = "open";
								break;
						}
						
						if(port->owner)
							bufs[4] = port->owner;
						else
							bufs[4] = "";

						if(port->version)
							bufs[5] = port->version;
						else
							bufs[5] = "";
							
						if(port->proto == PORT_PROTO_RPC)
						{
							rpcstr = malloc(100 + strlen(bufs[5]) + 1);
							
							if(port->version)
								sprintf(rpcstr, "%s (RPC port %d version %d.%d)", port->version, port->rpcnum, port->rpchighver, port->rpclowver);
							else
								sprintf(rpcstr, "(RPC port %d version %d.%d)", port->rpcnum, port->rpchighver, port->rpclowver);
							
							bufs[5] = rpcstr;
						}
						gtk_clist_prepend(GTK_CLIST(clist), bufs);
						i++;
						
						if(rpcstr)
							free(rpcstr);
					}
				}

				if(i == 0 || p->os_data == NULL)
				{
					gtk_widget_destroy(clist);
					/*
					 * dont show the clist, cause nothing is in it
					 */
					label = gtk_label_new("No service information avaliable");
					gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
					gtk_widget_show(label);
				}
				else
				{
					scrolled_window = gtk_scrolled_window_new(NULL,NULL);
					gtk_container_add(GTK_CONTAINER(scrolled_window), clist);
					gtk_widget_set_usize(scrolled_window,600,350);
					gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, FALSE, FALSE, 5);
					gtk_widget_show(scrolled_window);
					gtk_widget_show(clist);
					gtk_clist_columns_autosize(GTK_CLIST(clist));
				}
				
			}
		}
		else
		{				
			vbox = gtk_vbox_new(FALSE, 5);		
			gtk_widget_show(vbox);
			label = gtk_label_new( ip2str(po->ip) );
			gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);

	/* IP Address */		
			hbox = gtk_hbox_new(FALSE, 5);
			gtk_widget_show(hbox);
			label = gtk_label_new("IP Address: ");
			gtk_widget_show(label);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
			label = gtk_label_new( ip2str(po->ip) );		
			gtk_widget_show(label);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	/* DNS Address */
			hbox = gtk_hbox_new(FALSE, 5);
			gtk_widget_show(hbox);
			label = gtk_label_new("DNS Address: ");
			gtk_widget_show(label);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
			if(po->name)
				label = gtk_label_new( po->name );
			else
				label = gtk_label_new( "Not Known" );
			gtk_widget_show(label);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

		/* Uptime */
				hbox = gtk_hbox_new(FALSE, 5);
				gtk_widget_show(hbox);
				label = gtk_label_new("Uptime: ");
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				{
					char buffer[100];
					if(po->os_data)
					{
						sprintf(buffer, "%d seconds", po->os_data->uptime_seconds);
						label = gtk_label_new( buffer );
					}
					else
						label = gtk_label_new( "Not Known" );
				}
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

		/* Last Boot Time */
				hbox = gtk_hbox_new(FALSE, 5);
				gtk_widget_show(hbox);
				label = gtk_label_new("Last Boot Time: ");
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				{
					char buffer[100];
					if(po->os_data && po->os_data->uptime_last_boot)
					{
						sprintf(buffer, "%s", po->os_data->uptime_last_boot);
						label = gtk_label_new( buffer );
					}
					else
						label = gtk_label_new( "Not Known" );
				}
				gtk_widget_show(label);
				gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
				gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);

	/* Services running */
			clist = gtk_clist_new_with_titles(sizeof(clist_titles)/sizeof(char*), clist_titles);
			gtk_clist_set_selection_mode(GTK_CLIST(clist),GTK_SELECTION_SINGLE);
			gtk_clist_set_shadow_type(GTK_CLIST(clist), GTK_SHADOW_ETCHED_OUT); 
			if(po->os_data)
			{
				for(i = 0, port = po->os_data->ports; port; port = port->next)
				{
					char *rpcstr = NULL;
					
					bufs[0] = port->name;
					
					sprintf(port_buf, "%d", port->port);
					bufs[1] = port_buf;
					
					bufs[2] = ( port->protocol == 6 ? "TCP":"UDP");
					switch(port->state)
					{
						case PORT_STATE_OPEN:
							bufs[3] = "open";
							break;
						
						case PORT_STATE_CLOSED:
							bufs[3] = "closed";
							break;
						
						case PORT_STATE_FILTERED:
							bufs[3] = "filtered";
							break;
						
						case PORT_STATE_UNFILTERED:
							bufs[3] = "UNfiltered";
							break;
						
						case PORT_STATE_UNKNOWN:
						default:
							bufs[3] = "open";
							break;
					}

					if(port->owner)
						bufs[4] = port->owner;
					else
						bufs[4] = "";

					if(port->version)
						bufs[5] = port->version;
					else
						bufs[5] = "";

					if(port->proto == PORT_PROTO_RPC)
					{
						rpcstr = malloc(100 + strlen(bufs[5]) + 1);
						
						if(port->version)
							sprintf(rpcstr, "%s (RPC port %d version %d.%d)", port->version, port->rpcnum, port->rpchighver, port->rpclowver);
						else
							sprintf(rpcstr, "(RPC port %d version %d.%d)", port->rpcnum, port->rpchighver, port->rpclowver);
						
						bufs[5] = rpcstr;
					}
					gtk_clist_prepend(GTK_CLIST(clist), bufs);
					i++;
					
					if(rpcstr)
						free(rpcstr);
				}
			}

			if(i == 0 || po->os_data == NULL)
			{
				gtk_widget_destroy(clist);
				/*
				 * dont show the clist, cause nothing is in it
				 */
				label = gtk_label_new("No service information avaliable");
				gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
				gtk_widget_show(label);
			}
			else
			{
				scrolled_window = gtk_scrolled_window_new(NULL,NULL);
				gtk_container_add(GTK_CONTAINER(scrolled_window), clist);
				gtk_widget_set_usize(scrolled_window,600,350);
				gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, FALSE, FALSE, 5);
				gtk_widget_show(scrolled_window);
				gtk_widget_show(clist);
			}
			gtk_clist_columns_autosize(GTK_CLIST(clist));
			
		}

/*************************************
 *     monitoring
 *************************************/
 
		label = gtk_label_new("Monitoring");
		hbox = gtk_vbox_new(FALSE, 5);
		gtk_widget_show(hbox);
		gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook),hbox, label); 

#ifdef USING_MONITORING		
		label = gtk_label_new("Monitoring");
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);

		vbox = gtk_vbox_new(FALSE, 5);
		gtk_widget_show(vbox);
		gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 5);

		po->monitoring_vbox = hbox;
		po->monitoring_fields_vbox = vbox;
		gui_monitoring_toplevel(po);
#else /* USING_MONITORING */
		label = gtk_label_new("Not Working Yet");
		gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
#endif /* USING_MONITORING */
/*************************************
 *     comments
 *************************************/
 
		label = gtk_label_new("Comments");
		vbox = gtk_vbox_new(FALSE, 5);
		gtk_widget_show(vbox);
		gtk_notebook_append_page(GTK_NOTEBOOK(main_notebook),vbox, label); 

		edit = gtk_text_new(NULL,NULL);
		gtk_text_set_editable(GTK_TEXT(edit),TRUE);
		gtk_box_pack_start(GTK_BOX(vbox), edit, TRUE, TRUE, 5);
		if(po->notes)
			gtk_text_insert(GTK_TEXT(edit),NULL,NULL,NULL,po->notes,-1);
		gtk_widget_show(edit);		
		
		po->notes_widget = edit;

		
		po->info_window = window;
		gtk_widget_show(window);
	}
}


void page_object_service_callback(GtkWidget *widget, gpointer data)
{
	int port = (u32)(unsigned long)data >> 8;
	int protocol = (u32)(unsigned long)data & 0xFF;

	// there can only be one page_object selected that can show the service list,
	// so i use this quick and dirty way to get at the page object :)
	service_callback( (get_current_net_page())->plink, port, protocol);
}

void page_object_script_callback(GtkWidget *widget, script_t *script)
{
	page_object *po;
	
	for(po = get_current_net_page()->plink; po; po = po->plink)
		run_command_callback(po, 0, 0, script->script);
}

int net_page_count_plink(net_page *np)
{
	page_object *po;
	int i = 0;
	
	if(is_valid_net_page(np))
	{
		for(po = np->plink; po; po = po->plink, i++)
			;
	}
	
	return(i);
}

void seperator_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;
	
	item = gtk_menu_item_new(); // put in a seperator
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_widget_show(item);
}
	
void nonmultiple_seperator_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;
	
	if(net_page_count_plink(np) == 1)
	{
		item = gtk_menu_item_new(); // put in a seperator
		gtk_menu_append(GTK_MENU(menu), item);
		gtk_widget_show(item);
	}
}
	
void delete_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;

	item = gtk_menu_item_new_with_label("Delete");
	gtk_signal_connect(GTK_OBJECT(item), "activate",
	                   GTK_SIGNAL_FUNC(do_delete_page_object),
	                   (gpointer)NULL);
	
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_widget_show(item);
}

void properties_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;
	
	/*
	 * dont add anything into the menu if there are more than 1 page_objects
	 * selected (highlighted)
	 */
	if(net_page_count_plink(np) == 1)
	{
		item = gtk_menu_item_new_with_label("Properties");
		gtk_signal_connect(GTK_OBJECT(item), "activate",
		                   GTK_SIGNAL_FUNC(page_object_show_properties),
		                   (gpointer)NULL);
		
		gtk_menu_append(GTK_MENU(menu), item);
		gtk_widget_show(item);
	}
}

void map_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;
	char *name = "Map";
	
	if( net_page_count_plink(np) > 1 || po->links != NULL)
		name = "ReMap";

	item = gtk_menu_item_new_with_label(name);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
	                   GTK_SIGNAL_FUNC(net_page_map_plink),
	                   (gpointer)np);
	
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_widget_show(item);
}

void osscan_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;
	char *name = "OS Scan";
	
	if( net_page_count_plink(np) > 1 || po->os_data != NULL)
		name = "ReOS Scan";

	item = gtk_menu_item_new_with_label(name);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
	                   GTK_SIGNAL_FUNC(net_page_osscan_plink),
	                   (gpointer)np);
	
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_widget_show(item);
}

void probe_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;
	char *name = "Scan port versions";
	
	if( net_page_count_plink(np) > 1 || po->os_data != NULL)
		name = "ReScan port versions";

	item = gtk_menu_item_new_with_label(name);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
	                   GTK_SIGNAL_FUNC(net_page_probe_plink),
	                   (gpointer)np);
	
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_widget_show(item);
}

void dns_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;
	char *name = "Reverse DNS";
	
	if( net_page_count_plink(np) > 1 || po->name != NULL)
		name = "ReReverse DNS";

	item = gtk_menu_item_new_with_label(name);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
	                   GTK_SIGNAL_FUNC(net_page_dns_plink),
	                   (gpointer)np);
	
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_widget_show(item);
}

#ifdef USING_MONITORING
void monitoring_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;
	char *name = "Monitoring";
	
	item = gtk_menu_item_new_with_label(name);
	gtk_signal_connect(GTK_OBJECT(item), "activate",
	                   GTK_SIGNAL_FUNC(gui_monitoring_make_dialog),
	                   (gpointer)po);
	
	gtk_menu_append(GTK_MENU(menu), item);
	gtk_widget_show(item);
}
#endif /* USING_MONITORING */

void merge_option_set_merge_primary(GtkWidget *w, gpointer data)
{
	page_object *po = (page_object *)data;
	net_page *np = get_current_net_page();
	
	if(is_valid_page_object(np, po))
	{
		if(po->merge_primary && po->merge_primary != po)
		{
			page_object_set_merge_primary(np, po->merge_primary, po);
		}
	}
}

void merge_option_callback(net_page *np, page_object *po, GtkWidget *menu, void *data)
{
	GtkWidget *item;
	GtkWidget *submenu;
	char *name;
	page_object *p;
	
	if( net_page_count_plink(np) == 1 )
	{
		if(np->plink && np->plink->merge)
		{
			item = gtk_menu_item_new(); // put in a seperator
			gtk_menu_append(GTK_MENU(menu), item);
			gtk_widget_show(item);

/* UnMerge Host */
			name = "UnMerge Host";
			item = gtk_menu_item_new_with_label(name);
			gtk_signal_connect(GTK_OBJECT(item), "activate",
			                   GTK_SIGNAL_FUNC(net_page_unmerge_plink),
			                   (gpointer)np);
	
			set_widget_tooltips(item, "UnMerge IP aliased hosts (hosts with more than one IP address) with this");
			
			gtk_menu_append(GTK_MENU(menu), item);
			gtk_widget_show(item);

/* Change Merge Primary */
			name = "Change Merge Primary";
			item = gtk_menu_item_new_with_label(name);
			set_widget_tooltips(item, "Change the primary ip address for this multihomed host");
			submenu = gtk_menu_new();
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
			gtk_menu_append(GTK_MENU(menu), item);
			gtk_widget_show(item);
			gtk_widget_show(submenu);
			
			for(p = po->merge; p; p = p->merge)
			{
				if(options_use_ip_for_merged_ports ||  p->name == NULL)
					name = strdup(inet_ntoa(*((struct in_addr *)&p->ip)));
				else
					name = strdup(p->name);
					
				item = gtk_menu_item_new_with_label(name);
				gtk_signal_connect(GTK_OBJECT(item), "activate",
				                   GTK_SIGNAL_FUNC(merge_option_set_merge_primary),
				                   (gpointer)p);
		
				set_widget_tooltips(item, "Change the primary ip address of this host to this address");
				
				gtk_menu_append(GTK_MENU(submenu), item);
				gtk_widget_show(item);
			}
		}
	}
	else
	{
		name = "Merge Hosts";
		item = gtk_menu_item_new_with_label(name);
		gtk_signal_connect(GTK_OBJECT(item), "activate",
		                   GTK_SIGNAL_FUNC(net_page_merge_plink),
		                   (gpointer)np);

		set_widget_tooltips(item, "Merge IP aliased hosts (hosts with more than one IP address) with this");
		
		gtk_menu_append(GTK_MENU(menu), item);
		gtk_widget_show(item);
	}
}

void set_widget_tooltips(GtkWidget *w, char *text)
{
	GtkTooltips *tooltip;
	
	if(w && text)
	{
		tooltip = gtk_tooltips_new();
		gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltip),w,text,NULL);
	}
}

void page_object_option_list_addall(page_object_option *poo, poo_info *pooi)
{
	if(poo && poo->callback && pooi)
		poo->callback(pooi->np, pooi->po, pooi->menu, poo->data);
}

void page_object_script_list_add(script_t *script, poo_info *pooi)
{
	if(script && (script->flags & SCRIPT_FLAGS_VIEWABLE))
	{
		GtkWidget *item = gtk_menu_item_new_with_label(script->name);
		gtk_signal_connect(GTK_OBJECT(item), "activate",
		                   GTK_SIGNAL_FUNC(page_object_script_callback),
		                   (gpointer)script);

		if(net_page_count_plink(pooi->np) == 1)
			set_widget_tooltips(item, "Run the script in Edit->Settings->Scripts on this host (from cheops-ng)");
		else
			set_widget_tooltips(item, "Run the script in Edit->Settings->Scripts on these hosts (from cheops-ng)");
		
		gtk_menu_append(GTK_MENU(pooi->menu), item);
		gtk_widget_show(item);

		pooi->count++;
	}
}

/*
 * Shows an option list for things to do on a oage_object
 */
void page_object_show_option_list(net_page *np, page_object *po, GdkEventButton *eb)
{
	static GtkWidget *pagemenu = NULL;
	GtkWidget *item, *os_seperator_item;
	os_port_entry *port;
	char buf[100];
	poo_info pooi;
	int did_something = FALSE;
	struct servent *service;
	
	if(pagemenu)
		gtk_widget_destroy(pagemenu);
	
	pagemenu = gtk_menu_new();
	
/* start adding all of the other menues here */
	pooi.np = np;
	pooi.po = po;
	pooi.menu = pagemenu;
	g_list_foreach(page_object_option_list, (GFunc)page_object_option_list_addall, &pooi);

	os_seperator_item = gtk_menu_item_new();
	gtk_menu_append(GTK_MENU(pagemenu), os_seperator_item);

	pooi.count = 0;	
	script_foreach((GFunc)page_object_script_list_add, &pooi);

	if(pooi.count)
	{
		gtk_widget_show(os_seperator_item);

		os_seperator_item = gtk_menu_item_new();
		gtk_menu_append(GTK_MENU(pagemenu), os_seperator_item);
	}
	
	if( net_page_count_plink(np) == 1 )
	{
		if(po->merge_primary)
		{
			page_object *p;
			char hosts_name[1024];
			
			for(p = po; p; p = p->merge)
			{
				if(p->name && !options_use_ip_for_merged_ports)
					strncpy(hosts_name, p->name, sizeof(hosts_name) - 1);
				else
					strncpy(hosts_name, inet_ntoa(*((struct in_addr *)&(p->ip))), sizeof(hosts_name) - 1);
					
				if(p->os_data && p->os_data->ports)
				{
					GtkWidget *submenu = NULL;
					GtkWidget *submenu_name;
					for(port = p->os_data->ports; port; port = port->next)
					{
						if(!submenu)
						{
							snprintf(buf, sizeof(buf), "%s%s", hosts_name, (p == po? " (primary)":""));
							submenu_name = gtk_menu_item_new_with_label(buf);
							gtk_menu_append(GTK_MENU(pagemenu), submenu_name);
							gtk_widget_show(submenu_name);
							
							submenu = gtk_menu_new();
							gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenu_name), submenu);
							gtk_widget_show(submenu);
							did_something = TRUE;
						}
						if(port->state != PORT_STATE_OPEN)
							continue;
						service = getservbyport(htons(port->port), (port->protocol == 6 ? "tcp":"udp"));
						if(service)
							snprintf(buf, sizeof(buf), "%s", service->s_name);
						else
						{
							char *serv = get_service(port->port, port->protocol);
							if(serv && serv[0] != '\0')
								snprintf(buf, sizeof(buf), "%s", serv);
							else
								snprintf(buf, sizeof(buf), "Port %d", port->port);
						}

						if(port->name)
							free(port->name);
							
						port->name = strdup(buf);
							
						item = gtk_menu_item_new_with_label(buf);
						gtk_signal_connect(GTK_OBJECT(item), "activate",
						                   GTK_SIGNAL_FUNC(page_object_service_callback),
						                   GINT_TO_POINTER(port->port << 8) + port->protocol);
						
						gtk_menu_append(GTK_MENU(submenu), item);
						gtk_widget_show(item);
						
						did_something = TRUE;
					}		
				}
				else
				{
					GtkWidget *submenu = NULL;
					GtkWidget *submenu_name;

					snprintf(buf, sizeof(buf), "%s%s", hosts_name, (p == po? " (primary)":""));
					submenu_name = gtk_menu_item_new_with_label(buf);
					gtk_menu_append(GTK_MENU(pagemenu), submenu_name);
					gtk_widget_show(submenu_name);
					
					submenu = gtk_menu_new();
					gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenu_name), submenu);
					gtk_widget_show(submenu);
					did_something = TRUE;
				}
			}
		}
		else
		{
			// not a merged host
			if(po->os_data)
			{
				for(port = po->os_data->ports; port; port = port->next)
				{
					if(port->state != PORT_STATE_OPEN)
						continue;
						
					service = getservbyport(htons(port->port), (port->protocol == 6 ? "tcp":"udp"));
					if(service)
						snprintf(buf, sizeof(buf), "%s", service->s_name);
					else
					{
						char *serv = get_service(port->port, port->protocol);
						if(serv && serv[0] != '\0')
							snprintf(buf, sizeof(buf), "%s", serv);
						else
							snprintf(buf, sizeof(buf), "Port %d", port->port);
					}

					if(port->name)
						free(port->name);
						
					port->name = strdup(buf);
						
					item = gtk_menu_item_new_with_label(buf);
					gtk_signal_connect(GTK_OBJECT(item), "activate",
					                   GTK_SIGNAL_FUNC(page_object_service_callback),
					                   GINT_TO_POINTER(port->port << 8) + port->protocol);
					
					gtk_menu_append(GTK_MENU(pagemenu), item);
					gtk_widget_show(item);
					
					did_something = TRUE;
				}		
			}		
		}
	}
	endservent();
	
	if(did_something)
		gtk_widget_show(os_seperator_item);

	gtk_menu_popup(GTK_MENU(pagemenu), NULL, NULL, NULL, NULL, eb->button, eb->time);
}

void page_object_option_list_add(char *name, page_object_list_callback callback, void *data)
{
	page_object_option *poo;
	
	if( name && callback && (poo = malloc(sizeof(*poo))) )
	{
		page_object_option_list_remove(name);

		poo->name = strdup(name);
		poo->callback = callback;
		poo->data = data;
		
		page_object_option_list = g_list_append(page_object_option_list, poo);
	}
}

void page_object_option_list_remove(char *name)
{
	GList *gl;
	page_object_option *poo;
	
	for(gl = page_object_option_list; gl; gl = gl->next)
	{
		poo = gl->data;
		if( !strcmp(poo->name, name) )
		{
			page_object_option_list = g_list_remove(page_object_option_list, poo);
			free(poo->name);
			free(poo);
			break;
		}
	}
}

void init_net_page_stuff(void)
{
	/* configure the popup list things */
	page_object_option_list_add("properties",
	                            (page_object_list_callback)properties_option_callback, 
	                            NULL);
	page_object_option_list_add("seperator1", 
	                            (page_object_list_callback)nonmultiple_seperator_option_callback, 
	                            NULL);
	page_object_option_list_add("osscan",
	                            (page_object_list_callback)osscan_option_callback, 
	                            NULL);
	page_object_option_list_add("probe",
	                            (page_object_list_callback)probe_option_callback, 
	                            NULL);
	page_object_option_list_add("map",
	                            (page_object_list_callback)map_option_callback, 
	                            NULL);
	page_object_option_list_add("dns",
	                            (page_object_list_callback)dns_option_callback, 
	                            NULL);
	page_object_option_list_add("seperator2", 
	                            (page_object_list_callback)seperator_option_callback, 
	                            NULL);
#ifdef USING_MONITORING
	page_object_option_list_add("monitoring",
	                            (page_object_list_callback)monitoring_option_callback, 
	                            NULL);
	page_object_option_list_add("seperator3", 
	                            (page_object_list_callback)seperator_option_callback, 
	                            NULL);
#endif /* USING_MONITORING */

	page_object_option_list_add("delete", 
	                            (page_object_list_callback)delete_option_callback, 
	                            NULL);
	page_object_option_list_add("merge", 
	                            (page_object_list_callback)merge_option_callback, 
	                            NULL);
}
/*
 * event dispatcher for the page_object
 */
int page_object_event_dispatch(GtkObject *widget, GdkEvent *event, page_object *po)
{
	double x,y;
	GdkEventMotion *em = (GdkEventMotion *)event;
	GdkEventButton *eb = (GdkEventButton *)event;
	page_object *pl;
	net_page *np;
	
	switch( event->type )
	{
		case GDK_BUTTON_PRESS:
			if(eb->button == 1)
			{
				mouse_down_x = eb->x;
				mouse_down_y = eb->y;

				gnome_canvas_item_raise_to_top(GNOME_CANVAS_ITEM(po->group));

				po->flags |= PAGE_OBJECT_GRAB;
				
				if(eb->state & GDK_CONTROL_MASK)
				{
				/* trying to select multiple objects */
					if(po->flags & PAGE_OBJECT_HIGHLIGHT)
					{
					/* selected one that is already highlighted, unhighlight it */
						page_object_highlight_unhighlight(po);
						po->flags &= ~PAGE_OBJECT_GRAB;
					}
					else
					{
					/* it is not highlighted, so highlight it */
						page_object_highlight_highlight(po);
					}
				}
				else
				{
				/* not selecting multiple objects */
					if(!(po->flags & PAGE_OBJECT_HIGHLIGHT))
					{
					/* the object is not highlighted, unhighlight all and then highlight the one selected */
						np = get_current_net_page();
						net_page_highlight_all(np, FALSE);
						page_object_highlight_highlight(po);
						net_page_unset_plink();
					}
					page_object_highlight_highlight(po);
				}
				
				net_page_set_plink();
			}
			else
			{
				if(eb->button == 3)
				{
					if(eb->state & GDK_CONTROL_MASK)
					{
						np = get_current_net_page();
						page_object_highlight_highlight(po);
						net_page_set_plink();
//						while(gtk_events_pending())
//							gtk_main_iteration();
						page_object_show_option_list(np,po,eb);
					}
					else
					{
						np = get_current_net_page();
						if(!(po->flags & PAGE_OBJECT_HIGHLIGHT))
						{
							net_page_highlight_all(np, FALSE);
							page_object_highlight_highlight(po);
							net_page_set_plink();
						}
//						while(gtk_events_pending())
//							gtk_main_iteration();
						page_object_show_option_list(np,po,eb);
					}
					po->flags &= ~PAGE_OBJECT_GRAB;
				}
			}
			break;
			
		case GDK_BUTTON_RELEASE:
			po->flags &= ~PAGE_OBJECT_GRAB;
			if(!options_move_stuff_live)
			{
				page_object *po;
				net_page *np = get_current_net_page();
				for(po = np->plink; po; po = po->plink)
					page_object_links_move(po->links, po);
			}
			break;
		
		case GDK_MOTION_NOTIFY:
			if(po->flags & PAGE_OBJECT_GRAB)
			{
				double xx, yy;
				
//				printf("X = %f\nY = %f\n", em->x, em->y);

				np = get_current_net_page();
				
				if(em->x >= 0.0 && em->x <= CANVAS_FIXED_WIDTH) 
				{
					x = em->x - mouse_down_x;
					mouse_down_x = em->x;
				}
				else
					x = 0;
					
				if(em->y >= 0.0 && em->y <= CANVAS_FIXED_HEIGHT) 
				{
					y = em->y - mouse_down_y;
					mouse_down_y = em->y;
				}
				else
					y = 0;
					
				for(pl = np->plink; pl; pl = pl->plink)
				{
					double boundswidth=CANVAS_FIXED_WIDTH, boundsheight=CANVAS_FIXED_HEIGHT;
					
#if 0
					gnome_canvas_window_to_world(GNOME_CANVAS(np->canvas),
					                             CANVAS_FIXED_WIDTH, CANVAS_FIXED_HEIGHT,
					                             &boundswidth, &boundsheight);
					printf("%f %f\n", boundswidth, boundsheight);
#endif
					xx = x;
					yy = y;
					
					if( ((pl->x1 + x) >= 0) && ((pl->x2 + x) <= boundswidth) )
						pl->x += x;
					else
						xx = 0;
						
					if( ((pl->y1 + y) >= 0) && ((pl->y2 + y) <= boundsheight) )
						pl->y += y;
					else
						yy = 0;	
					
					gnome_canvas_item_move(GNOME_CANVAS_ITEM(pl->group), xx, yy);
					gnome_canvas_item_get_bounds(GNOME_CANVAS_ITEM(pl->group), 
					                             &pl->x1, &pl->y1, &pl->x2, &pl->y2);
					if(options_move_stuff_live)
						page_object_links_move(pl->links, pl);

	//				printf("x=%f y=%f\n",pl->x, pl->y);
				}
			}
			break;
		
		default:
			break;
	}
	return(0);
}	

/*
 * event function for the canvas (tidbit: the canvas event is called before the page items)
 */
int net_page_event_dispatch(GtkObject *widget, GdkEvent *event, net_page *np)
{
	GdkEventButton *eb = (GdkEventButton *)event;
	GdkEventMotion *em = (GdkEventMotion *)event;
	page_object *po;
	double x,y,x1,x2,y1,y2;

	switch( event->type )
	{
		case GDK_BUTTON_PRESS:
			if(eb->button == 1)
			{
				if( !(eb->state & GDK_CONTROL_MASK) )
				{
					gnome_canvas_window_to_world(GNOME_CANVAS(np->canvas),
					                             eb->x,
					                             eb->y,
					                             &x,
					                             &y);
					if(NULL == gnome_canvas_get_item_at(GNOME_CANVAS(np->canvas),x,y) )
					{
					/* we just hit the page to get rid of the highlight */
						net_page_highlight_all(np, FALSE);
						net_page_unset_plink();
						np->button_state |= GDK_BUTTON_PRESS;

						np->select_x = x;
						np->select_y = y;
					}
				}
			}
			else
			{
				if(NULL == gnome_canvas_get_item_at(GNOME_CANVAS(np->canvas),eb->x,eb->y) )
				{
					if (eb->button == 3)
					{
						GtkWidget *pagemenu = gtk_item_factory_get_widget(main_window->itemf, "/Viewspace");
						gtk_menu_popup(GTK_MENU(pagemenu), NULL, NULL, NULL, NULL, eb->button, eb->time);
					}
				}
			}
			break;
			
		case GDK_BUTTON_RELEASE:
			np->button_state &= ~GDK_BUTTON_PRESS;
			if(np->select_rectangle)
			{
				gtk_object_destroy(GTK_OBJECT(np->select_rectangle));
				np->select_rectangle = NULL;
			}
			for(po = np->page_objects; po; po = po->next)
				po->flags &= ~PAGE_OBJECT_GRAB;
			if(!options_move_stuff_live)
			{
				for(po = np->plink; po; po = po->plink)
					page_object_links_move(po->links, po);
			}
			break;
		
		case GDK_MOTION_NOTIFY:
			if(np->button_state & GDK_BUTTON_PRESS)
			{
				if(np->select_rectangle == NULL)
				{
					np->select_rectangle = gnome_canvas_item_new(	gnome_canvas_root(GNOME_CANVAS(np->canvas)),
											GNOME_TYPE_CANVAS_RECT,
											"outline_color", "black",
											"x1", np->select_x,
											"y1", np->select_y,
											"x2", np->select_x,
											"y2", np->select_y,
											NULL);

					gnome_canvas_item_show(GNOME_CANVAS_ITEM(np->select_rectangle));
				}

				gnome_canvas_window_to_world(GNOME_CANVAS(np->canvas),
				                             em->x,
				                             em->y,
				                             &x,
				                             &y);
				x1 = MIN(x, np->select_x);
				x2 = MAX(x, np->select_x);
				y1 = MIN(y, np->select_y);
				y2 = MAX(y, np->select_y);

				gnome_canvas_item_set(  GNOME_CANVAS_ITEM(np->select_rectangle), 
							"x1", x1,
							"y1", y1,
							"x2", x2,
							"y2", y2,
							NULL); 

//				x1 += np->select_x;
//				y1 += np->select_y;
//				x2 += np->select_x;
//				y2 += np->select_y;
							
//				printf("x1=%f y1=%f x2=%f y2=%f\n",x1,y1,x2,y2);
				 
				for(po = np->page_objects; po; po = po->next)
				{
					int pox = po->x;
					int poy = po->y;
					if( (pox > x1) && (pox < x2) && (poy > y1) && (poy < y2) )
					{
						if( !(po->flags & PAGE_OBJECT_HIGHLIGHT) )
							page_object_highlight_highlight(po);
					}
					else
					{
						if( (po->flags & PAGE_OBJECT_HIGHLIGHT) )
							page_object_highlight_unhighlight(po);
					}
				}
				net_page_set_plink();
			
			}
			break;

		default:
			break;
	}
	return(0);
}	


