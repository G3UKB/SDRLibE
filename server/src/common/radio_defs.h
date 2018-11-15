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

// ToDo: COMMENT
#define DLL_EXPORT __declspec(dllexport)
//#define DLL_EXPORT __stdcall

#define LOCAL_SERVER_PORT 1024
#define FRAME_SZ 1032
#define DATA_SZ 504
#define START_FRAME_1 16
#define END_FRAME_1 520
#define START_FRAME_2 528
#define END_FRAME_2 1032
#define NUM_SMPLS 126
#define DATA_PKT 0x01
#define EP2 0x02
#define M_PI 3.14159265358979323846
// WBS FFT
// Early firmware
#define WBS_SIZE 4096
// Later firmware
//#define WBS_SIZE 16384
#define WBS_SMOOTH 10


// Global vars defined in radio_defs.c
// Declare extern to satisfy linker
extern int channel_number;
extern int display_number;
extern int input_samplerate;
extern int output_samplerate;
extern unsigned char *iq;
extern unsigned char *mic;
extern char *local_mic;
extern float *pan;
extern int pan_sz;
extern int allocated;
extern char message[];
extern short peak_input_level;
extern Args *pargs;
extern Pipeline *ppl;
extern AudioDefault audioDefault;
extern ringb_t *rb_iq_in;
extern ringb_t *rb_mic_in;
extern ringb_t *rb_out;
extern pthread_mutex_t pipeline_mutex;
extern pthread_cond_t pipeline_con;
extern int local_input_running;
extern float wbs_results[];
extern float wbs_smooth[];
extern int wbs_smooth_cnt;
extern int wait_smooth;
extern float wbs_correction;
extern fftw_complex *wbs_in;
extern fftw_complex *wbs_out;
extern float wbs_window[];
extern float wbs_gain_adjust;
extern fftw_plan wbs_plan;

#endif
