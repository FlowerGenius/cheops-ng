/*
 * Cheops Next Generation GUI
 * 
 * gui-utils.h
 * GUI Utilities
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



#ifndef GUI_UTILS_H
#define GUI_UTILS_H

//#define DEBUGGING_MEMORY


#include <gnome.h>

/*
 * make a yes no dialog
 *
 * if you replace the no_callback then you MUST do a gtk_grab_remove on current_dialog !!!!
 * or call close_dialog(NULL,NULL)
 */
void make_yes_no_dialog(char *title,
			char *message,
			char *yes_label,
			char *no_label,
			void *yes_callback,
			void *yes_data,
			void *no_callback,
			void *no_data);

void close_dialog(GtkWidget *w, GtkWidget *dialog);

/* 
 * This makes a gnome yes no dialog box with a title message and default  button
 *
 * return: 0 if yes was pressed and 1 if no was pressed
 */
int make_gnome_yes_no_dialog(char *title, char *message, int no_default, int *ask_again);

/* 
 * This makes a gnome ok dialog box with a title message and default  button
 */
int make_gnome_ok_dialog(char *title, char *message);

/*
 * gtk callback function to focus another widget
 */
void gtk_widget_focus_me(GtkWidget *w, GtkWidget *it);


/*
 * Function to make a string from another one (returns NULL if it could not malloc the string)
 */
#ifndef DEBUGGING_MEMORY			
	extern char *makestring(char *s);
#else
	extern char *makestring_temp;
	
	#define makestring(s)										\
	(({															\
		if( NULL == (makestring_temp = malloc(strlen(s) + 1)) )	\
		{														\
			g_print("could not malloc\n");						\
		}														\
		strcpy(makestring_temp,s);								\
	}),makestring_temp)
#endif	

extern GtkWidget *current_dialog;


extern void kill_widget_argument(GtkWidget *w, GtkWidget *fs);

void click_ok_on_gnome_dialog(GtkWidget *w, GtkWidget *dialog);

void toggle_dialog_toggle(GtkWidget *w, int *value);

#endif /* GUI_UTILS_H */


