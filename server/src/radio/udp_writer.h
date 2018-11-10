/*
udp_writer.h

UDP writer header file

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

// Constants
// One USB frame is 63 24 bit samples
// Data is available as 16 bit samples
// Therefor we need 2 frames of 16bit I and Q
#define READ_SZ 63*2*4    // bytes

// Prototypes
void *udp_writer_imp();
void fcd_set_freq(unsigned int f);

#endif
