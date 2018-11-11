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

unsigned int MAX_SEQ = 0;
unsigned int EP2_SEQ = 0;
unsigned int EP4_SEQ = 0;
unsigned int EP6_SEQ = 0;
unsigned int EP2_SEQ_CHK = -1;
unsigned char be_seq[4] = { 0,0,0,0 };

// Set MAX_SEQ to unsigned int (32 bit) max value
void seq_init() {
	MAX_SEQ = (unsigned int)pow(2, 32);
}

// Convert a 4 byte sequence in BE to an unsigned int LE
unsigned int big_to_little_endian(unsigned char* big_endian) {
	unsigned int little_endian = 
		((big_endian[3]) & 0xff) |			// move byte 3 to byte 0
		((big_endian[2]) & 0xff00) |		// move byte 2 to byte 1
		((big_endian[1]) & 0xff0000) |		// move byte 1 to byte 2
		((big_endian[0]) & 0xff000000);		// move byte 0 to byte 3
	return little_endian;
}

// Convert an unsigned int LE to a  4 byte sequence in BE
unsigned char* little_to_big_endian(unsigned int little_endian) {
	be_seq[3] = little_endian & 0xff;		// move byte 3 to byte 0
	be_seq[2] = little_endian & 0xff00;		// move byte 2 to byte 1
	be_seq[1] = little_endian & 0xff0000;	// move byte 1 to byte 2
	be_seq[0] = little_endian & 0xff000000;	// move byte 0 to byte 3
	return be_seq;
}

// Next sequence number cyclic
unsigned int next_seq(unsigned int seq) {
	// This is a little endian seq number
	unsigned int new_seq;
	if ((new_seq = seq++) > MAX_SEQ)
		return 0;
	else
		return new_seq;
}

// Calculate and return next seq as a BE byte array
unsigned char* next_ep2_seq() {
	// Bump seq
	EP2_SEQ = next_seq(big_to_little_endian(EP2_SEQ));
	// Return this as a byte array in BE format
	return little_to_big_endian(EP2_SEQ);
}

unsigned char*  next_ep4_seq() {
	// Bump seq
	EP4_SEQ = next_seq(big_to_little_endian(EP4_SEQ));
	// Return this as a byte array in BE format
	return little_to_big_endian(EP4_SEQ);
}

unsigned char*  next_ep6_seq() {
	// Bump seq
	EP6_SEQ = next_seq(big_to_little_endian(EP6_SEQ));
	// Return this as a byte array in BE format
	return little_to_big_endian(EP6_SEQ);
}

// Check incoming EP2 seq number
void check_ep2_seq(unsigned char* ep2) {
	unsigned int seq = big_to_little_endian(ep2);
	if (EP2_SEQ_CHK == -1) {
		// First time so set to given sequence
		EP2_SEQ_CHK = seq;
	}
	else if (seq == 0) {
		// Cycled
		EP2_SEQ_CHK = 0;
	}
	else if (EP2_SEQ_CHK++ != seq) {
		// Oops
		printf("Seq error, expected %d, got %d!", EP2_SEQ_CHK, seq);
		// Reset
		EP2_SEQ_CHK = seq;
	}
}
