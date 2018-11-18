/*
server.h

'C' server API functions for the SdrScript SDR application

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

#ifndef _server_h
#define _server_h

// Data structures
typedef struct Tuple {
	int inst_id;
	int ch_id;
}Tuple;
typedef struct General {
	int in_rate;
	int out_rate;
	int iq_blk_sz;
	int mic_blk_sz;
	int duplex;
}General;
typedef struct Route {
	int rx;
	char srctype[30];
	char hostapi[50];
	char dev[50];
	char ch[10];
}Route;
typedef struct Routes {
	Route hpsdr[2];
	Route local[MAX_RX*2];
}Routes;
typedef struct Audio {
	char in_src[30];
	char in_hostapi[50];
	char in_dev[50];
	char out_sink[30];
	Routes routing;
}Audio;
typedef struct Args {
	// TX and RX allocations
	int num_tx;
	Tuple tx[MAX_TX];
	int num_rx;
	Tuple rx[MAX_RX];
	Tuple disp[MAX_RX];
	// Configuration
	General general;
	Audio audio;
}Args;

// Interface between Server and Pipeline thread
typedef struct LocalInput {
	int dsp_ch;
	int stream_id;
	PaStream *stream;
	int open;
	int prime;
	ringb_t *rb_la_in;
}LocalInput;
typedef struct LocalOutput {
	char srctype[30];
	char dev[50];
	int dsp_ch_left;
	int dsp_ch_right;
	int stream_id;
	PaStream *stream;
	int open;
	int prime;
	ringb_t *rb_la_out;
}LocalOutput;
typedef struct LocalAudio {
	LocalInput local_input;
	int num_outputs;
	LocalOutput local_output[MAX_RX*2];
}LocalAudio;
typedef struct Pipeline {
	int run;
	int display_run;
	int terminate;
	int terminating;
	ringb_t *rb_iq_in;
	ringb_t *rb_mic_in;
	ringb_t *rb_out;
	pthread_mutex_t *pipeline_mutex;
	pthread_cond_t *pipeline_con;
	Args *args;
	LocalAudio local_audio;
	float gain[MAX_RX];
	float mic_gain;
	float drive;
}Pipeline;

typedef struct AudioDefault {
    int rx_left;
    int rx_right;
}AudioDefault;

// Prototypes
int DLL_EXPORT c_server_init();
int DLL_EXPORT c_server_configure(char* args);
int DLL_EXPORT c_server_start();
int DLL_EXPORT c_radio_discover();
int DLL_EXPORT c_radio_start(int wbs);
int DLL_EXPORT c_radio_stop();
int DLL_EXPORT c_server_terminate();
void DLL_EXPORT c_server_set_input_samplerate(int channel, int rate);
void DLL_EXPORT c_server_set_dsp_sz(int channel, int sz);
void DLL_EXPORT c_server_set_tdelayup(int channel, int delay);
void DLL_EXPORT c_server_set_tslewup(int channel, int slew);
void DLL_EXPORT c_server_set_tdelaydown(int channel, int delay);
void DLL_EXPORT c_server_set_tslewdown(int channel, int slew);
void DLL_EXPORT c_server_make_wisdom(char *dir);
void DLL_EXPORT c_server_set_rx_mode(int channel, int mode);
void DLL_EXPORT c_server_set_rx_filter_run(int channel, int run);
void DLL_EXPORT c_server_set_rx_filter_freq(int channel, int low, int high);
void DLL_EXPORT c_server_set_rx_filter_window(int channel, int window);
void DLL_EXPORT c_server_set_agc_mode(int channel, int mode);
void DLL_EXPORT c_server_set_rx_gain(int rx, float gain);
double DLL_EXPORT c_server_get_rx_meter_data(int channel, int which);
void DLL_EXPORT c_server_set_tx_mode(int channel, int mode);
void DLL_EXPORT c_server_set_tx_filter_run(int channel, int run);
void DLL_EXPORT c_server_set_tx_filter_freq(int channel, int low, int high);
void DLL_EXPORT c_server_set_tx_filter_window(int channel, int window);
double DLL_EXPORT c_server_get_tx_meter_data(int channel, int which);
short DLL_EXPORT c_server_get_peak_input_level();
void DLL_EXPORT c_server_set_mic_gain(float gain);
void DLL_EXPORT c_server_set_rf_drive(float drive);
int DLL_EXPORT c_server_get_display_data(int display_id, void *display_data);
int DLL_EXPORT c_server_set_display(int ch_id, int display_width);
DLL_EXPORT char* c_server_enum_audio_inputs();
DLL_EXPORT char* c_server_enum_audio_outputs();
void DLL_EXPORT c_server_change_audio_outputs(int rx, char* audio_ch);
void DLL_EXPORT c_server_revert_audio_outputs();
void DLL_EXPORT c_server_process_wbs_frame(char *ptr_in_bytes);
int DLL_EXPORT c_server_get_wbs_data(int width, void *wbs_data);
void DLL_EXPORT c_server_cc_out_mox(int state);
void DLL_EXPORT c_server_cc_out_speed(int speed);
void DLL_EXPORT c_server_cc_out_10_ref(int ref);
void DLL_EXPORT c_server_cc_out_122_ref(int ref);
void DLL_EXPORT c_server_cc_out_config(int config);
void DLL_EXPORT c_server_cc_out_mic_src(int src);
void DLL_EXPORT c_server_cc_out_alex_attn(int attn);
void DLL_EXPORT c_server_cc_out_preamp(int preamp);
void DLL_EXPORT c_server_cc_out_alex_rx_ant(int ant);
void DLL_EXPORT c_server_cc_out_alex_rx_out(int out);
void DLL_EXPORT c_server_cc_out_alex_tx_rly(int rly);
void DLL_EXPORT c_server_cc_out_duplex(int duplex);
void DLL_EXPORT c_server_cc_out_num_rx(int num);
void DLL_EXPORT c_server_cc_out_alex_auto(int state);
void DLL_EXPORT c_server_cc_out_alex_hpf_bypass(int bypass);
void DLL_EXPORT c_server_cc_out_lpf_30_20(int setting);
void DLL_EXPORT c_server_cc_out_lpf_60_40(int setting);
void DLL_EXPORT c_server_cc_out_lpf_80(int setting);
void DLL_EXPORT c_server_cc_out_lpf_160(int setting);
void DLL_EXPORT c_server_cc_out_lpf_6(int setting);
void DLL_EXPORT c_server_cc_out_lpf_12_10(int setting);
void DLL_EXPORT c_server_cc_out_lpf_17_15(int setting);
void DLL_EXPORT c_server_cc_out_hpf_13(int setting);
void DLL_EXPORT c_server_cc_out_hpf_20(int setting);
void DLL_EXPORT c_server_cc_out_hpf_9_5(int setting);
void DLL_EXPORT c_server_cc_out_hpf_6_5(int setting);
void DLL_EXPORT c_server_cc_out_hpf_1_5(int setting);
void DLL_EXPORT c_server_cc_out_set_rx_1_freq(unsigned int freq_in_hz);
void DLL_EXPORT c_server_cc_out_set_rx_2_freq(unsigned int freq_in_hz);
void DLL_EXPORT c_server_cc_out_set_rx_3_freq(unsigned int freq_in_hz);
void DLL_EXPORT c_server_cc_out_set_tx_freq(unsigned int freq_in_hz);

void hanning_window(int size);
float get_pwr_wbs(int index);

#endif
