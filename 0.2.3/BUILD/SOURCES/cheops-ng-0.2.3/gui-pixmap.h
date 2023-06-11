/*
 * Cheops Next Generation GUI
 * 
 * gui-pixmap.h
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

#ifndef _GUI_PIXMAP_H
#define _GUI_PIXMAP_H

typedef struct _os_pixmap_list_t {
	char *string;
	char *pixmap_name;
	struct _os_pixmap_list_t *next;
} os_pixmap_list_t;

extern os_pixmap_list_t *os_pixmap_list;


os_pixmap_list_t *os_pixmap_list_add(char *string, char *pixmap);

os_pixmap_list_t *os_pixmap_list_change( os_pixmap_list_t *osp, char *string, char *pixmap);

char *os_pixmap_list_get(char *string);

void os_pixmap_list_default(void);

void os_pixmap_list_remove(char *string, char *pixmap);
void os_pixmap_list_remove_ptr(os_pixmap_list_t *q);

GdkImlibImage *get_my_imlib_image(char *filename);

char *get_my_image_path(char *filename);

void get_my_pixmap_and_mask(char *filename, GdkPixmap **pix, GdkBitmap **mask);

#endif /* _GUI_PIXMAP_H */
