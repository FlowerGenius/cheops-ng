/*
 * Cheops Next Generation GUI
 * 
 * misc.c
 * Misc functions and variables and such
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

#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include "misc.h"
#include "logger.h"

int make_home_dir()
{
	/* 
	 * Make home directory for user, or quit.
	 */
	char buf[256];
	char *c;
	c = getenv("HOME");
	if (!c) {
		/* This is extremely important to everything */
		clog(LOG_ERROR, "No home directory!\n");
		exit(1);
	}
	snprintf(buf, sizeof(buf), "%s/.cheops", c);
	return mkdir(buf,0700);
}

int parse(char **argv, int args, char *s)
{
	/* Parse s in to no more than args tokens */
	int c = 1;
	int inquote=0;
	/*
	 * Divide command into tokens separated by spaces and tabs...  We handle
	 * double quotes to contain spaces.
	 */
	argv[0]=s;
	while(*s) {
		switch(*s) {
		case '\"':
			inquote = !inquote;
			if (inquote) {
				argv[c - 1]++;
				break;
			} else
				*s = ' ';
		case ' ':
		case '\t':
			if (!inquote) {
				/* In the case of whitespace, advance to the end of
			   	   the whitespace and make a new token */
				*s='\0';
				s++;
				while(*s && ((*s == ' ') || (*s == '\t'))) s++;
				if (*s) {
					argv[c] = s;
					c++;
				}
				s--;
			}
		}
		s++;
	}

	if (inquote)
		return -1;
	else
		return c;
	 
}

