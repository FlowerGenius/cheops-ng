/*
 * Cheops Next Generation GUI
 * 
 * gui-canvas.h
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


#ifndef GUI_CANVAS_H
#define GUI_CANVAS_H

#include <gdk/gdk.h>
#include <gnome.h>
#include "cheops-gui.h"

/*
 * this callback function will provide a gtk_menu_item to be places into the
 * popup menu, a NULL return will not place anything in the menu
 * (data is the user data passed in the page_object_option_list_add() function)
 */
typedef GtkWidget *(*page_object_list_callback)(net_page *np, 
                                                page_object *po, 
                                                GtkWidget *menu, 
                                                void *data);

typedef struct _page_object_option {
	char                      *name;
	page_object_list_callback  callback;
	void                      *data;
} page_object_option;

/*
 * Create a new page_abject with all fields avaliable for use
 */
page_object	*page_object_new(GnomeCanvas *canvas);

/*
 * Display the page object, give it a pixmap, and put it on the canvas
 */
void	page_object_display( page_object *po );

/*
 * event function for the canvas (tidbit: the canvas event is called before the page items)
 */
int	net_page_event_dispatch(GtkObject *widget, GdkEvent *event, net_page *np);

void page_object_map(net_page *np, page_object *po);

/*
 * map the page_objects
 */
void	net_page_map_all(net_page *np);

/*
 * unmap all of the page_objects
 */
void	net_page_unmap_all(net_page *np);

void net_page_map_page_objects(net_page *np, page_object *po, page_object *po2);

/*
 * this highlights/unhighlights all of the oage_objects on a net_page (the highlight is TRUE or FALSE) 
 */
void	net_page_highlight_all(net_page *np, int highlight);

/*
 * delete all highlighted page_objects on a net_page (np can be NULL == use current page)
 */
void	net_page_delete_page_object(net_page *np);

/*
 * delete a single page_object on the net_page's canvas
 * force will not ask any questions
 */
void	page_object_delete(net_page *np, page_object *po, int force);

/*
 * Set up the plink adding all highlighted items on the current net_page
 */
void	net_page_set_plink(void);

/*
 * remove all plink's on the current net_page
 */
void	net_page_unset_plink(void);

/*
 * count the number of plinks on the net_page
 */
int net_page_count_plink(net_page *np);

/*
 * Callback for the menu delete selection (it creates a confirm dialog)
 */
void	delete_page_objects_popup_callback(GtkWidget *w,gpointer data);

void page_object_option_list_remove(char *name);
void page_object_option_list_add(char *name, page_object_list_callback callback, void *data);


/*
 * get a page object on the net_page
 */
page_object *page_object_get(net_page *np, char *name);
page_object *page_object_get_by_ip(net_page *np, unsigned int ip);

int is_valid_page_object(net_page *np, page_object *po);

void page_object_free_os_data(page_object *po);


void init_net_page_stuff(void);

void deleting_group(GtkWidget *w, page_object *po);
void page_object_remove_group(page_object *po);
void page_object_create_group(page_object *po);
char *page_object_get_name(page_object *po);
page_object_link *page_object_link_new();
void page_object_link_destroy(page_object_link *pol, page_object *po);
page_object_link *page_object_link_get(page_object *po1, page_object *po2);
void net_page_map_all(net_page *np);
void net_page_map_plink(GtkWidget *w, gpointer data);
void net_page_osscan_plink(GtkWidget *w, gpointer data);
void net_page_probe_plink(GtkWidget *w, gpointer data);
void net_page_dns_plink(GtkWidget *w, gpointer data);
void net_page_merge_plink(GtkWidget *w, gpointer data);
void net_page_unmerge_plink(GtkWidget *w, gpointer data);
void page_object_link_move(page_object_link *pol, page_object *po);
void page_object_links_move(GList *list, page_object *po);
void page_object_highlight_highlight(page_object *po);
void page_object_highlight_unhighlight(page_object *po);
void page_object_highlight_toggle(page_object *po);
void page_object_hide_notes(GtkWidget *w,page_object *po);
void page_object_show_notes(GtkWidget *w, gpointer data);
void page_object_hide_info(GtkWidget *w,page_object *po);
void page_object_show_info(GtkWidget *w, gpointer data);
void page_object_service_callback(GtkWidget *widget, gpointer data);
void set_widget_tooltips(GtkWidget *w, char *text);
void page_object_highlight_set(page_object *po);
void page_object_unmerge(page_object *po);

void page_object_merge(page_object *primary, page_object *secondary);

void net_page_set_pixels_per_unit(net_page *np, double ppu);
void net_page_zoom_normal(net_page *np);
void net_page_zoom_in(net_page *np);
void net_page_zoom_out(net_page *np);
#endif /* GUI_CANVAS_H */
