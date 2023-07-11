/*
 * Cheops Next Generation GUI
 * 
 * Brent Priddy <toopriddy@mailcity.com>
 *
 * Copyright(C) 1999
 * 
 * Distributed under the terms of the GNU General Public License (GPL) Version
 *
 * Config file reader and parser
 *
 */

#ifndef _GUI_CONFIG_H
#define _GUI_CONFIG_H

#include <gnome.h>

char *config_get_agent_ip_address(void);
int *config_get_agent_usessl(void);

void read_config_file(GtkWidget *w, GtkWidget *fs);

void write_config_file(GtkWidget *w, GtkWidget *fs);

#endif /* _GUI_CONFIG_H */
