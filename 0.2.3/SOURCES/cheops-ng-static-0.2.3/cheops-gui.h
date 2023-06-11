/*
 * Cheops Next Generation GUI
 * 
 * cheops-gui.h
 * An agent shell, for testing and communicating directly with agents
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

#ifndef _CHEOPS_GUI_H
#define _CHEOPS_GUI_H

#include <gnome.h>
#include <glib.h>
#include "config.h"
#include "cheops-config.h"
#include "event.h"
#include "gui-handlers.h"
#include "gui-pixmap.h"
#include "version.h"
#ifdef USING_MONITORING
	#include "gui-monitoring.h"
#endif
#define CHEOPS_DEFAULT_FILENAME_NOT_HOME ".gnome/cheops-ng.map"
#define CHEOPS_DEFAULT_FILENAME "~/.gnome/cheops-ng.map"
#define CHEOPS_TITLE "cheops-ng"
#define CHEOPS_MIN_WIDTH 400
#define CHEOPS_MIN_HEIGHT 400

#define CHEOPS_FULL_NAME CHEOPS_TITLE "-" CHEOPS_VERSION

extern char *unknown_xpm[];

extern unsigned int probe_timeout_ms;
struct _page_object;

typedef struct _os_port_entry {
	struct _os_port_entry *next;		
	int	port;
	int  protocol;
	char	*name;
	char *version;
	char *owner;
	unsigned int state;
	unsigned int rpcnum;
	unsigned int rpclowver;
	unsigned int rpchighver;
	unsigned int proto;
} os_port_entry;

typedef struct _os_stats {
	char	*os;
	os_port_entry *ports;
	unsigned int uptime_seconds;
	char *uptime_last_boot;
} os_stats;

typedef struct _page_object_link {
	GnomeCanvasItem *link;
	GnomeCanvasPoints *pts;
	struct _page_object *po1;
	struct _page_object *po2;
	struct _page_object *merge1;
	struct _page_object *merge2;
} page_object_link;

/*
 * An object that is placed on the canvas 
 */
typedef struct _page_object {
	struct _page_object *next;
	struct _page_object *plink;
	
	struct _page_object *merge;          // list of IP aliased hosts
	struct _page_object *merge_primary;  // the host that we will use to display
	
	char                *name;
	int                  ip;
	char                *icon_file_name;

	GdkImlibImage       *im;
	GnomeCanvasItem     *icon;
	GnomeCanvasItem     *label;
	GnomeCanvasItem     *box;
	GnomeCanvasItem     *group;
	GnomeCanvas         *canvas;
	GList               *links;
	
	GtkTooltips	        *tooltips;
	GtkItemFactoryEntry *popup_list;
	int                  popup_list_number;
	
	GtkWidget           *notes_widget;
	GtkWidget           *os_widget;
	GtkWidget           *notes_window;
	char                *notes;
	
	char                *data;
	
	int                  flags;
	double               x,y,x1,x2,y1,y2,width,height;
	
	os_stats            *os_data;
	
	GtkWidget           *info_window;
#ifdef USING_MONITORING
	GtkWidget           *monitoring_fields_vbox; // the container to put all of the stuff into
	GtkWidget           *monitoring_vbox;        // parent of the vbox
#endif
} page_object;

/* defines for the flags member in the page_object */
#define PAGE_OBJECT_GRAB				0x00000001
#define PAGE_OBJECT_HIGHLIGHT			0x00000002
#define PAGE_OBJECT_CHANGE_POPUP_LIST	0x00000004
#define PAGE_OBJECT_CREATED_NEW 		0x00000008
#define PAGE_OBJECT_CONFIG_NEW	 		0x00000010
#define PAGE_OBJECT_OS_DETECT_SENT		0x00000020

#define PAGE_OBJECT(a) ((page_object *)a)

/*
 * Each page in the cheops_window is a net_page
 */
typedef struct _net_page
{
	struct _net_page *next;
	GtkWidget        *label;
	GtkWidget        *hbox;
	GtkWidget        *scrolled_window;
	GtkWidget        *canvas;
	GtkWidget        *list;
	double            pixels_per_unit;	
	GnomeCanvasItem  *select_rectangle;
	GnomeCanvasItem  *select_group;
	char             *name;
	int	              button_state;
	double	          select_x;
	double	          select_y;
	page_object      *page_objects;
	page_object      *plink; /* link of page_objects that are highlighted */
	char             *agent_ip;
	agent            *agent;
	int               connected;
} net_page;

typedef struct _cheops_window
{
	GtkWidget       *window;
	GtkWidget       *menu;
	GtkWidget       *vbox;
	GtkWidget       *hbox;
	GtkWidget       *cvbox;
	GtkWidget       *container;
	GtkWidget       *infobox;
	GtkWidget       *notebook;
	GtkWidget       *status;
	GtkWidget       *namelabel;
	GtkTooltips     *tips;
	GtkItemFactory  *itemf;
	GtkAccelGroup	*accel_group;
	char            *filename;
	net_page        *net_pages;
} cheops_window;

#define MAX_DISCOVER_RETRIES 5

#define GUI_PREFERENCES_CONFIRM_DELETE 1

typedef struct _gui_preferences {
	int boolean_flags;
} gui_preferences;

extern gui_preferences preferences;

#define CANVAS_FIXED_WIDTH  640 * 2
#define CANVAS_FIXED_HEIGHT 480 * 2

#if (GTK_MINOR_VERSION > 1) || ((GTK_MICRO_VERSION > 1) &&  (GTK_MINOR_VERSION > 0))
	#define USE_ITEM
	#define GTK_MENU_FUNC(a) ((GtkItemFactoryCallback)(a))
#else
	#undef USE_ITEM
	typedef void (*_GTK_MENU_FUNC_T)(GtkWidget *, void *);
	#define GTK_MENU_FUNC(a) ((_GTK_MENU_FUNC_T)(a))
#endif


extern cheops_window *main_window;
extern net_page *current_page;
extern agent *primary_agent;

/* Buffer for creating all events */
extern char ebuf[65536];
extern event_hdr *eh;
extern event *ee;

/* GnomeDialog that we should kill if we do a quit() */
extern GtkWidget *kill_me_too;
extern int we_are_still_running;

extern void destroy(GtkWidget *w, gpointer p);

/* quit the application, and kill a modal dialog if it exists */
extern void quit(void);

/* make a new viewspace =0 or remove one =1 */
void do_page(GtkWidget *w, int which);

/*
 * Callback for the menu delete selection (it creates a confirm dialog)
 */
void do_delete_page_object(GtkWidget *w, gpointer data);

void showme(GtkWidget *widget, gpointer data);


/* make a new pointer thingee */
#define NEW(type)	(type *)malloc(sizeof(type))


#define FOR_EACH_DO(function_name, pointer, struct_name, next_name, argument) {\
	struct_name *FOR_EACH_DO_temp;                                             \
	for(FOR_EACH_DO_temp = pointer; FOR_EACH_DO_temp; FOR_EACH_DO_temp = FOR_EACH_DO_temp->next_name) \
		function_name (FOR_EACH_DO_temp, argument);                              \
}


#endif /* _CHEOPS_GUI_H */

