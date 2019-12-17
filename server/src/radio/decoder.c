/*
decoder.c

Decode a radio frame

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

void frame_decode(int n_smpls, int n_rx, int rate, int in_sz, char *ptr_in_bytes) {

	/* Perform data exchange
	*
	* Arguments:
	*  n_smpls			--	number of I/Q samples per frame per receiver
	*  n_rx				--	number of receivers
	*  rate				-- 	48000/96000/192000/384000
	* in_sz				--	size of input data buffer
	*  ptr_in_bytes   	--  ptr to the input data
	*/

	// Data exchange operates on the ring buffers.
	// 	if there is room to add the data the data is written, else the block is skipped
	//
	// The data is pre-processed such that only contiguous data is written to the ring buffers
	// separated into IQ and Mic data.

	// The Mic data is repeated at higher sampling rates
	// 48K = 1, 96K = 2, 192K = 4, 384K = 8
	// At 1 we take all blocks,
	// at 2 we take every 2nd block
	// at 4 we take every 4th block
	// at 8 we take every 8th block
	int mic_blk_sel = rate / 48000;
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
	total_iq_bytes = n_smpls * n_rx * 6;	// 6 bytes per sample (2 x 24 bit)
	total_iq_bytes_ct = total_iq_bytes - 1;

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
		total_mic_bytes = n_smpls * 2;	// 2 bytes per sample (1 x 16 bit)
	}
	else {
		// Take one frame and leave the other frame
		// We then skip frames if necessary
		total_mic_bytes = n_smpls;
	}
	total_mic_bytes_ct = total_mic_bytes - 1;

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
	iq_bytes = n_rx * 6; //2x24 bit samples per rx
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
	for (i = 0; i < in_sz; i++) {
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
		}
		else if (state == MIC) {
			if (!skip_mic_data) {
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
	if (ringb_write_space(rb_iq_in) >= total_iq_bytes) {
		ringb_write(rb_iq_in, (const char *)iq, total_iq_bytes);
		signal = TRUE;
	}
	else {
		send_message("c.server", "No write space in IQ ring buffer");
	}
	if (local) {
		// Using local mic input
		// See if we need to start the stream
		if (!local_input_running) {
			if ((ret = audio_start_stream(ppl->local_audio.local_input.stream) != paNoError)) {
				sprintf(message, "Failed to start local input stream [%s]\n", audio_get_last_error(ret));
				send_message("c.server", message);
			}
			local_input_running = TRUE;
		}
		else {
			// Do a transfer from audio input rb to mic rb
			write_space = ringb_write_space(rb_mic_in);
			//read_space = ringb_float_read_space (ppl->local_audio.local_input.rb_la_in);
			read_space = ringb_read_space(ppl->local_audio.local_input.rb_la_in);
			// The audio data is 16 bit little endian in a character buffer and the required format is 16 bit big endian packed as bytes
			if (write_space > total_mic_bytes && read_space > total_mic_bytes) {
				xfer_sz = total_mic_bytes;
				// Read xfer_sz audio ints to local buffer local_mic
				ringb_read(ppl->local_audio.local_input.rb_la_in, local_mic, (unsigned int)xfer_sz);
				// Pack into a big endian byte array
				for (i = 0; i < xfer_sz; i += 2) {
					mic[i] = local_mic[i + 1];
					mic[i + 1] = local_mic[i];
					// Stash the peak input level for VOX
					sample_input_level = ((short)local_mic[i + 1] & 0xFF) << 8;
					sample_input_level = sample_input_level | (((short)local_mic[i]) & 0xFF);
					if (sample_input_level > peak_input_inst) {
						peak_input_inst = sample_input_level;
					}
					peak_input_level = peak_input_inst;
				}
				// Write the data to the ring buffer
				ringb_write(rb_mic_in, (const char *)mic, xfer_sz);
			}
		}
	}
	else {
		// Using the HPSDR mic input
		if (skip_mic_data == 0) {
			if (ringb_write_space(rb_mic_in) >= total_mic_bytes) {
				ringb_write(rb_mic_in, (const char *)mic, total_mic_bytes);
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
	}
	else {
		skip_mic_data--;
	}

	// If data was copied then signal the pipeline
	if (signal) {
		pthread_cond_signal(&pipeline_con);
		pthread_mutex_unlock(&pipeline_mutex);
	}
}