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

// Forward refs
static void udprecvdata(udp_reader_thread_data* td);

// Module vars
unsigned char frame[FRAME_SZ];
unsigned char frame_data[DATA_SZ * 2];

// Thread entry point for processing
void *udp_reader_imp(void* data){
    // Get our thread parameters
    udp_reader_thread_data* td = (udp_reader_thread_data*)data;
    int sd = td->socket;
    struct sockaddr_in *srv_addr = td->srv_addr;

    printf("Started UDP reader thread\n");

	while (!td->terminate) {
		if (td->run && !td->terminate) {
			// Wile running we stay in the receive loop
			udprecvdata(td, srv_addr);
		}
		else {
			Sleep(0.1);
		}
	}

    printf("UDP Reader thread exiting...\n");
    return NULL;
}

static void udprecvdata(udp_reader_thread_data* td) {

    int i,j,n;
    unsigned char acc[DATA_SZ*2];

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
			// Dispatch the frame for processing

			// If output data then format and write to radio
		}
	}
}
