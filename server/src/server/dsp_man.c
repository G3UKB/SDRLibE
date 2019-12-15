/*
dsp_man.c

DSP Management functions

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

// ======================================================
// DSP operations

// ======================================================
// Basic channel operations

void c_server_open_channel(int ch_type, int channel, int iq_size, int mic_size, int in_rate, int out_rate, int tdelayup, int tslewup, int tdelaydown, int tslewdown) {

	/* Open a new DSP channel
	**
	** Arguments:
	** 	ch_type 	-- CH_RX | CH_TX
	**	channel		-- Channel to use
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

	// Calculate the input size
	if (ch_type == CH_RX) {
		// For RX we keep the input and dsp size the same.
		input_sz = iq_size;
	}
	else {
		// For TX we arrange that the same number of samples arrive at the output as for RX
		// This depends on the input and output rates
		input_sz = mic_size;
	}
	// Set the internal rate to the input samplerate
	dsp_rate = in_rate;
	OpenChannel(channel, input_sz, input_sz, in_rate, dsp_rate, out_rate, ch_type, 0, tdelayup, tslewup, tdelaydown, tslewdown);
}

void c_server_close_channel(int channel) {
	/*
	** Close an open channel
	**
	** Arguments:
	** 	channel 	-- the channel id as returned by open_channel()
	**
	*/

	CloseChannel(channel);
}

// Display functions
void c_server_open_display(int display, int fft_size, int win_type, int sub_spans, int in_sz, int display_width, int average_mode, int over_frames, int sample_rate, int frame_rate) {
	/*
	** Open a display unit.
	**
	** Arguments:
	**	display			-- display id to use
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

	int success = -1;
	
	// Create and set the pan array
	if (pan == NULL) {
		pan = (float *)safealloc(1920, sizeof(float), "PAN_STRUCT");
	}
	
	if (display == 0) pan_sz_r1 = display_width;
	else if (display == 1) pan_sz_r2 = display_width;
	else pan_sz_r3 = display_width;

	// Create the display analyzer
	XCreateAnalyzer(
		display,
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
		int max_w = fft_size + (int)min(KEEP_TIME * sample_rate, KEEP_TIME * fft_size * frame_rate);
		SetAnalyzer(
			display,	// the disply id
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
}

void c_impl_server_set_display(int display_id, int fft_size, int win_type, int sub_spans, int in_sz, int display_width, int average_mode, int over_frames, int sample_rate, int frame_rate) {

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
	
	if (display_id == 0) pan_sz_r1 = display_width;
	else if (display_id == 1) pan_sz_r2 = display_width;
	else pan_sz_r3 = display_width;

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

void c_server_close_display(int display_id) {
	/*
	** Close an open display
	**
	** Arguments:
	** 	display_id 	-- the channel id as returned by open_channel()
	**
	*/

	DestroyAnalyzer(display_id);

}
