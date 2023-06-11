/*
 * Cheops Next Generation GUI
 * 
 * gui-settings.h
 * Settings window stuff
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

#ifndef _GUI_SETTINGS_H
#define _GUI_SETTINGS_H

#include <gtk/gtk.h>

#define TCP_SCAN_0 "Connect Scan"
#define TCP_SCAN_1 "SYN Scan"
#define TCP_SCAN_2 "Stealth FIN Scan"
#define TCP_SCAN_3 "Stealth XMAS Scan"
#define TCP_SCAN_4 "Stealth NULL Scan"
#define TCP_SCAN_DEFAULT TCP_SCAN_0

#define TIMING_SCAN_NORMAL   "Normal"
#define TIMING_SCAN_PARANOID "Paranoid"
#define TIMING_SCAN_SNEAKY "Sneaky"
#define TIMING_SCAN_POLITE "Polite"
#define TIMING_SCAN_AGGRESSIVE "Aggressive"
#define TIMING_SCAN_INSANE "Insane"
#define TIMING_SCAN_DEFAULT TIMING_SCAN_NORMAL

extern int options_discover_retries;
#define OPTIONS_DISCOVER_RETRIES                        1

extern int options_operating_system_check;
#define OPTIONS_OPERATING_SYSTEM_CHECK                  2

extern int options_reverse_dns;
#define OPTIONS_REVERSE_DNS                             3

extern int options_tooltips_timeout;
#define OPTIONS_TOOLTIPS_TIMEOUT                        4

extern int options_confirm_delete;
#define OPTIONS_CONFIRM_DELETE                          5

extern int options_save_changes_on_exit;
#define OPTIONS_SAVE_CHANGES_ON_EXIT                    6

extern int options_use_ip_for_label;
#define OPTIONS_USE_IP_FOR_LABEL                        7

extern int options_map;
#define OPTIONS_MAP                                     8

extern int options_move_stuff_live;
#define OPTIONS_MOVE_STUFF_LIVE                         9

extern int options_probe_ports;
#define OPTIONS_PROBE_PORTS                            10

extern int options_rescan_at_startup;
#define OPTIONS_RESCAN_AT_STARTUP                      11

#define OPTIONS_OS_SCAN_PORTS_SIZE 100
extern char options_os_scan_ports[OPTIONS_OS_SCAN_PORTS_SIZE];
#define OPTIONS_OS_SCAN_PORTS                          12

extern int options_os_scan_udp_scan;
#define OPTIONS_OS_SCAN_UDP_SCAN                       13

#define OPTIONS_OS_SCAN_TCP_SCAN_SIZE 20
extern char options_os_scan_tcp_scan[OPTIONS_OS_SCAN_TCP_SCAN_SIZE];
#define OPTIONS_OS_SCAN_TCP_SCAN                       14

extern int options_os_scan_fastscan;
#define OPTIONS_OS_SCAN_FASTSCAN                       15

extern int options_os_scan_osscan;
#define OPTIONS_OS_SCAN_OSSCAN                         16

extern int options_os_scan_scan_specified_ports;
#define OPTIONS_OS_SCAN_SCAN_SPECIFIED_PORTS           17

extern int options_os_scan_dont_ping;
#define OPTIONS_OS_SCAN_DONT_PING                      18

extern unsigned int options_os_scan_flags;
#define OPTIONS_OS_SCAN_FLAGS                          19

extern int options_use_ip_for_merged_ports;
#define OPTIONS_USE_IP_FOR_MERGED_PORTS                20

#define OPTIONS_OS_SCAN_TIMING_SIZE 20
extern char options_os_scan_timing[OPTIONS_OS_SCAN_TIMING_SIZE];
#define OPTIONS_OS_SCAN_TIMING                         21

extern int options_os_scan_rpc_scan;
#define OPTIONS_OS_SCAN_RPC_SCAN                       22

extern int options_os_scan_identd_scan;
#define OPTIONS_OS_SCAN_IDENTD_SCAN                    23

extern int options_only_display_hostname;
#define OPTIONS_ONLY_DISPLAY_HOSTNAME                  24

/*
 * WARNING WARNING MAKE SURE THAT THIS IS THE VERY LAST OPTOIN
 * ****** WHICH MEANS THAT THIS NUMBER IS HIGHER THAN ALL OTHERS
 */
#define OPTIONS_LAST_OPTION                            25 




void do_settings(GtkWidget *widget, gpointer data);
void apply_the_setting(int flags);

void apply_all_settings(void);

#endif /* _GUI_SETTINGS_H */

