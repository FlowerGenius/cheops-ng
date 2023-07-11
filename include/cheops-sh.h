/*
 * Cheops Next Generation GUI
 * 
 * cheops-sh.h
 * Header for cheops shell stuff
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

#ifndef _CHEOPS_SH_H
#define _CHEOPS_SH_H


extern int execute_command(agent *a, int argc, char *argv[]);
extern void register_shell_handlers();
extern char *command_generator(char *text, int state);
extern int parse(char **argv, int args, char *s);


#define MAX_TOKENS 80

#endif /* _CHEOPS_SH_H */


