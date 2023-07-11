/*
 * Cheops Next Generation GUI
 * 
 * gui-script.h
 * Functions used for scripts used on hosts
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

#ifndef _CHEOPS_SCRIPT_H
#define _CHEOPS_SCRIPT_H

#include <glib.h>

typedef struct _script_t {
	char *name;
	char *script;
	unsigned int flags;
} script_t;

#define SCRIPT_FLAGS_VIEWABLE  0x00000001


script_t *script_add(char *name, char *script, unsigned int flags);
script_t *script_change( script_t *s, char *name, char *script, unsigned int flags);
script_t *script_get(char *name);
GList *script_get_list(void);

void script_default(void);
void script_remove(char *name);
void script_remove_ptr(script_t *s);
void script_foreach(GFunc func, void *user_data);


#endif /* _CHEOPS_SCRIPT_H */
