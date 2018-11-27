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
static void test1(cJSON *data);
static void test2(cJSON *data);

// The socket
int connector_socket;
// Threads
pthread_t conn_udp_thd;
// Structure pointers
UDPConnThreadData *udp_conn_td = NULL;
// Our address structure
struct sockaddr_in conn_srv_addr;
// Client address structure
struct sockaddr_in conn_cli_addr;

// Receive data packet
char data_in[CONN_DATA_SZ];

// Dispatcher table
// A dictionary entry
typedef struct { char* str; void(*fun_ptr)(cJSON *); }stringToFunc;
stringToFunc funcCases[] =
{
	{ "test1", &test1 },
	{ "test2", &test2 },
};

// Initialise module
int conn_udp_init() {

	int rc;
	
	// Create our UDP socket
	connector_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (connector_socket == INVALID_SOCKET) {
		printf("Connector: Failed to create UDP socket! [%d, %d]", connector_socket, WSAGetLastError());
		exit(-1);
	}

	// Bind local server port
	conn_srv_addr.sin_family = AF_INET;
	if ((rc=inet_pton(AF_INET, "127.0.0.1", &(conn_srv_addr.sin_addr))) != 1) {
		if( rc == 0)
			printf("Connector: InetPton says invalid IPv4 dotted decimal address\n");
		else if( rc == -1 ) 
			printf("Connector: InetPton error [%d]\n", WSAGetLastError());
		exit(-1);
	}
	conn_srv_addr.sin_port = htons(CONN_SERVER_PORT);
	rc = bind(connector_socket, (struct sockaddr *) &conn_srv_addr, sizeof(conn_srv_addr));
	if (rc != 0) {
		printf("Connector: Cannot bind port number [%d, %d]\n", CONN_SERVER_PORT, WSAGetLastError());
		exit(-1);
	}

	// Allocate thread data structure
	udp_conn_td = (ThreadData *)safealloc(sizeof(UDPConnThreadData), sizeof(char), "CONN_UDP_TD_STRUCT");
	// Init the thread data
	udp_conn_td->run = FALSE;
	udp_conn_td->terminate = FALSE;
	udp_conn_td->socket = connector_socket;
	udp_conn_td->srv_addr = &conn_srv_addr;

	// Create the connector thread
	rc = pthread_create(&conn_udp_thd, NULL, udp_conn_imp, (void *)udp_conn_td);
	if (rc != 0) {
		printf("Connector: Failed to create connector thread! [%d]\n", rc);
		exit(-1);
	}
	printf("Initialised Connector UDP interface\n");
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

//==========================================================================================
// UDP Reader
static void udpconndata(UDPConnThreadData* td) {

	// Extract thread data
	int sd = td->socket;
	struct sockaddr_in *srv_addr = td->srv_addr;

	// Local vars
	int svr_addr_sz = sizeof(*srv_addr);
	fd_set read_fd;
	FD_ZERO(&read_fd);
	FD_CLR(0, &read_fd);
	FD_SET(sd, &read_fd);
	struct timeval tv;
	tv.tv_sec = 10;
	tv.tv_usec = 0;
	int sel_result;
	int rd_sz;
	int cli_addr_sz = sizeof(conn_cli_addr);

	// Json vars
	cJSON * root;
	char name[30];

	// Loop receiving commands from client
	printf("Waiting for commands...\n");
	while (td->run && !td->terminate) {
		// Wait for data available
		sel_result = select(0, &read_fd, NULL, NULL, &tv);
		if (sel_result == 0) {
			// Timeout to check for termination etc
			printf("Timeout\n");
		}
		else if (sel_result == SOCKET_ERROR) {
			// Problem
			printf("Connector: Error in select! [%d]\n", WSAGetLastError());
			// Try to continue
		}
		else {
			// We have a command packet
			// Read a frame size data packet
			rd_sz = recvfrom(sd, (char*)data_in, CONN_DATA_SZ, 0, (struct sockaddr*)&conn_cli_addr, &cli_addr_sz);
			printf("Got data: %d", rd_sz);
			// Data is in Json encoding
			// Data format is of the following form:
			//	{
			//		"cmd" : "cmd_name",
			//		"params" : [list of command specific parameters]
			//	}
			// We only need to extract the cmd_name and look it up in the function pointer table then
			// pass it to the function to unpack the parameters and call the server side function.
			// This is not an RPC system. Calls are one way and if they fail then the client will
			// receive an async NAK with a reason. There is no ACK response. Data that eminates from 
			// server side, primarily display data is sent as and whan available or on a timer.
			//
			// Parse the incoming data
			root = cJSON_Parse(data_in);
			// Extract command name
			strcpy_s(name, sizeof(cJSON_GetObjectItemCaseSensitive(root, "cmd")->valuestring), cJSON_GetObjectItemCaseSensitive(root, "cmd")->valuestring);
			for (int i = 0; i < sizeof(funcCases); i++) {
				if (strcmp(funcCases[i].str, name) == 0) {
					(funcCases[i].fun_ptr)(root);
				}
			}
		}
	}
}

//==========================================================================================
// Execution functions

static void test1(cJSON *data) {
	printf("Test1\n");
}

static void test2(cJSON *data) {
	printf("Test2\n");
}