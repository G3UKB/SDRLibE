/*
conn_udp.h

UDP Connector Header

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

#ifndef _conn_udp_h
#define _conn_udp_h

//==================================================================
// Connector thread
pthread_t udp_conn_thd;

// Thread data structure for connector UDP reader/writer
typedef struct UDPConnThreadData {
	int run;
	int terminate;
	int socket;
	struct sockaddr_in *srv_addr;
}UDPConnThreadData;


// Prototypes
int conn_udp_init();
void conn_udp_start();
void conn_udp_stop();
void conn_udp_terminate();

#endif