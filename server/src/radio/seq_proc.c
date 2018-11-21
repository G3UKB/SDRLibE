/*
seq_proc.h

UDP sequence number check and generation

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

unsigned int MAX_SEQ = 0;
unsigned int EP2_SEQ = 0;
unsigned int EP4_SEQ = 0;
unsigned int EP6_SEQ_CHK = -1;
unsigned char be_seq[4] = { 0,0,0,0 };

// Local func declaration
static unsigned int big_to_little_endian(unsigned char* big_endian);
static unsigned char* little_to_big_endian(unsigned int little_endian);
static unsigned int next_seq(unsigned int seq);

// Set MAX_SEQ to unsigned int (32 bit) max value
void seq_init() {
	MAX_SEQ = (unsigned int)pow((double)2.0, (double)32.0)-1;
}

// Calculate and return next seq as a BE byte array
unsigned char* next_ep2_seq() {
	// Bump seq
	EP2_SEQ = next_seq(EP2_SEQ);
	//printf("Next seq %d\n", EP2_SEQ);
	// Return this as a byte array in BE format
	return little_to_big_endian(EP2_SEQ);
}

unsigned char*  next_ep4_seq() {
	// Bump seq
	EP4_SEQ = next_seq(EP4_SEQ);
	// Return this as a byte array in BE format
	return little_to_big_endian(EP4_SEQ);
}

// Check incoming EP6 seq number
void check_ep6_seq(unsigned char* ep6) {
	unsigned int seq = big_to_little_endian(ep6);
	if (EP6_SEQ_CHK == -1) {
		// First time so set to given sequence
		EP6_SEQ_CHK = seq;
	}
	else if (seq == 0) {
		// Cycled
		EP6_SEQ_CHK = 0;
	}
	else if (++EP6_SEQ_CHK != seq) {
		// Oops
		printf("Seq error, expected %d, got %d!\n", EP6_SEQ_CHK, seq);
		// Reset
		EP6_SEQ_CHK = seq;
	}
}

// Local functions
// Convert a 4 byte sequence in BE to an unsigned int LE
static unsigned int big_to_little_endian(unsigned char* big_endian) {
	unsigned int little_endian;
	little_endian = big_endian[0];
	little_endian = (little_endian << 8) | big_endian[1];
	little_endian = (little_endian << 8) | big_endian[2];
	little_endian = (little_endian << 8) | big_endian[3];
	return little_endian;
}

// Convert an unsigned int LE to a  4 byte sequence in BE
static unsigned char* little_to_big_endian(unsigned int little_endian) {
	be_seq[3] = little_endian & 0xff;			// move byte 3 to byte 0
	be_seq[2] = (little_endian >> 8) & 0xff;	// move byte 2 to byte 1
	be_seq[1] = (little_endian >> 16) & 0xff;	// move byte 1 to byte 2
	be_seq[0] = (little_endian >> 24) & 0xff;	// move byte 0 to byte 3
	return be_seq;
}

// Next sequence number cyclic
static unsigned int next_seq(unsigned int seq) {
	// This is a little endian seq number
	seq = seq++;
	if (seq > MAX_SEQ) {
		return 0;
	} else {
		return seq;
	}
}
