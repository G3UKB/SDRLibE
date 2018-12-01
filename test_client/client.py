import json
import socket
import traceback
import pprint
pp = pprint.PrettyPrinter(indent=4)
from _thread import start_new_thread
from time import sleep

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.settimeout(5)
sock1 = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
#sock1.settimeout(5)
sock1.bind(("localhost", 10011))

radio_discover = {
	"cmd" : "radio_discover",
	"params" : []
}
enum_outputs = {
	"cmd" : "enum_outputs",
	"params" : []
}
set_audio_route = {
	"cmd" : "set_audio_route",
	"params" : []
}
server_start = {
	"cmd" : "server_start",
	"params" : []
}
radio_start = {
	"cmd" : "radio_start",
	"params" : [0]
}
disp_status = {
	"cmd" : "set_disp_status",
	"params" : [True,False,False]
}
data = bytearray(4096)
data1 = bytearray(7680)

def do_send(cmd):
	sock.sendto(bytes(json.dumps(cmd), 'UTF-8'), 0, ("localhost", 10010))	
	
def do_receive(name):
	nbytes, address = sock.recvfrom_into(data)
	if nbytes > 0 :
		print("Resp:", name)
		return json.loads(data.decode('UTF-8')[:nbytes])

def set_audio():
	# Get the audio outputs
	do_send (enum_outputs)
	resp = do_receive("enum_outputs")
	# Get the speaker out parameters
	d = resp["outputs"][1]
	# Set parameters
	p = set_audio_route["params"]
	p.append(1)         # Direction
	p.append("LOCAL")   # Location
	p.append(1)         # RX 1
	p.append(d["api"])  # Audio API
	p.append(d["name"]) # Audio device
	p.append("BOTH")    # Both channels
	set_audio_route["params"] = p
	do_send(set_audio_route)
	print(do_receive("set_audio_route"))

def display() :
    do_send(disp_status)
    print(do_receive("disp_status"))
    sleep(0.2)
    while(1):
        nbytes, address = sock1.recvfrom_into(data1)
        if nbytes > 0 :
            print("Data:", nbytes)
        else:
            sleep(0.1)

def script():
	do_send (radio_discover)
	print(do_receive("radio_discover"))
	set_audio()
	do_send (server_start)
	print(do_receive("server_start"))
	do_send (radio_start)
	print(do_receive("radio_start"))
	start_new_thread(display, ())
	input("Press Enter to exit...")


	
#======================================================================================================================
# Main code
def main():
	
	try:
		script()
	except Exception as e:
		print ('Exception [%s][%s]' % (str(e), traceback.format_exc()))

# Entry point       
if __name__ == '__main__':
	main()