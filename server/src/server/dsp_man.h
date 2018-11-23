/*
dsp_man.h

DSP Management functions

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

// Prototypes
void c_server_open_channel(int ch_type, int ch_instance, int iq_size, int mic_size, int in_rate, int out_rate, int tdelayup, int tslewup, int tdelaydown, int tslewdown);
void c_server_close_channel(int channel);
void c_server_open_display(int display, int fft_size, int win_type, int sub_spans, int in_sz, int display_width, int average_mode, int over_frames, int sample_rate, int frame_rate);
void c_impl_server_set_display(int display_id, int fft_size, int win_type, int sub_spans, int in_sz, int display_width, int average_mode, int over_frames, int sample_rate, int frame_rate);
void c_server_close_display(int display_id);
