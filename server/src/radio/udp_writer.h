/*
udp_writer.h

Write UDP radio data stream

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

#ifndef _udp_writer_h
#define _udp_writer_h

//==================================================================

// Prototypes
void writer_init(int sd, struct sockaddr_in *srv_addr);
void prime_radio(int sd, struct sockaddr_in *srv_addr);
void write_data(int sd, struct sockaddr_in *srv_addr);

#endif
