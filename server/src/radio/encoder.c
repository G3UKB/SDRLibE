/*
encode.c

Encode a radio frame

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

void encode_output_data(char *data_frame, char *packet_buffer) {

	/*
	*	<0xEFFE><0x01><End Point><Sequence Number>< 2 x HPSDR frames>
	*	Where:
	*		End point = 1 byte[0x02 – representing USB EP2]
	*		Sequence Number = 4 bytes[32 bit unsigned]
	*		HPSDR data = 1024 bytes[2 x 512 byte USB format frames]
	*
	*	The following fields are merged :
	*		metis_header
	*		out_seq		-- next output sequence number to use
	*		cc_out 		-- round robin control bytes
	*		usb_header +
	*		proc_data	-- 2 frames worth of USB format frames
	*
	*	Data is encoded into the packet buffer
	*/

	int i,j;

	// Header
	packet_buffer[0] = 0xef;
	packet_buffer[1] = 0xfe;
	packet_buffer[2] = DATA_PKT;
	packet_buffer[3] = EP2;
	// Sequence number
	char *seq = next_ep2_seq();
	for( i=0; i<4; i++ ) {
		packet_buffer[4+i] = seq[i];
	}
	// First USB frame
	// Frame header
	for( i = 8; i < 10; i++ ) {
		packet_buffer[i] = 0x7f;
	}
	// Frame data
	for (i = 11, j = 0; i < 11+DATA_SZ; i++, j++) {
		packet_buffer[i] = data_frame[i];
	}
	// Second USB frame
	// Frame header
	for (i = 11+DATA_SZ ; i < 11 + 3 + DATA_SZ; i++) {
		packet_buffer[i] = 0x7f;
	}
	// Frame data
	for (i = 11 + 3 + DATA_SZ, j = 0; i < 11 + 3 + (2*DATA_SZ); i++, j++) {
		packet_buffer[i] = data_frame[j];
	}
}
