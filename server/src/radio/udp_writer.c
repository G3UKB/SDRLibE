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

// Initialise writer thread
void writer_init(int sd, struct sockaddr_in *srv_addr) {
	/* Initialise writer
	*
	* Arguments:
	*
	*/

	int rc;

	// Clear data arrays
	memset(frame, 0, sizeof(frame));
	memset(frame_data, 0, sizeof(frame_data));

	return TRUE;
}

// Prime radio
void prime_radio(int sd, struct sockaddr_in *srv_addr) {
	// Send a few data frames with cc data to initialise the radio
	for (int i = 0; i < 4; i++) {
		// Encode into a frame
		encode_output_data(frame_data, frame);
		// Dispatch to radio
		if (sendto(sd, (const char*)frame, FRAME_SZ, 0, (struct sockaddr*) srv_addr, sizeof(*srv_addr)) == -1) {
			printf("UDP prime failed!\n");
		}
	}
}

// Send next packet to radio
void write_data(int sd, struct sockaddr_in *srv_addr) {
	if (ringb_read_space(rb_out) >= DATA_SZ * 2) {
		// Enough to satisfy the required output block
		ringb_read(rb_out, frame_data, DATA_SZ * 2);
		// Encode into a frame
		encode_output_data(frame_data, frame);
		// Dispatch to radio
		if (sendto(sd, (const char*)frame, FRAME_SZ, 0, (struct sockaddr*) srv_addr, sizeof(*srv_addr)) == -1) {
			printf("UDP dispatch failed!\n");
		}
	}
}


