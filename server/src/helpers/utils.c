/*
utils.c

'C' utility functions for the SdrScript SDR application

Copyright (C) 2014 by G3UKB Bob Cowdery

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

/*
Utility functions called from the Cython layer to implement:
    1. Procesing that is difficult in pure Python
    2. Time critical processing
*/

# include "utils.h"

void c_set_freq(double freq, char *ptr_bytes) {

    /* Set the frequency in the given byte array as required by Metis
     *
     * Arguments:
     *  freq       --  float frequency to set
     *  ptr_bytes  --  pointer to a 4 byte array to store big-endian
     *                 format 32 bit frequency in Hz
     */

    unsigned int freq_in_hz = (unsigned int)(freq*1e6);
    unsigned char* B0 = (unsigned char *)ptr_bytes+4;
    unsigned char* B1 = (unsigned char *)ptr_bytes+3;
    unsigned char* B2 = (unsigned char *)ptr_bytes+2;
    unsigned char* B3 = (unsigned char *)ptr_bytes+1;

	// MS byte in BO
	*B3 = (unsigned char)((freq_in_hz >> 24) & 0xFF);
	*B2 = (unsigned char)((freq_in_hz >> 16) & 0xFF);
	*B1 = (unsigned char)((freq_in_hz >> 8) & 0xFF);
	*B0 = (unsigned char)(freq_in_hz & 0xFF);
}

void send_message(char *type, char *msg) {

    // Just output for now
    printf("%s: %s-%s\n", "Lib", type, msg);
}


