/*
 * Cheops Next Generation GUI
 * 
 * cheops-sh.c
 * An agent shell, for testing and communicating directly with agents
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
#include <unistd.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include "cheops.h"
#include "io.h"
#include "logger.h"
#include "event.h"
#include "cheops-sh.h"
#include "misc.h"

#ifdef DEBUG_CHEOPS_SH
	#define DEBUG(a) a
#else
	#define DEBUG(a)
#endif

static agent *primary_agent;

static void usage()
{
	fprintf(stderr, "Usage: cheops-agent [-n hostname|-l]\n"
	                "          -n: connect to agent on host <hostname>\n"
			"          -l: connect using local method\n");
	exit(1);
}

void handle_command()
{
	char *command = rl_line_buffer;
	char *toks[MAX_TOKENS];
	char *t;
	int cnt;
	if (command && *command)
		add_history(command);
	/*
	 * Make a copy we can work on...
	 */
	t = strdup(command);
	cnt = parse(toks, MAX_TOKENS, t);
	if (cnt >= 0) {
		execute_command(primary_agent, cnt, toks);
	} else if (cnt < 0) {
		printf("Unclosed quote\n");
	}
	free(t);
}

int handle_stdio(int *id, int fd, short events, void *cbdata)
{
	/*
	 * Let readline do its thing
	 */
	rl_callback_read_char();
	return 1;
}

static char **cheops_sh_completion(char *text, int start, int end)
{
	char **matches;
	int rs=0;
	matches = NULL;
	while((text[rs] == '\t') || (text[rs] == ' ')) rs++;
	if (start == rs) 
		matches = completion_matches(text, command_generator);
	return matches;
}

int main(int argc, char *argv[])
{
	char c;
	int local=0;
	int ipv4=0;
	char *hostname=NULL;

//	pth_init();

	while((c = getopt(argc, argv, "ln:")) >=0) {
		switch(c) {
		case 'l':
			local++;
			break;
		case 'n':
			ipv4++;
			hostname = optarg;
			break;
		default:
			usage();
		}
	}
	/* Local by default */
	if (!local && !ipv4)
		local++;
	printf("       Welcome to the Cheops Shell.\n");		
	printf("  This program is free software under the terms\n  of the GNU General Public License (GPL)\n\n");
	
	if (local) {
		if ((primary_agent = event_request_agent(AGENT_TYPE_LOCAL, NULL, 1))) {	
			printf("Connected to local agent.\n\n");
			register_shell_handlers();
			using_history();
			if (getenv("HOME")) {
				char filename[80];
				snprintf(filename, sizeof(filename), "%s/.cheops/history", getenv("HOME"));
				read_history(filename);
			}
			rl_callback_handler_install("cheops> ", handle_command);
			rl_completion_entry_function = (Function *)command_generator;
			rl_attempted_completion_function = cheops_sh_completion; 
			cheops_io_add(STDIN_FILENO, handle_stdio, CHEOPS_IO_IN, NULL);
			cheops_main();
		} else {
			clog(LOG_ERROR, "Unable to connect to local agent\n");
			exit(1);
		}
	}
	if (ipv4) {
		if ((primary_agent = event_request_agent(AGENT_TYPE_IPV4, hostname, 1))) {	
			printf("Connected to ipv4 agent.\n\n");
			register_shell_handlers();
			using_history();
			if (getenv("HOME")) {
				char filename[80];
				snprintf(filename, sizeof(filename), "%s/.cheops/history", getenv("HOME"));
				read_history(filename);
			}
			rl_callback_handler_install("cheops> ", handle_command);
			rl_completion_entry_function = (Function *)command_generator;
			rl_attempted_completion_function = cheops_sh_completion; 
			cheops_io_add(STDIN_FILENO, handle_stdio, CHEOPS_IO_IN, NULL);
			cheops_main();
		} else {
			clog(LOG_ERROR, "Unable to connect to local agent\n");
			exit(1);
		}
	}
	exit(0);	
}

