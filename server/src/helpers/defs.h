/*
defs.h

'C' definitions for the SdrScript SDR application

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

#ifndef _defs_h
#define _defs_h

//#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
//#else
//    #define DLL_EXPORT __declspec(dllimport)
//#endif

#define HAVE_STRUCT_TIMESPEC

#define TRUE 1
#define FALSE 0

#define MAX_TX 1
#define MAX_RX 7

#define HPSDR "HPSDR"
#define LOCAL "Local"
#define FLDIGI "Fldigi"
#define CWSKIMMER "CWSkimmer"
#define WSJT "WSJT"
#define WSPR "WSPR"
#define JT65HF "JT65-HF"

#define IQ 0
#define MIC 1
#define CH_RX 0
#define CH_TX 1

#define LEFT "left"
#define RIGHT "right"
#define BOTH "both"

#define DIR_IN 0
#define DIR_OUT 1

#endif
