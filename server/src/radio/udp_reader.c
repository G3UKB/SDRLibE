/*
udp_reader.c

Read UDP radio data stream

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

// Includes
#include "../common/include.h"

// Local funcs
static void udprecvdata(UDPReaderThreadData* td);

// Module vars
unsigned char frame[FRAME_SZ];
unsigned char frame_data[DATA_SZ * 2];
// Threads
pthread_t reader_thd;
// Structure pointers
UDPReaderThreadData *udp_reader_td = NULL;

// Initialise reader thread
void reader_init(int sd, struct sockaddr_in *srv_addr, int num_rx, int num_smpls, int rate) {
	/* Initialise reader
	*
	* Arguments:
	*
	*/

	int rc;

	// Allocate thread data structure
	udp_reader_td = (ThreadData *)safealloc(sizeof(UDPReaderThreadData), sizeof(char), "READER_TD_STRUCT");
	// Init the thread data
	udp_reader_td->run = FALSE;
	udp_reader_td->terminate = FALSE;
	udp_reader_td->socket= sd;
	udp_reader_td->num_rx = num_rx;
	udp_reader_td->num_smpls = num_smpls;
	udp_reader_td->rate = rate;
	udp_reader_td->srv_addr = srv_addr;
	
	// Create the reader thread
	rc = pthread_create(&reader_thd, NULL, udp_reader_imp, (void *)udp_reader_td);
	if (rc) {
		return FALSE;
	}

	return TRUE;
}

// Start reader thread
void reader_start() {
	udp_reader_td->run = TRUE;
}

// Start reader thread
void reader_stop() {
	udp_reader_td->run = FALSE;
}

// Terminate the reader
int reader_terminate() {
	/* Terminate reader thread
	*
	* Arguments:
	*
	*/

	int counter;

	udp_reader_td->run = FALSE;
	udp_reader_td->terminate = TRUE;

	// Signal the thread to ensure it sees the terminate
	
	// Wait for the thread to exit
	pthread_join(reader_thd, NULL);
	
	// Free thread data
	safefree((char *)udp_reader_td);
	
	return TRUE;
}

// Thread entry point for processing
void *udp_reader_imp(void* data){
    // Get our thread parameters
	UDPReaderThreadData* td = (UDPReaderThreadData*)data;

    printf("Started UDP reader thread\n");

	while (!td->terminate) {
		if (td->run && !td->terminate) {
			// While running we stay in the receive loop
			udprecvdata(td);
		}
		else {
			Sleep(0.1);
		}
	}

    printf("UDP Reader thread exiting...\n");
    return NULL;
}

static void udprecvdata(UDPReaderThreadData* td) {

    int i,j,n;
    unsigned char acc[DATA_SZ*2];

	int num_rx = td->num_rx;
	int num_sampls = td->num_smpls;
	int rate = td->rate;
	int sd = td->socket;
	struct sockaddr_in *srv_addr = td->srv_addr;
	int addr_sz = sizeof(*srv_addr);

	// Loop receiving stream from radio
	while (td->run && !td->terminate) {
		// Read a frame size data packet
		n = recvfrom(sd, (char*)frame, FRAME_SZ, 0, (struct sockaddr*)srv_addr, &addr_sz);
		if (n == FRAME_SZ) {
			// We have a frame
			// First 8 bytes are the header, then 2x512 bytes of data
			// Then the sync and cc bytes are the start of each data frame
			// 
			for (i = START_FRAME_1, j = 0; i < END_FRAME_1; i++, j++) {
				frame_data[j] = frame[i];
			}
			for (i = START_FRAME_2, j = DATA_SZ; i < END_FRAME_2; i++, j++) {
				frame_data[j] = frame[i];
			}
			// Decode the frame and dispatch for processing
			// void frame_decode(int n_smpls, int n_rx, int rate, int in_sz, char *ptr_in_bytes) 
			frame_decode(num_rx, num_sampls, rate, DATA_SZ, frame_data);
		}
	}
}
