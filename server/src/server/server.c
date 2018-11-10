/*
server.c

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

/*
Server processing called from the Cython layer to implement:
	1. Pipeline management
	2. Ring buffer management
	3. Forwarded API implementation for pipeline parameters
*/

#include "server.h"
#include "../pipeline/pipeline.h"
#include "../json/cJSON.h"

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
float wbs_results[WBS_SIZE*2];
float wbs_smooth[WBS_SIZE*2];
int wbs_smooth_cnt = 0;
int wait_smooth = 100;

float wbs_correction = 0.0f;
fftw_complex *wbs_in, *wbs_out;
float wbs_window [WBS_SIZE];
// The gain from the antenna socket to the input
//float wbs_gain_adjust = 55.0;   // This should be configured
float wbs_gain_adjust = 85.0;   // This should be configured
fftw_plan wbs_plan;

// ======================================================
// Server operations

int DLL_EXPORT c_server_init(char* args) {

	/*
	 * Initialise the C level server
	 *
	 * Arguments:
	 * 	args	--	ptr to a json encoded string
	 *
	 */

	// Create 2 ring buffers for input and output samples
	// Both are byte buffers which accommodate the
	//	24 bit I/Q and 16 bit mic samples
	// 	and the 16 bit output samples.
	size_t iq_ring_sz;
	size_t mic_ring_sz;
	AudioDescriptor *padesc;
	int i, j, num_tx, output_index, next_index, found;
	int array_len;
	cJSON * root;
	cJSON *tuple_tx;
	cJSON *tuple_rx;
	cJSON *tuple_disp;
	cJSON *array_item;
	cJSON *general;
	cJSON *audio;
	cJSON *routes;
	cJSON *hpsdr;
	cJSON *local;

    // Parse the json string
    root = cJSON_Parse(args);

	// Create the Args structure which is the current server state
	pargs = (Args *)safealloc(sizeof(Args), sizeof(char), "ARGS_STRUCT");
	// Copy the data from the json struct to the args struct
	pargs->num_tx = cJSON_GetObjectItemCaseSensitive(root, "num_tx")->valueint;
	// TX has only one value
	tuple_tx = cJSON_GetObjectItemCaseSensitive(root, "tuple_tx");
	pargs->tx->inst_id = cJSON_GetObjectItemCaseSensitive(tuple_tx, "inst_id")->valueint;
	pargs->tx->ch_id = cJSON_GetObjectItemCaseSensitive(tuple_tx, "ch_id")->valueint;
	pargs->num_rx = cJSON_GetObjectItemCaseSensitive(root, "num_rx")->valueint;
	// RX has an array of up to 7 receivers
	tuple_rx = cJSON_GetObjectItemCaseSensitive(root, "tuple_rx");
	array_len = cJSON_GetArraySize(tuple_rx);
	for (i = 0; i < array_len; i++) {
         array_item = cJSON_GetArrayItem(tuple_rx, i);
         pargs->rx[i].inst_id = cJSON_GetObjectItemCaseSensitive(array_item, "inst_id")->valueint;
         pargs->rx[i].ch_id = cJSON_GetObjectItemCaseSensitive(array_item, "ch_id")->valueint;
	}
	tuple_disp = cJSON_GetObjectItemCaseSensitive(root, "tuple_disp");
	array_len = cJSON_GetArraySize(tuple_disp);
	for (i = 0; i < array_len; i++) {
         array_item = cJSON_GetArrayItem(tuple_disp, i);
         pargs->disp[i].inst_id = cJSON_GetObjectItemCaseSensitive(array_item, "inst_id")->valueint;
         pargs->disp[i].ch_id = cJSON_GetObjectItemCaseSensitive(array_item, "ch_id")->valueint;
	}
	// General
    general = cJSON_GetObjectItemCaseSensitive(root, "General");
    pargs->general.in_rate = cJSON_GetObjectItemCaseSensitive(general, "in_rate")->valueint;
    pargs->general.out_rate = cJSON_GetObjectItemCaseSensitive(general, "out_rate")->valueint;
    pargs->general.iq_blk_sz = cJSON_GetObjectItemCaseSensitive(general, "iq_blk_sz")->valueint;
    pargs->general.mic_blk_sz = cJSON_GetObjectItemCaseSensitive(general, "mic_blk_sz")->valueint;
    pargs->general.duplex = cJSON_GetObjectItemCaseSensitive(general, "duplex")->valueint;

	// Audio
    audio = cJSON_GetObjectItemCaseSensitive(root, "Audio");
    strcpy(pargs->audio.in_src, cJSON_GetObjectItemCaseSensitive(audio, "in_src")->valuestring);
    strcpy(pargs->audio.in_hostapi, cJSON_GetObjectItemCaseSensitive(audio, "in_hostapi")->valuestring);
    strcpy(pargs->audio.in_dev, cJSON_GetObjectItemCaseSensitive(audio, "in_dev")->valuestring);
    strcpy(pargs->audio.out_sink, cJSON_GetObjectItemCaseSensitive(audio, "out_sink")->valuestring);

    // Routes
    routes = cJSON_GetObjectItemCaseSensitive(audio, "Routes");
    // Routes/hpsdr
    hpsdr = cJSON_GetObjectItemCaseSensitive(routes, "hpsdr");
    array_len = cJSON_GetArraySize(hpsdr);
    for (i = 0; i < array_len; i++) {
        array_item = cJSON_GetArrayItem(hpsdr, i);
        pargs->audio.routing.hpsdr[i].rx = cJSON_GetObjectItemCaseSensitive(array_item, "rx")->valueint;
        // strcpy(pargs->audio.routing.hpsdr[i].srctype, cJSON_GetObjectItemCaseSensitive(array_item, "src_type")->valuestring);
        strcpy(pargs->audio.routing.hpsdr[i].srctype, "");
        strcpy(pargs->audio.routing.hpsdr[i].hostapi, cJSON_GetObjectItemCaseSensitive(array_item, "hostapi")->valuestring);
        strcpy(pargs->audio.routing.hpsdr[i].dev, cJSON_GetObjectItemCaseSensitive(array_item, "dev")->valuestring);
        strcpy(pargs->audio.routing.hpsdr[i].ch, cJSON_GetObjectItemCaseSensitive(array_item, "ch")->valuestring);
    }

    // Routes/local
    local = cJSON_GetObjectItemCaseSensitive(routes, "local");
    array_len = cJSON_GetArraySize(local);
	for (i = 0; i < array_len; i++) {
        array_item = cJSON_GetArrayItem(local, i);
        pargs->audio.routing.local[i].rx = cJSON_GetObjectItemCaseSensitive(array_item, "rx")->valueint;
        // strcpy(pargs->audio.routing.local[i].srctype, cJSON_GetObjectItemCaseSensitive(array_item, "src_type")->valuestring);
        strcpy(pargs->audio.routing.local[i].srctype, "");
        strcpy(pargs->audio.routing.local[i].hostapi, cJSON_GetObjectItemCaseSensitive(array_item, "hostapi")->valuestring);
        strcpy(pargs->audio.routing.local[i].dev, cJSON_GetObjectItemCaseSensitive(array_item, "dev")->valuestring);
        strcpy(pargs->audio.routing.local[i].ch, cJSON_GetObjectItemCaseSensitive(array_item, "ch")->valuestring);
	}

	// Create the input ring which accommodates enough samples for 8x DSP block size per receiver
	// at 6 bytes per sample. The size must then be rounded up to the next power of 2 as required
	// by the ring buffer.
	iq_ring_sz = pow(2, ceil(log(pargs->num_rx * pargs->general.iq_blk_sz * 6 * 8)/log(2)));
	rb_iq_in = ringb_create (iq_ring_sz);
	// Mic input is similar except 2 bytes per sample
	// Note, even if there are no TX channels we still need to allocate a ring buffer as the input data
	// is still piped through (maybe not necessary!)
	if (pargs->num_tx == 0)
		num_tx = 1;
	else
		num_tx = pargs->num_tx;
	mic_ring_sz = pow(2, ceil(log(num_tx * pargs->general.mic_blk_sz * 2 * 8)/log(2)));
	rb_mic_in = ringb_create (mic_ring_sz);

	// At worst (48K) output rate == input rate, otherwise the output rate is lower
	// Allow the same buffer size so we can never run out.
	rb_out = ringb_create (iq_ring_sz);

	// Initialise the Pipeline structure
	// Note that the Args structure is referenced
	ppl = (Pipeline *)safealloc(sizeof(Pipeline), sizeof(char), "PIPELINE_STRUCT");
	// Dataflow
	ppl->run = FALSE;
	ppl->display_run = FALSE;
	ppl->terminate = FALSE;
	ppl->terminating = FALSE;
	ppl->rb_iq_in = rb_iq_in;
	ppl->rb_mic_in = rb_mic_in;
	ppl->rb_out = rb_out;
	ppl->pipeline_mutex = &pipeline_mutex;
	ppl->pipeline_con = &pipeline_con;
	ppl->args = pargs;
	pipeline_init(ppl);

	// Local audio
	// We can take input from a local mic input on the PC and push output to a local audio output on the PC
	// The routing for audio is in args->Audio. This contains device and channel info. What is required later
	// in the pipeline processing is to know which DSP channel reads or writes data to which audio ring buffer.
	// The audio processor callback gets kicked for each active stream when data input or output is required.
	// Allocate the required number of audio processors.
	if (strcmp(ppl->args->audio.in_src, LOCAL) == 0) {
		// We have local input defined
		padesc = open_audio_channel(DIR_IN, ppl->args->audio.in_hostapi, ppl->args->audio.in_dev);
		if (padesc == (AudioDescriptor*)NULL) {
			send_message("c.server", "Failed to open audio in!");
			return FALSE;
		}
		ppl->local_audio.local_input.rb_la_in = padesc->rb;
		ppl->local_audio.local_input.stream_id = padesc->stream_id;
		ppl->local_audio.local_input.stream = padesc->stream;
		ppl->local_audio.local_input.dsp_ch = ppl->args->tx[0].ch_id;
		ppl->local_audio.local_input.open = FALSE;
		ppl->local_audio.local_input.prime = 4;
	}

	// We always do any local audio that is defined as both can work together
	// We trust the correctness of the structure here as the generator did all the consistency checks
	// We convert the routing to something the pipeline can digest more easily and also open a stream which creates
	// a ring buffer for the interface. The stream is not running at this point.
	output_index = 0;
	// We allow for each receiver to have a local monitor when we output to an app such as FLDIGI.
	for (i=0 ; i < ppl->args->num_rx*2 ; i++) {
		next_index = 0;
		found = FALSE;
		if (ppl->args->audio.routing.local[i].rx != -1) {
			// We have an assignment for this RX and the RX is configured
			// Find an entry if we have one
			next_index = output_index;
			for(j=0 ; j <= output_index ; j++ ) {
				if (strcmp(ppl->local_audio.local_output[j].dev, ppl->args->audio.routing.local[i].dev) == 0) {
					next_index = j;
					found = TRUE;
					break;
				}
			}

			if (!found) {
				// Open the audio output channel which returns an AudioDescriptor
				padesc = open_audio_channel(DIR_OUT, ppl->args->audio.routing.local[i].hostapi, ppl->args->audio.routing.local[i].dev);
				if (padesc == (AudioDescriptor*)NULL) {
					send_message("c.server", "Failed to open audio out!");
					return FALSE;
				}
				// Set up the stream side of things
				strcpy(ppl->local_audio.local_output[next_index].srctype, ppl->args->audio.routing.local[i].srctype);
				strcpy(ppl->local_audio.local_output[next_index].dev, ppl->args->audio.routing.local[i].dev);
				ppl->local_audio.local_output[next_index].rb_la_out = padesc->rb;
				ppl->local_audio.local_output[next_index].stream_id = padesc->stream_id;
				ppl->local_audio.local_output[next_index].stream = padesc->stream;
				ppl->local_audio.local_output[next_index].open = FALSE;
				ppl->local_audio.local_output[next_index].prime = 4;
				output_index++;
			}
			// Assign the dsp channel to the left or right or both audio output(s)
			// Checkme, had to remove the -1 as it was wrong for what I'm sending over
			if (strcmp(ppl->args->audio.routing.local[i].ch, LEFT) == 0) {
				ppl->local_audio.local_output[next_index].dsp_ch_left = ppl->args->rx[ppl->args->audio.routing.local[i].rx].ch_id;
			} else if(strcmp(ppl->args->audio.routing.local[i].ch, RIGHT) == 0) {
				ppl->local_audio.local_output[next_index].dsp_ch_right = ppl->args->rx[ppl->args->audio.routing.local[i].rx].ch_id;
			} else {
				ppl->local_audio.local_output[next_index].dsp_ch_left = ppl->args->rx[ppl->args->audio.routing.local[i].rx].ch_id;
				ppl->local_audio.local_output[next_index].dsp_ch_right = ppl->args->rx[ppl->args->audio.routing.local[i].rx].ch_id;
			}
		}
	}
	ppl->local_audio.num_outputs = output_index;

    // Initialise the default audio routing for device 0 so we can revert after temporary changes
    audioDefault.rx_left = ppl->local_audio.local_output[0].dsp_ch_left;
    audioDefault.rx_right = ppl->local_audio.local_output[0].dsp_ch_right;

	// Initialise gains
	for (i=0 ; i < MAX_RX ; i++) {
		ppl->gain[i] = 0.2;
	}
	ppl->mic_gain = 1.0;
	ppl->drive = 1.0;

	// Initialise for the wide bandscope FFT
    wbs_in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * WBS_SIZE*2);
    wbs_out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * WBS_SIZE*2);
    wbs_plan = fftw_plan_dft_1d(WBS_SIZE*2, wbs_in, wbs_out, FFTW_FORWARD, FFTW_ESTIMATE);
    hanning_window(WBS_SIZE);

	send_message("c.server", "Server initialised");

	return TRUE;
}

int DLL_EXPORT c_server_start() {
	/*
	 * Start the pipeline
	 *
	 * Arguments:
	 *
	 */

	pipeline_start();
	return TRUE;
}

int DLL_EXPORT c_server_run_display(int display_state) {
	/*
	 * Run/stop the pipeline display activity
	 *
	 * Arguments:
	 * 	display_state	--	TRUE if display run
	 *
	 */

	return pipeline_run_display(display_state);
}

int DLL_EXPORT c_server_terminate() {
	/*
	 * Stop the pipeline
	 *
	 * Arguments:
	 *
	 */

	// Stop and terminate the pipeline
	pipeline_stop();
	pipeline_terminate();
	// Free memory
	if (pargs != NULL)
		safefree((char*)pargs);
	if (ppl != NULL)
		safefree((char*)ppl);
	if (iq != NULL)
		safefree((char*)iq);
	if (mic != NULL)
		safefree((char*)mic);
	if (local_mic != NULL)
		safefree((char*)local_mic);
	if (pan != NULL)
		safefree((char*)pan);
	pargs = NULL;
	ppl = NULL;
	iq = NULL;
	mic = NULL;
	local_mic = NULL;
	pan = NULL;

    // WBS
	fftw_destroy_plan(wbs_plan);
    fftw_free(wbs_in); fftw_free(wbs_out);

	// Free ring buffers
	ringb_free(rb_iq_in);
	ringb_free(rb_mic_in);
	ringb_free(rb_out);

	return TRUE;
}

int DLL_EXPORT c_server_exchange(int n_smpls, int n_rx, int rate, int in_sz, char *ptr_in_bytes, int out_sz, char *ptr_out_bytes) {

	/* Perform data exchange
	 *
	 * Arguments:
	 *  n_smpls			--	number of I/Q samples per frame per receiver
	 *  n_rx			--	number of receivers
	 *  rate			-- 	48000/96000/192000/384000
	 * 	in_sz			--	size of input data buffer
	 *  ptr_in_bytes   		--  	ptr to the input data
	 *  out_sz			--	size of output data buffer
	 *  ptr_out_bytes  		--  	pointer to a byte array to receive output data
	 */

	// Data exchange operates on the ring buffers.
	// 	if there is room to add the data the data is written, else the block is skipped
	//	if there is sufficient output data to satisfy the request it is returned and the
	//	function returns TRUE else it returns FALSE
	//
	// The data is pre-processed such that only contiguous data is written to the ring buffers
	// separated into IQ and Mic data.

	// The Mic data is repeated at higher sampling rates
	// 48K = 1, 96K = 2, 192K = 4, 384K = 8
	// At 1 we take all blocks,
	// at 2 we take every 2nd block
	// at 4 we take every 4th block
	// at 8 we take every 8th block
	int mic_blk_sel = rate/48000;
	// This is a count of blocks to skip and must be static
	static int skip_mic_data = 0;
	int nskip = 0;
	unsigned int total_iq_bytes;
	unsigned int total_mic_bytes;
	unsigned int total_iq_bytes_ct;
	unsigned int total_mic_bytes_ct;
	int iq_bytes;
	int iq_ct;
	int mic_bytes;
	int mic_ct;
	int iq_index;
	int mic_index;
	int state;
	int signal;
	short sample_input_level;
	short peak_input_inst;
	int i, local, ret, write_space, read_space, xfer_sz;

	// Reset the peak input level
	sample_input_level = 0;
	peak_input_inst = 0;

	// The total number of IQ bytes to be concatenated
	total_iq_bytes = n_smpls*n_rx*6;	// 6 bytes per sample (2 x 24 bit)
	total_iq_bytes_ct = total_iq_bytes-1;

	// Determine if we are using HPSDR or local mic input
	// Note that for local we let the normal processing run through except when it comes to
	// writing to the ring buffer we write data from the local audio input ring buffer to
	// the mic ring buffer.
	local = FALSE;
	if (ppl != NULL) {
		if (strcmp(ppl->args->audio.in_src, LOCAL) == 0) {
			// Local audio defined
			local = TRUE;
		}
	}

	// The total number of Mic bytes to be moved
	if (mic_blk_sel == 1) {
		// Take every byte in every frame
		total_mic_bytes = n_smpls*2;	// 2 bytes per sample (1 x 16 bit)
	} else {
		// Take one frame and leave the other frame
		// We then skip frames if necessary
		total_mic_bytes = n_smpls;
	}
	total_mic_bytes_ct = total_mic_bytes-1;

	// First time through allocate buffers
	if (!allocated) {
		// Buffer for IQ data
		iq = (unsigned char *)safealloc(total_iq_bytes, sizeof(char), "IQ_BYTES");
		// Buffer for mono input data from HPSDR mic (16 bit so 2*char per sample)
		mic = (unsigned char *)safealloc(ppl->args->general.mic_blk_sz * 2, sizeof(char), "MIC_BYTES");
		// Buffer for local mic data to overwrite HPSDR mic data (16 bit so 2*char per sample)
		local_mic = safealloc(ppl->args->general.mic_blk_sz * 2, sizeof(char), "LOCAL_MIC");
		allocated = TRUE;
	}

	// The number of IQ bytes for each receiver(s) sample
	iq_bytes = n_rx*6; //2x24 bit samples per rx
	// IQ byte counter
	iq_ct = iq_bytes;
	// The number of Mic bytes following receiver sample
	mic_bytes = 2;
	// Mic byte counter
	mic_ct = mic_bytes;

	// Initial state is reading IQ bytes as we always align at the start of IQ data
	state = IQ;

	// Iterate through every input byte
	iq_index = 0;
	mic_index = 0;

	for(i=0 ; i < in_sz ; i++) {
		if (state == IQ) {
			// Processing IQ bytes
			if (total_iq_bytes_ct >= 0) {
				// Still some IQ bytes to process
				// Pack the byte into iq at the next index
				iq[iq_index++] = ptr_in_bytes[i];
				// Dec counter
				total_iq_bytes_ct--;
			}
			if (--iq_ct == 0) {
				// Exhausted bytes for receiver(s) sample
				// Set the Mic count and change state
				mic_ct = mic_bytes;
				state = MIC;
			}
		} else if (state == MIC) {
			if (!skip_mic_data){
				if (total_mic_bytes_ct >= 0) {
					// Still some Mic bytes to process
					// Pack the byte into mic at the next index
					mic[mic_index++] = ptr_in_bytes[i];
					// Dec counter
					total_mic_bytes_ct--;
				}
			}
			if (--mic_ct == 0) {
				// Exhausted bytes for Mic sample
				// Set the IQ count and change state
				iq_ct = iq_bytes;
				state = IQ;
			}
		}
	}

	// We have now extracted contiguous IQ and Mic samples into separate buffers
	// Copy the temp buffers into the appropriate ring buffer
	if (ringb_write_space (rb_iq_in) >= total_iq_bytes) {
		ringb_write (rb_iq_in, (const char *)iq, total_iq_bytes);
		signal = TRUE;
	} else {
		send_message("c.server", "No write space in IQ ring buffer");
	}
	if (local) {
		// Using local mic input
		// See if we need to start the stream
		if (!local_input_running) {
			if ((ret=audio_start_stream(ppl->local_audio.local_input.stream) != paNoError)) {
				sprintf(message, "Failed to start local input stream [%s]\n", audio_get_last_error(ret));
				send_message("c.server", message);
			}
			local_input_running = TRUE;
		} else {
			// Do a transfer from audio input rb to mic rb
			write_space = ringb_write_space (rb_mic_in);
			//read_space = ringb_float_read_space (ppl->local_audio.local_input.rb_la_in);
			read_space = ringb_read_space (ppl->local_audio.local_input.rb_la_in);
			// The audio data is 16 bit little endian in a character buffer and the required format is 16 bit big endian packed as bytes
			if (write_space > total_mic_bytes && read_space > total_mic_bytes) {
				xfer_sz = total_mic_bytes;
				// Read xfer_sz audio ints to local buffer local_mic
				ringb_read (ppl->local_audio.local_input.rb_la_in, local_mic, (unsigned int)xfer_sz);
				// Pack into a big endian byte array
				for(i=0 ; i < xfer_sz ; i+=2) {
					mic[i] = local_mic[i+1];
					mic[i+1] = local_mic[i];
					// Stash the peak input level for VOX
					sample_input_level = ((short)local_mic[i+1] & 0xFF) << 8;
					sample_input_level = sample_input_level | (((short)local_mic[i]) & 0xFF);
					if (sample_input_level > peak_input_inst) {
						peak_input_inst = sample_input_level;
					}
					peak_input_level = peak_input_inst;
				}
				// Write the data to the ring buffer
				ringb_write (rb_mic_in, (const char *)mic, xfer_sz);
			}
		}
	} else {
		// Using the HPSDR mic input
		if (skip_mic_data == 0) {
			if (ringb_write_space (rb_mic_in) >= total_mic_bytes) {
				ringb_write (rb_mic_in, (const char *)mic, total_mic_bytes);
			}
		}
	}

	// Set the Mic data skip count
	switch (mic_blk_sel) {
	case 4:
		nskip = 1;
		break;
	case 8:
		nskip = 3;
		break;
	default:
		nskip = 0;
		break;
	}

	if (skip_mic_data == 0) {
		// This will skip 1 or 3 blocks (block = 2 frames)
		// Giving 1 in 4 or 1 in 8 frames of data
		skip_mic_data = nskip;
	} else {
		skip_mic_data--;
	}

	// If data was copied then signal the pipeline
	if (signal) {
		pthread_cond_signal(&pipeline_con);
		pthread_mutex_unlock(&pipeline_mutex);
	}

	// Check for output data
	if (ringb_read_space (rb_out) >= (unsigned int)out_sz) {
		// Enough to satisfy the required output block
		ringb_read (rb_out, ptr_out_bytes, (unsigned int)out_sz);
		return TRUE;
	}

	return FALSE;
}

// =========================================================================================================

// ======================================================
// Basic channel operations

int DLL_EXPORT c_server_open_channel(int ch_type, int ch_instance, int iq_size, int mic_size, int in_rate, int out_rate, int tdelayup, int tslewup, int tdelaydown, int tslewdown) {

	/* Open a new DSP channel
	**
	** Arguments:
	** 	ch_type 	-- CH_RX | CH_TX
	** 	ch_instance	-- the TX/RX instance id
	** 	iq_size		-- 128, 256, 1024, 2048, 4096
	** 	mic_size	-- as iq_size for same sample rate
	** 	in_rate		-- input sample rate
	** 	out_rate	-- output sample rate
	** 	tdelayup	-- delay before up slew
	** 	tslewup		-- length of up slew
	**  tdelaydown	-- delay before down slew
	** 	tslewdown	-- length of down slew
	**
	** Returns:
	** 	the assigned channel number
	**
	** Note:
	** 	There are additional parameters to open_channel. These are handled as follows:
	** 		o in_size - the number of samples supplied to the channel.
	** 		o input_samplerate - taken from the set_speed() API call, default 48K.
	** 		o dsp_rate - same as input_samplerate.
	** 		o output_samplerate - fixed at 48K for RX TBD TX
	**
	** The channel is not automatically started. Call set_ch_state() to start the channel.
	*/

	int input_sz;
	int dsp_rate;
	int ch;
	// Calculate the input size
	if (ch_type == CH_RX) {
		// For RX we keep the input and dsp size the same.
		input_sz = iq_size;
	} else {
		// For TX we arrange that the same number of samples arrive at the output as for RX
		// This depends on the input and output rates
		input_sz = mic_size;
	}
	// Set the internal rate to the input samplerate
	dsp_rate = in_rate;

	ch = channel_number;
	OpenChannel(channel_number, input_sz, input_sz, in_rate, dsp_rate, out_rate, ch_type, 0, tdelayup, tslewup, tdelaydown, tslewdown);
	channel_number++;
	return ch;
}

void DLL_EXPORT c_server_close_channel(int channel) {
	/*
	** Close an open channel
	**
	** Arguments:
	** 	channel 	-- the channel id as returned by open_channel()
	**
	*/

	CloseChannel(channel);
}

void DLL_EXPORT c_server_set_input_samplerate(int channel, int rate) {
	/*
	** Set the input sample rate
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	rate 	-- rate in Hz (48000 | 96000 | 192000 | 384000)
	**
	*/

	if (input_samplerate != rate) {
		input_samplerate = rate;
		if (channel != -1) {
			SetInputSamplerate(channel, rate);
		}
	}
}

void DLL_EXPORT c_server_set_ch_state(int channel, int state, int mode) {
	/*
	** Change the channel state
	**
	** Arguments:
	** 	channel 	-- the channel id as returned by open_channel()
	** 	state		-- CH_STATE_STOP | CH_STATE_START
	** 	mode		-- CH_TRANSITION_WAIT | CH_TRANSITION_NOWAIT
	**
	*/

	SetChannelState(channel, state, mode);
}

void DLL_EXPORT c_server_set_dsp_sz(int channel, int sz) {
	/*
	** Set the DSP block size in complex samples
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	sz 		-- block size
	**
	*/

	SetDSPBuffsize(channel, sz);
}

void DLL_EXPORT c_server_set_tdelayup(int channel, int delay) {
	/*
	** Change the delay up time in seconds
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	delay 	-- delay in seconds
	**
	*/

	SetChannelTDelayUp(channel, delay);
}

void DLL_EXPORT c_server_set_tslewup(int channel, int slew) {
	/*
	** Change the slew up time in seconds
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	slew 	-- slew up time in seconds
	**
	*/

	SetChannelTSlewUp(channel, slew);
}

void DLL_EXPORT c_server_set_tdelaydown(int channel, int delay) {
	/*
	** Change the delay up time in seconds
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	delay 	-- delay in seconds
	**
	*/

	SetChannelTDelayDown(channel, delay);
}

void DLL_EXPORT c_server_set_tslewdown(int channel, int slew) {
	/*
	** Change the slew down time in seconds
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	slew 	-- slew down time in seconds
	**
	*/

	SetChannelTSlewDown(channel, slew);
}

//======================================================
// FFTW wisdom (tune FFTW)
void DLL_EXPORT c_server_make_wisdom(char *dir) {
	/*
	** Make a wisdom file if it does not exist.
	**
	*/
	WDSPwisdom(dir);
}

//======================================================
// Receiver type channel parameters
void DLL_EXPORT c_server_set_rx_mode(int channel, int mode) {
	/*
	** Set the receiver mode on the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	mode 	-- CH_LSB | CH_USB |CH_DSB |CH_CWL |CH_CWU |CH_FM |CH_AM |CH_DIGU |CH_SPEC |CH_DIGL |CH_SAM | CH_DRM
	**
	*/
	SetRXAMode(channel, mode);
}

void DLL_EXPORT c_server_set_rx_filter_run(int channel, int run) {
	/*
	** Set the receiver bandpass run mode on the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	run  	-- CH_BP_OFF | CH_BP_ON
	**
	*/

	SetRXABandpassRun(channel, run);
}

void DLL_EXPORT c_server_set_rx_filter_freq(int channel, int low, int high) {
	/*
	** Set the receiver bandpass frequencies on the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	low  	-- low cutoff in Hz
	** 	high	-- high cutoff in Hz
	**
	** Note: frequencies can be +ve or _ve but low masy be numerically less that high
	**
	*/

	SetRXABandpassFreqs(channel, low, high);
}

void DLL_EXPORT c_server_set_rx_filter_window(int channel, int window) {
	/*
	** Set the receiver bandpass window on the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	window  -- CH_BP_WIN_4_TERM | CH_BP_WIN_7_TERM
	**
	** Note: Window is Blackman-Harris 4 or 7 term.
	**
	*/

	SetRXABandpassWindow(channel, window);
}

void DLL_EXPORT c_server_set_agc_mode(int channel, int mode) {
	/*
	** Set the receiver bandpass window on the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	mode  -- CH_AGC_OFF | CH_AGC_LONG | CH_AGC_SLOW | CH_AGC_MED | CH_AGC_FAST
	**
	** Note: All defaults apply at this time.
	**
	*/

	SetRXAAGCMode(channel, mode);
}

double DLL_EXPORT c_server_get_rx_meter_data(int channel, int which) {
	/*
	** Get the requested meter data for the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	which  -- M_RX_S_PEAK | M_RX_S_AV | M_RX_IN_PEAK | M_RX_IN_AV | M_RX_AGC_GAIN | M_RX_AGC_PEAK | M_RX_AGC_AV
	**
	** Returns: reading in db
	**
	*/

	return GetRXAMeter(channel, which);
}

void DLL_EXPORT c_server_set_rx_gain(int rx, float gain) {
	/*
	** Set the output gain for a receiver
	**
	** Arguments:
	** 	rx	-- the rx to set the gain for
	** 	gain  -- the gain value 0.0 - 1.0
	**
	*/

	ppl->gain[rx] = gain;
}

//======================================================
// Transmitter type channel parameters
void DLL_EXPORT c_server_set_tx_mode(int channel, int mode) {
	/*
	** Set the transmitter mode on the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	mode 	-- CH_LSB | CH_USB |CH_DSB |CH_CWL |CH_CWU |CH_FM |CH_AM |CH_DIGU |CH_SPEC |CH_DIGL |CH_SAM | CH_DRM
	**
	*/
	SetTXAMode(channel, mode);
}

void DLL_EXPORT c_server_set_tx_filter_run(int channel, int run) {
	/*
	** Set the transmitter bandpass run mode on the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	run  	-- CH_BP_OFF | CH_BP_ON
	**
	*/

	SetTXABandpassRun(channel, run);
}

void DLL_EXPORT c_server_set_tx_filter_freq(int channel, int low, int high) {
	/*
	** Set the transmitter bandpass frequencies on the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	low  	-- low cutoff in Hz
	** 	high	-- high cutoff in Hz
	**
	** Note: frequencies can be +ve or _ve but low masy be numerically less that high
	**
	*/

	SetTXABandpassFreqs(channel, low, high);
}

void DLL_EXPORT c_server_set_tx_filter_window(int channel, int window) {
	/*
	** Set the transmitter bandpass window on the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	window  -- CH_BP_WIN_4_TERM | CH_BP_WIN_7_TERM
	**
	** Note: Window is Blackman-Harris 4 or 7 term.
	**
	*/

	SetTXABandpassWindow(channel, window);
}

double DLL_EXPORT c_server_get_tx_meter_data(int channel, int which) {
	/*
	** Get the requested tx meter data for the given channel
	**
	** Arguments:
	** 	channel	-- the channel id as returned by open_channel()
	** 	which  -- M_TX_MIC_PK | M_TX_MIV_AV | M_TX_EQ_PK | M_TX_EQ_AV | M_TX_LEV_PK | M_TX_LEV_AV | M_TX_LEV_GAIN | M_TX_COMP_PK | M_TX_COMP_AV | M_TX_ALC_PK | M_TX_ALC_AV | M_TX_ALC_GAIN | M_TX_OUT_PK | M_TX_OUT_AV
	**
	** Returns: reading in db
	**
	*/

	return GetTXAMeter(channel, which);
}

void DLL_EXPORT c_server_set_mic_gain(float gain) {
	/*
	** Set the mic gain for the transmitter
	**
	** Arguments:
	** 	gain  -- the gain value 0.0 - 1.0
	**
	*/

	ppl->mic_gain = gain;
}

void DLL_EXPORT c_server_set_rf_drive(float drive) {
	/*
	** Set the rf output for the transmitter
	**
	** Arguments:
	** 	drive  -- the drive value 0.0 - 1.0
	**
	*/

	ppl->drive = drive;
}

short DLL_EXPORT c_server_get_peak_input_level() {
	/*
	** Get the active audio input level
	**
	** Arguments:

	** Returns: peak level as raw 16 bit sample
	**
	*/

	return peak_input_level;
}

// Display functions
int DLL_EXPORT c_server_open_display(int fft_size, int win_type, int sub_spans, int in_sz, int display_width, int average_mode, int over_frames, int sample_rate, int frame_rate) {

	/*
	** Open a display unit.
	**
	** Arguments:
	** 	fft_size		-- fft size to use, power of 2
	** 	win_type		-- window type
	** 	sub_spans		-- number of receivers to stitch
	** 	in_sz			-- number of input samples
	** 	display_width	-- number of points to plot, generally pixel width
	** 	average_mode	-- modes available :
	** 					-1	Peak detect
	** 					0	No averaging
	** 					1	Time weighted linear
	** 					2	Time weighted log
	** 					3	Window averaging linear
	**					4	Window averaging log
	** 					5	Weighted averaging linear, low noise floor mode
	** 					6	Weighted averaging log, low noise floor mode
	** 	over_frames		-- number of frames to average over
	** 	sample_rate		-- in Hz
	** 	frame_rate		-- required frames per second
	*/

	// There are minimal parameters passed in to make te interface as easy as possible.
	// This may not give enough flexibility so could be extended ion the future. for now we
	// make up the rest

	int success;
	int unit = display_number;

	// Create and set the pan array
	if (pan == NULL) {
		pan = (float *)safealloc(1920, sizeof(float), "PAN_STRUCT");
	}
	pan_sz = display_width;
	// Create the display analyzer
	XCreateAnalyzer(
		unit,
		&success,
		fft_size,
		1,
		sub_spans,
		(char *)NULL
	);
	if (success == 0) {
		// Set display parameters
		int flp[1] = { 0 };
		int overlap = (int)max(0.0, ceil(fft_size - (double)sample_rate / (double)frame_rate));
		const double CLIP_FRACTION = 0.17;
		int clp = (int)floor(CLIP_FRACTION * fft_size);
		const int MAX_AV_FRAMES = 60;
		const double KEEP_TIME = 0.1;
		int max_w = fft_size = (int)min(KEEP_TIME * sample_rate, KEEP_TIME * fft_size * frame_rate);

		SetAnalyzer(
			unit,		// the disply id
			1,			// no of LO freq, 1 for non-SA use
			1,			// complex data input
			flp,		// single value for non-SA use
			fft_size,		// actual fft size same as max fft size for now
			in_sz,		// no input samples per call
			win_type,		// window type
			14,			// window shaping function, 14 is recommended
			overlap,		// no of samples to use from previous frame
			clp,		// no of bins to clip off each side of the sub-span
			0,			// no of bins to clip from low end of span (zoom)
			0,			// no of bins to clip from high end of span (zoom)
			display_width,	// no of pixel values to return
			sub_spans,		// no of sub-spans to concatenate to form a complete span
			average_mode,	// select algorithm for averaging
			over_frames,	// number of frames to average over
			(double)0.0,	// not sure how to use this
			0,				// no calibration in use
			(double)0.0,	// min freq for calibration
			(double)0.0,	// max freq for calibration
			max_w		// how much data to keep in the display buffers
		);
	}
	display_number++;
	return unit;
}

void DLL_EXPORT c_server_set_display(int display_id, int fft_size, int win_type, int sub_spans, int in_sz, int display_width, int average_mode, int over_frames, int sample_rate, int frame_rate) {

	/*
	** Change display unit parameters.
	**
	** Arguments:
	**  display_id		-- id of the display to change
	** 	fft_size		-- fft size to use, power of 2
	** 	win_type		-- window type
	** 	sub_spans		-- number of receivers to stitch
	** 	in_sz			-- number of input samples
	** 	display_width	-- number of points to plot, generally pixel width
	** 	average_mode	-- modes available :
	** 					-1	Peak detect
	** 					0	No averaging
	** 					1	Time weighted linear
	** 					2	Time weighted log
	** 					3	Window averaging linear
	**					4	Window averaging log
	** 					5	Weighted averaging linear, low noise floor mode
	** 					6	Weighted averaging log, low noise floor mode
	** 	over_frames		-- number of frames to average over
	** 	sample_rate		-- in Hz
	** 	frame_rate		-- required frames per second
	*/

	// There are minimal parameters passed in to make the interface as easy as possible.
	// This may not give enough flexibility so could be extended ion the future. for now we
	// make up the rest

	// Set display parameters
	int flp[1] = { 0 };
	int overlap = (int)max(0.0, ceil(fft_size - (double)sample_rate / (double)frame_rate));
	const double CLIP_FRACTION = 0.017;
	int clp = (int)floor(CLIP_FRACTION * fft_size);
	const int MAX_AV_FRAMES = 60;
	const double KEEP_TIME = 0.1;
	int max_w = fft_size + (int)min(KEEP_TIME * sample_rate, KEEP_TIME * fft_size * frame_rate);
	pan_sz = display_width;

	SetAnalyzer(
		display_id,		// the disply id
		1,			// no of LO freq, 1 for non-SA use
		1,			// complex data input
		flp,			// single value for non-SA use
		fft_size,		// actual fft size same as max fft size for now
		in_sz,			// no input samples per call
		win_type,		// Window type
		14,			// window shaping function, 14 is recommended
		overlap,		// no of samples to use from previous frame
		clp,			// no of bins to clip off each side of the sub-span
		0,			// no of bins to clip from low end of span (zoom)
		0,			// no of bins to clip from high end of span (zoom)
		display_width,		// no of pixel values to return
		sub_spans,		// no of sub-spans to concatenate to form a complete span
		average_mode,		// select algorithm for averaging
		over_frames,		// number of frames to average over
		(double)0.0,		// not sure how to use this
		0,			// no calibration in use
		(double)0.0,		// min freq for calibration
		(double)0.0,		// max freq for calibration
		max_w			// how much data to keep in the display buffers
	);
}

void DLL_EXPORT c_server_close_display(int display_id) {
	/*
	** Close an open display
	**
	** Arguments:
	** 	display_id 	-- the channel id as returned by open_channel()
	**
	*/

	DestroyAnalyzer(display_id);

}

int DLL_EXPORT c_server_get_display_data(int display_id, void *display_data) {
	/*
	** Get display data if there is any available.
	**
	** Arguments:
	** 	display_id		-- 	id of the target display unit
	** 	display_data	-- 	out float* buffer to receive display data
	**
	** Return:
	** 	flag			-- 0=no data available, 1=data available
	*/

	int i, flag;
	float *data = (float*)display_data;
	flag = 0;
	GetPixels(display_id, pan, &flag);
	for (i=0 ; i<pan_sz ; i++) {
		data[i] = pan[i];
	}
	return flag;
}

// =========================================================================================================

// ======================================================
// Audio enumerations

DLL_EXPORT char* c_server_enum_audio_inputs() {

    /*
    The enumeration structure is parsed into a Json structure
    then converted to a string to pass back to 8th.

    These are the definitions we receive :-
        typedef struct DeviceEnum {
            int direction;
            int index;
            char name[50];
            int channels;
            char host_api[50];
        }DeviceEnum;

        typedef struct DeviceEnumList {
            int entries;
            DeviceEnum devices[50];
        }DeviceEnumList;

    We need all the info in order to make the correct routing decisions.
    */

    int i;
    //char * data;
    cJSON *root;
    cJSON *inputs;
    cJSON *items;
    DeviceEnumList* audio_inputs;

    // Get the enumeration
    audio_inputs = enum_inputs();
    // Create the Json root object
    root = cJSON_CreateObject();
    // Add an array to hold the enumerations
    cJSON_AddItemToObject(root, "inputs", inputs = cJSON_CreateArray());

    // Iterate the list and populate the Json structure
    for (i=0 ; i<audio_inputs->entries ; i++) {
        // Create an object to add items to
        items = cJSON_CreateObject();
        cJSON_AddStringToObject(items, "name", audio_inputs->devices[i].name);
        cJSON_AddStringToObject(items, "api", audio_inputs->devices[i].host_api);
        cJSON_AddNumberToObject(items, "index", audio_inputs->devices[i].index);
        cJSON_AddNumberToObject(items, "direction", audio_inputs->devices[i].direction);
        cJSON_AddNumberToObject(items, "channels", audio_inputs->devices[i].channels);
        cJSON_AddItemToArray(inputs, items);
    }
    return cJSON_Print(root);
}

DLL_EXPORT char* c_server_enum_audio_outputs() {

    /*
    The enumeration structure is parsed into a Json structure
    then converted to a string to pass back to 8th.

    These are the definitions we receive :-
        typedef struct DeviceEnum {
            int direction;
            int index;
            char name[50];
            int channels;
            char host_api[50];
        }DeviceEnum;

        typedef struct DeviceEnumList {
            int entries;
            DeviceEnum devices[50];
        }DeviceEnumList;

    We need all the info in order to make the correct routing decisions.
    */

    int i;
    cJSON *root;
    cJSON *outputs;
    cJSON *items;
    DeviceEnumList* audio_outputs;

    // Get the enumeration
    audio_outputs = enum_outputs();
    // Create the Json root object
    root = cJSON_CreateObject();
    // Add an array to hold the enumerations
    cJSON_AddItemToObject(root, "outputs", outputs = cJSON_CreateArray());

    // Iterate the list and populate the Json structure
    for (i=0 ; i<audio_outputs->entries ; i++) {
        // Create an object to add items to
        items = cJSON_CreateObject();
        cJSON_AddStringToObject(items, "name", audio_outputs->devices[i].name);
        cJSON_AddStringToObject(items, "api", audio_outputs->devices[i].host_api);
        cJSON_AddNumberToObject(items, "index", audio_outputs->devices[i].index);
        cJSON_AddNumberToObject(items, "direction", audio_outputs->devices[i].direction);
        cJSON_AddNumberToObject(items, "channels", audio_outputs->devices[i].channels);
        cJSON_AddItemToArray(outputs, items);
    }
    return cJSON_Print(root);
}

void DLL_EXPORT c_server_change_audio_outputs(int rx, char* audio_ch) {
    // Reassign the audio routing
    // The given receiver will be re-routed to the given device left/right/both channel
    if (strcmp(audio_ch, LEFT) == 0) {
        ppl->local_audio.local_output[0].dsp_ch_left = rx;
    } else if(strcmp(audio_ch, RIGHT) == 0) {
        ppl->local_audio.local_output[0].dsp_ch_right = rx;
    } else {
        ppl->local_audio.local_output[0].dsp_ch_left = rx;
        ppl->local_audio.local_output[0].dsp_ch_right = rx;
    }
}

void DLL_EXPORT c_server_revert_audio_outputs() {
    // Revert the audio routing
    ppl->local_audio.local_output[0].dsp_ch_left = audioDefault.rx_left;
    ppl->local_audio.local_output[0].dsp_ch_right = audioDefault.rx_right;
}

void hanning_window(int size) {

    int i,j;
    float angle = 0.0F;
    float freq = M_PI*2 / (float) size;
    int midn = size / 2;

    for (i = 0, j = size - 1, angle = 0.0F; i <= midn; i++, j--, angle += freq) {
        wbs_window[j] = (wbs_window[i] = 0.5F - 0.5F * (float)cos(angle));
        //printf("%d:%f ", j, wbs_window[j]);
    }
}

float get_pwr_wbs(int index) {
    //printf("%f ", wbs_out[0][index] * wbs_out[0][index] + wbs_out[1][index] * wbs_out[1][index]);
    return (float)((wbs_out[index][0] * wbs_out[index][0]) + (wbs_out[index][1] * wbs_out[index][1]));
}

void DLL_EXPORT c_server_process_wbs_frame(char *ptr_in_bytes) {
    // The input data is 4096 16 bit values presented as big-endian values in the byte array
    // We scale the values as follows -
    //  10 log Po = 10 log (FFT(Pin) * G)
    //  where -
    //  Po - is the level we will display
    //  Pin - is the level at the antenna socket
    //  G - is the gain between the antenna socket and here

    // Declarations
    int i, k;
    float sample;
    float real_average = 0.0;
    int blocksize = WBS_SIZE;
    int topsize = (blocksize * 2) - 1;
    float samples_real[WBS_SIZE];
    float samples_imag[WBS_SIZE];
    int rawsize = WBS_SIZE * 2;

    // Step 1 -
    //  Calculate scale factor
    float scale_in = (float)(1.0 / pow(2, 15));

    // Step 2 -
    //  Convert buffer to an float array of real values, scaling each value
    //  Sum all values and calculate the average value
    for (i = 0, k = 0; i < rawsize; i += 2, ++k) {
        sample = scale_in * (float)((short)((ptr_in_bytes[i + 1] << 8) | (ptr_in_bytes[i])));
        real_average += sample;
        samples_real[k] = sample;
    }

    // Step 3 -
    //  Subtract the average from the real value array to give SamplesReal
    //  Copy SamplesReal to SamplesImag array
    //  Not ideal, digital down conversion should be done to give I & Q
    real_average /= (float)blocksize;
    for (i = 0; i < blocksize; i++) {
        samples_real[i] -= real_average;      // Subtract average
        samples_imag[i] = samples_real[i];    // Make the same - not ideal
    }

    //  Step 4 -
    //  Window both SamplesReal and SamplesImag arrays
    //  and place in the fftw_complex input array
    for (i = 0; i < blocksize; i++) {
        wbs_in[i][0] = (double)(samples_real[i] * wbs_window[i]);
        wbs_in[i][1] = (double)(samples_imag[i] * wbs_window[i]);
    }

    // Step 5 -
    //  Do FFT forward (FFTW.execute(wbs_plan)) where plan is -
    //     FFTW.dft_1d - this is done at startup
    fftw_execute(wbs_plan);

    // Step 6 -
    //  Calculate terms and flip/reorder (as FFT reverses data)
    for (i = 0; i < blocksize; i++) {
        wbs_results[topsize - i] =
            (float)(10.0 * log10(get_pwr_wbs(i + blocksize) + 1e-180) + wbs_correction);

        wbs_results[blocksize - i] =
            (float)(10.0 * log10(get_pwr_wbs(i) + 1e-180) + wbs_correction);
    }

    // Step 7
    // Smoothing
    for (i = 0; i < topsize; i++) {
        if (wbs_smooth_cnt < WBS_SMOOTH) {
            wbs_smooth[i] += wbs_results[i];
        } else {
            wbs_smooth[i] -= wbs_smooth[i]/(float)WBS_SMOOTH;
            wbs_smooth[i] += wbs_results[i];
        }
    }
    wbs_smooth_cnt++;
}

int DLL_EXPORT c_server_get_wbs_data(int width, void *wbs_data) {
    // The WBS display data is calculated by c_server_process_wbs_frame
    // When a frame is available the buffer pointer is set in the variable wbs_data
    // When no data is available the pointer is set to null in the variable wbs_data
    // Return only 'width' data points for plotting.
    int i,j,k;
    int factor;
    float peak, temp;
    //float accumulator;
    float *data = (float*)wbs_data;
    // WBS_SIZE is 4096. At HD 1920 it would take over 2 screens to display this ..
    // so the user requests the width required and we reduce to that number of points.

    factor = (WBS_SIZE)/width;
    k=0;
    // We have twice the data points as we started with real data
    // Therefore we need to start half way down the dataset
    for (i = WBS_SIZE; i < WBS_SIZE*2; i+=factor) {
        //accumulator = 0.0;
        peak = 0.0;
        temp = 0.0;
        for(j=0; j<factor; j++) {

            // Take the peak value in each group
            // With smoothing
            temp = (wbs_smooth[i+j]/WBS_SMOOTH) - wbs_gain_adjust;
            // Without smoothing
            //temp = wbs_results[i+j] - wbs_gain_adjust;
            // Running peak
            if (temp < peak) {
                peak = temp;
            }

            // Average the group
            // With smoothing
            //accumulator += (wbs_smooth[i+j]/WBS_SMOOTH) - wbs_gain_adjust;
            // Without smoothing
            //accumulator += wbs_results[i+j] - wbs_gain_adjust;
        }
        // Peak
        data[k++] = peak;
        //Average
        //data[k++] = accumulator/factor;
    }
    return 1;
}
