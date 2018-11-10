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

short *smpl_buffer;
unsigned char packet_buffer[FRAME_SZ];

// Local functions
static void set_metis_header(unsigned char *buffer);
static unsigned int fcd_get_freq();
static void set_usb_header(int offset, unsigned char *buffer);
static void set_freq(int offset, unsigned int freq, unsigned char *buffer);

// Module vars
pthread_mutex_t fcd_mutex = PTHREAD_MUTEX_INITIALIZER;
int freq_hz;
int last_freq = -1;

// Safe set frequency
void fcd_set_freq(unsigned int f) {
    pthread_mutex_lock(&fcd_mutex);
    freq_hz = f;
    pthread_mutex_unlock(&fcd_mutex);
}

// Thread entry point for UDP writer processing
void *udp_writer_imp(void* data){
    unsigned int new_freq;

    // Get our thread parameters
    udp_writer_thread_data* td = (udp_writer_thread_data*)data;
    int sd = td->socket;
    struct sockaddr_in *srv_addr = td->srv_addr;

    printf("Started UDP writer thread\n");

    while (td->terminate == FALSE) {
        if (td->run) {
			;
        } else {
            Sleep(0.1);
        }
    }
    printf("UDP Writer thread exiting...\n");
    return NULL;
}

//===================================================================
// Private functions
// Set Metis header bytes
static void set_metis_header(unsigned char *buffer) {
    buffer[0] = 0xEF;
    buffer[1] = 0xFE;
    buffer[2] = 0x01;   //Commasnd byte
    buffer[3] = 0x02;   // Endpoint
}
// Set UDP sync bytes
static void set_usb_header(int offset, unsigned char *buffer) {
    buffer[offset] = 0x7f;
    buffer[offset+1] = 0x7f;
    buffer[offset+2] = 0x7f;
}
// Set frequency bytes for RX1
static void set_freq(int offset, unsigned int freq, unsigned char *buffer) {
    buffer[offset] = 0x02;  // Freq RX1
    // Pack as big endian
    buffer[offset+1] = (freq >> 24) & 0xff;
    buffer[offset+2] = (freq >> 16) & 0xff;
    buffer[offset+3] = (freq >> 8) & 0xff;
    buffer[offset+4] = freq & 0xff;
}

// Safe get frequency
static unsigned int fcd_get_freq() {
    int f;
    pthread_mutex_lock(&fcd_mutex);
    f = freq_hz;
    pthread_mutex_unlock(&fcd_mutex);
    return f;
}
