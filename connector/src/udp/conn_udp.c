/*
conn_udp.c

Connector UDP interface

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

// Include all server headers
#include "../common/include.h"

#define SVR_ADDR "127.0.0.1"
//#define SVR_ADDR "192.168.1.8"

//==========================================================================================
// Local functions
static void* udp_conn_imp(void* data);
static void udpconndata(UDPConnThreadData* td);
static void send_conn_resp(int sd, struct sockaddr_in* conn_cli_addr, char* resp);
static char* encode_ack_nak(char* data);

// Execution functions
// Heartbeat
static char* c_conn_poll(cJSON *params);
// General set functions
static char* c_conn_set_num_rx(cJSON *params);
static char* c_conn_set_in_rate (cJSON *params);
static char* c_conn_set_out_rate (cJSON *params);
static char* c_conn_set_iq_blk_sz (cJSON *params);
static char* c_conn_set_mic_blk_sz (cJSON *params);
static char* c_conn_set_duplex (cJSON *params);
static char* c_conn_set_fft_size (cJSON *params);
static char* c_conn_set_window_type (cJSON *params);
static char* c_conn_set_av_mode (cJSON *params);
static char* c_conn_set_display_width(cJSON *params);
static char* c_conn_set_audio_route(cJSON *params);
// Audio functions
static char* c_conn_enum_audio_inputs(cJSON *params);
static char* c_conn_enum_audio_outputs(cJSON *params);
static char* c_conn_change_audio_outputs(cJSON *params);
static char* c_conn_revert_audio_outputs(cJSON *params);
static char* c_conn_local_audio_run(cJSON *params);
static char* c_conn_clear_audio_routes(cJSON *params);
static char* c_conn_restart_audio_routes(cJSON *params);
// Management functions
static char* c_conn_server_start(cJSON *params);
static char* c_conn_server_terminate(cJSON *params);
static char* c_conn_radio_discover(cJSON *params);
static char* c_conn_radio_start(cJSON *params);
static char* c_conn_radio_stop(cJSON *params);
// Hardware control functions
static char* c_conn_cc_out_set_rx_1_freq(cJSON *params);
static char* c_conn_cc_out_set_rx_2_freq(cJSON *params);
static char* c_conn_cc_out_set_rx_3_freq(cJSON *params);
static char* c_conn_cc_out_set_tx_freq(cJSON *params);
static char* c_conn_cc_out_set_hf_pre(cJSON *params);
static char* c_conn_cc_out_set_attn(cJSON *params);
// Alex filters
static char* c_conn_cc_out_alex_auto(cJSON *params);
static char* c_conn_cc_out_alex_hpf_bypass(cJSON *params);
static char* c_conn_cc_out_lpf_30_20(cJSON *params);
static char* c_conn_cc_out_lpf_60_40(cJSON *params);
static char* c_conn_cc_out_lpf_80(cJSON *params);
static char* c_conn_cc_out_lpf_160(cJSON *params);
static char* c_conn_cc_out_lpf_6(cJSON *params);
static char* c_conn_cc_out_lpf_12_10(cJSON *params);
static char* c_conn_cc_out_lpf_17_15(cJSON *params);
static char* c_conn_cc_out_hpf_13(cJSON *params);
static char* c_conn_cc_out_hpf_20(cJSON *params);
static char* c_conn_cc_out_hpf_9_5(cJSON *params);
static char* c_conn_cc_out_hpf_6_5(cJSON *params);
static char* c_conn_cc_out_hpf_1_5(cJSON *params);
// DSP functions
static char* c_conn_make_wisdom(cJSON *params);
static char* c_conn_set_disp_period(cJSON *params);
static char* c_conn_set_disp_state(cJSON *params);
static char* c_conn_set_rx_1_mode(cJSON *params);
static char* c_conn_set_rx_2_mode(cJSON *params);
static char* c_conn_set_rx_3_mode(cJSON *params);
static char* c_conn_set_tx_mode(cJSON *params);
static char* c_conn_set_rx_1_agc(cJSON *params);
static char* c_conn_set_rx_2_agc(cJSON *params);
static char* c_conn_set_rx_3_agc(cJSON *params);
static char* c_conn_set_rx_1_gain(cJSON *params);
static char* c_conn_set_rx_2_gain(cJSON *params);
static char* c_conn_set_rx_3_gain(cJSON *params);
static char* c_conn_adjust_filter(int inst, int low, int high, int*new_low, int*new_high);
static char* c_conn_set_rx_1_filter(cJSON *params);
static do_rx_1_filter(int inst, int low, int high);
static char* c_conn_set_rx_2_filter(cJSON *params);
static do_rx_2_filter(int inst, int low, int high);
static char* c_conn_set_rx_3_filter(cJSON *params);
static do_rx_3_filter(int inst, int low, int high);
static char* c_conn_set_tx_filter(cJSON *params);


//==========================================================================================
// The socket
int connector_socket;
// Threads
pthread_t conn_udp_thd;
// Structure pointers
UDPConnThreadData *udp_conn_td = NULL;
// Our address structure
struct sockaddr_in conn_srv_addr;
// Client address structure
struct sockaddr_in conn_cli_addr;

// Receive data packet
char data_in[CONN_DATA_SZ];

// Last mode and filter
int last_mode[3];
int last_filter[3][2];

//==========================================================================================
// Dispatcher table
// A dictionary entry for each command. The order of these is most used first as the lookup is 
// pretty inefficient.
typedef char*(*FNPOINT)(cJSON *);
typedef struct { char* str; FNPOINT f; }stringToFunc;
stringToFunc funcCases[] =
{
	{ "poll",				c_conn_poll },
	{ "set_rx1_freq",		c_conn_cc_out_set_rx_1_freq },
	{ "set_rx2_freq",		c_conn_cc_out_set_rx_2_freq },
	{ "set_rx3_freq",		c_conn_cc_out_set_rx_3_freq },
	{ "set_tx_freq",		c_conn_cc_out_set_tx_freq },
	{ "set_rx1_mode",		c_conn_set_rx_1_mode },
	{ "set_rx2_mode",		c_conn_set_rx_2_mode },
	{ "set_rx3_mode",		c_conn_set_rx_3_mode },
	{ "set_tx_mode",		c_conn_set_tx_mode },
	{ "set_rx1_filter",		c_conn_set_rx_1_filter },
	{ "set_rx2_filter",		c_conn_set_rx_2_filter },
	{ "set_rx3_filter",		c_conn_set_rx_3_filter },
	{ "set_tx_filter",		c_conn_set_tx_filter },
	{ "set_rx1_agc",		c_conn_set_rx_1_agc },
	{ "set_rx2_agc",		c_conn_set_rx_2_agc },
	{ "set_rx3_agc",		c_conn_set_rx_3_agc },
	{ "set_rx1_gain",		c_conn_set_rx_1_gain },
	{ "set_rx2_gain",		c_conn_set_rx_2_gain },
	{ "set_rx3_gain",		c_conn_set_rx_3_gain },
	{ "set_in_rate",		c_conn_set_in_rate },
	{ "set_out_rate",		c_conn_set_out_rate },
	{ "set_iq_blk_sz",		c_conn_set_iq_blk_sz },
	{ "set_mic_blk_sz",		c_conn_set_mic_blk_sz },
	{ "set_duplex",			c_conn_set_duplex },
	{ "set_fft_size",		c_conn_set_fft_size },
	{ "set_window_type",	c_conn_set_window_type },
	{ "set_av_mode",		c_conn_set_av_mode },
	{ "set_display_width",	c_conn_set_display_width },
	{ "set_audio_route",	c_conn_set_audio_route },
	{ "server_start",		c_conn_server_start },
	{ "terminate",			c_conn_server_terminate },
	{ "radio_discover",		c_conn_radio_discover },
	{ "radio_start",		c_conn_radio_start },
	{ "radio_stop",			c_conn_radio_stop },
	{ "wisdom",				c_conn_make_wisdom },
	{ "enum_inputs",		c_conn_enum_audio_inputs },
	{ "enum_outputs",		c_conn_enum_audio_outputs },
	{ "change_outputs",		c_conn_change_audio_outputs },
	{ "revert_outputs",		c_conn_revert_audio_outputs },
	{ "local_audio_run",	c_conn_local_audio_run },
	{ "clear_audio_routes",	c_conn_clear_audio_routes },
	{ "restart_audio_routes", c_conn_restart_audio_routes },
	{ "set_disp_period",	c_conn_set_disp_period },
	{ "set_disp_state",		c_conn_set_disp_state },
	{ "set_num_rx",			c_conn_set_num_rx },
	{ "set_hf_pre",			c_conn_cc_out_set_hf_pre },
	{ "set_attn",			c_conn_cc_out_set_attn },
	{ "set_alex_auto",		c_conn_cc_out_alex_auto },
	{ "set_hf_bypass",		c_conn_cc_out_alex_hpf_bypass },
	{ "set_lpf_30_20",		c_conn_cc_out_lpf_30_20 },
	{ "set_lpf_60_40",		c_conn_cc_out_lpf_60_40 },
	{ "set_lpf_80",			c_conn_cc_out_lpf_80 },
	{ "set_lpf_160",		c_conn_cc_out_lpf_160 },
	{ "set_lpf_6",			c_conn_cc_out_lpf_6 },
	{ "set_lpf_12_10",		c_conn_cc_out_lpf_12_10 },
	{ "set_lpf_17_15",		c_conn_cc_out_lpf_17_15 },
	{ "set_hpf_13",			c_conn_cc_out_hpf_13 },
	{ "set_hpf_20",			c_conn_cc_out_hpf_20 },
	{ "set_hpf_9_5",		c_conn_cc_out_hpf_9_5 },
	{ "set_hpf_6_5",		c_conn_cc_out_hpf_6_5 },
	{ "set_hpf_1_5",		c_conn_cc_out_hpf_1_5 },
};
#define MAX_CASES 61

// Json structures
cJSON *root;
cJSON *params;

//==========================================================================================
// Initialise module
int conn_udp_init() {

	int i, rc;
	
	// Create our UDP socket
	connector_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (connector_socket == INVALID_SOCKET) {
		printf("Connector: Failed to create UDP socket! [%d, %d]", connector_socket, WSAGetLastError());
		exit(-1);
	}

	// Bind local server port
	conn_srv_addr.sin_family = AF_INET;
	if ((rc=inet_pton(AF_INET, SVR_ADDR, &(conn_srv_addr.sin_addr))) != 1) {
		if( rc == 0)
			printf("Connector: InetPton says invalid IPv4 dotted decimal address\n");
		else if( rc == -1 ) 
			printf("Connector: InetPton error [%d]\n", WSAGetLastError());
		exit(-1);
	}
	conn_srv_addr.sin_port = htons(CONN_SERVER_PORT);
	rc = bind(connector_socket, (struct sockaddr *) &conn_srv_addr, sizeof(conn_srv_addr));
	if (rc != 0) {
		printf("Connector: Cannot bind port number [%d, %d]\n", CONN_SERVER_PORT, WSAGetLastError());
		exit(-1);
	}

	// Allocate thread data structure
	udp_conn_td = (ThreadData *)safealloc(sizeof(UDPConnThreadData), sizeof(char), "CONN_UDP_TD_STRUCT");
	// Init the thread data
	udp_conn_td->run = FALSE;
	udp_conn_td->terminate = FALSE;
	udp_conn_td->socket = connector_socket;
	udp_conn_td->srv_addr = &conn_srv_addr;

	// Create the connector thread
	rc = pthread_create(&conn_udp_thd, NULL, udp_conn_imp, (void *)udp_conn_td);
	if (rc != 0) {
		printf("Connector: Failed to create connector thread! [%d]\n", rc);
		exit(-1);
	}

	// Set defaults for mode and filter
	for (i = 0; i < 3; i++) {
		last_mode[i] = 0;
		last_filter[i][0] = 300;
		last_filter[i][1] = 2700;
	}
	
	printf("Connector: Initialised Connector UDP interface\n");
}

//==========================================================================================
// Connector UDP thread

// Start connector thread
void conn_udp_start() {
	udp_conn_td->run = TRUE;
}

// Stop reader thread
void conn_udp_stop() {
	udp_conn_td->run = FALSE;
}

// Terminate the reader
void conn_udp_terminate() {
	/* Terminate reader thread
	*
	* Arguments:
	*
	*/

	int counter;

	udp_conn_td->run = FALSE;
	udp_conn_td->terminate = TRUE;

	// Signal the thread to ensure it sees the terminate
	// Wait for the thread to exit
	pthread_join(conn_udp_thd, NULL);

	// Free thread data
	safefree((char *)udp_conn_td);
}

// Thread entry point for processing
static void *udp_conn_imp(void* data) {
	// Get our thread parameters
	UDPConnThreadData* td = (UDPConnThreadData*)data;

	printf("Connector: Started UDP Connector thread\n");

	while (!td->terminate) {
		if (td->run && !td->terminate) {
			// While running we stay in the receive loop
			udpconndata(td);
		}
		else {
			Sleep(100);
		}
	}

	printf("Connector: UDP Connector thread exiting...\n");
	return NULL;
}

//==========================================================================================
// UDP Reader
static void udpconndata(UDPConnThreadData* td) {

	// Extract thread data
	int sd = td->socket;
	struct sockaddr_in *srv_addr = td->srv_addr;

	// Local vars
	int svr_addr_sz = sizeof(*srv_addr);
	fd_set read_fd;
	FD_ZERO(&read_fd);
	FD_CLR(0, &read_fd);
	FD_SET(sd, &read_fd);
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	int sel_result;
	int rd_sz;
	int cli_addr_sz = sizeof(conn_cli_addr);

	// Json vars
	char name[30];

	// Loop receiving commands from client
	printf("Connector: Waiting for commands...\n");
	while (td->run && !td->terminate) {
		// Wait for data available
		sel_result = select(0, &read_fd, NULL, NULL, &tv);
		if (sel_result == 0) {
			// Timeout to check for termination etc
			// This is not an error, it's expected but we have to reset the read fd's
			// otherwise it continually returns SOCKET_ERROR thereafter.
			// printf("Connector: Timeout\n");
			FD_SET(sd, &read_fd);
		}
		else if (sel_result == SOCKET_ERROR) {
			// Problem
			printf("Connector: Error in select! [%d]\n", WSAGetLastError());
			FD_SET(sd, &read_fd);
			// See if it goes away
			Sleep(5000);
		}
		else {
			// We have a command packet
			// Read a frame size data packet
			rd_sz = recvfrom(sd, (char*)data_in, CONN_DATA_SZ, 0, (struct sockaddr*)&conn_cli_addr, &cli_addr_sz);
			if (rd_sz > 0) {
				//char buffer[20];
				//inet_ntop(AF_INET, &(conn_cli_addr.sin_addr), buffer, 20);
				//printf("Connector: Client addr: %s, %d\n", buffer, conn_cli_addr.sin_port);
				// Data is in Json encoding
				// Data format is of the following form:
				//	{
				//		"cmd" : "cmd_name",
				//		"params" : [list of command specific parameters]
				//	}
				// We only need to extract the cmd_name and look it up in the function pointer table then
				// pass it to the function to unpack the parameters and call the server side function.
				// This is not an RPC system. Calls are one way and if they fail then the client will
				// receive an async NAK with a reason. There is no ACK response. Data that eminates from 
				// server side, primarily display data is sent as and whan available or on a timer.
				//
				// Parse the incoming data
				root = cJSON_Parse(data_in);
				params = cJSON_GetObjectItemCaseSensitive(root, "params");
				// Extract command name
				strcpy_s(name, 30, cJSON_GetObjectItemCaseSensitive(root, "cmd")->valuestring);
				// Dispatch
				// printf("Cmd: %s\n", name);
				for (int i = 0; i < MAX_CASES; i++) {
					if (strcmp(funcCases[i].str, name) == 0) {
						char* resp = (funcCases[i].f)(params);
						send_conn_resp(sd, (struct sockaddr*)&conn_cli_addr, resp);
						break;
					}
				}
			}
		}
	}
}

//==========================================================================================
// UDP Writer
static void send_conn_resp(int sd, struct sockaddr* conn_cli_addr, char* resp) {
	int cli_addr_sz = sizeof(*conn_cli_addr);
	if (sendto(sd, (const char*)resp, strlen(resp), 0, conn_cli_addr, &cli_addr_sz) == SOCKET_ERROR )
		printf("Connector: Failed to write response! [%d]\n", WSAGetLastError());
}

//==========================================================================================
// Execution functions
// These extract the param list items and call the server function
// Returning only ACK/NAK
static char* c_conn_poll(cJSON *params) {
	/*
	** Arguments:
	*/
	return encode_ack_nak("ACK");
}

static char* c_conn_set_num_rx(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	num rx
	*/
	c_server_set_num_rx(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_in_rate(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	input sample rate
	*/
	c_server_set_in_rate(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_out_rate(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	output sample rate
	*/
	c_server_set_out_rate(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_iq_blk_sz(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	iq block size
	*/
	c_server_set_iq_blk_sz(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_mic_blk_sz(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	mic block size
	*/
	c_server_set_mic_blk_sz(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_duplex(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	duplex flag
	*/
	c_server_set_duplex(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_fft_size(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- fft size for display
	*/
	c_server_set_fft_size(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_window_type(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	window type for display
	*/
	c_server_set_window_type(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_av_mode(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	average mode for display
	*/
	c_server_set_av_mode(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_display_width(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	display width
	*/
	int width = cJSON_GetArrayItem(params, 0)->valueint;
	c_server_set_display_width(width);
	conn_set_disp_width(width);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_audio_route(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	direction
	** 	p1		-- 	location
	** 	p2		-- 	receiver id
	** 	p3		-- 	host_api
	** 	p4		-- 	device name
	** 	p5		-- 	channel L/R/B
	*/
	char location[10];
	char host_api[50];
	char dev[50];
	char channel[10];
	int direction = cJSON_GetArrayItem(params, 0)->valueint;
	strcpy_s (location, 10, cJSON_GetArrayItem(params, 1)->valuestring);
	int receiver = cJSON_GetArrayItem(params, 2)->valueint;
	strcpy_s(host_api, 50, cJSON_GetArrayItem(params, 3)->valuestring);
	strcpy_s(dev, 50, cJSON_GetArrayItem(params, 4)->valuestring);
	strcpy_s(channel, 10, cJSON_GetArrayItem(params, 5)->valuestring);
	// printf("Route: %d,%s,%d,%s,%s,%s", direction, location, receiver, host_api, dev, channel);
	c_server_set_audio_route(direction, location, receiver, host_api, dev, channel);
	return encode_ack_nak("ACK");
}

static char* c_conn_server_start(cJSON *params) {
	/*
	** Arguments:
	*/
	if (c_server_start())
		return encode_ack_nak("ACK");
	else
		return encode_ack_nak("NAK");
}

static char* c_conn_server_terminate(cJSON *params) {
	/*
	** Arguments:
	*/
	c_server_terminate();
}

static char* c_conn_radio_discover(cJSON *params) {
	/*
	** Arguments:
	*/
	if (c_radio_discover())
		return encode_ack_nak("ACK");
	else
		return encode_ack_nak("NAK");
}

static char* c_conn_radio_start(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	WBS flag
	*/
	if (c_radio_start(cJSON_GetArrayItem(params, 0)->valueint))
		return encode_ack_nak("ACK");
	else
		return encode_ack_nak("NAK");
}

static char* c_conn_radio_stop(cJSON *params) {
	/*
	** Arguments:
	*/
	if (c_radio_stop())
		return encode_ack_nak("ACK");
	else
		return encode_ack_nak("NAK");
}

static char* c_conn_cc_out_set_rx_1_freq(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	freq in Hz
	*/
	c_server_cc_out_set_rx_1_freq(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_set_rx_2_freq(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	freq in Hz
	*/
	c_server_cc_out_set_rx_2_freq(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_set_rx_3_freq(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	freq in Hz
	*/
	c_server_cc_out_set_rx_3_freq(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_set_tx_freq(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	freq in Hz
	*/
	c_server_cc_out_set_tx_freq(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_set_hf_pre(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	on/off
	*/
	c_server_cc_out_preamp(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_set_attn(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	Attenuator value
	*/
	c_server_cc_out_alex_attn(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}


static char* c_conn_cc_out_alex_auto(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	auto on/off
	*/
	c_server_cc_out_alex_auto(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_alex_hpf_bypass(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	hf bypass on/off
	*/
	c_server_cc_out_alex_hpf_bypass(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_lpf_30_20(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	30/20 lpf on/off
	*/
	c_server_cc_out_lpf_30_20(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_lpf_60_40(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	60/40 lpf on/off
	*/
	c_server_cc_out_lpf_60_40(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_lpf_80(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	80 fpf on/off
	*/
	c_server_cc_out_lpf_80(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_lpf_160(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	160 lpf on/off
	*/
	c_server_cc_out_lpf_160(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_lpf_6(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	6 lpf on/off
	*/
	c_server_cc_out_lpf_6(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_lpf_12_10(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	12/10 lpf on/off
	*/
	c_server_cc_out_lpf_12_10(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_lpf_17_15(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	17/15 on/off
	*/
	c_server_cc_out_lpf_17_15(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_hpf_13(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	13 hpf on/off
	*/
	c_server_cc_out_hpf_13(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_hpf_20(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	20 hpf on/off
	*/
	c_server_cc_out_hpf_20(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_hpf_9_5(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	9/5 hpf on/off
	*/
	c_server_cc_out_hpf_9_5(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_hpf_6_5(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	6/5 hpf on/off
	*/
	c_server_cc_out_hpf_6_5(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_cc_out_hpf_1_5(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	1/5 hpf on/off
	*/
	c_server_cc_out_hpf_1_5(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_make_wisdom(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	directory
	*/
	c_server_make_wisdom(cJSON_GetArrayItem(params, 0)->valuestring);
	return encode_ack_nak("ACK");
}

// Command/response calls
static char* c_conn_enum_audio_inputs(cJSON *params) {
	/*
	** Arguments:
	*/

	int i;
	cJSON *root;
	cJSON *inputs;
	cJSON *items;

	DeviceEnumList* audio_inputs = c_server_enum_audio_inputs();
	/*
		Definition : -
		typedef struct DeviceEnum {
			int direction;
			int index;
			char name[50];
			int channels;
			char host_api[50];
		}DeviceEnum;

		typedef struct DeviceEnumList {
			int entries;
			DeviceEnum devices[50];
		}DeviceEnumList;
	*/

	// Create the Json root object
	root = cJSON_CreateObject();
	// Add an array to hold the enumerations
	cJSON_AddItemToObject(root, "inputs", inputs = cJSON_CreateArray());

	// Iterate the list and populate the Json structure
	for (i = 0; i<audio_inputs->entries; i++) {
		// Create an object to add items to
		items = cJSON_CreateObject();
		cJSON_AddStringToObject(items, "name", audio_inputs->devices[i].name);
		cJSON_AddStringToObject(items, "api", audio_inputs->devices[i].host_api);
		cJSON_AddNumberToObject(items, "index", audio_inputs->devices[i].index);
		cJSON_AddNumberToObject(items, "direction", audio_inputs->devices[i].direction);
		cJSON_AddNumberToObject(items, "channels", audio_inputs->devices[i].channels);
		cJSON_AddItemToArray(inputs, items);
	}
	return (cJSON_Print(root));
}

static char* c_conn_enum_audio_outputs(cJSON *params) {
	/*
	** Arguments:
	*/

	int i;
	cJSON *root;
	cJSON *outputs;
	cJSON *items;

	DeviceEnumList* audio_outputs = c_server_enum_audio_outputs();
	/*
	Definition : -
	typedef struct DeviceEnum {
		int direction;
		int index;
		char name[50];
		int channels;
		char host_api[50];
	}DeviceEnum;

	typedef struct DeviceEnumList {
		int entries;
		DeviceEnum devices[50];
	}DeviceEnumList;
	*/

	// Create the Json root object
	root = cJSON_CreateObject();
	// Add an array to hold the enumerations
	cJSON_AddItemToObject(root, "outputs", outputs = cJSON_CreateArray());

	// Iterate the list and populate the Json structure
	for (i = 0; i < audio_outputs->entries; i++) {
		// Create an object to add items to
		items = cJSON_CreateObject();
		cJSON_AddStringToObject(items, "name", audio_outputs->devices[i].name);
		cJSON_AddStringToObject(items, "api", audio_outputs->devices[i].host_api);
		cJSON_AddNumberToObject(items, "index", audio_outputs->devices[i].index);
		cJSON_AddNumberToObject(items, "direction", audio_outputs->devices[i].direction);
		cJSON_AddNumberToObject(items, "channels", audio_outputs->devices[i].channels);
		cJSON_AddItemToArray(outputs, items);
	}
	return (cJSON_Print(root));
}

static char* c_conn_change_audio_outputs(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	rx
	**	p1		--	audio-ch
	*/
	c_server_change_audio_outputs(cJSON_GetArrayItem(params, 0)->valueint, cJSON_GetArrayItem(params, 1)->valuestring);
	return encode_ack_nak("ACK");
}

static char* c_conn_revert_audio_outputs(cJSON *params) {
	/*
	** Arguments:
	*/
	c_server_revert_audio_outputs();
	return encode_ack_nak("ACK");
}

static char* c_conn_local_audio_run(cJSON *params) {
	/*
	** Arguments:
	*/
	if (c_server_local_audio_run(cJSON_GetArrayItem(params, 0)->valueint))
		return encode_ack_nak("ACK");
	else
		return encode_ack_nak("NAK");
}

static char* c_conn_clear_audio_routes(cJSON *params) {
	/*
	** Arguments:
	*/
	if (c_server_clear_audio_routes())
		return encode_ack_nak("ACK");
	else
		return encode_ack_nak("NAK");
}

static char* c_conn_restart_audio_routes(cJSON *params) {
	/*
	** Arguments:
	*/
	if (c_server_restart_audio_routes())
		return encode_ack_nak("ACK");
	else
		return encode_ack_nak("NAK");
}

static char* c_conn_set_disp_period(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	period
	*/
	conn_set_disp_period(cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_disp_state(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	TRUE - disp_1
	** 	p1		-- 	TRUE - disp_2
	** 	p2		-- 	TRUE - disp_3
	*/
	int state;
	state = cJSON_GetArrayItem(params, 0)->valueint;
	if (state) {
		conn_disp_1_udp_start();
	}
	else
		conn_disp_1_udp_stop();
	state = cJSON_GetArrayItem(params, 1)->valueint;
	if (state) {
		conn_disp_2_udp_start();
	}
	else
		conn_disp_2_udp_stop();
	state = cJSON_GetArrayItem(params, 2)->valueint;
	if (state) {
		conn_disp_3_udp_start();
	}
	else
		conn_disp_3_udp_stop();
	return encode_ack_nak("ACK");
}

static char* c_conn_set_rx_1_mode(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	mode id
	*/
	int mode = cJSON_GetArrayItem(params, 0)->valueint;
	last_mode[0] = mode;
	c_server_set_rx_mode(0, mode);
	do_rx_1_filter(0, last_filter[0][0], last_filter[0][1]);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_rx_2_mode(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	mode id
	*/
	int mode = cJSON_GetArrayItem(params, 0)->valueint;
	last_mode[1] = mode;
	c_server_set_rx_mode(1, mode);
	do_rx_2_filter(1, last_filter[1][0], last_filter[1][1]);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_rx_3_mode(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	mode id
	*/
	c_server_set_rx_mode(2, cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");

	int mode = cJSON_GetArrayItem(params, 0)->valueint;
	last_mode[2] = mode;
	c_server_set_rx_mode(2, mode);
	do_rx_3_filter(2, last_filter[2][0], last_filter[2][1]);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_tx_mode(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	mode id
	*/
	c_server_set_tx_mode(0, cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_adjust_filter(int inst, int low, int high, int*new_low, int*new_high) {
	int mode = last_mode[inst];
	if (mode == 0 || mode == 3 || mode == 9) {
		// LSB
		*new_low = -high;
		*new_high = -low;
	}
	else if (mode == 2 || mode == 5 || mode == 6 || mode == 8 || mode == 10 || mode == 11) {
		// LSB+USB
		*new_low = -high;
		*new_high = high;
	}
	else {
		// USB
		*new_low = low;
		*new_high = high;
	}
}

static char* c_conn_set_rx_1_filter(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	filter low
	** 	p1		-- 	filter high
	*/
	int low = cJSON_GetArrayItem(params, 0)->valueint;
	int high = cJSON_GetArrayItem(params, 1)->valueint;

	do_rx_1_filter(0, low, high);
	return encode_ack_nak("ACK");
}

static do_rx_1_filter(int inst, int low, int high) {
	int new_low;
	int new_high;

	// Adjust filter for mode and save filter
	c_conn_adjust_filter(0, low, high, &new_low, &new_high);
	last_filter[0][0] = low;
	last_filter[0][1] = high;

	c_server_set_rx_filter_freq(0, new_low, new_high);
}

static char* c_conn_set_rx_2_filter(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	filter low
	** 	p1		-- 	filter high
	*/
	int low = cJSON_GetArrayItem(params, 0)->valueint;
	int high = cJSON_GetArrayItem(params, 1)->valueint;

	do_rx_2_filter(1, low, high);
	return encode_ack_nak("ACK");
}

static do_rx_2_filter(int inst, int low, int high) {
	int new_low;
	int new_high;

	// Adjust filter for mode and save filter
	c_conn_adjust_filter(1, low, high, &new_low, &new_high);
	last_filter[1][0] = low;
	last_filter[1][1] = high;

	c_server_set_rx_filter_freq(1, new_low, new_high);
}

static char* c_conn_set_rx_3_filter(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	filter low
	** 	p1		-- 	filter high
	*/
	int low = cJSON_GetArrayItem(params, 0)->valueint;
	int high = cJSON_GetArrayItem(params, 1)->valueint;

	do_rx_3_filter(2, low, high);
	return encode_ack_nak("ACK");
}

static do_rx_3_filter(int inst, int low, int high) {
	int new_low;
	int new_high;

	// Adjust filter for mode and save filter
	c_conn_adjust_filter(2, low, high, &new_low, &new_high);
	last_filter[2][0] = low;
	last_filter[2][1] = high;

	c_server_set_rx_filter_freq(2, new_low, new_high);
}

static char* c_conn_set_tx_filter(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	filter low
	** 	p1		-- 	filter high
	*/
	c_server_set_tx_filter_freq(0, cJSON_GetArrayItem(params, 0)->valueint, cJSON_GetArrayItem(params, 1)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_rx_1_agc(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	AGC mode
	*/
	c_server_set_agc_mode(0, cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_rx_2_agc(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	AGC mode
	*/
	c_server_set_agc_mode(1, cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_rx_3_agc(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	AGC mode
	*/
	c_server_set_agc_mode(2, cJSON_GetArrayItem(params, 0)->valueint);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_rx_1_gain(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	RX gain
	*/
	c_server_set_rx_gain(0, (float)cJSON_GetArrayItem(params, 0)->valuedouble);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_rx_2_gain(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	RX gain
	*/
	c_server_set_rx_gain(1, (float)cJSON_GetArrayItem(params, 0)->valuedouble);
	return encode_ack_nak("ACK");
}

static char* c_conn_set_rx_3_gain(cJSON *params) {
	/*
	** Arguments:
	** 	p0		-- 	RX gain
	*/
	c_server_set_rx_gain(2, (float)cJSON_GetArrayItem(params, 0)->valuedouble);
	return encode_ack_nak("ACK");
}

//==========================================================================================
// Helper functions
static char* encode_ack_nak(char* data) {
	cJSON *root;
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "resp", data);
	return cJSON_Print(root);
}