/*
 * Cheops Next Generation GUI
 * 
 * gui-pixmap.c
 * Functions to handle pixmap stuff for the gui
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

os_pixmap_list_t *os_pixmap_list;

void os_pixmap_list_default(void)
{
	os_pixmap_list_t *p, *prev;
	char **c;
	char *default_list[] = {
		"ATLAS",      "atlas800.xpm",
		"RedBack",    "redback.xpm",
		"Cisco",      "cisco.xpm",
		"Printer",    "printer.xpm",
		"Linux",      "linux.xpm",
		"Mac",        "mac.xpm",
		"OS X",       "macosx.xpm",
		"IBM",        "ibm.xpm",
		"Novell",     "novell.xpm",
		"UNIX",       "unix.xpm",
		"Windows",    "windows.xpm",
		"HP",         "hp.xpm",
		"BSD",        "bsd.xpm",
		"SCO",        "sco.xpm",
		"VMS",        "vms.xpm",
		"Solaris",    "solaris.xpm",
		"Hub",        "hub.xpm",
		"QNX",        "qnx.xpm",
		NULL
	};
	
	if(os_pixmap_list)
	{
		for(p = os_pixmap_list; p; )
		{
			os_pixmap_list = p->next;
					
			free(p->string);
			free(p->pixmap_name);
			prev = p->next;
			free(p);
			p = prev;
		}
	}
	
	for(c = &default_list[0]; c[0];c+=2)
		os_pixmap_list_add(*c, *(c + 1));
}	

os_pixmap_list_t *os_pixmap_list_add(char *string, char *pixmap)
{
	os_pixmap_list_t *p;
	
	for(p = os_pixmap_list; p; p = p->next)
	{
		if( 0 == strcmp(p->string, string) )
		{
			if(p->pixmap_name)
				free(p->pixmap_name);
			p->pixmap_name = makestring(pixmap);
			return(p);
		}
	}
	// if it was not there then just add one
	
	p = malloc(sizeof(os_pixmap_list_t));
	if(!p)
	{
		c_log(LOG_ERROR," we ran out of memory?");
		exit(1);
	}
	p->string = makestring(string);
	p->pixmap_name = makestring(pixmap);
	p->next = os_pixmap_list;
	os_pixmap_list = p;
	
	return(p);
}

os_pixmap_list_t *os_pixmap_list_change(os_pixmap_list_t *osp, char *string, char *pixmap)
{
	os_pixmap_list_t *p, *ret;
	
	for(p = os_pixmap_list; p; p = p->next)
	{
		if( osp == p )
		{
			if(p->pixmap_name)
				free(p->pixmap_name);
			if(p->string)
				free(p->string);
			p->string = makestring(string);
			p->pixmap_name = makestring(pixmap);
			
			ret = p;
		      	for(p = os_pixmap_list; p; p = p->next)
		      	{
		      		// we would be adding a duplicate one
		      		if( ret != p && 0 == strcmp(p->string, string ) )
		      		{
		      			os_pixmap_list_remove_ptr(ret);
		      			return(NULL);
		      		}
		      	}
			return(ret);
		}
	}

	// uh... something went wrong?!?!?
	return(NULL);
}

void os_pixmap_list_remove_ptr(os_pixmap_list_t *q)
{
	os_pixmap_list_t *p, *prev = NULL;
	
	for(p = os_pixmap_list; p; p = p->next)
	{
		if( p == q )
		{
			if(prev)
				prev->next = p->next;
			else
				os_pixmap_list = p->next;
				
			free(p->string);
			free(p->pixmap_name);
			free(p);
			break;
		}	
		prev = p;
	}
}

void os_pixmap_list_remove(char *string, char *pixmap)
{
	os_pixmap_list_t *p, *prev = NULL;
	
	for(p = os_pixmap_list; p; p = p->next)
	{
		if( (0 == strcmp(string, p->string)) && (0 == strcmp(pixmap, p->pixmap_name)) )
		{
			if(prev)
				prev->next = p->next;
			else
				os_pixmap_list = p->next;
				
			free(p->string);
			free(p->pixmap_name);
			free(p);
			break;
		}	
		prev = p;
	}
}

char *os_pixmap_list_get(char *string)
{
	os_pixmap_list_t *p;

	if(os_pixmap_list == NULL)
		os_pixmap_list_default(); //initalize it
	
	for(p = os_pixmap_list; p; p = p->next)
	{
		if(strstr(string,p->string))
			return(p->pixmap_name);
	}

	if(strlen(string) == 0)
		return("unknown.xpm");
			
	p = os_pixmaps_make_dialog(NULL, "unknown.xpm", string, main_window->window);

	if(p == NULL)
		return("unknown.xpm");
	else
		return(p->pixmap_name);
}


char *get_my_image_path(char *filename)
{
	char **c;
	static char buf[256];
	char *paths[] = {
		"./pixmaps/",
		DEFAULT_PATH "/pixmaps/",
		""
	};
	FILE *fp;
	char *home;
	
	if( (fp = fopen(filename, "r")) != NULL )
	{
		strcpy(buf, filename);
		return(buf);
	}
	
	for(c = &paths[0]; *c[0];c++)
	{
		if(strstr(*c, "~/"))
		{
			home = getenv("HOME");
			if(home)
			{
				strcpy(buf, home);
				strcat(buf, "/");
				strcat(buf, *c + 2);
			}
			else
				strcpy(buf, *c);
				
		}
		else		
			strcpy(buf,*c);

		strcat(buf,filename);
		
		if( (fp = fopen(buf,"r")) != NULL )
		{
			fclose(fp);
			return(buf);
		}
	}

	c_log(LOG_ERROR, "Uh where are my images. looking for '%s'\n",filename);
	for(c = &paths[0]; *c[0]; c++)
		c_log(LOG_ERROR, "   looked in '%s'\n",*c);
	return("");
}

GdkImlibImage *get_my_imlib_image(char *filename)
{
	GdkImlibImage *im = NULL;

	im = gdk_imlib_load_image(get_my_image_path(filename));
	
	if(!im)
		im = gdk_imlib_create_image_from_xpm_data(unknown_xpm);
	
	return(im);
}

void get_my_pixmap_and_mask(char *filename, GdkPixmap **pix, GdkBitmap **mask)
{
	char *file;
	
	file = get_my_image_path(filename);
	
	if(file)
	{
		*pix = gdk_pixmap_create_from_xpm( main_window->window->window , mask, NULL, file);
	}
	else
	{
		printf("\n notthere\n");
		*pix = NULL;
		*mask = NULL;
	}
}

