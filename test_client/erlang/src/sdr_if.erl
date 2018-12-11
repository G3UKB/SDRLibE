%%
%% sdr_if.erl
%% 
%% Copyright (C) 2018 by G3UKB Bob Cowdery
%% This program is free software; you can redistribute it and/or modify
%% it under the terms of the GNU General Public License as published by
%% the Free Software Foundation; either version 2 of the License, or
%% (at your option) any later version.
%%    
%%  This program is distributed in the hope that it will be useful,
%%  but WITHOUT ANY WARRANTY; without even the implied warranty of
%%  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
%%  GNU General Public License for more details.
%%    
%%  You should have received a copy of the GNU General Public License
%%  along with this program; if not, write to the Free Software
%%  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
%%    
%%  The author can be reached by email at:   
%%     bob@bobcowdery.plus.com
%%

-module(sdr_if).
-export([start/0, get_resp/1]).

%% Constants
-define(CMD_PORT, 10010).

%%----------------------------------------------------------------------
%% Function: start
%% Purpose: Start the application
%% Args:  
%% Returns:	ok
%%	or 		{error, Reason}
%%----------------------------------------------------------------------
start() ->
	% Start the sdr interface
	{ok, Socket} = gen_udp:open(0, [binary]),	
	Msg = msg(enum_outputs),
	gen_udp:send(Socket, "localhost", ?CMD_PORT, Msg),
	Resp = get_resp(Socket),
	Dec = jsone:decode(Resp),
	io:format(Dec),
	io:format(Resp).

get_resp(Socket) ->
	receive
		{udp, Socket,_,_,Bin} ->
			Bin
	after 2000 ->
		error
	end.

%%----------------------------------------------------------------------
%% Function: message
%% Purpose: Return the selected message
%% Args:  
%% Returns:	message
%%	or 		{error, Reason}
%%----------------------------------------------------------------------
msg(discover) -> jsone:encode(#{<<"cmd">> => <<"discover">>, <<"params">> => []});
msg(enum_outputs) -> jsone:encode(#{<<"cmd">> => <<"enum_outputs">>, <<"params">> => []});
msg(set_audio_route) -> jsone:encode(#{<<"cmd">> => <<"set_audio_route">>, <<"params">> => []});
msg(server_start) -> jsone:encode(#{<<"cmd">> => <<"server_start">>, <<"params">> => []});
msg(radio_start) -> jsone:encode(#{<<"cmd">> => <<"radio_start">>, <<"params">> => [0]}).
