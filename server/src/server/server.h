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
	int fft_size;
	int window_type;
	int display_width;
	int av_mode;
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
	int local_audio_run;
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
// General
int c_server_init();
void c_server_set_num_rx(int num_rx);
void c_server_set_in_rate(int rate);
void c_server_set_out_rate(int rate);
void c_server_set_iq_blk_sz(int blk_sz);
void c_server_set_mic_blk_sz(int blk_sz);
void c_server_set_duplex(int duplex);
void c_server_set_fft_size(int size);
void c_server_set_window_type(int window_type);
void c_server_set_av_mode(int mode);
void c_server_set_display_width(int width);
int c_server_start();
int c_server_terminate();
int c_radio_discover();
int c_radio_start(int wbs);
int c_radio_stop();
void c_server_mox(int mox_state);
// RX & TX
void c_server_set_rx_mode(int channel, int mode);
void c_server_set_rx_filter_run(int channel, int run);
void c_server_set_rx_filter_freq(int channel, int low, int high);
void c_server_set_rx_filter_window(int channel, int window);
void c_server_set_agc_mode(int channel, int mode);
void c_server_set_rx_gain(int rx, float gain);
double c_server_get_rx_meter_data(int channel, int which);
void c_server_set_tx_mode(int channel, int mode);
void c_server_set_tx_filter_run(int channel, int run);
void c_server_set_tx_filter_freq(int channel, int low, int high);
void c_server_set_tx_filter_window(int channel, int window);
double c_server_get_tx_meter_data(int channel, int which);
void c_server_set_mic_gain(float gain);
void c_server_set_rf_drive(float drive);
short c_server_get_peak_input_level();
// Displays
void c_server_set_display(int ch_id, int display_width);
int c_server_get_display_data(int display_id, void *display_data);
void c_server_process_wbs_frame(char *ptr_in_bytes);
int c_server_get_wbs_data(int width, void *wbs_data);
// Audio
DeviceEnumList* c_server_enum_audio_inputs();
DeviceEnumList* c_server_enum_audio_outputs();
void c_server_set_audio_route(int direction, char* location, int receiver, char* host_api, char* dev, char* channel);
int c_server_clear_audio_routes();
int c_server_restart_audio_routes();
void c_server_change_audio_outputs(int rx, char* audio_ch);
void c_server_revert_audio_outputs();
int c_server_local_audio_run(int runstate);
// Wisdom
void c_server_make_wisdom(char *dir);
// CC data
void c_server_cc_out_alex_attn(int attn);
void c_server_cc_out_preamp(int preamp);
void c_server_cc_out_alex_rx_ant(int ant);
void c_server_cc_out_alex_rx_out(int out);
void c_server_cc_out_alex_tx_rly(int rly);
void c_server_cc_out_duplex(int duplex);
void c_server_cc_out_alex_auto(int state);
void c_server_cc_out_alex_hpf_bypass(int bypass);
void c_server_cc_out_lpf_30_20(int setting);
void c_server_cc_out_lpf_60_40(int setting);
void c_server_cc_out_lpf_80(int setting);
void c_server_cc_out_lpf_160(int setting);
void c_server_cc_out_lpf_6(int setting);
void c_server_cc_out_lpf_12_10(int setting);
void c_server_cc_out_lpf_17_15(int setting);
void c_server_cc_out_hpf_13(int setting);
void c_server_cc_out_hpf_20(int setting);
void c_server_cc_out_hpf_9_5(int setting);
void c_server_cc_out_hpf_6_5(int setting);
void c_server_cc_out_hpf_1_5(int setting);
void c_server_cc_out_set_rx_tx_freq(unsigned int freq_in_hz);
void c_server_cc_out_set_rx_2_freq(unsigned int freq_in_hz);
void c_server_cc_out_set_rx_3_freq(unsigned int freq_in_hz);
void c_server_cc_out_set_tx_freq(unsigned int freq_in_hz);

#endif
