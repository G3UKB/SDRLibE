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

// Prototypes
unsigned char* cc_out_next_seq();
void cc_out_mox(int state);
void cc_out_speed(int speed);
void cc_out_10_ref(int ref);
void cc_out_122_ref(int ref);
void cc_out_config(int config);
void cc_out_mic_src(int src);
void cc_out_alex_attn(int attn);
void cc_out_preamp(int preamp);
void cc_out_alex_rx_ant(int ant);
void cc_out_alex_rx_out(int out);
void cc_out_alex_tx_rly(int rly);
void cc_out_duplex(int duplex);
void cc_out_num_rx(int num);
void cc_out_alex_auto(int state);
void cc_out_alex_hpf_bypass(int bypass);
void cc_out_lpf_30_20(int setting);
void cc_out_lpf_60_40(int setting);
void cc_out_lpf_80(int setting);
void cc_out_lpf_160(int setting);
void cc_out_lpf_6(int setting);
void cc_out_lpf_12_10(int setting);
void cc_out_lpf_17_15(int setting);
void cc_out_hpf_13(int setting);
void cc_out_hpf_20(int setting);
void cc_out_hpf_9_5(int setting);
void cc_out_hpf_6_5(int setting);
void cc_out_hpf_1_5(int setting);
void cc_out_set_rx_1_freq(unsigned int freq_in_hz);
void cc_out_set_rx_2_freq(unsigned int freq_in_hz);
void cc_out_set_rx_3_freq(unsigned int freq_in_hz);
void cc_out_set_tx_freq(unsigned int freq_in_hz);
void cc_out_init();
