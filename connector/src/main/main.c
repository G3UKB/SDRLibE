/*
main.c

'C' server UDP Connector API functions for the SDRLibE SDR back-end

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

// Include all server headers
#include "../common/include.h"

/*
	This connector provides the startup code and a UDP command/response interface for driving
	the server. Commands and responses are Json encoded. The interface is kept as simple as
	possible and therefore create/manage of the server objects is as far as possible done
	automatically from the connector rather than expose those functions to the front-end.
*/

void SignalHandler(int signal)
{
	printf("Here!!\n");
	if (signal == SIGTERM) {
		// abort signal handler code  
		printf("Terminating...\n");
	}
}

// Entry point
int main() {

	// Announce
	printf("SDRLibE Connector starting...\n");
	printf("(Press any key to exit)\n\n\n");

	typedef void(*SignalHandlerPointer)(int);

	SignalHandlerPointer previousHandler;
	previousHandler = signal(SIGTERM, SignalHandler);

	// First see if we need to make the wisdom file
	c_server_make_wisdom("..\\wisdom\\");

	// Initialise Winsock 
	WSADATA wsa;                    // Winsock
	char last_error[128];           // Holder for last error
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Connector: Failed to initialise winsock! [%s]\n", last_error);
		exit (-1);
	}
	
	// Initialise command UDP interface
	if (!conn_udp_init()) {
		printf("Connector: Failed to initialise Connector UDP interface!\n");
		exit(-1);
	}
	
	// Initialise event UDP interface
	if (!conn_evnt_udp_init()) {
		printf("Connector: Failed to initialise Connector Event UDP interface!\n");
		exit(-1);

	}
	// Initialise server
	c_server_init();
	
	// Run UDP interface
	conn_udp_start();
	
	// Testing
	/*
	DeviceEnumList* audio_outputs = c_server_enum_audio_outputs();
	
	// Iterate the list and populate the Json structure
	for (int i = 0; i < audio_outputs->entries; i++) {
		// Create an object to add items to
		printf("%s\n", audio_outputs->devices[i].name);
		printf("%s\n", audio_outputs->devices[i].host_api);
		printf("%d\n", audio_outputs->devices[i].index);
		printf("%d\n", audio_outputs->devices[i].direction);
		printf("%d\n", audio_outputs->devices[i].channels);
	}
	*/

	getch();
	
	// Announce
	printf("SDRLibE Connector closing...\n");

	// Tidy up

}