/*
sockets.c

Manage UDP sockets

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

// Open a broadcast socket
int open_bc_socket() {
	int sd;
	int broadcast = 1;
	struct timeval tv;
	tv.tv_sec = 4;
	tv.tv_usec = 0;

	// Create socket
	sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sd<0) {
		return -1;
	}

	// Set to broadcast
	if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof broadcast) == -1) {
		return -1;
	}

	// Set receive timeout
	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (const const char*)&tv, sizeof tv) == -1) {
		printf("Failed to set option SO_RCVTIMEO!\n");
		return FALSE;
	}

	return sd;
}

// Revert broadcast socket to a normal socket
int revert_sd(int sd) {
	int broadcast = 0;
	int sendbuff = 32000;
	int recvbuff = 32000;
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	// Turn off broadcast
	if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof broadcast) == -1) {
		printf("Failed to set option SO_BROADCAST!\n");
		return FALSE;
	}
	// Set send buffer size
	if (setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const char*)&sendbuff, sizeof(sendbuff)) == -1) {
		printf("Failed to set option SO_SNDBUF!\n");
		return FALSE;
	}
	// Set receive buffer size
	if (setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const char*)&recvbuff, sizeof(recvbuff)) == -1) {
		printf("Failed to set option SO_RCVBUF!\n");
		return FALSE;
	}
	// Set receive timeout
	if (setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (const const char*)&tv, sizeof tv) == -1) {
		printf("Failed to set option SO_RCVTIMEO!\n");
		return FALSE;
	}
	return TRUE;
}
