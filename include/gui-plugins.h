/*
 * Cheops Next Generation GUI
 * 
 * gui-plugins.h
 * Functions used for pixmaps 
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

#ifndef GUI_PLUGINS_H
#define GUI_PLUGINS_H

#include <glib.h>

typedef struct _plugin_list_t {
	char *(*name)(void);
	char *(*description)(void);
	void *handle;
} plugin_list_t;

// do not my any means use this var in your code!!!
extern GList *plugin_list;

plugin_list_t *plugin_list_add(char *string, char *pixmap);

plugin_list_t *plugin_list_change( plugin_list_t *osp, char *string, char *pixmap);

char *plugin_list_get(char *string);

void plugin_list_default(void);

void plugin_list_remove(char *string, char *pixmap);
void plugin_list_remove_ptr(plugin_list_t *q);

#endif /* GUI_PLUGIN_H */
