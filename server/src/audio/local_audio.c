/*
local_audio.c

'C' local sudio control for the SdrScript SDR application

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

// Includes
#include "../common/include.h"

// Local functions
static PaStream* open_stream(UserData *paud, int direction, char* hostapi, char *device);
static int pa_stream_callback( 	const void *inputBuffer, void *outputBuffer,
								unsigned long framesPerBuffer,
								const PaStreamCallbackTimeInfo* timeInfo,
								PaStreamCallbackFlags statusFlags,
								void *userData );
static int __compatible(int dev, int direction, int format, double rate, int chs);

// Local data
int initialised = FALSE;
int stream_id = 0;
UserData* user_data[MAX_RX + MAX_TX];
AudioDescriptor* audio_desc[MAX_RX + MAX_TX];
DeviceEnumList* device_enum_list = NULL;


PaErrorCode audio_init() {
	/* Init the local audio server
	 *
	 * Arguments:
	 *
	 */

	int i;
	int r = TRUE;
	if (!initialised) {
		for (i=0 ; i < MAX_RX + MAX_TX ; i++) {
			user_data[i] = (UserData *)NULL;
			audio_desc[i] = (AudioDescriptor*)NULL;
		}

		// Create enum data structure
		device_enum_list = (DeviceEnumList *)safealloc(sizeof(DeviceEnumList), sizeof(char), "DEVICE_ENUM_STRUCT");

		r = Pa_Initialize();
		initialised = TRUE;
		}
	printf("Initialised local audio %d", r);
	return r;
}

PaErrorCode audio_uninit() {
	/* Uninit the local audio server
	 *
	 * Arguments:
	 *
	 */

	int i;
	int r = paNoError;

	if (initialised) {
		// Terminate streams

		for (i=0 ; i < MAX_RX + MAX_TX ; i++) {
			if (user_data[i] != (UserData *)NULL) {
				Pa_CloseStream(user_data[i]->stream);
				ringb_free(user_data[i]->rb);
			}
			if (audio_desc[i] != (AudioDescriptor*)NULL) {
				safefree((char*)audio_desc[i]);
			}
		}
		if (device_enum_list != NULL) {
			safefree((char*)device_enum_list);
		}
		stream_id = 0;

		// Terminate portaudio
		r = Pa_Terminate();
		initialised = FALSE;
	}
	return r;
}

const char * audio_get_last_error(int id) {
	/* Get the last error thrown by the given audio server
	 *
	 * Arguments:
	 * 		id	--	the stream id
	 */

	return user_data[id]->last_error;
}

DeviceEnumList* enum_inputs() {
	/* Get list of audio input device parameters
	 * We want to select inputs that are compatible with the data type and rate
	 * that is required by the pipeline.
	 *
	 * rate: 48k format: paFloat32
	 *
	 * Arguments:
	 *
	 */

	int dev, numDevices;
	const PaDeviceInfo *deviceInfo;
	double sample_rate = 48000.0;
	int index = 0;

	if (!initialised)
		audio_init();

	// Get the number of devices
	numDevices = Pa_GetDeviceCount();
	if( numDevices <= 0 ) {
		return (DeviceEnumList*) NULL;
	}
	// Iterate the device info by index
	for( dev=0; dev<numDevices; dev++ ) {
		deviceInfo = Pa_GetDeviceInfo(dev);
		if ((deviceInfo->maxInputChannels > 0) && __compatible(dev, DIR_IN, paFloat32, sample_rate, 1)) {
			device_enum_list->devices[index].direction = DIR_IN;
			device_enum_list->devices[index].index = dev;
			strcpy(device_enum_list->devices[index].name, deviceInfo->name);
			device_enum_list->devices[index].channels = deviceInfo->maxInputChannels;
			strcpy(device_enum_list->devices[index].host_api, Pa_GetHostApiInfo(deviceInfo->hostApi)->name);
			index++;
		}
	}
	device_enum_list->entries = index+1;

	return device_enum_list;
}

DeviceEnumList* enum_outputs() {
	/* Get list of audio output device parameters
	 * We want to select outputs that are compatible with the data type and rate
	 * that is required by the pipeline.
	 *
	 * rate: 48k format: paFloat32
	 *
	 * Arguments:
	 *
	 */

	int dev, numDevices;
	const PaDeviceInfo *deviceInfo;
	double sample_rate = 48000.0;
	int index = 0;
	char* default_name[50];
	default_name[0] = '\0';

	if (!initialised)
		audio_init();

	// Get the number of devices
	numDevices = Pa_GetDeviceCount();
	if( numDevices <= 0 ) {
		return (DeviceEnumList*) NULL;
	}

	// Get the default device name
	deviceInfo = Pa_GetDeviceInfo(Pa_GetDefaultOutputDevice());
	strcpy(default_name, deviceInfo->name);

	// Iterate the device info by index
	for( dev=0; dev<numDevices; dev++ ) {
		deviceInfo = Pa_GetDeviceInfo(dev);
		if ((deviceInfo->maxOutputChannels > 0) && __compatible(dev, DIR_OUT, paFloat32, sample_rate, 2)) {
			// For now we stick to looking for devices on Windows which are Speakers on Line outputs.
			// For Linux/RPi we just look for ALSA host API devices
#ifdef linux
			if (strstr(Pa_GetHostApiInfo(deviceInfo->hostApi)->name, "ALSA") != NULL) {
#else
			if ((strstr(deviceInfo->name, "Speakers") != NULL) || (strstr(deviceInfo->name, "Line") != NULL)) {
#endif
				printf("Name: %s", deviceInfo->name);
				device_enum_list->devices[index].direction = DIR_OUT;
				device_enum_list->devices[index].index = dev;
				strcpy(device_enum_list->devices[index].name, deviceInfo->name);
				device_enum_list->devices[index].channels = deviceInfo->maxOutputChannels;
				strcpy(device_enum_list->devices[index].host_api, Pa_GetHostApiInfo(deviceInfo->hostApi)->name);
				if (strcmp(deviceInfo->name, default_name) == 0) device_enum_list->devices[index].default_id = TRUE;
				else  device_enum_list->devices[index].default_id = FALSE;
				index++;
			}
		}
	}
	device_enum_list->entries = index+1;

	return device_enum_list;
}

AudioDescriptor *open_audio_channel(int direction, char* hostapi, char *device) {
	/* Create an audio channel
	 *
	 * Arguments:
	 * 	direction	--	DIR_IN | DIR_OUT
	 * 	hostapi		--	qualifier to device name
	 * 	device		--	audio device name to use
	 *
	 */

	// An audio output channel consists of
	//	o A ring buffer interface
	//	o A thread which runs the channel
	// 	o A portaudio output channel
	// The format is always as follows
	//	o Interleaved L/R channels
	//	o Format float
	//	o 48K sample rate
	// 	o 512 sample size making float 1024 L/R sz transfers
	//

	PaStream *stream;

	int current_stream = stream_id;
	stream_id++;
	//printf("Open %d,%s,%s\n", direction, hostapi, device);
	// Create a descriptor and user data structure
	audio_desc[current_stream] = (AudioDescriptor *)safealloc(sizeof(AudioDescriptor), sizeof(char), "AUDIO_DESC");
	// Create and populate a UserData structure
	user_data[current_stream] = (UserData *)safealloc(sizeof(UserData), sizeof(char), "AUDIO_USER_DATA");
	user_data[current_stream]->direction = direction;
	strcpy(user_data[current_stream]->last_error, "");
	// Ring buffer to hold 8 data blocks of L/R samples
	user_data[current_stream]->rb = ringb_create (1024 * 4 * 32);
	// Open portaudio stream
	if ((stream = open_stream(user_data[current_stream], direction, hostapi, device)) == (AudioDescriptor *)NULL) {
		strcpy(user_data[current_stream]->last_error, Pa_GetLastHostErrorInfo()->errorText);
		return (AudioDescriptor *)NULL;
	}
	user_data[current_stream]->stream = stream;

	// Return the descriptor for this audio server
	audio_desc[current_stream]->stream_id = current_stream;
	audio_desc[current_stream]->stream = stream;
	audio_desc[current_stream]->rb = user_data[current_stream]->rb;
	return audio_desc[current_stream];
}

PaErrorCode audio_start_stream (PaStream *stream) {
	/* Start a portaudio stream
	 *
	 * Arguments:
	 * 	stream	--	the stream
	 *
	 */
	return Pa_StartStream (stream);
}

PaErrorCode audio_stop_stream (PaStream *stream) {
	/* Stop a portaudio stream
	 *
	 * Arguments:
	 * 	stream	--	the stream
	 *
	 */

	return Pa_StopStream (stream);
}

static PaStream* open_stream(UserData *paud, int direction, char* hostapi, char *device) {
	/* Open a portaudio stream
	 *
	 * Arguments:
	 * 	direction	--	DIR_IN | DIR_OUT
	 * 	hostapi		--	qualifier to device name
	 * 	device		--	audio device name to use
	 *
	 */

	// The format is always as follows
	//	o Interleaved L/R channels
	//	o Format paInt16
	//	o 48K sample rate
	// 	o 512 sample size making float 1024 L/R sz transfers
	//

	int dev, numDevices;
	const PaDeviceInfo *deviceInfo;
	PaStreamParameters inputParameters;
	PaStreamParameters outputParameters;
	double sample_rate = 48000;
	unsigned long framesPerBuffer = 1024;
	PaStream *stream = (PaStream *)NULL;
	PaError err = paNoError;
	
	// Get the number of devices
	numDevices = Pa_GetDeviceCount();
	if( numDevices < 0 ) {
		strcpy(paud->last_error, "Invalid Channel Count!");
		return (PaStream *)NULL;
	}
	// Iterate the device info by index
	for( dev=0; dev<numDevices; dev++ ) {
		deviceInfo = Pa_GetDeviceInfo(dev);
		//printf("Dev name %s, name %s, hostapi %s, host %s, maxip %d, maxop %d\n", deviceInfo->name, device, Pa_GetHostApiInfo(deviceInfo->hostApi)->name, hostapi, deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels);
		if ((strcmp(deviceInfo->name, device) == 0) && (strcmp(Pa_GetHostApiInfo(deviceInfo->hostApi)->name, hostapi) == 0)) {
			// Found our device on the correct hostapi
			// Check we can support the direction
			//if (((direction == DIR_IN) && (deviceInfo->maxInputChannels <= 0)) || ((direction == DIR_OUT) && (deviceInfo->maxOutputChannels <= 0))) {
			//	paud->last_error = paDeviceUnavailable;
			//	return (PaStream *)NULL;
			//}
			// Set up the parameters to open the stream
			if ((direction == DIR_IN) && (deviceInfo->maxInputChannels > 0)) {
				memset (&inputParameters, 0, sizeof (inputParameters));
				inputParameters.channelCount = 1;
				inputParameters.device = dev;
				inputParameters.hostApiSpecificStreamInfo = NULL;
				inputParameters.sampleFormat = paInt16;
				inputParameters.suggestedLatency = Pa_GetDeviceInfo(dev)->defaultLowInputLatency;
				inputParameters.hostApiSpecificStreamInfo = NULL;
				// Check we can support the sample rate
				err = Pa_IsFormatSupported(&inputParameters, NULL, sample_rate);
				if (err != paFormatIsSupported) {
					strcpy(paud->last_error, "Invalid sample rate!");
					return (PaStream *)NULL;
				}
				// Open stream
				err = Pa_OpenStream(
							&stream,
							&inputParameters,
							NULL,
							sample_rate,
							framesPerBuffer,
							paNoFlag,
							pa_stream_callback,
							(void *)paud );
			} else if ((direction == DIR_OUT) && (deviceInfo->maxOutputChannels > 0)) {
				memset (&outputParameters, 0, sizeof (outputParameters));
				outputParameters.channelCount = 2;
				outputParameters.device = dev;
				outputParameters.hostApiSpecificStreamInfo = NULL;
				outputParameters.sampleFormat = paInt16;
				outputParameters.suggestedLatency = Pa_GetDeviceInfo(dev)->defaultLowOutputLatency ;
				outputParameters.hostApiSpecificStreamInfo = NULL;
				err = Pa_IsFormatSupported(NULL, &outputParameters, sample_rate);
				if (err != paFormatIsSupported) {
					strcpy(paud->last_error, "Invalid sample rate!");
					printf("Invalid sample rate!\n");
					return (PaStream *)NULL;
				}
				// Open stream
				err = Pa_OpenStream(
							&stream,
							NULL,
							&outputParameters,
							sample_rate,
							framesPerBuffer,
							paNoFlag,
							pa_stream_callback,
							(void *)paud );
			}
			break;
		}
	}
	if (err == paNoError)
		return stream;
	else {
		printf("Audio error %d!\n", err);
		return (PaStream *)NULL;
	}
}

static int pa_stream_callback( 	const void *inputBuffer, void *outputBuffer,
								unsigned long framesPerBuffer,
								const PaStreamCallbackTimeInfo* timeInfo,
								PaStreamCallbackFlags statusFlags,
								void *userData ) {
	/* Re-entrant callback from portaudio
	 *
	 * Arguments:
	 * 	inputBuffer
	 * 	outputBuffer
	 * 	framesPerBuffer
	 * 	timeInfo
	 * 	statusFlags
	 * 	userData
	 *
	 */

	// All allocated streams will callback here with
	// 	data available(DIR_IN) or
	// 	data required (DIR_OUT)
	UserData *pud = (UserData *)userData;
	if (pud->direction == DIR_IN) {
		// Mono stream
		if (ringb_write_space (pud->rb) >= framesPerBuffer*2) {
			ringb_write (pud->rb, (char *)inputBuffer, framesPerBuffer*2);
		}
	}else if (pud->direction == DIR_OUT) {
		// Stereo stream
		//printf("Audio: %d\n", framesPerBuffer * 4);
		if (ringb_read_space (pud->rb) >= framesPerBuffer*4) {
			ringb_read (pud->rb, (char *)outputBuffer, framesPerBuffer*4);
		} else {
			memset((char *)outputBuffer, 0, framesPerBuffer*4);
		}
	}

	return 0;
}

static int __compatible(int dev, int direction, int format, double rate, int chs) {

	PaStreamParameters inputParameters;
	PaStreamParameters outputParameters;
	PaError err;

	if (direction == DIR_IN) {
		memset (&inputParameters, 0, sizeof (inputParameters));
		inputParameters.channelCount = chs;
		inputParameters.device = dev;
		inputParameters.hostApiSpecificStreamInfo = NULL;
		inputParameters.sampleFormat = format;
		inputParameters.suggestedLatency = Pa_GetDeviceInfo(dev)->defaultLowInputLatency;
		inputParameters.hostApiSpecificStreamInfo = NULL;
		err = Pa_IsFormatSupported(&inputParameters, NULL, rate);
	} else {
		memset (&outputParameters, 0, sizeof (outputParameters));
		outputParameters.channelCount = chs;
		outputParameters.device = dev;
		outputParameters.hostApiSpecificStreamInfo = NULL;
		outputParameters.sampleFormat = format;
		outputParameters.suggestedLatency = Pa_GetDeviceInfo(dev)->defaultLowOutputLatency ;
		outputParameters.hostApiSpecificStreamInfo = NULL;
		err = Pa_IsFormatSupported(NULL, &outputParameters, rate);
	}

	if (err == paFormatIsSupported) {
		return TRUE;
	}

	return FALSE;
}
