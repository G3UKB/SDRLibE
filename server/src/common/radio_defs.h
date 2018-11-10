/*
defs.h

Common definitions for the SDRLibE library
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

#ifndef _defs_h
#define _defs_h

#define DLL_EXPORT __declspec(dllexport)
//#define DLL_EXPORT __stdcall

// Defines
#define TRUE 1
#define FALSE 0

#define FRAME_SZ 1032
#define DATA_SZ 504
#define START_FRAME_1 16
#define END_FRAME_1 520
#define START_FRAME_2 528
#define END_FRAME_2 1032
#define NUM_SMPLS 126

#endif
