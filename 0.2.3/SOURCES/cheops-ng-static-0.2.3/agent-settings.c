/*
 * Cheops Next Generation GUI
 * 
 * agent-settings.c
 * Handeler for changing the agent settings
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
#include "agent-discover.h"
#include "event.h"
#include "logger.h"

int handle_set_settings(event_hdr *h, event *e, agent *a)
{
	int flags = e->set_settings_e.flags;
	
	if(flags & SET_DISCOVER_RETRIES)
	{
		ping_retries = e->set_settings_e.discover_retries;
//		c_log(LOG_NOTICE, "I set the ping_retries to %d\n",ping_retries);
	}
	return(0);
}



