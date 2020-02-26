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

// Includes
#include "../common/include.h"

// Local functions
static void c_audio_init();
static void c_ppl_audio_init();
static void create_ring_buffers();
static void init_pipeline_structure();
static int local_audio_setup();
static void init_gains();
static void init_wbs();
static void create_dsp_channels();
static void create_display_channels();
static void set_cc_data();
static void hanning_window(int size);
static float get_pwr_wbs(int index);

// Module vars
int sd = 0;							// One and only socket
struct sockaddr_in srv_addr;		// Server address structure
int c_server_initialised = FALSE;	// Set when init completes successfully
int c_server_running = FALSE;		// After c_server_start completes
int c_radio_discovered = FALSE;		// Radio discovered flag
int c_radio_running = FALSE;		// Radio running flag
int c_server_disp[3] = { FALSE,FALSE,FALSE };

// Locking
pthread_mutex_t udp_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t udp_con = PTHREAD_COND_INITIALIZER;

//============================================================================================
// Server initialisation operations

// Perform initialisation sufficient to run the radio
// The rest of initisalisation is performed on server_start as parameters can be changed
// Must be first call
int c_server_init() {

	/*
	* Initialise the server
	*
	* Arguments:
	*
	*/

	int i;
	char last_error[128];           // Holder for last error

	//============================================================================================
	// Initialise Winsock 
	WSADATA wsa;                    // Winsock
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Server: Failed to initialise winsock!\n");
		exit(-1);
	}

	//============================================================================================
	// Create a broadcast socket
	if ((sd = open_bc_socket()) == -1) {
		printf("Server: Failed to create broadcast socket!\n");
		return FALSE;
	}
	
	//============================================================================================
	// Create the args structure and set reasonable defaults
	pargs = (Args *)safealloc(sizeof(Args), sizeof(char), "ARGS_STRUCT");
	// Set TX defaults
	pargs->num_tx = 1;
	// TX has only one value
	// Set next above RX channels
	pargs->tx->ch_id = 7;
	// Set RX defaults
	// Assume one RX for now
	pargs->num_rx = 1;
	// We set up all MAX_RX receivers although we are limited to 3 with current hardware
	for (i = 0; i < MAX_RX; i++) {
		// Set channel id's 0-6
		pargs->rx[i].ch_id = i;
	}
	
	// Set up display channels for MAX_RX receivers
	for (i = 0; i < MAX_RX; i++) {
		// Display id's are arbitary so just use 0-6
		pargs->disp[i].ch_id = i;
	}
	
	// Set up general configuration to normal values
	pargs->general.in_rate = IN_RATE;
	pargs->general.out_rate = OUT_RATE;
	pargs->general.iq_blk_sz = IQ_BLK_SZ;
	pargs->general.mic_blk_sz = MIC_BLK_SZ;
	pargs->general.fft_size = FFT_SZ;
	pargs->general.window_type = RECTANGULAR;
	pargs->general.display_width = DISPLAY_WIDTH;
	pargs->general.av_mode = PAN_TIME_AV_LIN;
	pargs->general.duplex = 0;
	
	// Initialise audio structures
	c_audio_init();
	
	// Init the CC bytes with defaults
	cc_out_init();
	
	// Done
	c_server_initialised = TRUE;
	printf("c.server: Server initialised\n");
	return TRUE;
}

// Optional updates to general configuration
// If required these must be updated before the server is started
// Updates are not accepted once the server is running
void c_server_set_num_rx(int num_rx) {
	if (!c_server_running) pargs->num_rx = num_rx;
}
void c_server_set_in_rate(int rate) {
	if( !c_server_running ) pargs->general.in_rate = rate;
}

void c_server_set_out_rate(int rate) {
	if (!c_server_running) pargs->general.out_rate = rate;
}

void c_server_set_iq_blk_sz(int blk_sz) {
	if (!c_server_running) pargs->general.iq_blk_sz = blk_sz;
}

void c_server_set_mic_blk_sz(int blk_sz) {
	if (!c_server_running) pargs->general.mic_blk_sz = blk_sz;
}

void c_server_set_duplex(int duplex) {
	if (!c_server_running) pargs->general.duplex = duplex;
}

void c_server_set_fft_size(int size) {
	if (!c_server_running) pargs->general.fft_size = size;
}

void c_server_set_window_type(int window_type) {
	if (!c_server_running) pargs->general.window_type = window_type;
}

void c_server_set_av_mode(int mode) {
	if (!c_server_running) pargs->general.av_mode = mode;
}

void c_server_set_display_width(int width) {
	if (!c_server_running) pargs->general.display_width = width;
	if (pargs->num_rx >= 1)
		c_server_set_display(0, width);
	if (pargs->num_rx >= 2)
		c_server_set_display(1, width);
	if (pargs->num_rx >= 3)
		c_server_set_display(2, width);
}

//============================================================================================
// Server control operations

// Server start completes the initialisation and starts services 
int c_server_start() {
	/*
	 * Start the server services
	 *
	 * Arguments:
	 *
	 */
	
	// Can't continue unless we are initialised
	if (!c_server_initialised) {
		printf("c.server: Please initialise server first!\n");
		return FALSE;
	}
	
	//==============================================================
	// Finish initialisation
	create_ring_buffers();
	init_pipeline_structure();
	if (!local_audio_setup()) {
		printf("c.server: Failed to initialise audio!\n");
		return FALSE;
	}
	init_gains();
	init_wbs();
	create_dsp_channels();
	create_display_channels();
	set_cc_data();
	// Revert to a normal socket with larger buffers
	revert_sd(sd);
	// Init the UDP reader
	reader_init(sd, &srv_addr, pargs->num_rx, pargs->general.in_rate);
	// Init sequence processing
	seq_init();

	//==============================================================
	// Start pipeline processing
	pipeline_start();

	// All done
	c_server_running = TRUE;
	printf("c.server: Server running\n");

	return TRUE;
}

// Tidy close
int c_server_terminate() {
	/*
	* Close all
	*
	* Arguments:
	*
	*/

	// Close all channels
	for (int i = 0; i < pargs->num_rx; i++) {
		c_server_close_display(pargs->disp[i].ch_id);
		c_server_close_channel(pargs->rx[i].ch_id);
	}
	c_server_close_channel(pargs->tx->ch_id);

	// Stop and terminate the pipeline
	reader_stop();
	reader_terminate();
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
	closesocket(sd);

	c_server_running = FALSE;
	c_server_initialised = FALSE;
	allocated = FALSE;
	
	return TRUE;
}

//============================================================================================
// Radio management operations

// Perform discovery protocol
int c_radio_discover() {
	/*
	* Discover hardware
	*
	* Arguments:
	*
	*/
	// Can't continue unless we are initialised
	if (!c_server_initialised) {
		printf("c.server: Must call c_server_initialise first!\n");
		return FALSE;
	}
	if (!do_discover(&srv_addr, sd)) {
		printf("c.server: No radio hardware found!\n");
		return FALSE;
	}
	//printf("\nPort:%d\n", ntohs(srv_addr.sin_port));
	//printf("\nIP:%s\n", inet_ntoa(srv_addr.sin_addr));
	c_radio_discovered = TRUE;
	printf("c.server: Found radio hardware\n");
	return TRUE;
}

// Start the radio services
int c_radio_start(int wbs) {
	/*
	* Start the radio services
	*
	* Arguments:
	*	wbs	-- TRUE to start the wide band scope
	*/
	
	// Can't continue unless we are configured
	if (!c_server_running || !c_radio_discovered) {
		printf("c.server: Server must be running and radio discovered !\n");
		return FALSE;
	}

	// Already running?
	if (c_radio_running) {
		printf("c.server: Radio is already running!\n");
		return FALSE;
	}
	// Start radio hardware
	if (do_start(sd, &srv_addr, wbs)) {
		// Before starting the reader we need to prime the radio
		prime_radio( sd, &srv_addr );
		reader_start();
		c_radio_running = TRUE;
	} else {
		printf("c.server: Failed to start radio hardware!\n");
		return FALSE;
	}
	printf("c.server: Radio running\n");
	return TRUE;
}

int c_radio_stop() {
	/*
	* Stop the radio services
	*
	* Arguments:
	*
	*/

	// Can't continue unless we are configured
	if (!c_server_running || !c_radio_discovered) {
		printf("c.server: Server must be running and radio discovered!\n");
		return FALSE;
	}

	// Are we stopped?
	if (!c_radio_running) {
		printf("c.server: Radio is already stopped!\n");
		return FALSE;
	}

	// Stop services
	reader_stop();
	// Stop radio hardware
	if (!do_stop(sd, &srv_addr)) {
		printf("c.server: Failed to stop radio hardware!\n");
		return FALSE;
	}
	c_radio_running = FALSE;
	printf("c.server: Radio stopped\n");
	return TRUE;
}

//============================================================================================
// Radio control operations

// Switch RX/TX
void c_server_mox(int mox_state) {
	/*
	** Set TX/RX state
	**
	** Arguments:
	** 	mox_state	-- TRUE if MOX on
	**
	*/

	if (mox_state) {
		// Switch to TX
		// We run down active receivers except RX-1 which is the monitor
		for (int i = 1; i < pargs->num_rx; i++) {
			SetChannelState(pargs->rx[1].ch_id, CH_STATE_STOP, CH_TRANSITION_NOWAIT);
		}
		// Run up TX
		SetChannelState(pargs->tx->ch_id, CH_STATE_START, CH_TRANSITION_NOWAIT);
		// Set hardware to TX
		cc_out_mox(TRUE);
	}
	else {
		// Switch to RX
		// Run down TX
		SetChannelState(pargs->tx->ch_id, CH_STATE_STOP, CH_TRANSITION_NOWAIT);
		// We run up active receivers except RX-1 which is the monitor
		for (int i = 1; i < pargs->num_rx; i++) {
			SetChannelState(pargs->rx[1].ch_id, CH_STATE_START, CH_TRANSITION_NOWAIT);
		}
		// Set hardware to RX
		cc_out_mox(FALSE);
	}
}

//============================================================================================
// Radio RX operations

void c_server_set_rx_mode(int channel, int mode) {
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

void c_server_set_rx_filter_run(int channel, int run) {
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

void c_server_set_rx_filter_freq(int channel, int low, int high) {
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

void c_server_set_rx_filter_window(int channel, int window) {
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

void c_server_set_agc_mode(int channel, int mode) {
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

double c_server_get_rx_meter_data(int channel, int which) {
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

void c_server_set_rx_gain(int rx, float gain) {
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

//============================================================================================
// Radio TX operations

void c_server_set_tx_mode(int channel, int mode) {
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

void c_server_set_tx_filter_run(int channel, int run) {
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

void c_server_set_tx_filter_freq(int channel, int low, int high) {
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

void c_server_set_tx_filter_window(int channel, int window) {
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

double c_server_get_tx_meter_data(int channel, int which) {
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

void c_server_set_mic_gain(float gain) {
	/*
	** Set the mic gain for the transmitter
	**
	** Arguments:
	** 	gain  -- the gain value 0.0 - 1.0
	**
	*/

	ppl->mic_gain = gain;
}

void c_server_set_rf_drive(float drive) {
	/*
	** Set the rf output for the transmitter
	**
	** Arguments:
	** 	drive  -- the drive value 0.0 - 1.0
	**
	*/

	ppl->drive = drive;
}

short c_server_get_peak_input_level() {
	/*
	** Get the active audio input level
	**
	** Arguments:

	** Returns: peak level as raw 16 bit sample
	**
	*/

	return peak_input_level;
}

// =========================================================================================================
// Display Processing

// Set display is called when the display width changes during run-time
void c_server_set_display(int ch_id, int display_width) {
	c_impl_server_set_display(	ch_id,
								pargs->general.fft_size,
								pargs->general.window_type,
								1,
								pargs->general.iq_blk_sz,
								display_width,
								pargs->general.av_mode,
								10,
								pargs->general.in_rate,
								10 );
}

// Get display data
int c_server_get_display_data(int display_id, void *display_data) {
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
	//float *data = (float*)display_data;
	double *data = (double*)display_data;
	flag = 0;
	int pan_sz;

	if (display_id == 0) pan_sz = pan_sz_r1;
	else if (display_id == 1) pan_sz = pan_sz_r1;
	else pan_sz = pan_sz_r3;
	
	GetPixels(display_id, pan, &flag);
	if (flag) {
		for (i = 0; i < pan_sz; i++) {
			data[i] = pan[i];
		}
	}
	return flag;
}

// =========================================================================================================
// WBS Processing

void c_server_process_wbs_frame(char *ptr_in_bytes) {
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

int c_server_get_wbs_data(int width, void *wbs_data) {
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


//============================================================================================
//============================================================================================
// These functions can be called and may need to be called before server initialisation

//============================================================================================
// Audio functions

// Call enumerations at SOD to populate UI. They do not need the server runnng.
// These are then used to select devices to set up the routes.
DeviceEnumList* c_server_enum_audio_inputs() {
	
	return enum_inputs();
}

DeviceEnumList* c_server_enum_audio_outputs() {

	return enum_outputs();

}

//==============================================
// This sets up a single audio route.
// It can be called at SOD before the server is started to set all routes.
// OR it can be called as part of c_server_clear_audio_routes()-c_server_set_audio_route()-c_server_restart_audio_routes
// dynamically to reconfigure the audio.
void c_server_set_audio_route(int direction, char* location, int receiver, char* host_api, char* dev, char* channel) {
	/*
	* Set an audio channel
	*
	* Arguments:
	* 	direction	--	0=input, 1=output
	*	location	--	HPSDR or LOCAL
	*	receiver	--	receiver id 0-n
	*	host_api	--	the audio host api string
	*	dev			--	the audio device name
	*	channel		--	LEFT, RIGHT, BOTH
	*/
	printf("%d,%s,%d,%s,%s,%s\n", direction, location, receiver, host_api, dev, channel);
	int i;

	if (direction == 0) {
		// Mic input
		if (strcmp(location, HPSDR) == 0) {
			strcpy(pargs->audio.in_src, HPSDR);
			strcpy(pargs->audio.in_hostapi, "");
			strcpy(pargs->audio.in_dev, "");
			strcpy(pargs->audio.out_sink, "");

		}
		else {
			strcpy(pargs->audio.in_src, LOCAL);
			strcpy(pargs->audio.in_hostapi, host_api);
			strcpy(pargs->audio.in_dev, dev);
			strcpy(pargs->audio.out_sink, channel);
		}
	}
	else {
		// Audio output
		// Remember that this is a DSP channel id for the receiver which is 0 based
		if (strcmp(location, HPSDR) == 0) {
			// HPSDR has two channels available
			for (i = 0; i < 2; i++) {
				if (pargs->audio.routing.hpsdr[i].rx == -1) {
					// Found a free slot
					pargs->audio.routing.hpsdr[i].rx = receiver - 1;
					strcpy(pargs->audio.routing.hpsdr[i].srctype, "");
					strcpy(pargs->audio.routing.hpsdr[i].hostapi, host_api);
					strcpy(pargs->audio.routing.hpsdr[i].dev, dev);
					strcpy(pargs->audio.routing.hpsdr[i].ch, channel);
					break;
				}
			}
		}
		else {
			for (i = 0; i < MAX_RX * 2; i++) {
				if (pargs->audio.routing.local[i].rx == -1) {
					// Found a free slot
					pargs->audio.routing.local[i].rx = receiver - 1;
					strcpy(pargs->audio.routing.local[i].srctype, "");
					strcpy(pargs->audio.routing.local[i].hostapi, host_api);
					strcpy(pargs->audio.routing.local[i].dev, dev);
					strcpy(pargs->audio.routing.local[i].ch, channel);
					break;
				}
			}
		}
	}
	printf("c.server: Audio route configured\n");
}

// Clear all audio routes
int c_server_clear_audio_routes() {

	int r;
	
	if (c_server_running) {
		// Stop local audio processing
		c_server_local_audio_run(FALSE);
	}

	// Then shutdown the audio process
	if (r = audio_uninit() != paNoError) {
		printf("c.server: Error closing audio! [%d]\n", r);
		return FALSE;
	}
	printf("c.server: Audio services closed\n");

	// Then re-initialise the args structure
	c_audio_init();

	if (c_server_running) {
		// (re)init the pipeline audio structure
		c_ppl_audio_init();
	}

	// Now it can be re-populated and the audio restarted
	printf("c.server: Audio routes cleared\n");
	
	return TRUE;
}

// Restart all audio routes
int c_server_restart_audio_routes() {

	int r;

	if (c_server_running) {
		// NOTE: c_server_clear_audio_routes() must have been called first
		// Then the routes re-populated using calls to c_server_set_audio_route()
		// Now we call the initial audio setup to re-populate the pipeline structure
		if (!local_audio_setup()) {
			printf("c.server: Problem setting up local audio!\n");
			return FALSE;
		}
		// Start local audio processing
		c_server_local_audio_run(TRUE);
	}
	printf("c.server: Audio routes restarted\n");
	return TRUE;
}

//==============================================
// This will reassign the given channel LEFT,RIGHT,BOTH to the given receiver.
// The device will be the first device assigned which is normally the default device.
// An enhancement would be to allow the device to be specified.
void c_server_change_audio_outputs(int rx, char* audio_ch) {
	if (strcmp(audio_ch, LEFT) == 0) {
		ppl->local_audio.local_output[0].dsp_ch_left = rx;
	}
	else if (strcmp(audio_ch, RIGHT) == 0) {
		ppl->local_audio.local_output[0].dsp_ch_right = rx;
	}
	else {
		ppl->local_audio.local_output[0].dsp_ch_left = rx;
		ppl->local_audio.local_output[0].dsp_ch_right = rx;
	}
}

// This will revert to the audio outputs to the configured receiver(s)
void c_server_revert_audio_outputs() {
	// Revert the audio routing
	ppl->local_audio.local_output[0].dsp_ch_left = audioDefault.rx_left;
	ppl->local_audio.local_output[0].dsp_ch_right = audioDefault.rx_right;
}

// Stop/start local audio while we make changes
int c_server_local_audio_run(int runstate) {
	// Start/stop local audio
	return pipeline_run_local_audio(runstate);
}

//============================================================================================
// These functions can be called and may need to be called before server initialisation

//======================================================
// FFTW wisdom (tune FFTW)

void c_server_make_wisdom(char *dir) {
	/*
	** Make a wisdom file if it does not exist.
	**
	*/
	WDSPwisdom(dir);
}

// ======================================================
// CC data updates
// ToDo complete implementation
void c_server_cc_out_alex_attn(int attn) {
	cc_out_alex_attn(attn);
}
void c_server_cc_out_preamp(int preamp) {
	cc_out_preamp(preamp);
}
void c_server_cc_out_alex_rx_ant(int ant) {
	cc_out_alex_rx_ant(ant);
}
void c_server_cc_out_alex_rx_out(int out) {
	cc_out_alex_rx_out(out);
}
void c_server_cc_out_alex_tx_rly(int rly) {
	cc_out_alex_tx_rly(rly);
}
void c_server_cc_out_duplex(int duplex) {
	cc_out_duplex(duplex);
}
void c_server_cc_out_alex_auto(int state) {
	cc_out_alex_auto(state);
}
void c_server_cc_out_alex_hpf_bypass(int bypass) {
	cc_out_alex_hpf_bypass(bypass);
}
void c_server_cc_out_lpf_30_20(int setting) {
	cc_out_lpf_30_20(setting);
}
void c_server_cc_out_lpf_60_40(int setting) {
	cc_out_lpf_60_40(setting);
}
void c_server_cc_out_lpf_80(int setting) {
	cc_out_lpf_80(setting);
}
void c_server_cc_out_lpf_160(int setting) {
	cc_out_lpf_160(setting);
}
void c_server_cc_out_lpf_6(int setting) {
	cc_out_lpf_6(setting);
}
void c_server_cc_out_lpf_12_10(int setting) {
	cc_out_lpf_12_10(setting);
}
void c_server_cc_out_lpf_17_15(int setting) {
	cc_out_lpf_17_15(setting);
}
void c_server_cc_out_hpf_13(int setting) {
	cc_out_hpf_13(setting);
}
void c_server_cc_out_hpf_20(int setting) {
	cc_out_hpf_20(setting);
}
void c_server_cc_out_hpf_9_5(int setting) {
	cc_out_hpf_9_5(setting);
}
void c_server_cc_out_hpf_6_5(int setting) {
	cc_out_hpf_6_5(setting);
}
void c_server_cc_out_hpf_1_5(int setting) {
	cc_out_hpf_1_5(setting);
}
void c_server_cc_out_set_rx_1_freq(unsigned int freq_in_hz) {
	cc_out_set_rx_1_freq(freq_in_hz);
}
void c_server_cc_out_set_rx_2_freq(unsigned int freq_in_hz) {
	cc_out_set_rx_2_freq(freq_in_hz);
}
void c_server_cc_out_set_rx_3_freq(unsigned int freq_in_hz) {
	cc_out_set_rx_3_freq(freq_in_hz);
}
void c_server_cc_out_set_tx_freq(unsigned int freq_in_hz) {
	cc_out_set_tx_freq(freq_in_hz);
}

//============================================================================================
//============================================================================================
// Local Functions

// Initialise audio structures
static void c_audio_init() {

	int i;

	// Audio inputs
	// Set up some default audio
	// We can only default to HPSDR audio as we can't select local audio automatically
	// Mic input
	strcpy(pargs->audio.in_src, HPSDR);
	strcpy(pargs->audio.in_hostapi, "");
	strcpy(pargs->audio.in_dev, "");
	strcpy(pargs->audio.out_sink, "");

	// Audio output
	// There are two route arrays, one for HPSDR and one for local
	// They define which receiver output is routed where
	// Default such that RX 1 left and right are HPSDR
	// There are 2 possible HPSDR routes for left and right. 
	// HPSDR
	// Put both on rx 1
	pargs->audio.routing.hpsdr[0].rx = 1;
	// These 3 values are for compatibility, not used for HPSDR outputs
	strcpy(pargs->audio.routing.hpsdr[0].srctype, "");
	strcpy(pargs->audio.routing.hpsdr[0].hostapi, "");
	strcpy(pargs->audio.routing.hpsdr[0].dev, "");
	strcpy(pargs->audio.routing.hpsdr[0].ch, BOTH);
	// Nothing on second channel
	pargs->audio.routing.hpsdr[0].rx = -1;
	strcpy(pargs->audio.routing.hpsdr[0].srctype, "");
	strcpy(pargs->audio.routing.hpsdr[0].hostapi, "");
	strcpy(pargs->audio.routing.hpsdr[0].dev, "");
	strcpy(pargs->audio.routing.hpsdr[0].ch, "");

	// Local
	// Clear for MAX_RX*2 receivers as each can be a separate channel
	for (i = 0; i < MAX_RX * 2; i++) {
		pargs->audio.routing.local[i].rx = -1;
		strcpy(pargs->audio.routing.local[i].srctype, "");
		strcpy(pargs->audio.routing.local[i].hostapi, "");
		strcpy(pargs->audio.routing.local[i].dev, "");
		strcpy(pargs->audio.routing.local[i].ch, "");
	}
}

// Initialise pipeline audio structures
static void c_ppl_audio_init() {
	int i;
	
	for (i = 0; i < ppl->args->num_rx * 2; i++) {
		strcpy(ppl->local_audio.local_output[i].srctype, "");
		strcpy(ppl->local_audio.local_output[i].dev, "");
		ppl->local_audio.local_output[i].dsp_ch_left = 0;
		ppl->local_audio.local_output[i].dsp_ch_right = 0;
		ppl->local_audio.local_output[i].stream_id = NULL;
		ppl->local_audio.local_output[i].open = FALSE;
		ppl->local_audio.local_output[i].prime = FALSE;
		ppl->local_audio.local_output[i].rb_la_out = NULL;
	}
}

// Create ring buffers for the IQ and Mic data streams
static void create_ring_buffers() {

	int num_tx;
	size_t iq_ring_sz;
	size_t mic_ring_sz;

	// Create the input ring which accommodates enough samples for 8x DSP block size per receiver
	// at 6 bytes per sample. The size must then be rounded up to the next power of 2 as required
	// by the ring buffer.
	iq_ring_sz = pow(2, ceil(log(pargs->num_rx * pargs->general.iq_blk_sz * 6 * 8) / log(2)));
	rb_iq_in = ringb_create(iq_ring_sz);
	// Mic input is similar except 2 bytes per sample
	// Note, even if there are no TX channels we still need to allocate a ring buffer as the input data
	// is still piped through (maybe not necessary!)
	if (pargs->num_tx == 0)
		num_tx = 1;
	else
		num_tx = pargs->num_tx;
	mic_ring_sz = pow(2, ceil(log(num_tx * pargs->general.mic_blk_sz * 2 * 8) / log(2)));
	rb_mic_in = ringb_create(mic_ring_sz);

	// At worst (48K) output rate == input rate, otherwise the output rate is lower
	// Allow the same buffer size so we can never run out.
	rb_out = ringb_create(iq_ring_sz);
}

// Initialise the Pipeline structure
static void init_pipeline_structure() {
	
	ppl = (Pipeline *)safealloc(sizeof(Pipeline), sizeof(char), "PIPELINE_STRUCT");
	ppl->run = FALSE;
	ppl->display_run = FALSE;
	ppl->local_audio_run = TRUE;
	ppl->terminate = FALSE;
	ppl->terminating = FALSE;
	ppl->rb_iq_in = rb_iq_in;
	ppl->rb_mic_in = rb_mic_in;
	ppl->rb_out = rb_out;
	ppl->pipeline_mutex = &pipeline_mutex;
	ppl->pipeline_con = &pipeline_con;
	ppl->args = pargs;
	pipeline_init(ppl);
}

// Set up and allocate local audio
static int local_audio_setup() {
	// Local audio
	// We can take input from a local mic input on the PC and push output to a local audio output on the PC
	// The routing for audio is in args->Audio. This contains device and channel info. What is required later
	// in the pipeline processing is to know which DSP channel reads or writes data to which audio ring buffer.
	// The audio processor callback gets kicked for each active stream when data input or output is required.
	// Allocate the required number of audio processors.
	// Init local audio

	AudioDescriptor *padesc;
	int i, j, output_index, next_index, found;
	audio_init();
	if (strcmp(ppl->args->audio.in_src, LOCAL) == 0) {
		// We have local input defined
		padesc = open_audio_channel(DIR_IN, ppl->args->audio.in_hostapi, ppl->args->audio.in_dev);
		if (padesc == (AudioDescriptor*)NULL) {
			printf("c.server: Failed to open audio in!\n");
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
	for (i = 0; i < ppl->args->num_rx * 2; i++) {
		next_index = 0;
		found = FALSE;
		if (ppl->args->audio.routing.local[i].rx != -1) {
			// We have an assignment for this RX and the RX is configured
			// Find an entry if we have one
			next_index = output_index;
			for (j = 0; j <= output_index; j++) {
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
					printf("c.server: Failed to open audio out!\n");
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
			if (strcmp(ppl->args->audio.routing.local[i].ch, LEFT) == 0) {
				ppl->local_audio.local_output[next_index].dsp_ch_left = ppl->args->rx[ppl->args->audio.routing.local[i].rx].ch_id;
			}
			else if (strcmp(ppl->args->audio.routing.local[i].ch, RIGHT) == 0) {
				ppl->local_audio.local_output[next_index].dsp_ch_right = ppl->args->rx[ppl->args->audio.routing.local[i].rx].ch_id;
			}
			else {
				ppl->local_audio.local_output[next_index].dsp_ch_left = ppl->args->rx[ppl->args->audio.routing.local[i].rx].ch_id;
				ppl->local_audio.local_output[next_index].dsp_ch_right = ppl->args->rx[ppl->args->audio.routing.local[i].rx].ch_id;
			}
		}
	}
	ppl->local_audio.num_outputs = output_index;

	// Initialise the default audio routing for device 0 so we can revert after temporary changes
	audioDefault.rx_left = ppl->local_audio.local_output[0].dsp_ch_left;
	audioDefault.rx_right = ppl->local_audio.local_output[0].dsp_ch_right;
	return TRUE;
}

// Set sensible values for the initial gains
static void init_gains() {

	int i;

	// Audio gain
	for (i = 0; i < MAX_RX; i++) {
		ppl->gain[i] = 0.2;
	}
	// Mic gain
	ppl->mic_gain = 1.0;
	// RF Drive
	ppl->drive = 1.0;
}

// Initialise the wide band scope
static void init_wbs() {

	// Initialise for the wide bandscope FFT
	wbs_in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * WBS_SIZE * 2);
	wbs_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex) * WBS_SIZE * 2);
	wbs_plan = fftw_plan_dft_1d(WBS_SIZE * 2, wbs_in, wbs_out, FFTW_FORWARD, FFTW_ESTIMATE);
	hanning_window(WBS_SIZE);
}

// Create a DSP channel for each active receiver
// Cretae a single TX channel
static void create_dsp_channels() {

	// RX channels
	for (int ch = 0; ch < pargs->num_rx; ch++) {
		c_server_open_channel(CH_RX, pargs->rx[ch].ch_id, pargs->general.iq_blk_sz, pargs->general.mic_blk_sz, pargs->general.in_rate, pargs->general.out_rate, 0, 0, 0, 0);
		SetChannelState(pargs->rx[ch].ch_id, CH_STATE_START, CH_TRANSITION_WAIT);
	}
	// TX channel
	//c_server_open_channel(CH_TX, pargs->tx->ch_id, pargs->general.iq_blk_sz, pargs->general.mic_blk_sz, pargs->general.in_rate, pargs->general.out_rate, 0, 0, 0, 0);
}

// Create a display channel for each active receiver
static void create_display_channels() {
	
	for (int ch = 0; ch < pargs->num_rx; ch++) {
		c_server_open_display(	ch, 
								pargs->general.fft_size, 
								pargs->general.window_type, 
								1, 
								pargs->general.iq_blk_sz,
								pargs->general.display_width,
								pargs->general.av_mode,
								10, 
								pargs->general.in_rate,
								10 );
	}
	pipeline_run_display(TRUE);
}

// Set any new values in the cc data to override defaults
static void set_cc_data() {
	// Set num rx
	int num_rx = pargs->num_rx;
	if (num_rx == 1)
		cc_out_num_rx(NUM_RX_1);
	else if (num_rx == 2)
		cc_out_num_rx(NUM_RX_2);
	else if (num_rx == 3)
		cc_out_num_rx(NUM_RX_3);

	// Set rate
	int rate = pargs->general.in_rate;
	if (rate == 48000)
		cc_out_speed(S_48kHz);
	else if (rate == 96000)
		cc_out_speed(S_96kHz);
	else if (rate == 192000)
		cc_out_speed(S_192kHz);
}

// WBS functions
static void hanning_window(int size) {

	int i, j;
	float angle = 0.0F;
	float freq = M_PI * 2 / (float)size;
	int midn = size / 2;

	for (i = 0, j = size - 1, angle = 0.0F; i <= midn; i++, j--, angle += freq) {
		wbs_window[j] = (wbs_window[i] = 0.5F - 0.5F * (float)cos(angle));
		//printf("%d:%f ", j, wbs_window[j]);
	}
}

static float get_pwr_wbs(int index) {
	//printf("%f ", wbs_out[0][index] * wbs_out[0][index] + wbs_out[1][index] * wbs_out[1][index]);
	return (float)((wbs_out[index][0] * wbs_out[index][0]) + (wbs_out[index][1] * wbs_out[index][1]));
}
