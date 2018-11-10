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
static void udprecvdata(int sd, struct sockaddr_in *cliAddr);

// Module vars
unsigned char frame[FRAME_SZ];

// Thread entry point for ALSA processing
void *udp_reader_imp(void* data){
    // Get our thread parameters
    udp_reader_thread_data* td = (udp_reader_thread_data*)data;
    int sd = td->socket;
    struct sockaddr_in *srv_addr = td->srv_addr;

    printf("Started UDP reader thread\n");

    while (td->terminate == FALSE) {
        if (td->run) {
            udprecvdata(sd, srv_addr);
        } else {
            Sleep(0.1);
        }
    }

    printf("UDP Reader thread exiting...\n");
    return NULL;
}


// Read and discard data but extract the tuning frequency
// Set the FCD to the frequency
// ToDo - Enhance for other sound card type devices
static void udprecvdata(int sd, struct sockaddr_in *srvAddr) {

    int i,j,n;
    int addr_sz = sizeof(*srvAddr);
    unsigned char acc[DATA_SZ*2];
    unsigned short data[NUM_SMPLS*2];

    // Read a frame size data packet
    n = recvfrom(sd, (char*)frame, FRAME_SZ, 0, (struct sockaddr*)srvAddr, &addr_sz);
    if(n == FRAME_SZ) {
        // We have a metis frame
        // First 8 bytes are the metis header, then 2x512 bytes of
        // Extract the I/Q data and add to the ring buffer
        // Get the raw byte sequence I=24bit,Q=24bit,Mic=16bit
        for (i=START_FRAME_1, j=0 ; i<END_FRAME_1 ; i++, j++) {
            acc[j] = frame[i];
        }
        for (i=START_FRAME_2, j=DATA_SZ ; i<END_FRAME_2 ; i++, j++) {
            acc[j] = frame[i];
        }
    }
}
