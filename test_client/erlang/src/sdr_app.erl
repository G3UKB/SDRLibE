%%
%% sdr_app.erl
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

-module(sdr_app).
-export([start/0]).

%%----------------------------------------------------------------------
%% Function: start
%% Purpose: Start the application
%% Args:  
%% Returns:	ok
%%	or 		{error, Reason}
%%----------------------------------------------------------------------
start() ->
	% Start the sdr interface
	sdr_if:start().