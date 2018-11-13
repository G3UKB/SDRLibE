/*
local_audio.h

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

/*
#include <stdio.h>
#include <string.h>

#include "../helpers/defs.h"
#include "../../../../libs/portaudio/include/portaudio.h"
#define HAVE_STRUCT_TIMESPEC
#include "../../../../libs/pthreads/include/pthread.h"
#include "../ringbuffer/ringb.h"
*/

#ifndef _local_audio_h
#define _local_audio_h

// Passed to the audio callback
typedef struct UserData {
	ringb_t *rb;
	int direction;
	PaStream* stream;
	char last_error[128];
}UserData;

// Returned by open_audio_channel
typedef struct AudioDescriptor {
	int stream_id;
	PaStream* stream;
	ringb_t *rb;
}AudioDescriptor;

// Returned from device enumerator
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

// Prototypes
PaErrorCode audio_init();
PaErrorCode audio_uninit();
const char * audio_get_last_error(int id);
DeviceEnumList* enum_inputs();
DeviceEnumList* enum_outputs();
AudioDescriptor *open_audio_channel(int direction, char* hostapi, char *device);
PaErrorCode  audio_start_stream (PaStream *stream);
PaErrorCode  audio_stop_stream (PaStream *stream);

#endif
