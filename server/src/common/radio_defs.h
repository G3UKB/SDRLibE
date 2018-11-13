/*
radio_defs.h

Common definitions for the SDRLibE library
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

#ifndef _radio_defs_h
#define _radio_defs_h

#define DLL_EXPORT __declspec(dllexport)
//#define DLL_EXPORT __stdcall

// Defines
#define TRUE 1
#define FALSE 0

#define FRAME_SZ 1032
#define DATA_SZ 504
#define START_FRAME_1 16
#define END_FRAME_1 520
#define START_FRAME_2 528
#define END_FRAME_2 1032
#define NUM_SMPLS 126

#define M_PI 3.14159265358979323846


int channel_number = 0;
int display_number = 0;
int input_samplerate = 48000;
int output_samplerate = 48000;

// Temporary buffers
static unsigned char *iq = NULL;
static unsigned char *mic = NULL;
static char *local_mic = NULL;
static float *pan = NULL;
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


#endif
