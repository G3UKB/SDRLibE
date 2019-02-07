/*
evnt_udp.c

SIMULATOR
Connector event UDP interface

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

//==========================================================================================
// Local functions
static void *udp_evnt_conn_imp(void* data);
static void udp_evnt_data(UDPEvntThreadData* td);
static void send_evnt_data(int sd, struct sockaddr* conn_cli_addr, char* data, int sz);
static void get_display_data();
static float get_meter_data();

//==========================================================================================
// The sockets
int disp_1_socket;
int disp_2_socket;
int disp_3_socket;
int wbs_socket;
// Thread
pthread_t conn_evnt_thd;
// Structure pointers
UDPEvntThreadData *udp_evnt_td = NULL;
// Client address structures
struct sockaddr_in disp_1_cli_addr;
struct sockaddr_in disp_2_cli_addr;
struct sockaddr_in disp_3_cli_addr;
struct sockaddr_in wbs_cli_addr;
// Data buffers
char *disp_1_data;
char *disp_2_data;
char *disp_3_data;

//==========================================================================================
// Initialise module
int conn_evnt_udp_init() {

	int rc;
	
	// Create our UDP socket
	disp_1_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (disp_1_socket == INVALID_SOCKET) {
		printf("Connector: Failed to create UDP display socket! [%d, %d]", disp_1_socket, WSAGetLastError());
		exit(-1);
	}
	// Create display and WBS client address structures
	disp_1_cli_addr.sin_family = AF_INET;
	if ((rc = inet_pton(AF_INET, "127.0.0.1", &(disp_1_cli_addr.sin_addr))) != 1) {
		if (rc == 0)
			printf("Connector: InetPton says invalid IPv4 dotted decimal address\n");
		else if (rc == -1)
			printf("Connector: InetPton error [%d]\n", WSAGetLastError());
		exit(-1);
	}
	disp_1_cli_addr.sin_port = htons(CLIENT_DISPLAY_PORT_1);
	
	disp_2_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (disp_2_socket == INVALID_SOCKET) {
		printf("Connector: Failed to create UDP display socket! [%d, %d]", disp_2_socket, WSAGetLastError());
		exit(-1);
	}
	disp_2_cli_addr.sin_family = AF_INET;
	if ((rc = inet_pton(AF_INET, "127.0.0.1", &(disp_2_cli_addr.sin_addr))) != 1) {
		if (rc == 0)
			printf("Connector: InetPton says invalid IPv4 dotted decimal address\n");
		else if (rc == -1)
			printf("Connector: InetPton error [%d]\n", WSAGetLastError());
		exit(-1);
	}
	disp_2_cli_addr.sin_port = htons(CLIENT_DISPLAY_PORT_2);

	disp_3_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (disp_3_socket == INVALID_SOCKET) {
		printf("Connector: Failed to create UDP display socket! [%d, %d]", disp_3_socket, WSAGetLastError());
		exit(-1);
	}
	disp_3_cli_addr.sin_family = AF_INET;
	if ((rc = inet_pton(AF_INET, "127.0.0.1", &(disp_3_cli_addr.sin_addr))) != 1) {
		if (rc == 0)
			printf("Connector: InetPton says invalid IPv4 dotted decimal address\n");
		else if (rc == -1)
			printf("Connector: InetPton error [%d]\n", WSAGetLastError());
		exit(-1);
	}
	disp_3_cli_addr.sin_port = htons(CLIENT_DISPLAY_PORT_3);

	wbs_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (wbs_socket == INVALID_SOCKET) {
		printf("Connector: Failed to create UDP WBS socket! [%d, %d]", wbs_socket, WSAGetLastError());
		exit(-1);
	}
	// Create a client address structure
	wbs_cli_addr.sin_family = AF_INET;
	if ((rc = inet_pton(AF_INET, "127.0.0.1", &(wbs_cli_addr.sin_addr))) != 1) {
		if (rc == 0)
			printf("Connector: InetPton says invalid IPv4 dotted decimal address\n");
		else if (rc == -1)
			printf("Connector: InetPton error [%d]\n", WSAGetLastError());
		exit(-1);
	}
	wbs_cli_addr.sin_port = htons(CLIENT_WBS_PORT);

	// Allocate data buffers
	disp_1_data = (UDPEvntThreadData *)safealloc(MAX_DISP_WIDTH * 4, sizeof(char), "CONN_EVNT_DISP_1_STRUCT");
	disp_2_data = (UDPEvntThreadData *)safealloc(MAX_DISP_WIDTH * 4, sizeof(char), "CONN_EVNT_DISP_2_STRUCT");
	disp_3_data = (UDPEvntThreadData *)safealloc(MAX_DISP_WIDTH * 4, sizeof(char), "CONN_EVNT_DISP_3_STRUCT");

	// Allocate thread data structure
	udp_evnt_td = (UDPEvntThreadData *)safealloc(sizeof(UDPEvntThreadData), sizeof(char), "CONN_EVNT_TD_STRUCT");
	// Init the thread data
	udp_evnt_td->run_disp[0] = FALSE;
	udp_evnt_td->run_disp[1] = FALSE;
	udp_evnt_td->run_disp[2] = FALSE;
	udp_evnt_td->run_wbs = FALSE;
	udp_evnt_td->terminate = FALSE;
	udp_evnt_td->disp_1_socket = disp_1_socket;
	udp_evnt_td->disp_2_socket = disp_2_socket;
	udp_evnt_td->disp_3_socket = disp_3_socket;
	udp_evnt_td->wbs_socket = wbs_socket;
	udp_evnt_td->disp_1_addr = &disp_1_cli_addr;
	udp_evnt_td->disp_2_addr = &disp_2_cli_addr;
	udp_evnt_td->disp_3_addr = &disp_3_cli_addr;
	udp_evnt_td->wbs_addr = &wbs_cli_addr;
	udp_evnt_td->disp_period = DISP_PERIOD;
	udp_evnt_td->disp_width = DISPLAY_WIDTH;
	udp_evnt_td->disp_1_data = disp_1_data;
	udp_evnt_td->disp_2_data = disp_2_data;
	udp_evnt_td->disp_3_data = disp_3_data;
	udp_evnt_td->wbs_period = DISP_PERIOD;
	udp_evnt_td->wbs_width = DISPLAY_WIDTH;

	// Create the event thread
	rc = pthread_create(&conn_evnt_thd, NULL, udp_evnt_conn_imp, (void *)udp_evnt_td);
	if (rc != 0) {
		printf("Connector: Failed to create Evnt thread! [%d]\n", rc);
		exit(-1);
	}
	printf("Connector: Initialised Connector Event UDP interface\n");
}

//==========================================================================================
// ConnectorEvent  UDP thread

// Set display period
void conn_set_disp_period (int period){
	udp_evnt_td->disp_period = period;
}

// Set display width
void conn_set_disp_width(int width) {
	udp_evnt_td->disp_width = width;
}

// Start sending display/wbs data
void conn_disp_1_udp_start() {
	udp_evnt_td->run_disp[0] = TRUE;
}
void conn_disp_2_udp_start() {
	udp_evnt_td->run_disp[1] = TRUE;
}
void conn_disp_3_udp_start() {
	udp_evnt_td->run_disp[2] = TRUE;
}
void conn_wbs_udp_start() {
	udp_evnt_td->run_wbs = TRUE;
}

// Stop sending display/wbs data
void conn_disp_1_udp_stop() {
	udp_evnt_td->run_disp[0] = FALSE;
}
void conn_disp_2_udp_stop() {
	udp_evnt_td->run_disp[1] = FALSE;
}
void conn_disp_3_udp_stop() {
	udp_evnt_td->run_disp[2] = FALSE;
}
void conn_wbs_udp_stop() {
	udp_evnt_td->run_wbs = FALSE;
}

// Terminate the reader
void conn_evnt_udp_terminate() {
	/* Terminate reader thread
	*
	* Arguments:
	*
	*/

	int counter;

	udp_evnt_td->run_disp[0] = FALSE;
	udp_evnt_td->run_disp[1] = FALSE;
	udp_evnt_td->run_disp[2] = FALSE;
	udp_evnt_td->run_wbs = FALSE;
	udp_evnt_td->terminate = TRUE;

	// Signal the thread to ensure it sees the terminate
	// Wait for the thread to exit
	pthread_join(conn_evnt_thd, NULL);

	// Free thread data
	safefree((char *)udp_evnt_td);
}

// Thread entry point for processing
static void *udp_evnt_conn_imp(void* data) {
	// Get our thread parameters
	UDPEvntThreadData* td = (UDPEvntThreadData*)data;

	printf("Connector: Started UDP Event Connector thread\n");

	while (1) {
		if (td->terminate)
			break;
		else
			udp_evnt_data(td);
	}

	printf("Connector: UDP Event Connector thread exiting...\n");
	return NULL;
}

//==========================================================================================
// UDP Evente Dispatch
static void udp_evnt_data(UDPEvntThreadData* td) {

	// Extract thread data
	struct sockaddr_in* disp_1_addr = td->disp_1_addr;	// Corresponding addresses
	struct sockaddr_in* disp_2_addr = td->disp_2_addr;	//
	struct sockaddr_in* disp_3_addr = td->disp_3_addr;	//
	struct sockaddr_in* wbs_addr = td->wbs_addr;		//
	float meter_data = 0.0;

	// Local vars
	const int TICK_PERIOD = 25;		// Wake every 25 ms
	int disp_acc = 0;
	int wbs_acc = 0;

	//=======================================================================================
	// Loop sending data to client
	while(!td->terminate) {
		if (td->run_disp[0] || td->run_disp[1] || td->run_disp[2]) {
			disp_acc++;
			if (disp_acc >= (td->disp_period / TICK_PERIOD)) {
				// Time to dispatch
				disp_acc = 0;
				//  printf("Send data\n");
				if (td->run_disp[0]) {
					// printf("Display 1\n");
					meter_data = get_meter_data();
					memcpy(td->disp_1_data, &meter_data, 4);
					get_display_data(td->disp_width, td->disp_1_data);
					// printf("Sending data\n");
					send_evnt_data(td->disp_1_socket, disp_1_addr, td->disp_1_data, td->disp_width * 4);
				}
				if (td->run_disp[1]) {
					// printf("Display 2\n");
					meter_data = get_meter_data();
					memcpy(td->disp_2_data, &meter_data, 4);
					get_display_data(td->disp_width, td->disp_2_data);
					send_evnt_data(td->disp_2_socket, disp_2_addr, td->disp_2_data, td->disp_width * 4);
				}
				if (td->run_disp[2]) {
					// printf("Display 3\n");
					meter_data = get_meter_data();
					memcpy(td->disp_3_data, &meter_data, 4);
					get_display_data(td->disp_width, td->disp_3_data);
					send_evnt_data(td->disp_3_socket, disp_3_addr, td->disp_3_data, td->disp_width * 4);
				}
				// printf("Done display data\n");
			}
		}
		if (td->run_wbs) {
			wbs_acc++;
			if (wbs_acc >= td->wbs_period / TICK_PERIOD) {
				// Time to dispatch
				wbs_acc = 0;

			}
		}
		Sleep(TICK_PERIOD);
	}
}

//==========================================================================================
// UDP Writer
static void send_evnt_data(int sd, struct sockaddr* conn_cli_addr, char* data, int sz) {
	int cli_addr_sz = sizeof(*conn_cli_addr);
	if (sendto(sd, (const char*)data, sz, 0, conn_cli_addr, &cli_addr_sz) == SOCKET_ERROR )
		printf("Connector: Failed to write event data! [%d]\n", WSAGetLastError());
}

// Simulated data
static float get_meter_data() {
	return -50.0;
}

static void get_display_data(int width, char* disp_data) {
	int i;
	float* data = (float*)disp_data;
	float value = -140.0;
	int dir = 0;
	for (i = 4; i < width + 4; i++) {
		data[i] = value;
		if (dir == 0) {
			value += 1;
			if (value >= -20.0)
				dir = 1;
		} else {
			value -= 1;
			if (value <= -140.0)
				dir = 0;
		}
	}
}
