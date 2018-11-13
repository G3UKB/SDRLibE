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

/*
#include <math.h>
#include "../../../../libs/fftw/fftw3.h"
#include "../helpers/defs.h"
#include "../helpers/utils.h"
#include "../ringbuffer/ringb.h"
#define HAVE_STRUCT_TIMESPEC
#include "../../../../libs/pthreads/include/pthread.h"
#include "../../../wdsp_win/src/channel.h"
#include "../../../wdsp_win/src/wisdom.h"
#include "../../../wdsp_win/src/bandpass.h"
#include "../audio/local_audio.h"
*/

// Data structures
// Interface between Cython and C Server
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
int DLL_EXPORT c_server_init(char* args);
int DLL_EXPORT c_server_start();
int DLL_EXPORT c_server_run_display(int display_run);
int DLL_EXPORT c_server_terminate();
int DLL_EXPORT c_server_open_channel(int ch_type, int ch_instance, int iq_size, int mic_size, int in_rate, int out_rate, int tdelayup, int tslewup, int tdelaydown, int tslewdown);
void DLL_EXPORT c_server_close_channel(int channel);
int DLL_EXPORT c_server_exchange(int n_smpls, int n_rx, int rate, int in_sz, char *ptr_in_bytes, int out_sz, char *ptr_out_bytes);
void DLL_EXPORT c_server_set_input_samplerate(int channel, int rate);
void DLL_EXPORT c_server_set_ch_state(int channel, int state, int mode);
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
int DLL_EXPORT c_server_open_display(int fft_size, int win_type, int sub_spans, int in_sz, int display_width, int average_mode, int over_frames, int sample_rate, int frame_rate);
void DLL_EXPORT c_server_set_display(int display_id, int fft_size, int win_type, int sub_spans, int in_sz, int display_width, int average_mode, int over_frames, int sample_rate, int frame_rate);
void DLL_EXPORT c_server_close_display(int display_id);
int DLL_EXPORT c_server_get_display_data(int display_id, void *display_data);
DLL_EXPORT char* c_server_enum_audio_inputs();
DLL_EXPORT char* c_server_enum_audio_outputs();
void DLL_EXPORT c_server_change_audio_outputs(int rx, char* audio_ch);
void DLL_EXPORT c_server_revert_audio_outputs();
void hanning_window(int size);
float get_pwr_wbs(int index);
void DLL_EXPORT c_server_process_wbs_frame(char *ptr_in_bytes);
int DLL_EXPORT c_server_get_wbs_data(int width, void *wbs_data);


#endif
