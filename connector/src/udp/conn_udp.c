/*
conn_udp.c

Connector UDP interface

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

// Include all server headers
#include "../common/include.h"

// Local functions
static void *udp_conn_imp(void* data);
static void udpconndata(UDPConnThreadData* td);

// The socket
int connector_socket;
// Threads
pthread_t conn_udp_thd;
// Structure pointers
UDPConnThreadData *udp_conn_td = NULL;
// Our address structure
struct sockaddr_in conn_addr;

// Initialise module
int conn_udp_init() {

	int rc;

	// Create our UDP socket
	connector_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (connector_socket < 0) {
		printf("Connector: Failed to create UDP socket! [%d]", connector_socket);
		exit(-1);
	}

	// Bind local server port
	conn_addr.sin_family = AF_INET;
	struct in_addr addr;
	InetPton(AF_INET,"127,0,0,1", &addr);
	conn_addr.sin_addr.s_addr = addr.S_un.S_addr;
	conn_addr.sin_port = htons(CONN_SERVER_PORT);
	rc = bind(connector_socket, (struct sockaddr *) &conn_addr, sizeof(conn_addr));
	if (rc<0) {
		printf("Connector: Cannot bind port number %d\n", CONN_SERVER_PORT);
		exit(-1);
	}

	// Allocate thread data structure
	udp_conn_td = (ThreadData *)safealloc(sizeof(UDPConnThreadData), sizeof(char), "CONN_UDP_TD_STRUCT");
	// Init the thread data
	udp_conn_td->run = FALSE;
	udp_conn_td->terminate = FALSE;
	udp_conn_td->socket = connector_socket;

	// Create the connector thread
	rc = pthread_create(&conn_udp_thd, NULL, udp_conn_imp, (void *)udp_conn_td);
	if (rc != 0) {
		printf("Connector: Failed to create connector thread! [%d]\n", rc);
		exit(-1);
	}
}

//==========================================================================================
// Connector UDP thread

// Start connector thread
void conn_udp_start() {
	udp_conn_td->run = TRUE;
}

// Stop reader thread
void conn_udp_stop() {
	udp_conn_td->run = FALSE;
}

// Terminate the reader
void conn_udp_terminate() {
	/* Terminate reader thread
	*
	* Arguments:
	*
	*/

	int counter;

	udp_conn_td->run = FALSE;
	udp_conn_td->terminate = TRUE;

	// Signal the thread to ensure it sees the terminate
	// Wait for the thread to exit
	pthread_join(conn_udp_thd, NULL);

	// Free thread data
	safefree((char *)udp_conn_td);
}

// Thread entry point for processing
static void *udp_conn_imp(void* data) {
	// Get our thread parameters
	UDPConnThreadData* td = (UDPConnThreadData*)data;

	printf("Started UDP Connector thread\n");

	while (!td->terminate) {
		if (td->run && !td->terminate) {
			// While running we stay in the receive loop
			udpconndata(td);
		}
		else {
			Sleep(100);
		}
	}

	printf("UDP Connector thread exiting...\n");
	return NULL;
}

static void udpconndata(UDPConnThreadData* td) {

}