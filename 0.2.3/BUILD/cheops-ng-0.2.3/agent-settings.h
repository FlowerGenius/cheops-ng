/*
 * Cheops Next Generation GUI
 * 
 * agent-settings.h
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

#ifndef _CHEOPS_AGENT_SETTINGS_H
#define _CHEOPS_AGENT_SETTINGS_H

#include "event.h"

int handle_set_settings(event_hdr *h, event *e, agent *a);

#endif /* _CHEOPS_AGENT_SETTINGS_H */

