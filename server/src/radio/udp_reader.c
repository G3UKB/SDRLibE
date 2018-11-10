/*
udp_reader.c

Read data from UDP.

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
unsigned char frame[METIS_FRAME_SZ];
ringb_t *rb;                    // The audio ring buffer

// Thread entry point for ALSA processing
void *udp_reader_imp(void* data){
    // Get our thread parameters
    udp_thread_data* td = (udp_thread_data*)data;
    int sd = td->socket;
    struct sockaddr_in *srv_addr = td->srv_addr;
    rb = td->rb;

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
    unsigned char acc[USB_DATA_SZ*2];
    unsigned short data[NUM_SMPLS*2];

    // Read a frame size data packet
    n = recvfrom(sd, (char*)frame, METIS_FRAME_SZ, 0, (struct sockaddr*)srvAddr, &addr_sz);
    if(n == METIS_FRAME_SZ) {
        // We have a metis frame
        // First 8 bytes are the metis header, then 2x512 bytes of
        // Extract the I/Q data and add to the ring buffer
        // Get the raw byte sequence I=24bit,Q=24bit,Mic=16bit
        for (i=START_FRAME_1, j=0 ; i<END_FRAME_1 ; i++, j++) {
            acc[j] = frame[i];
        }
        for (i=START_FRAME_2, j=USB_DATA_SZ ; i<END_FRAME_2 ; i++, j++) {
            acc[j] = frame[i];
        }
        // Extract just the IQ data and convert to 16 bit LE
        for (i=0,j=0 ; i<USB_DATA_SZ*2 ; i+=8, j+=2) {
            // We take interleaved 24 bit BE IQ data and convert to 16 bit LE
            // I data
            data[j] = ((acc[i+2] << 8) | (acc[i+1] << 16) | (acc[i] << 24)) >> 8;
            data[j+1] = ((acc[i+5] << 8) | (acc[i+4] << 16) | (acc[i+3] << 24)) >> 8;
        }
        // Add to ring buffer
        if (ringb_write_space (rb) > NUM_SMPLS*4) {
            ringb_write (rb, (const char *)data, NUM_SMPLS*4);
        }
    }
}
