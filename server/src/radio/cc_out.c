/*
cc_out.h

Command bytes out processing

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

// Defs
#define MAX_CC 6

// Locking
pthread_mutex_t cc_out_mutex = PTHREAD_MUTEX_INITIALIZER;

// Enumerations for bit fields in the CC structure
// CC buffer index
enum cco_general {
	B_Gen,
	B_RX1_TX_F,
	B_RX1_F,
	B_RX2_F,
	B_RX3_F,
	B_MISC_1,
	B_MISC_2
};

// CC byte index
enum cco_byte_idx {
	CC0,
	CC1,
	CC2,
	CC3,
	CC4
};

// Speed
enum cco_speed {
	S_48kHz,
	S_96kHz,
	S_192kHz,
	S_384kHz,
};
unsigned char cco_speed_b[] = { 0x00, 0x01, 0x10, 0x11 };
unsigned char cco_speed_m = 0xfc;

// 10MHz ref
enum cco_10mhz_ref {
	R_10MHz_ATLAS,
	R_10MHz_PEN,
	R_10MHz_MERC
};
unsigned char cco_10mhz_ref_b[] = { 0x00,0x04,0x08 };
unsigned char cco_10mhz_ref_m = 0xf3;

// 122MHz ref
enum cco_122mhz_ref {
	R_122MHz_PEN,
	R_122MHz_MERC
};
unsigned char cco_122mhz_ref_b[] = { 0x00,0x10 };
unsigned char cco_122mhz_ref_m = 0xef;

// Board config
enum cco_board_config {
	BOARD_NONE,
	BOARD_PEN,
	BOARD_MERC,
	BOARD_BOTH
};
unsigned char cco_board_config_b[] = { 0x00,0x20,0x40,0x60 };
unsigned char cco_board_config_m = 0x9f;

// Mic src
enum cco_mic_src {
	MIC_JANUS,
	MIC_PEN
};
unsigned char cco_mic_src_b[] = { 0x00,0x80 };
unsigned char cco_mic_src_m = 0x7f;

// Alex attenuator
enum cco_alex_attn {
	ATTN_0db,
	ATTN_10db,
	ATTN_20db,
	ATTN_30db
};
unsigned char cco_alex_attn_b[] = { 0x00,0x01,0x10,0x11 };
unsigned char cco_alex_attn_m = 0xfc;

// Preamp
enum cco_preamp {
	PREAMP_OFF,
	PREAMP_ON
};
unsigned char cco_preamp_b[] = { 0x00,0x04 };
unsigned char cco_preamp_m = 0xfb;

// Alex RX ant
enum cco_rx_ant {
	RX_ANT_NONE,
	RX_ANT_1,
	RX_ANT_2,
	RX_ANT_XV
};
unsigned char cco_rx_ant_b[] = { 0x00,0x20,0x40,0x60 };
unsigned char cco_rx_ant_m = 0x9f;

// Alex RX out
enum cco_alex_rx_out {
	RX_OUT_OFF,
	RX_OUT_ON
};
unsigned char cco_alex_rx_out_b[] = { 0x00,0x80 };
unsigned char cco_alex_rx_out_m = 0x7f;

// Alex TX relay
enum cco_alex_tx_rly {
	TX_RLY_TX1,
	TX_RLY_TX2,
	TX_RLY_TX3
};
unsigned char cco_alex_tx_rly_b[] = { 0x00,0x01,0x10 };
unsigned char cco_alex_tx_rly_m = 0xfc;

// Duplex
enum cco_duplex {
	DUPLEX_OFF,
	DUPLEX_ON
};
unsigned char cco_duplex_b[] = { 0x00,0x04 };
unsigned char cco_duplex_m = 0xfb;

// No.RX
enum cco_num_rx {
	NUM_RX_1,
	NUM_RX_2,
	NUM_RX_3
};
unsigned char cco_num_rx_b[] = { 0x00,0x08,0x10 };
unsigned char cco_num_rx_m = 0xc7;

// ========================================
// Alex filters

// Alex auto
enum cco_alex_auto {
	ALEX_AUTO,
	ALEX_MANUAL
};
unsigned char cco_alex_auto_b[] = { 0x00,0x40 };
unsigned char cco_alex_auto_m = 0xbf;

// Alex HPF bypass
enum cco_hpf_bypass {
	ALEX_HPF_DISABLE,
	ALEX_HPF_ENABLE
};
unsigned char cco_hpf_bypass_b[] = { 0x00,0x20 };
unsigned char cco_hpf_bypass_m = 0xdf;

// Alex LPF/HPF select
enum cco_alex_lpf_hpf {
	ALEX_FILT_DISABLE,
	ALEX_FILT_ENABLE
};

// LPF Filter selects
unsigned char cco_alex_lpf_30_20_b[] = { 0x00,0x01 };
unsigned char cco_alex_lpf_30_20_m = 0xfe;
unsigned char cco_alex_lpf_60_40_b[] = { 0x00,0x02 };
unsigned char cco_alex_lpf_60_40_m = 0xfd;
unsigned char cco_alex_lpf_80_b[] = { 0x00,0x04 };
unsigned char cco_alex_lpf_80_m = 0xfb;
unsigned char cco_alex_lpf_160_b[] = { 0x00,0x08 };
unsigned char cco_alex_lpf_160_m = 0xf7;
unsigned char cco_alex_lpf_6_b[] = { 0x00,0x10 };
unsigned char cco_alex_lpf_6_m = 0xef;
unsigned char cco_alex_lpf_12_10_b[] = { 0x00,0x20 };
unsigned char cco_alex_lpf_12_10_m = 0xdf;
unsigned char cco_alex_lpf_17_15_b[] = { 0x00,0x40 };
unsigned char cco_alex_lpf_17_15_m = 0xbf;
// HPF Filter selects
unsigned char cco_alex_hpf_13_b[] = { 0x00,0x01 };
unsigned char cco_alex_hpf_13_m = 0xfe;
unsigned char cco_alex_hpf_20_b[] = { 0x00,0x02 };
unsigned char cco_alex_hpf_20_m = 0xfd;
unsigned char cco_alex_hpf_9_5_b[] = { 0x00,0x04 };
unsigned char cco_alex_hpf_9_5_m = 0xfb;
unsigned char cco_alex_hpf_6_5_b[] = { 0x00,0x08 };
unsigned char cco_alex_hpf_6_5_m = 0xf7;
unsigned char cco_alex_hpf_1_5_b[] = { 0x00,0x10 };
unsigned char cco_alex_hpf_1_5_m = 0xef;

// ============================================================================== =
// State vars
// Track the cc_out id
int cc_id = 0;
int cc_mox_state = FALSE;

// The 5 byte CC arrays
unsigned char cc_out_array[7][5] = 
{
	{ 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x02, 0x00, 0x00, 0x00, 0x00 },
	{ 0x04, 0x00, 0x00, 0x00, 0x00 },
	{ 0x06, 0x00, 0x00, 0x00, 0x00 },
	{ 0x08, 0x00, 0x00, 0x00, 0x00 },
	{ 0x12, 0x00, 0x00, 0x00, 0x00 },
	{ 0x14, 0x00, 0x00, 0x00, 0x00 },
};
unsigned char* cc_array;

// ========================================
// Get

// Get the next CC sequence(round robin)
unsigned char* cc_out_next_seq() {
	pthread_mutex_lock(&cc_out_mutex);
	// Get the array
	cc_array = cc_out_array[cc_id];
	
	// Manage the MOX bit
	if (cc_mox_state) {
		// Need to set the MOX bit
		cc_array[0] = cc_array[0] | 0x01;
	}
	else {
		// Need to reset the MOX bit
		cc_array[0] = cc_array[0] & 0xfe;
	}

	// Bump the cc_id
	cc_id++;
	if (cc_id > MAX_CC) cc_id = 0;

	pthread_mutex_unlock(&cc_out_mutex);
	return cc_array;
}

// ========================================
// Set
// Functions to set/reset bit fields in cc_out_array

// Helpers
// Get the given byte in the given buffer
unsigned char cc_out_get_byte (int cc_id, int cc_byte) {
	pthread_mutex_lock(&cc_out_mutex);
	unsigned char b = cc_out_array[cc_id][cc_byte];
	pthread_mutex_unlock(&cc_out_mutex);
	return b;
}

// Overwrite the given byte in the given buffer
void cc_out_put_byte(int cc_id, int cc_byte, unsigned char b) {
	pthread_mutex_lock(&cc_out_mutex);
	cc_out_array[cc_id][cc_byte] = b;
	pthread_mutex_unlock(&cc_out_mutex);
}

// Given a byte and a target bit field setting return the modified byte
unsigned char cc_out_set_bits(int target, unsigned char *bits_array, unsigned char bit_field, unsigned char bit_mask) {
	pthread_mutex_lock(&cc_out_mutex);
	unsigned char b = (bit_field & bit_mask) | bits_array[target];
	pthread_mutex_unlock(&cc_out_mutex);
	return b;
}

// Update the setting
void update_setting(cc_array_idx, cc_byte_idx, value, bit_array, bit_msk) {
	unsigned char b = cc_out_get_byte(cc_array_idx, cc_byte_idx);
	unsigned char new_b = cc_out_set_bits(value, bit_array, b, bit_msk);
	cc_out_put_byte(cc_array_idx, cc_byte_idx, new_b);
}

// ========================================
// Every bit field has a settings word
// Set / clear MOX
void cc_out_mox(int state) {
	if (state)
		cc_mox_state = TRUE;
	else
		cc_mox_state = FALSE;
}

// Configuration Settings
// Set the bandwidth
void cc_out_speed(int speed) {
	update_setting(CC1, B_Gen, speed, cco_speed_b, cco_speed_m);
}

// Set the 10MHz ref source
void cc_out_10_ref(int ref) {
	update_setting(CC1, B_Gen, ref, cco_10mhz_ref_b, cco_10mhz_ref_m);
}

// Set the 122.88MHz ref source
void cc_out_122_ref(int ref) {
	update_setting(CC1, B_Gen, ref, cco_122mhz_ref_b, cco_122mhz_ref_m);
}

// Set the board config
void cc_out_config(int config) {
	update_setting(CC1, B_Gen, config, cco_board_config_b, cco_board_config_m);
}

// Set the mic source
void cc_out_mic_src(int src) {
	update_setting(CC1, B_Gen, src, cco_mic_src_b, cco_mic_src_m);
}

// Set the alex attenuator
void cc_out_alex_attn(int attn) {
	update_setting(CC3, B_Gen, attn, cco_alex_attn_b, cco_alex_attn_m);
}

// Set the preamp
void cc_out_preamp(int preamp) {
	update_setting(CC3, B_Gen, preamp, cco_preamp_b, cco_preamp_m);
}

// Set the alex rx antenna
void cc_out_alex_rx_ant(int ant) {
	update_setting(CC3, B_Gen, ant, cco_rx_ant_b, cco_rx_ant_m);
}

// Set the alex rx output
void cc_out_alex_rx_out(int out) {
	update_setting(CC3, B_Gen, out, cco_alex_rx_out_b, cco_alex_rx_out_m);
}

// Set the alex tx relay
void cc_out_alex_tx_rly(int rly) {
	update_setting(CC4, B_Gen, rly, cco_alex_tx_rly_b, cco_alex_tx_rly_m);
}

// Set duplex
void cc_out_duplex(int duplex) {
	update_setting(CC4, B_Gen, duplex, cco_duplex_b, cco_duplex_m);
}

// Set num rx
void cc_out_num_rx(int num) {
	update_setting(CC4, B_Gen, num, cco_num_rx_b, cco_num_rx_m);
}

// ========================================
// Alex filters

// Set the alex auto mode
void cc_out_alex_auto(int state) {
	update_setting(CC2, B_MISC_1, state, cco_alex_auto_b, cco_alex_auto_m);
}

// Bypass Alex HPF
void cc_out_alex_hpf_bypass(int bypass) {
	update_setting(CC3, B_MISC_1, bypass, cco_hpf_bypass_b, cco_hpf_bypass_m);
}

// LPF Filter Select
// 30/20
void cc_out_lpf_30_20(int setting) {
	update_setting(CC4, B_MISC_1, setting, cco_alex_lpf_30_20_b, cco_alex_lpf_30_20_m);
}

// 60/40
void cc_out_lpf_60_40(int setting) {
	update_setting(CC4, B_MISC_1, setting, cco_alex_lpf_60_40_b, cco_alex_lpf_60_40_m);
}

// 80
void cc_out_lpf_80(int setting) {
	update_setting(CC4, B_MISC_1, setting, cco_alex_lpf_80_b, cco_alex_lpf_80_m);
}

// 160
void cc_out_lpf_160(int setting) {
	update_setting(CC4, B_MISC_1, setting, cco_alex_lpf_160_b, cco_alex_lpf_160_m);
}

// 6
void cc_out_lpf_6(int setting) {
	update_setting(CC4, B_MISC_1, setting, cco_alex_lpf_6_b, cco_alex_lpf_6_m);
}

// 12/10
void cc_out_lpf_12_10(int setting) {
	update_setting(CC4, B_MISC_1, setting, cco_alex_lpf_12_10_b, cco_alex_lpf_12_10_m);
}

// 17/15
void cc_out_lpf_17_15(int setting) {
	update_setting(CC4, B_MISC_1, setting, cco_alex_lpf_17_15_b, cco_alex_lpf_17_15_m);
}

// HPF Filter Select
// 13 MHz
void cc_out_hpf_13(int setting) {
	update_setting(CC3, B_MISC_1, setting, cco_alex_hpf_13_b, cco_alex_hpf_13_m);
}

// 20 MHz
void cc_out_hpf_20(int setting) {
	update_setting(CC3, B_MISC_1, setting, cco_alex_hpf_20_b, cco_alex_hpf_20_m);
}

// 9.5 MHz
void cc_out_hpf_9_5(int setting) {
	update_setting(CC3, B_MISC_1, setting, cco_alex_hpf_9_5_b, cco_alex_hpf_9_5_m);
}

// 6.5 MHz
void cc_out_hpf_6_5(int setting) {
	update_setting(CC3, B_MISC_1, setting, cco_alex_hpf_6_5_b, cco_alex_hpf_6_5_m);
}

// 1.5 MHz
void cc_out_hpf_1_5(int setting) {
	update_setting(CC3, B_MISC_1, setting, cco_alex_hpf_1_5_b, cco_alex_hpf_1_5_m);
}

// ========================================
// Frequency set
// NCO-1 is the RX and TX frequency when a single receiver is running in simplex mode.
// NCO-1 is the TX only frequency when running in duplex mode.
// NC0-2 to NCO-8 are the RX frequencies for RX 1-7 when running multiple receivers.
// NCO-1 and NC0-2 need to be set for a single RX in simplex
// Most significant byte is in C1

// NB Must take account of duplex
// Common set freq
void cc_out_common_set_freq(unsigned int freq_in_hz, unsigned char* cc_array) {
	cc_array[1] = (freq_in_hz >> 24) & 0xff;
	cc_array[2] = (freq_in_hz >> 16) & 0xff;
	cc_array[3] = (freq_in_hz >> 8) & 0xff;
	cc_array[4] = freq_in_hz & 0xff;
}

// Set the RX frequency
void cc_out_set_rx_1_freq(unsigned int freq_in_hz) {
	// Set NCO-1
	unsigned char* cc_array;
	cc_array = cc_out_array[B_RX1_TX_F];
	cc_out_common_set_freq(freq_in_hz, cc_array);
	// Set NCO-2
	cc_array = cc_out_array[B_RX1_F];
	cc_out_common_set_freq(freq_in_hz, cc_array);
}

void cc_out_set_rx_2_freq(unsigned int freq_in_hz) {
	// Set NCO-2
	unsigned char* cc_array = cc_out_array[B_RX2_F];
	cc_out_common_set_freq(freq_in_hz, cc_array);
}

void cc_out_set_rx_3_freq(unsigned int freq_in_hz) {
	// Set NCO-3
	unsigned char* cc_array = cc_out_array[B_RX3_F];
	cc_out_common_set_freq(freq_in_hz, cc_array);
}

void cc_out_set_tx_freq(unsigned int freq_in_hz) {
	// Set NCO-1
	unsigned char* cc_array = cc_out_array[B_RX1_TX_F];
	cc_out_common_set_freq(freq_in_hz, cc_array);
}

// Initialise the CC arrays
void cc_out_init() {
	// Set some sensible default values
	cc_out_mox( FALSE );
	cc_out_speed(S_48kHz);
	cc_out_10_ref(R_10MHz_MERC);
	cc_out_122_ref(R_122MHz_MERC);
	cc_out_config(BOARD_BOTH);
	cc_out_mic_src(MIC_PEN);
	cc_out_alex_attn(ATTN_0db);
	cc_out_preamp(PREAMP_OFF );
	cc_out_alex_rx_ant(RX_ANT_NONE);
	cc_out_alex_rx_out(RX_OUT_OFF );
	cc_out_alex_tx_rly(TX_RLY_TX1);
	cc_out_duplex(DUPLEX_OFF);
	cc_out_num_rx( NUM_RX_1);
	cc_out_set_rx_1_freq( 7100000 );
}