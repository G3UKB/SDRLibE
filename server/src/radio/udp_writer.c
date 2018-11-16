/*
udp_writer.c

Write UDP radio data stream

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

// Module vars
unsigned char frame[FRAME_SZ];
unsigned char frame_data[DATA_SZ * 2];
// Threads
pthread_t writer_thd;
// Structure pointers
UDPWriterThreadData *udp_writer_td = NULL;

// Initialise writer thread
void writer_init(int sd, struct sockaddr_in *srv_addr) {
	/* Initialise writer
	*
	* Arguments:
	*
	*/

	int rc;

	// Allocate thread data structure
	udp_writer_td = (UDPWriterThreadData *)safealloc(sizeof(UDPWriterThreadData), sizeof(char), "WRITER_TD_STRUCT");
	// Init the thread data with the Pipeline and Transforms pointers
	udp_writer_td->run = FALSE;
	udp_writer_td->terminate = FALSE;
	udp_writer_td->socket = sd;
	udp_writer_td->srv_addr = srv_addr;

	// Create the writer thread
	rc = pthread_create(&writer_thd, NULL, udp_writer_imp, (void *)udp_writer_td);
	if (rc) {
		return FALSE;
	}

	return TRUE;
}

// Start writer thread
void writer_start() {
	udp_writer_td->run = TRUE;
}

// Start writer thread
void writer_stop() {
	udp_writer_td->run = FALSE;
}

// Terminate the writer
int writer_terminate() {
	/* Terminate writer thread
	*
	* Arguments:
	*
	*/

	int counter;

	udp_writer_td->run = FALSE;
	udp_writer_td->terminate = TRUE;

	// Signal the thread to ensure it sees the terminate

	// Wait for the thread to exit
	pthread_join(writer_thd, NULL);

	// Free thread data
	safefree((char *)udp_writer_td);

	return TRUE;
}

// Thread entry point for UDP writer processing
void *udp_writer_imp(void* data){
    unsigned int new_freq;

    // Get our thread parameters
	UDPWriterThreadData* td = (UDPWriterThreadData*)data;
    int sd = td->socket;
    struct sockaddr_in *srv_addr = td->srv_addr;

    printf("Started UDP writer thread\n");

    while (td->terminate == FALSE) {
        if (td->run) {
			// See if enough data available in the ring buffer
			if (ringb_read_space(rb_out) >= DATA_SZ*2) {
				// Enough to satisfy the required output block
				ringb_read(rb_out, frame_data, DATA_SZ * 2);
				// Encode into a frame
				encode_output_data(frame_data, frame);
				// Dispatch to radio
				if (sendto(sd, (const char*)frame, FRAME_SZ, 0, (struct sockaddr*) srv_addr, sizeof(*srv_addr)) == -1) {
					printf("UDP dispatch failed!\n");
				}
			}
        } else {
            Sleep(10.0);
        }
		Sleep(1.0);
    }
    printf("UDP Writer thread exiting...\n");
    return NULL;
}

