/*
pipeline.h

'C' pipeline functions for the SdrScript SDR application

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

#ifndef _pipeline_h
#define _pipeline_h

#include "../helpers/defs.h"
#include "../helpers/utils.h"
#include "../server/server.h"
#include "../ringbuffer/ringb.h"
#define HAVE_STRUCT_TIMESPEC
#include "../../../../libs/pthreads/include/pthread.h"

// Transforms data structure maintains the data transforms between the input and output ring buffers
// The read size must be divisable by 8 to maintain proper boundaries for decoding as the data is organised
// as 2x24 bit (I/Q) and 1x16 bit (Mic).
// Samples are pre-processed so we only have valid samples to process. The in-sz is 1024 bytes which allows
// for 128 samples which is the minimum DSP block size.
// All other sizes are calculated as they vary depending on number of receivers and sample rate.
typedef struct Transforms {

	// ====================================================================================
	// Data from the input ring buffers rb_iq_in and rb_mic_in for processing
	// Format as above 2x24 bit (I/Q), 1x16 bit (Mic)
	unsigned int in_iq_sz;
	unsigned char *rd_iq_buff;
	unsigned int in_mic_sz;
	unsigned char *rd_mic_buff;

	// ====================================================================================
	// Decoded IQ data can be up to 8 receivers
	// The outputs are scaled as double with a range of +-1.0
	// Decoded size will depend on the number of receivers
	unsigned int dec_iq_sz;
	double *dec_iq_data[MAX_RX];
	// Decoded Mic data size is irrespective of number of receivers and is always 48K
	// However, at IQ sample rates > 48K the extra samples must be discarded.
	unsigned int dec_mic_sz;
	double *dec_mic_data;

	// ====================================================================================
	// Decoded data is fed into the appropriate DSP channel. The DSP operates at blk_sz samples.
	// The blk_sz is given in the Pipeline structure. Each DSP channel will therefore output
	// samples during the exchange according to the IQ sample rate. At 48K the output rate will
	// equal the input rate as samples are output at 48K
	// The output samples are left/right channels
	unsigned int dsp_lr_sz;
	double *dsp_lr_data[MAX_RX + MAX_TX];
	unsigned int dsp_iq_sz;
	double *dsp_iq_data;
	// Note the L/R DSP data is taken directly from these buffers for audio output when using local audio

	// ====================================================================================
	// Encoding is performed on the appropriate buffer(s) according to the HPSDR audio routing. This may be
	// a single buffer or two buffers pushing different receivers to left and right channel.
	// The IQ output data is added to give the final format
	// 16 bit Left, 16 bit Right, 16 bit I, 16 bit Q. A total of 8 bytes being the same size as the input but
	// a different format
	// The data is written directly to the output ring buffer rb_out from this buffer.
	unsigned int out_sz;
	unsigned char *out_buff;
}Transforms;

// Passed to the pipeline thread
typedef struct ThreadData {

	Pipeline *ppl;
	Transforms *ptr;

}ThreadData;

// Prototypes
int pipeline_init(Pipeline *td);
int pipeline_start();
int pipeline_run_display(int run_state);
int pipeline_stop();
int pipeline_terminate();
/*static void *pipeline_imp(void *data);
static void init_transform(Pipeline *td);
static void uninit_transform(Pipeline *td);
static void do_decode(Pipeline *td, Transforms *ptr);
static void do_display(Pipeline *ppl, Transforms *ptr);
static void do_dsp(Pipeline *td, Transforms *ptr);
static void do_local_audio(Pipeline *ppl, Transforms *ptr);
static void do_encode(Pipeline *td, Transforms *ptr);*/

#endif
