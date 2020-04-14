/*
evnt_udp.h

UDP Event Connector Header

Copyright (C) 2018 by G3UKB Bob Cowdery

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

The authors can be reached by email at:

bob@bobcowdery.plus.com

*/

#ifndef _evnt_udp_h
#define _evnt_udp_h

//==================================================================
// Connector thread
pthread_t udp_evnt_thd;

// Thread data structure for connector UDP reader/writer
typedef struct UDPEvntThreadData {
	int run_disp[3];
	int run_wbs;
	int terminate;
	int disp_1_socket;
	int disp_2_socket;
	int disp_3_socket;
	int wbs_socket;
	struct sockaddr_in* disp_1_addr;
	struct sockaddr_in* disp_2_addr;
	struct sockaddr_in* disp_3_addr;
	struct sockaddr_in* wbs_addr;
	int disp_period;
	int disp_width;
	int wbs_period;
	int wbs_width;
	char* disp_1_data;
	char* disp_2_data;
	char* disp_3_data;
}UDPEvntThreadData;

// Prototypes
int conn_evnt_udp_init();
void conn_set_disp_period(int period);
void conn_set_disp_width(int width);
void conn_disp_1_udp_start();
void conn_disp_2_udp_start();
void conn_disp_3_udp_start();
void conn_disp_1_udp_stop();
void conn_disp_2_udp_stop();
void conn_disp_3_udp_stop();
void conn_evnt_udp_terminate();

#endif