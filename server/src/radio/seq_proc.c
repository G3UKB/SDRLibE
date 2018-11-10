/*
seq_proc.h

UDP sequence number check and generation

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

int MAX_SEQ = 0;
int EP2_SEQ = 0;
int EP4_SEQ = 0;
int EP6_SEQ = 0;

void seq_init() {
	MAX_SEQ = (int)pow(2, 32);
}

int next_seq(int seq) {
	int new_seq;
	if (new_seq = seq++ > MAX_SEQ)
		return 0;
	else
		return new_seq;
}

int next_ep2_seq() {
	EP2_SEQ = next_seq(EP2_SEQ);
	// Return this as a byte array?
}

int next_ep4_seq() {
	EP4_SEQ = next_seq(EP4_SEQ);
	// Return this as a byte array?
}

int next_ep6_seq() {
	EP6_SEQ = next_seq(EP6_SEQ);
	// Return this as a byte array?
}

int check_ep2_seq(int seq) {

}

\ Sequence state
2 32 ^ var, max_seq
0 var, ep2_seq_no
- 1 var, ep2_seq_chk
0 var, ep4_seq_no
0 var, ep6_seq_no

\ Get next outgoing sequence number
	: seq_com
	1 n : +dup
	max_seq @ n : >
	if drop 0 then
		;
\ for EP2
	: next_ep2_seq
	ep2_seq_no @
	seq_com
	ep2_seq_no !
	a:new 0 ep2_seq_no @ a : !"I" pack
	;
\ for EP4
	: next_ep4_seq
	ep4_seq_no @
	seq_com
	ep4_seq_no !
	a:new 0 ep4_seq_no @ a : !"I" pack
	;
\ for EP6
	: next_ep6_seq
	ep6_seq_no @
	seq_com
	ep6_seq_no !
	a:new 0 ep6_seq_no @ a : !"I" pack
	;

\ Check incoming sequence number
\ for EP2
	: check_ep2_seq	\ seq -- f
	dup
	ep2_seq_chk @ - 1 n: = if
	\ First time so accept the current sequence
	ep2_seq_chk !
	drop true
	else
	\ Running sequence
	1 ep2_seq_chk n : +!
	ep2_seq_chk @ n : = if
	drop true
	else
	\ Not as expected
	dup 0 n: = if
	\ Cycled by to zero
	"Seq reset" log
	0 ep2_seq_chk !
	drop true
	else
	\ Missed one or more packets!
	"Seq error - " . "Exp: ".ep2_seq_chk @ . " Got: ".dup.cr
	ep2_seq_chk !
	false
	then
	then
	then
	;