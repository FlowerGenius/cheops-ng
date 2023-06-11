/*
 * Cheops Next Generation GUI
 * 
 * Brent Priddy <toopriddy@mailcity.com>
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
 */
#ifndef GUI_SERVICE_H
#define GUI_SERVICE_H

#include "cheops-gui.h"

typedef struct _service_list_t {
	char                   *name;
	int                     port;
	int                     protocol;
	char                   *command;
	struct _service_list_t *next;
} service_list_t;

extern service_list_t *service_list;

extern int service_callback(page_object *po, int port, int protocol); 
extern service_list_t *service_list_add(char *name, int port, int protocol, char *string);
extern service_list_t *service_list_change(service_list_t *service, char *name, int port, int protocol, char *string);
extern void service_list_default(void);

extern void service_list_remove(int port, int protocol);
extern void service_list_remove_ptr(service_list_t *t);
extern char *service_list_get(int port, int protocol);

extern service_list_t *services_make_dialog(service_list_t *service, char *name, int port, int protocol, char *str, GtkWidget *parent);
extern os_pixmap_list_t *os_pixmaps_make_dialog(os_pixmap_list_t *os_pixmap, char *pix, char *str, GtkWidget *parent);

int run_command_callback(page_object *po, int port, int protocol, char *string);

#endif /* GUI_SERVICE_H */

