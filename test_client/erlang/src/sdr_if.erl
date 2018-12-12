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
	discover(Socket).

	%Msg = msg(enum_outputs),
	%gen_udp:send(Socket, "localhost", ?CMD_PORT, Msg),
	%Resp = get_resp(Socket),
	%Map = jsone:decode(Resp),
	%List = maps:get(<<"outputs">>, Map),
	%Element = lists:nth(2, List),
	%Api = my_binary_to_list(maps:get(<<"api">>, Element)),
	%Dev = my_binary_to_list(maps:get(<<"name">>, Element)),
	%io:format("~p, ~p~n", [Api, Dev]).

discover(Socket) ->
	gen_udp:send(Socket, "localhost", ?CMD_PORT, msg(discover)),
	decode_resp (get_resp(Socket)).

get_resp(Socket) ->
	receive
		{udp, Socket,_,_,Bin} ->
			Bin
	after 5000 ->
		exit("No response received!")
	end.

decode_resp(Resp) ->
	Map = jsone:decode(Resp),
	List = binary_to_list(maps:get(<<"resp">>, Map)),
	io:format("Resp: ~p~n", [List]).


my_binary_to_list(<<H,T/binary>>) ->
    [H|my_binary_to_list(T)];
my_binary_to_list(<<>>) -> [].

%%----------------------------------------------------------------------
%% Function: message
%% Purpose: Return the selected message
%% Args:  
%% Returns:	message
%%	or 		{error, Reason}
%%----------------------------------------------------------------------
msg(discover) -> jsone:encode(#{<<"cmd">> => <<"radio_discover">>, <<"params">> => []});
msg(enum_outputs) -> jsone:encode(#{<<"cmd">> => <<"enum_outputs">>, <<"params">> => []});
msg(set_audio_route) -> jsone:encode(#{<<"cmd">> => <<"set_audio_route">>, <<"params">> => []});
msg(server_start) -> jsone:encode(#{<<"cmd">> => <<"server_start">>, <<"params">> => []});
msg(radio_start) -> jsone:encode(#{<<"cmd">> => <<"radio_start">>, <<"params">> => [0]}).
