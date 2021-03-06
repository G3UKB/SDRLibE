/*  siphon.h

This file is part of a program that implements a Software-Defined Radio.

Copyright (C) 2013 Warren Pratt, NR0V

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

The author can be reached by email at

warren@wpratt.com

*/

// 'siphon' collects samples in a buffer.  These samples can then be PULLED from the buffer
//	in either raw or FFT'd form.

#ifndef _siphon_h
#define _siphon_h

typedef struct _siphon
{
	int run;
	int insize;
	double* in;
	int sipsize;	// NOTE:  sipsize MUST BE A POWER OF TWO!!
	double* sipbuff;
	int outsize;
	int idx;
	double* sipout;
	int fftsize;
	double* specout;
	int specmode;
	fftw_plan sipplan;
	double* window;
} siphon, *SIPHON;

extern SIPHON create_siphon (int run, int insize, double* in, int sipsize, int fftsize, int specmode);

extern void destroy_siphon (SIPHON a);

extern void flush_siphon (SIPHON a);

extern void xsiphon (SIPHON a);

// RXA Properties

void RXAGetaSipF  (int channel, float* out, int size);

void RXAGetaSipF1 (int channel, float* out, int size);

// TXA Properties

void TXAGetaSipF  (int channel, float* out, int size);

void TXAGetaSipF1 (int channel, float* out, int size);

void TXAGetSpecF1 (int channel, float* out);

#endif
