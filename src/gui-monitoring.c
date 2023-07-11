/*
 * Cheops Next Generation GUI
 * 
 * gui-monitoring.c
 * initial monitoring support
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
/*
 * check <insert host here>'s [dropdownbox][V]  
 * by [dropdownbox][V]
 */
#include "gui-monitoring.h"

#ifdef USING_MONITORING

#define DEBUG_GUI_MONITORING
#ifdef DEBUG_GUI_MONITORING
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

void gui_monitoring_os(monitoring_session *session, monitoring_node *node);
void gui_monitoring_port(monitoring_session *session, monitoring_node *node);
void gui_monitoring_state(monitoring_session *session, monitoring_node *node);
void gui_monitoring_os_change(monitoring_session *session, monitoring_node *node);
void gui_monitoring_port_open(monitoring_session *session, monitoring_node *node);
void gui_monitoring_port_no_change(monitoring_session *session, monitoring_node *node);
void gui_monitoring_port_no_new_ports(monitoring_session *session, monitoring_node *node);
void gui_monitoring_state_ping_host(monitoring_session *session, monitoring_node *node);
void gui_monitoring_state_check_port(monitoring_session *session, monitoring_node *node);
int  gui_monitoring_render(monitoring_session *mon);


monitoring_node toplevel_monitoring_default_values[] = {
	{ "OS",     TOPLEVEL_MONITOR_OS,        gui_monitoring_os,    NULL, NULL },
	{ "port",   TOPLEVEL_MONITOR_PORT,      gui_monitoring_port,  NULL, NULL },
	{ "state",  TOPLEVEL_MONITOR_STATE,     gui_monitoring_state, NULL, NULL }
};

monitoring_node os_monitoring_default_values[] = {
	{ "making sure it does not change",     MONITOR_OS_CHANGE,        gui_monitoring_os_change,    NULL, NULL },
};

monitoring_node port_monitoring_default_values[] = {
	{ "checking if it is open",                   MONITOR_PORT_OPEN,                  gui_monitoring_port_open,         NULL, NULL },
	{ "making sure the version does not change",  MONITOR_PORT_VERSION_DOESNT_CHANGE, gui_monitoring_port_no_change,    NULL, NULL },
	{ "making sure no new ports open up",         MONITOR_PORT_NO_NEW_PORTS_ADDED,    gui_monitoring_port_no_new_ports, NULL, NULL },
};

monitoring_node state_monitoring_default_values[] = {
	{ "pinging the host", MONITOR_STATE_PING_HOST,  gui_monitoring_state_ping_host,  NULL, NULL },
	{ "checking a port",  MONITOR_STATE_CHECK_PORT, gui_monitoring_state_check_port, NULL, NULL },
};


void gui_monitoring_init(void)
{
	monitoring_node *m;
	monitoring_node *parent;
	int i;
		
	for(i = 0; i < ARRAY_SIZE(toplevel_monitoring_default_values); i++)
	{
		m = monitoring_node_new(toplevel_monitoring_default_values[i].name,
		                        toplevel_monitoring_default_values[i].id,
		                        toplevel_monitoring_default_values[i].callback);
		monitoring_node_add_toplevel(m);
	}
	
	if( (parent = monitoring_node_find_toplevel(TOPLEVEL_MONITOR_OS)) )
	{
		for(i = 0; i < ARRAY_SIZE(os_monitoring_default_values); i++)
		{
			m = monitoring_node_new(os_monitoring_default_values[i].name,
			                        os_monitoring_default_values[i].id,
			                        os_monitoring_default_values[i].callback);
			monitoring_node_add(parent, m);
		}
	}

	if( (parent = monitoring_node_find_toplevel(TOPLEVEL_MONITOR_PORT)) )
	{
		for(i = 0; i < ARRAY_SIZE(port_monitoring_default_values); i++)
		{
			m = monitoring_node_new(port_monitoring_default_values[i].name,
			                        port_monitoring_default_values[i].id,
			                        port_monitoring_default_values[i].callback);
			monitoring_node_add(parent, m);
		}
	}

	if( (parent = monitoring_node_find_toplevel(TOPLEVEL_MONITOR_STATE)) )
	{
		for(i = 0; i < ARRAY_SIZE(state_monitoring_default_values); i++)
		{
			m = monitoring_node_new(state_monitoring_default_values[i].name,
			                        state_monitoring_default_values[i].id,
			                        state_monitoring_default_values[i].callback);
			monitoring_node_add(parent, m);
		}
	}
}

void gui_monitoring_destroy_session(GtkWidget *w, monitoring_session *m)
{
	DEBUG(printf("%s():\n", __FUNCTION__));
	if(m)
	{
		monitoring_session_destroy(m);
	}
}

void gui_monitoring_add_names_to_list(monitoring_node *node, void *arg)
{
	GList **list = (GList **)arg;
	
	DEBUG(printf("%s(): node name '%s'\n", __FUNCTION__, node->name));
	if(node && list)
	{
		*list = g_list_append((*list), node->name);
	}
}

void gui_monitoring_append(GtkWidget *w, monitoring_session *mon)
{
	char *str = gtk_entry_get_text(GTK_ENTRY(w));
	monitoring_node *m;
	
	DEBUG(printf("%s():\n", __FUNCTION__));
	
	m = (monitoring_node *)gtk_object_get_user_data(GTK_OBJECT(w));
	if(m)
	{
		m = monitoring_node_find_by_name(m, str);   // get the selected one 
	}
	else
	{
		m = monitoring_node_find_by_name_toplevel(str);   // get the selected one 
	}
	
	if(m)
	{
		if(mon->id)
			g_free(mon->id);
		mon->id = g_strdup(m->path);
	}

	if(str)
		g_free(str);
	
	// need to call this on a timer so that the gtk code can clean up the widget
	gtk_timeout_add(1, (GtkFunction)gui_monitoring_render, mon);
}

int gui_monitoring_render(monitoring_session *mon)
{
	GtkWidget *combo;
	GList *combo_list = NULL;
	page_object *po = mon->arg;
	GtkWidget *temp;	
	GtkWidget *label;
	
	DEBUG(printf("%s():\n", __FUNCTION__));

	temp = po->monitoring_fields_vbox;
	
	po->monitoring_fields_vbox = gtk_vbox_new(FALSE, 5);
	gtk_box_pack_start(GTK_BOX(po->monitoring_vbox), po->monitoring_fields_vbox, FALSE, FALSE, 5);
	gtk_widget_show(po->monitoring_fields_vbox);

	label = gtk_label_new("Check:");
	gtk_box_pack_start(GTK_BOX(po->monitoring_fields_vbox), label, FALSE, FALSE, 5);
		
	DEBUG(printf("before gtk_combo_new()\n"));
	combo = gtk_combo_new();
	DEBUG(printf("after gtk_combo_new()\n"));
	monitoring_list_foreach_toplevel(gui_monitoring_add_names_to_list, &combo_list);
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), combo_list);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
	gtk_box_pack_start(GTK_BOX(po->monitoring_fields_vbox), combo, FALSE, FALSE, 5);
	gtk_widget_show(combo);
	g_list_free(combo_list);

// need to put something in here to set the toplevel combo to the correct value

	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", GTK_SIGNAL_FUNC(gui_monitoring_append), mon);
	gtk_object_set_user_data(GTK_OBJECT(GTK_COMBO(combo)->entry), NULL); // we are using the monitoring_node for reference

	while(gtk_events_pending())
		gtk_main_iteration();

// why cant i unparent and destroy?
//	gtk_widget_unparent(temp);
//	gtk_widget_destroy(temp);
	gtk_widget_hide(temp);

	monitoring_session_run(mon);
	
	return(0);
}

void gui_monitoring_toplevel(page_object *po)
{
	monitoring_session *mon;
	
	DEBUG(printf("%s():\n", __FUNCTION__));
	if(po && po->monitoring_vbox)
	{
		mon = monitoring_session_new("");
		mon->arg = po;
		gtk_signal_connect(GTK_OBJECT(po->monitoring_vbox), "destroy", GTK_SIGNAL_FUNC(gui_monitoring_destroy_session), mon);
		gui_monitoring_render(mon);
	}
}

void gui_monitoring_make_dialog(GtkWidget *w, page_object *po)
{
	DEBUG(printf("%s():\n", __FUNCTION__));
}


void gui_monitoring_os(monitoring_session *session, monitoring_node *node)
{
	GtkWidget *combo;
	GList *combo_list = NULL;
	page_object *po = session->arg;
	GtkWidget *label;
	
	DEBUG(printf("%s():\n", __FUNCTION__));
	
	label = gtk_label_new("by:");
	gtk_box_pack_start(GTK_BOX(po->monitoring_fields_vbox), label, FALSE, FALSE, 5);
		
	DEBUG(printf("before gtk_combo_new()\n"));
	combo = gtk_combo_new();
	DEBUG(printf("after gtk_combo_new()\n"));
	monitoring_list_foreach(node, gui_monitoring_add_names_to_list, &combo_list);
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), combo_list);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
	gtk_box_pack_start(GTK_BOX(po->monitoring_fields_vbox), combo, FALSE, FALSE, 5);
	gtk_widget_show(combo);
	g_list_free(combo_list);

	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", GTK_SIGNAL_FUNC(gui_monitoring_append), session);
	gtk_object_set_user_data(GTK_OBJECT(GTK_COMBO(combo)->entry), node); // we are using the monitoring_node for reference
}


void gui_monitoring_port(monitoring_session *session, monitoring_node *node)
{
	GtkWidget *combo;
	GList *combo_list = NULL;
	page_object *po = session->arg;
	GtkWidget *label;
	
	DEBUG(printf("%s():\n", __FUNCTION__));
	
	label = gtk_label_new("by:");
	gtk_box_pack_start(GTK_BOX(po->monitoring_fields_vbox), label, FALSE, FALSE, 5);
		
	DEBUG(printf("before gtk_combo_new()\n"));
	combo = gtk_combo_new();
	DEBUG(printf("after gtk_combo_new()\n"));
	monitoring_list_foreach(node, gui_monitoring_add_names_to_list, &combo_list);
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), combo_list);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
	gtk_box_pack_start(GTK_BOX(po->monitoring_fields_vbox), combo, FALSE, FALSE, 5);
	gtk_widget_show(combo);
	g_list_free(combo_list);

	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", GTK_SIGNAL_FUNC(gui_monitoring_append), session);
	gtk_object_set_user_data(GTK_OBJECT(GTK_COMBO(combo)->entry), node); // we are using the monitoring_node for reference
}


void gui_monitoring_state(monitoring_session *session, monitoring_node *node)
{
	GtkWidget *combo;
	GList *combo_list = NULL;
	page_object *po = session->arg;
	GtkWidget *label;
	
	DEBUG(printf("%s():\n", __FUNCTION__));
	
	label = gtk_label_new("by:");
	gtk_box_pack_start(GTK_BOX(po->monitoring_fields_vbox), label, FALSE, FALSE, 5);
		
	DEBUG(printf("before gtk_combo_new()\n"));
	combo = gtk_combo_new();
	DEBUG(printf("after gtk_combo_new()\n"));
	monitoring_list_foreach(node, gui_monitoring_add_names_to_list, &combo_list);
	gtk_combo_set_popdown_strings(GTK_COMBO(combo), combo_list);
	gtk_entry_set_editable(GTK_ENTRY(GTK_COMBO(combo)->entry), FALSE);
	gtk_box_pack_start(GTK_BOX(po->monitoring_fields_vbox), combo, FALSE, FALSE, 5);
	gtk_widget_show(combo);
	g_list_free(combo_list);

	gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed", GTK_SIGNAL_FUNC(gui_monitoring_append), session);
	gtk_object_set_user_data(GTK_OBJECT(GTK_COMBO(combo)->entry), node); // we are using the monitoring_node for reference
}


void gui_monitoring_os_change(monitoring_session *session, monitoring_node *node)
{
	DEBUG(printf("%s():\n", __FUNCTION__));
}


void gui_monitoring_port_open(monitoring_session *session, monitoring_node *node)
{
	DEBUG(printf("%s():\n", __FUNCTION__));
}


void gui_monitoring_port_no_change(monitoring_session *session, monitoring_node *node)
{
	DEBUG(printf("%s():\n", __FUNCTION__));
}


void gui_monitoring_port_no_new_ports(monitoring_session *session, monitoring_node *node)
{
	DEBUG(printf("%s():\n", __FUNCTION__));
}


void gui_monitoring_state_ping_host(monitoring_session *session, monitoring_node *node)
{
	DEBUG(printf("%s():\n", __FUNCTION__));
}


void gui_monitoring_state_check_port(monitoring_session *session, monitoring_node *node)
{
	DEBUG(printf("%s():\n", __FUNCTION__));
}


#endif /* USING_MONITORING */
