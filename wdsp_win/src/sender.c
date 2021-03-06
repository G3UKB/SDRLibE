/*  sender.c

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

#include "comm.h"

SENDER create_sender (int run, int flag, int mode, int size, double* in, int arg0, int arg1, int arg2, int arg3)
{
	SENDER a = (SENDER) malloc0 (sizeof (sender));
	a->run = run;
	a->flag = flag;
	a->mode = mode;
	a->size = size;
	a->in = in;
	a->arg0 = arg0;
	a->arg1 = arg1;
	a->arg2 = arg2;
	a->arg3 = arg3;
	a->out = (double *) malloc0 (a->size * sizeof (complex));
	return a;
}

void destroy_sender (SENDER a)
{
	_aligned_free (a->out);
	_aligned_free (a);
}

void flush_sender (SENDER a)
{
	memset (a->out, 0, a->size * sizeof (complex));
}

void xsender (SENDER a)
{
	if (a->run && a->flag)
	{
		switch (a->mode)
		{
		case 0:
			{
				int i;
				float* outf = (float *)a->out;
				for (i = 0; i < a->size; i++)
				{
					outf [2 * i + 0] = (float)a->in[2 * i + 1];	// note the I/Q swap
					outf [2 * i + 1] = (float)a->in[2 * i + 0];
				}
				Spectrum2 (a->arg0, a->arg1, a->arg2, outf);	// Spectrum2 (disp, ss, LO, buff);
				break;
			}
		}
	}
}

/********************************************************************************************************
*																										*
*											RXA Properties												*
*																										*
********************************************************************************************************/

void SetRXASpectrum (int channel, int flag, int disp, int ss, int LO)
{
	SENDER a;
	EnterCriticalSection (&ch[channel].csDSP);
	a = rxa[channel].sender.p;
	a->flag = flag;
	a->arg0 = disp;
	a->arg1 = ss;
	a->arg2 = LO;
	LeaveCriticalSection (&ch[channel].csDSP);
}