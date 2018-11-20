/*
pipeline.c

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

/*
Pipeline processing called from the Cython layer to implement:
	1. Decoding of USB format frames
	2. Signal processing
	3. Encoding of USB format frames
*/

// Includes
#include "../common/include.h"

// Local functions
static void *pipeline_imp(void *data);
static void init_transform(Pipeline *td);
static void uninit_transform(Pipeline *td);
static void do_decode(Pipeline *td, Transforms *ptr);
static void do_display(Pipeline *ppl, Transforms *ptr);
static void do_dsp(Pipeline *td, Transforms *ptr);
static void do_local_audio(Pipeline *ppl, Transforms *ptr);
static void do_encode(Pipeline *td, Transforms *ptr);

// Threads
pthread_t pipeline_thd;

// Structure pointers
Transforms *ptr = NULL;
ThreadData *td = NULL;

// Temp buffers
char *f_local_audio;
float *f_display;
int donep = FALSE;
char message[100];

// Interface functions
int pipeline_init(Pipeline *ppl) {

	/* Initialise pipeline
	 *
	 * Arguments:
	 * 	ppl			--	pointer to Pipeline data structure
	 *
	 */

	int rc;

	// Allocate Transforms structure
	ptr = (Transforms *)safealloc(sizeof(Transforms), sizeof(char), "TRANSFORMS_STRUCT");
	// Initialise our Transform structure
	init_transform(ppl);

	// Allocate thread data structure
	td = (ThreadData *)safealloc(sizeof(ThreadData), sizeof(char), "TD_STRUCT");
	// Init the thread data with the Pipeline and Transforms pointers
	td->ppl = ppl;
	td->ptr = ptr;

	// Create the pipeline thread
	rc = pthread_create(&pipeline_thd, NULL, pipeline_imp, (void *)td);
	if (rc){
	   return FALSE;
	}

	// Allocate the local audio temp buffer
	f_local_audio = safealloc(ptr->dsp_lr_sz*2, sizeof(char), "TEMP_AUDIO_BUFF");

	// Allocate the display temp buffer
	f_display = (float *)safealloc(ppl->args->general.iq_blk_sz * 2, sizeof(float), "TEMP_DISPLAY_BUFF");

	return TRUE;
}

int pipeline_start() {
	/* Run pipeline
	 *
	 * Arguments:
	 *
	 */

	td->ppl->run = TRUE;
	return TRUE;
}

int pipeline_stop() {
	/* Stop pipeline
	 *
	 * Arguments:
	 *
	 */

	td->ppl->run = FALSE;
	return TRUE;
}

int pipeline_run_display(int run_state) {
	/* Stop or start display activity
	 *
	 * Arguments:
	 * 	run_state	--	TRUE is run display
	 *
	 */
	td->ppl->display_run = run_state;
	return TRUE;
}

int pipeline_terminate() {
	/* Terminate pipeline thread
	 *
	 * Arguments:
	 *
	 */

	int counter;

	td->ppl->run = FALSE;
	td->ppl->display_run = FALSE;
	td->ppl->terminate = TRUE;

	// Signal the thread to ensure it sees the terminate
	counter = 10;
	while (!td->ppl->terminating) {
		pthread_cond_signal(td->ppl->pipeline_con);
		pthread_mutex_unlock(td->ppl->pipeline_mutex);
		if (counter-- <= 0) {
			return FALSE;
		} else {
			Sleep(100);
		}
	}
	// Wait for the thread to exit
	pthread_join(pipeline_thd, NULL);
	// Uninit local audio
	audio_uninit();
	// Uninitialise our Transform data structure
	uninit_transform(td->ppl);
	// Free thread data
	safefree((char *)td);
	// Free temp buffers
	safefree((char *)f_local_audio);
	safefree((char *)f_display);

	return TRUE;
}

// ===========================================================================
// Pipeline implementation
// Runs on a separate thread
static void *pipeline_imp(void *data) {
	/* Pipeline thread
	 * Manages decode/dsp/encode operations
	 *
	 * Arguments:
	 * 	data	--	pointer to ThreadData structure
	 *
	 */

	// Cast the parameter to a Pipeline structure
	ThreadData* td = (ThreadData*)data;
	Pipeline *ppl = (Pipeline *)td->ppl;
	Transforms *ptr = (Transforms *)td->ptr;
	int data_available;

	// Run until terminated
	while (!ppl->terminate) {
		// Wait for work
		pthread_mutex_lock(ppl->pipeline_mutex);
		pthread_cond_wait(ppl->pipeline_con, ppl->pipeline_mutex);
		
		// Check if we were woken for a termination
		if (ppl->terminate) {
			break;
		}
		data_available = FALSE;
		// Sync up on data to make sure we read both IQ and Mic data
		if ((ringb_read_space (ppl->rb_iq_in) >= ptr->in_iq_sz) && (ringb_read_space (ppl->rb_mic_in) >= ptr->in_mic_sz)) {
			ringb_read (ppl->rb_iq_in, (char *)ptr->rd_iq_buff, ptr->in_iq_sz);
			ringb_read (ppl->rb_mic_in, (char *)ptr->rd_mic_buff, ptr->in_mic_sz);
			data_available = TRUE;
		}
		// Run the pipeline
		if (data_available) {
            //printf("Data available\n");
			do_decode(ppl, ptr);
            //printf("do_decode\n");
			if (ppl->display_run) {
				do_display(ppl, ptr);
				//printf("do_display\n");
			}
			do_dsp(ppl, ptr);
			//printf("do_dsp\n");
			do_local_audio(ppl, ptr);
			//printf("do_local_audio\n");
			do_encode(ppl, ptr);
			//printf("do_encode\n");
			// Write bytes to the output ring
			if (ringb_write_space (ppl->rb_out) >= ptr->out_sz) {
				ringb_write (ppl->rb_out, (char *)ptr->out_buff, ptr->out_sz);
			}
		}
	}
	ppl->terminating = TRUE;
	return (void*)0;
}

static void init_transform(Pipeline *ppl) {
	/* Initialise the Transform structure
	 *
	 * Arguments:
	 * 	ppl		--	the Pipeline data structure
	 *
	 */

	int i;
	// The transform structure is split into a section for each data transform.

	//========================================================================
	// Data in
	// Note there are two ring buffers -
	// IQ
	// Contiguous samples interleaved for 1-n receivers
	// Mic
	// Contiguous samples for mono mic input
	//
	// We must arrange for the output data from the pipeline to be the same
	// number of samples for all channels so it can be combined into the output ring buffer.

	// Variable ptr points to the common transforms structure.
	// The block sizes are default 1024 for a single RX. The mic block size then reduces
	// as the number of RX's increases as samples are duplicated. Block size can be changed
	// by configuration.

	// Set sizes and allocate buffers in the transform structure
	// There are 6 bytes per IQ sample per receiver
	ptr->in_iq_sz = ppl->args->general.iq_blk_sz*ppl->args->num_rx * 6;
	ptr->rd_iq_buff = (unsigned char *)safealloc(ptr->in_iq_sz, sizeof(char), "IN_IQ_BUFF");
	// Only 1 mic channel with 2 bytes per sample
	ptr->in_mic_sz = ppl->args->general.mic_blk_sz * 2;
	ptr->rd_mic_buff = (unsigned char *)safealloc(ptr->in_mic_sz, sizeof(char), "IN_MIC_BUFF");

	//========================================================================
	// Decoding
	// Data is decoded for each receiver (from the 24 bit big endian samples to doubles) into a decode array.
	// The DSP can accept any block size and will internally buffer until there are enough samples to process.
	// When the DSP channels were opened the size was set to be IQ size (RX) or mic size (TX).
	// Note block size is number of IQ samples so there are 2 doubles per sample.
	ptr->dec_iq_sz = ppl->args->general.iq_blk_sz * 2;
	// Allocate a buffer for each receiver
	for (i=0 ; i < ppl->args->num_rx ; i++ ) {
		ptr->dec_iq_data[i] = (double *)safealloc(ppl->args->general.iq_blk_sz * 2, sizeof(double), "DEC_IQ_BUFF");
	}
	// Allocate a buffer for mic input which is 1 16 bit value converted to a complex double
	ptr->dec_mic_sz = ppl->args->general.mic_blk_sz;
	ptr->dec_mic_data = (double *)safealloc(ppl->args->general.mic_blk_sz * 2, sizeof(double), "DEC_MIC_BUFF");

	//========================================================================
	// DSP
	// We exchange decoded data for processed data.
	// RX - IQ in gives us stereo audio L/R out.
	// TX - mono audio in gives us IQ data out
	// The exchange is a time-slice so the output size is related to the input size by the sample rate (output_rate/input_rate)
	ptr->dsp_lr_sz = (int)(((float)(ppl->args->general.iq_blk_sz * 2)) * ((float)(ppl->args->general.out_rate/(float)ppl->args->general.in_rate)));
	// Create for max RX + TX as we index this by DSP ch_id NOT rx_id which could in theory be anywhere in that range
	for (i=0 ; i < MAX_RX + MAX_TX ; i++ ) {
		ptr->dsp_lr_data[i] = (double *)safealloc(ptr->dsp_lr_sz, sizeof(double), "DSP_LR_BUFF");
	}
	ptr->dsp_iq_sz = ppl->args->general.mic_blk_sz * 2;
	ptr->dsp_iq_data = (double *)safealloc(ptr->dsp_iq_sz, sizeof(double), "DSP_IQ_BUFF");
	// The size must be the same on all outputs so we can combine into an output buffer.
	// ptr->dsp_lr_sz === ptr->dsp_iq_sz

	//========================================================================
	// Encoding
	// Note there is one ring buffer for output which contains Left/Right and IQ outputs
	// All data is 16 bit at 48KHz.
	// A total of 8 bytes per sample, 2x16 bit L/R audio and 2 x 16 bit I/Q TX data
	// Regardless of the number of RX's only one RX data (or two if we split L/R) can be output to the HPSDR. However
	// we can output more RX's to local audio hardware.
	ptr->out_sz = (int)(((float)(ppl->args->general.iq_blk_sz * 4)* ((float)(ppl->args->general.out_rate/(float)ppl->args->general.in_rate)) + (float)(ppl->args->general.mic_blk_sz * 4)));
	ptr->out_buff = (unsigned char *)safealloc(ptr->out_sz, sizeof(char), "OUT_BUFF");
}

static void uninit_transform(Pipeline *ppl) {
	/* Uninitialise the Transform structure
	 *
	 * Arguments:
	 * 	ppl		--	the Pipeline data structure
	 *
	 */

	int i;
	safefree((char *)ptr->rd_iq_buff);
	safefree((char *)ptr->rd_mic_buff);
	for (i=0 ; i < ppl->args->num_rx ; i++ ) {
		safefree((char *)ptr->dec_iq_data[i]);
	}
	safefree((char *)ptr->dec_mic_data);
	for (i=0 ; i < MAX_RX + MAX_TX ; i++ ) {
		safefree((char *)ptr->dsp_lr_data[i]);
	}
	safefree((char *)ptr->dsp_iq_data);
	safefree((char *)ptr->out_buff);
	safefree((char*)ptr);
}

static void do_decode(Pipeline *ppl, Transforms *ptr) {
	/* Decode data
	 *
	 * Arguments:
	 * 	ppl		--	pointer to the Pipeline data structure
	 * 	ptr		--	pointer to the Transform data structure
	 *
	 */

	/*
	 *
	 * Each IQ block is formatted as follows:
	 *	For 1 receiver:
	 *	0                        ... in_iq_sz
	 *	<I2><I1><I0><Q2><Q1><Q0>
	 *	For 2 receivers:
	 *	0                        					... in_iq_sz
	 *    <I12><I11><I10><Q12><Q11><Q10><I22><I21><I20><Q22><Q21><Q20>
	 *	etc to 8 receivers
	 *	The output is interleaved IQ per receiver.
	 *
	 * Each Mic block is formatted as follows:
	 *	0                        ... in_mic_sz
	 *	<M1><M0><M1><M0>
	 *
	 *
	 */

	int num_rx = ppl->args->num_rx;
	int raw_iq_sz = ptr->in_iq_sz;
	int raw_mic_sz = ptr->in_mic_sz;
	int i,j;
	int raw, rx;
	int as_int;
	short as_short;
	int mic_index;
	int src_index[8];
	double input_iq_scale = (double)((double)1.0 / (double)pow(2, 23));
	double input_mic_scale = (double)((double)1.0 / ((double)pow(2, 15) - (double)1.0));

	// Decode IQ
	// Clear the source index array
	for(i=0;i<8;i++) {
		src_index[i] = 0;
	}

	// Iterate over each set of sample data
	// There are 3xI and 3xQ bytes for each receiver interleaved
	for (raw=0 ; raw <= raw_iq_sz - (num_rx*6) ; raw += num_rx*6) {
		// Iterate for each receiver
		for (rx=0 ; rx < num_rx ; rx++) {
			// This byte and 2 bytes following represent the 24 bit big endian I or Q data for this receiver
			// big endian stores the most significant byte in the lowest address
			// Convert and stash the I
			as_int = ((ptr->rd_iq_buff[raw + 2 + (rx*6)] << 8) | (ptr->rd_iq_buff[raw + 1 + (rx*6)] << 16) | (ptr->rd_iq_buff[raw + (rx*6)] << 24)) >> 8;
			ptr->dec_iq_data[rx][src_index[rx]] = (double)(input_iq_scale * (double)as_int);
			src_index[rx] += 1;
			// Convert and stash the Q
			as_int = ((ptr->rd_iq_buff[raw + 5 + (rx*6)] << 8) | (ptr->rd_iq_buff[raw + 4 + (rx*6)] << 16) | (ptr->rd_iq_buff[raw + 3 + (rx*6)] << 24)) >> 8;
			ptr->dec_iq_data[rx][src_index[rx]] = (double)(input_iq_scale * (double)as_int);
			src_index[rx] += 1;
		}
	}

	// Decode Mic
	// Iterate through each byte in the raw data
	mic_index = 0;
	// Correct iteration
	for (i=0 ; i <= raw_mic_sz - 2 ; i+=2) {
		// Iterate for Mic data
		// Add to Real and zero Imag
		for (j=0 ; j < 2 ; j++) {
			// This byte and 1 byte following represent the 16 bit big endian Mic data for this transmitter
			// little endian stores the least significant byte in the lowest address
			if (j == 0) {
				// Add to real
				as_short = (ptr->rd_mic_buff[i+1]) | (ptr->rd_mic_buff[i] << 8);
				ptr->dec_mic_data[mic_index] = (double)(input_mic_scale * (double)as_short);
			}
			else {
				// Zero imag
				ptr->dec_mic_data[mic_index] = 0.0;
			}
			// Bump the mic index
			mic_index ++;
		}
	}
}

static void do_display(Pipeline *ppl, Transforms *ptr) {

	/* Push display data
	 *
	 * Arguments:
	 * 	ppl		--	pointer to the Pipeline data structure
	 * 	ptr		--	pointer to the Transform data structure
	 *
	 */

	int i, j, disp_id;
	int num_rx = ppl->args->num_rx;

	// For each receiver push the raw IQ data to the spectrum function
	for (i=0 ; i < num_rx ; i++) {
		disp_id = ppl->args->disp[i].ch_id;
		for (j=0 ; j < ppl->args->general.iq_blk_sz*2 ; j++) {
			f_display[j] = (float)ptr->dec_iq_data[i][j];
		}
		Spectrum2(disp_id, 0, 0, f_display);
	}
}

static void do_dsp(Pipeline *ppl, Transforms *ptr) {

	/* Run through DSP
	 *
	 * Arguments:
	 * 	ppl		--	the Pipeline data structure
	 * 	ptr		--	the Transform data structure
	 *
	 */

	int num_rx = ppl->args->num_rx;
	int i, j, ch_id;
	int error = 0;

	// Do RX DSP
	// Iterate for each receiver
	for (i=0 ; i < num_rx ; i++) {
		// Do the data exchange
		// Note that decoded data is indexed by RX id and DSP data is indexed by DSP channel id
		ch_id = ppl->args->rx[i].ch_id;
		memset((char *)ptr->dsp_lr_data[ch_id], 0, ptr->dsp_lr_sz * sizeof(double));
		fexchange0(ch_id, ptr->dec_iq_data[i], ptr->dsp_lr_data[ch_id], &error);
		if (error != 0) {
			sprintf(message, "DSP error %d\n", error);
			send_message("c.pipeline", message);
		}
		// Limit and apply gain factor
		for (j=0 ; j < ptr->dsp_lr_sz ; j++) {
			ptr->dsp_lr_data[ch_id][j] = ptr->dsp_lr_data[ch_id][j] * ppl->gain[i];
			if (ptr->dsp_lr_data[ch_id][j] > 1.0) ptr->dsp_lr_data[ch_id][j] = 1.0;
			if (ptr->dsp_lr_data[ch_id][j] < -1.0) ptr->dsp_lr_data[ch_id][j] = -1.0;
		}
	}

	// Do TX DSP
	if (ppl->args->num_tx > 0) {
		fexchange0(ppl->args->tx[0].ch_id, ptr->dec_mic_data, ptr->dsp_iq_data, &error);
		// Limit and apply rf drive factor
		for (j=0 ; j < ptr->dsp_iq_sz ; j++) {
			ptr->dsp_iq_data[j] = ptr->dsp_iq_data[j] * ppl->drive;
			if (ptr->dsp_iq_data[j] > 1.0) ptr->dsp_iq_data[j] = 1.0;
			if (ptr->dsp_iq_data[j] < -1.0) ptr->dsp_iq_data[j] = -1.0;
		}
		// Limit and apply mic gain factor
		// This is a complex double with the imaginary part zero
		for (j=0 ; j < ppl->args->general.mic_blk_sz * 2 ; j+=2) {
			ptr->dec_mic_data[j] = ptr->dec_mic_data[j] * ppl->mic_gain;
			if (ptr->dec_mic_data[j] > 1.0) ptr->dec_mic_data[j] = 1.0;
			if (ptr->dec_mic_data[j] < -1.0) ptr->dec_mic_data[j] = -1.0;
		}
	} else {
		// Zero the IQ data
		memset((char *)ptr->dsp_iq_data, 0, ptr->dsp_iq_sz * sizeof(double));
	}
}

static void do_local_audio(Pipeline *ppl, Transforms *ptr) {
	/* Write to any local audio outputs
	 *
	 * Arguments:
	 * 	ppl		--	the Pipeline data structure
	 * 	ptr		--	the Transform data structure
	 *
	 * The local audio output struct is arranged to have num_output entries.
	 * Each entry has a stream and the left and right dsp channels to be
	 * output to the stream.
	 */

	unsigned int i, j, k, ret;
	short LorI, RorQ;
	double output_scale = (double)pow(2, 15);
	// We have local output defined
	for (i=0 ; i < ppl->local_audio.num_outputs ; i++) {
		// We need a <short> buffer for the local audio to be compatible with VAC
		// However, our ring buffer only supports char or float so we pack into a char buffer
		// The output is interleaved so take the correct left or right output and copy to the temp buffer.
		// This was stopping 1 short of completion.
		for (j=0,k=0 ; j <= ptr->dsp_lr_sz - 2 ; j+=2,k+=4) {
			// First convert and scale to short
			if 	((strcmp(ppl->local_audio.local_output[i].srctype, CWSKIMMER) == 0) ||
				(strcmp(ppl->local_audio.local_output[i].srctype, WSPR) == 0)){
				// CWSkimmer and WSPR requires/can take - IQ data
				LorI = (short)(ptr->dec_iq_data[ppl->local_audio.local_output[i].dsp_ch_left][j] * output_scale);
				RorQ = (short)(ptr->dec_iq_data[ppl->local_audio.local_output[i].dsp_ch_right][j+1] * output_scale);
			} else {
				// All the rest require demodulated data
				LorI = (short)(ptr->dsp_lr_data[ppl->local_audio.local_output[i].dsp_ch_left][j] * output_scale);
				RorQ = (short)(ptr->dsp_lr_data[ppl->local_audio.local_output[i].dsp_ch_right][j+1] * output_scale);
				//LorI = (short)(ptr->dsp_lr_data[0][j] * output_scale);
				//RorQ = (short)(ptr->dsp_lr_data[0][j+1] * output_scale);
			}
			// then pack into the char buffer, little endian order
			f_local_audio[k] = (unsigned char)(LorI & 0xff);
			f_local_audio[k+1] = (unsigned char)((LorI >> 8) & 0xff);
			f_local_audio[k+2] = (unsigned char)(RorQ & 0xff);
			f_local_audio[k+3] = (unsigned char)((RorQ >> 8) & 0xff);
		}
		// Take the output from DSP dsp_ch_left and dsp_ch_right and write it to the ring buffer
		//printf("Ring buffer: %d, %d\n", ringb_write_space(ppl->local_audio.local_output[i].rb_la_out), ptr->dsp_lr_sz * 2);
		if (ringb_write_space (ppl->local_audio.local_output[i].rb_la_out) >= ptr->dsp_lr_sz*2) {
			ringb_write (ppl->local_audio.local_output[i].rb_la_out, f_local_audio, ptr->dsp_lr_sz*2);
		} else {
			send_message("c.pipeline", "No space in audio ring buffer");
		}
		// See if we need to start the stream
		if (!ppl->local_audio.local_output[i].open) {
			if (ppl->local_audio.local_output[i].prime-- <= 0) {
				if ((ret=audio_start_stream(ppl->local_audio.local_output[i].stream) != paNoError)) {
					sprintf(message, "Failed to start stream [%s]\n", audio_get_last_error(ret));
					send_message("c.pipeline", message);
				}
				ppl->local_audio.local_output[i].open = TRUE;
			}
		}
	}
}

static void do_encode(Pipeline *ppl, Transforms *ptr) {
	/* Encode data
	 *
	 * Arguments:
	 * 	ppl		--	the Pipeline data structure
	 * 	ptr		--	the Transform data structure
	 *
	 */

	/*
	 * The output data is structured as follows:
	 * <L1><L0><R1><R0><I1><I0><Q1><Q0><L1><L0><R1><R0><I1><I0><Q1><Q0>...
	 *
	 * The L and R samples are sourced according to the audio output spec.
	 */

	int audio_sz = ptr->dsp_lr_sz;
	int iq_sz = ptr->dsp_iq_sz;
	Route *routes;
	int i, src, dest, left, right;
	short L, R, I, Q;
	double output_scale = (double)pow(2, 15);

	// Sanity check as we must have both with the same number of samples in order to combine them in the output
	if(audio_sz == iq_sz) {
		// Get the audio routing
		left = right = 0;	// Default to DSP channel 0
		// Regardless of whether HPSDR or LOCAL is set we obey the HPSDR routing here
		// as its a separate output so no reason not to.
		routes = ppl->args->audio.routing.hpsdr;
		// There is only one hardware output so we can have max 2 RX outputs (left and right)
		for (i=0 ; i<2 ; i++) {
			if (routes[i].rx != -1) {
				if ((strcmp(routes[i].ch, LEFT) == 0) || (strcmp(routes[i].ch, BOTH) == 0)) {
					left = ppl->args->rx[routes[i].rx-1].ch_id;
				}
				if ((strcmp(routes[i].ch, RIGHT) == 0) || (strcmp(routes[i].ch, BOTH) == 0)) {
					right = ppl->args->rx[routes[i].rx-1].ch_id;
				}
			}
		}

		// Copy and encode the samples
		// dsp_lr[n] contains interleaved L/R double samples
		// dsp_iq contains interleaved I/Q double samples
		// wr_buff is the output buffer to receive byte data in 16 bit big endian format
		// Both audio and IQ data are 16 bit values making 8 bytes in all

		for (dest=0, src=0 ; dest <= ptr->out_sz - 8 ; dest+=8, src+=2) {
			L = (short)(ptr->dsp_lr_data[left][src] * output_scale);
			R = (short)(ptr->dsp_lr_data[right][src+1] * output_scale);
			I = (short)(ptr->dsp_iq_data[src] * output_scale);
			Q = (short)(ptr->dsp_iq_data[src+1] * output_scale);
			ptr->out_buff[dest] = (unsigned char)((L >> 8) & 0xff);
			ptr->out_buff[dest+1] = (unsigned char)(L & 0xff);
			ptr->out_buff[dest+2] = (unsigned char)((R >> 8) & 0xff);
			ptr->out_buff[dest+3] = (unsigned char)(R & 0xff);

			ptr->out_buff[dest+4] = (unsigned char)((I >> 8) & 0xff);
			ptr->out_buff[dest+5] = (unsigned char)(I & 0xff);
			ptr->out_buff[dest+6] = (unsigned char)((Q >> 8) & 0xff);
			ptr->out_buff[dest+7] = (unsigned char)(Q & 0xff);
		}
	} else {
		sprintf(message, "Error, audio_sz %d, iq_sz %d\n", audio_sz, iq_sz);
		send_message("c.pipeline", message);
	}
}
