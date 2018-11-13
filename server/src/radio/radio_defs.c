/*
radio_defs.c

Common allocations for the SDRLibE library
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

int channel_number = 0;
int display_number = 0;
int input_samplerate = 48000;
int output_samplerate = 48000;

// Temporary buffers
unsigned char *iq = NULL;
unsigned char *mic = NULL;
char *local_mic = NULL;
float *pan = NULL;
int pan_sz;
int allocated = FALSE;
char message[100];

// Active audio input level
short peak_input_level = 0;

// Structure pointers
Args *pargs = NULL;
Pipeline *ppl = NULL;

// Structure defs
AudioDefault audioDefault;

// Ring buffers
ringb_t *rb_iq_in;
ringb_t *rb_mic_in;
ringb_t *rb_out;

// Condition variable
pthread_mutex_t pipeline_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pipeline_con = PTHREAD_COND_INITIALIZER;

// Local input running flag
int local_input_running = FALSE;

// WBS FFT
// Early firmware
#define WBS_SIZE 4096
// Later firmware
//#define WBS_SIZE 16384
#define WBS_SMOOTH 10
float wbs_results[WBS_SIZE * 2];
float wbs_smooth[WBS_SIZE * 2];
int wbs_smooth_cnt = 0;
int wait_smooth = 100;

float wbs_correction = 0.0f;
fftw_complex *wbs_in, *wbs_out;
float wbs_window[WBS_SIZE];
// The gain from the antenna socket to the input
//float wbs_gain_adjust = 55.0;   // This should be configured
float wbs_gain_adjust = 85.0;   // This should be configured
fftw_plan wbs_plan;
