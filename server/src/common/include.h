/*
include.h

Common include file

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

#ifndef _include_h
#define _include_h

// System includes
#include <stdio.h>
#include <winsock2.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <ws2tcpip.h>
#include <Inaddr.h>

#define HAVE_STRUCT_TIMESPEC

// Libs
#include "../../../../libs/pthreads/include/pthread.h"
#include "../../../../libs/fftw/fftw3.h"
#include "../../../../libs/portaudio/include/portaudio.h"

// Application includes
// Helpers
#include "defs.h"
#include "../helpers/utils.h"
#include "../ringbuffer/ringb.h"
#include "../json/cJSON.h"
// Local audio
#include "../audio/local_audio.h"
// Lib interface
#include "../radio/sockets.h"
#include "../server/server.h"
#include "../server/dsp_man.h"
// Pipeline processing
#include "../pipeline/pipeline.h"
// Radio hardware interfacing and processing
#include "radio_defs.h"
#include "../radio/hw_control.h"
#include "../radio/udp_reader.h"
#include "../radio/udp_writer.h"
#include "../radio/cc_in.h"
#include "../radio/cc_out.h"
#include "../radio/seq_proc.h"
#include "../radio/encoder.h"
#include "../radio/decoder.h"
// WDSP
#include "../../../wdsp_win/src/channel.h"
#include "../../../wdsp_win/src/wisdom.h"
#include "../../../wdsp_win/src/bandpass.h"

#endif
