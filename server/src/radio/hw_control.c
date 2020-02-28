/*
hw_control.c

Control API functions for the radio hardware

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

// Module vars
// Client and server addresses
struct sockaddr_in bcAddr, svrAddr;
unsigned char msg[MAX_MSG];
unsigned char resp[MAX_RESP];

// Forward refs
static int udprecvcontrol(struct sockaddr *svr_addr, int sd);

// Discover protocol
int do_discover(struct sockaddr_in *srv_addr, int sd) {
	// Send discovery message
	// Clear message buffer
	memset(msg, 0x0, MAX_MSG);
	msg[0] = 0xEF;
	msg[1] = 0xFE;
	msg[2] = 0x02;

#ifdef linux
	memset(&bcAddr, '\0', sizeof(struct sockaddr_in));
	bcAddr.sin_family = AF_INET;
	bcAddr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
	bcAddr.sin_port = htons(REMOTE_SERVER_PORT);
#else
	// Set BC address
	memset((char *)&bcAddr, 0, sizeof(bcAddr));
	bcAddr.sin_family = AF_INET;
	bcAddr.sin_addr.S_un.S_addr = inet_addr("255.255.255.255");
	bcAddr.sin_port = htons(REMOTE_SERVER_PORT);
#endif

	// Dispatch
	if (sendto(sd, (const char*)msg, MAX_MSG, 0, (const struct sockaddr*) &bcAddr, sizeof(bcAddr)) == -1) {
		return (struct sockaddr_in *)NULL;
	}

	// Waiting for discovery response
	return udprecvcontrol(srv_addr, sd);
}

// Start radio hardware
int do_start(int sd, struct sockaddr_in *svrAddr, int wbs) {
	// Send start message
	// Clear message buffer
	memset(msg, 0x0, MAX_MSG);
	msg[0] = 0xEF;
	msg[1] = 0xFE;
	msg[2] = 0x04;
	if (wbs) {
		// Set wide band scope on
		msg[3] = 0x03;
	} else {
		msg[3] = 0x01;
	}

	// Dispatch
	if (sendto(sd, (const char*)msg, MAX_MSG, 0, (const struct sockaddr*) svrAddr, sizeof(struct sockaddr)) == -1) {
		printf("Start failed\n");
		return FALSE;
	}

	return TRUE;
}

// Stop streaming
int do_stop(int sd, struct sockaddr_in *svrAddr) {
	// Send stop message
	// Clear message buffer
	memset(msg, 0x0, MAX_MSG);
	msg[0] = 0xEF;
	msg[1] = 0xFE;
	msg[2] = 0x04;

	// Dispatch
	if (sendto(sd, (const char*)msg, MAX_MSG, 0, (const struct sockaddr*) svrAddr, sizeof(*svrAddr)) == -1) {
		return FALSE;
	}

	return TRUE;
}

// Receive one packet from the client
static int udprecvcontrol(struct sockaddr *svr_addr, int sd) {
	int n;
	int svr_len = sizeof(*svr_addr);
	// Clear message buffer
	memset(msg, 0x0, MAX_MSG);
	// Receive message
	// Allow 2s for response
	int count = 10;
	int success = FALSE;
	while (--count > 0) {
		n = recvfrom(sd, (char*)msg, MAX_MSG, 0, svr_addr, &svr_len);
		if (n > 0) {
			success = TRUE;
			break;
		}
		else {
			Sleep(200);
		}
	}
	
	return success;
}
